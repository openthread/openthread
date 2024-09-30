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
        OutputLine(" prf:%s", PreferenceToString(preference));
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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TESTING_API_ENABLE
    if (aArgs[0] == "test")
    {
        otIp6Prefix prefix;

        SuccessOrExit(error = aArgs[1].ParseAsIp6Prefix(prefix));
        otBorderRoutingSetOnLinkPrefix(GetInstancePtr(), &prefix);
        ExitNow();
    }
#endif

    error = ParsePrefixTypeArgs(aArgs, outputPrefixTypes);

    SuccessOrExit(error);

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
        OutputLine(" prf:%s", PreferenceToString(preference));
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

template <> otError Br::Process<Cmd("peers")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli br peers
     * @code
     * br peers
     * rloc16:0x5c00 age:00:00:49
     * rloc16:0xf800 age:00:01:51
     * Done
     * @endcode
     * @par
     * Get the list of peer BRs found in Network Data entries.
     * `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE` is required.
     * Peer BRs are other devices within the Thread mesh that provide external IP connectivity. A device is considered
     * to provide external IP connectivity if at least one of the following conditions is met regarding its Network
     * Data entries:
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with both the default-route and on-mesh flags set.
     * - It has added at least one domain prefix (with both the domain and on-mesh flags set).
     * The list of peer BRs specifically excludes the current device, even if its is itself acting as a BR.
     * Info per BR entry:
     * - RLOC16 of the BR
     * - Age as the duration interval since this BR appeared in Network Data. It is formatted as `{hh}:{mm}:{ss}` for
     *   hours, minutes, seconds, if the duration is less than 24 hours. If the duration is 24 hours or more, the
     *   format is `{dd}d.{hh}:{mm}:{ss}` for days, hours, minutes, seconds.
     * @sa otBorderRoutingGetNextPrefixTableEntry
     */
    if (aArgs[0].IsEmpty())
    {
        otBorderRoutingPrefixTableIterator   iterator;
        otBorderRoutingPeerBorderRouterEntry peerBrEntry;
        char                                 ageString[OT_DURATION_STRING_SIZE];

        otBorderRoutingPrefixTableInitIterator(GetInstancePtr(), &iterator);

        while (otBorderRoutingGetNextPeerBrEntry(GetInstancePtr(), &iterator, &peerBrEntry) == OT_ERROR_NONE)
        {
            otConvertDurationInSecondsToString(peerBrEntry.mAge, ageString, sizeof(ageString));
            OutputLine("rloc16:0x%04x age:%s", peerBrEntry.mRloc16, ageString);
        }
    }
    /**
     * @cli br peers count
     * @code
     * br peers count
     * 2 min-age:00:00:47
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingCountPeerBrs
     */
    else if (aArgs[0] == "count")
    {
        uint32_t minAge;
        uint16_t count;
        char     ageString[OT_DURATION_STRING_SIZE];

        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        count = otBorderRoutingCountPeerBrs(GetInstancePtr(), &minAge);
        otConvertDurationInSecondsToString(minAge, ageString, sizeof(ageString));
        OutputLine("%u min-age:%s", count, ageString);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

/**
 * @cli br prefixtable
 * @code
 * br prefixtable
 * prefix:fd00:1234:5678:0::/64, on-link:no, ms-since-rx:29526, lifetime:1800, route-prf:med,
 * router:ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1)
 * prefix:1200:abba:baba:0::/64, on-link:yes, ms-since-rx:29527, lifetime:1800, preferred:1800,
 * router:ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1)
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
 *   - S: SNAC Router flag
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
            OutputFormat("route-prf:%s, ", PreferenceToString(entry.mRoutePreference));
        }

        OutputFormat("router:");
        OutputRouterInfo(entry.mRouter, kShortVersion);
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
     */
    if (ProcessEnableDisable(aArgs, otBorderRoutingDhcp6PdSetEnabled) == OT_ERROR_NONE)
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
            "idle",     // (3) OT_BORDER_ROUTING_DHCP6_PD_STATE_IDLE
        };

        static_assert(0 == OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_DISABLED value is not expected!");
        static_assert(1 == OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_STOPPED value is not expected!");
        static_assert(2 == OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_RUNNING value is not expected!");
        static_assert(3 == OT_BORDER_ROUTING_DHCP6_PD_STATE_IDLE,
                      "OT_BORDER_ROUTING_DHCP6_PD_STATE_IDLE value is not expected!");

        OutputLine("%s", Stringify(otBorderRoutingDhcp6PdGetState(GetInstancePtr()), kDhcpv6PdStateStrings));
    }
    /**
     * @cli br pd omrprefix
     * @code
     * br pd omrprefix
     * 2001:db8:cafe:0:0/64 lifetime:1800 preferred:1800
     * Done
     * @endcode
     * @par api_copy
     * #otBorderRoutingGetPdOmrPrefix
     */
    else if (aArgs[0] == "omrprefix")
    {
        otBorderRoutingPrefixTableEntry entry;

        SuccessOrExit(error = otBorderRoutingGetPdOmrPrefix(GetInstancePtr(), &entry));

        OutputIp6Prefix(entry.mPrefix);
        OutputLine(" lifetime:%lu preferred:%lu", ToUlong(entry.mValidLifetime), ToUlong(entry.mPreferredLifetime));
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
 * ff02:0:0:0:0:0:0:1 (M:0 O:0 S:1) ms-since-rx:1505 reachable:yes age:00:18:13
 * Done
 * @endcode
 * @par
 * Get the list of discovered routers by Border Routing Manager on the infrastructure link.
 * Info per router:
 * - The router IPv6 address
 * - Flags in received Router Advertisement header:
 *   - M: Managed Address Config flag
 *   - O: Other Config flag
 *   - S: SNAC Router flag (indicates whether the router is a stub router)
 * - Milliseconds since last received message from this router
 * - Reachability flag: A router is marked as unreachable if it fails to respond to multiple Neighbor Solicitation
 *   probes.
 * - Age: Duration interval since this router was first discovered. It is formatted as `{hh}:{mm}:{ss}` for  hours,
 *   minutes, seconds, if the duration is less than 24 hours. If the duration is 24 hours or more, the format is
 *   `{dd}d.{hh}:{mm}:{ss}` for days, hours, minutes, seconds.
 * - `(this BR)` is appended when the router is the local device itself.
 * - `(peer BR)` is appended when the router is likely a peer BR connected to the same Thread mesh. This requires
 *   `OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE`.
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
        OutputRouterInfo(entry, kLongVersion);
    }

exit:
    return error;
}

void Br::OutputRouterInfo(const otBorderRoutingRouterEntry &aEntry, RouterOutputMode aMode)
{
    OutputIp6Address(aEntry.mAddress);
    OutputFormat(" (M:%u O:%u S:%u)", aEntry.mManagedAddressConfigFlag, aEntry.mOtherConfigFlag,
                 aEntry.mSnacRouterFlag);

    if (aMode == kLongVersion)
    {
        char ageString[OT_DURATION_STRING_SIZE];

        otConvertDurationInSecondsToString(aEntry.mAge, ageString, sizeof(ageString));

        OutputFormat(" ms-since-rx:%lu reachable:%s age:%s", ToUlong(aEntry.mMsecSinceLastUpdate),
                     aEntry.mIsReachable ? "yes" : "no", ageString);

        if (aEntry.mIsLocalDevice)
        {
            OutputFormat(" (this BR)");
        }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
        if (aEntry.mIsPeerBr)
        {
            OutputFormat(" (peer BR)");
        }
#endif
    }

    OutputNewLine();
}

template <> otError Br::Process<Cmd("raoptions")>(Arg aArgs[])
{
    static constexpr uint16_t kMaxExtraOptions = 800;

    otError  error = OT_ERROR_NONE;
    uint8_t  options[kMaxExtraOptions];
    uint16_t length;

    /**
     * @cli br raoptions (set,clear)
     * @code
     * br raoptions 0400ff00020001
     * Done
     * @endcode
     * @code
     * br raoptions clear
     * Done
     * @endcode
     * @cparam br raoptions @ca{options|clear}
     * `br raoptions clear` passes a `nullptr` to #otBorderRoutingSetExtraRouterAdvertOptions.
     * Otherwise, you can pass the `options` byte as hex data.
     * @par api_copy
     * #otBorderRoutingSetExtraRouterAdvertOptions
     */
    if (aArgs[0] == "clear")
    {
        length = 0;
    }
    else
    {
        length = sizeof(options);
        SuccessOrExit(error = aArgs[0].ParseAsHexString(length, options));
    }

    error = otBorderRoutingSetExtraRouterAdvertOptions(GetInstancePtr(), length > 0 ? options : nullptr, length);

exit:
    return error;
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
        OutputLine("%s", PreferenceToString(otBorderRoutingGetRouteInfoOptionPreference(GetInstancePtr())));
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
        OutputLine("%s", PreferenceToString(otBorderRoutingGetRoutePreference(GetInstancePtr())));
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
#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
        CmdEntry("peers"),
#endif
        CmdEntry("prefixtable"),
        CmdEntry("raoptions"),
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
