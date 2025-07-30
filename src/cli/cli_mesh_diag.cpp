/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the CLI interpreter for Mesh Diagnostics function.
 */

#include "cli_mesh_diag.hpp"

#include <openthread/mesh_diag.h>

#include "cli/cli.hpp"
#include "cli/cli_utils.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD

namespace ot {
namespace Cli {

MeshDiag::MeshDiag(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
{
}

/** @cli responsetimeout
 * @code
 * responsetimeout
 * 5000
 * Done
 * @endcode
 * @par api_copy
 * #otMeshDiagGetResponseTimeout
 */
template <> otError MeshDiag::Process<Cmd("responsetimeout")>(Arg aArgs[])
{
    /** @cli responsetimeout (set)
     * @code
     * responsetimeout 7000
     * Done
     * @endcode
     * @cparam responsetimeout @ca{timeout-msec}
     * @par api_copy
     * #otMeshDiagSetResponseTimeout
     */
    return ProcessGetSet(aArgs, otMeshDiagGetResponseTimeout, otMeshDiagSetResponseTimeout);
}

template <> otError MeshDiag::Process<Cmd("topology")>(Arg aArgs[])
{
    /**
     * @cli meshdiag topology
     * @code
     * meshdiag topology
     * id:02 rloc16:0x0800 ext-addr:8aa57d2c603fe16c ver:4 - me - leader
     *    3-links:{ 46 }
     * id:46 rloc16:0xb800 ext-addr:fe109d277e0175cc ver:4
     *    3-links:{ 02 51 57 }
     * id:33 rloc16:0x8400 ext-addr:d2e511a146b9e54d ver:4
     *    3-links:{ 51 57 }
     * id:51 rloc16:0xcc00 ext-addr:9aab43ababf05352 ver:4
     *    3-links:{ 33 57 }
     *    2-links:{ 46 }
     * id:57 rloc16:0xe400 ext-addr:dae9c4c0e9da55ff ver:4
     *    3-links:{ 46 51 }
     *    1-links:{ 33 }
     * Done
     * @endcode
     * @par
     * Discover network topology (list of routers and their connections).
     * Parameters are optional and indicate additional items to discover. Can be added in any order.
     * * `ip6-addrs` to discover the list of IPv6 addresses of every router.
     * * `children` to discover the child table of every router.
     * @par
     * Information per router:
     * * Router ID
     * * RLOC16
     * * Extended MAC address
     * * Thread Version (if known)
     * * Whether the router is this device is itself (`me`)
     * * Whether the router is the parent of this device when device is a child (`parent`)
     * * Whether the router is `leader`
     * * Whether the router acts as a border router providing external connectivity (`br`)
     * * List of routers to which this router has a link:
     *   * `3-links`: Router IDs to which this router has a incoming link with link quality 3
     *   * `2-links`: Router IDs to which this router has a incoming link with link quality 2
     *   * `1-links`: Router IDs to which this router has a incoming link with link quality 1
     *   * If a list if empty, it is omitted in the out.
     * * If `ip6-addrs`, list of IPv6 addresses of the router
     * * If `children`, list of all children of the router. Information per child:
     *   * RLOC16
     *   * Incoming Link Quality from perspective of parent to child (zero indicates unknown)
     *   * Child Device mode (`r` rx-on-when-idle, `d` Full Thread Device, `n` Full Network Data, `-` no flags set)
     *   * Whether the child is this device itself (`me`)
     *   * Whether the child acts as a border router providing external connectivity (`br`)
     * @cparam meshdiag topology [@ca{ip6-addrs}] [@ca{children}]
     * @sa otMeshDiagDiscoverTopology
     */
    otError                  error = OT_ERROR_NONE;
    otMeshDiagDiscoverConfig config;

    config.mDiscoverIp6Addresses = false;
    config.mDiscoverChildTable   = false;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        if (*aArgs == "ip6-addrs")
        {
            config.mDiscoverIp6Addresses = true;
        }
        else if (*aArgs == "children")
        {
            config.mDiscoverChildTable = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otMeshDiagDiscoverTopology(GetInstancePtr(), &config, HandleMeshDiagDiscoverDone, this));
    error = OT_ERROR_PENDING;

exit:
    return error;
}

template <> otError MeshDiag::Process<Cmd("childtable")>(Arg aArgs[])
{
    /**
     * @cli meshdiag childtable
     * @code
     * meshdiag childtable 0x6400
     * rloc16:0x6402 ext-addr:8e6f4d323bbed1fe ver:4
     *     timeout:120 age:36 supvn:129 q-msg:0
     *     rx-on:yes type:ftd full-net:yes
     *     rss - ave:-20 last:-20 margin:80
     *     err-rate - frame:11.51% msg:0.76%
     *     conn-time:00:11:07
     *     csl - sync:no period:0 timeout:0 channel:0
     * rloc16:0x6403 ext-addr:ee24e64ecf8c079a ver:4
     *     timeout:120 age:19 supvn:129 q-msg:0
     *     rx-on:no type:mtd full-net:no
     *     rss - ave:-20 last:-20  margin:80
     *     err-rate - frame:0.73% msg:0.00%
     *     conn-time:01:08:53
     *     csl - sync:no period:0 timeout:0 channel:0
     * Done
     * @endcode
     * @par
     * Start a query for child table of a router with a given RLOC16.
     * Output lists all child entries. Information per child:
     * - RLOC16
     * - Extended MAC address
     * - Thread Version
     * - Timeout (in seconds)
     * - Age (seconds since last heard)
     * - Supervision interval (in seconds)
     * - Number of queued messages (in case child is sleepy)
     * - Device Mode
     * - RSS (average and last)
     * - Error rates: frame tx (at MAC layer), IPv6 message tx (above MAC)
     * - Connection time (seconds since link establishment `{dd}d.{hh}:{mm}:{ss}` format)
     * - CSL info:
     *   - If synchronized
     *   - Period (in unit of 10-symbols-time)
     *   - Timeout (in seconds)
     *
     * @cparam meshdiag childtable @ca{router-rloc16}
     * @sa otMeshDiagQueryChildTable
     */
    otError  error = OT_ERROR_NONE;
    uint16_t routerRloc16;

    SuccessOrExit(error = aArgs[0].ParseAsUint16(routerRloc16));
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(
        error = otMeshDiagQueryChildTable(GetInstancePtr(), routerRloc16, HandleMeshDiagQueryChildTableResult, this));

    error = OT_ERROR_PENDING;

exit:
    return error;
}

template <> otError MeshDiag::Process<Cmd("childip6")>(Arg aArgs[])
{
    /**
     * @cli meshdiag childip6
     * @code
     * meshdiag childip6 0xdc00
     * child-rloc16: 0xdc02
     *     fdde:ad00:beef:0:ded8:cd58:b73:2c21
     *     fd00:2:0:0:c24a:456:3b6b:c597
     *     fd00:1:0:0:120b:95fe:3ecc:d238
     * child-rloc16: 0xdc03
     *     fdde:ad00:beef:0:3aa6:b8bf:e7d6:eefe
     *     fd00:2:0:0:8ff8:a188:7436:6720
     *     fd00:1:0:0:1fcf:5495:790a:370f
     * Done
     * @endcode
     * @par
     * Send a query to a parent to retrieve the IPv6 addresses of all its MTD children.
     * @cparam meshdiag childip6 @ca{parent-rloc16}
     * @sa otMeshDiagQueryChildrenIp6Addrs
     */
    otError  error = OT_ERROR_NONE;
    uint16_t parentRloc16;

    SuccessOrExit(error = aArgs[0].ParseAsUint16(parentRloc16));
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otMeshDiagQueryChildrenIp6Addrs(GetInstancePtr(), parentRloc16,
                                                          HandleMeshDiagQueryChildIp6Addrs, this));

    error = OT_ERROR_PENDING;

exit:
    return error;
}

template <> otError MeshDiag::Process<Cmd("routerneighbortable")>(Arg aArgs[])
{
    /**
     * @cli meshdiag routerneighbortable
     * @code
     * meshdiag routerneighbortable 0x7400
     * rloc16:0x9c00 ext-addr:764788cf6e57a4d2 ver:4
     *    rss - ave:-20 last:-20 margin:80
     *    err-rate - frame:1.38% msg:0.00%
     *    conn-time:01:54:02
     * rloc16:0x7c00 ext-addr:4ed24fceec9bf6d3 ver:4
     *    rss - ave:-20 last:-20 margin:80
     *    err-rate - frame:0.72% msg:0.00%
     *    conn-time:00:11:27
     * Done
     * @endcode
     * @par
     * Start a query for router neighbor table of a router with a given RLOC16.
     * Output lists all router neighbor entries. Information per entry:
     *  - RLOC16
     *  - Extended MAC address
     *  - Thread Version
     *  - RSS (average and last) and link margin
     *  - Error rates, frame tx (at MAC layer), IPv6 message tx (above MAC)
     *  - Connection time (seconds since link establishment `{dd}d.{hh}:{mm}:{ss}` format)
     * @cparam meshdiag routerneighbortable @ca{router-rloc16}
     * @sa otMeshDiagQueryRouterNeighborTable
     */
    otError  error = OT_ERROR_NONE;
    uint16_t routerRloc16;

    SuccessOrExit(error = aArgs[0].ParseAsUint16(routerRloc16));
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = otMeshDiagQueryRouterNeighborTable(GetInstancePtr(), routerRloc16,
                                                             HandleMeshDiagQueryRouterNeighborTableResult, this));

    error = OT_ERROR_PENDING;

exit:
    return error;
}

otError MeshDiag::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                \
    {                                                           \
        aCommandString, &MeshDiag::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("childip6"), CmdEntry("childtable"), CmdEntry("responsetimeout"), CmdEntry("routerneighbortable"),
        CmdEntry("topology"),
    };

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

void MeshDiag::HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext)
{
    reinterpret_cast<MeshDiag *>(aContext)->HandleMeshDiagDiscoverDone(aError, aRouterInfo);
}

void MeshDiag::HandleMeshDiagDiscoverDone(otError aError, otMeshDiagRouterInfo *aRouterInfo)
{
    VerifyOrExit(aRouterInfo != nullptr);

    OutputFormat("id:%02u rloc16:0x%04x ext-addr:", aRouterInfo->mRouterId, aRouterInfo->mRloc16);
    OutputExtAddress(aRouterInfo->mExtAddress);

    if (aRouterInfo->mVersion != OT_MESH_DIAG_VERSION_UNKNOWN)
    {
        OutputFormat(" ver:%u", aRouterInfo->mVersion);
    }

    if (aRouterInfo->mIsThisDevice)
    {
        OutputFormat(" - me");
    }

    if (aRouterInfo->mIsThisDeviceParent)
    {
        OutputFormat(" - parent");
    }

    if (aRouterInfo->mIsLeader)
    {
        OutputFormat(" - leader");
    }

    if (aRouterInfo->mIsBorderRouter)
    {
        OutputFormat(" - br");
    }

    OutputNewLine();

    for (uint8_t linkQuality = 3; linkQuality > 0; linkQuality--)
    {
        bool hasLinkQuality = false;

        for (uint8_t entryQuality : aRouterInfo->mLinkQualities)
        {
            if (entryQuality == linkQuality)
            {
                hasLinkQuality = true;
                break;
            }
        }

        if (hasLinkQuality)
        {
            OutputFormat(kIndentSize, "%u-links:{ ", linkQuality);

            for (uint8_t id = 0; id < static_cast<uint8_t>(OT_ARRAY_LENGTH(aRouterInfo->mLinkQualities)); id++)
            {
                if (aRouterInfo->mLinkQualities[id] == linkQuality)
                {
                    OutputFormat("%02u ", id);
                }
            }

            OutputLine("}");
        }
    }

    if (aRouterInfo->mIp6AddrIterator != nullptr)
    {
        otIp6Address ip6Address;

        OutputLine(kIndentSize, "ip6-addrs:");

        while (otMeshDiagGetNextIp6Address(aRouterInfo->mIp6AddrIterator, &ip6Address) == OT_ERROR_NONE)
        {
            OutputSpaces(kIndentSize * 2);
            OutputIp6AddressLine(ip6Address);
        }
    }

    if (aRouterInfo->mChildIterator != nullptr)
    {
        otMeshDiagChildInfo childInfo;
        char                linkModeString[kLinkModeStringSize];
        bool                isFirst = true;

        while (otMeshDiagGetNextChildInfo(aRouterInfo->mChildIterator, &childInfo) == OT_ERROR_NONE)
        {
            if (isFirst)
            {
                OutputLine(kIndentSize, "children:");
                isFirst = false;
            }

            OutputFormat(kIndentSize * 2, "rloc16:0x%04x lq:%u, mode:%s", childInfo.mRloc16, childInfo.mLinkQuality,
                         LinkModeToString(childInfo.mMode, linkModeString));

            if (childInfo.mIsThisDevice)
            {
                OutputFormat(" - me");
            }

            if (childInfo.mIsBorderRouter)
            {
                OutputFormat(" - br");
            }

            OutputNewLine();
        }

        if (isFirst)
        {
            OutputLine(kIndentSize, "children: none");
        }
    }

exit:
    OutputResult(aError);
}

void MeshDiag::HandleMeshDiagQueryChildTableResult(otError                     aError,
                                                   const otMeshDiagChildEntry *aChildEntry,
                                                   void                       *aContext)
{
    reinterpret_cast<MeshDiag *>(aContext)->HandleMeshDiagQueryChildTableResult(aError, aChildEntry);
}

void MeshDiag::HandleMeshDiagQueryChildTableResult(otError aError, const otMeshDiagChildEntry *aChildEntry)
{
    PercentageStringBuffer stringBuffer;
    char                   string[OT_DURATION_STRING_SIZE];

    VerifyOrExit(aChildEntry != nullptr);

    OutputFormat("rloc16:0x%04x ext-addr:", aChildEntry->mRloc16);
    OutputExtAddress(aChildEntry->mExtAddress);
    OutputLine(" ver:%u", aChildEntry->mVersion);

    OutputLine(kIndentSize, "timeout:%lu age:%lu supvn:%u q-msg:%u", ToUlong(aChildEntry->mTimeout),
               ToUlong(aChildEntry->mAge), aChildEntry->mSupervisionInterval, aChildEntry->mQueuedMessageCount);

    OutputLine(kIndentSize, "rx-on:%s type:%s full-net:%s", aChildEntry->mRxOnWhenIdle ? "yes" : "no",
               aChildEntry->mDeviceTypeFtd ? "ftd" : "mtd", aChildEntry->mFullNetData ? "yes" : "no");

    OutputLine(kIndentSize, "rss - ave:%d last:%d margin:%d", aChildEntry->mAverageRssi, aChildEntry->mLastRssi,
               aChildEntry->mLinkMargin);

    if (aChildEntry->mSupportsErrRate)
    {
        OutputFormat(kIndentSize, "err-rate - frame:%s%% ",
                     PercentageToString(aChildEntry->mFrameErrorRate, stringBuffer));
        OutputLine("msg:%s%% ", PercentageToString(aChildEntry->mMessageErrorRate, stringBuffer));
    }

    otConvertDurationInSecondsToString(aChildEntry->mConnectionTime, string, sizeof(string));
    OutputLine(kIndentSize, "conn-time:%s", string);

    OutputLine(kIndentSize, "csl - sync:%s period:%u timeout:%lu channel:%u",
               aChildEntry->mCslSynchronized ? "yes" : "no", aChildEntry->mCslPeriod, ToUlong(aChildEntry->mCslTimeout),
               aChildEntry->mCslChannel);

exit:
    OutputResult(aError);
}

void MeshDiag::HandleMeshDiagQueryRouterNeighborTableResult(otError                              aError,
                                                            const otMeshDiagRouterNeighborEntry *aNeighborEntry,
                                                            void                                *aContext)
{
    reinterpret_cast<MeshDiag *>(aContext)->HandleMeshDiagQueryRouterNeighborTableResult(aError, aNeighborEntry);
}

void MeshDiag::HandleMeshDiagQueryRouterNeighborTableResult(otError                              aError,
                                                            const otMeshDiagRouterNeighborEntry *aNeighborEntry)
{
    PercentageStringBuffer stringBuffer;
    char                   string[OT_DURATION_STRING_SIZE];

    VerifyOrExit(aNeighborEntry != nullptr);

    OutputFormat("rloc16:0x%04x ext-addr:", aNeighborEntry->mRloc16);
    OutputExtAddress(aNeighborEntry->mExtAddress);
    OutputLine(" ver:%u", aNeighborEntry->mVersion);

    OutputLine(kIndentSize, "rss - ave:%d last:%d margin:%d", aNeighborEntry->mAverageRssi, aNeighborEntry->mLastRssi,
               aNeighborEntry->mLinkMargin);

    if (aNeighborEntry->mSupportsErrRate)
    {
        OutputFormat(kIndentSize, "err-rate - frame:%s%% ",
                     PercentageToString(aNeighborEntry->mFrameErrorRate, stringBuffer));
        OutputLine("msg:%s%% ", PercentageToString(aNeighborEntry->mMessageErrorRate, stringBuffer));
    }

    otConvertDurationInSecondsToString(aNeighborEntry->mConnectionTime, string, sizeof(string));
    OutputLine(kIndentSize, "conn-time:%s", string);

exit:
    OutputResult(aError);
}

void MeshDiag::HandleMeshDiagQueryChildIp6Addrs(otError                    aError,
                                                uint16_t                   aChildRloc16,
                                                otMeshDiagIp6AddrIterator *aIp6AddrIterator,
                                                void                      *aContext)
{
    reinterpret_cast<MeshDiag *>(aContext)->HandleMeshDiagQueryChildIp6Addrs(aError, aChildRloc16, aIp6AddrIterator);
}

void MeshDiag::HandleMeshDiagQueryChildIp6Addrs(otError                    aError,
                                                uint16_t                   aChildRloc16,
                                                otMeshDiagIp6AddrIterator *aIp6AddrIterator)
{
    otIp6Address ip6Address;

    VerifyOrExit(aError == OT_ERROR_NONE || aError == OT_ERROR_PENDING);
    VerifyOrExit(aIp6AddrIterator != nullptr);

    OutputLine("child-rloc16: 0x%04x", aChildRloc16);

    while (otMeshDiagGetNextIp6Address(aIp6AddrIterator, &ip6Address) == OT_ERROR_NONE)
    {
        OutputSpaces(kIndentSize);
        OutputIp6AddressLine(ip6Address);
    }

exit:
    OutputResult(aError);
}

void MeshDiag::OutputResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MESH_DIAG_ENABLE && OPENTHREAD_FTD
