/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements CLI for the History Tracker.
 */

#include "cli_history.hpp"

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

#include <string.h>

#include "cli/cli.hpp"

namespace ot {
namespace Cli {

static const char *const kSimpleEventStrings[] = {
    "Added",  // (0) OT_HISTORY_TRACKER_{NET_DATA_ENTRY/ADDRESS_EVENT}_ADDED
    "Removed" // (1) OT_HISTORY_TRACKER_{NET_DATA_ENTRY/ADDRESS_EVENT}_REMOVED
};

otError History::ParseArgs(Arg aArgs[], bool &aIsList, uint16_t &aNumEntries) const
{
    if (*aArgs == "list")
    {
        aArgs++;
        aIsList = true;
    }
    else
    {
        aIsList = false;
    }

    if (aArgs->ParseAsUint16(aNumEntries) == OT_ERROR_NONE)
    {
        aArgs++;
    }
    else
    {
        aNumEntries = 0;
    }

    return aArgs[0].IsEmpty() ? OT_ERROR_NONE : OT_ERROR_INVALID_ARGS;
}

/**
 * @cli history ipaddr
 * @code
 * history ipaddr
 * | Age                  | Event   | Address / Prefix Length                     | Origin |Scope| P | V | R |
 * +----------------------+---------+---------------------------------------------+--------+-----+---+---+---+
 * |         00:00:04.991 | Removed | 2001:dead:beef:cafe:c4cb:caba:8d55:e30b/64  | slaac  |  14 | Y | Y | N |
 * |         00:00:44.647 | Added   | 2001:dead:beef:cafe:c4cb:caba:8d55:e30b/64  | slaac  |  14 | Y | Y | N |
 * |         00:01:07.199 | Added   | fd00:0:0:0:0:0:0:1/64                       | manual |  14 | Y | Y | N |
 * |         00:02:17.885 | Added   | fdde:ad00:beef:0:0:ff:fe00:fc00/64          | thread |   3 | N | Y | N |
 * |         00:02:17.885 | Added   | fdde:ad00:beef:0:0:ff:fe00:5400/64          | thread |   3 | N | Y | Y |
 * |         00:02:20.107 | Removed | fdde:ad00:beef:0:0:ff:fe00:5400/64          | thread |   3 | N | Y | Y |
 * |         00:02:21.575 | Added   | fdde:ad00:beef:0:0:ff:fe00:5400/64          | thread |   3 | N | Y | Y |
 * |         00:02:21.575 | Added   | fdde:ad00:beef:0:ecea:c4fc:ad96:4655/64     | thread |   3 | N | Y | N |
 * |         00:02:23.904 | Added   | fe80:0:0:0:3c12:a4d2:fbe0:31ad/64           | thread |   2 | Y | Y | N |
 * Done
 * @endcode
 * @code
 * history ipaddr list 5
 * 00:00:20.327 -> event:Removed address:2001:dead:beef:cafe:c4cb:caba:8d55:e30b <!--
 * -->prefixlen:64 origin:slaac scope:14 preferred:yes valid:yes rloc:no
 * 00:00:59.983 -> event:Added address:2001:dead:beef:cafe:c4cb:caba:8d55:e30b <!--
 * -->prefixlen:64 origin:slaac scope:14 preferred:yes valid:yes rloc:no
 * 00:01:22.535 -> event:Added address:fd00:0:0:0:0:0:0:1 prefixlen:64 <!--
 * -->origin:manual scope:14 preferred:yes valid:yes rloc:no
 * 00:02:33.221 -> event:Added address:fdde:ad00:beef:0:0:ff:fe00:fc00 <!--
 * -->prefixlen:64 origin:thread scope:3 preferred:no valid:yes rloc:no
 * 00:02:33.221 -> event:Added address:fdde:ad00:beef:0:0:ff:fe00:5400 <!--
 * -->prefixlen:64 origin:thread scope:3 preferred:no valid:yes rloc:yes
 * Done
 * @endcode
 * @cparam history ipaddr [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the unicast IPv6 address history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Event: Possible values are `Added` or `Removed`.
 * * Address/Prefix Length: Unicast address with its prefix length (in bits).
 * * Origin: Possible value are `thread`, `slaac`, `dhcp6`, or `manual`.
 * * Scope: IPv6 address scope.
 * * P: Preferred flag.
 * * V: Valid flag.
 * * RLOC (R): This flag indicates if the IPv6 address is a routing locator.
 * @note
 * All commands under `history` require the `OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE`
 * feature to be enabled.
 * @sa otHistoryTrackerEntryAgeToString
 * @sa otHistoryTrackerIterateUnicastAddressHistory
 */
template <> otError History::Process<Cmd("ipaddr")>(Arg aArgs[])
{
    otError                                   error;
    bool                                      isList;
    uint16_t                                  numEntries;
    otHistoryTrackerIterator                  iterator;
    const otHistoryTrackerUnicastAddressInfo *info;
    uint32_t                                  entryAge;
    char                                      ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                                      addressString[OT_IP6_ADDRESS_STRING_SIZE + 4];

    static_assert(0 == OT_HISTORY_TRACKER_ADDRESS_EVENT_ADDED, "ADDRESS_EVENT_ADDED is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_ADDRESS_EVENT_REMOVED, "ADDRESS_EVENT_REMOVED is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Event   | Address / PrefixLen                  /123   | Origin |Scope| P | V | R |
        // +----------------------+---------+---------------------------------------------+--------+-----+---+---+---+

        static const char *const kUnicastAddrInfoTitles[] = {
            "Age", "Event", "Address / PrefixLength", "Origin", "Scope", "P", "V", "R"};

        static const uint8_t kUnicastAddrInfoColumnWidths[] = {22, 9, 45, 8, 5, 3, 3, 3};

        OutputTableHeader(kUnicastAddrInfoTitles, kUnicastAddrInfoColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateUnicastAddressHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));
        otIp6AddressToString(&info->mAddress, addressString, sizeof(addressString));

        if (!isList)
        {
            size_t len = strlen(addressString);

            snprintf(&addressString[len], sizeof(addressString) - len, "/%d", info->mPrefixLength);

            OutputLine("| %20s | %-7s | %-43s | %-6s | %3d | %c | %c | %c |", ageString,
                       Stringify(info->mEvent, kSimpleEventStrings), addressString,
                       Interpreter::AddressOriginToString(info->mAddressOrigin), info->mScope,
                       info->mPreferred ? 'Y' : 'N', info->mValid ? 'Y' : 'N', info->mRloc ? 'Y' : 'N');
        }
        else
        {
            OutputLine("%s -> event:%s address:%s prefixlen:%d origin:%s scope:%d preferred:%s valid:%s rloc:%s",
                       ageString, Stringify(info->mEvent, kSimpleEventStrings), addressString, info->mPrefixLength,
                       Interpreter::AddressOriginToString(info->mAddressOrigin), info->mScope,
                       info->mPreferred ? "yes" : "no", info->mValid ? "yes" : "no", info->mRloc ? "yes" : "no");
        }
    }

exit:
    return error;
}

/**
 * @cli history ipmaddr
 * @code
 * history ipmaddr
 * | Age                  | Event        | Multicast Address                       | Origin |
 * +----------------------+--------------+-----------------------------------------+--------+
 * |         00:00:08.592 | Unsubscribed | ff05:0:0:0:0:0:0:1                      | Manual |
 * |         00:01:25.353 | Subscribed   | ff05:0:0:0:0:0:0:1                      | Manual |
 * |         00:01:54.953 | Subscribed   | ff03:0:0:0:0:0:0:2                      | Thread |
 * |         00:01:54.953 | Subscribed   | ff02:0:0:0:0:0:0:2                      | Thread |
 * |         00:01:59.329 | Subscribed   | ff33:40:fdde:ad00:beef:0:0:1            | Thread |
 * |         00:01:59.329 | Subscribed   | ff32:40:fdde:ad00:beef:0:0:1            | Thread |
 * |         00:02:01.129 | Subscribed   | ff03:0:0:0:0:0:0:fc                     | Thread |
 * |         00:02:01.129 | Subscribed   | ff03:0:0:0:0:0:0:1                      | Thread |
 * |         00:02:01.129 | Subscribed   | ff02:0:0:0:0:0:0:1                      | Thread |
 * Done
 * @endcode
 * @code
 * history ipmaddr list
 * 00:00:25.447 -> event:Unsubscribed address:ff05:0:0:0:0:0:0:1 origin:Manual
 * 00:01:42.208 -> event:Subscribed address:ff05:0:0:0:0:0:0:1 origin:Manual
 * 00:02:11.808 -> event:Subscribed address:ff03:0:0:0:0:0:0:2 origin:Thread
 * 00:02:11.808 -> event:Subscribed address:ff02:0:0:0:0:0:0:2 origin:Thread
 * 00:02:16.184 -> event:Subscribed address:ff33:40:fdde:ad00:beef:0:0:1 origin:Thread
 * 00:02:16.184 -> event:Subscribed address:ff32:40:fdde:ad00:beef:0:0:1 origin:Thread
 * 00:02:17.984 -> event:Subscribed address:ff03:0:0:0:0:0:0:fc origin:Thread
 * 00:02:17.984 -> event:Subscribed address:ff03:0:0:0:0:0:0:1 origin:Thread
 * 00:02:17.984 -> event:Subscribed address:ff02:0:0:0:0:0:0:1 origin:Thread
 * Done
 * @endcode
 * @cparam history ipmaddr [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the multicast IPv6 address history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Event: Possible values are `Subscribed` or `Unsubscribed`.
 * * Multicast Address
 * * Origin: Possible values are `Thread` or `Manual`.
 * @sa otHistoryTrackerEntryAgeToString
 * @sa otHistoryTrackerIterateMulticastAddressHistory
 */
template <> otError History::Process<Cmd("ipmaddr")>(Arg aArgs[])
{
    static const char *const kEventStrings[] = {
        "Subscribed",  // (0) OT_HISTORY_TRACKER_ADDRESS_EVENT_ADDED
        "Unsubscribed" // (1) OT_HISTORY_TRACKER_ADDRESS_EVENT_REMOVED
    };

    otError                                     error;
    bool                                        isList;
    uint16_t                                    numEntries;
    otHistoryTrackerIterator                    iterator;
    const otHistoryTrackerMulticastAddressInfo *info;
    uint32_t                                    entryAge;
    char                                        ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                                        addressString[OT_IP6_ADDRESS_STRING_SIZE];

    static_assert(0 == OT_HISTORY_TRACKER_ADDRESS_EVENT_ADDED, "ADDRESS_EVENT_ADDED is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_ADDRESS_EVENT_REMOVED, "ADDRESS_EVENT_REMOVED is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Event        | Multicast Address                       | Origin |
        // +----------------------+--------------+-----------------------------------------+--------+

        static const char *const kMulticastAddrInfoTitles[] = {
            "Age",
            "Event",
            "Multicast Address",
            "Origin",
        };

        static const uint8_t kMulticastAddrInfoColumnWidths[] = {22, 14, 42, 8};

        OutputTableHeader(kMulticastAddrInfoTitles, kMulticastAddrInfoColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateMulticastAddressHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));
        otIp6AddressToString(&info->mAddress, addressString, sizeof(addressString));

        OutputLine(isList ? "%s -> event:%s address:%s origin:%s" : "| %20s | %-12s | %-39s | %-6s |", ageString,
                   Stringify(info->mEvent, kEventStrings), addressString,
                   Interpreter::AddressOriginToString(info->mAddressOrigin));
    }

exit:
    return error;
}

/**
 * @cli history neighbor
 * @code
 * history neighbor
 * | Age                  | Type   | Event     | Extended Address | RLOC16 | Mode | Ave RSS |
 * +----------------------+--------+-----------+------------------+--------+------+---------+
 * |         00:00:29.233 | Child  | Added     | ae5105292f0b9169 | 0x8404 | -    |     -20 |
 * |         00:01:38.368 | Child  | Removed   | ae5105292f0b9169 | 0x8401 | -    |     -20 |
 * |         00:04:27.181 | Child  | Changed   | ae5105292f0b9169 | 0x8401 | -    |     -20 |
 * |         00:04:51.236 | Router | Added     | 865c7ca38a5fa960 | 0x9400 | rdn  |     -20 |
 * |         00:04:51.587 | Child  | Removed   | 865c7ca38a5fa960 | 0x8402 | rdn  |     -20 |
 * |         00:05:22.764 | Child  | Changed   | ae5105292f0b9169 | 0x8401 | rn   |     -20 |
 * |         00:06:40.764 | Child  | Added     | 4ec99efc874a1841 | 0x8403 | r    |     -20 |
 * |         00:06:44.060 | Child  | Added     | 865c7ca38a5fa960 | 0x8402 | rdn  |     -20 |
 * |         00:06:49.515 | Child  | Added     | ae5105292f0b9169 | 0x8401 | -    |     -20 |
 * Done
 * @endcode
 * @code
 * history neighbor list
 * 00:00:34.753 -> type:Child event:Added extaddr:ae5105292f0b9169 rloc16:0x8404 mode:- rss:-20
 * 00:01:43.888 -> type:Child event:Removed extaddr:ae5105292f0b9169 rloc16:0x8401 mode:- rss:-20
 * 00:04:32.701 -> type:Child event:Changed extaddr:ae5105292f0b9169 rloc16:0x8401 mode:- rss:-20
 * 00:04:56.756 -> type:Router event:Added extaddr:865c7ca38a5fa960 rloc16:0x9400 mode:rdn rss:-20
 * 00:04:57.107 -> type:Child event:Removed extaddr:865c7ca38a5fa960 rloc16:0x8402 mode:rdn rss:-20
 * 00:05:28.284 -> type:Child event:Changed extaddr:ae5105292f0b9169 rloc16:0x8401 mode:rn rss:-20
 * 00:06:46.284 -> type:Child event:Added extaddr:4ec99efc874a1841 rloc16:0x8403 mode:r rss:-20
 * 00:06:49.580 -> type:Child event:Added extaddr:865c7ca38a5fa960 rloc16:0x8402 mode:rdn rss:-20
 * 00:06:55.035 -> type:Child event:Added extaddr:ae5105292f0b9169 rloc16:0x8401 mode:- rss:-20
 * Done
 * @endcode
 * @cparam history neighbor [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the neighbor history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Type: `Child` or `Router`.
 * * Event: Possible values are `Added`, `Removed`, or `Changed`.
 * * Extended Address
 * * RLOC16
 * * Mode: MLE link mode. Possible values:
 *     * `-`: no flags set (rx-off-when-idle, minimal Thread device,
 *       stable network data).
 *     * `r`: rx-on-when-idle
 *     * `d`: Full Thread Device.
 *     * `n`: Full Network Data
 * * Ave RSS: Average number of frames (in dBm) received from the neighbor at the
 *   time the entry was recorded.
 * @sa otHistoryTrackerIterateNeighborHistory
 */
template <> otError History::Process<Cmd("neighbor")>(Arg aArgs[])
{
    static const char *const kEventString[] = {
        /* (0) OT_HISTORY_TRACKER_NEIGHBOR_EVENT_ADDED     -> */ "Added",
        /* (1) OT_HISTORY_TRACKER_NEIGHBOR_EVENT_REMOVED   -> */ "Removed",
        /* (2) OT_HISTORY_TRACKER_NEIGHBOR_EVENT_CHANGED   -> */ "Changed",
        /* (3) OT_HISTORY_TRACKER_NEIGHBOR_EVENT_RESTORING -> */ "Restoring",
    };

    otError                             error;
    bool                                isList;
    uint16_t                            numEntries;
    otHistoryTrackerIterator            iterator;
    const otHistoryTrackerNeighborInfo *info;
    uint32_t                            entryAge;
    char                                ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    otLinkModeConfig                    mode;
    char                                linkModeString[Interpreter::kLinkModeStringSize];

    static_assert(0 == OT_HISTORY_TRACKER_NEIGHBOR_EVENT_ADDED, "NEIGHBOR_EVENT_ADDED value is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_NEIGHBOR_EVENT_REMOVED, "NEIGHBOR_EVENT_REMOVED value is incorrect");
    static_assert(2 == OT_HISTORY_TRACKER_NEIGHBOR_EVENT_CHANGED, "NEIGHBOR_EVENT_CHANGED value is incorrect");
    static_assert(3 == OT_HISTORY_TRACKER_NEIGHBOR_EVENT_RESTORING, "NEIGHBOR_EVENT_RESTORING value is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Type   | Event     | Extended Address | RLOC16 | Mode | Ave RSS |
        // +----------------------+--------+-----------+------------------+--------+------+---------+

        static const char *const kNeighborInfoTitles[] = {
            "Age", "Type", "Event", "Extended Address", "RLOC16", "Mode", "Ave RSS",
        };

        static const uint8_t kNeighborInfoColumnWidths[] = {22, 8, 11, 18, 8, 6, 9};

        OutputTableHeader(kNeighborInfoTitles, kNeighborInfoColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateNeighborHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        mode.mRxOnWhenIdle = info->mRxOnWhenIdle;
        mode.mDeviceType   = info->mFullThreadDevice;
        mode.mNetworkData  = info->mFullNetworkData;
        Interpreter::LinkModeToString(mode, linkModeString);

        OutputFormat(isList ? "%s -> type:%s event:%s extaddr:" : "| %20s | %-6s | %-9s | ", ageString,
                     info->mIsChild ? "Child" : "Router", kEventString[info->mEvent]);
        OutputExtAddress(info->mExtAddress);
        OutputLine(isList ? " rloc16:0x%04x mode:%s rss:%d" : " | 0x%04x | %-4s | %7d |", info->mRloc16, linkModeString,
                   info->mAverageRssi);
    }

exit:
    return error;
}

/**
 * @cli history router
 * @code
 * history router
 * | Age                  | Event          | ID (RLOC16) | Next Hop    | Path Cost  |
 * +----------------------+----------------+-------------+-------------+------------+
 * |         00:00:05.258 | NextHopChanged |  7 (0x1c00) | 34 (0x8800) | inf ->   3 |
 * |         00:00:08.604 | NextHopChanged | 34 (0x8800) | 34 (0x8800) | inf ->   2 |
 * |         00:00:08.604 | Added          |  7 (0x1c00) |        none | inf -> inf |
 * |         00:00:11.931 | Added          | 34 (0x8800) |        none | inf -> inf |
 * |         00:00:14.948 | Removed        | 59 (0xec00) |        none | inf -> inf |
 * |         00:00:14.948 | Removed        | 54 (0xd800) |        none | inf -> inf |
 * |         00:00:14.948 | Removed        | 34 (0x8800) |        none | inf -> inf |
 * |         00:00:14.948 | Removed        |  7 (0x1c00) |        none | inf -> inf |
 * |         00:00:54.795 | NextHopChanged | 59 (0xec00) | 34 (0x8800) |   1 ->   5 |
 * |         00:02:33.735 | NextHopChanged | 54 (0xd800) |        none |  15 -> inf |
 * |         00:03:10.915 | CostChanged    | 54 (0xd800) | 34 (0x8800) |  13 ->  15 |
 * |         00:03:45.716 | NextHopChanged | 54 (0xd800) | 34 (0x8800) |  15 ->  13 |
 * |         00:03:46.188 | CostChanged    | 54 (0xd800) | 59 (0xec00) |  13 ->  15 |
 * |         00:04:19.124 | CostChanged    | 54 (0xd800) | 59 (0xec00) |  11 ->  13 |
 * |         00:04:52.008 | CostChanged    | 54 (0xd800) | 59 (0xec00) |   9 ->  11 |
 * |         00:05:23.176 | CostChanged    | 54 (0xd800) | 59 (0xec00) |   7 ->   9 |
 * |         00:05:51.081 | CostChanged    | 54 (0xd800) | 59 (0xec00) |   5 ->   7 |
 * |         00:06:48.721 | CostChanged    | 54 (0xd800) | 59 (0xec00) |   3 ->   5 |
 * |         00:07:13.792 | NextHopChanged | 54 (0xd800) | 59 (0xec00) |   1 ->   3 |
 * |         00:09:28.681 | NextHopChanged |  7 (0x1c00) | 34 (0x8800) | inf ->   3 |
 * |         00:09:31.882 | Added          |  7 (0x1c00) |        none | inf -> inf |
 * |         00:09:51.240 | NextHopChanged | 54 (0xd800) | 54 (0xd800) | inf ->   1 |
 * |         00:09:54.204 | Added          | 54 (0xd800) |        none | inf -> inf |
 * |         00:10:20.645 | NextHopChanged | 34 (0x8800) | 34 (0x8800) | inf ->   2 |
 * |         00:10:24.242 | NextHopChanged | 59 (0xec00) | 59 (0xec00) | inf ->   1 |
 * |         00:10:24.242 | Added          | 34 (0x8800) |        none | inf -> inf |
 * |         00:10:41.900 | NextHopChanged | 59 (0xec00) |        none |   1 -> inf |
 * |         00:10:42.480 | Added          |  3 (0x0c00) |  3 (0x0c00) | inf -> inf |
 * |         00:10:43.614 | Added          | 59 (0xec00) | 59 (0xec00) | inf ->   1 |
 * Done
 * @endcode
 * @code
 * history router list 20
 * 00:00:06.959 -> event:NextHopChanged router:7(0x1c00) nexthop:34(0x8800) old-cost:inf new-cost:3
 * 00:00:10.305 -> event:NextHopChanged router:34(0x8800) nexthop:34(0x8800) old-cost:inf new-cost:2
 * 00:00:10.305 -> event:Added router:7(0x1c00) nexthop:none old-cost:inf new-cost:inf
 * 00:00:13.632 -> event:Added router:34(0x8800) nexthop:none old-cost:inf new-cost:inf
 * 00:00:16.649 -> event:Removed router:59(0xec00) nexthop:none old-cost:inf new-cost:inf
 * 00:00:16.649 -> event:Removed router:54(0xd800) nexthop:none old-cost:inf new-cost:inf
 * 00:00:16.649 -> event:Removed router:34(0x8800) nexthop:none old-cost:inf new-cost:inf
 * 00:00:16.649 -> event:Removed router:7(0x1c00) nexthop:none old-cost:inf new-cost:inf
 * 00:00:56.496 -> event:NextHopChanged router:59(0xec00) nexthop:34(0x8800) old-cost:1 new-cost:5
 * 00:02:35.436 -> event:NextHopChanged router:54(0xd800) nexthop:none old-cost:15 new-cost:inf
 * 00:03:12.616 -> event:CostChanged router:54(0xd800) nexthop:34(0x8800) old-cost:13 new-cost:15
 * 00:03:47.417 -> event:NextHopChanged router:54(0xd800) nexthop:34(0x8800) old-cost:15 new-cost:13
 * 00:03:47.889 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:13 new-cost:15
 * 00:04:20.825 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:11 new-cost:13
 * 00:04:53.709 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:9 new-cost:11
 * 00:05:24.877 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:7 new-cost:9
 * 00:05:52.782 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:5 new-cost:7
 * 00:06:50.422 -> event:CostChanged router:54(0xd800) nexthop:59(0xec00) old-cost:3 new-cost:5
 * 00:07:15.493 -> event:NextHopChanged router:54(0xd800) nexthop:59(0xec00) old-cost:1 new-cost:3
 * 00:09:30.382 -> event:NextHopChanged router:7(0x1c00) nexthop:34(0x8800) old-cost:inf new-cost:3
 * Done
 * @endcode
 * @cparam history router [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the route-table history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Event: Possible values are `Added`, `Removed`, `NextHopChanged`, or `CostChanged`.
 * * ID (RLOC16): Router ID and RLOC16 of the router.
 * * Next Hop: Router ID and RLOC16 of the next hop. If there is no next hop,
 *             `none` is shown.
 * * Path Cost: old cost `->` new cost. A value of `inf` indicates an infinite
 *      	path cost.
 * @sa otHistoryTrackerIterateRouterHistory
 */
template <> otError History::Process<Cmd("router")>(Arg aArgs[])
{
    static const char *const kEventString[] = {
        /* (0) OT_HISTORY_TRACKER_ROUTER_EVENT_ADDED             -> */ "Added",
        /* (1) OT_HISTORY_TRACKER_ROUTER_EVENT_REMOVED           -> */ "Removed",
        /* (2) OT_HISTORY_TRACKER_ROUTER_EVENT_NEXT_HOP_CHANGED  -> */ "NextHopChanged",
        /* (3) OT_HISTORY_TRACKER_ROUTER_EVENT_COST_CHANGED      -> */ "CostChanged",
    };

    constexpr uint8_t kRouterIdOffset = 10; // Bit offset of Router ID in RLOC16

    otError                           error;
    bool                              isList;
    uint16_t                          numEntries;
    otHistoryTrackerIterator          iterator;
    const otHistoryTrackerRouterInfo *info;
    uint32_t                          entryAge;
    char                              ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];

    static_assert(0 == OT_HISTORY_TRACKER_ROUTER_EVENT_ADDED, "EVENT_ADDED is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_ROUTER_EVENT_REMOVED, "EVENT_REMOVED is incorrect");
    static_assert(2 == OT_HISTORY_TRACKER_ROUTER_EVENT_NEXT_HOP_CHANGED, "EVENT_NEXT_HOP_CHANGED is incorrect");
    static_assert(3 == OT_HISTORY_TRACKER_ROUTER_EVENT_COST_CHANGED, "EVENT_COST_CHANGED is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Event          | ID (RlOC16) | Next Hop   | Path Cost   |
        // +----------------------+----------------+-------------+------------+-------------+

        static const char *const kRouterInfoTitles[] = {
            "Age", "Event", "ID (RLOC16)", "Next Hop", "Path Cost",
        };

        static const uint8_t kRouterInfoColumnWidths[] = {22, 16, 13, 13, 12};

        OutputTableHeader(kRouterInfoTitles, kRouterInfoColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateRouterHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        OutputFormat(isList ? "%s -> event:%s router:%u(0x%04x) nexthop:" : "| %20s | %-14s | %2u (0x%04x) | ",
                     ageString, kEventString[info->mEvent], info->mRouterId,
                     static_cast<uint16_t>(info->mRouterId) << kRouterIdOffset);

        if (info->mNextHop != OT_HISTORY_TRACKER_NO_NEXT_HOP)
        {
            OutputFormat(isList ? "%u(0x%04x)" : "%2u (0x%04x)", info->mNextHop,
                         static_cast<uint16_t>(info->mNextHop) << kRouterIdOffset);
        }
        else
        {
            OutputFormat(isList ? "%s" : "%11s", "none");
        }

        if (info->mOldPathCost != OT_HISTORY_TRACKER_INFINITE_PATH_COST)
        {
            OutputFormat(isList ? " old-cost:%u" : " | %3u ->", info->mOldPathCost);
        }
        else
        {
            OutputFormat(isList ? " old-cost:inf" : " | inf ->");
        }

        if (info->mPathCost != OT_HISTORY_TRACKER_INFINITE_PATH_COST)
        {
            OutputLine(isList ? " new-cost:%u" : " %3u |", info->mPathCost);
        }
        else
        {
            OutputLine(isList ? " new-cost:inf" : " inf |");
        }
    }

exit:
    return error;
}

/**
 * @cli history netinfo
 * @code
 * history netinfo
 * | Age                  | Role     | Mode | RLOC16 | Partition ID |
 * +----------------------+----------+------+--------+--------------+
 * |         00:00:10.069 | router   | rdn  | 0x6000 |    151029327 |
 * |         00:02:09.337 | child    | rdn  | 0x2001 |    151029327 |
 * |         00:02:09.338 | child    | rdn  | 0x2001 |    151029327 |
 * |         00:07:40.806 | child    | -    | 0x2001 |    151029327 |
 * |         00:07:42.297 | detached | -    | 0x6000 |            0 |
 * |         00:07:42.968 | disabled | -    | 0x6000 |            0 |
 * Done
 * @endcode
 * @code
 * history netinfo list
 * 00:00:59.467 -> role:router mode:rdn rloc16:0x6000 partition-id:151029327
 * 00:02:58.735 -> role:child mode:rdn rloc16:0x2001 partition-id:151029327
 * 00:02:58.736 -> role:child mode:rdn rloc16:0x2001 partition-id:151029327
 * 00:08:30.204 -> role:child mode:- rloc16:0x2001 partition-id:151029327
 * 00:08:31.695 -> role:detached mode:- rloc16:0x6000 partition-id:0
 * 00:08:32.366 -> role:disabled mode:- rloc16:0x6000 partition-id:0
 * Done
 * @endcode
 * @code
 * history netinfo 2
 * | Age                  | Role     | Mode | RLOC16 | Partition ID |
 * +----------------------+----------+------+--------+--------------+
 * |         00:02:05.451 | router   | rdn  | 0x6000 |    151029327 |
 * |         00:04:04.719 | child    | rdn  | 0x2001 |    151029327 |
 * Done
 * @endcode
 * @cparam history netinfo [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the network info history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Role: Device role. Possible values are `router`, `child`, `detached`, or `disabled`.
 * * Mode: MLE link mode. Possible values:
 *     * `-`: no flags set (rx-off-when-idle, minimal Thread device,
 *       stable network data).
 *     * `r`: rx-on-when-idle
 *     * `d`: Full Thread Device.
 *     * `n`: Full Network Data
 * * RLOC16
 * * Partition ID.
 * @sa otHistoryTrackerIterateNetInfoHistory
 */
template <> otError History::Process<Cmd("netinfo")>(Arg aArgs[])
{
    otError                            error;
    bool                               isList;
    uint16_t                           numEntries;
    otHistoryTrackerIterator           iterator;
    const otHistoryTrackerNetworkInfo *info;
    uint32_t                           entryAge;
    char                               ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                               linkModeString[Interpreter::kLinkModeStringSize];

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Role     | Mode | RLOC16 | Partition ID |
        // +----------------------+----------+------+--------+--------------+

        static const char *const kNetInfoTitles[]       = {"Age", "Role", "Mode", "RLOC16", "Partition ID"};
        static const uint8_t     kNetInfoColumnWidths[] = {22, 10, 6, 8, 14};

        OutputTableHeader(kNetInfoTitles, kNetInfoColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateNetInfoHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        OutputLine(
            isList ? "%s -> role:%s mode:%s rloc16:0x%04x partition-id:%lu" : "| %20s | %-8s | %-4s | 0x%04x | %12lu |",
            ageString, otThreadDeviceRoleToString(info->mRole),
            Interpreter::LinkModeToString(info->mMode, linkModeString), info->mRloc16, ToUlong(info->mPartitionId));
    }

exit:
    return error;
}

/**
 * @cli history rx
 * @code
 * history rx
 * | Age                  | Type             | Len   | Chksum | Sec | Prio | RSS  |Dir | Neighb | Radio |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xbd26 |  no |  net |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:07.640 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | HopOpts          |    44 | 0x0000 | yes | norm |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:09.263 | src: [fdde:ad00:beef:0:0:ff:fe00:4800]:0                                    |
 * |                      | dst: [ff03:0:0:0:0:0:0:2]:0                                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    12 | 0x3f7d | yes |  net |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:09.302 | src: [fdde:ad00:beef:0:0:ff:fe00:4800]:61631                                |
 * |                      | dst: [fdde:ad00:beef:0:0:ff:fe00:4801]:61631                                |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | ICMP6(EchoReqst) |    16 | 0x942c | yes | norm |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:09.304 | src: [fdde:ad00:beef:0:ac09:a16b:3204:dc09]:0                               |
 * |                      | dst: [fdde:ad00:beef:0:dc0e:d6b3:f180:b75b]:0                               |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | HopOpts          |    44 | 0x0000 | yes | norm |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:09.304 | src: [fdde:ad00:beef:0:0:ff:fe00:4800]:0                                    |
 * |                      | dst: [ff03:0:0:0:0:0:0:2]:0                                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0x2e37 |  no |  net |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:21.622 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xe177 |  no |  net |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:26.640 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |   165 | 0x82ee | yes |  net |  -20 | RX | 0x4800 |  15.4 |
 * |         00:00:30.000 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788                                  |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    93 | 0x52df |  no |  net |  -20 | RX | unknwn |  15.4 |
 * |         00:00:30.480 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788                                  |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0x5ccf |  no |  net |  -20 | RX | unknwn |  15.4 |
 * |         00:00:30.772 | src: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * Done
 * @endcode
 * @code
 * history rx list 4
 * 00:00:13.368
       type:UDP len:50 checksum:0xbd26 sec:no prio:net rss:-20 from:0x4800 radio:15.4
       src:[fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788
       dst:[ff02:0:0:0:0:0:0:1]:19788
 * 00:00:14.991
       type:HopOpts len:44 checksum:0x0000 sec:yes prio:norm rss:-20 from:0x4800 radio:15.4
       src:[fdde:ad00:beef:0:0:ff:fe00:4800]:0
       dst:[ff03:0:0:0:0:0:0:2]:0
 * 00:00:15.030
       type:UDP len:12 checksum:0x3f7d sec:yes prio:net rss:-20 from:0x4800 radio:15.4
       src:[fdde:ad00:beef:0:0:ff:fe00:4800]:61631
       dst:[fdde:ad00:beef:0:0:ff:fe00:4801]:61631
 * 00:00:15.032
       type:ICMP6(EchoReqst) len:16 checksum:0x942c sec:yes prio:norm rss:-20 from:0x4800 radio:15.4
       src:[fdde:ad00:beef:0:ac09:a16b:3204:dc09]:0
       dst:[fdde:ad00:beef:0:dc0e:d6b3:f180:b75b]:0
 * Done
 * @endcode
 * @cparam history rx [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the IPv6 message RX history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Type:
 *     * IPv6 message type, such as `UDP`, `TCP`, `HopOpts`, and `ICMP6` (and its subtype).
 *     * `src`: Source IPv6 address and port number.
 *     * `dst`: Destination IPv6 address and port number (port number is valid
         for UDP/TCP, otherwise it is 0).
 * * Len: IPv6 payload length (excluding the IPv6 header).
 * * Chksum: Message checksum (valid for UDP, TCP, or ICMP6 messages).
 * * Sec: Indicates if link-layer security was used.
 * * Prio: Message priority. Possible values are `low`, `norm`, `high`, or
 *         `net` (for Thread control messages).
 * * RSS: Received Signal Strength (in dBm), averaged over all received fragment
 *        frames that formed the message. For TX history, `NA` (not applicable)
          is displayed.
 * * Dir: Shows whether the message was sent (`TX`) or received (`RX`). A failed
 *        transmission is indicated with `TX-F` in table format or
 *        `tx-success:no` in list format. Examples of a failed transmission
 *        include a `tx`getting aborted and no `ack` getting sent from the peer for
 *        any of the message fragments.
 * * Neighb: Short address (RLOC16) of the neighbor with whom the message was
 *           sent/received. If the frame was broadcast, it is shown as
 *           `bcast` in table format or `0xffff` in list format. If the short
 *           address of the neighbor is not available, it is shown as `unknwn` in
 *           table format or `0xfffe` in list format.
 * * Radio: Radio link on which the message was sent/received (useful when
            `OPENTHREAD_CONFIG_MULTI_RADIO` is enabled). Can be `15.4`, `trel`,
            or `all` (if sent on all radio links).
 * @sa otHistoryTrackerIterateRxHistory
 */
template <> otError History::Process<Cmd("rx")>(Arg aArgs[]) { return ProcessRxTxHistory(kRx, aArgs); }

/**
 * @cli history rxtx
 * @code
 * history rxtx
 * | Age                  | Type             | Len   | Chksum | Sec | Prio | RSS  |Dir | Neighb | Radio |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | HopOpts          |    44 | 0x0000 | yes | norm |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:09.267 | src: [fdde:ad00:beef:0:0:ff:fe00:800]:0                                     |
 * |                      | dst: [ff03:0:0:0:0:0:0:2]:0                                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    12 | 0x6c6b | yes |  net |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:09.290 | src: [fdde:ad00:beef:0:0:ff:fe00:800]:61631                                 |
 * |                      | dst: [fdde:ad00:beef:0:0:ff:fe00:801]:61631                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | ICMP6(EchoReqst) |    16 | 0xc6a2 | yes | norm |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:09.292 | src: [fdde:ad00:beef:0:efe8:4910:cf95:dee9]:0                               |
 * |                      | dst: [fdde:ad00:beef:0:af4c:3644:882a:3698]:0                               |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | ICMP6(EchoReply) |    16 | 0xc5a2 | yes | norm |  NA  | TX | 0x0800 |  15.4 |
 * |         00:00:09.292 | src: [fdde:ad00:beef:0:af4c:3644:882a:3698]:0                               |
 * |                      | dst: [fdde:ad00:beef:0:efe8:4910:cf95:dee9]:0                               |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xaa0d | yes |  net |  NA  | TX | 0x0800 |  15.4 |
 * |         00:00:09.294 | src: [fdde:ad00:beef:0:0:ff:fe00:801]:61631                                 |
 * |                      | dst: [fdde:ad00:beef:0:0:ff:fe00:800]:61631                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | HopOpts          |    44 | 0x0000 | yes | norm |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:09.296 | src: [fdde:ad00:beef:0:0:ff:fe00:800]:0                                     |
 * |                      | dst: [ff03:0:0:0:0:0:0:2]:0                                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xc1d8 |  no |  net |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:09.569 | src: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0x3cb1 |  no |  net |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:16.519 | src: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xeda0 |  no |  net |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:20.599 | src: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:1]:19788                                             |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |   165 | 0xbdfa | yes |  net |  -20 | RX | 0x0800 |  15.4 |
 * |         00:00:21.059 | src: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * |                      | dst: [fe80:0:0:0:8893:c2cc:d983:1e1c]:19788                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    64 | 0x1c11 |  no |  net |  NA  | TX | 0x0800 |  15.4 |
 * |         00:00:21.062 | src: [fe80:0:0:0:8893:c2cc:d983:1e1c]:19788                                 |
 * |                      | dst: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    93 | 0xedff |  no |  net |  -20 | RX | unknwn |  15.4 |
 * |         00:00:21.474 | src: [fe80:0:0:0:54d9:5153:ffc6:df26]:19788                                 |
 * |                      | dst: [fe80:0:0:0:8893:c2cc:d983:1e1c]:19788                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    44 | 0xd383 |  no |  net |  NA  | TX | bcast  |  15.4 |
 * |         00:00:21.811 | src: [fe80:0:0:0:8893:c2cc:d983:1e1c]:19788                                 |
 * |                      | dst: [ff02:0:0:0:0:0:0:2]:19788                                             |
 * Done
 * @endcode
 * @code
 * history rxtx list 5
 * 00:00:02.100
       type:UDP len:50 checksum:0xd843 sec:no prio:net rss:-20 from:0x0800 radio:15.4
       src:[fe80:0:0:0:54d9:5153:ffc6:df26]:19788
       dst:[ff02:0:0:0:0:0:0:1]:19788
 * 00:00:15.331
       type:HopOpts len:44 checksum:0x0000 sec:yes prio:norm rss:-20 from:0x0800 radio:15.4
       src:[fdde:ad00:beef:0:0:ff:fe00:800]:0
       dst:[ff03:0:0:0:0:0:0:2]:0
 * 00:00:15.354
       type:UDP len:12 checksum:0x6c6b sec:yes prio:net rss:-20 from:0x0800 radio:15.4
       src:[fdde:ad00:beef:0:0:ff:fe00:800]:61631
       dst:[fdde:ad00:beef:0:0:ff:fe00:801]:61631
 * 00:00:15.356
       type:ICMP6(EchoReqst) len:16 checksum:0xc6a2 sec:yes prio:norm rss:-20 from:0x0800 radio:15.4
       src:[fdde:ad00:beef:0:efe8:4910:cf95:dee9]:0
       dst:[fdde:ad00:beef:0:af4c:3644:882a:3698]:0
 * 00:00:15.356
       type:ICMP6(EchoReply) len:16 checksum:0xc5a2 sec:yes prio:norm tx-success:yes to:0x0800 radio:15.4
       src:[fdde:ad00:beef:0:af4c:3644:882a:3698]:0
       dst:[fdde:ad00:beef:0:efe8:4910:cf95:dee9]:0
 * Done
 * @endcode
 * @cparam history rxtx [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the combined IPv6 message RX and TX history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Type:
 *     * IPv6 message type, such as `UDP`, `TCP`, `HopOpts`, and `ICMP6` (and its subtype).
 *     * `src`: Source IPv6 address and port number.
 *     * `dst`: Destination IPv6 address and port number (port number is valid
         for UDP/TCP, otherwise it is 0).
 * * Len: IPv6 payload length (excluding the IPv6 header).
 * * Chksum: Message checksum (valid for UDP, TCP, or ICMP6 messages).
 * * Sec: Indicates if link-layer security was used.
 * * Prio: Message priority. Possible values are `low`, `norm`, `high`, or
 *         `net` (for Thread control messages).
 * * RSS: Received Signal Strength (in dBm), averaged over all received fragment
 *        frames that formed the message. For TX history, `NA` (not applicable)
          is displayed.
 * * Dir: Shows whether the message was sent (`TX`) or received (`RX`). A failed
 *        transmission is indicated with `TX-F` in table format or
 *        `tx-success:no` in list format. Examples of a failed transmission
 *        include a `tx`getting aborted and no `ack` getting sent from the peer for
 *        any of the message fragments.
 * * Neighb: Short address (RLOC16) of the neighbor with whom the message was
 *           sent/received. If the frame was broadcast, it is shown as
 *           `bcast` in table format or `0xffff` in list format. If the short
 *           address of the neighbor is not available, it is shown as `unknwn` in
 *           table format or `0xfffe` in list format.
 * * Radio: Radio link on which the message was sent/received (useful when
            `OPENTHREAD_CONFIG_MULTI_RADIO` is enabled). Can be `15.4`, `trel`,
            or `all` (if sent on all radio links).
 * @sa otHistoryTrackerIterateRxHistory
 * @sa otHistoryTrackerIterateTxHistory
 */
template <> otError History::Process<Cmd("rxtx")>(Arg aArgs[]) { return ProcessRxTxHistory(kRxTx, aArgs); }

/**
 * @cli history tx
 * @code
 * history tx
 * | Age                  | Type             | Len   | Chksum | Sec | Prio | RSS  |Dir | Neighb | Radio |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | ICMP6(EchoReply) |    16 | 0x932c | yes | norm |  NA  | TX | 0x4800 |  15.4 |
 * |         00:00:18.798 | src: [fdde:ad00:beef:0:dc0e:d6b3:f180:b75b]:0                               |
 * |                      | dst: [fdde:ad00:beef:0:ac09:a16b:3204:dc09]:0                               |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    50 | 0xce87 | yes |  net |  NA  | TX | 0x4800 |  15.4 |
 * |         00:00:18.800 | src: [fdde:ad00:beef:0:0:ff:fe00:4801]:61631                                |
 * |                      | dst: [fdde:ad00:beef:0:0:ff:fe00:4800]:61631                                |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    64 | 0xf7ba |  no |  net |  NA  | TX | 0x4800 |  15.4 |
 * |         00:00:39.499 | src: [fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788                                  |
 * |                      | dst: [fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788                                 |
 * +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+
 * |                      | UDP              |    44 | 0x26d4 |  no |  net |  NA  | TX | bcast  |  15.4 |
 * |         00:00:40.256 | src: [fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788                                  |
 * |                      | dst: [ff02:0:0:0:0:0:0:2]:19788                                             |
 * Done
 * @endcode
 * @code
 * history tx list
 * 00:00:23.957
       type:ICMP6(EchoReply) len:16 checksum:0x932c sec:yes prio:norm tx-success:yes to:0x4800 radio:15.4
       src:[fdde:ad00:beef:0:dc0e:d6b3:f180:b75b]:0
       dst:[fdde:ad00:beef:0:ac09:a16b:3204:dc09]:0
 * 00:00:23.959
       type:UDP len:50 checksum:0xce87 sec:yes prio:net tx-success:yes to:0x4800 radio:15.4
       src:[fdde:ad00:beef:0:0:ff:fe00:4801]:61631
       dst:[fdde:ad00:beef:0:0:ff:fe00:4800]:61631
 * 00:00:44.658
       type:UDP len:64 checksum:0xf7ba sec:no prio:net tx-success:yes to:0x4800 radio:15.4
       src:[fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788
       dst:[fe80:0:0:0:d03d:d3e7:cc5e:7cd7]:19788
 * 00:00:45.415
       type:UDP len:44 checksum:0x26d4 sec:no prio:net tx-success:yes to:0xffff radio:15.4
       src:[fe80:0:0:0:a4a5:bbac:a8e:bd07]:19788
       dst:[ff02:0:0:0:0:0:0:2]:19788
 * Done
 * @endcode
 * @cparam history tx [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the IPv6 message TX history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Type:
 *     * IPv6 message type, such as `UDP`, `TCP`, `HopOpts`, and `ICMP6` (and its subtype).
 *     * `src`: Source IPv6 address and port number.
 *     * `dst`: Destination IPv6 address and port number (port number is valid
         for UDP/TCP, otherwise it is 0).
 * * Len: IPv6 payload length (excluding the IPv6 header).
 * * Chksum: Message checksum (valid for UDP, TCP, or ICMP6 messages).
 * * Sec: Indicates if link-layer security was used.
 * * Prio: Message priority. Possible values are `low`, `norm`, `high`, or
 *         `net` (for Thread control messages).
 * * RSS: Received Signal Strength (in dBm), averaged over all received fragment
 *        frames that formed the message. For TX history, `NA` (not applicable)
          is displayed.
 * * Dir: Shows whether the message was sent (`TX`) or received (`RX`). A failed
 *        transmission is indicated with `TX-F` in table format or
 *        `tx-success:no` in list format. Examples of a failed transmission
 *        include a `tx`getting aborted and no `ack` getting sent from the peer for
 *        any of the message fragments.
 * * Neighb: Short address (RLOC16) of the neighbor with whom the message was
 *           sent/received. If the frame was broadcast, it is shown as
 *           `bcast` in table format or `0xffff` in list format. If the short
 *           address of the neighbor is not available, it is shown as `unknwn` in
 *           table format or `0xfffe` in list format.
 * * Radio: Radio link on which the message was sent/received (useful when
            `OPENTHREAD_CONFIG_MULTI_RADIO` is enabled). Can be `15.4`, `trel`,
            or `all` (if sent on all radio links).
 * @sa otHistoryTrackerIterateTxHistory
 */
template <> otError History::Process<Cmd("tx")>(Arg aArgs[]) { return ProcessRxTxHistory(kTx, aArgs); }

const char *History::MessagePriorityToString(uint8_t aPriority)
{
    static const char *const kPriorityStrings[] = {
        "low",  // (0) OT_HISTORY_TRACKER_MSG_PRIORITY_LOW
        "norm", // (1) OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL
        "high", // (2) OT_HISTORY_TRACKER_MSG_PRIORITY_HIGH
        "net",  // (3) OT_HISTORY_TRACKER_MSG_PRIORITY_NET
    };

    static_assert(0 == OT_HISTORY_TRACKER_MSG_PRIORITY_LOW, "MSG_PRIORITY_LOW value is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_MSG_PRIORITY_NORMAL, "MSG_PRIORITY_NORMAL value is incorrect");
    static_assert(2 == OT_HISTORY_TRACKER_MSG_PRIORITY_HIGH, "MSG_PRIORITY_HIGH value is incorrect");
    static_assert(3 == OT_HISTORY_TRACKER_MSG_PRIORITY_NET, "MSG_PRIORITY_NET value is incorrect");

    return Stringify(aPriority, kPriorityStrings, "unkn");
}

const char *History::RadioTypeToString(const otHistoryTrackerMessageInfo &aInfo)
{
    const char *str = "none";

    if (aInfo.mRadioTrelUdp6 && aInfo.mRadioIeee802154)
    {
        str = "all";
    }
    else if (aInfo.mRadioIeee802154)
    {
        str = "15.4";
    }
    else if (aInfo.mRadioTrelUdp6)
    {
        str = "trel";
    }

    return str;
}

const char *History::MessageTypeToString(const otHistoryTrackerMessageInfo &aInfo)
{
    const char *str = otIp6ProtoToString(aInfo.mIpProto);

    if (aInfo.mIpProto == OT_IP6_PROTO_ICMP6)
    {
        switch (aInfo.mIcmp6Type)
        {
        case OT_ICMP6_TYPE_DST_UNREACH:
            str = "ICMP6(Unreach)";
            break;
        case OT_ICMP6_TYPE_PACKET_TO_BIG:
            str = "ICMP6(TooBig)";
            break;
        case OT_ICMP6_TYPE_ECHO_REQUEST:
            str = "ICMP6(EchoReqst)";
            break;
        case OT_ICMP6_TYPE_ECHO_REPLY:
            str = "ICMP6(EchoReply)";
            break;
        case OT_ICMP6_TYPE_ROUTER_SOLICIT:
            str = "ICMP6(RouterSol)";
            break;
        case OT_ICMP6_TYPE_ROUTER_ADVERT:
            str = "ICMP6(RouterAdv)";
            break;
        default:
            str = "ICMP6(Other)";
            break;
        }
    }

    return str;
}

otError History::ProcessRxTxHistory(RxTx aRxTx, Arg aArgs[])
{
    otError                            error;
    bool                               isList;
    uint16_t                           numEntries;
    otHistoryTrackerIterator           rxIterator;
    otHistoryTrackerIterator           txIterator;
    bool                               isRx   = false;
    const otHistoryTrackerMessageInfo *info   = nullptr;
    const otHistoryTrackerMessageInfo *rxInfo = nullptr;
    const otHistoryTrackerMessageInfo *txInfo = nullptr;
    uint32_t                           entryAge;
    uint32_t                           rxEntryAge;
    uint32_t                           txEntryAge;

    // | Age                  | Type             | Len   | Chksum | Sec | Prio | RSS  |Dir | Neighb | Radio |
    // +----------------------+------------------+-------+--------+-----+------+------+----+--------+-------+

    static const char *const kTableTitles[] = {"Age",  "Type", "Len", "Chksum", "Sec",
                                               "Prio", "RSS",  "Dir", "Neighb", "Radio"};

    static const uint8_t kTableColumnWidths[] = {22, 18, 7, 8, 5, 6, 6, 4, 8, 7};

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        OutputTableHeader(kTableTitles, kTableColumnWidths);
    }

    otHistoryTrackerInitIterator(&txIterator);
    otHistoryTrackerInitIterator(&rxIterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        switch (aRxTx)
        {
        case kRx:
            info = otHistoryTrackerIterateRxHistory(GetInstancePtr(), &rxIterator, &entryAge);
            isRx = true;
            break;

        case kTx:
            info = otHistoryTrackerIterateTxHistory(GetInstancePtr(), &txIterator, &entryAge);
            isRx = false;
            break;

        case kRxTx:
            // Iterate through both RX and TX lists and determine the entry
            // with earlier age.

            if (rxInfo == nullptr)
            {
                rxInfo = otHistoryTrackerIterateRxHistory(GetInstancePtr(), &rxIterator, &rxEntryAge);
            }

            if (txInfo == nullptr)
            {
                txInfo = otHistoryTrackerIterateTxHistory(GetInstancePtr(), &txIterator, &txEntryAge);
            }

            if ((rxInfo != nullptr) && ((txInfo == nullptr) || (rxEntryAge <= txEntryAge)))
            {
                info     = rxInfo;
                entryAge = rxEntryAge;
                isRx     = true;
                rxInfo   = nullptr;
            }
            else
            {
                info     = txInfo;
                entryAge = txEntryAge;
                isRx     = false;
                txInfo   = nullptr;
            }

            break;
        }

        VerifyOrExit(info != nullptr);

        if (isList)
        {
            OutputRxTxEntryListFormat(*info, entryAge, isRx);
        }
        else
        {
            if (index != 0)
            {
                OutputTableSeparator(kTableColumnWidths);
            }

            OutputRxTxEntryTableFormat(*info, entryAge, isRx);
        }
    }

exit:
    return error;
}

void History::OutputRxTxEntryListFormat(const otHistoryTrackerMessageInfo &aInfo, uint32_t aEntryAge, bool aIsRx)
{
    constexpr uint8_t kIndentSize = 4;

    char ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];

    otHistoryTrackerEntryAgeToString(aEntryAge, ageString, sizeof(ageString));

    OutputLine("%s", ageString);
    OutputFormat(kIndentSize, "type:%s len:%u checksum:0x%04x sec:%s prio:%s ", MessageTypeToString(aInfo),
                 aInfo.mPayloadLength, aInfo.mChecksum, aInfo.mLinkSecurity ? "yes" : "no",
                 MessagePriorityToString(aInfo.mPriority));
    if (aIsRx)
    {
        OutputFormat("rss:%d", aInfo.mAveRxRss);
    }
    else
    {
        OutputFormat("tx-success:%s", aInfo.mTxSuccess ? "yes" : "no");
    }

    OutputLine(" %s:0x%04x radio:%s", aIsRx ? "from" : "to", aInfo.mNeighborRloc16, RadioTypeToString(aInfo));

    OutputFormat(kIndentSize, "src:");
    OutputSockAddrLine(aInfo.mSource);

    OutputFormat(kIndentSize, "dst:");
    OutputSockAddrLine(aInfo.mDestination);
}

void History::OutputRxTxEntryTableFormat(const otHistoryTrackerMessageInfo &aInfo, uint32_t aEntryAge, bool aIsRx)
{
    char ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char addrString[OT_IP6_SOCK_ADDR_STRING_SIZE];

    otHistoryTrackerEntryAgeToString(aEntryAge, ageString, sizeof(ageString));

    OutputFormat("| %20s | %-16.16s | %5u | 0x%04x | %3s | %4s | ", "", MessageTypeToString(aInfo),
                 aInfo.mPayloadLength, aInfo.mChecksum, aInfo.mLinkSecurity ? "yes" : "no",
                 MessagePriorityToString(aInfo.mPriority));

    if (aIsRx)
    {
        OutputFormat("%4d | RX ", aInfo.mAveRxRss);
    }
    else
    {
        OutputFormat(" NA  |");
        OutputFormat(aInfo.mTxSuccess ? " TX " : "TX-F");
    }

    if (aInfo.mNeighborRloc16 == kShortAddrBroadcast)
    {
        OutputFormat("| bcast  ");
    }
    else if (aInfo.mNeighborRloc16 == kShortAddrInvalid)
    {
        OutputFormat("| unknwn ");
    }
    else
    {
        OutputFormat("| 0x%04x ", aInfo.mNeighborRloc16);
    }

    OutputLine("| %5.5s |", RadioTypeToString(aInfo));

    otIp6SockAddrToString(&aInfo.mSource, addrString, sizeof(addrString));
    OutputLine("| %20s | src: %-70s |", ageString, addrString);

    otIp6SockAddrToString(&aInfo.mDestination, addrString, sizeof(addrString));
    OutputLine("| %20s | dst: %-70s |", "", addrString);
}

/**
 * @cli history prefix
 * @code
 * history prefix
 * | Age                  | Event   | Prefix                                      | Flags     | Pref | RLOC16 |
 * +----------------------+---------+---------------------------------------------+-----------+------+--------+
 * |         00:00:10.663 | Added   | fd00:1111:2222:3333::/64                    | paro      | med  | 0x5400 |
 * |         00:01:02.054 | Removed | fd00:dead:beef:1::/64                       | paros     | high | 0x5400 |
 * |         00:01:21.136 | Added   | fd00:abba:cddd:0::/64                       | paos      | med  | 0x5400 |
 * |         00:01:45.144 | Added   | fd00:dead:beef:1::/64                       | paros     | high | 0x3c00 |
 * |         00:01:50.944 | Added   | fd00:dead:beef:1::/64                       | paros     | high | 0x5400 |
 * |         00:01:59.887 | Added   | fd00:dead:beef:1::/64                       | paros     | med  | 0x8800 |
 * Done
 * @endcode
 * @code
 * history prefix list
 * 00:04:12.487 -> event:Added prefix:fd00:1111:2222:3333::/64 flags:paro pref:med rloc16:0x5400
 * 00:05:03.878 -> event:Removed prefix:fd00:dead:beef:1::/64 flags:paros pref:high rloc16:0x5400
 * 00:05:22.960 -> event:Added prefix:fd00:abba:cddd:0::/64 flags:paos pref:med rloc16:0x5400
 * 00:05:46.968 -> event:Added prefix:fd00:dead:beef:1::/64 flags:paros pref:high rloc16:0x3c00
 * 00:05:52.768 -> event:Added prefix:fd00:dead:beef:1::/64 flags:paros pref:high rloc16:0x5400
 * 00:06:01.711 -> event:Added prefix:fd00:dead:beef:1::/64 flags:paros pref:med rloc16:0x8800
 * Done
 * @endcode
 * @cparam history prefix [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the network data for the mesh prefix history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Event: Possible values are `Added` or `Removed`.
 * * Prefix
 * * Flags/meaning:
 *     * `p`: Preferred flag
 *     * `a`: Stateless IPv6 address auto-configuration flag.
 *     * `d`: DHCPv6 IPv6 address configuration flag.
 *     * `c`: DHCPv6 other-configuration flag.
 *     * `r`: Default route flag.
 *     * `o`: On mesh flag.
 *     * `s`: Stable flag.
 *     * `n`: Nd Dns flag.
 *     * `D`: Domain prefix flag.
 * * Pref: Preference. Values can be either `high`, `med`, or `low`.
 * * RLOC16
 * @sa otHistoryTrackerIterateOnMeshPrefixHistory
 */
template <> otError History::Process<Cmd("prefix")>(Arg aArgs[])
{
    otError                                 error;
    bool                                    isList;
    uint16_t                                numEntries;
    otHistoryTrackerIterator                iterator;
    const otHistoryTrackerOnMeshPrefixInfo *info;
    uint32_t                                entryAge;
    char                                    ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                                    prefixString[OT_IP6_PREFIX_STRING_SIZE];
    NetworkData::FlagsString                flagsString;

    static_assert(0 == OT_HISTORY_TRACKER_NET_DATA_ENTRY_ADDED, "NET_DATA_ENTRY_ADDED value is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_NET_DATA_ENTRY_REMOVED, "NET_DATA_ENTRY_REMOVED value is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Event   | Prefix                                      | Flags     | Pref | RLOC16 |
        // +----------------------+---------+---------------------------------------------+-----------+------+--------+

        static const char *const kPrefixTitles[]       = {"Age", "Event", "Prefix", "Flags", "Pref", "RLOC16"};
        static const uint8_t     kPrefixColumnWidths[] = {22, 9, 45, 11, 6, 8};

        OutputTableHeader(kPrefixTitles, kPrefixColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateOnMeshPrefixHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        otIp6PrefixToString(&info->mPrefix.mPrefix, prefixString, sizeof(prefixString));
        NetworkData::PrefixFlagsToString(info->mPrefix, flagsString);

        OutputLine(isList ? "%s -> event:%s prefix:%s flags:%s pref:%s rloc16:0x%04x"
                          : "| %20s | %-7s | %-43s | %-9s | %-4s | 0x%04x |",
                   ageString, Stringify(info->mEvent, kSimpleEventStrings), prefixString, flagsString,
                   PreferenceToString(info->mPrefix.mPreference), info->mPrefix.mRloc16);
    }

exit:
    return error;
}

/**
 * @cli history route
 * @code
 * history route
 * | Age                  | Event   | Route                                       | Flags     | Pref | RLOC16 |
 * +----------------------+---------+---------------------------------------------+-----------+------+--------+
 * |         00:00:05.456 | Removed | fd00:1111:0::/48                            | s         | med  | 0x3c00 |
 * |         00:00:29.310 | Added   | fd00:1111:0::/48                            | s         | med  | 0x3c00 |
 * |         00:00:42.822 | Added   | fd00:1111:0::/48                            | s         | med  | 0x5400 |
 * |         00:01:27.688 | Added   | fd00:aaaa:bbbb:cccc::/64                    | s         | med  | 0x8800 |
 * Done
 * @endcode
 * @code
 * history route list 2
 * 00:00:48.704 -> event:Removed route:fd00:1111:0::/48 flags:s pref:med rloc16:0x3c00
 * 00:01:12.558 -> event:Added route:fd00:1111:0::/48 flags:s pref:med rloc16:0x3c00
 * Done
 * @endcode
 * @cparam history route [@ca{list}] [@ca{num-entries}]
 * * Use the `list` option to display the output in list format. Otherwise,
 *   the output is shown in table format.
 * * Use the `num-entries` option to limit the output to the number of
 *   most-recent entries specified. If this option is not used, all stored
 *   entries are shown in the output.
 * @par
 * Displays the network data external-route history in table or list format.
 * @par
 * Each table or list entry provides:
 * * Age: Time elapsed since the command was issued, and given in the format:
 *        `hours`:`minutes`:`seconds`:`milliseconds`
 * * Event: Possible values are `Added` or `Removed`.
 * * Route
 * * Flags/meaning:
 *     * `s`: Stable flag.
 *     * `n`: NAT64 flag.
 * * Pref: Preference. Values can be either `high`, `med`, or `low`.
 * * RLOC16
 * @sa otHistoryTrackerIterateExternalRouteHistory
 */
template <> otError History::Process<Cmd("route")>(Arg aArgs[])
{
    otError                                  error;
    bool                                     isList;
    uint16_t                                 numEntries;
    otHistoryTrackerIterator                 iterator;
    const otHistoryTrackerExternalRouteInfo *info;
    uint32_t                                 entryAge;
    char                                     ageString[OT_HISTORY_TRACKER_ENTRY_AGE_STRING_SIZE];
    char                                     prefixString[OT_IP6_PREFIX_STRING_SIZE];
    NetworkData::FlagsString                 flagsString;

    static_assert(0 == OT_HISTORY_TRACKER_NET_DATA_ENTRY_ADDED, "NET_DATA_ENTRY_ADDED value is incorrect");
    static_assert(1 == OT_HISTORY_TRACKER_NET_DATA_ENTRY_REMOVED, "NET_DATA_ENTRY_REMOVED value is incorrect");

    SuccessOrExit(error = ParseArgs(aArgs, isList, numEntries));

    if (!isList)
    {
        // | Age                  | Event   | Route                                       | Flags     | Pref | RLOC16 |
        // +----------------------+---------+---------------------------------------------+-----------+------+--------+

        static const char *const kRouteTitles[]       = {"Age", "Event", "Route", "Flags", "Pref", "RLOC16"};
        static const uint8_t     kRouteColumnWidths[] = {22, 9, 45, 11, 6, 8};

        OutputTableHeader(kRouteTitles, kRouteColumnWidths);
    }

    otHistoryTrackerInitIterator(&iterator);

    for (uint16_t index = 0; (numEntries == 0) || (index < numEntries); index++)
    {
        info = otHistoryTrackerIterateExternalRouteHistory(GetInstancePtr(), &iterator, &entryAge);
        VerifyOrExit(info != nullptr);

        otHistoryTrackerEntryAgeToString(entryAge, ageString, sizeof(ageString));

        otIp6PrefixToString(&info->mRoute.mPrefix, prefixString, sizeof(prefixString));
        NetworkData::RouteFlagsToString(info->mRoute, flagsString);

        OutputLine(isList ? "%s -> event:%s route:%s flags:%s pref:%s rloc16:0x%04x"
                          : "| %20s | %-7s | %-43s | %-9s | %-4s | 0x%04x |",
                   ageString, Stringify(info->mEvent, kSimpleEventStrings), prefixString, flagsString,
                   PreferenceToString(info->mRoute.mPreference), info->mRoute.mRloc16);
    }

exit:
    return error;
}

otError History::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                               \
    {                                                          \
        aCommandString, &History::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("ipaddr"), CmdEntry("ipmaddr"), CmdEntry("neighbor"), CmdEntry("netinfo"), CmdEntry("prefix"),
        CmdEntry("route"),  CmdEntry("router"),  CmdEntry("rx"),       CmdEntry("rxtx"),    CmdEntry("tx"),
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

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
