/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the CLI interpreter.
 */

#include "cli.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/diag.h>
#include <openthread/dns.h>
#include <openthread/icmp6.h>
#include <openthread/link.h>
#include <openthread/logging.h>
#include <openthread/ncp.h>
#include <openthread/thread.h>
#include <openthread/platform/uart.h>
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#include <openthread/network_time.h>
#endif
#if OPENTHREAD_FTD
#include <openthread/dataset_ftd.h>
#include <openthread/thread_ftd.h>
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#include <openthread/border_router.h>
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
#include <openthread/server.h>
#endif
#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
#include <openthread/child_supervision.h>
#endif
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
#include <openthread/platform/misc.h>
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#include <openthread/backbone_router.h>
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
#include <openthread/backbone_router_ftd.h>
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
#include <openthread/link_metrics.h>
#endif
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
#include <openthread/channel_manager.h>
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
#include <openthread/channel_monitor.h>
#endif
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
#include <openthread/platform/debug_uart.h>
#endif

#include "common/encoding.hpp"
#include "common/new.hpp"
#include "common/string.hpp"
#include "mac/channel_mask.hpp"
#include "net/ip6.hpp"
#include "utils/otns.hpp"
#include "utils/parse_cmdline.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

using ot::Utils::CmdLineParser::ParseAsBool;
using ot::Utils::CmdLineParser::ParseAsHexString;
using ot::Utils::CmdLineParser::ParseAsInt8;
using ot::Utils::CmdLineParser::ParseAsIp6Address;
using ot::Utils::CmdLineParser::ParseAsIp6Prefix;
using ot::Utils::CmdLineParser::ParseAsUint16;
using ot::Utils::CmdLineParser::ParseAsUint32;
using ot::Utils::CmdLineParser::ParseAsUint64;
using ot::Utils::CmdLineParser::ParseAsUint8;

namespace ot {
namespace Cli {

constexpr Interpreter::Command Interpreter::sCommands[];

Interpreter *Interpreter::sInterpreter = nullptr;

Interpreter::Interpreter(Instance *aInstance)
    : mInstance(aInstance)
    , mUserCommands(nullptr)
    , mUserCommandsLength(0)
    , mPingLength(kDefaultPingLength)
    , mPingCount(kDefaultPingCount)
    , mPingInterval(kDefaultPingInterval)
    , mPingHopLimit(0)
    , mPingAllowZeroHopLimit(false)
    , mPingIdentifier(0)
    , mPingTimer(*aInstance, Interpreter::HandlePingTimer)
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    , mSntpQueryingInProgress(false)
#endif
    , mDataset(*this)
    , mNetworkData(*this)
    , mUdp(*this)
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    , mCoap(*this)
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    , mCoapSecure(*this)
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    , mCommissioner(*this)
#endif
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    , mJoiner(*this)
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    , mSrpClient(*this)
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    , mSrpServer(*this)
#endif
{
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    otThreadSetReceiveDiagnosticGetCallback(mInstance, &Interpreter::HandleDiagnosticGetResponse, this);
#endif
#if OPENTHREAD_FTD
    otThreadSetDiscoveryRequestCallback(mInstance, &Interpreter::HandleDiscoveryRequest, this);
#endif

    mIcmpHandler.mReceiveCallback = Interpreter::HandleIcmpReceive;
    mIcmpHandler.mContext         = this;
    IgnoreError(otIcmp6RegisterHandler(mInstance, &mIcmpHandler));
}

void Interpreter::OutputResult(otError aError)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        OutputLine("Done");
        break;

    case OT_ERROR_PENDING:
        break;

    default:
        OutputLine("Error %d: %s", aError, otThreadErrorToString(aError));
    }
}

void Interpreter::OutputBytes(const uint8_t *aBytes, uint16_t aLength)
{
    for (uint16_t i = 0; i < aLength; i++)
    {
        OutputFormat("%02x", aBytes[i]);
    }
}

void Interpreter::OutputEnabledDisabledStatus(bool aEnabled)
{
    OutputLine(aEnabled ? "Enabled" : "Disabled");
}

int Interpreter::OutputIp6Address(const otIp6Address &aAddress)
{
    return OutputFormat(
        "%x:%x:%x:%x:%x:%x:%x:%x", HostSwap16(aAddress.mFields.m16[0]), HostSwap16(aAddress.mFields.m16[1]),
        HostSwap16(aAddress.mFields.m16[2]), HostSwap16(aAddress.mFields.m16[3]), HostSwap16(aAddress.mFields.m16[4]),
        HostSwap16(aAddress.mFields.m16[5]), HostSwap16(aAddress.mFields.m16[6]), HostSwap16(aAddress.mFields.m16[7]));
}

otError Interpreter::ParseJoinerDiscerner(char *aString, otJoinerDiscerner &aDiscerner)
{
    otError error     = OT_ERROR_NONE;
    char *  separator = strstr(aString, "/");

    VerifyOrExit(separator != nullptr, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = ParseAsUint8(separator + 1, aDiscerner.mLength));
    VerifyOrExit(aDiscerner.mLength > 0 && aDiscerner.mLength <= 64, error = OT_ERROR_INVALID_ARGS);
    *separator = '\0';
    error      = ParseAsUint64(aString, aDiscerner.mValue);

exit:
    return error;
}

otError Interpreter::ParsePingInterval(const char *aString, uint32_t &aInterval)
{
    otError        error    = OT_ERROR_NONE;
    const uint32_t msFactor = 1000;
    uint32_t       factor   = msFactor;

    aInterval = 0;

    while (*aString)
    {
        if ('0' <= *aString && *aString <= '9')
        {
            // In the case of seconds, change the base of already calculated value.
            if (factor == msFactor)
            {
                aInterval *= 10;
            }

            aInterval += static_cast<uint32_t>(*aString - '0') * factor;

            // In the case of milliseconds, change the multiplier factor.
            if (factor != msFactor)
            {
                factor /= 10;
            }
        }
        else if (*aString == '.')
        {
            // Accept only one dot character.
            VerifyOrExit(factor == msFactor, error = OT_ERROR_INVALID_ARGS);

            // Start analyzing hundreds of milliseconds.
            factor /= 10;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        aString++;
    }

exit:
    return error;
}

otError Interpreter::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        OutputLine(command.mName);
    }

    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        OutputLine("%s", mUserCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError Interpreter::ProcessBorderRouting(uint8_t aArgsLength, char *aArgs[])
{
    otError error  = OT_ERROR_NONE;
    bool    enable = false;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "enable") == 0)
    {
        enable = true;
    }
    else if (strcmp(aArgs[0], "disable") == 0)
    {
        enable = false;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

    SuccessOrExit(error = otBorderRoutingSetEnabled(mInstance, enable));

exit:
    return error;
}
#endif

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
otError Interpreter::ProcessBackboneRouter(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    otError                error = OT_ERROR_INVALID_COMMAND;
    otBackboneRouterConfig config;

    if (aArgsLength == 0)
    {
        if (otBackboneRouterGetPrimary(mInstance, &config) == OT_ERROR_NONE)
        {
            OutputLine("BBR Primary:");
            OutputLine("server16: 0x%04X", config.mServer16);
            OutputLine("seqno:    %d", config.mSequenceNumber);
            OutputLine("delay:    %d secs", config.mReregistrationDelay);
            OutputLine("timeout:  %d secs", config.mMlrTimeout);
        }
        else
        {
            OutputLine("BBR Primary: None");
        }

        error = OT_ERROR_NONE;
    }
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    else
    {
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (strcmp(aArgs[0], "mgmt") == 0)
        {
            if (aArgsLength < 2)
            {
                ExitNow(error = OT_ERROR_INVALID_COMMAND);
            }
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
            else if (strcmp(aArgs[1], "dua") == 0)
            {
                uint8_t                   status;
                otIp6InterfaceIdentifier *mlIid = nullptr;
                otIp6InterfaceIdentifier  iid;

                VerifyOrExit((aArgsLength == 3 || aArgsLength == 4), error = OT_ERROR_INVALID_ARGS);

                SuccessOrExit(error = ParseAsUint8(aArgs[2], status));

                if (aArgsLength == 4)
                {
                    SuccessOrExit(error = ParseAsHexString(aArgs[3], iid.mFields.m8));
                    mlIid = &iid;
                }

                otBackboneRouterConfigNextDuaRegistrationResponse(mInstance, mlIid, status);
                ExitNow();
            }
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
            else if (strcmp(aArgs[1], "mlr") == 0)
            {
                error = ProcessBackboneRouterMgmtMlr(aArgsLength - 2, aArgs + 2);
                ExitNow();
            }
#endif
        }
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        SuccessOrExit(error = ProcessBackboneRouterLocal(aArgsLength, aArgs));
    }

exit:
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
otError Interpreter::ProcessBackboneRouterMgmtMlr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    VerifyOrExit(aArgsLength >= 1);

    if (!strcmp(aArgs[0], "listener"))
    {
        if (aArgsLength == 1)
        {
            PrintMulticastListenersTable();
            error = OT_ERROR_NONE;
        }
        else if (!strcmp(aArgs[1], "clear"))
        {
            otBackboneRouterMulticastListenerClear(mInstance);
            error = OT_ERROR_NONE;
        }
        else if (!strcmp(aArgs[1], "add"))
        {
            otIp6Address address;
            uint32_t     timeout = 0;

            VerifyOrExit(aArgsLength == 3 || aArgsLength == 4, error = OT_ERROR_INVALID_ARGS);

            SuccessOrExit(error = ParseAsIp6Address(aArgs[2], address));

            if (aArgsLength == 4)
            {
                SuccessOrExit(error = ParseAsUint32(aArgs[3], timeout));
            }

            error = otBackboneRouterMulticastListenerAdd(mInstance, &address, timeout);
        }
    }
    else if (!strcmp(aArgs[0], "response"))
    {
        uint8_t status;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = ParseAsUint8(aArgs[1], status));

        otBackboneRouterConfigNextMulticastListenerRegistrationResponse(mInstance, status);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void Interpreter::PrintMulticastListenersTable(void)
{
    otBackboneRouterMulticastListenerIterator iter = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ITERATOR_INIT;
    otBackboneRouterMulticastListenerInfo     listenerInfo;

    while (otBackboneRouterMulticastListenerGetNext(mInstance, &iter, &listenerInfo) == OT_ERROR_NONE)
    {
        OutputIp6Address(listenerInfo.mAddress);
        OutputLine(" %u", listenerInfo.mTimeout);
    }
}

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

otError Interpreter::ProcessBackboneRouterLocal(uint8_t aArgsLength, char *aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otBackboneRouterConfig config;

    if (strcmp(aArgs[0], "disable") == 0)
    {
        otBackboneRouterSetEnabled(mInstance, false);
    }
    else if (strcmp(aArgs[0], "enable") == 0)
    {
        otBackboneRouterSetEnabled(mInstance, true);
    }
    else if (strcmp(aArgs[0], "jitter") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otBackboneRouterGetRegistrationJitter(mInstance));
        }
        else if (aArgsLength == 2)
        {
            uint8_t jitter;

            SuccessOrExit(error = ParseAsUint8(aArgs[1], jitter));
            otBackboneRouterSetRegistrationJitter(mInstance, jitter);
        }
    }
    else if (strcmp(aArgs[0], "register") == 0)
    {
        SuccessOrExit(error = otBackboneRouterRegister(mInstance));
    }
    else if (strcmp(aArgs[0], "state") == 0)
    {
        switch (otBackboneRouterGetState(mInstance))
        {
        case OT_BACKBONE_ROUTER_STATE_DISABLED:
            OutputLine("Disabled");
            break;
        case OT_BACKBONE_ROUTER_STATE_SECONDARY:
            OutputLine("Secondary");
            break;
        case OT_BACKBONE_ROUTER_STATE_PRIMARY:
            OutputLine("Primary");
            break;
        }
    }
    else if (strcmp(aArgs[0], "config") == 0)
    {
        otBackboneRouterGetConfig(mInstance, &config);

        if (aArgsLength == 1)
        {
            OutputLine("seqno:    %d", config.mSequenceNumber);
            OutputLine("delay:    %d secs", config.mReregistrationDelay);
            OutputLine("timeout:  %d secs", config.mMlrTimeout);
        }
        else
        {
            // Set local Backbone Router configuration.
            for (int argCur = 1; argCur < aArgsLength; argCur++)
            {
                VerifyOrExit(argCur + 1 < aArgsLength, error = OT_ERROR_INVALID_ARGS);

                if (strcmp(aArgs[argCur], "seqno") == 0)
                {
                    SuccessOrExit(error = ParseAsUint8(aArgs[++argCur], config.mSequenceNumber));
                }
                else if (strcmp(aArgs[argCur], "delay") == 0)
                {
                    SuccessOrExit(error = ParseAsUint16(aArgs[++argCur], config.mReregistrationDelay));
                }
                else if (strcmp(aArgs[argCur], "timeout") == 0)
                {
                    SuccessOrExit(error = ParseAsUint32(aArgs[++argCur], config.mMlrTimeout));
                }
                else
                {
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }

            SuccessOrExit(error = otBackboneRouterSetConfig(mInstance, &config));
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

otError Interpreter::ProcessDomainName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%s", otThreadGetDomainName(mInstance));
    }
    else
    {
        SuccessOrExit(error = otThreadSetDomainName(mInstance, aArgs[0]));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
otError Interpreter::ProcessDua(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength >= 1 && strcmp(aArgs[0], "iid") == 0, error = OT_ERROR_INVALID_COMMAND);

    switch (aArgsLength)
    {
    case 1:
    {
        const otIp6InterfaceIdentifier *iid = otThreadGetFixedDuaInterfaceIdentifier(mInstance);

        if (iid != nullptr)
        {
            OutputBytes(iid->mFields.m8, sizeof(otIp6InterfaceIdentifier));
            OutputLine("");
        }
        break;
    }
    case 2:
        if (strcmp(aArgs[1], "clear") == 0)
        {
            SuccessOrExit(error = otThreadSetFixedDuaInterfaceIdentifier(mInstance, nullptr));
        }
        else
        {
            otIp6InterfaceIdentifier iid;

            SuccessOrExit(error = ParseAsHexString(aArgs[1], iid.mFields.m8));
            SuccessOrExit(error = otThreadSetFixedDuaInterfaceIdentifier(mInstance, &iid));
        }
        break;
    default:
        error = OT_ERROR_INVALID_ARGS;
        break;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

otError Interpreter::ProcessBufferInfo(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    OutputLine("total: %d", bufferInfo.mTotalBuffers);
    OutputLine("free: %d", bufferInfo.mFreeBuffers);
    OutputLine("6lo send: %d %d", bufferInfo.m6loSendMessages, bufferInfo.m6loSendBuffers);
    OutputLine("6lo reas: %d %d", bufferInfo.m6loReassemblyMessages, bufferInfo.m6loReassemblyBuffers);
    OutputLine("ip6: %d %d", bufferInfo.mIp6Messages, bufferInfo.mIp6Buffers);
    OutputLine("mpl: %d %d", bufferInfo.mMplMessages, bufferInfo.mMplBuffers);
    OutputLine("mle: %d %d", bufferInfo.mMleMessages, bufferInfo.mMleBuffers);
    OutputLine("arp: %d %d", bufferInfo.mArpMessages, bufferInfo.mArpBuffers);
    OutputLine("coap: %d %d", bufferInfo.mCoapMessages, bufferInfo.mCoapBuffers);
    OutputLine("coap secure: %d %d", bufferInfo.mCoapSecureMessages, bufferInfo.mCoapSecureBuffers);
    OutputLine("application coap: %d %d", bufferInfo.mApplicationCoapMessages, bufferInfo.mApplicationCoapBuffers);

    return OT_ERROR_NONE;
}

otError Interpreter::ProcessCcaThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    int8_t  cca;

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = otPlatRadioGetCcaEnergyDetectThreshold(mInstance, &cca));
        OutputLine("%d dBm", cca);
    }
    else
    {
        SuccessOrExit(error = ParseAsInt8(aArgs[0], cca));
        SuccessOrExit(error = otPlatRadioSetCcaEnergyDetectThreshold(mInstance, cca));
    }

exit:
    return error;
}

otError Interpreter::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t channel;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otLinkGetChannel(mInstance));
    }
    else if (strcmp(aArgs[0], "supported") == 0)
    {
        OutputLine("0x%x", otPlatRadioGetSupportedChannelMask(mInstance));
    }
    else if (strcmp(aArgs[0], "preferred") == 0)
    {
        OutputLine("0x%x", otPlatRadioGetPreferredChannelMask(mInstance));
    }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    else if (strcmp(aArgs[0], "monitor") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("enabled: %d", otChannelMonitorIsEnabled(mInstance));
            if (otChannelMonitorIsEnabled(mInstance))
            {
                uint32_t channelMask = otLinkGetSupportedChannelMask(mInstance);
                uint8_t  channelNum  = sizeof(channelMask) * CHAR_BIT;

                OutputLine("interval: %u", otChannelMonitorGetSampleInterval(mInstance));
                OutputLine("threshold: %d", otChannelMonitorGetRssiThreshold(mInstance));
                OutputLine("window: %u", otChannelMonitorGetSampleWindow(mInstance));
                OutputLine("count: %u", otChannelMonitorGetSampleCount(mInstance));

                OutputLine("occupancies:");
                for (channel = 0; channel < channelNum; channel++)
                {
                    uint32_t occupancy = 0;

                    if (!((1UL << channel) & channelMask))
                    {
                        continue;
                    }

                    occupancy = otChannelMonitorGetChannelOccupancy(mInstance, channel);

                    OutputFormat("ch %d (0x%04x) ", channel, occupancy);
                    occupancy = (occupancy * 10000) / 0xffff;
                    OutputLine("%2d.%02d%% busy", occupancy / 100, occupancy % 100);
                }
                OutputLine("");
            }
        }
        else if (strcmp(aArgs[1], "start") == 0)
        {
            error = otChannelMonitorSetEnabled(mInstance, true);
        }
        else if (strcmp(aArgs[1], "stop") == 0)
        {
            error = otChannelMonitorSetEnabled(mInstance, false);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif // OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
    else if (strcmp(aArgs[0], "manager") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("channel: %d", otChannelManagerGetRequestedChannel(mInstance));
            OutputLine("auto: %d", otChannelManagerGetAutoChannelSelectionEnabled(mInstance));

            if (otChannelManagerGetAutoChannelSelectionEnabled(mInstance))
            {
                Mac::ChannelMask supportedMask(otChannelManagerGetSupportedChannels(mInstance));
                Mac::ChannelMask favoredMask(otChannelManagerGetFavoredChannels(mInstance));

                OutputLine("delay: %d", otChannelManagerGetDelay(mInstance));
                OutputLine("interval: %u", otChannelManagerGetAutoChannelSelectionInterval(mInstance));
                OutputLine("supported: %s", supportedMask.ToString().AsCString());
                OutputLine("favored: %s", supportedMask.ToString().AsCString());
            }
        }
        else if (strcmp(aArgs[1], "change") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsUint8(aArgs[2], channel));
            otChannelManagerRequestChannelChange(mInstance, channel);
        }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
        else if (strcmp(aArgs[1], "select") == 0)
        {
            bool enable;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsBool(aArgs[2], enable));
            error = otChannelManagerRequestChannelSelect(mInstance, enable);
        }
#endif
        else if (strcmp(aArgs[1], "auto") == 0)
        {
            bool enable;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsBool(aArgs[2], enable));
            otChannelManagerSetAutoChannelSelectionEnabled(mInstance, enable);
        }
        else if (strcmp(aArgs[1], "delay") == 0)
        {
            uint8_t delay;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsUint8(aArgs[2], delay));
            error = otChannelManagerSetDelay(mInstance, delay);
        }
        else if (strcmp(aArgs[1], "interval") == 0)
        {
            uint32_t interval;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsUint32(aArgs[2], interval));
            error = otChannelManagerSetAutoChannelSelectionInterval(mInstance, interval);
        }
        else if (strcmp(aArgs[1], "supported") == 0)
        {
            uint32_t mask;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsUint32(aArgs[2], mask));
            otChannelManagerSetSupportedChannels(mInstance, mask);
        }
        else if (strcmp(aArgs[1], "favored") == 0)
        {
            uint32_t mask;

            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsUint32(aArgs[2], mask));
            otChannelManagerSetFavoredChannels(mInstance, mask);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif // OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
    else
    {
        SuccessOrExit(error = ParseAsUint8(aArgs[0], channel));
        error = otLinkSetChannel(mInstance, channel);
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessChild(uint8_t aArgsLength, char *aArgs[])
{
    otError     error = OT_ERROR_NONE;
    otChildInfo childInfo;
    uint16_t    childId;
    bool        isTable;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(aArgs[0], "table") == 0);

    if (isTable || strcmp(aArgs[0], "list") == 0)
    {
        uint16_t maxChildren;

        if (isTable)
        {
            OutputLine(
                "| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|D|N|Ver|CSL|QMsgCnt| Extended MAC     |");
            OutputLine(
                "+-----+--------+------------+------------+-------+------+-+-+-+---+---+-------+------------------+");
        }

        maxChildren = otThreadGetMaxAllowedChildren(mInstance);

        for (uint16_t i = 0; i < maxChildren; i++)
        {
            if ((otThreadGetChildInfoByIndex(mInstance, i, &childInfo) != OT_ERROR_NONE) || childInfo.mIsStateRestoring)
            {
                continue;
            }

            if (isTable)
            {
                OutputFormat("| %3d ", childInfo.mChildId);
                OutputFormat("| 0x%04x ", childInfo.mRloc16);
                OutputFormat("| %10d ", childInfo.mTimeout);
                OutputFormat("| %10d ", childInfo.mAge);
                OutputFormat("| %5d ", childInfo.mLinkQualityIn);
                OutputFormat("| %4d ", childInfo.mNetworkDataVersion);
                OutputFormat("|%1d", childInfo.mRxOnWhenIdle);
                OutputFormat("|%1d", childInfo.mFullThreadDevice);
                OutputFormat("|%1d", childInfo.mFullNetworkData);
                OutputFormat("|%3d", childInfo.mVersion);
                OutputFormat("| %1d ", childInfo.mIsCslSynced);
                OutputFormat("| %5d ", childInfo.mQueuedMessageCnt);
                OutputFormat("| ");
                OutputExtAddress(childInfo.mExtAddress);
                OutputLine(" |");
            }
            else
            {
                OutputFormat("%d ", childInfo.mChildId);
            }
        }

        OutputLine("");
        ExitNow();
    }

    SuccessOrExit(error = ParseAsUint16(aArgs[0], childId));
    SuccessOrExit(error = otThreadGetChildInfoById(mInstance, childId, &childInfo));

    OutputLine("Child ID: %d", childInfo.mChildId);
    OutputLine("Rloc: %04x", childInfo.mRloc16);
    OutputFormat("Ext Addr: ");
    OutputExtAddress(childInfo.mExtAddress);
    OutputLine("");
    OutputFormat("Mode: ");

    if (!(childInfo.mRxOnWhenIdle || childInfo.mFullThreadDevice || childInfo.mFullNetworkData))
    {
        OutputFormat("-");
    }
    else
    {
        if (childInfo.mRxOnWhenIdle)
        {
            OutputFormat("r");
        }

        if (childInfo.mFullThreadDevice)
        {
            OutputFormat("d");
        }

        if (childInfo.mFullNetworkData)
        {
            OutputFormat("n");
        }
    }

    OutputLine("");

    OutputLine("Net Data: %d", childInfo.mNetworkDataVersion);
    OutputLine("Timeout: %d", childInfo.mTimeout);
    OutputLine("Age: %d", childInfo.mAge);
    OutputLine("Link Quality In: %d", childInfo.mLinkQualityIn);
    OutputLine("RSSI: %d", childInfo.mAverageRssi);

exit:
    return error;
}

otError Interpreter::ProcessChildIp(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        uint16_t maxChildren = otThreadGetMaxAllowedChildren(mInstance);

        for (uint16_t childIndex = 0; childIndex < maxChildren; childIndex++)
        {
            otChildIp6AddressIterator iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;
            otIp6Address              ip6Address;
            otChildInfo               childInfo;

            if ((otThreadGetChildInfoByIndex(mInstance, childIndex, &childInfo) != OT_ERROR_NONE) ||
                childInfo.mIsStateRestoring)
            {
                continue;
            }

            iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;

            while (otThreadGetChildNextIp6Address(mInstance, childIndex, &iterator, &ip6Address) == OT_ERROR_NONE)
            {
                OutputFormat("%04x: ", childInfo.mRloc16);
                OutputIp6Address(ip6Address);
                OutputLine("");
            }
        }
    }
    else if (strcmp(aArgs[0], "max") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otThreadGetMaxChildIpAddresses(mInstance));
        }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        else if (aArgsLength == 2)
        {
            uint8_t maxIpAddresses;
            SuccessOrExit(error = ParseAsUint8(aArgs[1], maxIpAddresses));
            SuccessOrExit(error = otThreadSetMaxChildIpAddresses(mInstance, maxIpAddresses));
        }
#endif
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
exit:
#endif
    return error;
}

otError Interpreter::ProcessChildMax(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetMaxAllowedChildren(mInstance));
    }
    else
    {
        uint16_t maxChildren;

        SuccessOrExit(error = ParseAsUint16(aArgs[0], maxChildren));
        SuccessOrExit(error = otThreadSetMaxAllowedChildren(mInstance, maxChildren));
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
otError Interpreter::ProcessChildSupervision(uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint16_t value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "checktimeout") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%u", otChildSupervisionGetCheckTimeout(mInstance));
        }
        else if (aArgsLength == 2)
        {
            SuccessOrExit(error = ParseAsUint16(aArgs[1], value));
            otChildSupervisionSetCheckTimeout(mInstance, value);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#if OPENTHREAD_FTD
    else if (strcmp(aArgs[0], "interval") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%u", otChildSupervisionGetInterval(mInstance));
        }
        else if (aArgsLength == 2)
        {
            SuccessOrExit(error = ParseAsUint16(aArgs[1], value));
            otChildSupervisionSetInterval(mInstance, value);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE

otError Interpreter::ProcessChildTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetChildTimeout(mInstance));
    }
    else
    {
        uint32_t timeout;

        SuccessOrExit(error = ParseAsUint32(aArgs[0], timeout));
        otThreadSetChildTimeout(mInstance, timeout);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
otError Interpreter::ProcessCoap(uint8_t aArgsLength, char *aArgs[])
{
    return mCoap.Process(aArgsLength, aArgs);
}
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
otError Interpreter::ProcessCoapSecure(uint8_t aArgsLength, char *aArgs[])
{
    return mCoapSecure.Process(aArgsLength, aArgs);
}
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError Interpreter::ProcessCoexMetrics(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputEnabledDisabledStatus(otPlatRadioIsCoexEnabled(mInstance));
    }
    else if (strcmp(aArgs[0], "enable") == 0)
    {
        error = otPlatRadioSetCoexEnabled(mInstance, true);
    }
    else if (strcmp(aArgs[0], "disable") == 0)
    {
        error = otPlatRadioSetCoexEnabled(mInstance, false);
    }
    else if (strcmp(aArgs[0], "metrics") == 0)
    {
        otRadioCoexMetrics metrics;

        SuccessOrExit(error = otPlatRadioGetCoexMetrics(mInstance, &metrics));

        OutputLine("Stopped: %s", metrics.mStopped ? "true" : "false");
        OutputLine("Grant Glitch: %u", metrics.mNumGrantGlitch);
        OutputLine("Transmit metrics");
        OutputLine(kIndentSize, "Request: %u", metrics.mNumTxRequest);
        OutputLine(kIndentSize, "Grant Immediate: %u", metrics.mNumTxGrantImmediate);
        OutputLine(kIndentSize, "Grant Wait: %u", metrics.mNumTxGrantWait);
        OutputLine(kIndentSize, "Grant Wait Activated: %u", metrics.mNumTxGrantWaitActivated);
        OutputLine(kIndentSize, "Grant Wait Timeout: %u", metrics.mNumTxGrantWaitTimeout);
        OutputLine(kIndentSize, "Grant Deactivated During Request: %u", metrics.mNumTxGrantDeactivatedDuringRequest);
        OutputLine(kIndentSize, "Delayed Grant: %u", metrics.mNumTxDelayedGrant);
        OutputLine(kIndentSize, "Average Request To Grant Time: %u", metrics.mAvgTxRequestToGrantTime);
        OutputLine("Receive metrics");
        OutputLine(kIndentSize, "Request: %u", metrics.mNumRxRequest);
        OutputLine(kIndentSize, "Grant Immediate: %u", metrics.mNumRxGrantImmediate);
        OutputLine(kIndentSize, "Grant Wait: %u", metrics.mNumRxGrantWait);
        OutputLine(kIndentSize, "Grant Wait Activated: %u", metrics.mNumRxGrantWaitActivated);
        OutputLine(kIndentSize, "Grant Wait Timeout: %u", metrics.mNumRxGrantWaitTimeout);
        OutputLine(kIndentSize, "Grant Deactivated During Request: %u", metrics.mNumRxGrantDeactivatedDuringRequest);
        OutputLine(kIndentSize, "Delayed Grant: %u", metrics.mNumRxDelayedGrant);
        OutputLine(kIndentSize, "Average Request To Grant Time: %u", metrics.mAvgRxRequestToGrantTime);
        OutputLine(kIndentSize, "Grant None: %u", metrics.mNumRxGrantNone);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessContextIdReuseDelay(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetContextIdReuseDelay(mInstance));
    }
    else
    {
        uint32_t delay;

        SuccessOrExit(error = ParseAsUint32(aArgs[0], delay));
        otThreadSetContextIdReuseDelay(mInstance, delay);
    }

exit:
    return error;
}
#endif

otError Interpreter::ProcessCounters(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("mac");
        OutputLine("mle");
    }
    else if (strcmp(aArgs[0], "mac") == 0)
    {
        if (aArgsLength == 1)
        {
            const otMacCounters *macCounters = otLinkGetCounters(mInstance);

            OutputLine("TxTotal: %d", macCounters->mTxTotal);
            OutputLine(kIndentSize, "TxUnicast: %d", macCounters->mTxUnicast);
            OutputLine(kIndentSize, "TxBroadcast: %d", macCounters->mTxBroadcast);
            OutputLine(kIndentSize, "TxAckRequested: %d", macCounters->mTxAckRequested);
            OutputLine(kIndentSize, "TxAcked: %d", macCounters->mTxAcked);
            OutputLine(kIndentSize, "TxNoAckRequested: %d", macCounters->mTxNoAckRequested);
            OutputLine(kIndentSize, "TxData: %d", macCounters->mTxData);
            OutputLine(kIndentSize, "TxDataPoll: %d", macCounters->mTxDataPoll);
            OutputLine(kIndentSize, "TxBeacon: %d", macCounters->mTxBeacon);
            OutputLine(kIndentSize, "TxBeaconRequest: %d", macCounters->mTxBeaconRequest);
            OutputLine(kIndentSize, "TxOther: %d", macCounters->mTxOther);
            OutputLine(kIndentSize, "TxRetry: %d", macCounters->mTxRetry);
            OutputLine(kIndentSize, "TxErrCca: %d", macCounters->mTxErrCca);
            OutputLine(kIndentSize, "TxErrBusyChannel: %d", macCounters->mTxErrBusyChannel);
            OutputLine("RxTotal: %d", macCounters->mRxTotal);
            OutputLine(kIndentSize, "RxUnicast: %d", macCounters->mRxUnicast);
            OutputLine(kIndentSize, "RxBroadcast: %d", macCounters->mRxBroadcast);
            OutputLine(kIndentSize, "RxData: %d", macCounters->mRxData);
            OutputLine(kIndentSize, "RxDataPoll: %d", macCounters->mRxDataPoll);
            OutputLine(kIndentSize, "RxBeacon: %d", macCounters->mRxBeacon);
            OutputLine(kIndentSize, "RxBeaconRequest: %d", macCounters->mRxBeaconRequest);
            OutputLine(kIndentSize, "RxOther: %d", macCounters->mRxOther);
            OutputLine(kIndentSize, "RxAddressFiltered: %d", macCounters->mRxAddressFiltered);
            OutputLine(kIndentSize, "RxDestAddrFiltered: %d", macCounters->mRxDestAddrFiltered);
            OutputLine(kIndentSize, "RxDuplicated: %d", macCounters->mRxDuplicated);
            OutputLine(kIndentSize, "RxErrNoFrame: %d", macCounters->mRxErrNoFrame);
            OutputLine(kIndentSize, "RxErrNoUnknownNeighbor: %d", macCounters->mRxErrUnknownNeighbor);
            OutputLine(kIndentSize, "RxErrInvalidSrcAddr: %d", macCounters->mRxErrInvalidSrcAddr);
            OutputLine(kIndentSize, "RxErrSec: %d", macCounters->mRxErrSec);
            OutputLine(kIndentSize, "RxErrFcs: %d", macCounters->mRxErrFcs);
            OutputLine(kIndentSize, "RxErrOther: %d", macCounters->mRxErrOther);
        }
        else if ((aArgsLength == 2) && (strcmp(aArgs[1], "reset") == 0))
        {
            otLinkResetCounters(mInstance);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
    else if (strcmp(aArgs[0], "mle") == 0)
    {
        if (aArgsLength == 1)
        {
            const otMleCounters *mleCounters = otThreadGetMleCounters(mInstance);

            OutputLine("Role Disabled: %d", mleCounters->mDisabledRole);
            OutputLine("Role Detached: %d", mleCounters->mDetachedRole);
            OutputLine("Role Child: %d", mleCounters->mChildRole);
            OutputLine("Role Router: %d", mleCounters->mRouterRole);
            OutputLine("Role Leader: %d", mleCounters->mLeaderRole);
            OutputLine("Attach Attempts: %d", mleCounters->mAttachAttempts);
            OutputLine("Partition Id Changes: %d", mleCounters->mPartitionIdChanges);
            OutputLine("Better Partition Attach Attempts: %d", mleCounters->mBetterPartitionAttachAttempts);
            OutputLine("Parent Changes: %d", mleCounters->mParentChanges);
        }
        else if ((aArgsLength == 2) && (strcmp(aArgs[1], "reset") == 0))
        {
            otThreadResetMleCounters(mInstance);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError Interpreter::ProcessCsl(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgsLength == 0)
    {
        OutputLine("Channel: %u", otLinkCslGetChannel(mInstance));
        OutputLine("Period: %u(in units of 10 symbols), %ums", otLinkCslGetPeriod(mInstance),
                   otLinkCslGetPeriod(mInstance) * kUsPerTenSymbols / 1000);
        OutputLine("Timeout: %us", otLinkCslGetTimeout(mInstance));
        error = OT_ERROR_NONE;
    }
    else if (aArgsLength == 2)
    {
        if (strcmp(aArgs[0], "channel") == 0)
        {
            uint8_t channel;

            SuccessOrExit(error = ParseAsUint8(aArgs[1], channel));
            SuccessOrExit(error = otLinkCslSetChannel(mInstance, channel));
        }
        else if (strcmp(aArgs[0], "period") == 0)
        {
            uint16_t period;

            SuccessOrExit(error = ParseAsUint16(aArgs[1], period));
            SuccessOrExit(error = otLinkCslSetPeriod(mInstance, period));
        }
        else if (strcmp(aArgs[0], "timeout") == 0)
        {
            uint32_t timeout;

            SuccessOrExit(error = ParseAsUint32(aArgs[1], timeout));
            SuccessOrExit(error = otLinkCslSetTimeout(mInstance, timeout));
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessDelayTimerMin(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", (otDatasetGetDelayTimerMinimal(mInstance) / 1000));
    }
    else if (aArgsLength == 1)
    {
        uint32_t delay;
        SuccessOrExit(error = ParseAsUint32(aArgs[0], delay));
        SuccessOrExit(error = otDatasetSetDelayTimerMinimal(mInstance, static_cast<uint32_t>(delay * 1000)));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}
#endif

otError Interpreter::ProcessDiscover(uint8_t aArgsLength, char *aArgs[])
{
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;

    if (aArgsLength > 0)
    {
        uint8_t channel;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], channel));
        VerifyOrExit(channel < sizeof(scanChannels) * CHAR_BIT, error = OT_ERROR_INVALID_ARGS);
        scanChannels = 1 << channel;
    }

    SuccessOrExit(error = otThreadDiscover(mInstance, scanChannels, OT_PANID_BROADCAST, false, false,
                                           &Interpreter::HandleActiveScanResult, this));
    OutputLine("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |");
    OutputLine("+---+------------------+------------------+------+------------------+----+-----+-----+");

    error = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::OutputDnsTxtData(const uint8_t *aTxtData, uint16_t aTxtDataLength)
{
    otDnsTxtEntry         entry;
    otDnsTxtEntryIterator iterator;
    bool                  isFirst = true;

    otDnsInitTxtEntryIterator(&iterator, aTxtData, aTxtDataLength);

    OutputFormat("[");

    while (otDnsGetNextTxtEntry(&iterator, &entry) == OT_ERROR_NONE)
    {
        if (!isFirst)
        {
            OutputFormat(", ");
        }

        if (entry.mKey == nullptr)
        {
            // A null `mKey` indicates that the key in the entry is
            // longer than the recommended max key length, so the entry
            // could not be parsed. In this case, the whole entry is
            // returned encoded in `mValue`.

            OutputFormat("[");
            OutputBytes(entry.mValue, entry.mValueLength);
            OutputFormat("]");
        }
        else
        {
            OutputFormat("%s", entry.mKey);

            if (entry.mValue != nullptr)
            {
                OutputFormat("=");
                OutputBytes(entry.mValue, entry.mValueLength);
            }
        }

        isFirst = false;
    }

    OutputFormat("]");
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

otError Interpreter::GetDnsConfig(uint8_t            aArgsLength,
                                  char *             aArgs[],
                                  otDnsQueryConfig *&aConfig,
                                  uint8_t            aStartArgsIndex)
{
    // This method gets the optional config from given `aArgs` after the
    // `aStartArgsIndex`. The format: `[server IPv6 address] [server
    // port] [timeout] [max tx attempt] [recursion desired]`.

    otError error = OT_ERROR_NONE;
    bool    recursionDesired;

    memset(aConfig, 0, sizeof(otDnsQueryConfig));

    VerifyOrExit(aArgsLength > aStartArgsIndex, aConfig = nullptr);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[aStartArgsIndex], aConfig->mServerSockAddr.mAddress));

    VerifyOrExit(aArgsLength > aStartArgsIndex + 1);
    SuccessOrExit(error = ParseAsUint16(aArgs[aStartArgsIndex + 1], aConfig->mServerSockAddr.mPort));

    VerifyOrExit(aArgsLength > aStartArgsIndex + 2);
    SuccessOrExit(error = ParseAsUint32(aArgs[aStartArgsIndex + 2], aConfig->mResponseTimeout));

    VerifyOrExit(aArgsLength > aStartArgsIndex + 3);
    SuccessOrExit(error = ParseAsUint8(aArgs[aStartArgsIndex + 3], aConfig->mMaxTxAttempts));

    VerifyOrExit(aArgsLength > aStartArgsIndex + 4);
    SuccessOrExit(error = ParseAsBool(aArgs[aStartArgsIndex + 4], recursionDesired));
    aConfig->mRecursionFlag = recursionDesired ? OT_DNS_FLAG_RECURSION_DESIRED : OT_DNS_FLAG_NO_RECURSION;

exit:
    return error;
}

otError Interpreter::ProcessDns(uint8_t aArgsLength, char *aArgs[])
{
    otError           error = OT_ERROR_NONE;
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "config") == 0)
    {
        if (aArgsLength == 1)
        {
            const otDnsQueryConfig *defaultConfig = otDnsClientGetDefaultConfig(mInstance);

            OutputFormat("Server: [");
            OutputIp6Address(defaultConfig->mServerSockAddr.mAddress);
            OutputLine("]:%d", defaultConfig->mServerSockAddr.mPort);
            OutputLine("ResponseTimeout: %u ms", defaultConfig->mResponseTimeout);
            OutputLine("MaxTxAttempts: %u", defaultConfig->mMaxTxAttempts);
            OutputLine("RecursionDesired: %s",
                       (defaultConfig->mRecursionFlag == OT_DNS_FLAG_RECURSION_DESIRED) ? "yes" : "no");
        }
        else
        {
            SuccessOrExit(error = GetDnsConfig(aArgsLength, aArgs, config, 1));
            otDnsClientSetDefaultConfig(mInstance, config);
        }
    }
    else if (strcmp(aArgs[0], "resolve") == 0)
    {
        SuccessOrExit(error = GetDnsConfig(aArgsLength, aArgs, config, 2));
        SuccessOrExit(error = otDnsClientResolveAddress(mInstance, aArgs[1], &Interpreter::HandleDnsAddressResponse,
                                                        this, config));
        error = OT_ERROR_PENDING;
    }
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    else if (strcmp(aArgs[0], "browse") == 0)
    {
        SuccessOrExit(error = GetDnsConfig(aArgsLength, aArgs, config, 2));
        SuccessOrExit(error =
                          otDnsClientBrowse(mInstance, aArgs[1], &Interpreter::HandleDnsBrowseResponse, this, config));
        error = OT_ERROR_PENDING;
    }
    else if (strcmp(aArgs[0], "service") == 0)
    {
        SuccessOrExit(error = GetDnsConfig(aArgsLength, aArgs, config, 3));
        SuccessOrExit(error = otDnsClientResolveService(mInstance, aArgs[1], aArgs[2],
                                                        &Interpreter::HandleDnsServiceResponse, this, config));
        error = OT_ERROR_PENDING;
    }
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

void Interpreter::HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDnsAddressResponse(aError, aResponse);
}

void Interpreter::HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse)
{
    char         hostName[OT_DNS_MAX_NAME_SIZE];
    otIp6Address address;
    uint32_t     ttl;

    IgnoreError(otDnsAddressResponseGetHostName(aResponse, hostName, sizeof(hostName)));

    OutputFormat("DNS response for %s - ", hostName);

    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsAddressResponseGetAddress(aResponse, index, &address, &ttl) == OT_ERROR_NONE)
        {
            OutputIp6Address(address);
            OutputFormat(" TTL:%u ", ttl);
            index++;
        }

        OutputLine("");
    }

    OutputResult(aError);
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

void Interpreter::OutputDnsServiceInfo(uint8_t aIndentSize, const otDnsServiceInfo &aServiceInfo)
{
    OutputLine(aIndentSize, "Port:%d, Priority:%d, Weight:%d, TTL:%u", aServiceInfo.mPort, aServiceInfo.mPriority,
               aServiceInfo.mWeight, aServiceInfo.mTtl);
    OutputLine(aIndentSize, "Host:%s", aServiceInfo.mHostNameBuffer);
    OutputFormat(aIndentSize, "HostAddress:");
    OutputIp6Address(aServiceInfo.mHostAddress);
    OutputLine(" TTL:%u", aServiceInfo.mHostAddressTtl);
    OutputFormat(aIndentSize, "TXT:");
    OutputDnsTxtData(aServiceInfo.mTxtData, aServiceInfo.mTxtDataSize);
    OutputLine(" TTL:%u", aServiceInfo.mTxtDataTtl);
}

void Interpreter::HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDnsBrowseResponse(aError, aResponse);
}

void Interpreter::HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse)
{
    char             name[OT_DNS_MAX_NAME_SIZE];
    char             label[OT_DNS_MAX_LABEL_SIZE];
    uint8_t          txtBuffer[255];
    otDnsServiceInfo serviceInfo;

    IgnoreError(otDnsBrowseResponseGetServiceName(aResponse, name, sizeof(name)));

    OutputLine("DNS browse response for %s", name);

    if (aError == OT_ERROR_NONE)
    {
        uint16_t index = 0;

        while (otDnsBrowseResponseGetServiceInstance(aResponse, index, label, sizeof(label)) == OT_ERROR_NONE)
        {
            OutputLine("%s", label);
            index++;

            serviceInfo.mHostNameBuffer     = name;
            serviceInfo.mHostNameBufferSize = sizeof(name);
            serviceInfo.mTxtData            = txtBuffer;
            serviceInfo.mTxtDataSize        = sizeof(txtBuffer);

            if (otDnsBrowseResponseGetServiceInfo(aResponse, label, &serviceInfo) == OT_ERROR_NONE)
            {
                OutputDnsServiceInfo(kIndentSize, serviceInfo);
            }

            OutputLine("");
        }
    }

    OutputResult(aError);
}

void Interpreter::HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDnsServiceResponse(aError, aResponse);
}

void Interpreter::HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse)
{
    char             name[OT_DNS_MAX_NAME_SIZE];
    char             label[OT_DNS_MAX_LABEL_SIZE];
    uint8_t          txtBuffer[255];
    otDnsServiceInfo serviceInfo;

    IgnoreError(otDnsServiceResponseGetServiceName(aResponse, label, sizeof(label), name, sizeof(name)));

    OutputLine("DNS service resolution response for %s for service %s", label, name);

    if (aError == OT_ERROR_NONE)
    {
        serviceInfo.mHostNameBuffer     = name;
        serviceInfo.mHostNameBufferSize = sizeof(name);
        serviceInfo.mTxtData            = txtBuffer;
        serviceInfo.mTxtDataSize        = sizeof(txtBuffer);

        if (otDnsServiceResponseGetServiceInfo(aResponse, &serviceInfo) == OT_ERROR_NONE)
        {
            OutputDnsServiceInfo(/* aIndetSize */ 0, serviceInfo);
            OutputLine("");
        }
    }

    OutputResult(aError);
}

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessEidCache(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otCacheEntryIterator iterator;
    otCacheEntryInfo     entry;

    memset(&iterator, 0, sizeof(iterator));

    for (uint8_t i = 0;; i++)
    {
        SuccessOrExit(otThreadGetNextCacheEntry(mInstance, &entry, &iterator));

        OutputIp6Address(entry.mTarget);
        OutputLine(" %04x", entry.mRloc16);
    }

exit:
    return OT_ERROR_NONE;
}
#endif

otError Interpreter::ProcessEui64(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError      error = OT_ERROR_NONE;
    otExtAddress extAddress;

    VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);

    otLinkGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
    OutputBytes(extAddress.m8, OT_EXT_ADDRESS_SIZE);
    OutputLine("");

exit:
    return error;
}

otError Interpreter::ProcessExtAddress(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *extAddress = reinterpret_cast<const uint8_t *>(otLinkGetExtendedAddress(mInstance));
        OutputBytes(extAddress, OT_EXT_ADDRESS_SIZE);
        OutputLine("");
    }
    else
    {
        otExtAddress extAddress;

        SuccessOrExit(error = ParseAsHexString(aArgs[0], extAddress.m8));
        error = otLinkSetExtendedAddress(mInstance, &extAddress);
    }

exit:
    return error;
}

#if OPENTHREAD_POSIX
otError Interpreter::ProcessExit(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    exit(EXIT_SUCCESS);

    return OT_ERROR_NONE;
}
#endif

otError Interpreter::ProcessLog(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength >= 1, error = OT_ERROR_INVALID_ARGS);

    if (!strcmp(aArgs[0], "level"))
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otLoggingGetLevel());
        }
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
        else if (aArgsLength == 2)
        {
            uint8_t level;

            SuccessOrExit(error = ParseAsUint8(aArgs[1], level));
            SuccessOrExit(error = otLoggingSetLevel(static_cast<otLogLevel>(level)));
        }
#endif
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
    else if (!strcmp(aArgs[0], "filename"))
    {
        VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otPlatDebugUart_logfile(aArgs[1]));
    }
#endif
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Interpreter::ProcessExtPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *extPanId = reinterpret_cast<const uint8_t *>(otThreadGetExtendedPanId(mInstance));
        OutputBytes(extPanId, OT_EXT_PAN_ID_SIZE);
        OutputLine("");
    }
    else
    {
        otExtendedPanId extPanId;

        SuccessOrExit(error = ParseAsHexString(aArgs[0], extPanId.m8));
        error = otThreadSetExtendedPanId(mInstance, &extPanId);
    }

exit:
    return error;
}

otError Interpreter::ProcessFactoryReset(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceFactoryReset(mInstance);

    return OT_ERROR_NONE;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError Interpreter::ProcessFake(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    VerifyOrExit(aArgsLength >= 1);

    if (strcmp(aArgs[0], "/a/an") == 0)
    {
        otIp6Address             destination, target;
        otIp6InterfaceIdentifier mlIid;

        VerifyOrExit(aArgsLength == 4, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseAsIp6Address(aArgs[1], destination));
        SuccessOrExit(error = ParseAsIp6Address(aArgs[2], target));
        SuccessOrExit(error = ParseAsHexString(aArgs[3], mlIid.mFields.m8));
        otThreadSendAddressNotification(mInstance, &destination, &target, &mlIid);
    }
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    else if (strcmp(aArgs[0], "/b/ba") == 0)
    {
        otIp6Address             target;
        otIp6InterfaceIdentifier mlIid;
        uint32_t                 timeSinceLastTransaction;

        VerifyOrExit(aArgsLength == 4, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseAsIp6Address(aArgs[1], target));
        SuccessOrExit(error = ParseAsHexString(aArgs[2], mlIid.mFields.m8));
        SuccessOrExit(error = ParseAsUint32(aArgs[3], timeSinceLastTransaction));

        error = otThreadSendProactiveBackboneNotification(mInstance, &target, &mlIid, timeSinceLastTransaction);
    }
#endif
exit:
    return error;
}
#endif

otError Interpreter::ProcessFem(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        int8_t lnaGain;

        SuccessOrExit(error = otPlatRadioGetFemLnaGain(mInstance, &lnaGain));
        OutputLine("LNA gain %d dBm", lnaGain);
    }
    else if (strcmp(aArgs[0], "lnagain") == 0)
    {
        if (aArgsLength == 1)
        {
            int8_t lnaGain;

            SuccessOrExit(error = otPlatRadioGetFemLnaGain(mInstance, &lnaGain));
            OutputLine("%d", lnaGain);
        }
        else
        {
            int8_t lnaGain;

            SuccessOrExit(error = ParseAsInt8(aArgs[1], lnaGain));
            SuccessOrExit(error = otPlatRadioSetFemLnaGain(mInstance, lnaGain));
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Interpreter::ProcessIfconfig(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otIp6IsEnabled(mInstance))
        {
            OutputLine("up");
        }
        else
        {
            OutputLine("down");
        }
    }
    else if (strcmp(aArgs[0], "up") == 0)
    {
        SuccessOrExit(error = otIp6SetEnabled(mInstance, true));
    }
    else if (strcmp(aArgs[0], "down") == 0)
    {
        SuccessOrExit(error = otIp6SetEnabled(mInstance, false));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Interpreter::ProcessIpAddrAdd(uint8_t aArgsLength, char *aArgs[])
{
    otError        error;
    otNetifAddress aAddress;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], aAddress.mAddress));
    aAddress.mPrefixLength  = 64;
    aAddress.mPreferred     = true;
    aAddress.mValid         = true;
    aAddress.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;
    error                   = otIp6AddUnicastAddress(mInstance, &aAddress);

exit:
    return error;
}

otError Interpreter::ProcessIpAddrDel(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));
    error = otIp6RemoveUnicastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessIpAddr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(mInstance);

        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
        {
            OutputIp6Address(addr->mAddress);
            OutputLine("");
        }
    }
    else
    {
        if (strcmp(aArgs[0], "add") == 0)
        {
            SuccessOrExit(error = ProcessIpAddrAdd(aArgsLength - 1, aArgs + 1));
        }
        else if (strcmp(aArgs[0], "del") == 0)
        {
            SuccessOrExit(error = ProcessIpAddrDel(aArgsLength - 1, aArgs + 1));
        }
        else if (strcmp(aArgs[0], "linklocal") == 0)
        {
            OutputIp6Address(*otThreadGetLinkLocalIp6Address(mInstance));
            OutputLine("");
        }
        else if (strcmp(aArgs[0], "rloc") == 0)
        {
            OutputIp6Address(*otThreadGetRloc(mInstance));
            OutputLine("");
        }
        else if (strcmp(aArgs[0], "mleid") == 0)
        {
            OutputIp6Address(*otThreadGetMeshLocalEid(mInstance));
            OutputLine("");
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_COMMAND);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddrAdd(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));
    error = otIp6SubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddrDel(uint8_t aArgsLength, char *aArgs[])
{
    otError      error;
    otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));
    error = otIp6UnsubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessMulticastPromiscuous(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputEnabledDisabledStatus(otIp6IsMulticastPromiscuousEnabled(mInstance));
    }
    else
    {
        if (strcmp(aArgs[0], "enable") == 0)
        {
            otIp6SetMulticastPromiscuousEnabled(mInstance, true);
        }
        else if (strcmp(aArgs[0], "disable") == 0)
        {
            otIp6SetMulticastPromiscuousEnabled(mInstance, false);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        for (const otNetifMulticastAddress *addr = otIp6GetMulticastAddresses(mInstance); addr; addr = addr->mNext)
        {
            OutputIp6Address(addr->mAddress);
            OutputLine("");
        }
    }
    else
    {
        if (strcmp(aArgs[0], "add") == 0)
        {
            SuccessOrExit(error = ProcessIpMulticastAddrAdd(aArgsLength - 1, aArgs + 1));
        }
        else if (strcmp(aArgs[0], "del") == 0)
        {
            SuccessOrExit(error = ProcessIpMulticastAddrDel(aArgsLength - 1, aArgs + 1));
        }
        else if (strcmp(aArgs[0], "promiscuous") == 0)
        {
            SuccessOrExit(error = ProcessMulticastPromiscuous(aArgsLength - 1, aArgs + 1));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_COMMAND);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessKeySequence(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength == 1 || aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "counter") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otThreadGetKeySequenceCounter(mInstance));
        }
        else
        {
            uint32_t counter;

            SuccessOrExit(error = ParseAsUint32(aArgs[1], counter));
            otThreadSetKeySequenceCounter(mInstance, counter);
        }
    }
    else if (strcmp(aArgs[0], "guardtime") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otThreadGetKeySwitchGuardTime(mInstance));
        }
        else
        {
            uint32_t guardTime;

            SuccessOrExit(error = ParseAsUint32(aArgs[1], guardTime));
            otThreadSetKeySwitchGuardTime(mInstance, guardTime);
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Interpreter::ProcessLeaderData(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError      error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(mInstance, &leaderData));

    OutputLine("Partition ID: %u", leaderData.mPartitionId);
    OutputLine("Weighting: %d", leaderData.mWeighting);
    OutputLine("Data Version: %d", leaderData.mDataVersion);
    OutputLine("Stable Data Version: %d", leaderData.mStableDataVersion);
    OutputLine("Leader Router ID: %d", leaderData.mLeaderRouterId);

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessPartitionId(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgsLength == 0)
    {
        OutputLine("%u", otThreadGetPartitionId(mInstance));
        error = OT_ERROR_NONE;
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (strcmp(aArgs[0], "preferred") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%u", otThreadGetPreferredLeaderPartitionId(mInstance));
            error = OT_ERROR_NONE;
        }
        else if (aArgsLength == 2)
        {
            uint32_t partitionId;

            SuccessOrExit(error = ParseAsUint32(aArgs[1], partitionId));
            otThreadSetPreferredLeaderPartitionId(mInstance, partitionId);
        }
    }

exit:
#endif

    return error;
}

otError Interpreter::ProcessLeaderWeight(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetLocalLeaderWeight(mInstance));
    }
    else
    {
        uint8_t weight;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], weight));
        otThreadSetLocalLeaderWeight(mInstance, weight);
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
void Interpreter::HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          uint8_t                    aStatus,
                                          void *                     aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsReport(aAddress, aMetricsValues, aStatus);
}

void Interpreter::PrintLinkMetricsValue(const otLinkMetricsValues *aMetricsValues)
{
    const char kLinkMetricsTypeCount[]   = "(Count/Summation)";
    const char kLinkMetricsTypeAverage[] = "(Exponential Moving Average)";

    if (aMetricsValues->mMetrics.mPduCount)
    {
        OutputLine(" - PDU Counter: %d %s", aMetricsValues->mPduCountValue, kLinkMetricsTypeCount);
    }

    if (aMetricsValues->mMetrics.mLqi)
    {
        OutputLine(" - LQI: %d %s", aMetricsValues->mLqiValue, kLinkMetricsTypeAverage);
    }

    if (aMetricsValues->mMetrics.mLinkMargin)
    {
        OutputLine(" - Margin: %d (dB) %s", aMetricsValues->mLinkMarginValue, kLinkMetricsTypeAverage);
    }

    if (aMetricsValues->mMetrics.mRssi)
    {
        OutputLine(" - RSSI: %d (dBm) %s", aMetricsValues->mRssiValue, kLinkMetricsTypeAverage);
    }
}

void Interpreter::HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          uint8_t                    aStatus)
{
    OutputFormat("Received Link Metrics Report from: ");
    OutputIp6Address(*aAddress);
    OutputLine("");

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
    else
    {
        OutputLine("Link Metrics Report, status: %s", LinkMetricsStatusToStr(aStatus));
    }
}

void Interpreter::HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsMgmtResponse(aAddress, aStatus);
}

void Interpreter::HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus)
{
    OutputFormat("Received Link Metrics Management Response from: ");
    OutputIp6Address(*aAddress);
    OutputLine("");

    OutputLine("Status: %s", LinkMetricsStatusToStr(aStatus));
}

void Interpreter::HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                   const otExtAddress *       aExtAddress,
                                                   const otLinkMetricsValues *aMetricsValues,
                                                   void *                     aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsEnhAckProbingIe(aShortAddress, aExtAddress, aMetricsValues);
}

void Interpreter::HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                   const otExtAddress *       aExtAddress,
                                                   const otLinkMetricsValues *aMetricsValues)
{
    OutputFormat("Received Link Metrics data in Enh Ack from neighbor, short address:0x%02x , extended address:",
                 aShortAddress);
    OutputExtAddress(*aExtAddress);
    OutputLine("");

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
}

const char *Interpreter::LinkMetricsStatusToStr(uint8_t aStatus)
{
    uint8_t            strIndex                = 0;
    static const char *linkMetricsStatusText[] = {
        "Success",
        "Cannot support new series",
        "Series ID already registered",
        "Series ID not recognized",
        "No matching series ID",
        "Other error",
        "Unknown error",
    };

    switch (aStatus)
    {
    case OT_LINK_METRICS_STATUS_SUCCESS:
        strIndex = 0;
        break;
    case OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES:
        strIndex = 1;
        break;
    case OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED:
        strIndex = 2;
        break;
    case OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED:
        strIndex = 3;
        break;
    case OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED:
        strIndex = 4;
        break;
    case OT_LINK_METRICS_STATUS_OTHER_ERROR:
        strIndex = 5;
        break;
    default:
        strIndex = 6;
        break;
    }

    return linkMetricsStatusText[strIndex];
}

otError Interpreter::ProcessLinkMetrics(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    VerifyOrExit(aArgsLength >= 1);

    if (strcmp(aArgs[0], "query") == 0)
    {
        error = ProcessLinkMetricsQuery(aArgsLength - 1, aArgs + 1);
    }
    else if (strcmp(aArgs[0], "mgmt") == 0)
    {
        error = ProcessLinkMetricsMgmt(aArgsLength - 1, aArgs + 1);
    }
    else if (strcmp(aArgs[0], "probe") == 0)
    {
        error = ProcessLinkMetricsProbe(aArgsLength - 1, aArgs + 1);
    }

exit:
    return error;
}

otError Interpreter::ProcessLinkMetricsQuery(uint8_t aArgsLength, char *aArgs[])
{
    otError       error = OT_ERROR_INVALID_ARGS;
    otIp6Address  address;
    otLinkMetrics linkMetrics;

    VerifyOrExit(aArgsLength == 3);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));

    if (strcmp(aArgs[1], "single") == 0)
    {
        SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[2]));
        error = otLinkMetricsQuery(mInstance, &address, /* aSeriesId */ 0, &linkMetrics,
                                   &Interpreter::HandleLinkMetricsReport, this);
    }
    else if (strcmp(aArgs[1], "forward") == 0)
    {
        uint8_t seriesId;
        SuccessOrExit(error = ParseAsUint8(aArgs[2], seriesId));
        error = otLinkMetricsQuery(mInstance, &address, seriesId, nullptr, &Interpreter::HandleLinkMetricsReport, this);
    }

exit:
    return error;
}

otError Interpreter::ParseLinkMetricsFlags(otLinkMetrics &aLinkMetrics, char *aFlags)
{
    otError error = OT_ERROR_NONE;

    memset(&aLinkMetrics, 0, sizeof(aLinkMetrics));

    for (char *arg = aFlags; *arg != '\0'; arg++)
    {
        switch (*arg)
        {
        case 'p':
            aLinkMetrics.mPduCount = true;
            break;

        case 'q':
            aLinkMetrics.mLqi = true;
            break;

        case 'm':
            aLinkMetrics.mLinkMargin = true;
            break;

        case 'r':
            aLinkMetrics.mRssi = true;
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessLinkMetricsMgmt(uint8_t aArgsLength, char *aArgs[])
{
    otError                  error = OT_ERROR_INVALID_ARGS;
    otIp6Address             address;
    otLinkMetricsSeriesFlags seriesFlags;
    bool                     clear = false;

    VerifyOrExit(aArgsLength >= 2);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));

    memset(&seriesFlags, 0, sizeof(otLinkMetricsSeriesFlags));

    if (strcmp(aArgs[1], "forward") == 0)
    {
        uint8_t       seriesId;
        otLinkMetrics linkMetrics;

        VerifyOrExit(aArgsLength >= 4);

        memset(&linkMetrics, 0, sizeof(otLinkMetrics));
        SuccessOrExit(error = ParseAsUint8(aArgs[2], seriesId));

        for (char *arg = aArgs[3]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'l':
                seriesFlags.mLinkProbe = true;
                break;

            case 'd':
                seriesFlags.mMacData = true;
                break;

            case 'r':
                seriesFlags.mMacDataRequest = true;
                break;

            case 'a':
                seriesFlags.mMacAck = true;
                break;

            case 'X':
                VerifyOrExit(arg == aArgs[3] && *(arg + 1) == '\0' && aArgsLength == 4,
                             error = OT_ERROR_INVALID_ARGS); // Ensure the flags only contain 'X'
                clear = true;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        if (!clear)
        {
            VerifyOrExit(aArgsLength == 5);
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[4]));
        }

        error = otLinkMetricsConfigForwardTrackingSeries(mInstance, &address, seriesId, seriesFlags,
                                                         clear ? nullptr : &linkMetrics,
                                                         &Interpreter::HandleLinkMetricsMgmtResponse, this);
    }
    else if (strcmp(aArgs[1], "enhanced-ack") == 0)
    {
        otLinkMetricsEnhAckFlags enhAckFlags;
        otLinkMetrics            linkMetrics;
        otLinkMetrics *          pLinkMetrics = &linkMetrics;

        VerifyOrExit(aArgsLength >= 3);

        if (strcmp(aArgs[2], "clear") == 0)
        {
            enhAckFlags  = OT_LINK_METRICS_ENH_ACK_CLEAR;
            pLinkMetrics = nullptr;
        }
        else if (strcmp(aArgs[2], "register") == 0)
        {
            enhAckFlags = OT_LINK_METRICS_ENH_ACK_REGISTER;
            VerifyOrExit(aArgsLength >= 4);
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[3]));
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
            if (aArgsLength > 4 && strcmp(aArgs[4], "r") == 0)
            {
                linkMetrics.mReserved = true;
            }
#endif
        }
        else
        {
            ExitNow();
        }

        error = otLinkMetricsConfigEnhAckProbing(mInstance, &address, enhAckFlags, pLinkMetrics,
                                                 &Interpreter::HandleLinkMetricsMgmtResponse, this,
                                                 &Interpreter::HandleLinkMetricsEnhAckProbingIe, this);
    }

exit:
    return error;
}

otError Interpreter::ProcessLinkMetricsProbe(uint8_t aArgsLength, char *aArgs[])
{
    otError      error = OT_ERROR_INVALID_ARGS;
    otIp6Address address;
    uint8_t      seriesId = 0;
    uint8_t      length   = 0;

    VerifyOrExit(aArgsLength == 3);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], address));
    SuccessOrExit(error = ParseAsUint8(aArgs[1], seriesId));
    SuccessOrExit(error = ParseAsUint8(aArgs[2], length));

    error = otLinkMetricsSendLinkProbe(mInstance, &address, seriesId, length);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessPskc(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const otPskc *pskc = otThreadGetPskc(mInstance);

        OutputBytes(pskc->m8);
        OutputLine("");
    }
    else
    {
        otPskc pskc;

        if (aArgsLength == 1)
        {
            SuccessOrExit(error = ParseAsHexString(aArgs[0], pskc.m8));
        }
        else if (!strcmp(aArgs[0], "-p"))
        {
            SuccessOrExit(error = otDatasetGeneratePskc(
                              aArgs[1], reinterpret_cast<const otNetworkName *>(otThreadGetNetworkName(mInstance)),
                              otThreadGetExtendedPanId(mInstance), &pskc));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        SuccessOrExit(error = otThreadSetPskc(mInstance, &pskc));
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

otError Interpreter::ProcessMasterKey(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputBytes(otThreadGetMasterKey(mInstance)->m8);
        OutputLine("");
    }
    else
    {
        otMasterKey key;

        SuccessOrExit(error = ParseAsHexString(aArgs[0], key.m8));
        SuccessOrExit(error = otThreadSetMasterKey(mInstance, &key));
    }

exit:
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

otError Interpreter::ProcessMlr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_COMMAND);

    if (!strcmp(aArgs[0], "reg"))
    {
        error = ProcessMlrReg(aArgsLength - 1, aArgs + 1);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError Interpreter::ProcessMlrReg(uint8_t aArgsLength, char *aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otIp6Address addresses[kIPv6AddressesNumMax];
    uint32_t     timeout;
    uint8_t      i;

    VerifyOrExit(aArgsLength >= 1, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aArgsLength <= kIPv6AddressesNumMax + 1, error = OT_ERROR_INVALID_ARGS);

    for (i = 0; i < aArgsLength && i < kIPv6AddressesNumMax; i++)
    {
        if (ParseAsIp6Address(aArgs[i], addresses[i]) != OT_ERROR_NONE)
        {
            break;
        }
    }

    VerifyOrExit(i > 0 && (i == aArgsLength || i == aArgsLength - 1), error = OT_ERROR_INVALID_ARGS);

    if (i == aArgsLength - 1)
    {
        // Parse the last argument as a timeout in seconds
        SuccessOrExit(error = ParseAsUint32(aArgs[i], timeout));
    }

    SuccessOrExit(error = otIp6RegisterMulticastListeners(mInstance, addresses, i,
                                                          i == aArgsLength - 1 ? &timeout : nullptr,
                                                          Interpreter::HandleMlrRegResult, this));

    error = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::HandleMlrRegResult(void *              aContext,
                                     otError             aError,
                                     uint8_t             aMlrStatus,
                                     const otIp6Address *aFailedAddresses,
                                     uint8_t             aFailedAddressNum)
{
    static_cast<Interpreter *>(aContext)->HandleMlrRegResult(aError, aMlrStatus, aFailedAddresses, aFailedAddressNum);
}

void Interpreter::HandleMlrRegResult(otError             aError,
                                     uint8_t             aMlrStatus,
                                     const otIp6Address *aFailedAddresses,
                                     uint8_t             aFailedAddressNum)
{
    if (aError == OT_ERROR_NONE)
    {
        OutputLine("status %d, %d failed", aMlrStatus, aFailedAddressNum);

        for (uint8_t i = 0; i < aFailedAddressNum; i++)
        {
            OutputIp6Address(aFailedAddresses[i]);
            OutputLine("");
        }
    }

    OutputResult(aError);
}

#endif // (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

otError Interpreter::ProcessMode(uint8_t aArgsLength, char *aArgs[])
{
    otError          error = OT_ERROR_NONE;
    otLinkModeConfig linkMode;

    memset(&linkMode, 0, sizeof(otLinkModeConfig));

    if (aArgsLength == 0)
    {
        linkMode = otThreadGetLinkMode(mInstance);

        if (!(linkMode.mRxOnWhenIdle || linkMode.mDeviceType || linkMode.mNetworkData))
        {
            OutputFormat("-");
        }
        else
        {
            if (linkMode.mRxOnWhenIdle)
            {
                OutputFormat("r");
            }

            if (linkMode.mDeviceType)
            {
                OutputFormat("d");
            }

            if (linkMode.mNetworkData)
            {
                OutputFormat("n");
            }
        }

        OutputLine("");

        ExitNow();
    }

    if (strcmp(aArgs[0], "-") != 0)
    {
        for (char *arg = aArgs[0]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'r':
                linkMode.mRxOnWhenIdle = true;
                break;

            case 'd':
                linkMode.mDeviceType = true;
                break;

            case 'n':
                linkMode.mNetworkData = true;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
    }

    error = otThreadSetLinkMode(mInstance, linkMode);

exit:
    return error;
}

otError Interpreter::ProcessMultiRadio(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aArgs);

    if (aArgsLength == 0)
    {
        bool isFirst = true;

        OutputFormat("[");
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        OutputFormat("15.4");
        isFirst = false;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        OutputFormat("%sTREL", isFirst ? "" : ", ");
#endif
        OutputLine("]");

        OT_UNUSED_VARIABLE(isFirst);
    }
#if OPENTHREAD_CONFIG_MULTI_RADIO
    else if (strcmp(aArgs[0], "neighbor") == 0)
    {
        otMultiRadioNeighborInfo multiRadioInfo;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

        if (strcmp(aArgs[1], "list") == 0)
        {
            otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
            otNeighborInfo         neighInfo;

            while (otThreadGetNextNeighborInfo(mInstance, &iterator, &neighInfo) == OT_ERROR_NONE)
            {
                if (otMultiRadioGetNeighborInfo(mInstance, &neighInfo.mExtAddress, &multiRadioInfo) != OT_ERROR_NONE)
                {
                    continue;
                }

                OutputFormat("ExtAddr:");
                OutputExtAddress(neighInfo.mExtAddress);
                OutputFormat(", RLOC16:0x%04x, Radios:", neighInfo.mRloc16);
                OutputMultiRadioInfo(multiRadioInfo);
            }
        }
        else
        {
            otExtAddress extAddress;

            SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddress.m8));
            SuccessOrExit(error = otMultiRadioGetNeighborInfo(mInstance, &extAddress, &multiRadioInfo));
            OutputMultiRadioInfo(multiRadioInfo);
        }
    }
#endif // OPENTHREAD_CONFIG_MULTI_RADIO
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MULTI_RADIO
void Interpreter::OutputMultiRadioInfo(const otMultiRadioNeighborInfo &aMultiRadioInfo)
{
    bool isFirst = true;

    OutputFormat("[");

    if (aMultiRadioInfo.mSupportsIeee802154)
    {
        OutputFormat("15.4(%d)", aMultiRadioInfo.mIeee802154Info.mPreference);
        isFirst = false;
    }

    if (aMultiRadioInfo.mSupportsTrelUdp6)
    {
        OutputFormat("%sTREL(%d)", isFirst ? "" : ", ", aMultiRadioInfo.mTrelUdp6Info.mPreference);
    }

    OutputLine("]");
}
#endif // OPENTHREAD_CONFIG_MULTI_RADIO

#if OPENTHREAD_FTD
otError Interpreter::ProcessNeighbor(uint8_t aArgsLength, char *aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otNeighborInfo         neighborInfo;
    bool                   isTable;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(aArgs[0], "table") == 0);

    if (isTable || strcmp(aArgs[0], "list") == 0)
    {
        if (isTable)
        {
            OutputLine("| Role | RLOC16 | Age | Avg RSSI | Last RSSI |R|D|N| Extended MAC     |");
            OutputLine("+------+--------+-----+----------+-----------+-+-+-+------------------+");
        }

        while (otThreadGetNextNeighborInfo(mInstance, &iterator, &neighborInfo) == OT_ERROR_NONE)
        {
            if (isTable)
            {
                OutputFormat("| %3c  ", neighborInfo.mIsChild ? 'C' : 'R');
                OutputFormat("| 0x%04x ", neighborInfo.mRloc16);
                OutputFormat("| %3d ", neighborInfo.mAge);
                OutputFormat("| %8d ", neighborInfo.mAverageRssi);
                OutputFormat("| %9d ", neighborInfo.mLastRssi);
                OutputFormat("|%1d", neighborInfo.mRxOnWhenIdle);
                OutputFormat("|%1d", neighborInfo.mFullThreadDevice);
                OutputFormat("|%1d", neighborInfo.mFullNetworkData);
                OutputFormat("| ");
                OutputExtAddress(neighborInfo.mExtAddress);
                OutputLine(" |");
            }
            else
            {
                OutputFormat("0x%04x ", neighborInfo.mRloc16);
            }
        }

        OutputLine("");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
otError Interpreter::ProcessNetif(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError      error    = OT_ERROR_NONE;
    const char * netif    = nullptr;
    unsigned int netifidx = 0;

    SuccessOrExit(error = otPlatGetNetif(mInstance, &netif, &netifidx));

    OutputLine("%s:%u", netif ? netif : "(null)", netifidx);

exit:
    return error;
}
#endif

otError Interpreter::ProcessNetstat(uint8_t aArgsLength, char *aArgs[])
{
    otUdpSocket *socket = otUdpGetSockets(mInstance);

    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("|                 Local Address                 |                  Peer Address                 |");
    OutputLine("+-----------------------------------------------+-----------------------------------------------+");

    while (socket)
    {
        constexpr int kMaxOutputLength = 45;
        int           outputLength;

        OutputFormat("| ");

        outputLength = OutputSocketAddress(socket->mSockName);
        for (int i = outputLength; 0 <= i && i < kMaxOutputLength; ++i)
        {
            OutputFormat(" ");
        }
        OutputFormat(" | ");

        outputLength = OutputSocketAddress(socket->mPeerName);
        for (int i = outputLength; 0 <= i && i < kMaxOutputLength; ++i)
        {
            OutputFormat(" ");
        }
        OutputLine(" |");

        socket = socket->mNext;
    }

    return OT_ERROR_NONE;
}

int Interpreter::OutputSocketAddress(const otSockAddr &aAddress)
{
    int outputLength;
    int result = 0;

    VerifyOrExit((outputLength = OutputIp6Address(aAddress.mAddress)) >= 0, result = -1);
    result += outputLength;

    VerifyOrExit((outputLength = OutputFormat(":")) >= 0, result = -1);
    result += outputLength;
    if (aAddress.mPort == 0)
    {
        VerifyOrExit((outputLength = OutputFormat("*")) >= 0, result = -1);
        result += outputLength;
    }
    else
    {
        VerifyOrExit((outputLength = OutputFormat("%d", aAddress.mPort)) >= 0, result = -1);
        result += outputLength;
    }

exit:
    return result;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
otError Interpreter::ProcessServiceList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otServiceConfig       config;

    while (otServerGetNextService(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mNetworkData.OutputService(config);
    }

    return OT_ERROR_NONE;
}

otError Interpreter::ProcessService(uint8_t aArgsLength, char *aArgs[])
{
    otError         error = OT_ERROR_INVALID_COMMAND;
    otServiceConfig cfg;

    if (aArgsLength == 0)
    {
        error = ProcessServiceList();
    }
    else
    {
        uint16_t length;

        VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseAsUint32(aArgs[1], cfg.mEnterpriseNumber));

        length = sizeof(cfg.mServiceData);
        SuccessOrExit(error = ParseAsHexString(aArgs[2], length, cfg.mServiceData));
        VerifyOrExit(length > 0, error = OT_ERROR_INVALID_ARGS);
        cfg.mServiceDataLength = static_cast<uint8_t>(length);

        if (strcmp(aArgs[0], "add") == 0)
        {
            VerifyOrExit(aArgsLength > 3, error = OT_ERROR_INVALID_ARGS);

            length = sizeof(cfg.mServerConfig.mServerData);
            SuccessOrExit(error = ParseAsHexString(aArgs[3], length, cfg.mServerConfig.mServerData));
            VerifyOrExit(length > 0, error = OT_ERROR_INVALID_ARGS);
            cfg.mServerConfig.mServerDataLength = static_cast<uint8_t>(length);

            cfg.mServerConfig.mStable = true;

            error = otServerAddService(mInstance, &cfg);
        }
        else if (strcmp(aArgs[0], "remove") == 0)
        {
            error = otServerRemoveService(mInstance, cfg.mEnterpriseNumber, cfg.mServiceData, cfg.mServiceDataLength);
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

otError Interpreter::ProcessNetworkData(uint8_t aArgsLength, char *aArgs[])
{
    return mNetworkData.Process(aArgsLength, aArgs);
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessNetworkIdTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetNetworkIdTimeout(mInstance));
    }
    else
    {
        uint8_t timeout;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], timeout));
        otThreadSetNetworkIdTimeout(mInstance, timeout);
    }

exit:
    return error;
}
#endif

otError Interpreter::ProcessNetworkName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%s", otThreadGetNetworkName(mInstance));
    }
    else
    {
        SuccessOrExit(error = otThreadSetNetworkName(mInstance, aArgs[0]));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
otError Interpreter::ProcessNetworkTime(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        uint64_t            time;
        otNetworkTimeStatus networkTimeStatus;

        networkTimeStatus = otNetworkTimeGet(mInstance, &time);

        OutputFormat("Network Time:     %luus", time);

        switch (networkTimeStatus)
        {
        case OT_NETWORK_TIME_UNSYNCHRONIZED:
            OutputLine(" (unsynchronized)");
            break;

        case OT_NETWORK_TIME_RESYNC_NEEDED:
            OutputLine(" (resync needed)");
            break;

        case OT_NETWORK_TIME_SYNCHRONIZED:
            OutputLine(" (synchronized)");
            break;

        default:
            break;
        }

        OutputLine("Time Sync Period: %ds", otNetworkTimeGetSyncPeriod(mInstance));
        OutputLine("XTAL Threshold:   %dppm", otNetworkTimeGetXtalThreshold(mInstance));
    }
    else if (aArgsLength == 2)
    {
        uint16_t period;
        uint16_t xtalThreshold;

        SuccessOrExit(error = ParseAsUint16(aArgs[0], period));
        ;
        SuccessOrExit(error = ParseAsUint16(aArgs[1], xtalThreshold));
        SuccessOrExit(error = otNetworkTimeSetSyncPeriod(mInstance, period));
        SuccessOrExit(error = otNetworkTimeSetXtalThreshold(mInstance, xtalThreshold));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

otError Interpreter::ProcessPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("0x%04x", otLinkGetPanId(mInstance));
    }
    else
    {
        uint16_t panId;

        SuccessOrExit(error = ParseAsUint16(aArgs[0], panId));
        error = otLinkSetPanId(mInstance, panId);
    }

exit:
    return error;
}

otError Interpreter::ProcessParent(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError      error = OT_ERROR_NONE;
    otRouterInfo parentInfo;

    SuccessOrExit(error = otThreadGetParentInfo(mInstance, &parentInfo));
    OutputFormat("Ext Addr: ");
    OutputExtAddress(parentInfo.mExtAddress);
    OutputLine("");
    OutputLine("Rloc: %x", parentInfo.mRloc16);
    OutputLine("Link Quality In: %d", parentInfo.mLinkQualityIn);
    OutputLine("Link Quality Out: %d", parentInfo.mLinkQualityOut);
    OutputLine("Age: %d", parentInfo.mAge);

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessParentPriority(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetParentPriority(mInstance));
    }
    else
    {
        int8_t priority;

        SuccessOrExit(error = ParseAsInt8(aArgs[0], priority));
        error = otThreadSetParentPriority(mInstance, priority);
    }

exit:
    return error;
}
#endif

void Interpreter::HandleIcmpReceive(void *               aContext,
                                    otMessage *          aMessage,
                                    const otMessageInfo *aMessageInfo,
                                    const otIcmp6Header *aIcmpHeader)
{
    static_cast<Interpreter *>(aContext)->HandleIcmpReceive(aMessage, aMessageInfo, aIcmpHeader);
}

void Interpreter::HandleIcmpReceive(otMessage *          aMessage,
                                    const otMessageInfo *aMessageInfo,
                                    const otIcmp6Header *aIcmpHeader)
{
    uint32_t timestamp = 0;
    uint16_t dataSize;

    VerifyOrExit(aIcmpHeader->mType == OT_ICMP6_TYPE_ECHO_REPLY);
    VerifyOrExit((mPingIdentifier != 0) && (mPingIdentifier == HostSwap16(aIcmpHeader->mData.m16[0])));

    dataSize = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    OutputFormat("%u bytes from ", dataSize + static_cast<uint16_t>(sizeof(otIcmp6Header)));

    OutputIp6Address(aMessageInfo->mPeerAddr);

    OutputFormat(": icmp_seq=%d hlim=%d", HostSwap16(aIcmpHeader->mData.m16[1]), aMessageInfo->mHopLimit);

    if (otMessageRead(aMessage, otMessageGetOffset(aMessage), &timestamp, sizeof(uint32_t)) == sizeof(uint32_t))
    {
        OutputFormat(" time=%dms", TimerMilli::GetNow().GetValue() - HostSwap32(timestamp));
    }

    OutputLine("");

    SignalPingReply(static_cast<const Ip6::MessageInfo *>(aMessageInfo)->GetPeerAddr(), dataSize, HostSwap32(timestamp),
                    aMessageInfo->mHopLimit);

exit:
    return;
}

otError Interpreter::ProcessPing(uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint8_t  index = 1;
    uint32_t interval;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "stop") == 0)
    {
        mPingIdentifier = 0;
        VerifyOrExit(mPingTimer.IsRunning(), error = OT_ERROR_INVALID_STATE);
        mPingTimer.Stop();
        ExitNow();
    }

    VerifyOrExit(!mPingTimer.IsRunning(), error = OT_ERROR_BUSY);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[0], mPingDestAddress));

    mPingLength            = kDefaultPingLength;
    mPingCount             = kDefaultPingCount;
    mPingInterval          = kDefaultPingInterval;
    mPingHopLimit          = 0;
    mPingAllowZeroHopLimit = false;

    while (index < aArgsLength)
    {
        switch (index)
        {
        case 1:
            SuccessOrExit(error = ParseAsUint16(aArgs[index], mPingLength));
            break;

        case 2:
            SuccessOrExit(error = ParseAsUint16(aArgs[index], mPingCount));
            break;

        case 3:
            SuccessOrExit(error = ParsePingInterval(aArgs[index], interval));
            VerifyOrExit(0 < interval && interval <= Timer::kMaxDelay, error = OT_ERROR_INVALID_ARGS);
            mPingInterval = interval;
            break;

        case 4:
            SuccessOrExit(error = ParseAsUint8(aArgs[index], mPingHopLimit));
            mPingAllowZeroHopLimit = (mPingHopLimit == 0);
            break;

        default:
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        index++;
    }

    mPingIdentifier++;

    if (mPingIdentifier == 0)
    {
        mPingIdentifier++;
    }

    SendPing();

exit:
    return error;
}

void Interpreter::HandlePingTimer(Timer &aTimer)
{
    GetOwner(aTimer).SendPing();
}

void Interpreter::SendPing(void)
{
    uint32_t      timestamp = HostSwap32(TimerMilli::GetNow().GetValue());
    otMessage *   message   = nullptr;
    otMessageInfo messageInfo;

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr          = mPingDestAddress;
    messageInfo.mHopLimit          = mPingHopLimit;
    messageInfo.mAllowZeroHopLimit = mPingAllowZeroHopLimit;

    message = otIp6NewMessage(mInstance, nullptr);
    VerifyOrExit(message != nullptr);

    SuccessOrExit(otMessageAppend(message, &timestamp, sizeof(timestamp)));
    SuccessOrExit(otMessageSetLength(message, mPingLength));
    SuccessOrExit(otIcmp6SendEchoRequest(mInstance, message, &messageInfo, mPingIdentifier));

    SignalPingRequest(static_cast<Ip6::MessageInfo *>(&messageInfo)->GetPeerAddr(), mPingLength, HostSwap32(timestamp),
                      messageInfo.mHopLimit);

    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }

    if (--mPingCount)
    {
        mPingTimer.Start(mPingInterval);
    }
}

otError Interpreter::ProcessPollPeriod(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otLinkGetPollPeriod(mInstance));
    }
    else
    {
        uint32_t pollPeriod;

        SuccessOrExit(error = ParseAsUint32(aArgs[0], pollPeriod));
        error = otLinkSetPollPeriod(mInstance, pollPeriod);
    }

exit:
    return error;
}

otError Interpreter::ProcessPromiscuous(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputEnabledDisabledStatus(otLinkIsPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance));
    }
    else
    {
        if (strcmp(aArgs[0], "enable") == 0)
        {
            SuccessOrExit(error = otLinkSetPromiscuous(mInstance, true));
            otLinkSetPcapCallback(mInstance, &HandleLinkPcapReceive, this);
        }
        else if (strcmp(aArgs[0], "disable") == 0)
        {
            otLinkSetPcapCallback(mInstance, nullptr, nullptr);
            SuccessOrExit(error = otLinkSetPromiscuous(mInstance, false));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkPcapReceive(aFrame, aIsTx);
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx)
{
    OT_UNUSED_VARIABLE(aIsTx);

    OutputLine("");

    for (size_t i = 0; i < 44; i++)
    {
        OutputFormat("=");
    }

    OutputFormat("[len = %3u]", aFrame->mLength);

    for (size_t i = 0; i < 28; i++)
    {
        OutputFormat("=");
    }

    OutputLine("");

    for (size_t i = 0; i < aFrame->mLength; i += 16)
    {
        OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                OutputFormat(" %02X", aFrame->mPsdu[i + j]);
            }
            else
            {
                OutputFormat(" ..");
            }
        }

        OutputFormat("|");

        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < aFrame->mLength)
            {
                if (31 < aFrame->mPsdu[i + j] && aFrame->mPsdu[i + j] < 127)
                {
                    OutputFormat(" %c", aFrame->mPsdu[i + j]);
                }
                else
                {
                    OutputFormat(" ?");
                }
            }
            else
            {
                OutputFormat(" .");
            }
        }

        OutputLine("|");
    }

    for (size_t i = 0; i < 83; i++)
    {
        OutputFormat("-");
    }

    OutputLine("");
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
otError Interpreter::ProcessPrefixAdd(uint8_t aArgsLength, char *aArgs[])
{
    otError              error = OT_ERROR_NONE;
    otBorderRouterConfig config;
    uint8_t              argcur = 0;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&config, 0, sizeof(otBorderRouterConfig));

    SuccessOrExit(error = ParseAsIp6Prefix(aArgs[argcur], config.mPrefix));
    argcur++;

    for (; argcur < aArgsLength; argcur++)
    {
        if (strcmp(aArgs[argcur], "high") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
        }
        else if (strcmp(aArgs[argcur], "med") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_MED;
        }
        else if (strcmp(aArgs[argcur], "low") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_LOW;
        }
        else
        {
            for (char *arg = aArgs[argcur]; *arg != '\0'; arg++)
            {
                switch (*arg)
                {
                case 'p':
                    config.mPreferred = true;
                    break;

                case 'a':
                    config.mSlaac = true;
                    break;

                case 'd':
                    config.mDhcp = true;
                    break;

                case 'c':
                    config.mConfigure = true;
                    break;

                case 'r':
                    config.mDefaultRoute = true;
                    break;

                case 'o':
                    config.mOnMesh = true;
                    break;

                case 's':
                    config.mStable = true;
                    break;

                case 'n':
                    config.mNdDns = true;
                    break;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
                case 'D':
                    config.mDp = true;
                    break;
#endif
                default:
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }
        }
    }

    error = otBorderRouterAddOnMeshPrefix(mInstance, &config);

exit:
    return error;
}

otError Interpreter::ProcessPrefixRemove(uint8_t aArgsLength, char *aArgs[])
{
    otError     error = OT_ERROR_NONE;
    otIp6Prefix prefix;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Prefix(aArgs[0], prefix));

    error = otBorderRouterRemoveOnMeshPrefix(mInstance, &prefix);

exit:
    return error;
}

otError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;

    while (otBorderRouterGetNextOnMeshPrefix(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mNetworkData.OutputPrefix(config);
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (otBackboneRouterGetState(mInstance) == OT_BACKBONE_ROUTER_STATE_DISABLED)
    {
        SuccessOrExit(otBackboneRouterGetDomainPrefix(mInstance, &config));
        OutputFormat("- ");
        mNetworkData.OutputPrefix(config);
    }
    // Else already printed via above while loop.
exit:
#endif

    return OT_ERROR_NONE;
}

otError Interpreter::ProcessPrefix(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = ProcessPrefixList());
    }
    else if (strcmp(aArgs[0], "add") == 0)
    {
        SuccessOrExit(error = ProcessPrefixAdd(aArgsLength - 1, aArgs + 1));
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        SuccessOrExit(error = ProcessPrefixRemove(aArgsLength - 1, aArgs + 1));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessPreferRouterId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t routerId;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = ParseAsUint8(aArgs[0], routerId));
    error = otThreadSetPreferredRouterId(mInstance, routerId);

exit:
    return error;
}
#endif

otError Interpreter::ProcessRcp(uint8_t aArgsLength, char *aArgs[])
{
    otError     error   = OT_ERROR_NONE;
    const char *version = otPlatRadioGetVersionString(mInstance);

    VerifyOrExit(version != otGetVersionString(), error = OT_ERROR_NOT_IMPLEMENTED);
    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "version") == 0)
    {
        OutputLine("%s", version);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Interpreter::ProcessRegion(uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint16_t regionCode;

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = otPlatRadioGetRegion(mInstance, &regionCode));
        OutputLine("%c%c", regionCode >> 8, regionCode & 0xff);
    }
    else
    {
        VerifyOrExit(strlen(aArgs[0]) == 2, error = OT_ERROR_INVALID_ARGS);

        regionCode =
            static_cast<uint16_t>(static_cast<uint16_t>(aArgs[0][0]) << 8) + static_cast<uint16_t>(aArgs[0][1]);
        error = otPlatRadioSetRegion(mInstance, regionCode);
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessReleaseRouterId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    uint8_t routerId;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsUint8(aArgs[0], routerId));
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, routerId));

exit:
    return error;
}
#endif

otError Interpreter::ProcessReset(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceReset(mInstance);

    return OT_ERROR_NONE;
}

otError Interpreter::ProcessRloc16(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%04x", otThreadGetRloc16(mInstance));

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
otError Interpreter::ProcessRouteAdd(uint8_t aArgsLength, char *aArgs[])
{
    otError               error = OT_ERROR_NONE;
    otExternalRouteConfig config;
    uint8_t               argcur = 0;

    memset(&config, 0, sizeof(otExternalRouteConfig));

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Prefix(aArgs[argcur], config.mPrefix));
    argcur++;

    for (; argcur < aArgsLength; argcur++)
    {
        if (strcmp(aArgs[argcur], "s") == 0)
        {
            config.mStable = true;
        }
        else if (strcmp(aArgs[argcur], "high") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_HIGH;
        }
        else if (strcmp(aArgs[argcur], "med") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_MED;
        }
        else if (strcmp(aArgs[argcur], "low") == 0)
        {
            config.mPreference = OT_ROUTE_PREFERENCE_LOW;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    error = otBorderRouterAddRoute(mInstance, &config);

exit:
    return error;
}

otError Interpreter::ProcessRouteRemove(uint8_t aArgsLength, char *aArgs[])
{
    otError     error = OT_ERROR_NONE;
    otIp6Prefix prefix;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Prefix(aArgs[0], prefix));

    error = otBorderRouterRemoveRoute(mInstance, &prefix);

exit:
    return error;
}

otError Interpreter::ProcessRouteList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;

    while (otBorderRouterGetNextRoute(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        mNetworkData.OutputRoute(config);
    }

    return OT_ERROR_NONE;
}

otError Interpreter::ProcessRoute(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = ProcessRouteList());
    }
    else if (strcmp(aArgs[0], "add") == 0)
    {
        SuccessOrExit(error = ProcessRouteAdd(aArgsLength - 1, aArgs + 1));
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        SuccessOrExit(error = ProcessRouteRemove(aArgsLength - 1, aArgs + 1));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
otError Interpreter::ProcessRouter(uint8_t aArgsLength, char *aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    uint16_t     routerId;
    bool         isTable;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(aArgs[0], "table") == 0);

    if (isTable || strcmp(aArgs[0], "list") == 0)
    {
        uint8_t maxRouterId;

        if (isTable)
        {
            OutputLine("| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     | Link |");
            OutputLine("+----+--------+----------+-----------+-------+--------+-----+------------------+------+");
        }

        maxRouterId = otThreadGetMaxRouterId(mInstance);

        for (uint8_t i = 0; i <= maxRouterId; i++)
        {
            if (otThreadGetRouterInfo(mInstance, i, &routerInfo) != OT_ERROR_NONE)
            {
                continue;
            }

            if (isTable)
            {
                OutputFormat("| %2d ", routerInfo.mRouterId);
                OutputFormat("| 0x%04x ", routerInfo.mRloc16);
                OutputFormat("| %8d ", routerInfo.mNextHop);
                OutputFormat("| %9d ", routerInfo.mPathCost);
                OutputFormat("| %5d ", routerInfo.mLinkQualityIn);
                OutputFormat("| %6d ", routerInfo.mLinkQualityOut);
                OutputFormat("| %3d ", routerInfo.mAge);
                OutputFormat("| ");
                OutputExtAddress(routerInfo.mExtAddress);
                OutputLine(" | %4d |", routerInfo.mLinkEstablished);
            }
            else
            {
                OutputFormat("%d ", i);
            }
        }

        OutputLine("");
        ExitNow();
    }

    SuccessOrExit(error = ParseAsUint16(aArgs[0], routerId));
    SuccessOrExit(error = otThreadGetRouterInfo(mInstance, routerId, &routerInfo));

    OutputLine("Alloc: %d", routerInfo.mAllocated);

    if (routerInfo.mAllocated)
    {
        OutputLine("Router ID: %d", routerInfo.mRouterId);
        OutputLine("Rloc: %04x", routerInfo.mRloc16);
        OutputLine("Next Hop: %04x", static_cast<uint16_t>(routerInfo.mNextHop) << 10);
        OutputLine("Link: %d", routerInfo.mLinkEstablished);

        if (routerInfo.mLinkEstablished)
        {
            OutputFormat("Ext Addr: ");
            OutputExtAddress(routerInfo.mExtAddress);
            OutputLine("");
            OutputLine("Cost: %d", routerInfo.mPathCost);
            OutputLine("Link Quality In: %d", routerInfo.mLinkQualityIn);
            OutputLine("Link Quality Out: %d", routerInfo.mLinkQualityOut);
            OutputLine("Age: %d", routerInfo.mAge);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterDowngradeThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterDowngradeThreshold(mInstance));
    }
    else
    {
        uint8_t threshold;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], threshold));
        otThreadSetRouterDowngradeThreshold(mInstance, threshold);
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterEligible(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputEnabledDisabledStatus(otThreadIsRouterEligible(mInstance));
    }
    else if (strcmp(aArgs[0], "enable") == 0)
    {
        error = otThreadSetRouterEligible(mInstance, true);
    }
    else if (strcmp(aArgs[0], "disable") == 0)
    {
        error = otThreadSetRouterEligible(mInstance, false);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterSelectionJitter(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterSelectionJitter(mInstance));
    }
    else
    {
        uint8_t jitter;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], jitter));
        otThreadSetRouterSelectionJitter(mInstance, jitter);
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterUpgradeThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        uint8_t threshold;

        SuccessOrExit(error = ParseAsUint8(aArgs[0], threshold));
        otThreadSetRouterUpgradeThreshold(mInstance, threshold);
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

otError Interpreter::ProcessScan(uint8_t aArgsLength, char *aArgs[])
{
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    uint16_t scanDuration = 0;
    bool     energyScan   = false;
    uint8_t  index        = 0;

    if (aArgsLength > 0)
    {
        if (strcmp(aArgs[index], "energy") == 0)
        {
            energyScan = true;
            index++;

            if (aArgsLength > 1)
            {
                SuccessOrExit(error = ParseAsUint16(aArgs[index++], scanDuration));
            }
        }

        if (index < aArgsLength)
        {
            uint8_t channel;

            SuccessOrExit(error = ParseAsUint8(aArgs[index++], channel));
            VerifyOrExit(channel < sizeof(scanChannels) * CHAR_BIT, error = OT_ERROR_INVALID_ARGS);
            scanChannels = 1 << channel;
        }
    }

    if (energyScan)
    {
        OutputLine("| Ch | RSSI |");
        OutputLine("+----+------+");
        SuccessOrExit(error = otLinkEnergyScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::HandleEnergyScanResult, this));
    }
    else
    {
        OutputLine("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |");
        OutputLine("+---+------------------+------------------+------+------------------+----+-----+-----+");
        SuccessOrExit(error = otLinkActiveScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::HandleActiveScanResult, this));
    }

    error = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleActiveScanResult(aResult);
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult)
{
    if (aResult == nullptr)
    {
        OutputResult(OT_ERROR_NONE);
        ExitNow();
    }

    OutputFormat("| %d ", aResult->mIsJoinable);

    OutputFormat("| %-16s ", aResult->mNetworkName.m8);

    OutputFormat("| ");
    OutputBytes(aResult->mExtendedPanId.m8);
    OutputFormat(" ");

    OutputFormat("| %04x | ", aResult->mPanId);
    OutputExtAddress(aResult->mExtAddress);
    OutputFormat(" | %2d ", aResult->mChannel);
    OutputFormat("| %3d ", aResult->mRssi);
    OutputLine("| %3d |", aResult->mLqi);

exit:
    return;
}

void Interpreter::HandleEnergyScanResult(otEnergyScanResult *aResult, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleEnergyScanResult(aResult);
}

void Interpreter::HandleEnergyScanResult(otEnergyScanResult *aResult)
{
    if (aResult == nullptr)
    {
        OutputResult(OT_ERROR_NONE);
        ExitNow();
    }

    OutputLine("| %2d | %4d |", aResult->mChannel, aResult->mMaxRssi);

exit:
    return;
}

otError Interpreter::ProcessSingleton(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    if (otThreadIsSingleton(mInstance))
    {
        OutputLine("true");
    }
    else
    {
        OutputLine("false");
    }

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
otError Interpreter::ProcessSntp(uint8_t aArgsLength, char *aArgs[])
{
    otError          error = OT_ERROR_NONE;
    uint16_t         port  = OT_SNTP_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otSntpQuery      query;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "query") == 0)
    {
        VerifyOrExit(!mSntpQueryingInProgress, error = OT_ERROR_BUSY);

        if (aArgsLength > 1)
        {
            SuccessOrExit(error = ParseAsIp6Address(aArgs[1], messageInfo.GetPeerAddr()));
        }
        else
        {
            // Use IPv6 address of default SNTP server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_SNTP_DEFAULT_SERVER_IP));
        }

        if (aArgsLength > 2)
        {
            SuccessOrExit(error = ParseAsUint16(aArgs[2], port));
        }

        messageInfo.SetPeerPort(port);

        query.mMessageInfo = static_cast<const otMessageInfo *>(&messageInfo);

        SuccessOrExit(error = otSntpClientQuery(mInstance, &query, &Interpreter::HandleSntpResponse, this));

        mSntpQueryingInProgress = true;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

    error = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::HandleSntpResponse(void *aContext, uint64_t aTime, otError aResult)
{
    static_cast<Interpreter *>(aContext)->HandleSntpResponse(aTime, aResult);
}

void Interpreter::HandleSntpResponse(uint64_t aTime, otError aResult)
{
    if (aResult == OT_ERROR_NONE)
    {
        // Some Embedded C libraries do not support printing of 64-bit unsigned integers.
        // To simplify, unix epoch time and era number are printed separately.
        OutputLine("SNTP response - Unix time: %u (era: %u)", static_cast<uint32_t>(aTime),
                   static_cast<uint32_t>(aTime >> 32));
    }
    else
    {
        OutputLine("SNTP error - %s", otThreadErrorToString(aResult));
    }

    mSntpQueryingInProgress = false;

    OutputResult(OT_ERROR_NONE);
}
#endif // OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
otError Interpreter::ProcessSrp(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
        OutputLine("client");
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
        OutputLine("server");
#endif
        ExitNow();
    }

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    if (strcmp(aArgs[0], "client") == 0)
    {
        ExitNow(error = mSrpClient.Process(aArgsLength - 1, aArgs + 1));
    }
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    if (strcmp(aArgs[0], "server") == 0)
    {
        ExitNow(error = mSrpServer.Process(aArgsLength - 1, aArgs + 1));
    }
#endif

    error = OT_ERROR_INVALID_COMMAND;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

otError Interpreter::ProcessState(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        switch (otThreadGetDeviceRole(mInstance))
        {
        case OT_DEVICE_ROLE_DISABLED:
            OutputLine("disabled");
            break;

        case OT_DEVICE_ROLE_DETACHED:
            OutputLine("detached");
            break;

        case OT_DEVICE_ROLE_CHILD:
            OutputLine("child");
            break;

#if OPENTHREAD_FTD
        case OT_DEVICE_ROLE_ROUTER:
            OutputLine("router");
            break;

        case OT_DEVICE_ROLE_LEADER:
            OutputLine("leader");
            break;
#endif

        default:
            OutputLine("invalid state");
            break;
        }
    }
    else
    {
        if (strcmp(aArgs[0], "detached") == 0)
        {
            SuccessOrExit(error = otThreadBecomeDetached(mInstance));
        }
        else if (strcmp(aArgs[0], "child") == 0)
        {
            SuccessOrExit(error = otThreadBecomeChild(mInstance));
        }

#if OPENTHREAD_FTD
        else if (strcmp(aArgs[0], "router") == 0)
        {
            SuccessOrExit(error = otThreadBecomeRouter(mInstance));
        }
        else if (strcmp(aArgs[0], "leader") == 0)
        {
            SuccessOrExit(error = otThreadBecomeLeader(mInstance));
        }
#endif
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessThread(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "start") == 0)
    {
        SuccessOrExit(error = otThreadSetEnabled(mInstance, true));
    }
    else if (strcmp(aArgs[0], "stop") == 0)
    {
        SuccessOrExit(error = otThreadSetEnabled(mInstance, false));
    }
    else if (strcmp(aArgs[0], "version") == 0)
    {
        OutputLine("%u", otThreadGetVersion());
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

otError Interpreter::ProcessDataset(uint8_t aArgsLength, char *aArgs[])
{
    return mDataset.Process(aArgsLength, aArgs);
}

otError Interpreter::ProcessTxPower(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    int8_t  power;

    if (aArgsLength == 0)
    {
        SuccessOrExit(error = otPlatRadioGetTransmitPower(mInstance, &power));
        OutputLine("%d dBm", power);
    }
    else
    {
        SuccessOrExit(error = ParseAsInt8(aArgs[0], power));
        SuccessOrExit(error = otPlatRadioSetTransmitPower(mInstance, power));
    }

exit:
    return error;
}

otError Interpreter::ProcessUdp(uint8_t aArgsLength, char *aArgs[])
{
    return mUdp.Process(aArgsLength, aArgs);
}

otError Interpreter::ProcessUnsecurePort(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength >= 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "add") == 0)
    {
        uint16_t port;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = ParseAsUint16(aArgs[1], port));
        SuccessOrExit(error = otIp6AddUnsecurePort(mInstance, port));
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

        if (strcmp(aArgs[1], "all") == 0)
        {
            otIp6RemoveAllUnsecurePorts(mInstance);
        }
        else
        {
            uint16_t port;

            SuccessOrExit(error = ParseAsUint16(aArgs[1], port));
            SuccessOrExit(error = otIp6RemoveUnsecurePort(mInstance, port));
        }
    }
    else if (strcmp(aArgs[0], "get") == 0)
    {
        const uint16_t *ports;
        uint8_t         numPorts;

        ports = otIp6GetUnsecurePorts(mInstance, &numPorts);

        if (ports != nullptr)
        {
            for (uint8_t i = 0; i < numPorts; i++)
            {
                OutputFormat("%d ", ports[i]);
            }
        }

        OutputLine("");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

otError Interpreter::ProcessVersion(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%s", otGetVersionString());
        ExitNow();
    }

    if (strcmp(aArgs[0], "api") == 0)
    {
        OutputLine("%d", OPENTHREAD_API_VERSION);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
otError Interpreter::ProcessCommissioner(uint8_t aArgsLength, char *aArgs[])
{
    return mCommissioner.Process(aArgsLength, aArgs);
}
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
otError Interpreter::ProcessJoiner(uint8_t aArgsLength, char *aArgs[])
{
    return mJoiner.Process(aArgsLength, aArgs);
}
#endif

#if OPENTHREAD_FTD
otError Interpreter::ProcessJoinerPort(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetJoinerUdpPort(mInstance));
    }
    else
    {
        uint16_t port;

        SuccessOrExit(error = ParseAsUint16(aArgs[0], port));
        error = otThreadSetJoinerUdpPort(mInstance, port);
    }

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
otError Interpreter::ProcessMacFilter(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        PrintMacFilter();
    }
    else
    {
        if (strcmp(aArgs[0], "addr") == 0)
        {
            error = ProcessMacFilterAddress(aArgsLength - 1, aArgs + 1);
        }
        else if (strcmp(aArgs[0], "rss") == 0)
        {
            error = ProcessMacFilterRss(aArgsLength - 1, aArgs + 1);
        }
        else
        {
            error = OT_ERROR_INVALID_COMMAND;
        }
    }

    return error;
}

void Interpreter::PrintMacFilter(void)
{
    otMacFilterEntry       entry;
    otMacFilterIterator    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterAddressMode mode     = otLinkFilterGetAddressMode(mInstance);

    if (mode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
    {
        OutputLine("Address Mode: Disabled");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST)
    {
        OutputLine("Address Mode: Allowlist");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_DENYLIST)
    {
        OutputLine("Address Mode: Denylist");
    }

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        OutputExtAddress(entry.mExtAddress);

        if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            OutputFormat(" : rss %d (lqi %d)", entry.mRssIn, otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }

        OutputLine("");
    }

    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    OutputLine("RssIn List:");

    while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        uint8_t i = 0;

        for (; i < OT_EXT_ADDRESS_SIZE; i++)
        {
            if (entry.mExtAddress.m8[i] != 0xff)
            {
                break;
            }
        }

        if (i == OT_EXT_ADDRESS_SIZE)
        {
            OutputLine("Default rss : %d (lqi %d)", entry.mRssIn,
                       otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }
        else
        {
            OutputExtAddress(entry.mExtAddress);
            OutputLine(" : rss %d (lqi %d)", entry.mRssIn, otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }
    }
}

otError Interpreter::ProcessMacFilterAddress(uint8_t aArgsLength, char *aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otExtAddress           extAddr;
    otMacFilterEntry       entry;
    otMacFilterIterator    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterAddressMode mode     = otLinkFilterGetAddressMode(mInstance);

    if (aArgsLength == 0)
    {
        if (mode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
        {
            OutputLine("Disabled");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST)
        {
            OutputLine("Allowlist");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_DENYLIST)
        {
            OutputLine("Denylist");
        }

        while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            OutputExtAddress(entry.mExtAddress);

            if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
            {
                OutputFormat(" : rss %d (lqi %d)", entry.mRssIn,
                             otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }

            OutputLine("");
        }
    }
    else
    {
        if (strcmp(aArgs[0], "disable") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_DISABLED);
        }
        else if (strcmp(aArgs[0], "allowlist") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST);
        }
        else if (strcmp(aArgs[0], "denylist") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_DENYLIST);
        }
        else if (strcmp(aArgs[0], "add") == 0)
        {
            VerifyOrExit(aArgsLength >= 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddr.m8));
            error = otLinkFilterAddAddress(mInstance, &extAddr);

            VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

            if (aArgsLength > 2)
            {
                int8_t rss;

                VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);
                SuccessOrExit(error = ParseAsInt8(aArgs[2], rss));
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(aArgs[0], "remove") == 0)
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddr.m8));
            otLinkFilterRemoveAddress(mInstance, &extAddr);
        }
        else if (strcmp(aArgs[0], "clear") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterClearAddresses(mInstance);
        }
        else
        {
            error = OT_ERROR_INVALID_COMMAND;
        }
    }

exit:
    return error;
}

otError Interpreter::ProcessMacFilterRss(uint8_t aArgsLength, char *aArgs[])
{
    otError             error = OT_ERROR_NONE;
    otMacFilterEntry    entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otExtAddress        extAddr;
    int8_t              rss;

    if (aArgsLength == 0)
    {
        while (otLinkFilterGetNextRssIn(mInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            uint8_t i = 0;

            for (; i < OT_EXT_ADDRESS_SIZE; i++)
            {
                if (entry.mExtAddress.m8[i] != 0xff)
                {
                    break;
                }
            }

            if (i == OT_EXT_ADDRESS_SIZE)
            {
                OutputLine("Default rss: %d (lqi %d)", entry.mRssIn,
                           otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
            else
            {
                OutputExtAddress(entry.mExtAddress);
                OutputLine(" : rss %d (lqi %d)", entry.mRssIn, otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
        }
    }
    else
    {
        if (strcmp(aArgs[0], "add-lqi") == 0)
        {
            uint8_t linkQuality;

            VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);

            SuccessOrExit(error = ParseAsUint8(aArgs[2], linkQuality));
            VerifyOrExit(linkQuality <= 3, error = OT_ERROR_INVALID_ARGS);
            rss = otLinkConvertLinkQualityToRss(mInstance, linkQuality);

            if (strcmp(aArgs[1], "*") == 0)
            {
                otLinkFilterSetDefaultRssIn(mInstance, rss);
            }
            else
            {
                SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddr.m8));
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(aArgs[0], "add") == 0)
        {
            VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseAsInt8(aArgs[2], rss));

            if (strcmp(aArgs[1], "*") == 0)
            {
                otLinkFilterSetDefaultRssIn(mInstance, rss);
            }
            else
            {
                SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddr.m8));
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(aArgs[0], "remove") == 0)
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

            if (strcmp(aArgs[1], "*") == 0)
            {
                otLinkFilterClearDefaultRssIn(mInstance);
            }
            else
            {
                SuccessOrExit(error = ParseAsHexString(aArgs[1], extAddr.m8));
                otLinkFilterRemoveRssIn(mInstance, &extAddr);
            }
        }
        else if (strcmp(aArgs[0], "clear") == 0)
        {
            otLinkFilterClearAllRssIn(mInstance);
        }
        else
        {
            error = OT_ERROR_INVALID_COMMAND;
        }
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

otError Interpreter::ProcessMac(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "retries") == 0)
    {
        error = ProcessMacRetries(aArgsLength - 1, aArgs + 1);
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (strcmp(aArgs[0], "send") == 0)
    {
        error = ProcessMacSend(aArgsLength - 1, aArgs + 1);
    }
#endif
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError Interpreter::ProcessMacRetries(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0 && aArgsLength <= 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "direct") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otLinkGetMaxFrameRetriesDirect(mInstance));
        }
        else
        {
            uint8_t retries;

            SuccessOrExit(error = ParseAsUint8(aArgs[1], retries));
            otLinkSetMaxFrameRetriesDirect(mInstance, retries);
        }
    }
#if OPENTHREAD_FTD
    else if (strcmp(aArgs[0], "indirect") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otLinkGetMaxFrameRetriesIndirect(mInstance));
        }
        else
        {
            uint8_t retries;

            SuccessOrExit(error = ParseAsUint8(aArgs[1], retries));
            otLinkSetMaxFrameRetriesIndirect(mInstance, retries);
        }
    }
#endif
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError Interpreter::ProcessMacSend(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    VerifyOrExit(aArgsLength == 1);

    if (strcmp(aArgs[0], "datarequest") == 0)
    {
        error = otLinkSendDataRequest(mInstance);
    }
    else if (strcmp(aArgs[0], "emptydata") == 0)
    {
        error = otLinkSendEmptyData(mInstance);
    }

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
otError Interpreter::ProcessDiag(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];

    // all diagnostics related features are processed within diagnostics module
    output[0]                  = '\0';
    output[sizeof(output) - 1] = '\0';

    error = otDiagProcessCmd(mInstance, aArgsLength, aArgs, output, sizeof(output) - 1);
    Output(output, static_cast<uint16_t>(strlen(output)));

    return error;
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength)
{
    char *         args[kMaxArgs] = {nullptr};
    uint8_t        argsLength;
    const Command *command;

    VerifyOrExit(aBuf != nullptr && StringLength(aBuf, aBufLength + 1) <= aBufLength);

    VerifyOrExit(Utils::CmdLineParser::ParseCmd(aBuf, argsLength, args, kMaxArgs) == OT_ERROR_NONE,
                 OutputLine("Error: too many args (max %d)", kMaxArgs));
    VerifyOrExit(argsLength >= 1, OutputLine("Error: no given command."));

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    VerifyOrExit((!otDiagIsEnabled(mInstance) || (strcmp(args[0], "diag") == 0)),
                 OutputLine("under diagnostics mode, execute 'diag stop' before running any other commands."));
#endif

    command = Utils::LookupTable::Find(args[0], sCommands);

    if (command != nullptr)
    {
        OutputResult((this->*command->mHandler)(argsLength - 1, &args[1]));
        ExitNow();
    }

    // Check user defined commands if built-in command has not been found
    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        if (strcmp(args[0], mUserCommands[i].mName) == 0)
        {
            mUserCommands[i].mCommand(mUserCommandsContext, argsLength - 1, &args[1]);
            ExitNow();
        }
    }

    OutputResult(OT_ERROR_INVALID_COMMAND);

exit:
    return;
}

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
otError Interpreter::ProcessNetworkDiagnostic(uint8_t aArgsLength, char *aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otIp6Address address;
    uint8_t      tlvTypes[OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];
    uint8_t      count     = 0;
    uint8_t      argsIndex = 0;

    // Include operation, address and type tlv list.
    VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseAsIp6Address(aArgs[1], address));

    argsIndex = 2;

    while (argsIndex < aArgsLength && count < sizeof(tlvTypes))
    {
        SuccessOrExit(error = ParseAsUint8(aArgs[argsIndex++], tlvTypes[count++]));
    }

    if (strcmp(aArgs[0], "get") == 0)
    {
        IgnoreError(otThreadSendDiagnosticGet(mInstance, &address, tlvTypes, count));
        ExitNow(error = OT_ERROR_PENDING);
    }
    else if (strcmp(aArgs[0], "reset") == 0)
    {
        IgnoreError(otThreadSendDiagnosticReset(mInstance, &address, tlvTypes, count));
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void Interpreter::HandleDiagnosticGetResponse(otError              aError,
                                              otMessage *          aMessage,
                                              const otMessageInfo *aMessageInfo,
                                              void *               aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDiagnosticGetResponse(
        aError, aMessage, static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Interpreter::HandleDiagnosticGetResponse(otError                 aError,
                                              const otMessage *       aMessage,
                                              const Ip6::MessageInfo *aMessageInfo)
{
    uint8_t               buf[16];
    uint16_t              bytesToPrint;
    uint16_t              bytesPrinted = 0;
    uint16_t              length;
    otNetworkDiagTlv      diagTlv;
    otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;

    SuccessOrExit(aError);

    OutputFormat("DIAG_GET.rsp/ans from ");
    OutputIp6Address(aMessageInfo->mPeerAddr);
    OutputFormat(": ");

    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    while (length > 0)
    {
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);

        OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

        length -= bytesToPrint;
        bytesPrinted += bytesToPrint;
    }

    OutputLine("");

    // Output Network Diagnostic TLV values in standard YAML format.
    while ((aError = otThreadGetNextDiagnosticTlv(aMessage, &iterator, &diagTlv)) == OT_ERROR_NONE)
    {
        switch (diagTlv.mType)
        {
        case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
            OutputFormat("Ext Address: '");
            OutputExtAddress(diagTlv.mData.mExtAddress);
            OutputLine("'");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
            OutputLine("Rloc16: 0x%04x", diagTlv.mData.mAddr16);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
            OutputLine("Mode:");
            OutputMode(kIndentSize, diagTlv.mData.mMode);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
            OutputLine("Timeout: %u", diagTlv.mData.mTimeout);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
            OutputLine("Connectivity:");
            OutputConnectivity(kIndentSize, diagTlv.mData.mConnectivity);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
            OutputLine("Route:");
            OutputRoute(kIndentSize, diagTlv.mData.mRoute);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
            OutputLine("Leader Data:");
            OutputLeaderData(kIndentSize, diagTlv.mData.mLeaderData);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
            OutputFormat("Network Data: '");
            OutputBytes(diagTlv.mData.mNetworkData.m8, diagTlv.mData.mNetworkData.mCount);
            OutputLine("'");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
            OutputLine("IP6 Address List:");
            for (uint16_t i = 0; i < diagTlv.mData.mIp6AddrList.mCount; ++i)
            {
                OutputFormat(kIndentSize, "- ");
                OutputIp6Address(diagTlv.mData.mIp6AddrList.mList[i]);
                OutputLine("");
            }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
            OutputLine("MAC Counters:");
            OutputNetworkDiagMacCounters(kIndentSize, diagTlv.mData.mMacCounters);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
            OutputLine("Battery Level: %u%%", diagTlv.mData.mBatteryLevel);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
            OutputLine("Supply Voltage: %umV", diagTlv.mData.mSupplyVoltage);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
            OutputLine("Child Table:");
            for (uint16_t i = 0; i < diagTlv.mData.mChildTable.mCount; ++i)
            {
                OutputFormat(kIndentSize, "- ");
                OutputChildTableEntry(kIndentSize + 2, diagTlv.mData.mChildTable.mTable[i]);
            }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
            OutputFormat("Channel Pages: '");
            OutputBytes(diagTlv.mData.mChannelPages.m8, diagTlv.mData.mChannelPages.mCount);
            OutputLine("'");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
            OutputLine("Max Child Timeout: %u", diagTlv.mData.mMaxChildTimeout);
            break;
        }
    }

    if (aError == OT_ERROR_NOT_FOUND)
    {
        aError = OT_ERROR_NONE;
    }

exit:
    OutputResult(aError);
}

void Interpreter::OutputMode(uint8_t aIndentSize, const otLinkModeConfig &aMode)
{
    OutputLine(aIndentSize, "RxOnWhenIdle: %d", aMode.mRxOnWhenIdle);
    OutputLine(aIndentSize, "DeviceType: %d", aMode.mDeviceType);
    OutputLine(aIndentSize, "NetworkData: %d", aMode.mNetworkData);
}

void Interpreter::OutputConnectivity(uint8_t aIndentSize, const otNetworkDiagConnectivity &aConnectivity)
{
    OutputLine(aIndentSize, "ParentPriority: %d", aConnectivity.mParentPriority);
    OutputLine(aIndentSize, "LinkQuality3: %u", aConnectivity.mLinkQuality3);
    OutputLine(aIndentSize, "LinkQuality2: %u", aConnectivity.mLinkQuality2);
    OutputLine(aIndentSize, "LinkQuality1: %u", aConnectivity.mLinkQuality1);
    OutputLine(aIndentSize, "LeaderCost: %u", aConnectivity.mLeaderCost);
    OutputLine(aIndentSize, "IdSequence: %u", aConnectivity.mIdSequence);
    OutputLine(aIndentSize, "ActiveRouters: %u", aConnectivity.mActiveRouters);
    OutputLine(aIndentSize, "SedBufferSize: %u", aConnectivity.mSedBufferSize);
    OutputLine(aIndentSize, "SedDatagramCount: %u", aConnectivity.mSedDatagramCount);
}
void Interpreter::OutputRoute(uint8_t aIndentSize, const otNetworkDiagRoute &aRoute)
{
    OutputLine(aIndentSize, "IdSequence: %u", aRoute.mIdSequence);
    OutputLine(aIndentSize, "RouteData:");

    aIndentSize += kIndentSize;
    for (uint16_t i = 0; i < aRoute.mRouteCount; ++i)
    {
        OutputFormat(aIndentSize, "- ");
        OutputRouteData(aIndentSize + 2, aRoute.mRouteData[i]);
    }
}

void Interpreter::OutputRouteData(uint8_t aIndentSize, const otNetworkDiagRouteData &aRouteData)
{
    OutputLine("RouteId: 0x%02x", aRouteData.mRouterId);

    OutputLine(aIndentSize, "LinkQualityOut: %u", aRouteData.mLinkQualityOut);
    OutputLine(aIndentSize, "LinkQualityIn: %u", aRouteData.mLinkQualityIn);
    OutputLine(aIndentSize, "RouteCost: %u", aRouteData.mRouteCost);
}

void Interpreter::OutputLeaderData(uint8_t aIndentSize, const otLeaderData &aLeaderData)
{
    OutputLine(aIndentSize, "PartitionId: 0x%08x", aLeaderData.mPartitionId);
    OutputLine(aIndentSize, "Weighting: %u", aLeaderData.mWeighting);
    OutputLine(aIndentSize, "DataVersion: %u", aLeaderData.mDataVersion);
    OutputLine(aIndentSize, "StableDataVersion: %u", aLeaderData.mStableDataVersion);
    OutputLine(aIndentSize, "LeaderRouterId: 0x%02x", aLeaderData.mLeaderRouterId);
}

void Interpreter::OutputNetworkDiagMacCounters(uint8_t aIndentSize, const otNetworkDiagMacCounters &aMacCounters)
{
    OutputLine(aIndentSize, "IfInUnknownProtos: %u", aMacCounters.mIfInUnknownProtos);
    OutputLine(aIndentSize, "IfInErrors: %u", aMacCounters.mIfInErrors);
    OutputLine(aIndentSize, "IfOutErrors: %u", aMacCounters.mIfOutErrors);
    OutputLine(aIndentSize, "IfInUcastPkts: %u", aMacCounters.mIfInUcastPkts);
    OutputLine(aIndentSize, "IfInBroadcastPkts: %u", aMacCounters.mIfInBroadcastPkts);
    OutputLine(aIndentSize, "IfInDiscards: %u", aMacCounters.mIfInDiscards);
    OutputLine(aIndentSize, "IfOutUcastPkts: %u", aMacCounters.mIfOutUcastPkts);
    OutputLine(aIndentSize, "IfOutBroadcastPkts: %u", aMacCounters.mIfOutBroadcastPkts);
    OutputLine(aIndentSize, "IfOutDiscards: %u", aMacCounters.mIfOutDiscards);
}

void Interpreter::OutputChildTableEntry(uint8_t aIndentSize, const otNetworkDiagChildEntry &aChildEntry)
{
    OutputLine("ChildId: 0x%04x", aChildEntry.mChildId);

    OutputLine(aIndentSize, "Timeout: %u", aChildEntry.mTimeout);
    OutputLine(aIndentSize, "Mode:");
    OutputMode(aIndentSize + kIndentSize, aChildEntry.mMode);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

void Interpreter::SetUserCommands(const otCliCommand *aCommands, uint8_t aLength, void *aContext)
{
    mUserCommands        = aCommands;
    mUserCommandsLength  = aLength;
    mUserCommandsContext = aContext;
}

Interpreter &Interpreter::GetOwner(InstanceLocator &aInstanceLocator)
{
    OT_UNUSED_VARIABLE(aInstanceLocator);
    return Interpreter::GetInterpreter();
}

void Interpreter::SignalPingRequest(const Ip6::Address &aPeerAddress,
                                    uint16_t            aPingLength,
                                    uint32_t            aTimestamp,
                                    uint8_t             aHopLimit)
{
    OT_UNUSED_VARIABLE(aPeerAddress);
    OT_UNUSED_VARIABLE(aPingLength);
    OT_UNUSED_VARIABLE(aTimestamp);
    OT_UNUSED_VARIABLE(aHopLimit);

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    mInstance->Get<Utils::Otns>().EmitPingRequest(aPeerAddress, aPingLength, aTimestamp, aHopLimit);
#endif
}

void Interpreter::SignalPingReply(const Ip6::Address &aPeerAddress,
                                  uint16_t            aPingLength,
                                  uint32_t            aTimestamp,
                                  uint8_t             aHopLimit)
{
    OT_UNUSED_VARIABLE(aPeerAddress);
    OT_UNUSED_VARIABLE(aPingLength);
    OT_UNUSED_VARIABLE(aTimestamp);
    OT_UNUSED_VARIABLE(aHopLimit);

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    mInstance->Get<Utils::Otns>().EmitPingReply(aPeerAddress, aPingLength, aTimestamp, aHopLimit);
#endif
}

void Interpreter::HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo &aInfo)
{
    OutputFormat("~ Discovery Request from ");
    OutputExtAddress(aInfo.mExtAddress);
    OutputLine(": version=%u,joiner=%d", aInfo.mVersion, aInfo.mIsJoiner);
}

int Interpreter::OutputFormat(const char *aFormat, ...)
{
    int     rval;
    va_list ap;

    va_start(ap, aFormat);
    rval = OutputFormatV(aFormat, ap);
    va_end(ap);

    return rval;
}

void Interpreter::OutputFormat(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list ap;

    OutputSpaces(aIndentSize);

    va_start(ap, aFormat);
    OutputFormatV(aFormat, ap);
    va_end(ap);
}

void Interpreter::OutputLine(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputFormat("\r\n");
}

void Interpreter::OutputLine(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list args;

    OutputSpaces(aIndentSize);

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputFormat("\r\n");
}

void Interpreter::OutputSpaces(uint8_t aCount)
{
    static const char kSpaces[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    while (aCount > 0)
    {
        uint8_t len = OT_MIN(aCount, sizeof(kSpaces));

        Output(kSpaces, len);
        aCount -= len;
    }
}

int Interpreter::OutputFormatV(const char *aFormat, va_list aArguments)
{
    char buf[kMaxLineLength];

    vsnprintf(buf, sizeof(buf), aFormat, aArguments);

    return Output(buf, static_cast<uint16_t>(strlen(buf)));
}

extern "C" void otCliSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength, void *aContext)
{
    Interpreter::GetInterpreter().SetUserCommands(aUserCommands, aLength, aContext);
}

extern "C" void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    Interpreter::GetInterpreter().OutputBytes(aBytes, aLength);
}

extern "C" void otCliOutputFormat(const char *aFmt, ...)
{
    va_list aAp;
    va_start(aAp, aFmt);
    Interpreter::GetInterpreter().OutputFormatV(aFmt, aAp);
    va_end(aAp);
}

extern "C" void otCliOutput(const char *aString, uint16_t aLength)
{
    Interpreter::GetInterpreter().Output(aString, aLength);
}

extern "C" void otCliAppendResult(otError aError)
{
    Interpreter::GetInterpreter().OutputResult(aError);
}

extern "C" void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    VerifyOrExit(Interpreter::IsInitialized());

    Interpreter::GetInterpreter().OutputFormatV(aFormat, aArgs);
    Interpreter::GetInterpreter().OutputLine("");
exit:
    return;
}

extern "C" void otCliPlatLogLine(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aLogLine)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    VerifyOrExit(Interpreter::IsInitialized());
    Interpreter::GetInterpreter().OutputLine(aLogLine);

exit:
    return;
}

} // namespace Cli
} // namespace ot

#if OPENTHREAD_CONFIG_LEGACY_ENABLE
OT_TOOL_WEAK void otNcpRegisterLegacyHandlers(const otNcpLegacyHandlers *aHandlers)
{
    OT_UNUSED_VARIABLE(aHandlers);
}

OT_TOOL_WEAK void otNcpHandleDidReceiveNewLegacyUlaPrefix(const uint8_t *aUlaPrefix)
{
    OT_UNUSED_VARIABLE(aUlaPrefix);
}

OT_TOOL_WEAK void otNcpHandleLegacyNodeDidJoin(const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aExtAddr);
}
#endif
