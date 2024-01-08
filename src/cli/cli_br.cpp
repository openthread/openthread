/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements CLI for Border Router.
 */

#include "cli_br.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <string.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

/**
 * @cli br init
 * @code
 * br init 2 1
 * Done
 * @endcode
 * @cparam br init @ca{infrastructure-network-index} @ca{is-running}
 * @par
 * Initializes the Border Routing Manager.
 * @sa otBorderRoutingInit
 */
template <> otError Br::Process<Cmd("init")>(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    uint32_t ifIndex;
    bool     isRunning;

    SuccessOrExit(error = aArgs[0].ParseAsUint32(ifIndex));
    SuccessOrExit(error = aArgs[1].ParseAsBool(isRunning));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otBorderRoutingInit(GetInstancePtr(), ifIndex, isRunning);

exit:
    return error;
}

/**
 * @cli br enable
 * @code
 * br enable
 * Done
 * @endcode
 * @par
 * Enables the Border Routing Manager.
 * @sa otBorderRoutingSetEnabled
 */
template <> otError Br::Process<Cmd("enable")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otBorderRoutingSetEnabled(GetInstancePtr(), true);

exit:
    return error;
}

/**
 * @cli br disable
 * @code
 * br disable
 * Done
 * @endcode
 * @par
 * Disables the Border Routing Manager.
 * @sa otBorderRoutingSetEnabled
 */
template <> otError Br::Process<Cmd("disable")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error = otBorderRoutingSetEnabled(GetInstancePtr(), false);

exit:
    return error;
}

/**
 * @cli br state
 * @code
 * br state
 * running
 * @endcode
 * @par api_copy
 * #otBorderRoutingGetState
 */
template <> otError Br::Process<Cmd("state")>(Arg aArgs[])
{
    static const char *const kStateStrings[] = {

        "uninitialized", // (0) OT_BORDER_ROUTING_STATE_UNINITIALIZED
        "disabled",      // (1) OT_BORDER_ROUTING_STATE_DISABLED
        "stopped",       // (2) OT_BORDER_ROUTING_STATE_STOPPED
        "running",       // (3) OT_BORDER_ROUTING_STATE_RUNNING
    };

    otError error = OT_ERROR_NONE;

    static_assert(0 == OT_BORDER_ROUTING_STATE_UNINITIALIZED, "STATE_UNINITIALIZED value is incorrect");
    static_assert(1 == OT_BORDER_ROUTING_STATE_DISABLED, "STATE_DISABLED value is incorrect");
    static_assert(2 == OT_BORDER_ROUTING_STATE_STOPPED, "STATE_STOPPED value is incorrect");
    static_assert(3 == OT_BORDER_ROUTING_STATE_RUNNING, "STATE_RUNNING value is incorrect");

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine("%s", Stringify(otBorderRoutingGetState(GetInstancePtr()), kStateStrings));

exit:
    return error;
}

otError Br::ParsePrefixTypeArgs(Arg aArgs[], PrefixType &aFlags)
{
    otError error = OT_ERROR_NONE;

    aFlags = 0;

    if (aArgs[0].IsEmpty())
    {
        aFlags = kPrefixTypeFavored | kPrefixTypeLocal;
        ExitNow();
    }

    if (aArgs[0] == "local")
    {
        aFlags = kPrefixTypeLocal;
    }
    else if (aArgs[0] == "favored")
    {
        aFlags = kPrefixTypeFavored;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

/**
 * @cli br omrprefix
 * @code
 * br omrprefix
 * Local: fdfc:1ff5:1512:5622::/64
 * Favored: fdfc:1ff5:1512:5622::/64 prf:low
 * Done
 * @endcode
 * @par
 * Outputs both local and favored OMR prefix.
 * @sa otBorderRoutingGetOmrPrefix
 * @sa otBorderRoutingGetFavoredOmrPrefix
 */
template <> otError Br::Process<Cmd("omrprefix")>(Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    PrefixType outputPrefixTypes;

    SuccessOrExit(error = ParsePrefixTypeArgs(aArgs, outputPrefixTypes));

    /**
     * @cli br omrprefix local
     * @code
     * br omrprefix local
     * fdfc:1ff5:1512:5622::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetOmrPrefix
     */
    if (outputPrefixTypes & kPrefixTypeLocal)
    {
        otIp6Prefix local;

        SuccessOrExit(error = otBorderRoutingGetOmrPrefix(GetInstancePtr(), &local));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeLocal ? "" : "Local: ");
        OutputIp6PrefixLine(local);
    }

    /**
     * @cli br omrprefix favored
     * @code
     * br omrprefix favored
     * fdfc:1ff5:1512:5622::/64 prf:low
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetFavoredOmrPrefix
     */
    if (outputPrefixTypes & kPrefixTypeFavored)
    {
        otIp6Prefix       favored;
        otRoutePreference preference;

        SuccessOrExit(error = otBorderRoutingGetFavoredOmrPrefix(GetInstancePtr(), &favored, &preference));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeFavored ? "" : "Favored: ");
        OutputIp6Prefix(favored);
        OutputLine(" prf:%s", Interpreter::PreferenceToString(preference));
    }

exit:
    return error;
}

/**
 * @cli br onlinkprefix
 * @code
 * br onlinkprefix
 * Local: fd41:2650:a6f5:0::/64
 * Favored: 2600::0:1234:da12::/64
 * Done
 * @endcode
 * @par
 * Outputs both local and favored on-link prefixes.
 * @sa otBorderRoutingGetOnLinkPrefix
 * @sa otBorderRoutingGetFavoredOnLinkPrefix
 */
template <> otError Br::Process<Cmd("onlinkprefix")>(Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    PrefixType outputPrefixTypes;

    SuccessOrExit(error = ParsePrefixTypeArgs(aArgs, outputPrefixTypes));

    /**
     * @cli br onlinkprefix local
     * @code
     * br onlinkprefix local
     * fd41:2650:a6f5:0::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetOnLinkPrefix
     */
    if (outputPrefixTypes & kPrefixTypeLocal)
    {
        otIp6Prefix local;

        SuccessOrExit(error = otBorderRoutingGetOnLinkPrefix(GetInstancePtr(), &local));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeLocal ? "" : "Local: ");
        OutputIp6PrefixLine(local);
    }

    /**
     * @cli br onlinkprefix favored
     * @code
     * br onlinkprefix favored
     * 2600::0:1234:da12::/64
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetFavoredOnLinkPrefix
     */
    if (outputPrefixTypes & kPrefixTypeFavored)
    {
        otIp6Prefix favored;

        SuccessOrExit(error = otBorderRoutingGetFavoredOnLinkPrefix(GetInstancePtr(), &favored));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeFavored ? "" : "Favored: ");
        OutputIp6PrefixLine(favored);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

/**
 * @cli br nat64prefix
 * @code
 * br nat64prefix
 * Local: fd14:1078:b3d5:b0b0:0:0::/96
 * Favored: fd14:1078:b3d5:b0b0:0:0::/96 prf:low
 * Done
 * @endcode
 * @par
 * Outputs both local and favored NAT64 prefixes.
 * @sa otBorderRoutingGetNat64Prefix
 * @sa otBorderRoutingGetFavoredNat64Prefix
 */
template <> otError Br::Process<Cmd("nat64prefix")>(Arg aArgs[])
{
    otError    error = OT_ERROR_NONE;
    PrefixType outputPrefixTypes;

    SuccessOrExit(error = ParsePrefixTypeArgs(aArgs, outputPrefixTypes));

    /**
     * @cli br nat64prefix local
     * @code
     * br nat64prefix local
     * fd14:1078:b3d5:b0b0:0:0::/96
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetNat64Prefix
     */
    if (outputPrefixTypes & kPrefixTypeLocal)
    {
        otIp6Prefix local;

        SuccessOrExit(error = otBorderRoutingGetNat64Prefix(GetInstancePtr(), &local));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeLocal ? "" : "Local: ");
        OutputIp6PrefixLine(local);
    }

    /**
     * @cli br nat64prefix favored
     * @code
     * br nat64prefix favored
     * fd14:1078:b3d5:b0b0:0:0::/96 prf:low
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetFavoredNat64Prefix
     */
    if (outputPrefixTypes & kPrefixTypeFavored)
    {
        otIp6Prefix       favored;
        otRoutePreference preference;

        SuccessOrExit(error = otBorderRoutingGetFavoredNat64Prefix(GetInstancePtr(), &favored, &preference));

        OutputFormat("%s", outputPrefixTypes == kPrefixTypeFavored ? "" : "Favored: ");
        OutputIp6Prefix(favored);
        OutputLine(" prf:%s", Interpreter::PreferenceToString(preference));
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

/**
 * @cli br prefixtable
 * @code
 * br prefixtable
 * prefix:fd00:1234:5678:0::/64, on-link:no, ms-since-rx:29526, lifetime:1800, route-prf:med,
 * router:ff02:0:0:0:0:0:0:1 (M:0 O:0 Stub:1)
 * prefix:1200:abba:baba:0::/64, on-link:yes, ms-since-rx:29527, lifetime:1800, preferred:1800,
 * router:ff02:0:0:0:0:0:0:1 (M:0 O:0 Stub:1)
 * Done
 * @endcode
 * @par
 * Get the discovered prefixes by Border Routing Manager on the infrastructure link.
 * Info per prefix entry:
 * - The prefix
 * - Whether the prefix is on-link or route
 * - Milliseconds since last received Router Advertisement containing this prefix
 * - Prefix lifetime in seconds
 * - Preferred lifetime in seconds only if prefix is on-link
 * - Route preference (low, med, high) only if prefix is route (not on-link)
 * - The router IPv6 address which advertising this prefix
 * - Flags in received Router Advertisement header:
 *   - M: Managed Address Config flag
 *   - O: Other Config flag
 *   - Stub: Stub Router flag (indicates whether the router is a stub router)
 * @sa otBorderRoutingGetNextPrefixTableEntry
 */
template <> otError Br::Process<Cmd("prefixtable")>(Arg aArgs[])
{
    otError                            error = OT_ERROR_NONE;
    otBorderRoutingPrefixTableIterator iterator;
    otBorderRoutingPrefixTableEntry    entry;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    otBorderRoutingPrefixTableInitIterator(GetInstancePtr(), &iterator);

    while (otBorderRoutingGetNextPrefixTableEntry(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
    {
        char string[OT_IP6_PREFIX_STRING_SIZE];

        otIp6PrefixToString(&entry.mPrefix, string, sizeof(string));
        OutputFormat("prefix:%s, on-link:%s, ms-since-rx:%lu, lifetime:%lu, ", string, entry.mIsOnLink ? "yes" : "no",
                     ToUlong(entry.mMsecSinceLastUpdate), ToUlong(entry.mValidLifetime));

        if (entry.mIsOnLink)
        {
            OutputFormat("preferred:%lu, ", ToUlong(entry.mPreferredLifetime));
        }
        else
        {
            OutputFormat("route-prf:%s, ", Interpreter::PreferenceToString(entry.mRoutePreference));
        }

        OutputFormat("router:");
        OutputRouterInfo(entry.mRouter);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
template <> otError Br::Process<Cmd("pd")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli br pd (enable,disable)
     * @code
     * br pd enable
     * Done
     * @endcode
     * @code
     * br pd disable
     * Done
     * @endcode
     * @cparam br pd @ca{enable|disable}
     * @par api_copy
     * #otBorderRoutingDhcp6PdSetEnabled
     *
     */
    if (Interpreter::GetInterpreter().ProcessEnableDisable(aArgs, otBorderRoutingDhcp6PdSetEnabled) == OT_ERROR_NONE)
    {
    }
    /**
     * @cli br pd state
     * @code
     * br pd state
     * running
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingDhcp6PdGetState
     */
    else if (aArgs[0] == "state")
    {
        static const char *const kDhcpv6PdStateStrings[] = {
            "disabled", // (0) OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED
            "stopped",  // (1) OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED
            "running",  // (2) OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING
        };

        static_assert(0 == OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED value is not expected!");
        static_assert(1 == OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED value is not expected!");
        static_assert(2 == OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING value is not expected!");

        OutputLine("%s", Stringify(otBorderRoutingDhcp6PdGetState(GetInstancePtr()), kDhcpv6PdStateStrings));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_COMMAND);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

/**
 * @cli br routers
 * @code
 * br routers
 * ff02:0:0:0:0:0:0:1 (M:0 O:0 Stub:1)
 * Done
 * @endcode
 * @par
 * Get the list of discovered routers by Border Routing Manager on the infrastructure link.
 * Info per router:
 * - The router IPv6 address
 * - Flags in received Router Advertisement header:
 *   - M: Managed Address Config flag
 *   - O: Other Config flag
 *   - Stub: Stub Router flag (indicates whether the router is a stub router)
 * @sa otBorderRoutingGetNextRouterEntry
 */
template <> otError Br::Process<Cmd("routers")>(Arg aArgs[])
{
    otError                            error = OT_ERROR_NONE;
    otBorderRoutingPrefixTableIterator iterator;
    otBorderRoutingRouterEntry         entry;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    otBorderRoutingPrefixTableInitIterator(GetInstancePtr(), &iterator);

    while (otBorderRoutingGetNextRouterEntry(GetInstancePtr(), &iterator, &entry) == OT_ERROR_NONE)
    {
        OutputRouterInfo(entry);
    }

exit:
    return error;
}

void Br::OutputRouterInfo(const otBorderRoutingRouterEntry &aEntry)
{
    OutputIp6Address(aEntry.mAddress);
    OutputLine(" (M:%u O:%u Stub:%u)", aEntry.mManagedAddressConfigFlag, aEntry.mOtherConfigFlag,
               aEntry.mStubRouterFlag);
}

template <> otError Br::Process<Cmd("rioprf")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli br rioprf
     * @code
     * br rioprf
     * med
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetRouteInfoOptionPreference
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s",
                   Interpreter::PreferenceToString(otBorderRoutingGetRouteInfoOptionPreference(GetInstancePtr())));
    }
    /**
     * @cli br rioprf clear
     * @code
     * br rioprf clear
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingClearRouteInfoOptionPreference
     */
    else if (aArgs[0] == "clear")
    {
        otBorderRoutingClearRouteInfoOptionPreference(GetInstancePtr());
    }
    /**
     * @cli br rioprf (high,med,low)
     * @code
     * br rioprf low
     * Done
     * @endcode
     * @cparam br rioprf [@ca{high}|@ca{med}|@ca{low}]
     * @par api_copy
     * #otBorderRoutingSetRouteInfoOptionPreference
     */
    else
    {
        otRoutePreference preference;

        SuccessOrExit(error = Interpreter::ParsePreference(aArgs[0], preference));
        otBorderRoutingSetRouteInfoOptionPreference(GetInstancePtr(), preference);
    }

exit:
    return error;
}

template <> otError Br::Process<Cmd("routeprf")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli br routeprf
     * @code
     * br routeprf
     * med
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetRoutePreference
     */
    if (aArgs[0].IsEmpty())
    {
        OutputLine("%s", Interpreter::PreferenceToString(otBorderRoutingGetRoutePreference(GetInstancePtr())));
    }
    /**
     * @cli br routeprf clear
     * @code
     * br routeprf clear
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingClearRoutePreference
     */
    else if (aArgs[0] == "clear")
    {
        otBorderRoutingClearRoutePreference(GetInstancePtr());
    }
    /**
     * @cli br routeprf (high,med,low)
     * @code
     * br routeprf low
     * Done
     * @endcode
     * @cparam br routeprf [@ca{high}|@ca{med}|@ca{low}]
     * @par api_copy
     * #otBorderRoutingSetRoutePreference
     */
    else
    {
        otRoutePreference preference;

        SuccessOrExit(error = Interpreter::ParsePreference(aArgs[0], preference));
        otBorderRoutingSetRoutePreference(GetInstancePtr(), preference);
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE

/**
 * @cli br counters
 * @code
 * br counters
 * Inbound Unicast: Packets 4 Bytes 320
 * Inbound Multicast: Packets 0 Bytes 0
 * Outbound Unicast: Packets 2 Bytes 160
 * Outbound Multicast: Packets 0 Bytes 0
 * RA Rx: 4
 * RA TxSuccess: 2
 * RA TxFailed: 0
 * RS Rx: 0
 * RS TxSuccess: 2
 * RS TxFailed: 0
 * Done
 * @endcode
 * @par api_copy
 * #otIp6GetBorderRoutingCounters
 */
template <> otError Br::Process<Cmd("counters")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    Interpreter::GetInterpreter().OutputBorderRouterCounters();

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE

otError Br::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                          \
    {                                                     \
        aCommandString, &Br::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
        CmdEntry("counters"),
#endif
        CmdEntry("disable"),
        CmdEntry("enable"),
        CmdEntry("init"),
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
        CmdEntry("nat64prefix"),
#endif
        CmdEntry("omrprefix"),
        CmdEntry("onlinkprefix"),
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
        CmdEntry("pd"),
#endif
        CmdEntry("prefixtable"),
        CmdEntry("rioprf"),
        CmdEntry("routeprf"),
        CmdEntry("routers"),
        CmdEntry("state"),
    };

#undef CmdEntry

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
