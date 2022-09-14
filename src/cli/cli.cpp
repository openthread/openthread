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
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
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
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
#include <openthread/trel.h>
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
#include <openthread/nat64.h>
#endif

#include "common/new.hpp"
#include "common/string.hpp"
#include "mac/channel_mask.hpp"

namespace ot {
namespace Cli {

Interpreter *Interpreter::sInterpreter = nullptr;
static OT_DEFINE_ALIGNED_VAR(sInterpreterRaw, sizeof(Interpreter), uint64_t);

Interpreter::Interpreter(Instance *aInstance, otCliOutputCallback aCallback, void *aContext)
    : Output(aInstance, aCallback, aContext)
    , mUserCommands(nullptr)
    , mUserCommandsLength(0)
    , mCommandIsPending(false)
    , mTimer(*aInstance, HandleTimer, this)
#if OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
    , mSntpQueryingInProgress(false)
#endif
    , mDataset(*this)
    , mNetworkData(*this)
    , mUdp(*this)
#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
    , mTcp(*this)
#endif
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
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    , mHistory(*this)
#endif
#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    , mLocateInProgress(false)
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    , mLinkMetricsQueryInProgress(false)
#endif
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
{
#if OPENTHREAD_FTD
    otThreadSetDiscoveryRequestCallback(GetInstancePtr(), &Interpreter::HandleDiscoveryRequest, this);
#endif

    OutputPrompt();
}

void Interpreter::OutputResult(otError aError)
{
    OT_ASSERT(mCommandIsPending);

    VerifyOrExit(aError != OT_ERROR_PENDING);

    if (aError == OT_ERROR_NONE)
    {
        OutputLine("Done");
    }
    else
    {
        OutputLine("Error %d: %s", aError, otThreadErrorToString(aError));
    }

    mCommandIsPending = false;
    mTimer.Stop();
    OutputPrompt();

exit:
    return;
}

const char *Interpreter::LinkModeToString(const otLinkModeConfig &aLinkMode, char (&aStringBuffer)[kLinkModeStringSize])
{
    char *flagsPtr = &aStringBuffer[0];

    if (aLinkMode.mRxOnWhenIdle)
    {
        *flagsPtr++ = 'r';
    }

    if (aLinkMode.mDeviceType)
    {
        *flagsPtr++ = 'd';
    }

    if (aLinkMode.mNetworkData)
    {
        *flagsPtr++ = 'n';
    }

    if (flagsPtr == &aStringBuffer[0])
    {
        *flagsPtr++ = '-';
    }

    *flagsPtr = '\0';

    return aStringBuffer;
}

#if OPENTHREAD_CONFIG_DIAG_ENABLE
template <> otError Interpreter::Process<Cmd("diag")>(Arg aArgs[])
{
    otError error;
    char *  args[kMaxArgs];
    char    output[OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE];

    // all diagnostics related features are processed within diagnostics module
    Arg::CopyArgsToStringArray(aArgs, args);

    error = otDiagProcessCmd(GetInstancePtr(), Arg::GetArgsLength(aArgs), args, output, sizeof(output));

    OutputFormat("%s", output);

    return error;
}
#endif

template <> otError Interpreter::Process<Cmd("version")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otGetVersionString());
    }
    else if (aArgs[0] == "api")
    {
        OutputLine("%d", OPENTHREAD_API_VERSION);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

template <> otError Interpreter::Process<Cmd("reset")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceReset(GetInstancePtr());

    return OT_ERROR_NONE;
}

void Interpreter::ProcessLine(char *aBuf)
{
    Arg     args[kMaxArgs + 1];
    otError error = OT_ERROR_NONE;

    OT_ASSERT(aBuf != nullptr);

    // Ignore the command if another command is pending.
    VerifyOrExit(!mCommandIsPending, args[0].Clear());
    mCommandIsPending = true;

    VerifyOrExit(StringLength(aBuf, kMaxLineLength) <= kMaxLineLength - 1, error = OT_ERROR_PARSE);

    SuccessOrExit(error = Utils::CmdLineParser::ParseCmd(aBuf, args, kMaxArgs));
    VerifyOrExit(!args[0].IsEmpty(), mCommandIsPending = false);

    LogInput(args);

#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otDiagIsEnabled(GetInstancePtr()) && (args[0] != "diag") && (args[0] != "factoryreset"))
    {
        OutputLine("under diagnostics mode, execute 'diag stop' before running any other commands.");
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }
#endif

    error = ProcessCommand(args);

exit:
    if ((error != OT_ERROR_NONE) || !args[0].IsEmpty())
    {
        OutputResult(error);
    }
    else if (!mCommandIsPending)
    {
        OutputPrompt();
    }
}

otError Interpreter::ProcessUserCommands(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    for (uint8_t i = 0; i < mUserCommandsLength; i++)
    {
        if (aArgs[0] == mUserCommands[i].mName)
        {
            char *args[kMaxArgs];

            Arg::CopyArgsToStringArray(aArgs, args);
            mUserCommands[i].mCommand(mUserCommandsContext, Arg::GetArgsLength(aArgs) - 1, args + 1);
            error = OT_ERROR_NONE;
            break;
        }
    }

    return error;
}

void Interpreter::SetUserCommands(const otCliCommand *aCommands, uint8_t aLength, void *aContext)
{
    mUserCommands        = aCommands;
    mUserCommandsLength  = aLength;
    mUserCommandsContext = aContext;
}

#if OPENTHREAD_FTD || OPENTHREAD_MTD
otError Interpreter::ParseEnableOrDisable(const Arg &aArg, bool &aEnable)
{
    otError error = OT_ERROR_NONE;

    if (aArg == "enable")
    {
        aEnable = true;
    }
    else if (aArg == "disable")
    {
        aEnable = false;
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

otError Interpreter::ParseJoinerDiscerner(Arg &aArg, otJoinerDiscerner &aDiscerner)
{
    otError error;
    char *  separator;

    VerifyOrExit(!aArg.IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    separator = strstr(aArg.GetCString(), "/");

    VerifyOrExit(separator != nullptr, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint8(separator + 1, aDiscerner.mLength));
    VerifyOrExit(aDiscerner.mLength > 0 && aDiscerner.mLength <= 64, error = OT_ERROR_INVALID_ARGS);
    *separator = '\0';
    error      = aArg.ParseAsUint64(aDiscerner.mValue);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE

otError Interpreter::ParsePingInterval(const Arg &aArg, uint32_t &aInterval)
{
    otError        error    = OT_ERROR_NONE;
    const char *   string   = aArg.GetCString();
    const uint32_t msFactor = 1000;
    uint32_t       factor   = msFactor;

    aInterval = 0;

    while (*string)
    {
        if ('0' <= *string && *string <= '9')
        {
            // In the case of seconds, change the base of already calculated value.
            if (factor == msFactor)
            {
                aInterval *= 10;
            }

            aInterval += static_cast<uint32_t>(*string - '0') * factor;

            // In the case of milliseconds, change the multiplier factor.
            if (factor != msFactor)
            {
                factor /= 10;
            }
        }
        else if (*string == '.')
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

        string++;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_PING_SENDER_ENABLE

otError Interpreter::ParsePreference(const Arg &aArg, otRoutePreference &aPreference)
{
    otError error = OT_ERROR_NONE;

    if (aArg == "high")
    {
        aPreference = OT_ROUTE_PREFERENCE_HIGH;
    }
    else if (aArg == "med")
    {
        aPreference = OT_ROUTE_PREFERENCE_MED;
    }
    else if (aArg == "low")
    {
        aPreference = OT_ROUTE_PREFERENCE_LOW;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

const char *Interpreter::PreferenceToString(signed int aPreference)
{
    const char *str = "";

    switch (aPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        str = "low";
        break;

    case OT_ROUTE_PREFERENCE_MED:
        str = "med";
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        str = "high";
        break;

    default:
        break;
    }

    return str;
}

otError Interpreter::ParseAndConvertToIp6Address(otInstance *  aInstance,
                                                 const Arg &   aArg,
                                                 otIp6Address &aAddress,
                                                 bool &        aConverted)
{
    Error error = kErrorNone;

    VerifyOrExit(!aArg.IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error      = aArg.ParseAsIp6Address(aAddress);
    aConverted = false;
    if (error != kErrorNone)
    {
        // It might be an IPv4 address, let's have a try.
        otIp4Address ip4Address;

        // Do not touch the error value if we failed to parse it as an IPv4 address.
        SuccessOrExit(aArg.ParseAsIp4Address(ip4Address));
        SuccessOrExit(error = otNat64SynthersizeIp6Address(aInstance, &ip4Address, &aAddress));
        aConverted = true;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
template <> otError Interpreter::Process<Cmd("history")>(Arg aArgs[])
{
    return mHistory.Process(aArgs);
}
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
template <> otError Interpreter::Process<Cmd("ba")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli ba port
     * @code
     * ba port
     * 49153
     * Done
     * @endcode
     * @par api_copy
     * #otBorderAgentGetUdpPort
     */
    if (aArgs[0] == "port")
    {
        OutputLine("%hu", otBorderAgentGetUdpPort(GetInstancePtr()));
    }
    /**
     * @cli ba state
     * @code
     * ba state
     * Started
     * Done
     * @endcode
     * @par api_copy
     * #otBorderAgentGetState
     */
    else if (aArgs[0] == "state")
    {
        static const char *const kStateStrings[] = {
            "Stopped"  // (0) OT_BORDER_AGENT_STATE_STOPPED
            "Started", // (1) OT_BORDER_AGENT_STATE_STARTED
            "Active",  // (2) OT_BORDER_AGENT_STATE_ACTIVE
        };

        static_assert(0 == OT_BORDER_AGENT_STATE_STOPPED, "OT_BORDER_AGENT_STATE_STOPPED value is incorrect");
        static_assert(1 == OT_BORDER_AGENT_STATE_STARTED, "OT_BORDER_AGENT_STATE_STARTED value is incorrect");
        static_assert(2 == OT_BORDER_AGENT_STATE_ACTIVE, "OT_BORDER_AGENT_STATE_ACTIVE value is incorrect");

        OutputLine("%s", Stringify(otBorderAgentGetState(GetInstancePtr()), kStateStrings));
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
template <> otError Interpreter::Process<Cmd("br")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    /**
     * @cli br (enable,disable)
     * @code
     * br enable
     * Done
     * @endcode
     * @code
     * br disable
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingSetEnabled
     */
    if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = otBorderRoutingSetEnabled(GetInstancePtr(), enable));
    }
    /**
     * @cli br omrprefix
     * @code
     * br omrprefix
     * fdfc:1ff5:1512:5622::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetOmrPrefix
     */
    else if (aArgs[0] == "omrprefix")
    {
        otIp6Prefix omrPrefix;

        SuccessOrExit(error = otBorderRoutingGetOmrPrefix(GetInstancePtr(), &omrPrefix));
        OutputIp6PrefixLine(omrPrefix);
    }
    /**
     * @cli br favoredomrprefix
     * @code
     * br favoredomrprefix
     * fdfc:1ff5:1512:5622::/64 prf:low
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetFavoredOmrPrefix
     */
    else if (aArgs[0] == "favoredomrprefix")
    {
        otIp6Prefix       prefix;
        otRoutePreference preference;

        SuccessOrExit(error = otBorderRoutingGetFavoredOmrPrefix(GetInstancePtr(), &prefix, &preference));
        OutputIp6Prefix(prefix);
        OutputLine(" prf:%s", PreferenceToString(preference));
    }
    /**
     * @cli br onlinkprefix
     * @code
     * br onlinkprefix
     * fd41:2650:a6f5:0::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetOnLinkPrefix
     */
    else if (aArgs[0] == "onlinkprefix")
    {
        otIp6Prefix onLinkPrefix;

        SuccessOrExit(error = otBorderRoutingGetOnLinkPrefix(GetInstancePtr(), &onLinkPrefix));
        OutputIp6PrefixLine(onLinkPrefix);
    }
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    /**
     * @cli br nat64prefix
     * @code
     * br nat64prefix
     * fd14:1078:b3d5:b0b0:0:0::/96
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetNat64Prefix
     */
    else if (aArgs[0] == "nat64prefix")
    {
        otIp6Prefix nat64Prefix;

        SuccessOrExit(error = otBorderRoutingGetNat64Prefix(GetInstancePtr(), &nat64Prefix));
        OutputIp6PrefixLine(nat64Prefix);
    }
    /**
     * @cli br favorednat64prefix
     * @code
     * br favorednat64prefix
     * fd14:1078:b3d5:b0b0:0:0::/96 prf:low
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetFavoredNat64Prefix
     */
    else if (aArgs[0] == "favorednat64prefix")
    {
        otIp6Prefix       prefix;
        otRoutePreference preference;

        SuccessOrExit(error = otBorderRoutingGetFavoredNat64Prefix(GetInstancePtr(), &prefix, &preference));
        OutputIp6Prefix(prefix);
        OutputLine(" prf:%s", PreferenceToString(preference));
    }
#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    /**
     * @cli br rioprf (high,med,low)
     * @code
     * br rioprf
     * med
     * Done
     * @endcode
     * @code
     * br rioprf low
     * Done
     * @endcode
     * @cparam br rioprf [@ca{high}|@ca{med}|@ca{low}]
     * @par api_copy
     * #otBorderRoutingSetRouteInfoOptionPreference
     *
     */
    else if (aArgs[0] == "rioprf")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputLine("%s", PreferenceToString(otBorderRoutingGetRouteInfoOptionPreference(GetInstancePtr())));
        }
        else
        {
            otRoutePreference preference;

            SuccessOrExit(error = ParsePreference(aArgs[1], preference));
            otBorderRoutingSetRouteInfoOptionPreference(GetInstancePtr(), preference);
        }
    }
    /**
     * @cli br prefixtable
     * @code
     * br prefixtable
     * prefix:fd00:1234:5678:0::/64, on-link:no, ms-since-rx:29526, lifetime:1800, route-prf:med,
     * router:ff02:0:0:0:0:0:0:1
     * prefix:1200:abba:baba:0::/64, on-link:yes, ms-since-rx:29527, lifetime:1800, preferred:1800,
     * router:ff02:0:0:0:0:0:0:1
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetNextPrefixTableEntry
     *
     */
    else if (aArgs[0] == "prefixtable")
    {
        otBorderRoutingPrefixTableIterator iterator;
        otBorderRoutingPrefixTableEntry    entry;

        otBorderRoutingPrefixTableInitIterator(GetInstancePtr(), &iterator);

        while (otBorderRoutingGetNextPrefixTableEntry(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
        {
            char string[OT_IP6_PREFIX_STRING_SIZE];

            otIp6PrefixToString(&entry.mPrefix, string, sizeof(string));
            OutputFormat("prefix:%s, on-link:%s, ms-since-rx:%u, lifetime:%u, ", string, entry.mIsOnLink ? "yes" : "no",
                         entry.mMsecSinceLastUpdate, entry.mValidLifetime);

            if (entry.mIsOnLink)
            {
                OutputFormat("preferred:%u, ", entry.mPreferredLifetime);
            }
            else
            {
                OutputFormat("route-prf:%s, ", PreferenceToString(entry.mRoutePreference));
            }

            otIp6AddressToString(&entry.mRouterAddress, string, sizeof(string));
            OutputLine("router:%s", string);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
template <> otError Interpreter::Process<Cmd("nat64")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }
    /**
     * @cli nat64 cidr
     * @code
     * nat64 cidr
     * 192.168.64.0/24
     * Done
     * @endcode
     * @par api_copy
     * #otNat64GetCidr
     *
     */
    else if (aArgs[0] == "cidr")
    {
        otIp4Cidr cidr;
        char      cidrString[OT_IP4_CIDR_STRING_SIZE];

        SuccessOrExit(error = otNat64GetCidr(GetInstancePtr(), &cidr));
        otIp4CidrToString(&cidr, cidrString, sizeof(cidrString));
        OutputLine("%s", cidrString);
    }
    /**
     * @cli nat64 mappings
     * @code
     * nat64 mappings
     * |          | Address                   |        | 4 to 6       | 6 to 4       |
     * +----------+---------------------------+--------+--------------+--------------+
     * | ID       | IPv6       | IPv4         | Expiry | Pkts | Bytes | Pkts | Bytes |
     * +----------+------------+--------------+--------+------+-------+------+-------+
     * | 00021cb9 | fdc7::df79 | 192.168.64.2 |  7196s |    6 |   456 |   11 |  1928 |
     * |          |                                TCP |    0 |     0 |    0 |     0 |
     * |          |                                UDP |    1 |   136 |   16 |  1608 |
     * |          |                               ICMP |    5 |   320 |    5 |   320 |
     * @endcode
     * @par api_copy
     * #otNat64GetNextAddressMapping
     *
     */
    else if (aArgs[0] == "mappings")
    {
        otNat64AddressMappingIterator iterator;
        otNat64AddressMapping         mapping;

        static const char *const kNat64StatusLevel1Title[] = {"", "Address", "", "4 to 6", "6 to 4"};

        static const uint8_t kNat64StatusLevel1ColumnWidths[] = {
            18, 61, 8, 25, 25,
        };

        static const char *const kNat64StatusTableHeader[] = {
            "ID", "IPv6", "IPv4", "Expiry", "Pkts", "Bytes", "Pkts", "Bytes",
        };

        static const uint8_t kNat64StatusTableColumnWidths[] = {
            18, 42, 18, 8, 10, 14, 10, 14,
        };

        OutputTableHeader(kNat64StatusLevel1Title, kNat64StatusLevel1ColumnWidths);
        OutputTableHeader(kNat64StatusTableHeader, kNat64StatusTableColumnWidths);

        otNat64InitAddressMappingIterator(GetInstancePtr(), &iterator);
        while (otNat64GetNextAddressMapping(GetInstancePtr(), &iterator, &mapping) == OT_ERROR_NONE)
        {
            char ip4AddressString[OT_IP4_ADDRESS_STRING_SIZE];
            char ip6AddressString[OT_IP6_PREFIX_STRING_SIZE];

            otIp6AddressToString(&mapping.mIp6, ip6AddressString, sizeof(ip6AddressString));
            otIp4AddressToString(&mapping.mIp4, ip4AddressString, sizeof(ip4AddressString));

            OutputFormat("| %016llx ", mapping.mId);
            OutputFormat("| %40s ", ip6AddressString);
            OutputFormat("| %16s ", ip4AddressString);
            OutputFormat("| %5llus ", mapping.mRemainingTimeMs / 1000);
            OutputFormat("| %8llu ", mapping.mCounters.mTotal.m4To6Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mTotal.m4To6Bytes);
            OutputFormat("| %8llu ", mapping.mCounters.mTotal.m6To4Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mTotal.m6To4Bytes);

            OutputLine("|");

            OutputFormat("| %016s ", "");
            OutputFormat("| %68s ", "TCP");
            OutputFormat("| %8llu ", mapping.mCounters.mTcp.m4To6Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mTcp.m4To6Bytes);
            OutputFormat("| %8llu ", mapping.mCounters.mTcp.m6To4Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mTcp.m6To4Bytes);
            OutputLine("|");

            OutputFormat("| %016s ", "");
            OutputFormat("| %68s ", "UDP");
            OutputFormat("| %8llu ", mapping.mCounters.mUdp.m4To6Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mUdp.m4To6Bytes);
            OutputFormat("| %8llu ", mapping.mCounters.mUdp.m6To4Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mUdp.m6To4Bytes);
            OutputLine("|");

            OutputFormat("| %016s ", "");
            OutputFormat("| %68s ", "ICMP");
            OutputFormat("| %8llu ", mapping.mCounters.mIcmp.m4To6Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mIcmp.m4To6Bytes);
            OutputFormat("| %8llu ", mapping.mCounters.mIcmp.m6To4Packets);
            OutputFormat("| %12llu ", mapping.mCounters.mIcmp.m6To4Bytes);
            OutputLine("|");
        }
    }
    /**
     * @cli nat64 counters
     * @code
     * nat64 counters
     * |               | 4 to 6                  | 6 to 4                  |
     * +---------------+-------------------------+-------------------------+
     * | Protocol      | Pkts     | Bytes        | Pkts     | Bytes        |
     * +---------------+----------+--------------+----------+--------------+
     * |         Total |       11 |          704 |       11 |          704 |
     * |           TCP |        0 |            0 |        0 |            0 |
     * |           UDP |        0 |            0 |        0 |            0 |
     * |          ICMP |       11 |          704 |       11 |          704 |
     * | Errors        | Pkts                    | Pkts                    |
     * +---------------+-------------------------+-------------------------+
     * |         Total |                       8 |                       4 |
     * |   Illegal Pkt |                       0 |                       0 |
     * |   Unsup Proto |                       0 |                       0 |
     * |    No Mapping |                       2 |                       0 |
     * Done
     * @endcode
     * @par
     * Gets the NAT64 translator packet and error counters.
     * @par
     * Available when `OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE` is enabled.
     * @sa otNat64GetCounters
     * @sa otNat64GetErrorCounters
     *
     */
    else if (aArgs[0] == "counters")
    {
        static const char *const kNat64CounterTableHeader[] = {
            "",
            "4 to 6",
            "6 to 4",
        };
        static const uint8_t     kNat64CounterTableHeaderColumns[] = {15, 25, 25};
        static const char *const kNat64CounterTableSubHeader[]     = {
            "Protocol", "Pkts", "Bytes", "Pkts", "Bytes",
        };
        static const uint8_t kNat64CounterTableSubHeaderColumns[] = {
            15, 10, 14, 10, 14,
        };
        static const char *const kNat64CounterTableErrorSubHeader[] = {
            "Errors",
            "Pkts",
            "Pkts",
        };
        static const uint8_t kNat64CounterTableErrorSubHeaderColumns[] = {
            15,
            25,
            25,
        };
        static const char *const kNat64CounterErrorType[] = {
            "Unknown",
            "Illegal Pkt",
            "Unsup Proto",
            "No Mapping",
        };

        otNat64ProtocolCounters counters;
        otNat64ErrorCounters    errorCounters;

        OutputTableHeader(kNat64CounterTableHeader, kNat64CounterTableHeaderColumns);
        OutputTableHeader(kNat64CounterTableSubHeader, kNat64CounterTableSubHeaderColumns);

        otNat64GetCounters(GetInstancePtr(), &counters);
        otNat64GetErrorCounters(GetInstancePtr(), &errorCounters);

        OutputLine("| %13s | %8llu | %12llu | %8llu | %12llu |", "Total", counters.mTotal.m4To6Packets,
                   counters.mTotal.m4To6Bytes, counters.mTotal.m6To4Packets, counters.mTotal.m6To4Bytes);
        OutputLine("| %13s | %8llu | %12llu | %8llu | %12llu |", "TCP", counters.mTcp.m4To6Packets,
                   counters.mTcp.m4To6Bytes, counters.mTcp.m6To4Packets, counters.mTcp.m6To4Bytes);
        OutputLine("| %13s | %8llu | %12llu | %8llu | %12llu |", "UDP", counters.mUdp.m4To6Packets,
                   counters.mUdp.m4To6Bytes, counters.mUdp.m6To4Packets, counters.mUdp.m6To4Bytes);
        OutputLine("| %13s | %8llu | %12llu | %8llu | %12llu |", "ICMP", counters.mIcmp.m4To6Packets,
                   counters.mIcmp.m4To6Bytes, counters.mIcmp.m6To4Packets, counters.mIcmp.m6To4Bytes);

        OutputTableHeader(kNat64CounterTableErrorSubHeader, kNat64CounterTableErrorSubHeaderColumns);
        for (uint8_t i = 0; i < OT_NAT64_DROP_REASON_COUNT; i++)
        {
            OutputLine("| %13s | %23llu | %23llu |", kNat64CounterErrorType[i], errorCounters.mCount4To6[i],
                       errorCounters.mCount6To4[i]);
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
template <> otError Interpreter::Process<Cmd("bbr")>(Arg aArgs[])
{
    otError                error = OT_ERROR_INVALID_COMMAND;
    otBackboneRouterConfig config;

    /**
     * @cli bbr
     * @code
     * bbr
     * BBR Primary:
     * server16: 0xE400
     * seqno:    10
     * delay:    120 secs
     * timeout:  300 secs
     * Done
     * @endcode
     * @code
     * bbr
     * BBR Primary: None
     * Done
     * @endcode
     * @par
     * Returns the current Primary Backbone Router information for the Thread device.
     */
    if (aArgs[0].IsEmpty())
    {
        if (otBackboneRouterGetPrimary(GetInstancePtr(), &config) == OT_ERROR_NONE)
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
        if (aArgs[0] == "mgmt")
        {
            if (aArgs[1].IsEmpty())
            {
                ExitNow(error = OT_ERROR_INVALID_COMMAND);
            }
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
            /**
             * @cli bbr mgmt dua
             * @code
             * bbr mgmt dua 1 2f7c235e5025a2fd
             * Done
             * @endcode
             * @code
             * bbr mgmt dua 160
             * Done
             * @endcode
             * @cparam bbr mgmt dua @ca{status|coap-code} [@ca{meshLocalIid}]
             * For `status` or `coap-code`, use:
             * *    0: ST_DUA_SUCCESS
             * *    1: ST_DUA_REREGISTER
             * *    2: ST_DUA_INVALID
             * *    3: ST_DUA_DUPLICATE
             * *    4: ST_DUA_NO_RESOURCES
             * *    5: ST_DUA_BBR_NOT_PRIMARY
             * *    6: ST_DUA_GENERAL_FAILURE
             * *    160: COAP code 5.00
             * @par
             * With the `meshLocalIid` included, this command configures the response status
             * for the next DUA registration. Without `meshLocalIid`, respond to the next
             * DUA.req with the specified `status` or `coap-code`.
             * @par
             * Available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
             * @sa otBackboneRouterConfigNextDuaRegistrationResponse
             */
            else if (aArgs[1] == "dua")
            {
                uint8_t                   status;
                otIp6InterfaceIdentifier *mlIid = nullptr;
                otIp6InterfaceIdentifier  iid;

                SuccessOrExit(error = aArgs[2].ParseAsUint8(status));

                if (!aArgs[3].IsEmpty())
                {
                    SuccessOrExit(error = aArgs[3].ParseAsHexString(iid.mFields.m8));
                    mlIid = &iid;
                    VerifyOrExit(aArgs[4].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
                }

                otBackboneRouterConfigNextDuaRegistrationResponse(GetInstancePtr(), mlIid, status);
                ExitNow();
            }
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
            else if (aArgs[1] == "mlr")
            {
                error = ProcessBackboneRouterMgmtMlr(aArgs + 2);
                ExitNow();
            }
#endif
        }
        SuccessOrExit(error = ProcessBackboneRouterLocal(aArgs));
    }

exit:
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
otError Interpreter::ProcessBackboneRouterMgmtMlr(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    /**
     * @cli bbr mgmt mlr listener
     * @code
     * bbr mgmt mlr listener
     * ff04:0:0:0:0:0:0:abcd 3534000
     * ff04:0:0:0:0:0:0:eeee 3537610
     * Done
     * @endcode
     * @par
     * Returns the Multicast Listeners with the #otBackboneRouterMulticastListenerInfo
     * `mTimeout` in seconds.
     * @par
     * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` and
     * `OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE` are enabled.
     * @sa otBackboneRouterMulticastListenerGetNext
     */
    if (aArgs[0] == "listener")
    {
        if (aArgs[1].IsEmpty())
        {
            PrintMulticastListenersTable();
            ExitNow(error = OT_ERROR_NONE);
        }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        /**
         * @cli bbr mgmt mlr listener clear
         * @code
         * bbr mgmt mlr listener clear
         * Done
         * @endcode
         * @par api_copy
         * #otBackboneRouterMulticastListenerClear
         */
        if (aArgs[1] == "clear")
        {
            otBackboneRouterMulticastListenerClear(GetInstancePtr());
            error = OT_ERROR_NONE;
        }
        /**
         * @cli bbr mgmt mlr listener add
         * @code
         * bbr mgmt mlr listener add ff04::1
         * Done
         * @endcode
         * @code
         * bbr mgmt mlr listener add ff04::2 300
         * Done
         * @endcode
         * @cparam bbr mgmt mlr listener add @ca{ipaddress} [@ca{timeout-seconds}]
         * @par api_copy
         * #otBackboneRouterMulticastListenerAdd
         */
        else if (aArgs[1] == "add")
        {
            otIp6Address address;
            uint32_t     timeout = 0;

            SuccessOrExit(error = aArgs[2].ParseAsIp6Address(address));

            if (!aArgs[3].IsEmpty())
            {
                SuccessOrExit(error = aArgs[3].ParseAsUint32(timeout));
                VerifyOrExit(aArgs[4].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            }

            error = otBackboneRouterMulticastListenerAdd(GetInstancePtr(), &address, timeout);
        }
    }
    /**
     * @cli bbr mgmt mlr response
     * @code
     * bbr mgmt mlr response 2
     * Done
     * @endcode
     * @cparam bbr mgmt mlr response @ca{status-code}
     * For `status-code`, use:
     * *    0: ST_MLR_SUCCESS
     * *    2: ST_MLR_INVALID
     * *    3: ST_MLR_NO_PERSISTENT
     * *    4: ST_MLR_NO_RESOURCES
     * *    5: ST_MLR_BBR_NOT_PRIMARY
     * *    6: ST_MLR_GENERAL_FAILURE
     * @par api_copy
     * #otBackboneRouterConfigNextMulticastListenerRegistrationResponse
     */
    else if (aArgs[0] == "response")
    {
        error = ProcessSet(aArgs + 1, otBackboneRouterConfigNextMulticastListenerRegistrationResponse);
#endif
    }

exit:
    return error;
}

void Interpreter::PrintMulticastListenersTable(void)
{
    otBackboneRouterMulticastListenerIterator iter = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ITERATOR_INIT;
    otBackboneRouterMulticastListenerInfo     listenerInfo;

    while (otBackboneRouterMulticastListenerGetNext(GetInstancePtr(), &iter, &listenerInfo) == OT_ERROR_NONE)
    {
        OutputIp6Address(listenerInfo.mAddress);
        OutputLine(" %u", listenerInfo.mTimeout);
    }
}

#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

otError Interpreter::ProcessBackboneRouterLocal(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otBackboneRouterConfig config;
    bool                   enable;

    /**
     * @cli bbr (enable,disable)
     * @code
     * bbr enable
     * Done
     * @endcode
     * @code
     * bbr disable
     * Done
     * @endcode
     * @par api_copy
     * #otBackboneRouterSetEnabled
     */
    if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        otBackboneRouterSetEnabled(GetInstancePtr(), enable);
    }
    /**
     * @cli bbr jitter (get,set)
     * @code
     * bbr jitter
     * 20
     * Done
     * @endcode
     * @code
     * bbr jitter 10
     * Done
     * @endcode
     * @cparam bbr jitter [@ca{jitter}]
     * @par
     * Gets or sets jitter (in seconds) for Backbone Router registration.
     * @par
     * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is enabled.
     * @sa otBackboneRouterGetRegistrationJitter
     * @sa otBackboneRouterSetRegistrationJitter
     */
    else if (aArgs[0] == "jitter")
    {
        error = ProcessGetSet(aArgs + 1, otBackboneRouterGetRegistrationJitter, otBackboneRouterSetRegistrationJitter);
    }
    /**
     * @cli bbr register
     * @code
     * bbr register
     * Done
     * @endcode
     * @par api_copy
     * #otBackboneRouterRegister
     */
    else if (aArgs[0] == "register")
    {
        SuccessOrExit(error = otBackboneRouterRegister(GetInstancePtr()));
    }
    /**
     * @cli bbr state
     * @code
     * bbr state
     * Disabled
     * Done
     * @endcode
     * @code
     * bbr state
     * Primary
     * Done
     * @endcode
     * @code
     * bbr state
     * Secondary
     * Done
     * @endcode
     * @par
     * Available when `OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE` is enabled.
     * @par api_copy
     * #otBackboneRouterGetState
     */
    else if (aArgs[0] == "state")
    {
        static const char *const kStateStrings[] = {
            "Disabled",  // (0) OT_BACKBONE_ROUTER_STATE_DISABLED
            "Secondary", // (1) OT_BACKBONE_ROUTER_STATE_SECONDARY
            "Primary",   // (2) OT_BACKBONE_ROUTER_STATE_PRIMARY
        };

        static_assert(0 == OT_BACKBONE_ROUTER_STATE_DISABLED, "OT_BACKBONE_ROUTER_STATE_DISABLED value is incorrect");
        static_assert(1 == OT_BACKBONE_ROUTER_STATE_SECONDARY, "OT_BACKBONE_ROUTER_STATE_SECONDARY value is incorrect");
        static_assert(2 == OT_BACKBONE_ROUTER_STATE_PRIMARY, "OT_BACKBONE_ROUTER_STATE_PRIMARY value is incorrect");

        OutputLine("%s", Stringify(otBackboneRouterGetState(GetInstancePtr()), kStateStrings));
    }
    /**
     * @cli bbr config
     * @code
     * bbr config
     * seqno:    10
     * delay:    120 secs
     * timeout:  300 secs
     * Done
     * @endcode
     * @par api_copy
     * #otBackboneRouterGetConfig
     */
    else if (aArgs[0] == "config")
    {
        otBackboneRouterGetConfig(GetInstancePtr(), &config);

        if (aArgs[1].IsEmpty())
        {
            OutputLine("seqno:    %d", config.mSequenceNumber);
            OutputLine("delay:    %d secs", config.mReregistrationDelay);
            OutputLine("timeout:  %d secs", config.mMlrTimeout);
        }
        else
        {
            // Set local Backbone Router configuration.
            /**
             * @cli bbr config (set)
             * @code
             * bbr config seqno 20 delay 30
             * Done
             * @endcode
             * @cparam bbr config [seqno @ca{seqno}] [delay @ca{delay}] [timeout @ca{timeout}]
             * @par
             * `bbr register` should be issued explicitly to register Backbone Router service to Leader
             * for Secondary Backbone Router.
             * @par api_copy
             * #otBackboneRouterSetConfig
             */
            for (Arg *arg = &aArgs[1]; !arg->IsEmpty(); arg++)
            {
                if (*arg == "seqno")
                {
                    arg++;
                    SuccessOrExit(error = arg->ParseAsUint8(config.mSequenceNumber));
                }
                else if (*arg == "delay")
                {
                    arg++;
                    SuccessOrExit(error = arg->ParseAsUint16(config.mReregistrationDelay));
                }
                else if (*arg == "timeout")
                {
                    arg++;
                    SuccessOrExit(error = arg->ParseAsUint32(config.mMlrTimeout));
                }
                else
                {
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }

            SuccessOrExit(error = otBackboneRouterSetConfig(GetInstancePtr(), &config));
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

template <> otError Interpreter::Process<Cmd("domainname")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli domainname
     * @code
     * domainname
     * Thread
     * Done
     * @endcode
     * @par api_copy
     * #otThreadGetDomainName
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otThreadGetDomainName(GetInstancePtr()));
    }
    /**
     * @cli domainname (set)
     * @code
     * domainname Test\ Thread
     * Done
     * @endcode
     * @cparam domainname @ca{name}
     * Use a `backslash` to escape spaces.
     * @par api_copy
     * #otThreadSetDomainName
     */
    else
    {
        SuccessOrExit(error = otThreadSetDomainName(GetInstancePtr(), aArgs[0].GetCString()));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
template <> otError Interpreter::Process<Cmd("dua")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli dua iid
     * @code
     * dua iid
     * 0004000300020001
     * Done
     * @endcode
     * @par api_copy
     * #otThreadGetFixedDuaInterfaceIdentifier
     */
    if (aArgs[0] == "iid")
    {
        if (aArgs[1].IsEmpty())
        {
            const otIp6InterfaceIdentifier *iid = otThreadGetFixedDuaInterfaceIdentifier(GetInstancePtr());

            if (iid != nullptr)
            {
                OutputBytesLine(iid->mFields.m8);
            }
        }
        /**
         * @cli dua iid (set,clear)
         * @code
         * dua iid 0004000300020001
         * Done
         * @endcode
         * @code
         * dua iid clear
         * Done
         * @endcode
         * @cparam dua iid @ca{iid|clear}
         * `dua iid clear` passes a `nullptr` to #otThreadSetFixedDuaInterfaceIdentifier.
         * Otherwise, you can pass the `iid`.
         * @par api_copy
         * #otThreadSetFixedDuaInterfaceIdentifier
         */
        else if (aArgs[1] == "clear")
        {
            error = otThreadSetFixedDuaInterfaceIdentifier(GetInstancePtr(), nullptr);
        }
        else
        {
            otIp6InterfaceIdentifier iid;

            SuccessOrExit(error = aArgs[1].ParseAsHexString(iid.mFields.m8));
            error = otThreadSetFixedDuaInterfaceIdentifier(GetInstancePtr(), &iid);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

/**
 * @cli bufferinfo
 * @code
 * bufferinfo
 * total: 40
 * free: 40
 * 6lo send: 0 0 0
 * 6lo reas: 0 0 0
 * ip6: 0 0 0
 * mpl: 0 0 0
 * mle: 0 0 0
 * coap: 0 0 0
 * coap secure: 0 0 0
 * application coap: 0 0 0
 * Done
 * @endcode
 * @par
 * Gets the current message buffer information.
 * *   `total` displays the total number of message buffers in pool.
 * *   `free` displays the number of free message buffers.
 * @par
 * Next, the CLI displays info about different queues used by the OpenThread stack,
 * for example `6lo send`. Each line after the queue represents info about a queue:
 * *   The first number shows number messages in the queue.
 * *   The second number shows number of buffers used by all messages in the queue.
 * *   The third number shows total number of bytes of all messages in the queue.
 * @sa otMessageGetBufferInfo
 */
template <> otError Interpreter::Process<Cmd("bufferinfo")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    struct BufferInfoName
    {
        const otMessageQueueInfo otBufferInfo::*mQueuePtr;
        const char *                            mName;
    };

    static const BufferInfoName kBufferInfoNames[] = {
        {&otBufferInfo::m6loSendQueue, "6lo send"},
        {&otBufferInfo::m6loReassemblyQueue, "6lo reas"},
        {&otBufferInfo::mIp6Queue, "ip6"},
        {&otBufferInfo::mMplQueue, "mpl"},
        {&otBufferInfo::mMleQueue, "mle"},
        {&otBufferInfo::mCoapQueue, "coap"},
        {&otBufferInfo::mCoapSecureQueue, "coap secure"},
        {&otBufferInfo::mApplicationCoapQueue, "application coap"},
    };

    otBufferInfo bufferInfo;

    otMessageGetBufferInfo(GetInstancePtr(), &bufferInfo);

    OutputLine("total: %d", bufferInfo.mTotalBuffers);
    OutputLine("free: %d", bufferInfo.mFreeBuffers);

    for (const BufferInfoName &info : kBufferInfoNames)
    {
        OutputLine("%s: %u %u %u", info.mName, (bufferInfo.*info.mQueuePtr).mNumMessages,
                   (bufferInfo.*info.mQueuePtr).mNumBuffers, (bufferInfo.*info.mQueuePtr).mTotalBytes);
    }

    return OT_ERROR_NONE;
}

/**
 * @cli ccathreshold (get,set)
 * @code
 * ccathreshold
 * -75 dBm
 * Done
 * @endcode
 * @code
 * ccathreshold -62
 * Done
 * @endcode
 * @cparam ccathreshold [@ca{CCA-threshold-dBm}]
 * Use the optional `CCA-threshold-dBm` argument to set the CCA threshold.
 * @par
 * Gets or sets the CCA threshold in dBm measured at the antenna connector per
 * IEEE 802.15.4 - 2015 section 10.1.4.
 * @sa otPlatRadioGetCcaEnergyDetectThreshold
 * @sa otPlatRadioSetCcaEnergyDetectThreshold
 */
template <> otError Interpreter::Process<Cmd("ccathreshold")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    int8_t  cca;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otPlatRadioGetCcaEnergyDetectThreshold(GetInstancePtr(), &cca));
        OutputLine("%d dBm", cca);
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsInt8(cca));
        error = otPlatRadioSetCcaEnergyDetectThreshold(GetInstancePtr(), cca);
    }

exit:
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
template <> otError Interpreter::Process<Cmd("ccm")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_COMMAND);

    SuccessOrExit(error = ParseEnableOrDisable(aArgs[0], enable));
    otThreadSetCcmEnabled(GetInstancePtr(), enable);

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("tvcheck")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_COMMAND);

    SuccessOrExit(error = ParseEnableOrDisable(aArgs[0], enable));
    otThreadSetThreadVersionCheckEnabled(GetInstancePtr(), enable);

exit:
    return error;
}

#endif

/**
 * @cli channel (get,set)
 * @code
 * channel
 * 11
 * Done
 * @endcode
 * @code
 * channel 11
 * Done
 * @endcode
 * @cparam channel [@ca{channel-num}]
 * Use `channel-num` to set the channel.
 * @par
 * Gets or sets the IEEE 802.15.4 Channel value.
 */
template <> otError Interpreter::Process<Cmd("channel")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli channel supported
     * @code
     * channel supported
     * 0x7fff800
     * Done
     * @endcode
     * @par api_copy
     * #otPlatRadioGetSupportedChannelMask
     */
    if (aArgs[0] == "supported")
    {
        OutputLine("0x%x", otPlatRadioGetSupportedChannelMask(GetInstancePtr()));
    }
    /**
     * @cli channel preferred
     * @code
     * channel preferred
     * 0x7fff800
     * Done
     * @endcode
     * @par api_copy
     * #otPlatRadioGetPreferredChannelMask
     */
    else if (aArgs[0] == "preferred")
    {
        OutputLine("0x%x", otPlatRadioGetPreferredChannelMask(GetInstancePtr()));
    }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
    /**
     * @cli channel monitor
     * @code
     * channel monitor
     * enabled: 1
     * interval: 41000
     * threshold: -75
     * window: 960
     * count: 10552
     * occupancies:
     * ch 11 (0x0cb7)  4.96% busy
     * ch 12 (0x2e2b) 18.03% busy
     * ch 13 (0x2f54) 18.48% busy
     * ch 14 (0x0fef)  6.22% busy
     * ch 15 (0x1536)  8.28% busy
     * ch 16 (0x1746)  9.09% busy
     * ch 17 (0x0b8b)  4.50% busy
     * ch 18 (0x60a7) 37.75% busy
     * ch 19 (0x0810)  3.14% busy
     * ch 20 (0x0c2a)  4.75% busy
     * ch 21 (0x08dc)  3.46% busy
     * ch 22 (0x101d)  6.29% busy
     * ch 23 (0x0092)  0.22% busy
     * ch 24 (0x0028)  0.06% busy
     * ch 25 (0x0063)  0.15% busy
     * ch 26 (0x058c)  2.16% busy
     * Done
     * @endcode
     * @par
     * Get the current channel monitor state and channel occupancy.
     * `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.
     */
    else if (aArgs[0] == "monitor")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputLine("enabled: %d", otChannelMonitorIsEnabled(GetInstancePtr()));
            if (otChannelMonitorIsEnabled(GetInstancePtr()))
            {
                uint32_t channelMask = otLinkGetSupportedChannelMask(GetInstancePtr());
                uint8_t  channelNum  = sizeof(channelMask) * CHAR_BIT;

                OutputLine("interval: %u", otChannelMonitorGetSampleInterval(GetInstancePtr()));
                OutputLine("threshold: %d", otChannelMonitorGetRssiThreshold(GetInstancePtr()));
                OutputLine("window: %u", otChannelMonitorGetSampleWindow(GetInstancePtr()));
                OutputLine("count: %u", otChannelMonitorGetSampleCount(GetInstancePtr()));

                OutputLine("occupancies:");
                for (uint8_t channel = 0; channel < channelNum; channel++)
                {
                    uint32_t occupancy = 0;

                    if (!((1UL << channel) & channelMask))
                    {
                        continue;
                    }

                    occupancy = otChannelMonitorGetChannelOccupancy(GetInstancePtr(), channel);

                    OutputFormat("ch %d (0x%04x) ", channel, occupancy);
                    occupancy = (occupancy * 10000) / 0xffff;
                    OutputLine("%2d.%02d%% busy", occupancy / 100, occupancy % 100);
                }
                OutputLine("");
            }
        }
        /**
         * @cli channel monitor start
         * @code
         * channel monitor start
         * channel monitor start
         * Done
         * @endcode
         * @par
         * Start the channel monitor.
         * OT CLI sends a boolean value of `true` to #otChannelMonitorSetEnabled.
         * `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.
         * @sa otChannelMonitorSetEnabled
         */
        else if (aArgs[1] == "start")
        {
            error = otChannelMonitorSetEnabled(GetInstancePtr(), true);
        }
        /**
         * @cli channel monitor stop
         * @code
         * channel monitor stop
         * channel monitor stop
         * Done
         * @endcode
         * @par
         * Stop the channel monitor.
         * OT CLI sends a boolean value of `false` to #otChannelMonitorSetEnabled.
         * `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` is required.
         * @sa otChannelMonitorSetEnabled
         */
        else if (aArgs[1] == "stop")
        {
            error = otChannelMonitorSetEnabled(GetInstancePtr(), false);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif // OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
    else if (aArgs[0] == "manager")
    {
        /**
         * @cli channel manager
         * @code
         * channel manager
         * channel: 11
         * auto: 1
         * delay: 120
         * interval: 10800
         * supported: { 11-26}
         * favored: { 11-26}
         * Done
         * @endcode
         * @par
         * Get the channel manager state.
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` is required.
         * @sa otChannelManagerGetRequestedChannel
         */
        if (aArgs[1].IsEmpty())
        {
            OutputLine("channel: %d", otChannelManagerGetRequestedChannel(GetInstancePtr()));
            OutputLine("auto: %d", otChannelManagerGetAutoChannelSelectionEnabled(GetInstancePtr()));

            if (otChannelManagerGetAutoChannelSelectionEnabled(GetInstancePtr()))
            {
                Mac::ChannelMask supportedMask(otChannelManagerGetSupportedChannels(GetInstancePtr()));
                Mac::ChannelMask favoredMask(otChannelManagerGetFavoredChannels(GetInstancePtr()));

                OutputLine("delay: %d", otChannelManagerGetDelay(GetInstancePtr()));
                OutputLine("interval: %u", otChannelManagerGetAutoChannelSelectionInterval(GetInstancePtr()));
                OutputLine("cca threshold: 0x%04x", otChannelManagerGetCcaFailureRateThreshold(GetInstancePtr()));
                OutputLine("supported: %s", supportedMask.ToString().AsCString());
                OutputLine("favored: %s", supportedMask.ToString().AsCString());
            }
        }
        /**
         * @cli channel manager change
         * @code
         * channel manager change 11
         * channel manager change 11
         * Done
         * @endcode
         * @cparam channel manager change @ca{channel-num}
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` is required.
         * @par api_copy
         * #otChannelManagerRequestChannelChange
         */
        else if (aArgs[1] == "change")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerRequestChannelChange);
        }
#if OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE
        /**
         * @cli channel manager select
         * @code
         * channel manager select 1
         * channel manager select 1
         * Done
         * @endcode
         * @cparam channel manager select @ca{skip-quality-check}
         * Use a `1` or `0` for the boolean `skip-quality-check`.
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerRequestChannelSelect
         */
        else if (aArgs[1] == "select")
        {
            bool enable;

            SuccessOrExit(error = aArgs[2].ParseAsBool(enable));
            error = otChannelManagerRequestChannelSelect(GetInstancePtr(), enable);
        }
#endif
        /**
         * @cli channel manager auto
         * @code
         * channel manager auto 1
         * channel manager auto 1
         * Done
         * @endcode
         * @cparam channel manager auto @ca{enable}
         * `1` is a boolean to `enable`.
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetAutoChannelSelectionEnabled
         */
        else if (aArgs[1] == "auto")
        {
            bool enable;

            SuccessOrExit(error = aArgs[2].ParseAsBool(enable));
            otChannelManagerSetAutoChannelSelectionEnabled(GetInstancePtr(), enable);
        }
        /**
         * @cli channel manager delay
         * @code
         * channel manager delay 120
         * channel manager delay 120
         * Done
         * @endcode
         * @cparam channel manager delay @ca{delay-seconds}
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetDelay
         */
        else if (aArgs[1] == "delay")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerSetDelay);
        }
        /**
         * @cli channel manager interval
         * @code
         * channel manager interval 10800
         * channel manager interval 10800
         * Done
         * @endcode
         * @cparam channel manager interval @ca{interval-seconds}
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetAutoChannelSelectionInterval
         */
        else if (aArgs[1] == "interval")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerSetAutoChannelSelectionInterval);
        }
        /**
         * @cli channel manager supported
         * @code
         * channel manager supported 0x7fffc00
         * channel manager supported 0x7fffc00
         * Done
         * @endcode
         * @cparam channel manager supported @ca{mask}
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetSupportedChannels
         */
        else if (aArgs[1] == "supported")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerSetSupportedChannels);
        }
        /**
         * @cli channel manager favored
         * @code
         * channel manager favored 0x7fffc00
         * channel manager favored 0x7fffc00
         * Done
         * @endcode
         * @cparam channel manager favored @ca{mask}
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetFavoredChannels
         */
        else if (aArgs[1] == "favored")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerSetFavoredChannels);
        }
        /**
         * @cli channel manager threshold
         * @code
         * channel manager threshold 0xffff
         * channel manager threshold 0xffff
         * Done
         * @endcode
         * @cparam channel manager threshold @ca{threshold-percent}
         * Use a hex value for `threshold-percent`. `0` maps to 0% and `0xffff` maps to 100%.
         * @par
         * `OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE` and `OPENTHREAD_CONFIG_CHANNEL_MONITOR_ENABLE` are required.
         * @par api_copy
         * #otChannelManagerSetCcaFailureRateThreshold
         */
        else if (aArgs[1] == "threshold")
        {
            error = ProcessSet(aArgs + 2, otChannelManagerSetCcaFailureRateThreshold);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
#endif // OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE && OPENTHREAD_FTD
    else
    {
        ExitNow(error = ProcessGetSet(aArgs, otLinkGetChannel, otLinkSetChannel));
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("child")>(Arg aArgs[])
{
    otError          error = OT_ERROR_NONE;
    otChildInfo      childInfo;
    uint16_t         childId;
    bool             isTable;
    otLinkModeConfig linkMode;
    char             linkModeString[kLinkModeStringSize];

    isTable = (aArgs[0] == "table");

    if (isTable || (aArgs[0] == "list"))
    {
        uint16_t maxChildren;

        /**
         * @cli child table
         * @code
         * child table
         * | ID  | RLOC16 | Timeout    | Age        | LQ In | C_VN |R|D|N|Ver|CSL|QMsgCnt| Extended MAC     |
         * +-----+--------+------------+------------+-------+------+-+-+-+---+---+-------+------------------+
         * |   1 | 0xc801 |        240 |         24 |     3 |  131 |1|0|0|  3| 0 |     0 | 4ecede68435358ac |
         * |   2 | 0xc802 |        240 |          2 |     3 |  131 |0|0|0|  3| 1 |     0 | a672a601d2ce37d8 |
         * Done
         * @endcode
         * @par
         * Prints a table of the attached children.
         * @sa otThreadGetChildInfoByIndex
         */
        if (isTable)
        {
            static const char *const kChildTableTitles[] = {
                "ID", "RLOC16", "Timeout", "Age", "LQ In",   "C_VN",         "R",
                "D",  "N",      "Ver",     "CSL", "QMsgCnt", "Extended MAC",
            };

            static const uint8_t kChildTableColumnWidths[] = {
                5, 8, 12, 12, 7, 6, 1, 1, 1, 3, 3, 7, 18,
            };

            OutputTableHeader(kChildTableTitles, kChildTableColumnWidths);
        }

        maxChildren = otThreadGetMaxAllowedChildren(GetInstancePtr());

        for (uint16_t i = 0; i < maxChildren; i++)
        {
            if ((otThreadGetChildInfoByIndex(GetInstancePtr(), i, &childInfo) != OT_ERROR_NONE) ||
                childInfo.mIsStateRestoring)
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
            /**
             * @cli child list
             * @code
             * child list
             * 1 2 3 6 7 8
             * Done
             * @endcode
             * @par
             * Returns a list of attached Child IDs.
             * @sa otThreadGetChildInfoByIndex
             */
            else
            {
                OutputFormat("%d ", childInfo.mChildId);
            }
        }

        OutputLine("");
        ExitNow();
    }

    SuccessOrExit(error = aArgs[0].ParseAsUint16(childId));
    SuccessOrExit(error = otThreadGetChildInfoById(GetInstancePtr(), childId, &childInfo));

    /**
     * @cli child (id)
     * @code
     * child 1
     * Child ID: 1
     * Rloc: 9c01
     * Ext Addr: e2b3540590b0fd87
     * Mode: rn
     * Net Data: 184
     * Timeout: 100
     * Age: 0
     * Link Quality In: 3
     * RSSI: -20
     * Done
     * @endcode
     * @cparam child @ca{child-id}
     * @par api_copy
     * #otThreadGetChildInfoById
     */
    OutputLine("Child ID: %d", childInfo.mChildId);
    OutputLine("Rloc: %04x", childInfo.mRloc16);
    OutputFormat("Ext Addr: ");
    OutputExtAddressLine(childInfo.mExtAddress);
    linkMode.mRxOnWhenIdle = childInfo.mRxOnWhenIdle;
    linkMode.mDeviceType   = childInfo.mFullThreadDevice;
    linkMode.mNetworkData  = childInfo.mFullThreadDevice;
    OutputLine("Mode: %s", LinkModeToString(linkMode, linkModeString));
    OutputLine("Net Data: %d", childInfo.mNetworkDataVersion);
    OutputLine("Timeout: %d", childInfo.mTimeout);
    OutputLine("Age: %d", childInfo.mAge);
    OutputLine("Link Quality In: %d", childInfo.mLinkQualityIn);
    OutputLine("RSSI: %d", childInfo.mAverageRssi);

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("childip")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli childip
     * @code
     * childip
     * 3401: fdde:ad00:beef:0:3037:3e03:8c5f:bc0c
     * Done
     * @endcode
     * @par
     * Gets a list of IP addresses stored for MTD children.
     * @sa otThreadGetChildNextIp6Address
     */
    if (aArgs[0].IsEmpty())
    {
        uint16_t maxChildren = otThreadGetMaxAllowedChildren(GetInstancePtr());

        for (uint16_t childIndex = 0; childIndex < maxChildren; childIndex++)
        {
            otChildIp6AddressIterator iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;
            otIp6Address              ip6Address;
            otChildInfo               childInfo;

            if ((otThreadGetChildInfoByIndex(GetInstancePtr(), childIndex, &childInfo) != OT_ERROR_NONE) ||
                childInfo.mIsStateRestoring)
            {
                continue;
            }

            iterator = OT_CHILD_IP6_ADDRESS_ITERATOR_INIT;

            while (otThreadGetChildNextIp6Address(GetInstancePtr(), childIndex, &iterator, &ip6Address) ==
                   OT_ERROR_NONE)
            {
                OutputFormat("%04x: ", childInfo.mRloc16);
                OutputIp6AddressLine(ip6Address);
            }
        }
    }
    /**
     * @cli childip max
     * @code
     * childip max
     * 4
     * Done
     * @endcode
     * @par api_copy
     * #otThreadGetMaxChildIpAddresses
     */
    else if (aArgs[0] == "max")
    {
#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        error = ProcessGet(aArgs + 1, otThreadGetMaxChildIpAddresses);
#else
        /**
         * @cli childip max (set)
         * @code
         * childip max 2
         * Done
         * @endcode
         * @cparam childip max @ca{count}
         * @par api_copy
         * #otThreadSetMaxChildIpAddresses
         */
        error = ProcessGetSet(aArgs + 1, otThreadGetMaxChildIpAddresses, otThreadSetMaxChildIpAddresses);
#endif
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

/**
 * @cli childmax
 * @code
 * childmax
 * 5
 * Done
 * @endcode
 * @par api_copy
 * #otThreadGetMaxAllowedChildren
 */
template <> otError Interpreter::Process<Cmd("childmax")>(Arg aArgs[])
{
    /**
     * @cli childmax (set)
     * @code
     * childmax 2
     * Done
     * @endcode
     * @cparam childmax @ca{count}
     * @par api_copy
     * #otThreadSetMaxAllowedChildren
     */
    return ProcessGetSet(aArgs, otThreadGetMaxAllowedChildren, otThreadSetMaxAllowedChildren);
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
template <> otError Interpreter::Process<Cmd("childsupervision")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    /**
     * @cli childsupervision checktimeout
     * @code
     * childsupervision checktimeout
     * 30
     * Done
     * @endcode
     * @par api_copy
     * #otChildSupervisionGetCheckTimeout
     */
    if (aArgs[0] == "checktimeout")
    {
        /** @cli childsupervision checktimeout (set)
         * @code
         * childsupervision checktimeout 30
         * Done
         * @endcode
         * @cparam childsupervision checktimeout @ca{timeout-seconds}
         * @par api_copy
         * #otChildSupervisionSetCheckTimeout
         */
        error = ProcessGetSet(aArgs + 1, otChildSupervisionGetCheckTimeout, otChildSupervisionSetCheckTimeout);
    }
#if OPENTHREAD_FTD
    /**
     * @cli childsupervision interval
     * @code
     * childsupervision interval
     * 30
     * Done
     * @endcode
     * @par
     * This command can only be used with FTD devices.
     * @par api_copy
     * #otChildSupervisionGetInterval
     */
    else if (aArgs[0] == "interval")
    {
        /**
         * @cli childsupervision interval (set)
         * @code
         * childsupervision interval 30
         * Done
         * @endcode
         * @cparam childsupervision interval @ca{interval-seconds}
         * @par
         * This command can only be used with FTD devices.
         * @par api_copy
         * #otChildSupervisionSetInterval
         */
        error = ProcessGetSet(aArgs + 1, otChildSupervisionGetInterval, otChildSupervisionSetInterval);
    }
#endif

    return error;
}
#endif // OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE

/** @cli childtimeout
 * @code
 * childtimeout
 * 300
 * Done
 * @endcode
 * @par api_copy
 * #otThreadGetChildTimeout
 */
template <> otError Interpreter::Process<Cmd("childtimeout")>(Arg aArgs[])
{
    /** @cli childtimeout (set)
     * @code
     * childtimeout 300
     * Done
     * @endcode
     * @cparam childtimeout @ca{timeout-seconds}
     * @par api_copy
     * #otThreadSetChildTimeout
     */
    return ProcessGetSet(aArgs, otThreadGetChildTimeout, otThreadSetChildTimeout);
}

#if OPENTHREAD_CONFIG_COAP_API_ENABLE
template <> otError Interpreter::Process<Cmd("coap")>(Arg aArgs[])
{
    return mCoap.Process(aArgs);
}
#endif

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
template <> otError Interpreter::Process<Cmd("coaps")>(Arg aArgs[])
{
    return mCoapSecure.Process(aArgs);
}
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
template <> otError Interpreter::Process<Cmd("coex")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otPlatRadioIsCoexEnabled(GetInstancePtr()));
    }
    else if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        error = otPlatRadioSetCoexEnabled(GetInstancePtr(), enable);
    }
    else if (aArgs[0] == "metrics")
    {
        struct RadioCoexMetricName
        {
            const uint32_t otRadioCoexMetrics::*mValuePtr;
            const char *                        mName;
        };

        static const RadioCoexMetricName kTxMetricNames[] = {
            {&otRadioCoexMetrics::mNumTxRequest, "Request"},
            {&otRadioCoexMetrics::mNumTxGrantImmediate, "Grant Immediate"},
            {&otRadioCoexMetrics::mNumTxGrantWait, "Grant Wait"},
            {&otRadioCoexMetrics::mNumTxGrantWaitActivated, "Grant Wait Activated"},
            {&otRadioCoexMetrics::mNumTxGrantWaitTimeout, "Grant Wait Timeout"},
            {&otRadioCoexMetrics::mNumTxGrantDeactivatedDuringRequest, "Grant Deactivated During Request"},
            {&otRadioCoexMetrics::mNumTxDelayedGrant, "Delayed Grant"},
            {&otRadioCoexMetrics::mAvgTxRequestToGrantTime, "Average Request To Grant Time"},
        };

        static const RadioCoexMetricName kRxMetricNames[] = {
            {&otRadioCoexMetrics::mNumRxRequest, "Request"},
            {&otRadioCoexMetrics::mNumRxGrantImmediate, "Grant Immediate"},
            {&otRadioCoexMetrics::mNumRxGrantWait, "Grant Wait"},
            {&otRadioCoexMetrics::mNumRxGrantWaitActivated, "Grant Wait Activated"},
            {&otRadioCoexMetrics::mNumRxGrantWaitTimeout, "Grant Wait Timeout"},
            {&otRadioCoexMetrics::mNumRxGrantDeactivatedDuringRequest, "Grant Deactivated During Request"},
            {&otRadioCoexMetrics::mNumRxDelayedGrant, "Delayed Grant"},
            {&otRadioCoexMetrics::mAvgRxRequestToGrantTime, "Average Request To Grant Time"},
            {&otRadioCoexMetrics::mNumRxGrantNone, "Grant None"},
        };

        otRadioCoexMetrics metrics;

        SuccessOrExit(error = otPlatRadioGetCoexMetrics(GetInstancePtr(), &metrics));

        OutputLine("Stopped: %s", metrics.mStopped ? "true" : "false");
        OutputLine("Grant Glitch: %u", metrics.mNumGrantGlitch);
        OutputLine("Transmit metrics");

        for (const RadioCoexMetricName &metric : kTxMetricNames)
        {
            OutputLine(kIndentSize, "%s: %u", metric.mName, metrics.*metric.mValuePtr);
        }

        OutputLine("Receive metrics");

        for (const RadioCoexMetricName &metric : kRxMetricNames)
        {
            OutputLine(kIndentSize, "%s: %u", metric.mName, metrics.*metric.mValuePtr);
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE

#if OPENTHREAD_FTD
/**
 * @cli contextreusedelay (get,set)
 * @code
 * contextreusedelay
 * 11
 * Done
 * @endcode
 * @code
 * contextreusedelay 11
 * Done
 * @endcode
 * @cparam contextreusedelay @ca{delay}
 * Use the optional `delay` argument to set the `CONTEXT_ID_REUSE_DELAY`.
 * @par
 * Gets or sets the `CONTEXT_ID_REUSE_DELAY` value.
 * @sa otThreadGetContextIdReuseDelay
 * @sa otThreadSetContextIdReuseDelay
 */
template <> otError Interpreter::Process<Cmd("contextreusedelay")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetContextIdReuseDelay, otThreadSetContextIdReuseDelay);
}
#endif

template <> otError Interpreter::Process<Cmd("counters")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli counters
     * @code
     * counters
     * ip
     * mac
     * mle
     * Done
     * @endcode
     * @par
     * Gets the supported counter names.
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("ip");
        OutputLine("mac");
        OutputLine("mle");
    }
    /**
     * @cli counters (mac)
     * @code
     * counters mac
     * TxTotal: 10
     *    TxUnicast: 3
     *    TxBroadcast: 7
     *    TxAckRequested: 3
     *    TxAcked: 3
     *    TxNoAckRequested: 7
     *    TxData: 10
     *    TxDataPoll: 0
     *    TxBeacon: 0
     *    TxBeaconRequest: 0
     *    TxOther: 0
     *    TxRetry: 0
     *    TxErrCca: 0
     *    TxErrBusyChannel: 0
     * RxTotal: 2
     *    RxUnicast: 1
     *    RxBroadcast: 1
     *    RxData: 2
     *    RxDataPoll: 0
     *    RxBeacon: 0
     *    RxBeaconRequest: 0
     *    RxOther: 0
     *    RxAddressFiltered: 0
     *    RxDestAddrFiltered: 0
     *    RxDuplicated: 0
     *    RxErrNoFrame: 0
     *    RxErrNoUnknownNeighbor: 0
     *    RxErrInvalidSrcAddr: 0
     *    RxErrSec: 0
     *    RxErrFcs: 0
     *    RxErrOther: 0
     * Done
     * @endcode
     * @cparam counters @ca{mac}
     * @par api_copy
     * #otLinkGetCounters
     */
    else if (aArgs[0] == "mac")
    {
        if (aArgs[1].IsEmpty())
        {
            struct MacCounterName
            {
                const uint32_t otMacCounters::*mValuePtr;
                const char *                   mName;
            };

            static const MacCounterName kTxCounterNames[] = {
                {&otMacCounters::mTxUnicast, "TxUnicast"},
                {&otMacCounters::mTxBroadcast, "TxBroadcast"},
                {&otMacCounters::mTxAckRequested, "TxAckRequested"},
                {&otMacCounters::mTxAcked, "TxAcked"},
                {&otMacCounters::mTxNoAckRequested, "TxNoAckRequested"},
                {&otMacCounters::mTxData, "TxData"},
                {&otMacCounters::mTxDataPoll, "TxDataPoll"},
                {&otMacCounters::mTxBeacon, "TxBeacon"},
                {&otMacCounters::mTxBeaconRequest, "TxBeaconRequest"},
                {&otMacCounters::mTxOther, "TxOther"},
                {&otMacCounters::mTxRetry, "TxRetry"},
                {&otMacCounters::mTxErrCca, "TxErrCca"},
                {&otMacCounters::mTxErrBusyChannel, "TxErrBusyChannel"},
            };

            static const MacCounterName kRxCounterNames[] = {
                {&otMacCounters::mRxUnicast, "RxUnicast"},
                {&otMacCounters::mRxBroadcast, "RxBroadcast"},
                {&otMacCounters::mRxData, "RxData"},
                {&otMacCounters::mRxDataPoll, "RxDataPoll"},
                {&otMacCounters::mRxBeacon, "RxBeacon"},
                {&otMacCounters::mRxBeaconRequest, "RxBeaconRequest"},
                {&otMacCounters::mRxOther, "RxOther"},
                {&otMacCounters::mRxAddressFiltered, "RxAddressFiltered"},
                {&otMacCounters::mRxDestAddrFiltered, "RxDestAddrFiltered"},
                {&otMacCounters::mRxDuplicated, "RxDuplicated"},
                {&otMacCounters::mRxErrNoFrame, "RxErrNoFrame"},
                {&otMacCounters::mRxErrUnknownNeighbor, "RxErrNoUnknownNeighbor"},
                {&otMacCounters::mRxErrInvalidSrcAddr, "RxErrInvalidSrcAddr"},
                {&otMacCounters::mRxErrSec, "RxErrSec"},
                {&otMacCounters::mRxErrFcs, "RxErrFcs"},
                {&otMacCounters::mRxErrOther, "RxErrOther"},
            };

            const otMacCounters *macCounters = otLinkGetCounters(GetInstancePtr());

            OutputLine("TxTotal: %d", macCounters->mTxTotal);

            for (const MacCounterName &counter : kTxCounterNames)
            {
                OutputLine(kIndentSize, "%s: %u", counter.mName, macCounters->*counter.mValuePtr);
            }

            OutputLine("RxTotal: %d", macCounters->mRxTotal);

            for (const MacCounterName &counter : kRxCounterNames)
            {
                OutputLine(kIndentSize, "%s: %u", counter.mName, macCounters->*counter.mValuePtr);
            }
        }
        /**
         * @cli counters mac reset
         * @code
         * counters mac reset
         * Done
         * @endcode
         * @cparam counters @ca{mac} reset
         * @par api_copy
         * #otLinkResetCounters
         */
        else if ((aArgs[1] == "reset") && aArgs[2].IsEmpty())
        {
            otLinkResetCounters(GetInstancePtr());
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    /**
     * @cli counters (mle)
     * @code
     * counters mle
     * Role Disabled: 0
     * Role Detached: 1
     * Role Child: 0
     * Role Router: 0
     * Role Leader: 1
     * Attach Attempts: 1
     * Partition Id Changes: 1
     * Better Partition Attach Attempts: 0
     * Parent Changes: 0
     * Done
     * @endcode
     * @cparam counters @ca{mle}
     * @par api_copy
     * #otThreadGetMleCounters
     */
    else if (aArgs[0] == "mle")
    {
        if (aArgs[1].IsEmpty())
        {
            struct MleCounterName
            {
                const uint16_t otMleCounters::*mValuePtr;
                const char *                   mName;
            };

            static const MleCounterName kCounterNames[] = {
                {&otMleCounters::mDisabledRole, "Role Disabled"},
                {&otMleCounters::mDetachedRole, "Role Detached"},
                {&otMleCounters::mChildRole, "Role Child"},
                {&otMleCounters::mRouterRole, "Role Router"},
                {&otMleCounters::mLeaderRole, "Role Leader"},
                {&otMleCounters::mAttachAttempts, "Attach Attempts"},
                {&otMleCounters::mPartitionIdChanges, "Partition Id Changes"},
                {&otMleCounters::mBetterPartitionAttachAttempts, "Better Partition Attach Attempts"},
                {&otMleCounters::mParentChanges, "Parent Changes"},
            };

            const otMleCounters *mleCounters = otThreadGetMleCounters(GetInstancePtr());

            for (const MleCounterName &counter : kCounterNames)
            {
                OutputLine("%s: %d", counter.mName, mleCounters->*counter.mValuePtr);
            }
        }
        /**
         * @cli counters mle reset
         * @code
         * counters mle reset
         * Done
         * @endcode
         * @cparam counters @ca{mle} reset
         * @par api_copy
         * #otThreadResetMleCounters
         */
        else if ((aArgs[1] == "reset") && aArgs[2].IsEmpty())
        {
            otThreadResetMleCounters(GetInstancePtr());
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    /**
     * @cli counters ip
     * @code
     * counters ip
     * TxSuccess: 10
     * TxFailed: 0
     * RxSuccess: 5
     * RxFailed: 0
     * Done
     * @endcode
     * @cparam counters @ca{ip}
     * @par api_copy
     * #otThreadGetIp6Counters
     */
    else if (aArgs[0] == "ip")
    {
        if (aArgs[1].IsEmpty())
        {
            struct IpCounterName
            {
                const uint32_t otIpCounters::*mValuePtr;
                const char *                  mName;
            };

            static const IpCounterName kCounterNames[] = {
                {&otIpCounters::mTxSuccess, "TxSuccess"},
                {&otIpCounters::mTxFailure, "TxFailed"},
                {&otIpCounters::mRxSuccess, "RxSuccess"},
                {&otIpCounters::mRxFailure, "RxFailed"},
            };

            const otIpCounters *ipCounters = otThreadGetIp6Counters(GetInstancePtr());

            for (const IpCounterName &counter : kCounterNames)
            {
                OutputLine("%s: %d", counter.mName, ipCounters->*counter.mValuePtr);
            }
        }
        /**
         * @cli counters ip reset
         * @code
         * counters ip reset
         * Done
         * @endcode
         * @cparam counters @ca{ip} reset
         * @par api_copy
         * #otThreadResetIp6Counters
         */
        else if ((aArgs[1] == "reset") && aArgs[2].IsEmpty())
        {
            otThreadResetIp6Counters(GetInstancePtr());
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
template <> otError Interpreter::Process<Cmd("csl")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli csl
     * @code
     * csl
     * Channel: 11
     * Period: 1000 (in units of 10 symbols), 160ms
     * Timeout: 1000s
     * Done
     * @endcode
     * @par
     * Gets the CSL configuration.
     * @sa otLinkCslGetChannel
     * @sa otLinkCslGetPeriod
     * @sa otLinkCslGetPeriod
     * @sa otLinkCslGetTimeout
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("Channel: %u", otLinkCslGetChannel(GetInstancePtr()));
        OutputLine("Period: %u(in units of 10 symbols), %ums", otLinkCslGetPeriod(GetInstancePtr()),
                   otLinkCslGetPeriod(GetInstancePtr()) * kUsPerTenSymbols / 1000);
        OutputLine("Timeout: %us", otLinkCslGetTimeout(GetInstancePtr()));
    }
    /**
     * @cli csl channel
     * @code
     * csl channel 20
     * Done
     * @endcode
     * @cparam csl channel @ca{channel}
     * @par api_copy
     * #otLinkCslSetChannel
     */
    else if (aArgs[0] == "channel")
    {
        error = ProcessSet(aArgs + 1, otLinkCslSetChannel);
    }
    /**
     * @cli csl period
     * @code
     * csl period 3000
     * Done
     * @endcode
     * @cparam csl period @ca{period}
     * @par api_copy
     * #otLinkCslSetPeriod
     */
    else if (aArgs[0] == "period")
    {
        error = ProcessSet(aArgs + 1, otLinkCslSetPeriod);
    }
    /**
     * @cli csl timeout
     * @code
     * cls timeout 10
     * Done
     * @endcode
     * @cparam csl timeout @ca{timeout}
     * @par api_copy
     * #otLinkCslSetTimeout
     */
    else if (aArgs[0] == "timeout")
    {
        error = ProcessSet(aArgs + 1, otLinkCslSetTimeout);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("delaytimermin")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli delaytimermin
     * @code
     * delaytimermin
     * 30
     * Done
     * @endcode
     * @par
     * Get the minimal delay timer (in seconds).
     * @sa otDatasetGetDelayTimerMinimal
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("%d", (otDatasetGetDelayTimerMinimal(GetInstancePtr()) / 1000));
    }
    /**
     * @cli delaytimermin (set)
     * @code
     * delaytimermin 60
     * Done
     * @endcode
     * @cparam delaytimermin @ca{delaytimermin}
     * @par
     * Sets the minimal delay timer (in seconds).
     * @sa otDatasetSetDelayTimerMinimal
     */
    else if (aArgs[1].IsEmpty())
    {
        uint32_t delay;
        SuccessOrExit(error = aArgs[0].ParseAsUint32(delay));
        SuccessOrExit(error = otDatasetSetDelayTimerMinimal(GetInstancePtr(), static_cast<uint32_t>(delay * 1000)));
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}
#endif

template <> otError Interpreter::Process<Cmd("detach")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "async")
    {
        SuccessOrExit(error = otThreadDetachGracefully(GetInstancePtr(), nullptr, nullptr));
    }
    else
    {
        SuccessOrExit(error =
                          otThreadDetachGracefully(GetInstancePtr(), &Interpreter::HandleDetachGracefullyResult, this));
        error = OT_ERROR_PENDING;
    }

exit:
    return error;
}

/**
 * @cli discover
 * @code
 * discover
 * | J | Network Name     | Extended PAN     | PAN  | MAC Address      | Ch | dBm | LQI |
 * +---+------------------+------------------+------+------------------+----+-----+-----+
 * | 0 | OpenThread       | dead00beef00cafe | ffff | f1d92a82c8d8fe43 | 11 | -20 |   0 |
 * Done
 * @endcode
 * @cparam discover [@ca{channel}]
 * `channel`: The channel to discover on. If no channel is provided, the discovery will cover all
 * valid channels.
 * @par
 * Perform an MLE Discovery operation.
 * @sa otThreadDiscover
 */
template <> otError Interpreter::Process<Cmd("discover")>(Arg aArgs[])
{
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;

    if (!aArgs[0].IsEmpty())
    {
        uint8_t channel;

        SuccessOrExit(error = aArgs[0].ParseAsUint8(channel));
        VerifyOrExit(channel < sizeof(scanChannels) * CHAR_BIT, error = OT_ERROR_INVALID_ARGS);
        scanChannels = 1 << channel;
    }

    SuccessOrExit(error = otThreadDiscover(GetInstancePtr(), scanChannels, OT_PANID_BROADCAST, false, false,
                                           &Interpreter::HandleActiveScanResult, this));

    static const char *const kScanTableTitles[] = {
        "Network Name", "Extended PAN", "PAN", "MAC Address", "Ch", "dBm", "LQI",
    };

    static const uint8_t kScanTableColumnWidths[] = {
        18, 18, 6, 18, 4, 5, 5,
    };

    OutputTableHeader(kScanTableTitles, kScanTableColumnWidths);

    error = OT_ERROR_PENDING;

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("dns")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error = OT_ERROR_NONE;
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    otDnsQueryConfig  queryConfig;
    otDnsQueryConfig *config = &queryConfig;
#endif

    if (aArgs[0].IsEmpty())
    {
        error = OT_ERROR_INVALID_ARGS;
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (aArgs[0] == "compression")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputEnabledDisabledStatus(otDnsIsNameCompressionEnabled());
        }
        else
        {
            bool enable;

            SuccessOrExit(error = ParseEnableOrDisable(aArgs[1], enable));
            otDnsSetNameCompressionEnabled(enable);
        }
    }
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    else if (aArgs[0] == "config")
    {
        if (aArgs[1].IsEmpty())
        {
            const otDnsQueryConfig *defaultConfig = otDnsClientGetDefaultConfig(GetInstancePtr());

            OutputFormat("Server: ");
            OutputSockAddrLine(defaultConfig->mServerSockAddr);
            OutputLine("ResponseTimeout: %u ms", defaultConfig->mResponseTimeout);
            OutputLine("MaxTxAttempts: %u", defaultConfig->mMaxTxAttempts);
            OutputLine("RecursionDesired: %s",
                       (defaultConfig->mRecursionFlag == OT_DNS_FLAG_RECURSION_DESIRED) ? "yes" : "no");
        }
        else
        {
            SuccessOrExit(error = GetDnsConfig(aArgs + 1, config));
            otDnsClientSetDefaultConfig(GetInstancePtr(), config);
        }
    }
    else if (aArgs[0] == "resolve")
    {
        VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = GetDnsConfig(aArgs + 2, config));
        SuccessOrExit(error = otDnsClientResolveAddress(GetInstancePtr(), aArgs[1].GetCString(),
                                                        &Interpreter::HandleDnsAddressResponse, this, config));
        error = OT_ERROR_PENDING;
    }
#if OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE
    else if (aArgs[0] == "resolve4")
    {
        VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = GetDnsConfig(aArgs + 2, config));
        SuccessOrExit(error = otDnsClientResolveIp4Address(GetInstancePtr(), aArgs[1].GetCString(),
                                                           &Interpreter::HandleDnsAddressResponse, this, config));
        error = OT_ERROR_PENDING;
    }
#endif
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    else if (aArgs[0] == "browse")
    {
        VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = GetDnsConfig(aArgs + 2, config));
        SuccessOrExit(error = otDnsClientBrowse(GetInstancePtr(), aArgs[1].GetCString(),
                                                &Interpreter::HandleDnsBrowseResponse, this, config));
        error = OT_ERROR_PENDING;
    }
    else if (aArgs[0] == "service")
    {
        VerifyOrExit(!aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = GetDnsConfig(aArgs + 3, config));
        SuccessOrExit(error = otDnsClientResolveService(GetInstancePtr(), aArgs[1].GetCString(), aArgs[2].GetCString(),
                                                        &Interpreter::HandleDnsServiceResponse, this, config));
        error = OT_ERROR_PENDING;
    }
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
#endif // OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE

otError Interpreter::GetDnsConfig(Arg aArgs[], otDnsQueryConfig *&aConfig)
{
    // This method gets the optional DNS config from `aArgs[]`.
    // The format: `[server IPv6 address] [server port] [timeout]
    // [max tx attempt] [recursion desired]`.

    otError error = OT_ERROR_NONE;
    bool    recursionDesired;

    memset(aConfig, 0, sizeof(otDnsQueryConfig));

    VerifyOrExit(!aArgs[0].IsEmpty(), aConfig = nullptr);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(aConfig->mServerSockAddr.mAddress));

    VerifyOrExit(!aArgs[1].IsEmpty());
    SuccessOrExit(error = aArgs[1].ParseAsUint16(aConfig->mServerSockAddr.mPort));

    VerifyOrExit(!aArgs[2].IsEmpty());
    SuccessOrExit(error = aArgs[2].ParseAsUint32(aConfig->mResponseTimeout));

    VerifyOrExit(!aArgs[3].IsEmpty());
    SuccessOrExit(error = aArgs[3].ParseAsUint8(aConfig->mMaxTxAttempts));

    VerifyOrExit(!aArgs[4].IsEmpty());
    SuccessOrExit(error = aArgs[4].ParseAsBool(recursionDesired));
    aConfig->mRecursionFlag = recursionDesired ? OT_DNS_FLAG_RECURSION_DESIRED : OT_DNS_FLAG_NO_RECURSION;

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
    }

    OutputLine("");
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
const char *EidCacheStateToString(otCacheEntryState aState)
{
    static const char *const kStateStrings[4] = {
        "cache",
        "snoop",
        "query",
        "retry",
    };

    return Interpreter::Stringify(aState, kStateStrings);
}

void Interpreter::OutputEidCacheEntry(const otCacheEntryInfo &aEntry)
{
    OutputIp6Address(aEntry.mTarget);
    OutputFormat(" %04x", aEntry.mRloc16);
    OutputFormat(" %s", EidCacheStateToString(aEntry.mState));
    OutputFormat(" canEvict=%d", aEntry.mCanEvict);

    if (aEntry.mState == OT_CACHE_ENTRY_STATE_CACHED)
    {
        if (aEntry.mValidLastTrans)
        {
            OutputFormat(" transTime=%u eid=", aEntry.mLastTransTime);
            OutputIp6Address(aEntry.mMeshLocalEid);
        }
    }
    else
    {
        OutputFormat(" timeout=%u", aEntry.mTimeout);
    }

    if (aEntry.mState == OT_CACHE_ENTRY_STATE_RETRY_QUERY)
    {
        OutputFormat(" retryDelay=%u", aEntry.mRetryDelay);
    }

    OutputLine("");
}

/**
 * @cli eidcache
 * @code
 * eidcache
 * fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7d 2000 cache canEvict=1 transTime=0 eid=fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7d
 * fd49:caf4:a29f:dc0e:97fc:69dd:3c16:df7f fffe retry canEvict=1 timeout=10 retryDelay=30
 * Done
 * @endcode
 * @par
 * Returns the EID-to-RLOC cache entries.
 * @sa otThreadGetNextCacheEntry
 */
template <> otError Interpreter::Process<Cmd("eidcache")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otCacheEntryIterator iterator;
    otCacheEntryInfo     entry;

    memset(&iterator, 0, sizeof(iterator));

    for (uint8_t i = 0;; i++)
    {
        SuccessOrExit(otThreadGetNextCacheEntry(GetInstancePtr(), &entry, &iterator));
        OutputEidCacheEntry(entry);
    }

exit:
    return OT_ERROR_NONE;
}
#endif

/**
 * @cli eui64
 * @code
 * eui64
 * 0615aae900124b00
 * Done
 * @endcode
 * @par api_copy
 * #otPlatRadioGetIeeeEui64
 */
template <> otError Interpreter::Process<Cmd("eui64")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError      error = OT_ERROR_NONE;
    otExtAddress extAddress;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    otLinkGetFactoryAssignedIeeeEui64(GetInstancePtr(), &extAddress);
    OutputExtAddressLine(extAddress);

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("extaddr")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli extaddr
     * @code
     * extaddr
     * dead00beef00cafe
     * Done
     * @endcode
     * @par api_copy
     * #otLinkGetExtendedAddress
     */
    if (aArgs[0].IsEmpty())
    {
        OutputExtAddressLine(*otLinkGetExtendedAddress(GetInstancePtr()));
    }
    /**
     * @cli extaddr (set)
     * @code
     * extaddr dead00beef00cafe
     * dead00beef00cafe
     * Done
     * @endcode
     * @cparam extaddr @ca{extaddr}
     * @par api_copy
     * #otLinkSetExtendedAddress
     */
    else
    {
        otExtAddress extAddress;

        SuccessOrExit(error = aArgs[0].ParseAsHexString(extAddress.m8));
        error = otLinkSetExtendedAddress(GetInstancePtr(), &extAddress);
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("log")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "level")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputLine("%d", otLoggingGetLevel());
        }
        else
        {
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
            uint8_t level;

            VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = aArgs[1].ParseAsUint8(level));
            error = otLoggingSetLevel(static_cast<otLogLevel>(level));
#else
            error = OT_ERROR_INVALID_ARGS;
#endif
        }
    }
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && OPENTHREAD_POSIX
    else if (aArgs[0] == "filename")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otPlatDebugUart_logfile(aArgs[1].GetCString()));
    }
#endif
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("extpanid")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli extpanid
     * @code
     * extpanid
     * dead00beef00cafe
     * Done
     * @endcode
     * @par api_copy
     * #otThreadGetExtendedPanId
     */
    if (aArgs[0].IsEmpty())
    {
        OutputBytesLine(otThreadGetExtendedPanId(GetInstancePtr())->m8);
    }
    /**
     * @cli extpanid (set)
     * @code
     * extpanid dead00beef00cafe
     * Done
     * @endcode
     * @cparam extpanid @ca{extpanid}
     * @par
     * @note The current commissioning credential becomes stale after changing this value.
     * Use `pskc` to reset.
     * @par api_copy
     * #otThreadSetExtendedPanId
     */
    else
    {
        otExtendedPanId extPanId;

        SuccessOrExit(error = aArgs[0].ParseAsHexString(extPanId.m8));
        error = otThreadSetExtendedPanId(GetInstancePtr(), &extPanId);
    }

exit:
    return error;
}

/**
 * @cli factoryreset
 * @code
 * factoryreset
 * @endcode
 * @par api_copy
 * #otInstanceFactoryReset
 */
template <> otError Interpreter::Process<Cmd("factoryreset")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otInstanceFactoryReset(GetInstancePtr());

    return OT_ERROR_NONE;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
template <> otError Interpreter::Process<Cmd("fake")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    /**
     * @cli fake (a,an)
     * @code
     * fake /a/an fdde:ad00:beef:0:0:ff:fe00:a800 fd00:7d03:7d03:7d03:55f2:bb6a:7a43:a03b 1111222233334444
     * Done
     * @endcode
     * @cparam fake /a/an @ca{dst-ipaddr} @ca{target} @ca{meshLocalIid}
     * @par
     * Sends fake Thread messages.
     * @par
     * Available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     * @sa otThreadSendAddressNotification
     */
    if (aArgs[0] == "/a/an")
    {
        otIp6Address             destination, target;
        otIp6InterfaceIdentifier mlIid;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(destination));
        SuccessOrExit(error = aArgs[2].ParseAsIp6Address(target));
        SuccessOrExit(error = aArgs[3].ParseAsHexString(mlIid.mFields.m8));
        otThreadSendAddressNotification(GetInstancePtr(), &destination, &target, &mlIid);
    }
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    else if (aArgs[0] == "/b/ba")
    {
        otIp6Address             target;
        otIp6InterfaceIdentifier mlIid;
        uint32_t                 timeSinceLastTransaction;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(target));
        SuccessOrExit(error = aArgs[2].ParseAsHexString(mlIid.mFields.m8));
        SuccessOrExit(error = aArgs[3].ParseAsUint32(timeSinceLastTransaction));

        error = otThreadSendProactiveBackboneNotification(GetInstancePtr(), &target, &mlIid, timeSinceLastTransaction);
    }
#endif

exit:
    return error;
}
#endif

template <> otError Interpreter::Process<Cmd("fem")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli fem
     * @code
     * fem
     * LNA gain 11 dBm
     * Done
     * @endcode
     * @par
     * Gets external FEM parameters.
     * @sa otPlatRadioGetFemLnaGain
     */
    if (aArgs[0].IsEmpty())
    {
        int8_t lnaGain;

        SuccessOrExit(error = otPlatRadioGetFemLnaGain(GetInstancePtr(), &lnaGain));
        OutputLine("LNA gain %d dBm", lnaGain);
    }
    /**
     * @cli fem lnagain (get)
     * @code
     * fem lnagain
     * 11
     * Done
     * @endcode
     * @par api_copy
     * #otPlatRadioGetFemLnaGain
     */
    else if (aArgs[0] == "lnagain")
    {
        if (aArgs[1].IsEmpty())
        {
            int8_t lnaGain;

            SuccessOrExit(error = otPlatRadioGetFemLnaGain(GetInstancePtr(), &lnaGain));
            OutputLine("%d", lnaGain);
        }
        /**
         * @cli fem lnagain (set)
         * @code
         * fem lnagain 8
         * Done
         * @endcode
         * @par api_copy
         * #otPlatRadioSetFemLnaGain
         */
        else
        {
            int8_t lnaGain;

            SuccessOrExit(error = aArgs[1].ParseAsInt8(lnaGain));
            SuccessOrExit(error = otPlatRadioSetFemLnaGain(GetInstancePtr(), lnaGain));
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("ifconfig")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        if (otIp6IsEnabled(GetInstancePtr()))
        {
            OutputLine("up");
        }
        else
        {
            OutputLine("down");
        }
    }
    else if (aArgs[0] == "up")
    {
        SuccessOrExit(error = otIp6SetEnabled(GetInstancePtr(), true));
    }
    else if (aArgs[0] == "down")
    {
        SuccessOrExit(error = otIp6SetEnabled(GetInstancePtr(), false));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

const char *Interpreter::AddressOriginToString(uint8_t aOrigin)
{
    static const char *const kOriginStrings[4] = {
        "thread", // 0, OT_ADDRESS_ORIGIN_THREAD
        "slaac",  // 1, OT_ADDRESS_ORIGIN_SLAAC
        "dhcp6",  // 2, OT_ADDRESS_ORIGIN_DHCPV6
        "manual", // 3, OT_ADDRESS_ORIGIN_MANUAL
    };

    static_assert(0 == OT_ADDRESS_ORIGIN_THREAD, "OT_ADDRESS_ORIGIN_THREAD value is incorrect");
    static_assert(1 == OT_ADDRESS_ORIGIN_SLAAC, "OT_ADDRESS_ORIGIN_SLAAC value is incorrect");
    static_assert(2 == OT_ADDRESS_ORIGIN_DHCPV6, "OT_ADDRESS_ORIGIN_DHCPV6 value is incorrect");
    static_assert(3 == OT_ADDRESS_ORIGIN_MANUAL, "OT_ADDRESS_ORIGIN_MANUAL value is incorrect");

    return Stringify(aOrigin, kOriginStrings);
}

template <> otError Interpreter::Process<Cmd("ipaddr")>(Arg aArgs[])
{
    otError error   = OT_ERROR_NONE;
    bool    verbose = false;

    if (aArgs[0] == "-v")
    {
        aArgs++;
        verbose = true;
    }

    if (aArgs[0].IsEmpty())
    {
        const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(GetInstancePtr());

        for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
        {
            OutputIp6Address(addr->mAddress);

            if (verbose)
            {
                OutputFormat(" origin:%s", AddressOriginToString(addr->mAddressOrigin));
            }

            OutputLine("");
        }
    }
    else if (aArgs[0] == "add")
    {
        otNetifAddress address;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address.mAddress));
        address.mPrefixLength  = 64;
        address.mPreferred     = true;
        address.mValid         = true;
        address.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;

        error = otIp6AddUnicastAddress(GetInstancePtr(), &address);
    }
    else if (aArgs[0] == "del")
    {
        otIp6Address address;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address));
        error = otIp6RemoveUnicastAddress(GetInstancePtr(), &address);
    }
    else if (aArgs[0] == "linklocal")
    {
        OutputIp6AddressLine(*otThreadGetLinkLocalIp6Address(GetInstancePtr()));
    }
    else if (aArgs[0] == "rloc")
    {
        OutputIp6AddressLine(*otThreadGetRloc(GetInstancePtr()));
    }
    else if (aArgs[0] == "mleid")
    {
        OutputIp6AddressLine(*otThreadGetMeshLocalEid(GetInstancePtr()));
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("ipmaddr")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        for (const otNetifMulticastAddress *addr = otIp6GetMulticastAddresses(GetInstancePtr()); addr;
             addr                                = addr->mNext)
        {
            OutputIp6AddressLine(addr->mAddress);
        }
    }
    else if (aArgs[0] == "add")
    {
        otIp6Address address;

        aArgs++;

        do
        {
            SuccessOrExit(error = aArgs->ParseAsIp6Address(address));
            SuccessOrExit(error = otIp6SubscribeMulticastAddress(GetInstancePtr(), &address));
        }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        while (!(++aArgs)->IsEmpty());
#else
        while (false);
#endif
    }
    else if (aArgs[0] == "del")
    {
        otIp6Address address;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address));
        error = otIp6UnsubscribeMulticastAddress(GetInstancePtr(), &address);
    }
    else if (aArgs[0] == "promiscuous")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputEnabledDisabledStatus(otIp6IsMulticastPromiscuousEnabled(GetInstancePtr()));
        }
        else
        {
            bool enable;

            SuccessOrExit(error = ParseEnableOrDisable(aArgs[1], enable));
            otIp6SetMulticastPromiscuousEnabled(GetInstancePtr(), enable);
        }
    }
    else if (aArgs[0] == "llatn")
    {
        OutputIp6AddressLine(*otThreadGetLinkLocalAllThreadNodesMulticastAddress(GetInstancePtr()));
    }
    else if (aArgs[0] == "rlatn")
    {
        OutputIp6AddressLine(*otThreadGetRealmLocalAllThreadNodesMulticastAddress(GetInstancePtr()));
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("keysequence")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0] == "counter")
    {
        error = ProcessGetSet(aArgs + 1, otThreadGetKeySequenceCounter, otThreadSetKeySequenceCounter);
    }
    else if (aArgs[0] == "guardtime")
    {
        error = ProcessGetSet(aArgs + 1, otThreadGetKeySwitchGuardTime, otThreadSetKeySwitchGuardTime);
    }

    return error;
}

template <> otError Interpreter::Process<Cmd("leaderdata")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError      error;
    otLeaderData leaderData;

    SuccessOrExit(error = otThreadGetLeaderData(GetInstancePtr(), &leaderData));

    OutputLine("Partition ID: %u", leaderData.mPartitionId);
    OutputLine("Weighting: %d", leaderData.mWeighting);
    OutputLine("Data Version: %d", leaderData.mDataVersion);
    OutputLine("Stable Data Version: %d", leaderData.mStableDataVersion);
    OutputLine("Leader Router ID: %d", leaderData.mLeaderRouterId);

exit:
    return error;
}

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("partitionid")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%u", otThreadGetPartitionId(GetInstancePtr()));
        error = OT_ERROR_NONE;
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (aArgs[0] == "preferred")
    {
        error = ProcessGetSet(aArgs + 1, otThreadGetPreferredLeaderPartitionId, otThreadSetPreferredLeaderPartitionId);
    }
#endif

    return error;
}

template <> otError Interpreter::Process<Cmd("leaderweight")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetLocalLeaderWeight, otThreadSetLocalLeaderWeight);
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Interpreter::HandleLinkMetricsReport(const otIp6Address *       aAddress,
                                          const otLinkMetricsValues *aMetricsValues,
                                          uint8_t                    aStatus,
                                          void *                     aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsReport(aAddress, aMetricsValues, aStatus);
}

void Interpreter::PrintLinkMetricsValue(const otLinkMetricsValues *aMetricsValues)
{
    static const char kLinkMetricsTypeAverage[] = "(Exponential Moving Average)";

    if (aMetricsValues->mMetrics.mPduCount)
    {
        OutputLine(" - PDU Counter: %d (Count/Summation)", aMetricsValues->mPduCountValue);
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
    OutputIp6AddressLine(*aAddress);

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
    else
    {
        OutputLine("Link Metrics Report, status: %s", LinkMetricsStatusToStr(aStatus));
    }

    if (mLinkMetricsQueryInProgress)
    {
        mLinkMetricsQueryInProgress = false;
        OutputResult(OT_ERROR_NONE);
    }
}

void Interpreter::HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleLinkMetricsMgmtResponse(aAddress, aStatus);
}

void Interpreter::HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, uint8_t aStatus)
{
    OutputFormat("Received Link Metrics Management Response from: ");
    OutputIp6AddressLine(*aAddress);

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
    OutputExtAddressLine(*aExtAddress);

    if (aMetricsValues != nullptr)
    {
        PrintLinkMetricsValue(aMetricsValues);
    }
}

const char *Interpreter::LinkMetricsStatusToStr(uint8_t aStatus)
{
    static const char *const kStatusStrings[] = {
        "Success",                      // (0) OT_LINK_METRICS_STATUS_SUCCESS
        "Cannot support new series",    // (1) OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES
        "Series ID already registered", // (2) OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED
        "Series ID not recognized",     // (3) OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED
        "No matching series ID",        // (4) OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED
    };

    const char *str = "Unknown error";

    static_assert(0 == OT_LINK_METRICS_STATUS_SUCCESS, "STATUS_SUCCESS is incorrect");
    static_assert(1 == OT_LINK_METRICS_STATUS_CANNOT_SUPPORT_NEW_SERIES, "CANNOT_SUPPORT_NEW_SERIES is incorrect");
    static_assert(2 == OT_LINK_METRICS_STATUS_SERIESID_ALREADY_REGISTERED, "SERIESID_ALREADY_REGISTERED is incorrect");
    static_assert(3 == OT_LINK_METRICS_STATUS_SERIESID_NOT_RECOGNIZED, "SERIESID_NOT_RECOGNIZED is incorrect");
    static_assert(4 == OT_LINK_METRICS_STATUS_NO_MATCHING_FRAMES_RECEIVED, "NO_MATCHING_FRAMES_RECEIVED is incorrect");

    if (aStatus < OT_ARRAY_LENGTH(kStatusStrings))
    {
        str = kStatusStrings[aStatus];
    }
    else if (aStatus == OT_LINK_METRICS_STATUS_OTHER_ERROR)
    {
        str = "Other error";
    }

    return str;
}

template <> otError Interpreter::Process<Cmd("linkmetrics")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgs[0] == "query")
    {
        otIp6Address  address;
        bool          isSingle;
        bool          blocking;
        uint8_t       seriesId;
        otLinkMetrics linkMetrics;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address));

        if (aArgs[2] == "single")
        {
            isSingle = true;
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[3]));
        }
        else if (aArgs[2] == "forward")
        {
            isSingle = false;
            SuccessOrExit(error = aArgs[3].ParseAsUint8(seriesId));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        blocking = (aArgs[4] == "block");

        SuccessOrExit(error = otLinkMetricsQuery(GetInstancePtr(), &address, isSingle ? 0 : seriesId,
                                                 isSingle ? &linkMetrics : nullptr, HandleLinkMetricsReport, this));

        if (blocking)
        {
            mLinkMetricsQueryInProgress = true;
            error                       = OT_ERROR_PENDING;
        }
    }
    else if (aArgs[0] == "mgmt")
    {
        error = ProcessLinkMetricsMgmt(aArgs + 1);
    }
    else if (aArgs[0] == "probe")
    {
        otIp6Address address;
        uint8_t      seriesId;
        uint8_t      length;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address));
        SuccessOrExit(error = aArgs[2].ParseAsUint8(seriesId));
        SuccessOrExit(error = aArgs[3].ParseAsUint8(length));

        error = otLinkMetricsSendLinkProbe(GetInstancePtr(), &address, seriesId, length);
    }

exit:
    return error;
}

otError Interpreter::ParseLinkMetricsFlags(otLinkMetrics &aLinkMetrics, const Arg &aFlags)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aFlags.IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    memset(&aLinkMetrics, 0, sizeof(aLinkMetrics));

    for (const char *arg = aFlags.GetCString(); *arg != '\0'; arg++)
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

otError Interpreter::ProcessLinkMetricsMgmt(Arg aArgs[])
{
    otError                  error;
    otIp6Address             address;
    otLinkMetricsSeriesFlags seriesFlags;
    bool                     clear = false;

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(address));

    memset(&seriesFlags, 0, sizeof(otLinkMetricsSeriesFlags));

    if (aArgs[1] == "forward")
    {
        uint8_t       seriesId;
        otLinkMetrics linkMetrics;

        memset(&linkMetrics, 0, sizeof(otLinkMetrics));
        SuccessOrExit(error = aArgs[2].ParseAsUint8(seriesId));
        VerifyOrExit(!aArgs[3].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        for (const char *arg = aArgs[3].GetCString(); *arg != '\0'; arg++)
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
                VerifyOrExit(arg == aArgs[3].GetCString() && *(arg + 1) == '\0' && aArgs[4].IsEmpty(),
                             error = OT_ERROR_INVALID_ARGS); // Ensure the flags only contain 'X'
                clear = true;
                break;

            default:
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }

        if (!clear)
        {
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[4]));
            VerifyOrExit(aArgs[5].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        }

        error = otLinkMetricsConfigForwardTrackingSeries(GetInstancePtr(), &address, seriesId, seriesFlags,
                                                         clear ? nullptr : &linkMetrics,
                                                         &Interpreter::HandleLinkMetricsMgmtResponse, this);
    }
    else if (aArgs[1] == "enhanced-ack")
    {
        otLinkMetricsEnhAckFlags enhAckFlags;
        otLinkMetrics            linkMetrics;
        otLinkMetrics *          pLinkMetrics = &linkMetrics;

        if (aArgs[2] == "clear")
        {
            enhAckFlags  = OT_LINK_METRICS_ENH_ACK_CLEAR;
            pLinkMetrics = nullptr;
        }
        else if (aArgs[2] == "register")
        {
            enhAckFlags = OT_LINK_METRICS_ENH_ACK_REGISTER;
            SuccessOrExit(error = ParseLinkMetricsFlags(linkMetrics, aArgs[3]));
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
            if (aArgs[4] == "r")
            {
                linkMetrics.mReserved = true;
            }
#endif
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        error = otLinkMetricsConfigEnhAckProbing(GetInstancePtr(), &address, enhAckFlags, pLinkMetrics,
                                                 &Interpreter::HandleLinkMetricsMgmtResponse, this,
                                                 &Interpreter::HandleLinkMetricsEnhAckProbingIe, this);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE

template <> otError Interpreter::Process<Cmd("locate")>(Arg aArgs[])
{
    otError      error = OT_ERROR_INVALID_ARGS;
    otIp6Address anycastAddress;

    if (aArgs[0].IsEmpty())
    {
        OutputLine(otThreadIsAnycastLocateInProgress(GetInstancePtr()) ? "In Progress" : "Idle");
        ExitNow(error = OT_ERROR_NONE);
    }

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(anycastAddress));
    SuccessOrExit(error =
                      otThreadLocateAnycastDestination(GetInstancePtr(), &anycastAddress, HandleLocateResult, this));
    SetCommandTimeout(kLocateTimeoutMsecs);
    mLocateInProgress = true;
    error             = OT_ERROR_PENDING;

exit:
    return error;
}

void Interpreter::HandleLocateResult(void *              aContext,
                                     otError             aError,
                                     const otIp6Address *aMeshLocalAddress,
                                     uint16_t            aRloc16)
{
    static_cast<Interpreter *>(aContext)->HandleLocateResult(aError, aMeshLocalAddress, aRloc16);
}

void Interpreter::HandleLocateResult(otError aError, const otIp6Address *aMeshLocalAddress, uint16_t aRloc16)
{
    VerifyOrExit(mLocateInProgress);

    mLocateInProgress = false;

    if (aError == OT_ERROR_NONE)
    {
        OutputIp6Address(*aMeshLocalAddress);
        OutputLine(" 0x%04x", aRloc16);
    }

    OutputResult(aError);

exit:
    return;
}

#endif //  OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("pskc")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    otPskc  pskc;

    if (aArgs[0].IsEmpty())
    {
        otThreadGetPskc(GetInstancePtr(), &pskc);
        OutputBytesLine(pskc.m8);
    }
    else
    {
        if (aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[0].ParseAsHexString(pskc.m8));
        }
        else if (aArgs[0] == "-p")
        {
            SuccessOrExit(error = otDatasetGeneratePskc(
                              aArgs[1].GetCString(),
                              reinterpret_cast<const otNetworkName *>(otThreadGetNetworkName(GetInstancePtr())),
                              otThreadGetExtendedPanId(GetInstancePtr()), &pskc));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        error = otThreadSetPskc(GetInstancePtr(), &pskc);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
template <> otError Interpreter::Process<Cmd("pskcref")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("0x%04x", otThreadGetPskcRef(GetInstancePtr()));
    }
    else
    {
        otPskcRef pskcRef;

        if (aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[0].ParseAsUint32(pskcRef));
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        error = otThreadSetPskcRef(GetInstancePtr(), pskcRef);
    }

exit:
    return error;
}
#endif
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
template <> otError Interpreter::Process<Cmd("mliid")>(Arg aArgs[])
{
    otError                  error = OT_ERROR_NONE;
    otIp6InterfaceIdentifier iid;

    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = aArgs[0].ParseAsHexString(iid.mFields.m8));
    SuccessOrExit(error = otIp6SetMeshLocalIid(GetInstancePtr(), &iid));

exit:
    return error;
}
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

template <> otError Interpreter::Process<Cmd("mlr")>(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgs[0] == "reg")
    {
        otIp6Address addresses[OT_IP6_MAX_MLR_ADDRESSES];
        uint32_t     timeout;
        bool         hasTimeout   = false;
        uint8_t      numAddresses = 0;

        aArgs++;

        while (aArgs->ParseAsIp6Address(addresses[numAddresses]) == OT_ERROR_NONE)
        {
            aArgs++;
            numAddresses++;

            if (numAddresses == OT_ARRAY_LENGTH(addresses))
            {
                break;
            }
        }

        if (aArgs->ParseAsUint32(timeout) == OT_ERROR_NONE)
        {
            aArgs++;
            hasTimeout = true;
        }

        VerifyOrExit(aArgs->IsEmpty() && (numAddresses > 0), error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = otIp6RegisterMulticastListeners(GetInstancePtr(), addresses, numAddresses,
                                                              hasTimeout ? &timeout : nullptr,
                                                              Interpreter::HandleMlrRegResult, this));

        error = OT_ERROR_PENDING;
    }

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
            OutputIp6AddressLine(aFailedAddresses[i]);
        }
    }

    OutputResult(aError);
}

#endif // (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

template <> otError Interpreter::Process<Cmd("mode")>(Arg aArgs[])
{
    otError          error = OT_ERROR_NONE;
    otLinkModeConfig linkMode;

    memset(&linkMode, 0, sizeof(otLinkModeConfig));

    if (aArgs[0].IsEmpty())
    {
        char linkModeString[kLinkModeStringSize];

        OutputLine("%s", LinkModeToString(otThreadGetLinkMode(GetInstancePtr()), linkModeString));
        ExitNow();
    }

    if (aArgs[0] != "-")
    {
        for (const char *arg = aArgs[0].GetCString(); *arg != '\0'; arg++)
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

    error = otThreadSetLinkMode(GetInstancePtr(), linkMode);

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("multiradio")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aArgs);

    if (aArgs[0].IsEmpty())
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
    else if (aArgs[0] == "neighbor")
    {
        otMultiRadioNeighborInfo multiRadioInfo;

        if (aArgs[1] == "list")
        {
            otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
            otNeighborInfo         neighInfo;

            while (otThreadGetNextNeighborInfo(GetInstancePtr(), &iterator, &neighInfo) == OT_ERROR_NONE)
            {
                if (otMultiRadioGetNeighborInfo(GetInstancePtr(), &neighInfo.mExtAddress, &multiRadioInfo) !=
                    OT_ERROR_NONE)
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

            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddress.m8));
            SuccessOrExit(error = otMultiRadioGetNeighborInfo(GetInstancePtr(), &extAddress, &multiRadioInfo));
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
template <> otError Interpreter::Process<Cmd("neighbor")>(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otNeighborInfo         neighborInfo;
    bool                   isTable;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    isTable = (aArgs[0] == "table");

    if (isTable || (aArgs[0] == "list"))
    {
        if (isTable)
        {
            static const char *const kNeighborTableTitles[] = {
                "Role", "RLOC16", "Age", "Avg RSSI", "Last RSSI", "R", "D", "N", "Extended MAC",
            };

            static const uint8_t kNeighborTableColumnWidths[] = {
                6, 8, 5, 10, 11, 1, 1, 1, 18,
            };

            OutputTableHeader(kNeighborTableTitles, kNeighborTableColumnWidths);
        }

        while (otThreadGetNextNeighborInfo(GetInstancePtr(), &iterator, &neighborInfo) == OT_ERROR_NONE)
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
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}
#endif // OPENTHREAD_FTD

template <> otError Interpreter::Process<Cmd("netstat")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    static const char *const kNetstatTableTitles[]       = {"Local Address", "Peer Address"};
    static const uint8_t     kNetstatTableColumnWidths[] = {49, 49};

    char string[OT_IP6_SOCK_ADDR_STRING_SIZE];

    OutputTableHeader(kNetstatTableTitles, kNetstatTableColumnWidths);

    for (const otUdpSocket *socket = otUdpGetSockets(GetInstancePtr()); socket != nullptr; socket = socket->mNext)
    {
        otIp6SockAddrToString(&socket->mSockName, string, sizeof(string));
        OutputFormat("| %-47s ", string);
        otIp6SockAddrToString(&socket->mPeerName, string, sizeof(string));
        OutputLine("| %-47s |", string);
    }

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
template <> otError Interpreter::Process<Cmd("service")>(Arg aArgs[])
{
    otError         error = OT_ERROR_INVALID_COMMAND;
    otServiceConfig cfg;

    if (aArgs[0].IsEmpty())
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otServiceConfig       config;

        while (otServerGetNextService(GetInstancePtr(), &iterator, &config) == OT_ERROR_NONE)
        {
            mNetworkData.OutputService(config);
        }

        error = OT_ERROR_NONE;
    }
    else
    {
        uint16_t length;

        SuccessOrExit(error = aArgs[1].ParseAsUint32(cfg.mEnterpriseNumber));

        length = sizeof(cfg.mServiceData);
        SuccessOrExit(error = aArgs[2].ParseAsHexString(length, cfg.mServiceData));
        VerifyOrExit(length > 0, error = OT_ERROR_INVALID_ARGS);
        cfg.mServiceDataLength = static_cast<uint8_t>(length);

        if (aArgs[0] == "add")
        {
            length = sizeof(cfg.mServerConfig.mServerData);
            SuccessOrExit(error = aArgs[3].ParseAsHexString(length, cfg.mServerConfig.mServerData));
            VerifyOrExit(length > 0, error = OT_ERROR_INVALID_ARGS);
            cfg.mServerConfig.mServerDataLength = static_cast<uint8_t>(length);

            cfg.mServerConfig.mStable = true;

            error = otServerAddService(GetInstancePtr(), &cfg);
        }
        else if (aArgs[0] == "remove")
        {
            error = otServerRemoveService(GetInstancePtr(), cfg.mEnterpriseNumber, cfg.mServiceData,
                                          cfg.mServiceDataLength);
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

template <> otError Interpreter::Process<Cmd("netdata")>(Arg aArgs[])
{
    return mNetworkData.Process(aArgs);
}

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("networkidtimeout")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetNetworkIdTimeout, otThreadSetNetworkIdTimeout);
}
#endif

template <> otError Interpreter::Process<Cmd("networkkey")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        otNetworkKey networkKey;

        otThreadGetNetworkKey(GetInstancePtr(), &networkKey);
        OutputBytesLine(networkKey.m8);
    }
    else
    {
        otNetworkKey key;

        SuccessOrExit(error = aArgs[0].ParseAsHexString(key.m8));
        SuccessOrExit(error = otThreadSetNetworkKey(GetInstancePtr(), &key));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
template <> otError Interpreter::Process<Cmd("networkkeyref")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("0x%04x", otThreadGetNetworkKeyRef(GetInstancePtr()));
    }
    else
    {
        otNetworkKeyRef keyRef;

        SuccessOrExit(error = aArgs[0].ParseAsUint32(keyRef));
        SuccessOrExit(error = otThreadSetNetworkKeyRef(GetInstancePtr(), keyRef));
    }

exit:
    return error;
}
#endif

template <> otError Interpreter::Process<Cmd("networkname")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otThreadGetNetworkName(GetInstancePtr()));
    }
    else
    {
        SuccessOrExit(error = otThreadSetNetworkName(GetInstancePtr(), aArgs[0].GetCString()));
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
template <> otError Interpreter::Process<Cmd("networktime")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli networktime
     * @code
     * networktime
     * Network Time:     21084154us (synchronized)
     * Time Sync Period: 100s
     * XTAL Threshold:   300ppm
     * Done
     * @endcode
     * @par
     * Gets the Thread network time and the time sync parameters.
     * @sa otNetworkTimeGet
     * @sa otNetworkTimeGetSyncPeriod
     * @sa otNetworkTimeGetXtalThreshold
     */
    if (aArgs[0].IsEmpty())
    {
        uint64_t            time;
        otNetworkTimeStatus networkTimeStatus;

        networkTimeStatus = otNetworkTimeGet(GetInstancePtr(), &time);

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

        OutputLine("Time Sync Period: %ds", otNetworkTimeGetSyncPeriod(GetInstancePtr()));
        OutputLine("XTAL Threshold:   %dppm", otNetworkTimeGetXtalThreshold(GetInstancePtr()));
    }
    /**
     * @cli networktime (set)
     * @code
     * networktime 100 300
     * Done
     * @endcode
     * @cparam networktime @ca{timesyncperiod} @ca{xtalthreshold}
     * @par
     * Sets the time sync parameters.
     * *   `timesyncperiod`: The time synchronization period, in seconds.
     * *   `xtalthreshold`: The XTAL accuracy threshold for a device to become Router-Capable device, in PPM.
     * @sa otNetworkTimeSetSyncPeriod
     * @sa otNetworkTimeSetXtalThreshold
     */
    else
    {
        uint16_t period;
        uint16_t xtalThreshold;

        SuccessOrExit(error = aArgs[0].ParseAsUint16(period));
        SuccessOrExit(error = aArgs[1].ParseAsUint16(xtalThreshold));
        SuccessOrExit(error = otNetworkTimeSetSyncPeriod(GetInstancePtr(), period));
        SuccessOrExit(error = otNetworkTimeSetXtalThreshold(GetInstancePtr(), xtalThreshold));
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

template <> otError Interpreter::Process<Cmd("panid")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("0x%04x", otLinkGetPanId(GetInstancePtr()));
    }
    else
    {
        error = ProcessSet(aArgs, otLinkSetPanId);
    }

    return error;
}

template <> otError Interpreter::Process<Cmd("parent")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        otRouterInfo parentInfo;

        SuccessOrExit(error = otThreadGetParentInfo(GetInstancePtr(), &parentInfo));
        OutputFormat("Ext Addr: ");
        OutputExtAddressLine(parentInfo.mExtAddress);
        OutputLine("Rloc: %x", parentInfo.mRloc16);
        OutputLine("Link Quality In: %d", parentInfo.mLinkQualityIn);
        OutputLine("Link Quality Out: %d", parentInfo.mLinkQualityOut);
        OutputLine("Age: %d", parentInfo.mAge);
        OutputLine("Version: %d", parentInfo.mVersion);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        OutputLine("CSL clock accuracy: %d", parentInfo.mCslClockAccuracy);
        OutputLine("CSL uncertainty: %d", parentInfo.mCslUncertainty);
#endif
    }
    /**
     * @cli parent search
     * @code
     * parent search
     * Done
     * @endcode
     * @par api_copy
     * #otThreadSearchForBetterParent
     */
    else if (aArgs[0] == "search")
    {
        error = otThreadSearchForBetterParent(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("parentpriority")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetParentPriority, otThreadSetParentPriority);
}
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
template <> otError Interpreter::Process<Cmd("routeridrange")>(Arg *aArgs)
{
    uint8_t minRouterId;
    uint8_t maxRouterId;
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        otThreadGetRouterIdRange(GetInstancePtr(), &minRouterId, &maxRouterId);
        OutputLine("%d %d", minRouterId, maxRouterId);
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsUint8(minRouterId));
        SuccessOrExit(error = aArgs[1].ParseAsUint8(maxRouterId));
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        error = otThreadSetRouterIdRange(GetInstancePtr(), minRouterId, maxRouterId);
    }

exit:
    return error;
}
#endif

#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE

void Interpreter::HandlePingReply(const otPingSenderReply *aReply, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePingReply(aReply);
}

void Interpreter::HandlePingReply(const otPingSenderReply *aReply)
{
    OutputFormat("%u bytes from ", static_cast<uint16_t>(aReply->mSize + sizeof(otIcmp6Header)));
    OutputIp6Address(aReply->mSenderAddress);
    OutputLine(": icmp_seq=%d hlim=%d time=%dms", aReply->mSequenceNumber, aReply->mHopLimit, aReply->mRoundTripTime);
}

void Interpreter::HandlePingStatistics(const otPingSenderStatistics *aStatistics, void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandlePingStatistics(aStatistics);
}

void Interpreter::HandlePingStatistics(const otPingSenderStatistics *aStatistics)
{
    OutputFormat("%u packets transmitted, %u packets received.", aStatistics->mSentCount, aStatistics->mReceivedCount);

    if ((aStatistics->mSentCount != 0) && !aStatistics->mIsMulticast &&
        aStatistics->mReceivedCount <= aStatistics->mSentCount)
    {
        uint32_t packetLossRate =
            1000 * (aStatistics->mSentCount - aStatistics->mReceivedCount) / aStatistics->mSentCount;
        OutputFormat(" Packet loss = %u.%u%%.", packetLossRate / 10, packetLossRate % 10);
    }

    if (aStatistics->mReceivedCount != 0)
    {
        uint32_t avgRoundTripTime = 1000 * aStatistics->mTotalRoundTripTime / aStatistics->mReceivedCount;
        OutputFormat(" Round-trip min/avg/max = %u/%u.%u/%u ms.", aStatistics->mMinRoundTripTime,
                     avgRoundTripTime / 1000, avgRoundTripTime % 1000, aStatistics->mMaxRoundTripTime);
    }

    OutputLine("");

    if (!mPingIsAsync)
    {
        OutputResult(OT_ERROR_NONE);
    }
}

template <> otError Interpreter::Process<Cmd("ping")>(Arg aArgs[])
{
    otError            error = OT_ERROR_NONE;
    otPingSenderConfig config;
    bool               async = false;
    bool               nat64ConvertedAddress;

    if (aArgs[0] == "stop")
    {
        otPingSenderStop(GetInstancePtr());
        ExitNow();
    }
    else if (aArgs[0] == "async")
    {
        async = true;
        aArgs++;
    }

    memset(&config, 0, sizeof(config));

    if (aArgs[0] == "-I")
    {
        SuccessOrExit(error = aArgs[1].ParseAsIp6Address(config.mSource));

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        {
            bool                  valid        = false;
            const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(GetInstancePtr());

            for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext)
            {
                if (otIp6IsAddressEqual(&addr->mAddress, &config.mSource))
                {
                    valid = true;
                    break;
                }
            }

            VerifyOrExit(valid, error = OT_ERROR_INVALID_ARGS);
        }
#endif

        aArgs += 2;
    }

    SuccessOrExit(
        error = ParseAndConvertToIp6Address(GetInstancePtr(), aArgs[0], config.mDestination, nat64ConvertedAddress));
    if (nat64ConvertedAddress)
    {
        OutputFormat("Pinging IPv4-converted IPv6 address: ");
        OutputIp6AddressLine(config.mDestination);
    }

    if (!aArgs[1].IsEmpty())
    {
        SuccessOrExit(error = aArgs[1].ParseAsUint16(config.mSize));
    }

    if (!aArgs[2].IsEmpty())
    {
        SuccessOrExit(error = aArgs[2].ParseAsUint16(config.mCount));
    }

    if (!aArgs[3].IsEmpty())
    {
        SuccessOrExit(error = ParsePingInterval(aArgs[3], config.mInterval));
    }

    if (!aArgs[4].IsEmpty())
    {
        SuccessOrExit(error = aArgs[4].ParseAsUint8(config.mHopLimit));
        config.mAllowZeroHopLimit = (config.mHopLimit == 0);
    }

    if (!aArgs[5].IsEmpty())
    {
        uint32_t timeout;

        SuccessOrExit(error = ParsePingInterval(aArgs[5], timeout));
        VerifyOrExit(timeout <= NumericLimits<uint16_t>::kMax, error = OT_ERROR_INVALID_ARGS);
        config.mTimeout = static_cast<uint16_t>(timeout);
    }

    VerifyOrExit(aArgs[6].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    config.mReplyCallback      = Interpreter::HandlePingReply;
    config.mStatisticsCallback = Interpreter::HandlePingStatistics;
    config.mCallbackContext    = this;

    SuccessOrExit(error = otPingSenderPing(GetInstancePtr(), &config));

    mPingIsAsync = async;

    if (!async)
    {
        error = OT_ERROR_PENDING;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_PING_SENDER_ENABLE

template <> otError Interpreter::Process<Cmd("pollperiod")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otLinkGetPollPeriod, otLinkSetPollPeriod);
}

template <> otError Interpreter::Process<Cmd("promiscuous")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otLinkIsPromiscuous(GetInstancePtr()) &&
                                    otPlatRadioGetPromiscuous(GetInstancePtr()));
    }
    else
    {
        bool enable;

        SuccessOrExit(error = ParseEnableOrDisable(aArgs[0], enable));

        if (!enable)
        {
            otLinkSetPcapCallback(GetInstancePtr(), nullptr, nullptr);
        }

        SuccessOrExit(error = otLinkSetPromiscuous(GetInstancePtr(), enable));

        if (enable)
        {
            otLinkSetPcapCallback(GetInstancePtr(), &HandleLinkPcapReceive, this);
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
otError Interpreter::ParsePrefix(Arg aArgs[], otBorderRouterConfig &aConfig)
{
    otError error = OT_ERROR_NONE;

    memset(&aConfig, 0, sizeof(otBorderRouterConfig));

    SuccessOrExit(error = aArgs[0].ParseAsIp6Prefix(aConfig.mPrefix));
    aArgs++;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        otRoutePreference preference;

        if (ParsePreference(*aArgs, preference) == OT_ERROR_NONE)
        {
            aConfig.mPreference = preference;
        }
        else
        {
            for (char *arg = aArgs->GetCString(); *arg != '\0'; arg++)
            {
                switch (*arg)
                {
                case 'p':
                    aConfig.mPreferred = true;
                    break;

                case 'a':
                    aConfig.mSlaac = true;
                    break;

                case 'd':
                    aConfig.mDhcp = true;
                    break;

                case 'c':
                    aConfig.mConfigure = true;
                    break;

                case 'r':
                    aConfig.mDefaultRoute = true;
                    break;

                case 'o':
                    aConfig.mOnMesh = true;
                    break;

                case 's':
                    aConfig.mStable = true;
                    break;

                case 'n':
                    aConfig.mNdDns = true;
                    break;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
                case 'D':
                    aConfig.mDp = true;
                    break;
#endif
                default:
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }
        }
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("prefix")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli prefix
     * @code
     * prefix
     * 2001:dead:beef:cafe::/64 paros med
     * - fd00:7d03:7d03:7d03::/64 prosD med
     * Done
     * @endcode
     * @par
     * Get the prefix list in the local Network Data.
     * @note For the Thread 1.2 border router with backbone capability, the local Domain Prefix
     * is listed as well and includes the `D` flag. If backbone functionality is disabled, a dash
     * `-` is printed before the local Domain Prefix.
     * @par
     * For more information about #otBorderRouterConfig flags, refer to @overview.
     * @sa otBorderRouterGetNextOnMeshPrefix
     */
    if (aArgs[0].IsEmpty())
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otBorderRouterConfig  config;

        while (otBorderRouterGetNextOnMeshPrefix(GetInstancePtr(), &iterator, &config) == OT_ERROR_NONE)
        {
            mNetworkData.OutputPrefix(config);
        }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        if (otBackboneRouterGetState(GetInstancePtr()) == OT_BACKBONE_ROUTER_STATE_DISABLED)
        {
            SuccessOrExit(otBackboneRouterGetDomainPrefix(GetInstancePtr(), &config));
            OutputFormat("- ");
            mNetworkData.OutputPrefix(config);
        }
#endif
    }
    /**
     * @cli prefix add
     * @code
     * prefix add 2001:dead:beef:cafe::/64 paros med
     * Done
     * @endcode
     * @code
     * prefix add fd00:7d03:7d03:7d03::/64 prosD low
     * Done
     * @endcode
     * @cparam prefix add @ca{prefix} [@ca{padcrosnD}] [@ca{high}|@ca{med}|@ca{low}]
     * OT CLI uses mapped arguments to configure #otBorderRouterConfig values. @moreinfo{the @overview}.
     * @par
     * Adds a valid prefix to the Network Data.
     * @sa otBorderRouterAddOnMeshPrefix
     */
    else if (aArgs[0] == "add")
    {
        otBorderRouterConfig config;

        SuccessOrExit(error = ParsePrefix(aArgs + 1, config));
        error = otBorderRouterAddOnMeshPrefix(GetInstancePtr(), &config);
    }
    /**
     * @cli prefix remove
     * @code
     * prefix remove 2001:dead:beef:cafe::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRouterRemoveOnMeshPrefix
     */
    else if (aArgs[0] == "remove")
    {
        otIp6Prefix prefix;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Prefix(prefix));
        error = otBorderRouterRemoveOnMeshPrefix(GetInstancePtr(), &prefix);
    }
    /**
     * @cli prefix meshlocal
     * @code
     * prefix meshlocal
     * fdde:ad00:beef:0::/64
     * Done
     * @endcode
     * @par
     * Get the mesh local prefix.
     */
    else if (aArgs[0] == "meshlocal")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputIp6PrefixLine(*otThreadGetMeshLocalPrefix(GetInstancePtr()));
        }
        else
        {
            otIp6Prefix prefix;

            SuccessOrExit(error = aArgs[1].ParseAsIp6Prefix(prefix));
            VerifyOrExit(prefix.mLength == OT_IP6_PREFIX_BITSIZE, error = OT_ERROR_INVALID_ARGS);
            error =
                otThreadSetMeshLocalPrefix(GetInstancePtr(), reinterpret_cast<otMeshLocalPrefix *>(&prefix.mPrefix));
        }
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("preferrouterid")>(Arg aArgs[])
{
    return ProcessSet(aArgs, otThreadSetPreferredRouterId);
}
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
template <> otError Interpreter::Process<Cmd("radiofilter")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otLinkIsRadioFilterEnabled(GetInstancePtr()));
    }
    else
    {
        bool enable;

        SuccessOrExit(error = ParseEnableOrDisable(aArgs[0], enable));
        otLinkSetRadioFilterEnabled(GetInstancePtr(), enable);
    }

exit:
    return error;
}
#endif

template <> otError Interpreter::Process<Cmd("rcp")>(Arg aArgs[])
{
    otError     error   = OT_ERROR_NONE;
    const char *version = otPlatRadioGetVersionString(GetInstancePtr());

    VerifyOrExit(version != otGetVersionString(), error = OT_ERROR_NOT_IMPLEMENTED);

    if (aArgs[0] == "version")
    {
        OutputLine("%s", version);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("region")>(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint16_t regionCode;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otPlatRadioGetRegion(GetInstancePtr(), &regionCode));
        OutputLine("%c%c", regionCode >> 8, regionCode & 0xff);
    }
    else
    {
        VerifyOrExit(aArgs[0].GetLength() == 2, error = OT_ERROR_INVALID_ARGS);

        regionCode = static_cast<uint16_t>(static_cast<uint16_t>(aArgs[0].GetCString()[0]) << 8) +
                     static_cast<uint16_t>(aArgs[0].GetCString()[1]);
        error = otPlatRadioSetRegion(GetInstancePtr(), regionCode);
    }

exit:
    return error;
}

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("releaserouterid")>(Arg aArgs[])
{
    return ProcessSet(aArgs, otThreadReleaseRouterId);
}
#endif

template <> otError Interpreter::Process<Cmd("rloc16")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine("%04x", otThreadGetRloc16(GetInstancePtr()));

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
otError Interpreter::ParseRoute(Arg aArgs[], otExternalRouteConfig &aConfig)
{
    otError error = OT_ERROR_NONE;

    memset(&aConfig, 0, sizeof(otExternalRouteConfig));

    SuccessOrExit(error = aArgs[0].ParseAsIp6Prefix(aConfig.mPrefix));
    aArgs++;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        otRoutePreference preference;

        if (*aArgs == "s")
        {
            aConfig.mStable = true;
        }
        else if (*aArgs == "n")
        {
            aConfig.mNat64 = true;
        }
        else if (ParsePreference(*aArgs, preference) == OT_ERROR_NONE)
        {
            aConfig.mPreference = preference;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("route")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
        otExternalRouteConfig config;

        while (otBorderRouterGetNextRoute(GetInstancePtr(), &iterator, &config) == OT_ERROR_NONE)
        {
            mNetworkData.OutputRoute(config);
        }
    }
    else if (aArgs[0] == "add")
    {
        otExternalRouteConfig config;

        SuccessOrExit(error = ParseRoute(aArgs + 1, config));
        error = otBorderRouterAddRoute(GetInstancePtr(), &config);
    }
    else if (aArgs[0] == "remove")
    {
        otIp6Prefix prefix;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Prefix(prefix));
        error = otBorderRouterRemoveRoute(GetInstancePtr(), &prefix);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("router")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otRouterInfo routerInfo;
    uint16_t     routerId;
    bool         isTable;

    isTable = (aArgs[0] == "table");

    if (isTable || (aArgs[0] == "list"))
    {
        uint8_t maxRouterId;

        if (isTable)
        {
            static const char *const kRouterTableTitles[] = {
                "ID", "RLOC16", "Next Hop", "Path Cost", "LQ In", "LQ Out", "Age", "Extended MAC", "Link",
            };

            static const uint8_t kRouterTableColumnWidths[] = {
                4, 8, 10, 11, 7, 8, 5, 18, 6,
            };

            OutputTableHeader(kRouterTableTitles, kRouterTableColumnWidths);
        }

        maxRouterId = otThreadGetMaxRouterId(GetInstancePtr());

        for (uint8_t i = 0; i <= maxRouterId; i++)
        {
            if (otThreadGetRouterInfo(GetInstancePtr(), i, &routerInfo) != OT_ERROR_NONE)
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

    SuccessOrExit(error = aArgs[0].ParseAsUint16(routerId));
    SuccessOrExit(error = otThreadGetRouterInfo(GetInstancePtr(), routerId, &routerInfo));

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
            OutputExtAddressLine(routerInfo.mExtAddress);
            OutputLine("Cost: %d", routerInfo.mPathCost);
            OutputLine("Link Quality In: %d", routerInfo.mLinkQualityIn);
            OutputLine("Link Quality Out: %d", routerInfo.mLinkQualityOut);
            OutputLine("Age: %d", routerInfo.mAge);
        }
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("routerdowngradethreshold")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetRouterDowngradeThreshold, otThreadSetRouterDowngradeThreshold);
}

template <> otError Interpreter::Process<Cmd("routereligible")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otThreadIsRouterEligible(GetInstancePtr()));
    }
    else
    {
        bool enable;

        SuccessOrExit(error = ParseEnableOrDisable(aArgs[0], enable));
        error = otThreadSetRouterEligible(GetInstancePtr(), enable);
    }

exit:
    return error;
}

template <> otError Interpreter::Process<Cmd("routerselectionjitter")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetRouterSelectionJitter, otThreadSetRouterSelectionJitter);
}

template <> otError Interpreter::Process<Cmd("routerupgradethreshold")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetRouterUpgradeThreshold, otThreadSetRouterUpgradeThreshold);
}
#endif // OPENTHREAD_FTD

template <> otError Interpreter::Process<Cmd("scan")>(Arg aArgs[])
{
    otError  error        = OT_ERROR_NONE;
    uint32_t scanChannels = 0;
    uint16_t scanDuration = 0;
    bool     energyScan   = false;

    if (aArgs[0] == "energy")
    {
        energyScan = true;
        aArgs++;

        if (!aArgs->IsEmpty())
        {
            SuccessOrExit(error = aArgs->ParseAsUint16(scanDuration));
            aArgs++;
        }
    }

    if (!aArgs->IsEmpty())
    {
        uint8_t channel;

        SuccessOrExit(error = aArgs->ParseAsUint8(channel));
        VerifyOrExit(channel < sizeof(scanChannels) * CHAR_BIT, error = OT_ERROR_INVALID_ARGS);
        scanChannels = 1 << channel;
    }

    if (energyScan)
    {
        static const char *const kEnergyScanTableTitles[]       = {"Ch", "RSSI"};
        static const uint8_t     kEnergyScanTableColumnWidths[] = {4, 6};

        OutputTableHeader(kEnergyScanTableTitles, kEnergyScanTableColumnWidths);
        SuccessOrExit(error = otLinkEnergyScan(GetInstancePtr(), scanChannels, scanDuration,
                                               &Interpreter::HandleEnergyScanResult, this));
    }
    else
    {
        static const char *const kScanTableTitles[]       = {"PAN", "MAC Address", "Ch", "dBm", "LQI"};
        static const uint8_t     kScanTableColumnWidths[] = {6, 18, 4, 5, 5};

        OutputTableHeader(kScanTableTitles, kScanTableColumnWidths);

        SuccessOrExit(error = otLinkActiveScan(GetInstancePtr(), scanChannels, scanDuration,
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

    if (aResult->mDiscover)
    {
        OutputFormat("| %-16s ", aResult->mNetworkName.m8);

        OutputFormat("| ");
        OutputBytes(aResult->mExtendedPanId.m8);
        OutputFormat(" ");
    }

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

template <> otError Interpreter::Process<Cmd("singleton")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    OutputLine(otThreadIsSingleton(GetInstancePtr()) ? "true" : "false");

    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
template <> otError Interpreter::Process<Cmd("sntp")>(Arg aArgs[])
{
    otError          error = OT_ERROR_NONE;
    uint16_t         port  = OT_SNTP_DEFAULT_SERVER_PORT;
    Ip6::MessageInfo messageInfo;
    otSntpQuery      query;

    if (aArgs[0] == "query")
    {
        VerifyOrExit(!mSntpQueryingInProgress, error = OT_ERROR_BUSY);

        if (!aArgs[1].IsEmpty())
        {
            SuccessOrExit(error = aArgs[1].ParseAsIp6Address(messageInfo.GetPeerAddr()));
        }
        else
        {
            // Use IPv6 address of default SNTP server.
            SuccessOrExit(error = messageInfo.GetPeerAddr().FromString(OT_SNTP_DEFAULT_SERVER_IP));
        }

        if (!aArgs[2].IsEmpty())
        {
            SuccessOrExit(error = aArgs[2].ParseAsUint16(port));
        }

        messageInfo.SetPeerPort(port);

        query.mMessageInfo = static_cast<const otMessageInfo *>(&messageInfo);

        SuccessOrExit(error = otSntpClientQuery(GetInstancePtr(), &query, &Interpreter::HandleSntpResponse, this));

        mSntpQueryingInProgress = true;
        error                   = OT_ERROR_PENDING;
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

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
template <> otError Interpreter::Process<Cmd("srp")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
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
    if (aArgs[0] == "client")
    {
        ExitNow(error = mSrpClient.Process(aArgs + 1));
    }
#endif
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    if (aArgs[0] == "server")
    {
        ExitNow(error = mSrpServer.Process(aArgs + 1));
    }
#endif

    error = OT_ERROR_INVALID_COMMAND;

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

template <> otError Interpreter::Process<Cmd("state")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", otThreadDeviceRoleToString(otThreadGetDeviceRole(GetInstancePtr())));
    }
    else if (aArgs[0] == "detached")
    {
        error = otThreadBecomeDetached(GetInstancePtr());
    }
    else if (aArgs[0] == "child")
    {
        error = otThreadBecomeChild(GetInstancePtr());
    }
#if OPENTHREAD_FTD
    else if (aArgs[0] == "router")
    {
        error = otThreadBecomeRouter(GetInstancePtr());
    }
    else if (aArgs[0] == "leader")
    {
        error = otThreadBecomeLeader(GetInstancePtr());
    }
#endif
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

template <> otError Interpreter::Process<Cmd("thread")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "start")
    {
        error = otThreadSetEnabled(GetInstancePtr(), true);
    }
    else if (aArgs[0] == "stop")
    {
        error = otThreadSetEnabled(GetInstancePtr(), false);
    }
    else if (aArgs[0] == "version")
    {
        OutputLine("%u", otThreadGetVersion());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

template <> otError Interpreter::Process<Cmd("dataset")>(Arg aArgs[])
{
    return mDataset.Process(aArgs);
}

template <> otError Interpreter::Process<Cmd("txpower")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    int8_t  power;

    if (aArgs[0].IsEmpty())
    {
        SuccessOrExit(error = otPlatRadioGetTransmitPower(GetInstancePtr(), &power));
        OutputLine("%d dBm", power);
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsInt8(power));
        error = otPlatRadioSetTransmitPower(GetInstancePtr(), power);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
template <> otError Interpreter::Process<Cmd("tcp")>(Arg aArgs[])
{
    return mTcp.Process(aArgs);
}
#endif

template <> otError Interpreter::Process<Cmd("udp")>(Arg aArgs[])
{
    return mUdp.Process(aArgs);
}

template <> otError Interpreter::Process<Cmd("unsecureport")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "add")
    {
        error = ProcessSet(aArgs + 1, otIp6AddUnsecurePort);
    }
    else if (aArgs[0] == "remove")
    {
        if (aArgs[1] == "all")
        {
            otIp6RemoveAllUnsecurePorts(GetInstancePtr());
        }
        else
        {
            error = ProcessSet(aArgs + 1, otIp6RemoveUnsecurePort);
        }
    }
    else if (aArgs[0] == "get")
    {
        const uint16_t *ports;
        uint8_t         numPorts;

        ports = otIp6GetUnsecurePorts(GetInstancePtr(), &numPorts);

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
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
template <> otError Interpreter::Process<Cmd("uptime")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        char string[OT_UPTIME_STRING_SIZE];

        otInstanceGetUptimeAsString(GetInstancePtr(), string, sizeof(string));
        OutputLine("%s", string);
    }
    else if (aArgs[0] == "ms")
    {
        OutputLine("%lu", otInstanceGetUptime(GetInstancePtr()));
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("commissioner")>(Arg aArgs[])
{
    return mCommissioner.Process(aArgs);
}
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
template <> otError Interpreter::Process<Cmd("joiner")>(Arg aArgs[])
{
    return mJoiner.Process(aArgs);
}
#endif

#if OPENTHREAD_FTD
template <> otError Interpreter::Process<Cmd("joinerport")>(Arg aArgs[])
{
    return ProcessGetSet(aArgs, otThreadGetJoinerUdpPort, otThreadSetJoinerUdpPort);
}
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
template <> otError Interpreter::Process<Cmd("macfilter")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        PrintMacFilter();
    }
    else if (aArgs[0] == "addr")
    {
        error = ProcessMacFilterAddress(aArgs + 1);
    }
    else if (aArgs[0] == "rss")
    {
        error = ProcessMacFilterRss(aArgs + 1);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

void Interpreter::PrintMacFilter(void)
{
    otMacFilterEntry    entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;

    OutputLine("Address Mode: %s", MacFilterAddressModeToString(otLinkFilterGetAddressMode(GetInstancePtr())));

    while (otLinkFilterGetNextAddress(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
    {
        OutputMacFilterEntry(entry);
    }

    iterator = OT_MAC_FILTER_ITERATOR_INIT;
    OutputLine("RssIn List:");

    while (otLinkFilterGetNextRssIn(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
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
                       otLinkConvertRssToLinkQuality(GetInstancePtr(), entry.mRssIn));
        }
        else
        {
            OutputMacFilterEntry(entry);
        }
    }
}

otError Interpreter::ProcessMacFilterAddress(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otExtAddress extAddr;

    if (aArgs[0].IsEmpty())
    {
        otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
        otMacFilterEntry    entry;

        OutputLine("%s", MacFilterAddressModeToString(otLinkFilterGetAddressMode(GetInstancePtr())));

        while (otLinkFilterGetNextAddress(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
        {
            OutputMacFilterEntry(entry);
        }
    }
    else if (aArgs[0] == "disable")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        otLinkFilterSetAddressMode(GetInstancePtr(), OT_MAC_FILTER_ADDRESS_MODE_DISABLED);
    }
    else if (aArgs[0] == "allowlist")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        otLinkFilterSetAddressMode(GetInstancePtr(), OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST);
    }
    else if (aArgs[0] == "denylist")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        otLinkFilterSetAddressMode(GetInstancePtr(), OT_MAC_FILTER_ADDRESS_MODE_DENYLIST);
    }
    else if (aArgs[0] == "add")
    {
        SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
        error = otLinkFilterAddAddress(GetInstancePtr(), &extAddr);

        VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY);

        if (!aArgs[2].IsEmpty())
        {
            int8_t rss;

            SuccessOrExit(error = aArgs[2].ParseAsInt8(rss));
            SuccessOrExit(error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss));
        }
    }
    else if (aArgs[0] == "remove")
    {
        SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
        otLinkFilterRemoveAddress(GetInstancePtr(), &extAddr);
    }
    else if (aArgs[0] == "clear")
    {
        otLinkFilterClearAddresses(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

otError Interpreter::ProcessMacFilterRss(Arg aArgs[])
{
    otError             error = OT_ERROR_NONE;
    otMacFilterEntry    entry;
    otMacFilterIterator iterator = OT_MAC_FILTER_ITERATOR_INIT;
    otExtAddress        extAddr;
    int8_t              rss;

    if (aArgs[0].IsEmpty())
    {
        while (otLinkFilterGetNextRssIn(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
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
                           otLinkConvertRssToLinkQuality(GetInstancePtr(), entry.mRssIn));
            }
            else
            {
                OutputMacFilterEntry(entry);
            }
        }
    }
    else if (aArgs[0] == "add-lqi")
    {
        uint8_t linkQuality;

        SuccessOrExit(error = aArgs[2].ParseAsUint8(linkQuality));
        VerifyOrExit(linkQuality <= 3, error = OT_ERROR_INVALID_ARGS);
        rss = otLinkConvertLinkQualityToRss(GetInstancePtr(), linkQuality);

        if (aArgs[1] == "*")
        {
            otLinkFilterSetDefaultRssIn(GetInstancePtr(), rss);
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss);
        }
    }
    else if (aArgs[0] == "add")
    {
        SuccessOrExit(error = aArgs[2].ParseAsInt8(rss));

        if (aArgs[1] == "*")
        {
            otLinkFilterSetDefaultRssIn(GetInstancePtr(), rss);
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            error = otLinkFilterAddRssIn(GetInstancePtr(), &extAddr, rss);
        }
    }
    else if (aArgs[0] == "remove")
    {
        if (aArgs[1] == "*")
        {
            otLinkFilterClearDefaultRssIn(GetInstancePtr());
        }
        else
        {
            SuccessOrExit(error = aArgs[1].ParseAsHexString(extAddr.m8));
            otLinkFilterRemoveRssIn(GetInstancePtr(), &extAddr);
        }
    }
    else if (aArgs[0] == "clear")
    {
        otLinkFilterClearAllRssIn(GetInstancePtr());
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

void Interpreter::OutputMacFilterEntry(const otMacFilterEntry &aEntry)
{
    OutputExtAddress(aEntry.mExtAddress);

    if (aEntry.mRssIn != OT_MAC_FILTER_FIXED_RSS_DISABLED)
    {
        OutputFormat(" : rss %d (lqi %d)", aEntry.mRssIn,
                     otLinkConvertRssToLinkQuality(GetInstancePtr(), aEntry.mRssIn));
    }

    OutputLine("");
}

const char *Interpreter::MacFilterAddressModeToString(otMacFilterAddressMode aMode)
{
    static const char *const kModeStrings[] = {
        "Disabled",  // (0) OT_MAC_FILTER_ADDRESS_MODE_DISABLED
        "Allowlist", // (1) OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST
        "Denylist",  // (2) OT_MAC_FILTER_ADDRESS_MODE_DENYLIST
    };

    static_assert(0 == OT_MAC_FILTER_ADDRESS_MODE_DISABLED, "OT_MAC_FILTER_ADDRESS_MODE_DISABLED value is incorrect");
    static_assert(1 == OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST, "OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST value is incorrect");
    static_assert(2 == OT_MAC_FILTER_ADDRESS_MODE_DENYLIST, "OT_MAC_FILTER_ADDRESS_MODE_DENYLIST value is incorrect");

    return Stringify(aMode, kModeStrings);
}

#endif // OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

template <> otError Interpreter::Process<Cmd("mac")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0] == "retries")
    {
        if (aArgs[1] == "direct")
        {
            error = ProcessGetSet(aArgs + 2, otLinkGetMaxFrameRetriesDirect, otLinkSetMaxFrameRetriesDirect);
        }
#if OPENTHREAD_FTD
        else if (aArgs[1] == "indirect")
        {
            error = ProcessGetSet(aArgs + 2, otLinkGetMaxFrameRetriesIndirect, otLinkSetMaxFrameRetriesIndirect);
        }
#endif
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    else if (aArgs[0] == "send")
    {
        VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        if (aArgs[1] == "datarequest")
        {
            error = otLinkSendDataRequest(GetInstancePtr());
        }
        else if (aArgs[1] == "emptydata")
        {
            error = otLinkSendEmptyData(GetInstancePtr());
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
        }
    }
#endif
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
        ExitNow(); // To silence unused `exit` label warning when `REFERENCE_DEVICE_ENABLE` is not enabled.
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
template <> otError Interpreter::Process<Cmd("trel")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(otTrelIsEnabled(GetInstancePtr()));
    }
    else if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        if (enable)
        {
            otTrelEnable(GetInstancePtr());
        }
        else
        {
            otTrelDisable(GetInstancePtr());
        }
    }
    else if (aArgs[0] == "filter")
    {
        if (aArgs[1].IsEmpty())
        {
            OutputEnabledDisabledStatus(otTrelIsFilterEnabled(GetInstancePtr()));
        }
        else
        {
            SuccessOrExit(error = ParseEnableOrDisable(aArgs[1], enable));
            otTrelSetFilterEnabled(GetInstancePtr(), enable);
        }
    }
    else if (aArgs[0] == "peers")
    {
        uint16_t           index = 0;
        otTrelPeerIterator iterator;
        const otTrelPeer * peer;
        bool               isTable = true;

        if (aArgs[1] == "list")
        {
            isTable = false;
        }
        else
        {
            VerifyOrExit(aArgs[1].IsEmpty(), error = kErrorInvalidArgs);
        }

        if (isTable)
        {
            static const char *const kTrelPeerTableTitles[] = {"No", "Ext MAC Address", "Ext PAN Id",
                                                               "IPv6 Socket Address"};

            static const uint8_t kTrelPeerTableColumnWidths[] = {5, 18, 18, 50};

            OutputTableHeader(kTrelPeerTableTitles, kTrelPeerTableColumnWidths);
        }

        otTrelInitPeerIterator(GetInstancePtr(), &iterator);

        while ((peer = otTrelGetNextPeer(GetInstancePtr(), &iterator)) != nullptr)
        {
            if (!isTable)
            {
                OutputFormat("%03u ExtAddr:", ++index);
                OutputExtAddress(peer->mExtAddress);
                OutputFormat(" ExtPanId:");
                OutputBytes(peer->mExtPanId.m8);
                OutputFormat(" SockAddr:");
                OutputSockAddrLine(peer->mSockAddr);
            }
            else
            {
                char string[OT_IP6_SOCK_ADDR_STRING_SIZE];

                OutputFormat("| %3u | ", ++index);
                OutputExtAddress(peer->mExtAddress);
                OutputFormat(" | ");
                OutputBytes(peer->mExtPanId.m8);
                otIp6SockAddrToString(&peer->mSockAddr, string, sizeof(string));
                OutputLine(" | %-48s |", string);
            }
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}
#endif

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
template <> otError Interpreter::Process<Cmd("networkdiagnostic")>(Arg aArgs[])
{
    otError      error = OT_ERROR_NONE;
    otIp6Address address;
    uint8_t      tlvTypes[OT_NETWORK_DIAGNOSTIC_TYPELIST_MAX_ENTRIES];
    uint8_t      count = 0;

    SuccessOrExit(error = aArgs[1].ParseAsIp6Address(address));

    for (Arg *arg = &aArgs[2]; !arg->IsEmpty(); arg++)
    {
        VerifyOrExit(count < sizeof(tlvTypes), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = arg->ParseAsUint8(tlvTypes[count++]));
    }

    if (aArgs[0] == "get")
    {
        SuccessOrExit(error = otThreadSendDiagnosticGet(GetInstancePtr(), &address, tlvTypes, count,
                                                        &Interpreter::HandleDiagnosticGetResponse, this));
        SetCommandTimeout(kNetworkDiagnosticTimeoutMsecs);
        error = OT_ERROR_PENDING;
    }
    else if (aArgs[0] == "reset")
    {
        IgnoreError(otThreadSendDiagnosticReset(GetInstancePtr(), &address, tlvTypes, count));
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
    while (otThreadGetNextDiagnosticTlv(aMessage, &iterator, &diagTlv) == OT_ERROR_NONE)
    {
        switch (diagTlv.mType)
        {
        case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
            OutputFormat("Ext Address: '");
            OutputExtAddressLine(diagTlv.mData.mExtAddress);
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
            OutputBytesLine(diagTlv.mData.mNetworkData.m8, diagTlv.mData.mNetworkData.mCount);
            break;
        case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
            OutputLine("IP6 Address List:");
            for (uint16_t i = 0; i < diagTlv.mData.mIp6AddrList.mCount; ++i)
            {
                OutputFormat(kIndentSize, "- ");
                OutputIp6AddressLine(diagTlv.mData.mIp6AddrList.mList[i]);
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

exit:
    return;
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

void Interpreter::HandleDetachGracefullyResult(void *aContext)
{
    static_cast<Interpreter *>(aContext)->HandleDetachGracefullyResult();
}

void Interpreter::HandleDetachGracefullyResult(void)
{
    OutputLine("Finished detaching");
    OutputResult(OT_ERROR_NONE);
}

void Interpreter::HandleDiscoveryRequest(const otThreadDiscoveryRequestInfo &aInfo)
{
    OutputFormat("~ Discovery Request from ");
    OutputExtAddress(aInfo.mExtAddress);
    OutputLine(": version=%u,joiner=%d", aInfo.mVersion, aInfo.mIsJoiner);
}

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

void Interpreter::Initialize(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    Interpreter::sInterpreter = new (&sInterpreterRaw) Interpreter(instance, aCallback, aContext);
}

void Interpreter::OutputPrompt(void)
{
#if OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
    static const char sPrompt[] = "> ";

    // The `OutputFormat()` below is adding the prompt which is not
    // part of any command output, so we set the `EmittingCommandOutput`
    // flag to false to avoid it being included in the command output
    // log (under `OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE`).

    SetEmittingCommandOutput(false);
    OutputFormat("%s", sPrompt);
    SetEmittingCommandOutput(true);
#endif // OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
}

void Interpreter::HandleTimer(Timer &aTimer)
{
    static_cast<Interpreter *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void Interpreter::HandleTimer(void)
{
#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
    if (mLocateInProgress)
    {
        mLocateInProgress = false;
        OutputResult(OT_ERROR_RESPONSE_TIMEOUT);
    }
    else
#endif
    {
        OutputResult(OT_ERROR_NONE);
    }
}

void Interpreter::SetCommandTimeout(uint32_t aTimeoutMilli)
{
    OT_ASSERT(mCommandIsPending);
    mTimer.Start(aTimeoutMilli);
}

otError Interpreter::ProcessCommand(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                   \
    {                                                              \
        aCommandString, &Interpreter::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
#if OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
        CmdEntry("ba"),
#endif
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        CmdEntry("bbr"),
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
        CmdEntry("br"),
#endif
        CmdEntry("bufferinfo"),
        CmdEntry("ccathreshold"),
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("ccm"),
#endif
        CmdEntry("channel"),
#if OPENTHREAD_FTD
        CmdEntry("child"),
        CmdEntry("childip"),
        CmdEntry("childmax"),
#endif
#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
        CmdEntry("childsupervision"),
#endif
        CmdEntry("childtimeout"),
#if OPENTHREAD_CONFIG_COAP_API_ENABLE
        CmdEntry("coap"),
#endif
#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
        CmdEntry("coaps"),
#endif
#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
        CmdEntry("coex"),
#endif
#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
        CmdEntry("commissioner"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("contextreusedelay"),
#endif
        CmdEntry("counters"),
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        CmdEntry("csl"),
#endif
        CmdEntry("dataset"),
#if OPENTHREAD_FTD
        CmdEntry("delaytimermin"),
#endif
        CmdEntry("detach"),
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        CmdEntry("diag"),
#endif
#if OPENTHREAD_FTD || OPENTHREAD_MTD
        CmdEntry("discover"),
        CmdEntry("dns"),
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        CmdEntry("domainname"),
#endif
#if OPENTHREAD_CONFIG_DUA_ENABLE
        CmdEntry("dua"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("eidcache"),
#endif
        CmdEntry("eui64"),
        CmdEntry("extaddr"),
        CmdEntry("extpanid"),
        CmdEntry("factoryreset"),
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("fake"),
#endif
        CmdEntry("fem"),
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_FTD || OPENTHREAD_MTD
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        CmdEntry("history"),
#endif
        CmdEntry("ifconfig"),
        CmdEntry("ipaddr"),
        CmdEntry("ipmaddr"),
#if OPENTHREAD_CONFIG_JOINER_ENABLE
        CmdEntry("joiner"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("joinerport"),
#endif
        CmdEntry("keysequence"),
        CmdEntry("leaderdata"),
#if OPENTHREAD_FTD
        CmdEntry("leaderweight"),
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
        CmdEntry("linkmetrics"),
#endif
#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
        CmdEntry("locate"),
#endif
        CmdEntry("log"),
        CmdEntry("mac"),
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
        CmdEntry("macfilter"),
#endif
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("mliid"),
#endif
#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
        CmdEntry("mlr"),
#endif
        CmdEntry("mode"),
        CmdEntry("multiradio"),
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
        CmdEntry("nat64"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("neighbor"),
#endif
        CmdEntry("netdata"),
        CmdEntry("netstat"),
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
        CmdEntry("networkdiagnostic"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("networkidtimeout"),
#endif
        CmdEntry("networkkey"),
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        CmdEntry("networkkeyref"),
#endif
        CmdEntry("networkname"),
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        CmdEntry("networktime"),
#endif
        CmdEntry("panid"),
        CmdEntry("parent"),
#if OPENTHREAD_FTD
        CmdEntry("parentpriority"),
        CmdEntry("partitionid"),
#endif
#if OPENTHREAD_CONFIG_PING_SENDER_ENABLE
        CmdEntry("ping"),
#endif
        CmdEntry("pollperiod"),
#if OPENTHREAD_FTD
        CmdEntry("preferrouterid"),
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        CmdEntry("prefix"),
#endif
        CmdEntry("promiscuous"),
#if OPENTHREAD_FTD
        CmdEntry("pskc"),
#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
        CmdEntry("pskcref"),
#endif
#endif
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        CmdEntry("radiofilter"),
#endif
        CmdEntry("rcp"),
        CmdEntry("region"),
#if OPENTHREAD_FTD
        CmdEntry("releaserouterid"),
#endif
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
        CmdEntry("reset"),
#if OPENTHREAD_FTD || OPENTHREAD_MTD
        CmdEntry("rloc16"),
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        CmdEntry("route"),
#endif
#if OPENTHREAD_FTD
        CmdEntry("router"),
        CmdEntry("routerdowngradethreshold"),
        CmdEntry("routereligible"),
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("routeridrange"),
#endif
        CmdEntry("routerselectionjitter"),
        CmdEntry("routerupgradethreshold"),
#endif
        CmdEntry("scan"),
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        CmdEntry("service"),
#endif
        CmdEntry("singleton"),
#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
        CmdEntry("sntp"),
#endif
#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE || OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
        CmdEntry("srp"),
#endif
        CmdEntry("state"),
#if OPENTHREAD_CONFIG_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_TCP_ENABLE
        CmdEntry("tcp"),
#endif
        CmdEntry("thread"),
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        CmdEntry("trel"),
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        CmdEntry("tvcheck"),
#endif
        CmdEntry("txpower"),
        CmdEntry("udp"),
        CmdEntry("unsecureport"),
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
        CmdEntry("uptime"),
#endif
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD
        CmdEntry("version"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "Command Table is not sorted");

    otError        error   = OT_ERROR_NONE;
    const Command *command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);

    if (command != nullptr)
    {
        error = (this->*command->mHandler)(aArgs + 1);
    }
    else if (aArgs[0] == "help")
    {
        OutputCommandTable(kCommands);

        for (uint8_t i = 0; i < mUserCommandsLength; i++)
        {
            OutputLine("%s", mUserCommands[i].mName);
        }
    }
    else
    {
        error = ProcessUserCommands(aArgs);
    }

    return error;
}

extern "C" void otCliInit(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext)
{
    Interpreter::Initialize(aInstance, aCallback, aContext);
}

extern "C" void otCliInputLine(char *aBuf)
{
    Interpreter::GetInterpreter().ProcessLine(aBuf);
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

extern "C" void otCliAppendResult(otError aError)
{
    Interpreter::GetInterpreter().OutputResult(aError);
}

extern "C" void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    VerifyOrExit(Interpreter::IsInitialized());

    // CLI output is being used for logging, so we set the flag
    // `EmittingCommandOutput` to false indicate this.
    Interpreter::GetInterpreter().SetEmittingCommandOutput(false);
    Interpreter::GetInterpreter().OutputFormatV(aFormat, aArgs);
    Interpreter::GetInterpreter().OutputLine("");
    Interpreter::GetInterpreter().SetEmittingCommandOutput(true);

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
