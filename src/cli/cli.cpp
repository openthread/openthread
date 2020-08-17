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
#include <openthread/netdata.h>
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

#define INDENT_SIZE (4)

namespace ot {

namespace Cli {

const struct Command Interpreter::sCommands[] = {
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    {"bbr", &Interpreter::ProcessBackboneRouter},
#endif
    {"bufferinfo", &Interpreter::ProcessBufferInfo},
    {"channel", &Interpreter::ProcessChannel},
#if OPENTHREAD_FTD
    {"child", &Interpreter::ProcessChild},
    {"childip", &Interpreter::ProcessChildIp},
    {"childmax", &Interpreter::ProcessChildMax},
#endif
    {"childtimeout", &Interpreter::ProcessChildTimeout},
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
    {"coap", &Interpreter::ProcessCoap},
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
    {"coaps", &Interpreter::ProcessCoapSecure},
#endif
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
    {"coex", &Interpreter::ProcessCoexMetrics},
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    {"commissioner", &Interpreter::ProcessCommissioner},
#endif
#if OPENTHREAD_FTD
    {"contextreusedelay", &Interpreter::ProcessContextIdReuseDelay},
#endif
    {"counters", &Interpreter::ProcessCounters},
    {"dataset", &Interpreter::ProcessDataset},
#if OPENTHREAD_FTD
    {"delaytimermin", &Interpreter::ProcessDelayTimerMin},
#endif
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    {"diag", &Interpreter::ProcessDiag},
#endif
    {"discover", &Interpreter::ProcessDiscover},
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    {"dns", &Interpreter::ProcessDns},
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    {"domainname", &Interpreter::ProcessDomainName},
#endif
#if OPENTHREAD_CONFIG_DUA_ENABLE
    {"dua", &Interpreter::ProcessDua},
#endif
#if OPENTHREAD_FTD
    {"eidcache", &Interpreter::ProcessEidCache},
#endif
    {"eui64", &Interpreter::ProcessEui64},
#if OPENTHREAD_POSIX
    {"exit", &Interpreter::ProcessExit},
#endif
    {"log", &Interpreter::ProcessLog},
    {"extaddr", &Interpreter::ProcessExtAddress},
    {"extpanid", &Interpreter::ProcessExtPanId},
    {"factoryreset", &Interpreter::ProcessFactoryReset},
    {"help", &Interpreter::ProcessHelp},
    {"ifconfig", &Interpreter::ProcessIfconfig},
    {"ipaddr", &Interpreter::ProcessIpAddr},
    {"ipmaddr", &Interpreter::ProcessIpMulticastAddr},
#if OPENTHREAD_CONFIG_JOINER_ENABLE
    {"joiner", &Interpreter::ProcessJoiner},
#endif
#if OPENTHREAD_FTD
    {"joinerport", &Interpreter::ProcessJoinerPort},
#endif
    {"keysequence", &Interpreter::ProcessKeySequence},
    {"leaderdata", &Interpreter::ProcessLeaderData},
#if OPENTHREAD_FTD
    {"leaderpartitionid", &Interpreter::ProcessLeaderPartitionId},
    {"leaderweight", &Interpreter::ProcessLeaderWeight},
#endif
    {"mac", &Interpreter::ProcessMac},
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    {"macfilter", &Interpreter::ProcessMacFilter},
#endif
    {"masterkey", &Interpreter::ProcessMasterKey},
    {"mode", &Interpreter::ProcessMode},
#if OPENTHREAD_FTD
    {"neighbor", &Interpreter::ProcessNeighbor},
#endif
    {"netdata", &Interpreter::ProcessNetworkData},
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    {"netdataregister", &Interpreter::ProcessNetworkDataRegister},
#endif
    {"netdatashow", &Interpreter::ProcessNetworkDataShow},
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    {"netif", &Interpreter::ProcessNetif},
#endif
    {"netstat", &Interpreter::ProcessNetstat},
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
    {"networkdiagnostic", &Interpreter::ProcessNetworkDiagnostic},
#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
#if OPENTHREAD_FTD
    {"networkidtimeout", &Interpreter::ProcessNetworkIdTimeout},
#endif
    {"networkname", &Interpreter::ProcessNetworkName},
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {"networktime", &Interpreter::ProcessNetworkTime},
#endif
    {"panid", &Interpreter::ProcessPanId},
    {"parent", &Interpreter::ProcessParent},
#if OPENTHREAD_FTD
    {"parentpriority", &Interpreter::ProcessParentPriority},
#endif
    {"ping", &Interpreter::ProcessPing},
    {"pollperiod", &Interpreter::ProcessPollPeriod},
    {"promiscuous", &Interpreter::ProcessPromiscuous},
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    {"prefix", &Interpreter::ProcessPrefix},
#endif
#if OPENTHREAD_FTD
    {"preferrouterid", &Interpreter::ProcessPreferRouterId},
    {"pskc", &Interpreter::ProcessPskc},
#endif
    {"rcp", &Interpreter::ProcessRcp},
#if OPENTHREAD_FTD
    {"releaserouterid", &Interpreter::ProcessReleaseRouterId},
#endif
    {"reset", &Interpreter::ProcessReset},
    {"rloc16", &Interpreter::ProcessRloc16},
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    {"route", &Interpreter::ProcessRoute},
#endif
#if OPENTHREAD_FTD
    {"router", &Interpreter::ProcessRouter},
    {"routerdowngradethreshold", &Interpreter::ProcessRouterDowngradeThreshold},
    {"routereligible", &Interpreter::ProcessRouterEligible},
    {"routerselectionjitter", &Interpreter::ProcessRouterSelectionJitter},
    {"routerupgradethreshold", &Interpreter::ProcessRouterUpgradeThreshold},
#endif
    {"scan", &Interpreter::ProcessScan},
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    {"service", &Interpreter::ProcessService},
#endif
    {"singleton", &Interpreter::ProcessSingleton},
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    {"sntp", &Interpreter::ProcessSntp},
#endif
    {"state", &Interpreter::ProcessState},
    {"thread", &Interpreter::ProcessThread},
    {"txpower", &Interpreter::ProcessTxPower},
    {"udp", &Interpreter::ProcessUdp},
    {"unsecureport", &Interpreter::ProcessUnsecurePort},
    {"version", &Interpreter::ProcessVersion},
};

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
    , mUdp(*this)
    , mDataset(*this)
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
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
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

void Interpreter::AppendResult(otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        OutputFormat("Done\r\n");
    }
    else
    {
        OutputFormat("Error %d: %s\r\n", aError, otThreadErrorToString(aError));
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

void Interpreter::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        OutputFormat("%s\r\n", command.mName);
    }

    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        OutputFormat("%s\r\n", mUserCommands[i].mName);
    }

    AppendResult(OT_ERROR_NONE);
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
void Interpreter::ProcessBackboneRouter(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);
    otError                error = OT_ERROR_INVALID_COMMAND;
    otBackboneRouterConfig config;

    if (aArgsLength == 0)
    {
        if (otBackboneRouterGetPrimary(mInstance, &config) == OT_ERROR_NONE)
        {
            OutputFormat("BBR Primary:\r\n");
            OutputFormat("server16: 0x%04X\r\n", config.mServer16);
            OutputFormat("seqno:    %d\r\n", config.mSequenceNumber);
            OutputFormat("delay:    %d secs\r\n", config.mReregistrationDelay);
            OutputFormat("timeout:  %d secs\r\n", config.mMlrTimeout);
        }
        else
        {
            OutputFormat("BBR Primary: None\r\n");
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

            VerifyOrExit((aArgsLength == 3 || aArgsLength == 4), error = OT_ERROR_INVALID_ARGS);

            if (strcmp(aArgs[1], "dua") == 0)
            {
                otIp6InterfaceIdentifier *mlIid = nullptr;
                otIp6InterfaceIdentifier  iid;

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
        }
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        SuccessOrExit(error = ProcessBackboneRouterLocal(aArgsLength, aArgs));
    }

exit:
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

    AppendResult(error);
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
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
            OutputFormat("%d\r\n", otBackboneRouterGetRegistrationJitter(mInstance));
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
            OutputFormat("Disabled\r\n");
            break;
        case OT_BACKBONE_ROUTER_STATE_SECONDARY:
            OutputFormat("Secondary\r\n");
            break;
        case OT_BACKBONE_ROUTER_STATE_PRIMARY:
            OutputFormat("Primary\r\n");
            break;
        }
    }
    else if (strcmp(aArgs[0], "config") == 0)
    {
        otBackboneRouterGetConfig(mInstance, &config);

        if (aArgsLength == 1)
        {
            OutputFormat("seqno:    %d\r\n", config.mSequenceNumber);
            OutputFormat("delay:    %d secs\r\n", config.mReregistrationDelay);
            OutputFormat("timeout:  %d secs\r\n", config.mMlrTimeout);
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

void Interpreter::ProcessDomainName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const char *domainName = otThreadGetDomainName(mInstance);
        OutputFormat("%s\r\n", static_cast<const char *>(domainName));
    }
    else
    {
        SuccessOrExit(error = otThreadSetDomainName(mInstance, aArgs[0]));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
void Interpreter::ProcessDua(uint8_t aArgsLength, char *aArgs[])
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
            OutputFormat("\r\n");
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
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

void Interpreter::ProcessBufferInfo(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(mInstance, &bufferInfo);

    OutputFormat("total: %d\r\n", bufferInfo.mTotalBuffers);
    OutputFormat("free: %d\r\n", bufferInfo.mFreeBuffers);
    OutputFormat("6lo send: %d %d\r\n", bufferInfo.m6loSendMessages, bufferInfo.m6loSendBuffers);
    OutputFormat("6lo reas: %d %d\r\n", bufferInfo.m6loReassemblyMessages, bufferInfo.m6loReassemblyBuffers);
    OutputFormat("ip6: %d %d\r\n", bufferInfo.mIp6Messages, bufferInfo.mIp6Buffers);
    OutputFormat("mpl: %d %d\r\n", bufferInfo.mMplMessages, bufferInfo.mMplBuffers);
    OutputFormat("mle: %d %d\r\n", bufferInfo.mMleMessages, bufferInfo.mMleBuffers);
    OutputFormat("arp: %d %d\r\n", bufferInfo.mArpMessages, bufferInfo.mArpBuffers);
    OutputFormat("coap: %d %d\r\n", bufferInfo.mCoapMessages, bufferInfo.mCoapBuffers);
    OutputFormat("coap secure: %d %d\r\n", bufferInfo.mCoapSecureMessages, bufferInfo.mCoapSecureBuffers);
    OutputFormat("application coap: %d %d\r\n", bufferInfo.mApplicationCoapMessages,
                 bufferInfo.mApplicationCoapBuffers);

    AppendResult(OT_ERROR_NONE);
}

void Interpreter::ProcessChannel(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otLinkGetChannel(mInstance));
    }
    else if (strcmp(aArgs[0], "supported") == 0)
    {
        OutputFormat("0x%x\r\n", otPlatRadioGetSupportedChannelMask(mInstance));
    }
    else if (strcmp(aArgs[0], "preferred") == 0)
    {
        OutputFormat("0x%x\r\n", otPlatRadioGetPreferredChannelMask(mInstance));
    }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    else if (strcmp(aArgs[0], "monitor") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputFormat("enabled: %d\r\n", otChannelMonitorIsEnabled(mInstance));
            if (otChannelMonitorIsEnabled(mInstance))
            {
                uint32_t channelMask = otLinkGetSupportedChannelMask(mInstance);
                uint8_t  channelNum  = sizeof(channelMask) * CHAR_BIT;

                OutputFormat("interval: %u\r\n", otChannelMonitorGetSampleInterval(mInstance));
                OutputFormat("threshold: %d\r\n", otChannelMonitorGetRssiThreshold(mInstance));
                OutputFormat("window: %u\r\n", otChannelMonitorGetSampleWindow(mInstance));
                OutputFormat("count: %u\r\n", otChannelMonitorGetSampleCount(mInstance));

                OutputFormat("occupancies:\r\n");
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
                    OutputFormat("%2d.%02d%% busy\r\n", occupancy / 100, occupancy % 100);
                }
                OutputFormat("\r\n");
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
            OutputFormat("channel: %d\r\n", otChannelManagerGetRequestedChannel(mInstance));
            OutputFormat("auto: %d\r\n", otChannelManagerGetAutoChannelSelectionEnabled(mInstance));

            if (otChannelManagerGetAutoChannelSelectionEnabled(mInstance))
            {
                Mac::ChannelMask supportedMask(otChannelManagerGetSupportedChannels(mInstance));
                Mac::ChannelMask favoredMask(otChannelManagerGetFavoredChannels(mInstance));

                OutputFormat("delay: %d\r\n", otChannelManagerGetDelay(mInstance));
                OutputFormat("interval: %lu\r\n", otChannelManagerGetAutoChannelSelectionInterval(mInstance));
                OutputFormat("supported: %s\r\n", supportedMask.ToString().AsCString());
                OutputFormat("favored: %s\r\n", supportedMask.ToString().AsCString());
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
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessChild(uint8_t aArgsLength, char *aArgs[])
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
            OutputFormat("| ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|S|D|N| Extended MAC     |\r\n");
            OutputFormat("+-----+--------+------------+------------+-------+------+-+-+-+-+------------------+\r\n");
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

                OutputFormat(" |\r\n");
            }
            else
            {
                OutputFormat("%d ", childInfo.mChildId);
            }
        }

        OutputFormat("\r\n");
        ExitNow();
    }

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadGetChildInfoById(mInstance, static_cast<uint16_t>(value), &childInfo));

    OutputFormat("Child ID: %d\r\n", childInfo.mChildId);
    OutputFormat("Rloc: %04x\r\n", childInfo.mRloc16);
    OutputFormat("Ext Addr: ");

    for (uint8_t b : childInfo.mExtAddress.m8)
    {
        OutputFormat("%02x", b);
    }

    OutputFormat("\r\n");
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

    OutputFormat("\r\n");

    OutputFormat("Net Data: %d\r\n", childInfo.mNetworkDataVersion);
    OutputFormat("Timeout: %d\r\n", childInfo.mTimeout);
    OutputFormat("Age: %d\r\n", childInfo.mAge);
    OutputFormat("Link Quality In: %d\r\n", childInfo.mLinkQualityIn);
    OutputFormat("RSSI: %d\r\n", childInfo.mAverageRssi);

exit:
    AppendResult(error);
}

void Interpreter::ProcessChildIp(uint8_t aArgsLength, char *aArgs[])
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
                OutputFormat("\r\n");
            }
        }
    }
    else if (strcmp(aArgs[0], "max") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputFormat("%d\r\n", otThreadGetMaxChildIpAddresses(mInstance));
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
    AppendResult(error);
}

void Interpreter::ProcessChildMax(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetMaxAllowedChildren(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otThreadSetMaxAllowedChildren(mInstance, static_cast<uint16_t>(value)));
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessChildTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetChildTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetChildTimeout(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_CONFIG_COAP_API_ENABLE

void Interpreter::ProcessCoap(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mCoap.Process(aArgsLength, aArgs);
    AppendResult(error);
}

#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

void Interpreter::ProcessCoapSecure(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mCoapSecure.Process(aArgsLength, aArgs);
    AppendResult(error);
}

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
void Interpreter::ProcessCoexMetrics(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputFormat("%s\r\n", otPlatRadioIsCoexEnabled(mInstance) ? "Enabled" : "Disabled");
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

        OutputFormat("Stopped: %s\r\n", metrics.mStopped ? "true" : "false");
        OutputFormat("Grant Glitch: %u\r\n", metrics.mNumGrantGlitch);
        OutputFormat("Transmit metrics\r\n");
        OutputFormat("    Request: %u\r\n", metrics.mNumTxRequest);
        OutputFormat("    Grant Immediate: %u\r\n", metrics.mNumTxGrantImmediate);
        OutputFormat("    Grant Wait: %u\r\n", metrics.mNumTxGrantWait);
        OutputFormat("    Grant Wait Activated: %u\r\n", metrics.mNumTxGrantWaitActivated);
        OutputFormat("    Grant Wait Timeout: %u\r\n", metrics.mNumTxGrantWaitTimeout);
        OutputFormat("    Grant Deactivated During Request: %u\r\n", metrics.mNumTxGrantDeactivatedDuringRequest);
        OutputFormat("    Delayed Grant: %u\r\n", metrics.mNumTxDelayedGrant);
        OutputFormat("    Average Request To Grant Time: %u\r\n", metrics.mAvgTxRequestToGrantTime);
        OutputFormat("Receive metrics\r\n");
        OutputFormat("    Request: %u\r\n", metrics.mNumRxRequest);
        OutputFormat("    Grant Immediate: %u\r\n", metrics.mNumRxGrantImmediate);
        OutputFormat("    Grant Wait: %u\r\n", metrics.mNumRxGrantWait);
        OutputFormat("    Grant Wait Activated: %u\r\n", metrics.mNumRxGrantWaitActivated);
        OutputFormat("    Grant Wait Timeout: %u\r\n", metrics.mNumRxGrantWaitTimeout);
        OutputFormat("    Grant Deactivated During Request: %u\r\n", metrics.mNumRxGrantDeactivatedDuringRequest);
        OutputFormat("    Delayed Grant: %u\r\n", metrics.mNumRxDelayedGrant);
        OutputFormat("    Average Request To Grant Time: %u\r\n", metrics.mAvgRxRequestToGrantTime);
        OutputFormat("    Grant None: %u\r\n", metrics.mNumRxGrantNone);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE

#if OPENTHREAD_FTD
void Interpreter::ProcessContextIdReuseDelay(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetContextIdReuseDelay(mInstance));
    }
    else
    {
        SuccessOrExit(ParseLong(aArgs[0], value));
        otThreadSetContextIdReuseDelay(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessCounters(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputFormat("mac\r\n");
        OutputFormat("mle\r\n");
    }
    else if (strcmp(aArgs[0], "mac") == 0)
    {
        if (aArgsLength == 1)
        {
            const otMacCounters *macCounters = otLinkGetCounters(mInstance);

            OutputFormat("TxTotal: %d\r\n", macCounters->mTxTotal);
            OutputFormat("    TxUnicast: %d\r\n", macCounters->mTxUnicast);
            OutputFormat("    TxBroadcast: %d\r\n", macCounters->mTxBroadcast);
            OutputFormat("    TxAckRequested: %d\r\n", macCounters->mTxAckRequested);
            OutputFormat("    TxAcked: %d\r\n", macCounters->mTxAcked);
            OutputFormat("    TxNoAckRequested: %d\r\n", macCounters->mTxNoAckRequested);
            OutputFormat("    TxData: %d\r\n", macCounters->mTxData);
            OutputFormat("    TxDataPoll: %d\r\n", macCounters->mTxDataPoll);
            OutputFormat("    TxBeacon: %d\r\n", macCounters->mTxBeacon);
            OutputFormat("    TxBeaconRequest: %d\r\n", macCounters->mTxBeaconRequest);
            OutputFormat("    TxOther: %d\r\n", macCounters->mTxOther);
            OutputFormat("    TxRetry: %d\r\n", macCounters->mTxRetry);
            OutputFormat("    TxErrCca: %d\r\n", macCounters->mTxErrCca);
            OutputFormat("    TxErrBusyChannel: %d\r\n", macCounters->mTxErrBusyChannel);
            OutputFormat("RxTotal: %d\r\n", macCounters->mRxTotal);
            OutputFormat("    RxUnicast: %d\r\n", macCounters->mRxUnicast);
            OutputFormat("    RxBroadcast: %d\r\n", macCounters->mRxBroadcast);
            OutputFormat("    RxData: %d\r\n", macCounters->mRxData);
            OutputFormat("    RxDataPoll: %d\r\n", macCounters->mRxDataPoll);
            OutputFormat("    RxBeacon: %d\r\n", macCounters->mRxBeacon);
            OutputFormat("    RxBeaconRequest: %d\r\n", macCounters->mRxBeaconRequest);
            OutputFormat("    RxOther: %d\r\n", macCounters->mRxOther);
            OutputFormat("    RxAddressFiltered: %d\r\n", macCounters->mRxAddressFiltered);
            OutputFormat("    RxDestAddrFiltered: %d\r\n", macCounters->mRxDestAddrFiltered);
            OutputFormat("    RxDuplicated: %d\r\n", macCounters->mRxDuplicated);
            OutputFormat("    RxErrNoFrame: %d\r\n", macCounters->mRxErrNoFrame);
            OutputFormat("    RxErrNoUnknownNeighbor: %d\r\n", macCounters->mRxErrUnknownNeighbor);
            OutputFormat("    RxErrInvalidSrcAddr: %d\r\n", macCounters->mRxErrInvalidSrcAddr);
            OutputFormat("    RxErrSec: %d\r\n", macCounters->mRxErrSec);
            OutputFormat("    RxErrFcs: %d\r\n", macCounters->mRxErrFcs);
            OutputFormat("    RxErrOther: %d\r\n", macCounters->mRxErrOther);
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

            OutputFormat("Role Disabled: %d\r\n", mleCounters->mDisabledRole);
            OutputFormat("Role Detached: %d\r\n", mleCounters->mDetachedRole);
            OutputFormat("Role Child: %d\r\n", mleCounters->mChildRole);
            OutputFormat("Role Router: %d\r\n", mleCounters->mRouterRole);
            OutputFormat("Role Leader: %d\r\n", mleCounters->mLeaderRole);
            OutputFormat("Attach Attempts: %d\r\n", mleCounters->mAttachAttempts);
            OutputFormat("Partition Id Changes: %d\r\n", mleCounters->mPartitionIdChanges);
            OutputFormat("Better Partition Attach Attempts: %d\r\n", mleCounters->mBetterPartitionAttachAttempts);
            OutputFormat("Parent Changes: %d\r\n", mleCounters->mParentChanges);
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
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessDelayTimerMin(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", (otDatasetGetDelayTimerMinimal(mInstance) / 1000));
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
    AppendResult(error);
}
#endif

void Interpreter::ProcessDiscover(uint8_t aArgsLength, char *aArgs[])
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
    OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
    OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
void Interpreter::ProcessDns(uint8_t aArgsLength, char *aArgs[])
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

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
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
        OutputFormat(" TTL: %d\r\n", aTtl);
    }

    AppendResult(aResult);

    mResolvingInProgress = false;
}
#endif

#if OPENTHREAD_FTD
void Interpreter::ProcessEidCache(uint8_t aArgsLength, char *aArgs[])
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
        OutputFormat(" %04x\r\n", entry.mRloc16);
    }

exit:
    AppendResult(OT_ERROR_NONE);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessEui64(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError      error = OT_ERROR_NONE;
    otExtAddress extAddress;

    VerifyOrExit(aArgsLength == 0, error = OT_ERROR_INVALID_ARGS);

    otLinkGetFactoryAssignedIeeeEui64(mInstance, &extAddress);
    OutputBytes(extAddress.m8, OT_EXT_ADDRESS_SIZE);
    OutputFormat("\r\n");

exit:
    AppendResult(error);
}

void Interpreter::ProcessExtAddress(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *extAddress = reinterpret_cast<const uint8_t *>(otLinkGetExtendedAddress(mInstance));
        OutputBytes(extAddress, OT_EXT_ADDRESS_SIZE);
        OutputFormat("\r\n");
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
    AppendResult(error);
}

#if OPENTHREAD_POSIX
void Interpreter::ProcessExit(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    exit(EXIT_SUCCESS);
}
#endif

void Interpreter::ProcessLog(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength >= 1, error = OT_ERROR_INVALID_ARGS);

    if (!strcmp(aArgs[0], "level"))
    {
        if (aArgsLength == 1)
        {
            OutputFormat("%d\r\n", otLoggingGetLevel());
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
    AppendResult(error);
}

void Interpreter::ProcessExtPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *extPanId = reinterpret_cast<const uint8_t *>(otThreadGetExtendedPanId(mInstance));
        OutputBytes(extPanId, OT_EXT_PAN_ID_SIZE);
        OutputFormat("\r\n");
    }
    else
    {
        otExtendedPanId extPanId;

        VerifyOrExit(Hex2Bin(aArgs[0], extPanId.m8, sizeof(extPanId)) == sizeof(extPanId),
                     error = OT_ERROR_INVALID_ARGS);

        error = otThreadSetExtendedPanId(mInstance, &extPanId);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessFactoryReset(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceFactoryReset(mInstance);
}

void Interpreter::ProcessIfconfig(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otIp6IsEnabled(mInstance))
        {
            OutputFormat("up\r\n");
        }
        else
        {
            OutputFormat("down\r\n");
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
    AppendResult(error);
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

void Interpreter::ProcessIpAddr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(mInstance);

        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
        {
            OutputIp6Address(addr->mAddress);
            OutputFormat("\r\n");
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
            OutputFormat("\r\n");
        }
        else if (strcmp(aArgs[0], "rloc") == 0)
        {
            OutputIp6Address(*otThreadGetRloc(mInstance));
            OutputFormat("\r\n");
        }
        else if (strcmp(aArgs[0], "mleid") == 0)
        {
            OutputIp6Address(*otThreadGetMeshLocalEid(mInstance));
            OutputFormat("\r\n");
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_COMMAND);
        }
    }

exit:
    AppendResult(error);
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
            OutputFormat("Enabled\r\n");
        }
        else
        {
            OutputFormat("Disabled\r\n");
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

void Interpreter::ProcessIpMulticastAddr(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        for (const otNetifMulticastAddress *addr = otIp6GetMulticastAddresses(mInstance); addr; addr = addr->mNext)
        {
            OutputIp6Address(addr->mAddress);
            OutputFormat("\r\n");
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
    AppendResult(error);
}

void Interpreter::ProcessKeySequence(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength == 1 || aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "counter") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputFormat("%d\r\n", otThreadGetKeySequenceCounter(mInstance));
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
            OutputFormat("%d\r\n", otThreadGetKeySwitchGuardTime(mInstance));
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
    AppendResult(error);
}

void Interpreter::ProcessLeaderData(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError      error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(mInstance, &leaderData));

    OutputFormat("Partition ID: %u\r\n", leaderData.mPartitionId);
    OutputFormat("Weighting: %d\r\n", leaderData.mWeighting);
    OutputFormat("Data Version: %d\r\n", leaderData.mDataVersion);
    OutputFormat("Stable Data Version: %d\r\n", leaderData.mStableDataVersion);
    OutputFormat("Leader Router ID: %d\r\n", leaderData.mLeaderRouterId);

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessLeaderPartitionId(uint8_t aArgsLength, char *aArgs[])
{
    otError       error = OT_ERROR_NONE;
    unsigned long value;

    if (aArgsLength == 0)
    {
        OutputFormat("%u\r\n", otThreadGetLocalLeaderPartitionId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseUnsignedLong(aArgs[0], value));
        otThreadSetLocalLeaderPartitionId(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessLeaderWeight(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetLocalLeaderWeight(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetLocalLeaderWeight(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_FTD
void Interpreter::ProcessPskc(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const otPskc *pskc = otThreadGetPskc(mInstance);

        for (uint8_t b : pskc->m8)
        {
            OutputFormat("%02x", b);
        }

        OutputFormat("\r\n");
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
    AppendResult(error);
}
#endif

void Interpreter::ProcessMasterKey(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const uint8_t *key = reinterpret_cast<const uint8_t *>(otThreadGetMasterKey(mInstance));

        for (int i = 0; i < OT_MASTER_KEY_SIZE; i++)
        {
            OutputFormat("%02x", key[i]);
        }

        OutputFormat("\r\n");
    }
    else
    {
        otMasterKey key;

        VerifyOrExit(Hex2Bin(aArgs[0], key.m8, sizeof(key.m8)) == OT_MASTER_KEY_SIZE, error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otThreadSetMasterKey(mInstance, &key));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessMode(uint8_t aArgsLength, char *aArgs[])
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

        OutputFormat("\r\n");
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
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessNeighbor(uint8_t aArgsLength, char *aArgs[])
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
            OutputFormat("| Role | RLOC16 | Age | Avg RSSI | Last RSSI |R|S|D|N| Extended MAC     |\r\n");
            OutputFormat("+------+--------+-----+----------+-----------+-+-+-+-+------------------+\r\n");
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

                OutputFormat(" |\r\n");
            }
            else
            {
                OutputFormat("0x%04x ", neighborInfo.mRloc16);
            }
        }

        OutputFormat("\r\n");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessNetworkDataShow(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;
    uint8_t data[255];
    uint8_t len = sizeof(data);

    SuccessOrExit(error = otNetDataGet(mInstance, false, data, &len));

    OutputBytes(data, static_cast<uint8_t>(len));
    OutputFormat("\r\n");

exit:
    AppendResult(error);
}

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
void Interpreter::ProcessNetif(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError      error    = OT_ERROR_NONE;
    const char * netif    = nullptr;
    unsigned int netifidx = 0;

    SuccessOrExit(error = otPlatGetNetif(mInstance, &netif, &netifidx));

    OutputFormat("%s:%u\r\n", netif ? netif : "(null)", netifidx);

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessNetstat(uint8_t aArgsLength, char *aArgs[])
{
    otUdpSocket *socket = otUdpGetSockets(mInstance);

    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    OutputFormat("|                 Local Address                 |                  Peer Address                 |\n");
    OutputFormat("+-----------------------------------------------+-----------------------------------------------+\n");

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
        OutputFormat(" |\n");

        socket = socket->mNext;
    }

    AppendResult(OT_ERROR_NONE);
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
void Interpreter::ProcessService(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "add") == 0)
    {
        otServiceConfig cfg;
        long            enterpriseNumber;
        size_t          length;

        VerifyOrExit(aArgsLength > 3, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(aArgs[1], enterpriseNumber));
        cfg.mEnterpriseNumber = static_cast<uint32_t>(enterpriseNumber);

        length = strlen(aArgs[2]);
        VerifyOrExit(length <= sizeof(cfg.mServiceData), error = OT_ERROR_NO_BUFS);
        cfg.mServiceDataLength = static_cast<uint8_t>(length);
        memcpy(cfg.mServiceData, aArgs[2], cfg.mServiceDataLength);

        length = strlen(aArgs[3]);
        VerifyOrExit(length <= sizeof(cfg.mServerConfig.mServerData), error = OT_ERROR_NO_BUFS);
        cfg.mServerConfig.mServerDataLength = static_cast<uint8_t>(length);
        memcpy(cfg.mServerConfig.mServerData, aArgs[3], cfg.mServerConfig.mServerDataLength);

        cfg.mServerConfig.mStable = true;

        SuccessOrExit(error = otServerAddService(mInstance, &cfg));
    }
    else if (strcmp(aArgs[0], "remove") == 0)
    {
        long enterpriseNumber = 0;

        VerifyOrExit(aArgsLength > 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(aArgs[1], enterpriseNumber));

        SuccessOrExit(error = otServerRemoveService(mInstance, static_cast<uint32_t>(enterpriseNumber),
                                                    reinterpret_cast<uint8_t *>(aArgs[2]),
                                                    static_cast<uint8_t>(strlen(aArgs[2]))));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessNetworkData(uint8_t aArgsLength, char *aArgs[])
{
    otError error;

    if (aArgsLength > 2 && strcmp(aArgs[0], "steeringdata") == 0)
    {
        if (strcmp(aArgs[1], "check") == 0)
        {
            otExtAddress      addr;
            otJoinerDiscerner discerner;

            discerner.mLength = 0;

            error = Interpreter::ParseJoinerDiscerner(aArgs[2], discerner);

            if (error == OT_ERROR_NOT_FOUND)
            {
                VerifyOrExit(Interpreter::Hex2Bin(aArgs[2], addr.m8, sizeof(addr)) == sizeof(addr),
                             error = OT_ERROR_INVALID_ARGS);
            }
            else if (error != OT_ERROR_NONE)
            {
                ExitNow();
            }

            if (discerner.mLength)
            {
                ExitNow(error = otNetDataSteeringDataCheckJoinerWithDiscerner(mInstance, &discerner));
            }
            else
            {
                ExitNow(error = otNetDataSteeringDataCheckJoiner(mInstance, &addr));
            }
        }
    }

    error = OT_ERROR_INVALID_COMMAND;

exit:
    AppendResult(error);
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
void Interpreter::ProcessNetworkDataRegister(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    SuccessOrExit(error = otBorderRouterRegister(mInstance));
#else
    SuccessOrExit(error = otServerRegister(mInstance));
#endif

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if OPENTHREAD_FTD
void Interpreter::ProcessNetworkIdTimeout(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetNetworkIdTimeout(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetNetworkIdTimeout(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessNetworkName(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        const char *networkName = otThreadGetNetworkName(mInstance);
        OutputFormat("%s\r\n", static_cast<const char *>(networkName));
    }
    else
    {
        SuccessOrExit(error = otThreadSetNetworkName(mInstance, aArgs[0]));
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
void Interpreter::ProcessNetworkTime(uint8_t aArgsLength, char *aArgs[])
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
            OutputFormat(" (unsynchronized)\r\n");
            break;

        case OT_NETWORK_TIME_RESYNC_NEEDED:
            OutputFormat(" (resync needed)\r\n");
            break;

        case OT_NETWORK_TIME_SYNCHRONIZED:
            OutputFormat(" (synchronized)\r\n");
            break;

        default:
            break;
        }

        OutputFormat("Time Sync Period: %ds\r\n", otNetworkTimeGetSyncPeriod(mInstance));
        OutputFormat("XTAL Threshold:   %dppm\r\n", otNetworkTimeGetXtalThreshold(mInstance));
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
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

void Interpreter::ProcessPanId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("0x%04x\r\n", otLinkGetPanId(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otLinkSetPanId(mInstance, static_cast<otPanId>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessParent(uint8_t aArgsLength, char *aArgs[])
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

    OutputFormat("\r\n");

    OutputFormat("Rloc: %x\r\n", parentInfo.mRloc16);
    OutputFormat("Link Quality In: %d\r\n", parentInfo.mLinkQualityIn);
    OutputFormat("Link Quality Out: %d\r\n", parentInfo.mLinkQualityOut);
    OutputFormat("Age: %d\r\n", parentInfo.mAge);

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessParentPriority(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetParentPriority(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otThreadSetParentPriority(mInstance, static_cast<int8_t>(value));
    }

exit:
    AppendResult(error);
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

    OutputFormat("\r\n");

    SignalPingReply(static_cast<const Ip6::MessageInfo *>(aMessageInfo)->GetPeerAddr(), dataSize, HostSwap32(timestamp),
                    aMessageInfo->mHopLimit);

exit:
    return;
}

void Interpreter::ProcessPing(uint8_t aArgsLength, char *aArgs[])
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
    AppendResult(error);
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

void Interpreter::ProcessPollPeriod(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otLinkGetPollPeriod(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otLinkSetPollPeriod(mInstance, static_cast<uint32_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessPromiscuous(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otLinkIsPromiscuous(mInstance) && otPlatRadioGetPromiscuous(mInstance))
        {
            OutputFormat("Enabled\r\n");
        }
        else
        {
            OutputFormat("Disabled\r\n");
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
    AppendResult(error);
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkPcapReceive(aFrame, aIsTx);
}

void Interpreter::HandleLinkPcapReceive(const otRadioFrame *aFrame, bool aIsTx)
{
    OT_UNUSED_VARIABLE(aIsTx);

    OutputFormat("\r\n");

    for (size_t i = 0; i < 44; i++)
    {
        OutputFormat("=");
    }

    OutputFormat("[len = %3u]", aFrame->mLength);

    for (size_t i = 0; i < 28; i++)
    {
        OutputFormat("=");
    }

    OutputFormat("\r\n");

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

        OutputFormat("|\r\n");
    }

    for (size_t i = 0; i < 83; i++)
    {
        OutputFormat("-");
    }

    OutputFormat("\r\n");
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

void Interpreter::OutputPrefix(otBorderRouterConfig &aConfig)
{
    OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[0]),
                 HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[1]), HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[2]),
                 HostSwap16(aConfig.mPrefix.mPrefix.mFields.m16[3]), aConfig.mPrefix.mLength);

    if (aConfig.mPreferred)
    {
        OutputFormat("p");
    }

    if (aConfig.mSlaac)
    {
        OutputFormat("a");
    }

    if (aConfig.mDhcp)
    {
        OutputFormat("d");
    }

    if (aConfig.mConfigure)
    {
        OutputFormat("c");
    }

    if (aConfig.mDefaultRoute)
    {
        OutputFormat("r");
    }

    if (aConfig.mOnMesh)
    {
        OutputFormat("o");
    }

    if (aConfig.mStable)
    {
        OutputFormat("s");
    }

    if (aConfig.mNdDns)
    {
        OutputFormat("n");
    }

    if (aConfig.mDp)
    {
        OutputFormat("D");
    }

    switch (aConfig.mPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        OutputFormat(" low");
        break;

    case OT_ROUTE_PREFERENCE_MED:
        OutputFormat(" med");
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        OutputFormat(" high");
        break;
    }

    OutputFormat("\r\n");
}

otError Interpreter::ProcessPrefixList(void)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;

    while (otBorderRouterGetNextOnMeshPrefix(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        OutputPrefix(config);
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (otBackboneRouterGetState(mInstance) == OT_BACKBONE_ROUTER_STATE_DISABLED)
    {
        SuccessOrExit(otBackboneRouterGetDomainPrefix(mInstance, &config));
        OutputFormat("- ");
        OutputPrefix(config);
    }
    // Else already printed via above while loop.
exit:
#endif

    return OT_ERROR_NONE;
}

void Interpreter::ProcessPrefix(uint8_t aArgsLength, char *aArgs[])
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
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
void Interpreter::ProcessPreferRouterId(uint8_t aArgsLength, char *aArgs[])
{
    otError       error = OT_ERROR_NONE;
    unsigned long value;

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
    SuccessOrExit(error = ParseUnsignedLong(aArgs[0], value));
    error = otThreadSetPreferredRouterId(mInstance, static_cast<uint8_t>(value));

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessRcp(uint8_t aArgsLength, char *aArgs[])
{
    otError     error   = OT_ERROR_NONE;
    const char *version = otPlatRadioGetVersionString(mInstance);

    VerifyOrExit(version != otGetVersionString(), error = OT_ERROR_NOT_IMPLEMENTED);
    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "version") == 0)
    {
        OutputFormat("%s\r\n", version);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    AppendResult(error);
}

#if OPENTHREAD_FTD
void Interpreter::ProcessReleaseRouterId(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadReleaseRouterId(mInstance, static_cast<uint8_t>(value)));

exit:
    AppendResult(error);
}
#endif

void Interpreter::ProcessReset(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceReset(mInstance);
}

void Interpreter::ProcessRloc16(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    OutputFormat("%04x\r\n", otThreadGetRloc16(mInstance));
    AppendResult(OT_ERROR_NONE);
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
        OutputFormat("%x:%x:%x:%x::/%d ", HostSwap16(config.mPrefix.mPrefix.mFields.m16[0]),
                     HostSwap16(config.mPrefix.mPrefix.mFields.m16[1]),
                     HostSwap16(config.mPrefix.mPrefix.mFields.m16[2]),
                     HostSwap16(config.mPrefix.mPrefix.mFields.m16[3]), config.mPrefix.mLength);

        if (config.mStable)
        {
            OutputFormat("s");
        }

        switch (config.mPreference)
        {
        case OT_ROUTE_PREFERENCE_LOW:
            OutputFormat(" low\r\n");
            break;

        case OT_ROUTE_PREFERENCE_MED:
            OutputFormat(" med\r\n");
            break;

        case OT_ROUTE_PREFERENCE_HIGH:
            OutputFormat(" high\r\n");
            break;
        }
    }

    return OT_ERROR_NONE;
}

void Interpreter::ProcessRoute(uint8_t aArgsLength, char *aArgs[])
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
    AppendResult(error);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
void Interpreter::ProcessRouter(uint8_t aArgsLength, char *aArgs[])
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
            OutputFormat("| ID | RLOC16 | Next Hop | Path Cost | LQ In | LQ Out | Age | Extended MAC     |\r\n");
            OutputFormat("+----+--------+----------+-----------+-------+--------+-----+------------------+\r\n");
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

                OutputFormat(" |\r\n");
            }
            else
            {
                OutputFormat("%d ", i);
            }
        }

        OutputFormat("\r\n");
        ExitNow();
    }

    SuccessOrExit(error = ParseLong(aArgs[0], value));
    SuccessOrExit(error = otThreadGetRouterInfo(mInstance, static_cast<uint16_t>(value), &routerInfo));

    OutputFormat("Alloc: %d\r\n", routerInfo.mAllocated);

    if (routerInfo.mAllocated)
    {
        OutputFormat("Router ID: %d\r\n", routerInfo.mRouterId);
        OutputFormat("Rloc: %04x\r\n", routerInfo.mRloc16);
        OutputFormat("Next Hop: %04x\r\n", static_cast<uint16_t>(routerInfo.mNextHop) << 10);
        OutputFormat("Link: %d\r\n", routerInfo.mLinkEstablished);

        if (routerInfo.mLinkEstablished)
        {
            OutputFormat("Ext Addr: ");

            for (uint8_t b : routerInfo.mExtAddress.m8)
            {
                OutputFormat("%02x", b);
            }

            OutputFormat("\r\n");
            OutputFormat("Cost: %d\r\n", routerInfo.mPathCost);
            OutputFormat("Link Quality In: %d\r\n", routerInfo.mLinkQualityIn);
            OutputFormat("Link Quality Out: %d\r\n", routerInfo.mLinkQualityOut);
            OutputFormat("Age: %d\r\n", routerInfo.mAge);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterDowngradeThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetRouterDowngradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetRouterDowngradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterEligible(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        if (otThreadIsRouterEligible(mInstance))
        {
            OutputFormat("Enabled\r\n");
        }
        else
        {
            OutputFormat("Disabled\r\n");
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
    AppendResult(error);
}

void Interpreter::ProcessRouterSelectionJitter(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetRouterSelectionJitter(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        VerifyOrExit(0 < value && value < 256, error = OT_ERROR_INVALID_ARGS);
        otThreadSetRouterSelectionJitter(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessRouterUpgradeThreshold(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetRouterUpgradeThreshold(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        otThreadSetRouterUpgradeThreshold(mInstance, static_cast<uint8_t>(value));
    }

exit:
    AppendResult(error);
}
#endif // OPENTHREAD_FTD

void Interpreter::ProcessScan(uint8_t aArgsLength, char *aArgs[])
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
        OutputFormat("| Ch | RSSI |\r\n");
        OutputFormat("+----+------+\r\n");
        SuccessOrExit(error = otLinkEnergyScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::HandleEnergyScanResult, this));
    }
    else
    {
        OutputFormat("| J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |\r\n");
        OutputFormat("+---+------------------+------------------+------+------------------+----+-----+-----+\r\n");
        SuccessOrExit(error = otLinkActiveScan(mInstance, scanChannels, scanDuration,
                                               &Interpreter::HandleActiveScanResult, this));
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleActiveScanResult(aResult);
}

void Interpreter::HandleActiveScanResult(otActiveScanResult *aResult)
{
    if (aResult == nullptr)
    {
        AppendResult(OT_ERROR_NONE);
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
    OutputFormat("| %3d |\r\n", aResult->mLqi);

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
        AppendResult(OT_ERROR_NONE);
        ExitNow();
    }

    OutputFormat("| %2d | %4d |\r\n", aResult->mChannel, aResult->mMaxRssi);

exit:
    return;
}

void Interpreter::ProcessSingleton(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;

    if (otThreadIsSingleton(mInstance))
    {
        OutputFormat("true\r\n");
    }
    else
    {
        OutputFormat("false\r\n");
    }

    AppendResult(error);
}

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
void Interpreter::ProcessSntp(uint8_t aArgsLength, char *aArgs[])
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

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
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
        OutputFormat("SNTP response - Unix time: %u (era: %u)\r\n", static_cast<uint32_t>(aTime),
                     static_cast<uint32_t>(aTime >> 32));
    }
    else
    {
        OutputFormat("SNTP error - %s\r\n", otThreadErrorToString(aResult));
    }

    mSntpQueryingInProgress = false;

    AppendResult(OT_ERROR_NONE);
}
#endif

void Interpreter::ProcessState(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        switch (otThreadGetDeviceRole(mInstance))
        {
        case OT_DEVICE_ROLE_DISABLED:
            OutputFormat("disabled\r\n");
            break;

        case OT_DEVICE_ROLE_DETACHED:
            OutputFormat("detached\r\n");
            break;

        case OT_DEVICE_ROLE_CHILD:
            OutputFormat("child\r\n");
            break;

#if OPENTHREAD_FTD

        case OT_DEVICE_ROLE_ROUTER:
            OutputFormat("router\r\n");
            break;

        case OT_DEVICE_ROLE_LEADER:
            OutputFormat("leader\r\n");
            break;
#endif // OPENTHREAD_FTD

        default:
            OutputFormat("invalid state\r\n");
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

#endif // OPENTHREAD_FTD
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessThread(uint8_t aArgsLength, char *aArgs[])
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
        OutputFormat("%u\r\n", otThreadGetVersion());
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessDataset(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mDataset.Process(aArgsLength, aArgs);
    AppendResult(error);
}

void Interpreter::ProcessTxPower(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 0)
    {
        int8_t power;

        SuccessOrExit(error = otPlatRadioGetTransmitPower(mInstance, &power));
        OutputFormat("%d dBm\r\n", power);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgs[0], value));
        SuccessOrExit(error = otPlatRadioSetTransmitPower(mInstance, static_cast<int8_t>(value)));
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessUdp(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mUdp.Process(aArgsLength, aArgs);
    AppendResult(error);
}

void Interpreter::ProcessUnsecurePort(uint8_t aArgsLength, char *aArgs[])
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

        OutputFormat("\r\n");
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    AppendResult(error);
}

void Interpreter::ProcessVersion(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    const char *version = otGetVersionString();
    OutputFormat("%s\r\n", static_cast<const char *>(version));
    AppendResult(OT_ERROR_NONE);
}

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD

void Interpreter::ProcessCommissioner(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mCommissioner.Process(aArgsLength, aArgs);
    AppendResult(error);
}

#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE

void Interpreter::ProcessJoiner(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    error = mJoiner.Process(aArgsLength, aArgs);
    AppendResult(error);
}

#endif

#if OPENTHREAD_FTD
void Interpreter::ProcessJoinerPort(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    long    value;

    if (aArgsLength == 0)
    {
        OutputFormat("%d\r\n", otThreadGetJoinerUdpPort(mInstance));
    }
    else
    {
        SuccessOrExit(error = ParseLong(aArgs[0], value));
        error = otThreadSetJoinerUdpPort(mInstance, static_cast<uint16_t>(value));
    }

exit:
    AppendResult(error);
}
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
void Interpreter::ProcessMacFilter(uint8_t aArgsLength, char *aArgs[])
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

    AppendResult(error);
}

void Interpreter::PrintMacFilter(void)
{
    otMacFilterEntry       entry;
    otMacFilterIterator    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otMacFilterAddressMode mode     = otLinkFilterGetAddressMode(mInstance);

    if (mode == OT_MAC_FILTER_ADDRESS_MODE_DISABLED)
    {
        OutputFormat("Address Mode: Disabled\r\n");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
    {
        OutputFormat("Address Mode: Whitelist\r\n");
    }
    else if (mode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
    {
        OutputFormat("Address Mode: Blacklist\r\n");
    }

    while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

        if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
        {
            OutputFormat(" : rss %d (lqi %d)", entry.mRssIn, otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }

        OutputFormat("\r\n");
    }

    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    OutputFormat("RssIn List:\r\n");

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
            OutputFormat("Default rss : %d (lqi %d)\r\n", entry.mRssIn,
                         otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
        }
        else
        {
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
            OutputFormat(" : rss %d (lqi %d)\r\n", entry.mRssIn,
                         otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
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
            OutputFormat("Disabled\r\n");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
        {
            OutputFormat("Whitelist\r\n");
        }
        else if (mode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
        {
            OutputFormat("Blacklist\r\n");
        }

        while (otLinkFilterGetNextAddress(mInstance, &iterator, &entry) == OT_ERROR_NONE)
        {
            OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

            if (entry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
            {
                OutputFormat(" : rss %d (lqi %d)", entry.mRssIn,
                             otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }

            OutputFormat("\r\n");
        }
    }
    else
    {
        if (strcmp(aArgs[0], "disable") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_DISABLED);
        }
        else if (strcmp(aArgs[0], "whitelist") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_WHITELIST);
        }
        else if (strcmp(aArgs[0], "blacklist") == 0)
        {
            VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);
            otLinkFilterSetAddressMode(mInstance, OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST);
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
                OutputFormat("Default rss: %d (lqi %d)\r\n", entry.mRssIn,
                             otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
            }
            else
            {
                OutputBytes(entry.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
                OutputFormat(" : rss %d (lqi %d)\r\n", entry.mRssIn,
                             otLinkConvertRssToLinkQuality(mInstance, entry.mRssIn));
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

void Interpreter::ProcessMac(uint8_t aArgsLength, char *aArgs[])
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
    AppendResult(error);
}

otError Interpreter::ProcessMacRetries(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgsLength > 0 && aArgsLength <= 2, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "direct") == 0)
    {
        if (aArgsLength == 1)
        {
            OutputFormat("%d\r\n", otLinkGetMaxFrameRetriesDirect(mInstance));
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
            OutputFormat("%d\r\n", otLinkGetMaxFrameRetriesIndirect(mInstance));
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
void Interpreter::ProcessDiag(uint8_t aArgsLength, char *aArgs[])
{
    otError error;
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];

    // all diagnostics related features are processed within diagnostics module
    output[0]                  = '\0';
    output[sizeof(output) - 1] = '\0';

    error = otDiagProcessCmd(mInstance, aArgsLength, aArgs, output, sizeof(output) - 1);
    Output(output, static_cast<uint16_t>(strlen(output)));
    AppendResult(error);
}
#endif

void Interpreter::ProcessLine(char *aBuf, uint16_t aBufLength)
{
    char *  aArgs[kMaxArgs] = {nullptr};
    char *  cmd;
    uint8_t aArgsLength = 0;
    size_t  i           = 0;

    VerifyOrExit(aBuf != nullptr && StringLength(aBuf, aBufLength + 1) <= aBufLength, OT_NOOP);

    VerifyOrExit(Utils::CmdLineParser::ParseCmd(aBuf, aArgsLength, aArgs, kMaxArgs) == OT_ERROR_NONE,
                 OutputFormat("Error: too many args (max %d)\r\n", kMaxArgs));
    VerifyOrExit(aArgsLength >= 1, OutputFormat("Error: no given command.\r\n"));

    cmd = aArgs[0];

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    VerifyOrExit((!otDiagIsEnabled(mInstance) || (strcmp(cmd, "diag") == 0)),
                 OutputFormat("under diagnostics mode, execute 'diag stop' before running any other commands.\r\n"));
#endif

    for (i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(cmd, sCommands[i].mName) == 0)
        {
            (this->*sCommands[i].mCommand)(aArgsLength - 1, &aArgs[1]);
            break;
        }
    }

    // Check user defined commands if built-in command
    // has not been found
    if (i == OT_ARRAY_LENGTH(sCommands))
    {
        for (i = 0; i < mUserCommandsLength; i++)
        {
            if (strcmp(cmd, mUserCommands[i].mName) == 0)
            {
                mUserCommands[i].mCommand(aArgsLength - 1, &aArgs[1]);
                break;
            }
        }

        if (i == mUserCommandsLength)
        {
            AppendResult(OT_ERROR_INVALID_COMMAND);
        }
    }

exit:
    return;
}

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
void Interpreter::ProcessNetworkDiagnostic(uint8_t aArgsLength, char *aArgs[])
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
        ExitNow();
    }
    else if (strcmp(aArgs[0], "reset") == 0)
    {
        IgnoreError(otThreadSendDiagnosticReset(mInstance, &address, tlvTypes, count));
        AppendResult(OT_ERROR_NONE);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        AppendResult(error);
    }
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

    OutputFormat("\r\n");

    // Output Network Diagnostic TLV values in standard YAML format.
    while ((error = otThreadGetNextDiagnosticTlv(&aMessage, &iterator, &diagTlv)) == OT_ERROR_NONE)
    {
        uint16_t column = 0;
        switch (diagTlv.mType)
        {
        case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
            OutputFormat("Ext Address: '");
            OutputBytes(diagTlv.mData.mExtAddress.m8, sizeof(diagTlv.mData.mExtAddress.m8));
            OutputFormat("'\r\n");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
            OutputFormat("Rloc16: 0x%04x\r\n", diagTlv.mData.mAddr16);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
            OutputFormat("Mode:\r\n");
            OutputMode(diagTlv.mData.mMode, column + INDENT_SIZE);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
            OutputFormat("Timeout: %u\r\n", diagTlv.mData.mTimeout);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
            OutputFormat("Connectivity:\r\n");
            OutputConnectivity(diagTlv.mData.mConnectivity, column + INDENT_SIZE);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
            OutputFormat("Route:\r\n");
            OutputRoute(diagTlv.mData.mRoute, column + INDENT_SIZE);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
            OutputFormat("Leader Data:\r\n");
            OutputLeaderData(diagTlv.mData.mLeaderData, column + INDENT_SIZE);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
            OutputFormat("Network Data: '");
            OutputBytes(diagTlv.mData.mNetworkData.m8, diagTlv.mData.mNetworkData.mCount);
            OutputFormat("'\r\n");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
            OutputFormat("IP6 Address List:\r\n");
            for (uint16_t i = 0; i < diagTlv.mData.mIp6AddrList.mCount; ++i)
            {
                OutputSpaces(column + INDENT_SIZE);
                OutputFormat("- ");
                OutputIp6Address(diagTlv.mData.mIp6AddrList.mList[i]);
                OutputFormat("\r\n");
            }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
            OutputFormat("MAC Counters:\r\n");
            OutputNetworkDiagMacCounters(diagTlv.mData.mMacCounters, column + INDENT_SIZE);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
            OutputFormat("Battery Level: %u%%\r\n", diagTlv.mData.mBatteryLevel);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
            OutputFormat("Supply Voltage: %umV\r\n", diagTlv.mData.mSupplyVoltage);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
            OutputFormat("Child Table:\r\n");
            for (uint16_t i = 0; i < diagTlv.mData.mChildTable.mCount; ++i)
            {
                OutputSpaces(column + INDENT_SIZE);
                OutputFormat("- ");
                OutputChildTableEntry(diagTlv.mData.mChildTable.mTable[i], column + INDENT_SIZE + 2);
            }
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
            OutputFormat("Channel Pages: '");
            OutputBytes(diagTlv.mData.mChannelPages.m8, diagTlv.mData.mChannelPages.mCount);
            OutputFormat("'\r\n");
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
            OutputFormat("Max Child Timeout: %u\r\n", diagTlv.mData.mMaxChildTimeout);
            break;
        }
    }

    AppendResult(error == OT_ERROR_NOT_FOUND ? OT_ERROR_NONE : error);
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
    OutputFormat("RxOnWhenIdle: %d\r\n", aMode.mRxOnWhenIdle);

    OutputSpaces(aColumn);
    OutputFormat("SecureDataRequests: %d\r\n", aMode.mSecureDataRequests);

    OutputSpaces(aColumn);
    OutputFormat("DeviceType: %d\r\n", aMode.mDeviceType);

    OutputSpaces(aColumn);
    OutputFormat("NetworkData: %d\r\n", aMode.mNetworkData);
}

void Interpreter::OutputConnectivity(const otNetworkDiagConnectivity &aConnectivity, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputFormat("ParentPriority: %d\r\n", aConnectivity.mParentPriority);

    OutputSpaces(aColumn);
    OutputFormat("LinkQuality3: %u\r\n", aConnectivity.mLinkQuality3);

    OutputSpaces(aColumn);
    OutputFormat("LinkQuality2: %u\r\n", aConnectivity.mLinkQuality2);

    OutputSpaces(aColumn);
    OutputFormat("LinkQuality1: %u\r\n", aConnectivity.mLinkQuality1);

    OutputSpaces(aColumn);
    OutputFormat("LeaderCost: %u\r\n", aConnectivity.mLeaderCost);

    OutputSpaces(aColumn);
    OutputFormat("IdSequence: %u\r\n", aConnectivity.mIdSequence);

    OutputSpaces(aColumn);
    OutputFormat("ActiveRouters: %u\r\n", aConnectivity.mActiveRouters);

    OutputSpaces(aColumn);
    OutputFormat("SedBufferSize: %u\r\n", aConnectivity.mSedBufferSize);

    OutputSpaces(aColumn);
    OutputFormat("SedDatagramCount: %u\r\n", aConnectivity.mSedDatagramCount);
}

void Interpreter::OutputRoute(const otNetworkDiagRoute &aRoute, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputFormat("IdSequence: %u\r\n", aRoute.mIdSequence);

    OutputSpaces(aColumn);
    OutputFormat("RouteData:\r\n");

    aColumn += INDENT_SIZE;
    for (uint16_t i = 0; i < aRoute.mRouteCount; ++i)
    {
        OutputSpaces(aColumn);
        OutputFormat("- ");

        OutputRouteData(aRoute.mRouteData[i], aColumn + 2);
    }
}

void Interpreter::OutputRouteData(const otNetworkDiagRouteData &aRouteData, uint16_t aColumn)
{
    OutputFormat("RouteId: 0x%02x\r\n", aRouteData.mRouterId);

    OutputSpaces(aColumn);
    OutputFormat("LinkQualityOut: %u\r\n", aRouteData.mLinkQualityOut);

    OutputSpaces(aColumn);
    OutputFormat("LinkQualityIn: %u\r\n", aRouteData.mLinkQualityIn);

    OutputSpaces(aColumn);
    OutputFormat("RouteCost: %u\r\n", aRouteData.mRouteCost);
}

void Interpreter::OutputLeaderData(const otLeaderData &aLeaderData, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputFormat("PartitionId: 0x%08x\r\n", aLeaderData.mPartitionId);

    OutputSpaces(aColumn);
    OutputFormat("Weighting: %u\r\n", aLeaderData.mWeighting);

    OutputSpaces(aColumn);
    OutputFormat("DataVersion: %u\r\n", aLeaderData.mDataVersion);

    OutputSpaces(aColumn);
    OutputFormat("StableDataVersion: %u\r\n", aLeaderData.mStableDataVersion);

    OutputSpaces(aColumn);
    OutputFormat("LeaderRouterId: 0x%02x\r\n", aLeaderData.mLeaderRouterId);
}

void Interpreter::OutputNetworkDiagMacCounters(const otNetworkDiagMacCounters &aMacCounters, uint16_t aColumn)
{
    OutputSpaces(aColumn);
    OutputFormat("IfInUnknownProtos: %u\r\n", aMacCounters.mIfInUnknownProtos);

    OutputSpaces(aColumn);
    OutputFormat("IfInErrors: %u\r\n", aMacCounters.mIfInErrors);

    OutputSpaces(aColumn);
    OutputFormat("IfOutErrors: %u\r\n", aMacCounters.mIfOutErrors);

    OutputSpaces(aColumn);
    OutputFormat("IfInUcastPkts: %u\r\n", aMacCounters.mIfInUcastPkts);

    OutputSpaces(aColumn);
    OutputFormat("IfInBroadcastPkts: %u\r\n", aMacCounters.mIfInBroadcastPkts);

    OutputSpaces(aColumn);
    OutputFormat("IfInDiscards: %u\r\n", aMacCounters.mIfInDiscards);

    OutputSpaces(aColumn);
    OutputFormat("IfOutUcastPkts: %u\r\n", aMacCounters.mIfOutUcastPkts);

    OutputSpaces(aColumn);
    OutputFormat("IfOutBroadcastPkts: %u\r\n", aMacCounters.mIfOutBroadcastPkts);

    OutputSpaces(aColumn);
    OutputFormat("IfOutDiscards: %u\r\n", aMacCounters.mIfOutDiscards);
}

void Interpreter::OutputChildTableEntry(const otNetworkDiagChildEntry &aChildEntry, uint16_t aColumn)
{
    OutputFormat("ChildId: 0x%04x\r\n", aChildEntry.mChildId);

    OutputSpaces(aColumn);
    OutputFormat("Timeout: %u\r\n", aChildEntry.mTimeout);

    OutputSpaces(aColumn);
    OutputFormat("Mode:\r\n");

    OutputMode(aChildEntry.mMode, aColumn + INDENT_SIZE);
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
    OutputFormat(": version=%u,joiner=%d\r\n", aInfo.mVersion, aInfo.mIsJoiner);
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
    Interpreter::GetInterpreter().AppendResult(aError);
}

extern "C" void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    Interpreter::GetInterpreter().OutputFormatV(aFormat, aArgs);
    Interpreter::GetInterpreter().OutputFormat("\r\n");
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
#endif // OPENTHREAD_CONFIG_LEGACY_ENABLE
