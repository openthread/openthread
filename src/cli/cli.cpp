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
#include "mac/channel_mask.hpp"
#include "utils/parse_cmdline.hpp"

#include <openthread/icmp6.h>
#include <openthread/link.h>
#include <openthread/ncp.h>
#include <openthread/thread.h>
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

#include <openthread/diag.h>
#include <openthread/icmp6.h>
#include <openthread/logging.h>
#include <openthread/platform/uart.h>
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
#include <openthread/platform/misc.h>
#endif

#include "common/new.hpp"
#include "net/ip6.hpp"
#include "utils/otns.hpp"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#include <openthread/backbone_router.h>
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
#include <openthread/backbone_router_ftd.h>
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
#include <openthread/link_metrics.h>
#endif
#endif

#include "cli_dataset.hpp"

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
#include "common/string.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

namespace Cli {

constexpr Interpreter::Command Interpreter::sCommands[];

Interpreter *Interpreter::sInterpreter = nullptr;

Interpreter::Interpreter(Instance *aInstance)
    : mUserCommands(nullptr)
    , mUserCommandsLength(0)
    , mPingLength(kDefaultPingLength)
    , mPingCount(kDefaultPingCount)
    , mPingInterval(kDefaultPingInterval)
    , mPingHopLimit(0)
    , mPingAllowZeroHopLimit(false)
    , mPingIdentifier(0)
    , mPingTimer(*aInstance, Interpreter::HandlePingTimer, this)
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    , mResolvingInProgress(0)
#endif
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
    , mInstance(aInstance)
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

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    memset(mResolvingHostname, 0, sizeof(mResolvingHostname));
#endif
}

int Interpreter::Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength, bool aAllowTruncate)
{
    size_t      hexLength = strlen(aHex);
    const char *hexEnd    = aHex + hexLength;
    uint8_t *   cur       = aBin;
    uint8_t     numChars  = hexLength & 1;
    uint8_t     byte      = 0;
    int         len       = 0;
    int         rval;

    if (!aAllowTruncate)
    {
        VerifyOrExit((hexLength + 1) / 2 <= aBinLength, rval = -1);
    }

    while (aHex < hexEnd)
    {
        if ('A' <= *aHex && *aHex <= 'F')
        {
            byte |= 10 + (*aHex - 'A');
        }
        else if ('a' <= *aHex && *aHex <= 'f')
        {
            byte |= 10 + (*aHex - 'a');
        }
        else if ('0' <= *aHex && *aHex <= '9')
        {
            byte |= *aHex - '0';
        }
        else
        {
            ExitNow(rval = -1);
        }

        aHex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars = 0;
            *cur++   = byte;
            byte     = 0;
            len++;

            if (len == aBinLength)
            {
                ExitNow(rval = len);
            }
        }
        else
        {
            byte <<= 4;
        }
    }

    rval = len;

exit:
    return rval;
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

void Interpreter::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        OutputFormat("%02x", aBytes[i]);
    }
}

int Interpreter::OutputIp6Address(const otIp6Address &aAddress)
{
    return OutputFormat(
        "%x:%x:%x:%x:%x:%x:%x:%x", HostSwap16(aAddress.mFields.m16[0]), HostSwap16(aAddress.mFields.m16[1]),
        HostSwap16(aAddress.mFields.m16[2]), HostSwap16(aAddress.mFields.m16[3]), HostSwap16(aAddress.mFields.m16[4]),
        HostSwap16(aAddress.mFields.m16[5]), HostSwap16(aAddress.mFields.m16[6]), HostSwap16(aAddress.mFields.m16[7]));
}

otError Interpreter::ParseLong(char *aString, long &aLong)
{
    char *endptr;
    aLong = strtol(aString, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_INVALID_ARGS;
}

otError Interpreter::ParseUnsignedLong(char *aString, unsigned long &aUnsignedLong)
{
    char *endptr;
    aUnsignedLong = strtoul(aString, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_INVALID_ARGS;
}

otError Interpreter::ParseJoinerDiscerner(char *aString, otJoinerDiscerner &aDiscerner)
{
    otError       error     = OT_ERROR_NONE;
    char *        separator = strstr(aString, "/");
    unsigned long length;

    VerifyOrExit(separator != nullptr, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = ParseUnsignedLong(separator + 1, length));
    VerifyOrExit(length > 0 && length <= 64, error = OT_ERROR_INVALID_ARGS);

    {
        char *             end;
        unsigned long long value = strtoull(aString, &end, 0);
        aDiscerner.mValue        = value;
        VerifyOrExit(end == separator, error = OT_ERROR_INVALID_ARGS);
    }

    aDiscerner.mLength = static_cast<uint8_t>(length);

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

    static_assert(IsArraySorted(sCommands, OT_ARRAY_LENGTH(sCommands)), "Command list is not sorted");

    for (const Command &command : sCommands)
    {
        OutputLine("%s", command.mName);
    }

    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        OutputLine("%s", mUserCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

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
            unsigned long value;

            VerifyOrExit(aArgsLength >= 2, error = OT_ERROR_INVALID_COMMAND);

            if (strcmp(aArgs[1], "dua") == 0)
            {
                otIp6InterfaceIdentifier *mlIid = nullptr;
                otIp6InterfaceIdentifier  iid;

                VerifyOrExit((aArgsLength == 3 || aArgsLength == 4), error = OT_ERROR_INVALID_ARGS);

                SuccessOrExit(error = ParseUnsignedLong(aArgs[2], value));

                if (aArgsLength == 4)
                {
                    VerifyOrExit(Hex2Bin(aArgs[3], iid.mFields.m8, sizeof(iid)) == sizeof(iid),
                                 error = OT_ERROR_INVALID_ARGS);
                    mlIid = &iid;
                }

                otBackboneRouterConfigNextDuaRegistrationResponse(mInstance, mlIid, static_cast<uint8_t>(value));
                ExitNow();
            }
            else if (strcmp(aArgs[1], "mlr") == 0)
            {
                error = ProcessBackboneRouterMgmtMlr(aArgsLength - 2, aArgs + 2);
                ExitNow();
            }
        }
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        SuccessOrExit(error = ProcessBackboneRouterLocal(aArgsLength, aArgs));
    }

exit:
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError Interpreter::ProcessBackboneRouterMgmtMlr(uint8_t aArgsLength, char **aArgs)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_INVALID_COMMAND;

    VerifyOrExit(aArgsLength >= 1, OT_NOOP);

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
            struct otIp6Address address;
            unsigned long       value;
            uint32_t            timeout = 0;

            VerifyOrExit(aArgsLength == 3 || aArgsLength == 4, error = OT_ERROR_INVALID_ARGS);

            SuccessOrExit(error = otIp6AddressFromString(aArgs[2], &address));
            if (aArgsLength == 4)
            {
                SuccessOrExit(error = ParseUnsignedLong(aArgs[3], value));
                timeout = static_cast<uint32_t>(value);
            }

            error = otBackboneRouterMulticastListenerAdd(mInstance, &address, timeout);
        }
    }
    else if (!strcmp(aArgs[0], "response"))
    {
        unsigned long value;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));

        otBackboneRouterConfigNextMulticastListenerRegistrationResponse(mInstance, static_cast<uint8_t>(value));
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

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

otError Interpreter::ProcessBackboneRouterLocal(uint8_t aArgsLength, char *aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otBackboneRouterConfig config;
    unsigned long          value;

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
            SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
            otBackboneRouterSetRegistrationJitter(mInstance, static_cast<uint8_t>(value));
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
                    SuccessOrExit(error = ParseUnsignedLong(aArgs[++argCur], value));
                    config.mSequenceNumber = static_cast<uint8_t>(value);
                }
                else if (strcmp(aArgs[argCur], "delay") == 0)
                {
                    SuccessOrExit(error = ParseUnsignedLong(aArgs[++argCur], value));
                    config.mReregistrationDelay = static_cast<uint16_t>(value);
                }
                else if (strcmp(aArgs[argCur], "timeout") == 0)
                {
                    SuccessOrExit(error = ParseUnsignedLong(aArgs[++argCur], value));
                    config.mMlrTimeout = static_cast<uint32_t>(value);
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
        const char *domainName = otThreadGetDomainName(mInstance);
        OutputLine("%s", static_cast<const char *>(domainName));
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

            VerifyOrExit(Hex2Bin(aArgs[1], iid.mFields.m8, sizeof(otIp6InterfaceIdentifier)) ==
                             sizeof(otIp6InterfaceIdentifier),
                         error = OT_ERROR_INVALID_ARGS);
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

    if (aArgsLength == 0)
    {
        int8_t cca;

        SuccessOrExit(error = otPlatRadioGetCcaEnergyDetectThreshold(mInstance, &cca));
        OutputLine("%d dBm", cca);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otPlatRadioSetCcaEnergyDetectThreshold(mInstance, static_cast<int8_t>(value)));
    }

exit:
    return error;
}

otError Interpreter::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

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
                for (uint8_t channel = 0; channel < channelNum; channel++)
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
#endif
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
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            otChannelManagerRequestChannelChange(mInstance, static_cast<uint8_t>(value));
        }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
        else if (strcmp(aArgs[1], "select") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            error = otChannelManagerRequestChannelSelect(mInstance, (value != 0) ? true : false);
        }
#endif
        else if (strcmp(aArgs[1], "auto") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            otChannelManagerSetAutoChannelSelectionEnabled(mInstance, (value != 0) ? true : false);
        }
        else if (strcmp(aArgs[1], "delay") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            error = otChannelManagerSetDelay(mInstance, static_cast<uint8_t>(value));
        }
        else if (strcmp(aArgs[1], "interval") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            error = otChannelManagerSetAutoChannelSelectionInterval(mInstance, static_cast<uint32_t>(value));
        }
        else if (strcmp(aArgs[1], "supported") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            otChannelManagerSetSupportedChannels(mInstance, static_cast<uint32_t>(value));
        }
        else if (strcmp(aArgs[1], "favored") == 0)
        {
            VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            otChannelManagerSetFavoredChannels(mInstance, static_cast<uint32_t>(value));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otLinkSetChannel(mInstance, static_cast<uint8_t>(value));
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessChild(uint8_t aArgsLength, char *aArgs[])
{
    otError     error = OT_ERROR_NONE;
    otChildInfo childInfo;
    long        value;
    bool        isTable;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(aArgs[0], "table") == 0);

    if (isTable || strcmp(aArgs[0], "list") == 0)
    {
        uint16_t maxChildren;

        if (isTable)
        {
            OutputLine("| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|S|D|N| Extended MAC     |");
            OutputLine("+-----+--------+------------+------------+-------+------+-+-+-+-+------------------+");
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
                OutputFormat("|%1d", childInfo.mSecureDataRequest);
                OutputFormat("|%1d", childInfo.mFullThreadDevice);
                OutputFormat("|%1d", childInfo.mFullNetworkData);
                OutputFormat("| ");

                for (uint8_t b : childInfo.mExtAddress.m8)
                {
                    OutputFormat("%02x", b);
                }

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

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadGetChildInfoById(mInstance, static_cast<uint16_t>(value), &childInfo));

    OutputLine("Child ID: %d", childInfo.mChildId);
    OutputLine("Rloc: %04x", childInfo.mRloc16);
    OutputFormat("Ext Addr: ");

    for (uint8_t b : childInfo.mExtAddress.m8)
    {
        OutputFormat("%02x", b);
    }

    OutputLine("");
    OutputFormat("Mode: ");

    if (childInfo.mRxOnWhenIdle)
    {
        OutputFormat("r");
    }

    if (childInfo.mSecureDataRequest)
    {
        OutputFormat("s");
    }

    if (childInfo.mFullThreadDevice)
    {
        OutputFormat("d");
    }

    if (childInfo.mFullNetworkData)
    {
        OutputFormat("n");
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
    OT_UNUSED_VARIABLE(aArgs);
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
            unsigned long value;
            SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
            SuccessOrExit(error = otThreadSetMaxChildIpAddresses(mInstance, static_cast<uint8_t>(value)));
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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetMaxAllowedChildren(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otThreadSetMaxAllowedChildren(mInstance, static_cast<uint16_t>(value)));
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

otError Interpreter::ProcessChildTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetChildTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetChildTimeout(mInstance, static_cast<uint32_t>(value));
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
        OutputLine("%s", otPlatRadioIsCoexEnabled(mInstance) ? "Enabled" : "Disabled");
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
        OutputLine("    Request: %u", metrics.mNumTxRequest);
        OutputLine("    Grant Immediate: %u", metrics.mNumTxGrantImmediate);
        OutputLine("    Grant Wait: %u", metrics.mNumTxGrantWait);
        OutputLine("    Grant Wait Activated: %u", metrics.mNumTxGrantWaitActivated);
        OutputLine("    Grant Wait Timeout: %u", metrics.mNumTxGrantWaitTimeout);
        OutputLine("    Grant Deactivated During Request: %u", metrics.mNumTxGrantDeactivatedDuringRequest);
        OutputLine("    Delayed Grant: %u", metrics.mNumTxDelayedGrant);
        OutputLine("    Average Request To Grant Time: %u", metrics.mAvgTxRequestToGrantTime);
        OutputLine("Receive metrics");
        OutputLine("    Request: %u", metrics.mNumRxRequest);
        OutputLine("    Grant Immediate: %u", metrics.mNumRxGrantImmediate);
        OutputLine("    Grant Wait: %u", metrics.mNumRxGrantWait);
        OutputLine("    Grant Wait Activated: %u", metrics.mNumRxGrantWaitActivated);
        OutputLine("    Grant Wait Timeout: %u", metrics.mNumRxGrantWaitTimeout);
        OutputLine("    Grant Deactivated During Request: %u", metrics.mNumRxGrantDeactivatedDuringRequest);
        OutputLine("    Delayed Grant: %u", metrics.mNumRxDelayedGrant);
        OutputLine("    Average Request To Grant Time: %u", metrics.mAvgRxRequestToGrantTime);
        OutputLine("    Grant None: %u", metrics.mNumRxGrantNone);
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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetContextIdReuseDelay(mInstance));
    }
    else
    {
        SuccessOrExit(ParseLong(aArgs[0], value));
        otThreadSetContextIdReuseDelay(mInstance, static_cast<uint32_t>(value));
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
            OutputLine("    TxUnicast: %d", macCounters->mTxUnicast);
            OutputLine("    TxBroadcast: %d", macCounters->mTxBroadcast);
            OutputLine("    TxAckRequested: %d", macCounters->mTxAckRequested);
            OutputLine("    TxAcked: %d", macCounters->mTxAcked);
            OutputLine("    TxNoAckRequested: %d", macCounters->mTxNoAckRequested);
            OutputLine("    TxData: %d", macCounters->mTxData);
            OutputLine("    TxDataPoll: %d", macCounters->mTxDataPoll);
            OutputLine("    TxBeacon: %d", macCounters->mTxBeacon);
            OutputLine("    TxBeaconRequest: %d", macCounters->mTxBeaconRequest);
            OutputLine("    TxOther: %d", macCounters->mTxOther);
            OutputLine("    TxRetry: %d", macCounters->mTxRetry);
            OutputLine("    TxErrCca: %d", macCounters->mTxErrCca);
            OutputLine("    TxErrBusyChannel: %d", macCounters->mTxErrBusyChannel);
            OutputLine("RxTotal: %d", macCounters->mRxTotal);
            OutputLine("    RxUnicast: %d", macCounters->mRxUnicast);
            OutputLine("    RxBroadcast: %d", macCounters->mRxBroadcast);
            OutputLine("    RxData: %d", macCounters->mRxData);
            OutputLine("    RxDataPoll: %d", macCounters->mRxDataPoll);
            OutputLine("    RxBeacon: %d", macCounters->mRxBeacon);
            OutputLine("    RxBeaconRequest: %d", macCounters->mRxBeaconRequest);
            OutputLine("    RxOther: %d", macCounters->mRxOther);
            OutputLine("    RxAddressFiltered: %d", macCounters->mRxAddressFiltered);
            OutputLine("    RxDestAddrFiltered: %d", macCounters->mRxDestAddrFiltered);
            OutputLine("    RxDuplicated: %d", macCounters->mRxDuplicated);
            OutputLine("    RxErrNoFrame: %d", macCounters->mRxErrNoFrame);
            OutputLine("    RxErrNoUnknownNeighbor: %d", macCounters->mRxErrUnknownNeighbor);
            OutputLine("    RxErrInvalidSrcAddr: %d", macCounters->mRxErrInvalidSrcAddr);
            OutputLine("    RxErrSec: %d", macCounters->mRxErrSec);
            OutputLine("    RxErrFcs: %d", macCounters->mRxErrFcs);
            OutputLine("    RxErrOther: %d", macCounters->mRxErrOther);
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
otError Interpreter::ProcessCsl(uint8_t aArgsLength, char *argv[])
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
        long value;

        SuccessOrExit(error = ParseLong(argv[1], value));

        if (strcmp(argv[0], "channel") == 0)
        {
            SuccessOrExit(error = otLinkCslSetChannel(mInstance, static_cast<uint8_t>(value)));
        }
        else if (strcmp(argv[0], "period") == 0)
        {
            SuccessOrExit(error = otLinkCslSetPeriod(mInstance, static_cast<uint16_t>(value)));
        }
        else if (strcmp(argv[0], "timeout") == 0)
        {
            SuccessOrExit(error = otLinkCslSetTimeout(mInstance, static_cast<uint32_t>(value)));
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
        unsigned long value;
        SuccessOrExit(error = ParseUnsignedLong(aArgs[0], value));
        SuccessOrExit(error = otDatasetSetDelayTimerMinimal(mInstance, static_cast<uint32_t>(value * 1000)));
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
    long     value;

    if (aArgsLength > 0)
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        VerifyOrExit((0 <= value) && (value < static_cast<long>(sizeof(scanChannels) * CHAR_BIT)),
                     error = OT_ERROR_INVALID_ARGS);
        scanChannels = 1 << value;
    }

    SuccessOrExit(error = otThreadDiscover(mInstance, scanChannels, OT_PANID_BROADCAST, false, false,
                                           &Interpreter::HandleActiveScanResult, this));
    OutputLine("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |");
    OutputLine("+---+------------------+------------------+------+------------------+----+-----+-----+");

    error = OT_ERROR_PENDING;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
otError Interpreter::ProcessDns(uint8_t aArgsLength, char *aArgs[])
{
    otError          error = OT_ERROR_NONE;
    long             port  = OT_DNS_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otDnsQuery       query;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "resolve") == 0)
    {
        VerifyOrExit(!mResolvingInProgress, error = OT_ERROR_BUSY);
        VerifyOrExit(aArgsLength > 1, error = OT_ERROR_INVALID_ARGS);
        VerifyOrExit(strlen(aArgs[1]) < OT_DNS_MAX_HOSTNAME_LENGTH, error = OT_ERROR_INVALID_ARGS);

        strcpy(mResolvingHostname, aArgs[1]);

        if (aArgsLength > 2)
        {
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(aArgs[2]));
        }
        else
        {
            // Use IPv6 address of default DNS server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_DNS_DEFAULT_SERVER_IP));
        }

        if (aArgsLength > 3)
        {
            SuccessOrExit(error = ParseLong(aArgs[3], port));
        }

        messageInfo.SetPeerPort(static_cast<uint16_t>(port));

        query.mHostname    = mResolvingHostname;
        query.mMessageInfo = static_cast<const otMessageInfo *>(&messageInfo);
        query.mNoRecursion = false;

        SuccessOrExit(error = otDnsClientQuery(mInstance, &query, &Interpreter::HandleDnsResponse, this));

        mResolvingInProgress = true;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

    error = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::HandleDnsResponse(void *              aContext,
                                    const char *        aHostname,
                                    const otIp6Address *aAddress,
                                    uint32_t            aTtl,
                                    otError             aResult)
{
    static_cast<Interpreter *>(aContext)->HandleDnsResponse(aHostname, static_cast<const Ip6::Address *>(aAddress),
                                                            aTtl, aResult);
}

void Interpreter::HandleDnsResponse(const char *aHostname, const Ip6::Address *aAddress, uint32_t aTtl, otError aResult)
{
    OutputFormat("DNS response for %s - ", aHostname);

    if (aResult == OT_ERROR_NONE)
    {
        if (aAddress != nullptr)
        {
            OutputIp6Address(*aAddress);
        }
        OutputLine(" TTL: %d", aTtl);
    }

    OutputResult(aResult);

    mResolvingInProgress = false;
}
#endif

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

        memset(&extAddress, 0, sizeof(extAddress));
        VerifyOrExit(Hex2Bin(aArgs[0], extAddress.m8, sizeof(extAddress.m8)) == sizeof(extAddress.m8),
                     error = OT_ERROR_INVALID_ARGS);

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
            long level;

            SuccessOrExit(error = ParseLong(aArgs[1], level));
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

        VerifyOrExit(Hex2Bin(aArgs[0], extPanId.m8, sizeof(extPanId)) == sizeof(extPanId),
                     error = OT_ERROR_INVALID_ARGS);

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

    VerifyOrExit(aArgsLength >= 1, OT_NOOP);

    if (strcmp(aArgs[0], "/a/an") == 0)
    {
        otIp6Address             destination, target;
        otIp6InterfaceIdentifier mlIid;

        VerifyOrExit(aArgsLength == 4, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = otIp6AddressFromString(aArgs[1], &destination));
        SuccessOrExit(error = otIp6AddressFromString(aArgs[2], &target));
        VerifyOrExit(Hex2Bin(aArgs[3], mlIid.mFields.m8, sizeof(otIp6InterfaceIdentifier)) ==
                         sizeof(otIp6InterfaceIdentifier),
                     error = OT_ERROR_INVALID_ARGS);

        otThreadSendAddressNotification(mInstance, &destination, &target, &mlIid);
    }
exit:
    return error;
}
#endif

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

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &aAddress.mAddress));
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
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &address));
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
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &address));
    error = otIp6SubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessIpMulticastAddrDel(uint8_t aArgsLength, char *aArgs[])
{
    otError             error;
    struct otIp6Address address;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &address));
    error = otIp6UnsubscribeMulticastAddress(mInstance, &address);

exit:
    return error;
}

otError Interpreter::ProcessMulticastPromiscuous(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otIp6IsMulticastPromiscuousEnabled(mInstance))
        {
            OutputLine("Enabled");
        }
        else
        {
            OutputLine("Disabled");
        }
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
    long    value;

    VerifyOrExit(aArgsLength == 1 || aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "counter") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputLine("%d", otThreadGetKeySequenceCounter(mInstance));
        }
        else
        {
            SuccessOrExit(error = ParseLong(aArgs[1], value));
            otThreadSetKeySequenceCounter(mInstance, static_cast<uint32_t>(value));
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
            SuccessOrExit(error = ParseLong(aArgs[1], value));
            otThreadSetKeySwitchGuardTime(mInstance, static_cast<uint32_t>(value));
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
otError Interpreter::ProcessLeaderPartitionId(uint8_t aArgsLength, char *aArgs[])
{
    otError       error = OT_ERROR_NONE;
    unsigned long value;

    if (aArgsLength == 0)
    {
        OutputLine("%u", otThreadGetLocalLeaderPartitionId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseUnsignedLong(aArgs[0], value));
        otThreadSetLocalLeaderPartitionId(mInstance, static_cast<uint32_t>(value));
    }

exit:
    return error;
}

otError Interpreter::ProcessLeaderWeight(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetLocalLeaderWeight(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetLocalLeaderWeight(mInstance, static_cast<uint8_t>(value));
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
void Interpreter::HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          void *                     aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsReport(aAddress, aMetricsValues);
}

void Interpreter::HandleLinkMetricsReport(const otIp6Address *aAddress, const otLinkMetricsValues *aMetricsValues)
{
    const char kLinkMetricsTypeCount[]   = "(Count/Summation)";
    const char kLinkMetricsTypeAverage[] = "(Exponential Moving Average)";

    OutputFormat("Received Link Metrics Report from: ");
    OutputIp6Address(*aAddress);
    OutputLine("");

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

otError Interpreter::ProcessLinkMetrics(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    VerifyOrExit(aArgsLength >= 1, OT_NOOP);

    if (strcmp(aArgs[0], "query") == 0)
    {
        error = ProcessLinkMetricsQuery(aArgsLength - 1, aArgs + 1);
    }

exit:
    return error;
}

otError Interpreter::ProcessLinkMetricsQuery(uint8_t aArgsLength, char *aArgs[])
{
    otError       error = OT_ERROR_INVALID_ARGS;
    otIp6Address  address;
    otLinkMetrics linkMetrics;
    long          seriesId = 0;

    VerifyOrExit(aArgsLength >= 2, OT_NOOP);

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &address));

    memset(&linkMetrics, 0, sizeof(otLinkMetrics));

    if (strcmp(aArgs[1], "single") == 0)
    {
        VerifyOrExit(aArgsLength == 3, OT_NOOP);
        for (char *arg = aArgs[2]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'p':
                linkMetrics.mPduCount = 1;
                break;

            case 'q':
                linkMetrics.mLqi = 1;
                break;

            case 'm':
                linkMetrics.mLinkMargin = 1;
                break;

            case 'r':
                linkMetrics.mRssi = 1;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
        error = otLinkMetricsQuery(mInstance, &address, static_cast<uint8_t>(seriesId), &linkMetrics,
                                   &Interpreter::HandleLinkMetricsReport, this);
    }

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

        for (uint8_t b : pskc->m8)
        {
            OutputFormat("%02x", b);
        }

        OutputLine("");
    }
    else
    {
        otPskc pskc;

        if (aArgsLength == 1)
        {
            VerifyOrExit(Hex2Bin(aArgs[0], pskc.m8, sizeof(pskc)) == sizeof(pskc), error = OT_ERROR_INVALID_ARGS);
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
#endif

otError Interpreter::ProcessMasterKey(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *key = reinterpret_cast<const uint8_t *>(otThreadGetMasterKey(mInstance));

        for (int i = 0; i < OT_MASTER_KEY_SIZE; i++)
        {
            OutputFormat("%02x", key[i]);
        }

        OutputLine("");
    }
    else
    {
        otMasterKey key;

        VerifyOrExit(Hex2Bin(aArgs[0], key.m8, sizeof(key.m8)) == OT_MASTER_KEY_SIZE, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otThreadSetMasterKey(mInstance, &key));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

otError Interpreter::ProcessMlr(uint8_t aArgsLength, char **aArgs)
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
        if (otIp6AddressFromString(aArgs[i], &addresses[i]) != OT_ERROR_NONE)
        {
            break;
        }
    }

    VerifyOrExit(i > 0 && (i == aArgsLength || i == aArgsLength - 1), error = OT_ERROR_INVALID_ARGS);

    if (i == aArgsLength - 1)
    {
        // Parse the last argument as a timeout in seconds
        unsigned long value;

        SuccessOrExit(error = ParseUnsignedLong(aArgs[i], value));

        timeout = static_cast<uint32_t>(value);
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

#endif // OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

otError Interpreter::ProcessMode(uint8_t aArgsLength, char *aArgs[])
{
    otError          error = OT_ERROR_NONE;
    otLinkModeConfig linkMode;

    memset(&linkMode, 0, sizeof(otLinkModeConfig));

    if (aArgsLength == 0)
    {
        linkMode = otThreadGetLinkMode(mInstance);

        if (linkMode.mRxOnWhenIdle)
        {
            OutputFormat("r");
        }

        if (linkMode.mSecureDataRequests)
        {
            OutputFormat("s");
        }

        if (linkMode.mDeviceType)
        {
            OutputFormat("d");
        }

        if (linkMode.mNetworkData)
        {
            OutputFormat("n");
        }

        OutputLine("");
    }
    else
    {
        for (char *arg = aArgs[0]; *arg != '\0'; arg++)
        {
            switch (*arg)
            {
            case 'r':
                linkMode.mRxOnWhenIdle = 1;
                break;

            case 's':
                linkMode.mSecureDataRequests = 1;
                break;

            case 'd':
                linkMode.mDeviceType = 1;
                break;

            case 'n':
                linkMode.mNetworkData = 1;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        SuccessOrExit(error = otThreadSetLinkMode(mInstance, linkMode));
    }

exit:
    return error;
}

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
            OutputLine("| Role | RLOC16 | Age | Avg RSSI | Last RSSI |R|S|D|N| Extended MAC     |");
            OutputLine("+------+--------+-----+----------+-----------+-+-+-+-+------------------+");
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
                OutputFormat("|%1d", neighborInfo.mSecureDataRequest);
                OutputFormat("|%1d", neighborInfo.mFullThreadDevice);
                OutputFormat("|%1d", neighborInfo.mFullNetworkData);
                OutputFormat("| ");

                for (uint8_t b : neighborInfo.mExtAddress.m8)
                {
                    OutputFormat("%02x", b);
                }

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
#endif

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
        long enterpriseNumber;
        int  length;

        VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(aArgs[1], enterpriseNumber));
        cfg.mEnterpriseNumber = static_cast<uint32_t>(enterpriseNumber);

        length = Hex2Bin(aArgs[2], cfg.mServiceData, sizeof(cfg.mServiceData));
        VerifyOrExit(length > 0, error = OT_ERROR_INVALID_ARGS);
        cfg.mServiceDataLength = static_cast<uint8_t>(length);

        if (strcmp(aArgs[0], "add") == 0)
        {
            VerifyOrExit(aArgsLength > 3, error = OT_ERROR_INVALID_ARGS);

            length = Hex2Bin(aArgs[3], cfg.mServerConfig.mServerData, sizeof(cfg.mServerConfig.mServerData));
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
#endif

otError Interpreter::ProcessNetworkData(uint8_t aArgsLength, char *aArgs[])
{
    return mNetworkData.Process(aArgsLength, aArgs);
}

#if OPENTHREAD_FTD
otError Interpreter::ProcessNetworkIdTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetNetworkIdTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetNetworkIdTimeout(mInstance, static_cast<uint8_t>(value));
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
        const char *networkName = otThreadGetNetworkName(mInstance);
        OutputLine("%s", static_cast<const char *>(networkName));
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
    long    value;

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
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otNetworkTimeSetSyncPeriod(mInstance, static_cast<uint16_t>(value)));

        SuccessOrExit(error = ParseLong(aArgs[1], value));
        SuccessOrExit(error = otNetworkTimeSetXtalThreshold(mInstance, static_cast<uint16_t>(value)));
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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("0x%04x", otLinkGetPanId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otLinkSetPanId(mInstance, static_cast<otPanId>(value));
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

    for (uint8_t b : parentInfo.mExtAddress.m8)
    {
        OutputFormat("%02x", b);
    }

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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetParentPriority(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otThreadSetParentPriority(mInstance, static_cast<int8_t>(value));
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

    VerifyOrExit(aIcmpHeader->mType == OT_ICMP6_TYPE_ECHO_REPLY, OT_NOOP);
    VerifyOrExit((mPingIdentifier != 0) && (mPingIdentifier == HostSwap16(aIcmpHeader->mData.m16[0])), OT_NOOP);

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
    long     value;
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

    SuccessOrExit(error = otIp6AddressFromString(aArgs[0], &mPingDestAddress));

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
            SuccessOrExit(error = ParseLong(aArgs[index], value));
            mPingLength = static_cast<uint16_t>(value);
            break;

        case 2:
            SuccessOrExit(error = ParseLong(aArgs[index], value));
            mPingCount = static_cast<uint16_t>(value);
            break;

        case 3:
            SuccessOrExit(error = ParsePingInterval(aArgs[index], interval));
            VerifyOrExit(0 < interval && interval <= Timer::kMaxDelay, error = OT_ERROR_INVALID_ARGS);
            mPingInterval = interval;
            break;

        case 4:
            SuccessOrExit(error = ParseLong(aArgs[index], value));
            VerifyOrExit(0 <= value && value <= 255, error = OT_ERROR_INVALID_ARGS);
            mPingHopLimit          = static_cast<uint8_t>(value);
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
    VerifyOrExit(message != nullptr, OT_NOOP);

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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otLinkGetPollPeriod(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otLinkSetPollPeriod(mInstance, static_cast<uint32_t>(value));
    }

exit:
    return error;
}

otError Interpreter::ProcessPromiscuous(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otLinkIsPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance))
        {
            OutputLine("Enabled");
        }
        else
        {
            OutputLine("Disabled");
        }
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
    char *               prefixLengthStr;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&config, 0, sizeof(otBorderRouterConfig));

    if ((prefixLengthStr = strchr(aArgs[argcur], '/')) == nullptr)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(aArgs[argcur], &config.mPrefix.mPrefix));

    {
        unsigned long length;

        SuccessOrExit(error = ParseUnsignedLong(prefixLengthStr, length));
        config.mPrefix.mLength = static_cast<uint8_t>(length);
    }

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
    OT_UNUSED_VARIABLE(aArgsLength);

    otError            error = OT_ERROR_NONE;
    struct otIp6Prefix prefix;
    uint8_t            argcur = 0;
    char *             prefixLengthStr;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    memset(&prefix, 0, sizeof(otIp6Prefix));

    if ((prefixLengthStr = strchr(aArgs[argcur], '/')) == nullptr)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(aArgs[argcur], &prefix.mPrefix));

    {
        unsigned long length;

        SuccessOrExit(error = ParseUnsignedLong(prefixLengthStr, length));
        prefix.mLength = static_cast<uint8_t>(length);
    }

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
    otError       error = OT_ERROR_NONE;
    unsigned long value;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = ParseUnsignedLong(aArgs[0], value));
    error = otThreadSetPreferredRouterId(mInstance, static_cast<uint8_t>(value));

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

#if OPENTHREAD_FTD
otError Interpreter::ProcessReleaseRouterId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

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
    char *                prefixLengthStr;

    memset(&config, 0, sizeof(otExternalRouteConfig));

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if ((prefixLengthStr = strchr(aArgs[argcur], '/')) == nullptr)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(aArgs[argcur], &config.mPrefix.mPrefix));

    {
        unsigned long length;

        SuccessOrExit(error = ParseUnsignedLong(prefixLengthStr, length));
        config.mPrefix.mLength = static_cast<uint8_t>(length);
    }

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
    otError            error = OT_ERROR_NONE;
    struct otIp6Prefix prefix;
    uint8_t            argcur = 0;
    char *             prefixLengthStr;

    memset(&prefix, 0, sizeof(struct otIp6Prefix));

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if ((prefixLengthStr = strchr(aArgs[argcur], '/')) == nullptr)
    {
        ExitNow();
    }

    *prefixLengthStr++ = '\0';

    SuccessOrExit(error = otIp6AddressFromString(aArgs[argcur], &prefix.mPrefix));

    {
        unsigned long length;

        SuccessOrExit(error = ParseUnsignedLong(prefixLengthStr, length));
        prefix.mLength = static_cast<uint8_t>(length);
    }

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
    long         value;
    bool         isTable;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    isTable = (strcmp(aArgs[0], "table") == 0);

    if (isTable || strcmp(aArgs[0], "list") == 0)
    {
        uint8_t maxRouterId;

        if (isTable)
        {
            OutputLine("| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     |");
            OutputLine("+----+--------+----------+-----------+-------+--------+-----+------------------+");
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

                for (uint8_t b : routerInfo.mExtAddress.m8)
                {
                    OutputFormat("%02x", b);
                }

                OutputLine(" |");
            }
            else
            {
                OutputFormat("%d ", i);
            }
        }

        OutputLine("");
        ExitNow();
    }

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadGetRouterInfo(mInstance, static_cast<uint16_t>(value), &routerInfo));

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

            for (uint8_t b : routerInfo.mExtAddress.m8)
            {
                OutputFormat("%02x", b);
            }

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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterDowngradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetRouterDowngradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterEligible(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otThreadIsRouterEligible(mInstance))
        {
            OutputLine("Enabled");
        }
        else
        {
            OutputLine("Disabled");
        }
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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterSelectionJitter(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        VerifyOrExit(0 < value && value < 256, error = OT_ERROR_INVALID_ARGS);
        otThreadSetRouterSelectionJitter(mInstance, static_cast<uint8_t>(value));
    }

exit:
    return error;
}

otError Interpreter::ProcessRouterUpgradeThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetRouterUpgradeThreshold(mInstance, static_cast<uint8_t>(value));
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
    long     value;

    if (aArgsLength > 0)
    {
        if (strcmp(aArgs[0], "energy") == 0)
        {
            energyScan = true;

            if (aArgsLength > 1)
            {
                SuccessOrExit(error = ParseLong(aArgs[1], value));
                scanDuration = static_cast<uint16_t>(value);
            }
        }
        else
        {
            SuccessOrExit(error = ParseLong(aArgs[0], value));
            VerifyOrExit((0 <= value) && (value < static_cast<long>(sizeof(scanChannels) * CHAR_BIT)),
                         error = OT_ERROR_INVALID_ARGS);
            scanChannels = 1 << value;
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
    OutputBytes(aResult->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
    OutputFormat(" ");

    OutputFormat("| %04x | ", aResult->mPanId);
    OutputBytes(aResult->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
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
    long             port  = OT_SNTP_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otSntpQuery      query;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "query") == 0)
    {
        VerifyOrExit(!mSntpQueryingInProgress, error = OT_ERROR_BUSY);

        if (aArgsLength > 1)
        {
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(aArgs[1]));
        }
        else
        {
            // Use IPv6 address of default SNTP server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_SNTP_DEFAULT_SERVER_IP));
        }

        if (aArgsLength > 2)
        {
            SuccessOrExit(error = ParseLong(aArgs[2], port));
        }

        messageInfo.SetPeerPort(static_cast<uint16_t>(port));

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
#endif

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
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

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

    if (aArgsLength == 0)
    {
        int8_t power;

        SuccessOrExit(error = otPlatRadioGetTransmitPower(mInstance, &power));
        OutputLine("%d dBm", power);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otPlatRadioSetTransmitPower(mInstance, static_cast<int8_t>(value)));
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
        unsigned long value;

        VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
        VerifyOrExit(value <= 0xffff, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otIp6AddUnsecurePort(mInstance, static_cast<uint16_t>(value)));
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
            unsigned long value;

            SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
            VerifyOrExit(value <= 0xffff, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otIp6RemoveUnsecurePort(mInstance, static_cast<uint16_t>(value)));
        }
    }
    else if (strcmp(aArgs[0], "get") == 0)
    {
        const uint16_t *ports;
        uint8_t         numPorts;

        ports = otIp6GetUnsecurePorts(mInstance, &numPorts);

        if (ports != NULL)
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
    long    value;

    if (aArgsLength == 0)
    {
        OutputLine("%d", otThreadGetJoinerUdpPort(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otThreadSetJoinerUdpPort(mInstance, static_cast<uint16_t>(value));
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
        OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

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
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
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
    long                   value;

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
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

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
            VerifyOrExit(Hex2Bin(aArgs[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_INVALID_ARGS);

            error = otLinkFilterAddAddress(mInstance, &extAddr);

            VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY, OT_NOOP);

            if (aArgsLength > 2)
            {
                int8_t rss = 0;
                VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);
                SuccessOrExit(error = ParseLong(aArgs[2], value));
                rss = static_cast<int8_t>(value);
                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(aArgs[0], "remove") == 0)
        {
            VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(Hex2Bin(aArgs[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                         error = OT_ERROR_INVALID_ARGS);
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
    long                value;
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
                OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
                OutputLine(" : rss %d (lqi %d)", entry.mRssIn, otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
        }
    }
    else
    {
        if (strcmp(aArgs[0], "add-lqi") == 0)
        {
            uint8_t linkquality = 0;
            VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            linkquality = static_cast<uint8_t>(value);
            VerifyOrExit(linkquality <= 3, error = OT_ERROR_INVALID_ARGS);
            rss = otLinkConvertLinkQualityToRss(mInstance, linkquality);

            if (strcmp(aArgs[1], "*") == 0)
            {
                otLinkFilterSetDefaultRssIn(mInstance, rss);
            }
            else
            {
                VerifyOrExit(Hex2Bin(aArgs[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_INVALID_ARGS);

                SuccessOrExit(error = otLinkFilterAddRssIn(mInstance, &extAddr, rss));
            }
        }
        else if (strcmp(aArgs[0], "add") == 0)
        {
            VerifyOrExit(aArgsLength == 3, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = ParseLong(aArgs[2], value));
            rss = static_cast<int8_t>(value);

            if (strcmp(aArgs[1], "*") == 0)
            {
                otLinkFilterSetDefaultRssIn(mInstance, rss);
            }
            else
            {
                VerifyOrExit(Hex2Bin(aArgs[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_INVALID_ARGS);

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
                VerifyOrExit(Hex2Bin(aArgs[1], extAddr.m8, OT_EXT_ADDRESS_SIZE) == OT_EXT_ADDRESS_SIZE,
                             error = OT_ERROR_INVALID_ARGS);

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
            unsigned long value;

            SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
            VerifyOrExit(value <= 0xff, error = OT_ERROR_INVALID_ARGS);

            otLinkSetMaxFrameRetriesDirect(mInstance, static_cast<uint8_t>(value));
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
            unsigned long value;

            SuccessOrExit(error = ParseUnsignedLong(aArgs[1], value));
            VerifyOrExit(value <= 0xff, error = OT_ERROR_INVALID_ARGS);

            otLinkSetMaxFrameRetriesIndirect(mInstance, static_cast<uint8_t>(value));
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

const Interpreter::Command *Interpreter::FindCommand(const char *aName) const
{
    const Command *rval  = nullptr;
    uint16_t       left  = 0;
    uint16_t       right = OT_ARRAY_LENGTH(sCommands);

    while (left < right)
    {
        uint16_t middle  = (left + right) / 2;
        int      compare = strcmp(aName, sCommands[middle].mName);

        if (compare == 0)
        {
            rval = &sCommands[middle];
            break;
        }
        else if (compare > 0)
        {
            left = middle + 1;
        }
        else
        {
            right = middle;
        }
    }

    return rval;
}

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength)
{
    char *         aArgs[kMaxArgs] = {nullptr};
    char *         cmdName;
    uint8_t        aArgsLength = 0;
    const Command *command;

    VerifyOrExit(aBuf != nullptr && StringLength(aBuf, aBufLength + 1) <= aBufLength, OT_NOOP);

    VerifyOrExit(Utils::CmdLineParser::ParseCmd(aBuf, aArgsLength, aArgs, kMaxArgs) == OT_ERROR_NONE,
                 OutputLine("Error: too many args (max %d)", kMaxArgs));
    VerifyOrExit(aArgsLength >= 1, OutputLine("Error: no given command."));

    cmdName = aArgs[0];

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    VerifyOrExit((!otDiagIsEnabled(mInstance) || (strcmp(cmdName, "diag") == 0)),
                 OutputLine("under diagnostics mode, execute 'diag stop' before running any other commands."));
#endif

    command = FindCommand(cmdName);

    if (command != nullptr)
    {
        OutputResult((this->*command->mCommand)(aArgsLength - 1, &aArgs[1]));
        ExitNow();
    }

    // Check user defined commands if built-in command has not been found
    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        if (strcmp(cmdName, mUserCommands[i].mName) == 0)
        {
            mUserCommands[i].mCommand(aArgsLength - 1, &aArgs[1]);
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
    otError             error = OT_ERROR_NONE;
    struct otIp6Address address;
    uint8_t             tlvTypes[OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];
    uint8_t             count     = 0;
    uint8_t             argsIndex = 0;

    // Include operation, address and type tlv list.
    VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otIp6AddressFromString(aArgs[1], &address));

    argsIndex = 2;

    while (argsIndex < aArgsLength && count < sizeof(tlvTypes))
    {
        long value;
        SuccessOrExit(error = ParseLong(aArgs[argsIndex++], value));
        tlvTypes[count++] = static_cast<uint8_t>(value);
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

void Interpreter::HandleDiagnosticGetResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDiagnosticGetResponse(
        *aMessage, *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Interpreter::HandleDiagnosticGetResponse(const otMessage &aMessage, const Ip6::MessageInfo &)
{
    uint8_t               buf[16];
    uint16_t              bytesToPrint;
    uint16_t              bytesPrinted = 0;
    uint16_t              length       = otMessageGetLength(&aMessage) - otMessageGetOffset(&aMessage);
    otNetworkDiagTlv      diagTlv;
    otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
    otError               error    = OT_ERROR_NONE;

    OutputFormat("DIAG_GET.rsp/ans: ");

    while (length > 0)
    {
        bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
        otMessageRead(&aMessage, otMessageGetOffset(&aMessage) + bytesPrinted, buf, bytesToPrint);

        OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

        length -= bytesToPrint;
        bytesPrinted += bytesToPrint;
    }

    OutputLine("");

    // Output Network Diagnostic TLV values in standard YAML format.
    while ((error = otThreadGetNextDiagnosticTlv(&aMessage, &iterator, &diagTlv)) == OT_ERROR_NONE)
    {
        uint16_t column = 0;
        switch (diagTlv.mType)
        {
        case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
            OutputFormat("Ext Address: '");
            OutputBytes(diagTlv.mData.mExtAddress.m8, sizeof(diagTlv.mData.mExtAddress.m8));
            OutputLine("'");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
            OutputLine("Rloc16: 0x%04x", diagTlv.mData.mAddr16);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
            OutputLine("Mode:");
            OutputMode(diagTlv.mData.mMode, column + kIndentationSize);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
            OutputLine("Timeout: %u", diagTlv.mData.mTimeout);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
            OutputLine("Connectivity:");
            OutputConnectivity(diagTlv.mData.mConnectivity, column + kIndentationSize);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
            OutputLine("Route:");
            OutputRoute(diagTlv.mData.mRoute, column + kIndentationSize);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
            OutputLine("Leader Data:");
            OutputLeaderData(diagTlv.mData.mLeaderData, column + kIndentationSize);
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
                OutputSpaces(column + kIndentationSize);
                OutputFormat("- ");
                OutputIp6Address(diagTlv.mData.mIp6AddrList.mList[i]);
                OutputLine("");
            }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
            OutputLine("MAC Counters:");
            OutputNetworkDiagMacCounters(diagTlv.mData.mMacCounters, column + kIndentationSize);
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
                OutputSpaces(column + kIndentationSize);
                OutputFormat("- ");
                OutputChildTableEntry(diagTlv.mData.mChildTable.mTable[i], column + kIndentationSize + 2);
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

    OutputResult(error == OT_ERROR_NOT_FOUND ? OT_ERROR_NONE : error);
}

void Interpreter::OutputSpaces(uint16_t aCount)
{
    static const uint16_t kSpaceStrLen = 16;
    char                  spaceStr[kSpaceStrLen + 1];

    memset(spaceStr, ' ', kSpaceStrLen);
    spaceStr[kSpaceStrLen] = '\0';

    for (uint16_t i = 0; i < aCount; i += kSpaceStrLen)
    {
        uint16_t idx = (i + kSpaceStrLen <= aCount) ? 0 : (i + kSpaceStrLen - aCount);
        OutputFormat(&spaceStr[idx]);
    }
}

void Interpreter::OutputMode(const otLinkModeConfig &aMode, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputLine("RxOnWhenIdle: %d", aMode.mRxOnWhenIdle);

    OutputSpaces(aColumn);
    OutputLine("SecureDataRequests: %d", aMode.mSecureDataRequests);

    OutputSpaces(aColumn);
    OutputLine("DeviceType: %d", aMode.mDeviceType);

    OutputSpaces(aColumn);
    OutputLine("NetworkData: %d", aMode.mNetworkData);
}

void Interpreter::OutputConnectivity(const otNetworkDiagConnectivity &aConnectivity, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputLine("ParentPriority: %d", aConnectivity.mParentPriority);

    OutputSpaces(aColumn);
    OutputLine("LinkQuality3: %u", aConnectivity.mLinkQuality3);

    OutputSpaces(aColumn);
    OutputLine("LinkQuality2: %u", aConnectivity.mLinkQuality2);

    OutputSpaces(aColumn);
    OutputLine("LinkQuality1: %u", aConnectivity.mLinkQuality1);

    OutputSpaces(aColumn);
    OutputLine("LeaderCost: %u", aConnectivity.mLeaderCost);

    OutputSpaces(aColumn);
    OutputLine("IdSequence: %u", aConnectivity.mIdSequence);

    OutputSpaces(aColumn);
    OutputLine("ActiveRouters: %u", aConnectivity.mActiveRouters);

    OutputSpaces(aColumn);
    OutputLine("SedBufferSize: %u", aConnectivity.mSedBufferSize);

    OutputSpaces(aColumn);
    OutputLine("SedDatagramCount: %u", aConnectivity.mSedDatagramCount);
}

void Interpreter::OutputRoute(const otNetworkDiagRoute &aRoute, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputLine("IdSequence: %u", aRoute.mIdSequence);

    OutputSpaces(aColumn);
    OutputLine("RouteData:");

    aColumn += kIndentationSize;
    for (uint16_t i = 0; i < aRoute.mRouteCount; ++i)
    {
        OutputSpaces(aColumn);
        OutputFormat("- ");

        OutputRouteData(aRoute.mRouteData[i], aColumn + 2);
    }
}

void Interpreter::OutputRouteData(const otNetworkDiagRouteData &aRouteData, uint16_t aColumn)
{
    OutputLine("RouteId: 0x%02x", aRouteData.mRouterId);

    OutputSpaces(aColumn);
    OutputLine("LinkQualityOut: %u", aRouteData.mLinkQualityOut);

    OutputSpaces(aColumn);
    OutputLine("LinkQualityIn: %u", aRouteData.mLinkQualityIn);

    OutputSpaces(aColumn);
    OutputLine("RouteCost: %u", aRouteData.mRouteCost);
}

void Interpreter::OutputLeaderData(const otLeaderData &aLeaderData, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputLine("PartitionId: 0x%08x", aLeaderData.mPartitionId);

    OutputSpaces(aColumn);
    OutputLine("Weighting: %u", aLeaderData.mWeighting);

    OutputSpaces(aColumn);
    OutputLine("DataVersion: %u", aLeaderData.mDataVersion);

    OutputSpaces(aColumn);
    OutputLine("StableDataVersion: %u", aLeaderData.mStableDataVersion);

    OutputSpaces(aColumn);
    OutputLine("LeaderRouterId: 0x%02x", aLeaderData.mLeaderRouterId);
}

void Interpreter::OutputNetworkDiagMacCounters(const otNetworkDiagMacCounters &aMacCounters, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputLine("IfInUnknownProtos: %u", aMacCounters.mIfInUnknownProtos);

    OutputSpaces(aColumn);
    OutputLine("IfInErrors: %u", aMacCounters.mIfInErrors);

    OutputSpaces(aColumn);
    OutputLine("IfOutErrors: %u", aMacCounters.mIfOutErrors);

    OutputSpaces(aColumn);
    OutputLine("IfInUcastPkts: %u", aMacCounters.mIfInUcastPkts);

    OutputSpaces(aColumn);
    OutputLine("IfInBroadcastPkts: %u", aMacCounters.mIfInBroadcastPkts);

    OutputSpaces(aColumn);
    OutputLine("IfInDiscards: %u", aMacCounters.mIfInDiscards);

    OutputSpaces(aColumn);
    OutputLine("IfOutUcastPkts: %u", aMacCounters.mIfOutUcastPkts);

    OutputSpaces(aColumn);
    OutputLine("IfOutBroadcastPkts: %u", aMacCounters.mIfOutBroadcastPkts);

    OutputSpaces(aColumn);
    OutputLine("IfOutDiscards: %u", aMacCounters.mIfOutDiscards);
}

void Interpreter::OutputChildTableEntry(const otNetworkDiagChildEntry &aChildEntry, uint16_t aColumn)
{
    OutputLine("ChildId: 0x%04x", aChildEntry.mChildId);

    OutputSpaces(aColumn);
    OutputLine("Timeout: %u", aChildEntry.mTimeout);

    OutputSpaces(aColumn);
    OutputLine("Mode:");

    OutputMode(aChildEntry.mMode, aColumn + kIndentationSize);
}
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE

void Interpreter::SetUserCommands(const otCliCommand *aCommands, uint8_t aLength)
{
    mUserCommands       = aCommands;
    mUserCommandsLength = aLength;
}

Interpreter &Interpreter::GetOwner(OwnerLocator &aOwnerLocator)
{
#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
    Interpreter &interpreter = (aOwnerLocator.GetOwner<Interpreter>());
#else
    OT_UNUSED_VARIABLE(aOwnerLocator);

    Interpreter &interpreter = Interpreter::GetInterpreter();
#endif
    return interpreter;
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
    OutputBytes(aInfo.mExtAddress.m8, sizeof(aInfo.mExtAddress.m8));
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

void Interpreter::OutputLine(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputFormat("\r\n");
}

int Interpreter::OutputFormatV(const char *aFormat, va_list aArguments)
{
    char buf[kMaxLineLength];

    vsnprintf(buf, sizeof(buf), aFormat, aArguments);

    return Output(buf, static_cast<uint16_t>(strlen(buf)));
}

extern "C" void otCliSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength)
{
    Interpreter::GetInterpreter().SetUserCommands(aUserCommands, aLength);
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

    VerifyOrExit(Interpreter::IsInitialized(), OT_NOOP);

    Interpreter::GetInterpreter().OutputFormatV(aFormat, aArgs);
    Interpreter::GetInterpreter().OutputLine("");
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
