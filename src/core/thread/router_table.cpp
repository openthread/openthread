/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include "router_table.hpp"

#if OPENTHREAD_FTD

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/timer.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

RegisterLogModule("RouterTable");

RouterTable::RouterTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRouters(aInstance)
    , mRouterIdSequenceLastUpdated(0)
    , mRouterIdSequence(Random::NonCrypto::GetUint8())
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mMinRouterId(0)
    , mMaxRouterId(Mle::kMaxRouterId)
#endif
{
    Clear();
}

void RouterTable::Clear(void)
{
    ClearNeighbors();
    mRouterIdMap.Clear();
    mRouters.Clear();
}

void RouterTable::ClearNeighbors(void)
{
    for (Router &router : mRouters)
    {
        if (router.IsStateValid())
        {
            Get<NeighborTable>().Signal(NeighborTable::kRouterRemoved, router);
        }

        router.SetState(Neighbor::kStateInvalid);
    }
}

Router *RouterTable::AddRouter(uint8_t aRouterId)
{
    // Add a new `Router` entry to `mRouters` array with given
    // `aRouterId` and update the `mRouterIdMap`.

    Router *router = mRouters.PushBack();

    VerifyOrExit(router != nullptr);

    router->Clear();
    router->SetRloc16(Mle::Rloc16FromRouterId(aRouterId));
    router->SetNextHop(Mle::kInvalidRouterId);

    mRouterIdMap.SetIndex(aRouterId, mRouters.IndexOf(*router));

exit:
    return router;
}

void RouterTable::RemoveRouter(Router &aRouter)
{
    // Remove an existing `aRouter` entry from `mRouters` and update the
    // `mRouterIdMap`.

    if (aRouter.IsStateValid())
    {
        Get<NeighborTable>().Signal(NeighborTable::kRouterRemoved, aRouter);
    }

    mRouterIdMap.Release(aRouter.GetRouterId());
    mRouters.Remove(aRouter);

    // Removing `aRouter` from `mRouters` array will replace it with
    // the last entry in the array (if not already the last entry) so
    // we update the index in `mRouteIdMap` for the moved entry.

    if (IsAllocated(aRouter.GetRouterId()))
    {
        mRouterIdMap.SetIndex(aRouter.GetRouterId(), mRouters.IndexOf((aRouter)));
    }
}

Router *RouterTable::Allocate(void)
{
    Router *router           = nullptr;
    uint8_t numAvailable     = 0;
    uint8_t selectedRouterId = Mle::kInvalidRouterId;

    VerifyOrExit(!mRouters.IsFull());

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    for (uint8_t routerId = mMinRouterId; routerId <= mMaxRouterId; routerId++)
#else
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
#endif
    {
        if (mRouterIdMap.CanAllocate(routerId))
        {
            numAvailable++;

            // Randomly select a router ID as we iterate through the
            // list using Reservoir algorithm: We replace the
            // selected ID with current entry in the list with
            // probably `1/numAvailable`.

            if (Random::NonCrypto::GetUint8InRange(0, numAvailable) == 0)
            {
                selectedRouterId = routerId;
            }
        }
    }

    VerifyOrExit(selectedRouterId != Mle::kInvalidRouterId);

    router = Allocate(selectedRouterId);
    OT_ASSERT(router != nullptr);

exit:
    return router;
}

Router *RouterTable::Allocate(uint8_t aRouterId)
{
    Router *router = nullptr;

    VerifyOrExit(aRouterId <= Mle::kMaxRouterId && mRouterIdMap.CanAllocate(aRouterId));

    router = AddRouter(aRouterId);
    VerifyOrExit(router != nullptr);

    router->SetLastHeard(TimerMilli::GetNow());

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();
    Get<Mle::MleRouter>().ResetAdvertiseInterval();

    LogNote("Allocate router id %d", aRouterId);

exit:
    return router;
}

Error RouterTable::Release(uint8_t aRouterId)
{
    Error   error = kErrorNone;
    Router *router;

    OT_ASSERT(aRouterId <= Mle::kMaxRouterId);

    VerifyOrExit(Get<Mle::MleRouter>().IsLeader(), error = kErrorInvalidState);

    router = FindRouterById(aRouterId);
    VerifyOrExit(router != nullptr, error = kErrorNotFound);

    RemoveRouter(*router);

    for (Router &otherRouter : mRouters)
    {
        if (otherRouter.GetNextHop() == aRouterId)
        {
            otherRouter.SetNextHop(Mle::kInvalidRouterId);
            otherRouter.SetCost(0);
        }
    }

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    Get<AddressResolver>().Remove(aRouterId);
    Get<NetworkData::Leader>().RemoveBorderRouter(Mle::Rloc16FromRouterId(aRouterId),
                                                  NetworkData::Leader::kMatchModeRouterId);
    Get<Mle::MleRouter>().ResetAdvertiseInterval();

    LogNote("Release router id %d", aRouterId);

exit:
    return error;
}

void RouterTable::RemoveRouterLink(Router &aRouter)
{
    if (aRouter.GetLinkQualityOut() != kLinkQuality0)
    {
        aRouter.SetLinkQualityOut(kLinkQuality0);
        aRouter.SetLastHeard(TimerMilli::GetNow());
    }

    for (Router &router : mRouters)
    {
        if (router.GetNextHop() == aRouter.GetRouterId())
        {
            router.SetNextHop(Mle::kInvalidRouterId);
            router.SetCost(0);

            if (GetLinkCost(router) >= Mle::kMaxRouteCost)
            {
                Get<Mle::MleRouter>().ResetAdvertiseInterval();
            }
        }
    }

    if (aRouter.GetNextHop() == Mle::kInvalidRouterId)
    {
        Get<Mle::MleRouter>().ResetAdvertiseInterval();

        // Clear all EID-to-RLOC entries associated with the router.
        Get<AddressResolver>().Remove(aRouter.GetRouterId());
    }
}

const Router *RouterTable::FindRouter(const Router::AddressMatcher &aMatcher) const
{
    return mRouters.FindMatching(aMatcher);
}

Router *RouterTable::FindNeighbor(uint16_t aRloc16)
{
    Router *router = nullptr;

    VerifyOrExit(aRloc16 != Get<Mle::MleRouter>().GetRloc16());
    router = FindRouter(Router::AddressMatcher(aRloc16, Router::kInStateValid));

exit:
    return router;
}

Router *RouterTable::FindNeighbor(const Mac::ExtAddress &aExtAddress)
{
    return FindRouter(Router::AddressMatcher(aExtAddress, Router::kInStateValid));
}

Router *RouterTable::FindNeighbor(const Mac::Address &aMacAddress)
{
    return FindRouter(Router::AddressMatcher(aMacAddress, Router::kInStateValid));
}

const Router *RouterTable::FindRouterById(uint8_t aRouterId) const
{
    const Router *router = nullptr;

    VerifyOrExit(aRouterId <= Mle::kMaxRouterId);

    VerifyOrExit(IsAllocated(aRouterId));
    router = &mRouters[mRouterIdMap.GetIndex(aRouterId)];

exit:
    return router;
}

const Router *RouterTable::FindRouterByRloc16(uint16_t aRloc16) const
{
    return FindRouterById(Mle::RouterIdFromRloc16(aRloc16));
}

const Router *RouterTable::FindNextHopOf(const Router &aRouter) const { return FindRouterById(aRouter.GetNextHop()); }

Router *RouterTable::FindRouter(const Mac::ExtAddress &aExtAddress)
{
    return FindRouter(Router::AddressMatcher(aExtAddress, Router::kInStateAny));
}

Error RouterTable::GetRouterInfo(uint16_t aRouterId, Router::Info &aRouterInfo)
{
    Error   error = kErrorNone;
    Router *router;
    uint8_t routerId;

    if (aRouterId <= Mle::kMaxRouterId)
    {
        routerId = static_cast<uint8_t>(aRouterId);
    }
    else
    {
        VerifyOrExit(Mle::IsActiveRouter(aRouterId), error = kErrorInvalidArgs);
        routerId = Mle::RouterIdFromRloc16(aRouterId);
        VerifyOrExit(routerId <= Mle::kMaxRouterId, error = kErrorInvalidArgs);
    }

    router = FindRouterById(routerId);
    VerifyOrExit(router != nullptr, error = kErrorNotFound);

    aRouterInfo.SetFrom(*router);

exit:
    return error;
}

const Router *RouterTable::GetLeader(void) const { return FindRouterById(Get<Mle::MleRouter>().GetLeaderId()); }

uint32_t RouterTable::GetLeaderAge(void) const
{
    return (!mRouters.IsEmpty()) ? Time::MsecToSec(TimerMilli::GetNow() - mRouterIdSequenceLastUpdated) : 0xffffffff;
}

uint8_t RouterTable::GetNeighborCount(void) const
{
    uint8_t count = 0;

    for (const Router &router : mRouters)
    {
        if (router.IsStateValid())
        {
            count++;
        }
    }

    return count;
}

uint8_t RouterTable::GetLinkCost(const Router &aRouter) const
{
    uint8_t rval = Mle::kMaxRouteCost;

    VerifyOrExit(aRouter.GetRloc16() != Get<Mle::MleRouter>().GetRloc16() && aRouter.IsStateValid());

    rval = CostForLinkQuality(aRouter.GetTwoWayLinkQuality());

exit:
    return rval;
}

uint8_t RouterTable::GetLinkCost(uint8_t aRouterId) const
{
    uint8_t       rval = Mle::kMaxRouteCost;
    const Router *router;

    router = FindRouterById(aRouterId);

    // `nullptr` aRouterId indicates non-existing next hop, hence return kMaxRouteCost for it.
    VerifyOrExit(router != nullptr);

    rval = GetLinkCost(*router);

exit:
    return rval;
}

uint8_t RouterTable::GetPathCost(uint16_t aDestRloc16) const
{
    uint8_t       cost = Mle::kMaxRouteCost;
    uint8_t       destRouterId;
    const Router *router;
    const Router *nextHop;

    VerifyOrExit(Get<Mle::Mle>().IsAttached());

    if (aDestRloc16 == Get<Mle::Mle>().GetRloc16())
    {
        // Destination is this device, return cost as zero.
        // This is also valid when device is a child.
        ExitNow(cost = 0);
    }

    destRouterId = Mle::RouterIdFromRloc16(aDestRloc16);

    router  = FindRouterById(destRouterId);
    nextHop = (router != nullptr) ? FindNextHopOf(*router) : nullptr;

    if (Get<Mle::MleRouter>().IsChild())
    {
        const Router &parent = Get<Mle::Mle>().GetParent();

        // If destination is our parent or another child of our
        // parent, we use the link cost to our parent. Otherwise we
        // check if we have a next hop towards the destination and
        // add its cost to the link cost to parent.

        VerifyOrExit((destRouterId == parent.GetRouterId()) || (nextHop != nullptr));

        cost = CostForLinkQuality(parent.GetLinkQualityIn());

        if (destRouterId != parent.GetRouterId())
        {
            cost += router->GetCost();
        }

        // The case where destination itself is a child is handled at
        // the end (after `else` block).
    }
    else // Role is router or leader
    {
        if (destRouterId == Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()))
        {
            // Destination is a one of our children.

            const Child *child = Get<ChildTable>().FindChild(aDestRloc16, Child::kInStateValid);

            VerifyOrExit(child != nullptr);
            ExitNow(cost = CostForLinkQuality(child->GetLinkQualityIn()));
        }

        VerifyOrExit(router != nullptr);

        cost = GetLinkCost(*router);

        if (nextHop != nullptr)
        {
            // Determine whether direct link or forwarding hop link
            // has a lower cost.
            cost = Min(cost, static_cast<uint8_t>(router->GetCost() + GetLinkCost(*nextHop)));
        }
    }

    if (!Mle::IsActiveRouter(aDestRloc16))
    {
        // Destination is a child. we assume best link quality
        // between destination and its parent router.

        cost += kCostForLinkQuality3;
    }

exit:
    return cost;
}

uint8_t RouterTable::GetPathCostToLeader(void) const
{
    return GetPathCost(Mle::Rloc16FromRouterId(Get<Mle::Mle>().GetLeaderId()));
}

uint16_t RouterTable::GetNextHop(uint16_t aDestRloc16) const
{
    uint16_t      nextHopRloc16 = Mle::kInvalidRloc16;
    uint8_t       destRouterId  = Mle::RouterIdFromRloc16(aDestRloc16);
    uint8_t       linkCost;
    const Router *router;
    const Router *nextHop;

    if (Get<Mle::Mle>().IsChild())
    {
        const Router &parent = Get<Mle::Mle>().GetParent();

        VerifyOrExit(parent.IsStateValid());
        nextHopRloc16 = parent.GetRloc16();
        ExitNow();
    }

    if (destRouterId == Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16()))
    {
        // Destination is device itself or one of its
        // children.
        ExitNow(nextHopRloc16 = aDestRloc16);
    }

    router = FindRouterById(destRouterId);
    VerifyOrExit(router != nullptr);

    linkCost = GetLinkCost(*router);

    if (linkCost < Mle::kMaxRouteCost)
    {
        nextHopRloc16 = Mle::Rloc16FromRouterId(destRouterId);
    }

    // Check if we have a forwarding route path towards the
    // destination and whether direct link or forwarding path has a
    // lower cost.

    nextHop = FindNextHopOf(*router);
    VerifyOrExit(nextHop != nullptr);

    if (router->GetCost() + GetLinkCost(*nextHop) < linkCost)
    {
        nextHopRloc16 = nextHop->GetRloc16();
    }

exit:
    return nextHopRloc16;
}

void RouterTable::UpdateRouterIdSet(uint8_t aRouterIdSequence, const Mle::RouterIdSet &aRouterIdSet)
{
    bool shouldAdd = false;

    mRouterIdSequence            = aRouterIdSequence;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    // Remove all previously allocated routers that are now removed in
    // new `aRouterIdSet`.

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (IsAllocated(routerId) == aRouterIdSet.Contains(routerId))
        {
            continue;
        }

        if (IsAllocated(routerId))
        {
            Router *router = FindRouterById(routerId);

            OT_ASSERT(router != nullptr);
            router->SetNextHop(Mle::kInvalidRouterId);
            RemoveRouterLink(*router);
            RemoveRouter(*router);
        }
        else
        {
            shouldAdd = true;
        }
    }

    VerifyOrExit(shouldAdd);

    // Now add all new routers in `aRouterIdSet`.

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (!IsAllocated(routerId) && aRouterIdSet.Contains(routerId))
        {
            AddRouter(routerId);
        }
    }

    Get<Mle::MleRouter>().ResetAdvertiseInterval();

exit:
    return;
}

void RouterTable::UpdateRoutes(const Mle::RouteTlv &aRouteTlv, uint8_t aRouterId)
{
    Router *neighbor;
    bool    resetAdvInterval = false;
    bool    changed          = false;

    neighbor = FindRouterById(aRouterId);
    VerifyOrExit(neighbor != nullptr);

    // update link quality out to neighbor
    changed = UpdateLinkQualityOut(aRouteTlv, *neighbor, resetAdvInterval);

    // update routes
    for (uint8_t routerId = 0, routeCount = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        Router *router;
        Router *nextHop;
        uint8_t cost;

        if (!aRouteTlv.IsRouterIdSet(routerId))
        {
            continue;
        }

        router = FindRouterById(routerId);

        if (router == nullptr || router->GetRloc16() == Get<Mle::Mle>().GetRloc16() || router == neighbor)
        {
            routeCount++;
            continue;
        }

        nextHop = FindNextHopOf(*router);

        cost = aRouteTlv.GetRouteCost(routeCount);

        if (cost == 0)
        {
            cost = Mle::kMaxRouteCost;
        }

        if (nextHop == nullptr || nextHop == neighbor)
        {
            // router has no next hop or next hop is neighbor (sender)

            if (cost + GetLinkCost(*neighbor) < Mle::kMaxRouteCost)
            {
                if (nextHop == nullptr && GetLinkCost(*router) >= Mle::kMaxRouteCost)
                {
                    resetAdvInterval = true;
                }

                if (router->GetNextHop() != aRouterId)
                {
                    router->SetNextHop(aRouterId);
                    changed = true;
                }

                if (router->GetCost() != cost)
                {
                    router->SetCost(cost);
                    changed = true;
                }
            }
            else if (nextHop == neighbor)
            {
                if (GetLinkCost(*router) >= Mle::kMaxRouteCost)
                {
                    resetAdvInterval = true;
                }

                router->SetNextHop(Mle::kInvalidRouterId);
                router->SetCost(0);
                router->SetLastHeard(TimerMilli::GetNow());
                changed = true;
            }
        }
        else
        {
            uint8_t curCost = router->GetCost() + GetLinkCost(*nextHop);
            uint8_t newCost = cost + GetLinkCost(*neighbor);

            if (newCost < curCost)
            {
                router->SetNextHop(aRouterId);
                router->SetCost(cost);
                changed = true;
            }
        }

        routeCount++;
    }

    if (resetAdvInterval)
    {
        Get<Mle::MleRouter>().ResetAdvertiseInterval();
    }

    if (changed)
    {
        LogRouteTable();
    }

exit:
    return;
}

bool RouterTable::UpdateLinkQualityOut(const Mle::RouteTlv &aRouteTlv, Router &aNeighbor, bool &aResetAdvInterval)
{
    bool        changed = false;
    LinkQuality linkQuality;
    uint8_t     myRouterId;
    uint8_t     myRouteCount;
    uint8_t     oldLinkCost;
    Router     *nextHop;

    myRouterId = Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16());
    VerifyOrExit(aRouteTlv.IsRouterIdSet(myRouterId));

    myRouteCount = 0;
    for (uint8_t routerId = 0; routerId < myRouterId; routerId++)
    {
        myRouteCount += aRouteTlv.IsRouterIdSet(routerId);
    }

    linkQuality = aRouteTlv.GetLinkQualityIn(myRouteCount);
    VerifyOrExit(aNeighbor.GetLinkQualityOut() != linkQuality);

    oldLinkCost = GetLinkCost(aNeighbor);

    aNeighbor.SetLinkQualityOut(linkQuality);
    nextHop = FindNextHopOf(aNeighbor);

    // reset MLE advertisement timer if neighbor route cost changed to or from infinite
    if (nextHop == nullptr && (oldLinkCost >= Mle::kMaxRouteCost) != (GetLinkCost(aNeighbor) >= Mle::kMaxRouteCost))
    {
        aResetAdvInterval = true;
    }

    changed = true;

exit:
    return changed;
}

void RouterTable::FillRouteTlv(Mle::RouteTlv &aRouteTlv, const Neighbor *aNeighbor) const
{
    uint8_t          routerIdSequence = mRouterIdSequence;
    Mle::RouterIdSet routerIdSet;
    uint8_t          routerIndex;

    mRouterIdMap.GetAsRouterIdSet(routerIdSet);

    if ((aNeighbor != nullptr) && Mle::IsActiveRouter(aNeighbor->GetRloc16()))
    {
        // Sending a Link Accept message that may require truncation
        // of Route64 TLV.

        uint8_t routerCount = mRouters.GetLength();

        if (routerCount > Mle::kLinkAcceptMaxRouters)
        {
            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                if (routerCount <= Mle::kLinkAcceptMaxRouters)
                {
                    break;
                }

                if ((routerId == Mle::RouterIdFromRloc16(Get<Mle::Mle>().GetRloc16())) ||
                    (routerId == aNeighbor->GetRouterId()) || (routerId == Get<Mle::Mle>().GetLeaderId()))
                {
                    // Route64 TLV must contain this device and the
                    // neighboring router to ensure that at least this
                    // link can be established.
                    continue;
                }

                if (routerIdSet.Contains(routerId))
                {
                    routerIdSet.Remove(routerId);
                    routerCount--;
                }
            }

            // Ensure that the neighbor will process the current
            // Route64 TLV in a subsequent message exchange
            routerIdSequence -= Mle::kLinkAcceptSequenceRollback;
        }
    }

    aRouteTlv.SetRouterIdSequence(routerIdSequence);
    aRouteTlv.SetRouterIdMask(routerIdSet);

    routerIndex = 0;

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        const Router *router;

        if (!routerIdSet.Contains(routerId))
        {
            continue;
        }

        router = FindRouterById(routerId);
        OT_ASSERT(router != nullptr);

        if (router->GetRloc16() == Get<Mle::Mle>().GetRloc16())
        {
            aRouteTlv.SetRouteData(routerIndex, kLinkQuality0, kLinkQuality0, 1);
        }
        else
        {
            const Router *nextHop  = FindNextHopOf(*router);
            uint8_t       linkCost = GetLinkCost(*router);
            uint8_t       routeCost;

            if (nextHop == nullptr)
            {
                routeCost = linkCost;
            }
            else
            {
                routeCost = router->GetCost() + GetLinkCost(*nextHop);

                if (linkCost < routeCost)
                {
                    routeCost = linkCost;
                }
            }

            if (routeCost >= Mle::kMaxRouteCost)
            {
                routeCost = 0;
            }

            aRouteTlv.SetRouteData(routerIndex, router->GetLinkQualityIn(), router->GetLinkQualityOut(), routeCost);
        }

        routerIndex++;
    }

    aRouteTlv.SetRouteDataLength(routerIndex);
}

void RouterTable::HandleTimeTick(void)
{
    mRouterIdMap.HandleTimeTick();

    VerifyOrExit(Get<Mle::MleRouter>().IsLeader());

    // Update router id sequence
    if (GetLeaderAge() >= Mle::kRouterIdSequencePeriod)
    {
        mRouterIdSequence++;
        mRouterIdSequenceLastUpdated = TimerMilli::GetNow();
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void RouterTable::GetRouterIdRange(uint8_t &aMinRouterId, uint8_t &aMaxRouterId) const
{
    aMinRouterId = mMinRouterId;
    aMaxRouterId = mMaxRouterId;
}

Error RouterTable::SetRouterIdRange(uint8_t aMinRouterId, uint8_t aMaxRouterId)
{
    Error error = kErrorNone;

    VerifyOrExit(aMinRouterId <= aMaxRouterId, error = kErrorInvalidArgs);
    VerifyOrExit(aMaxRouterId <= Mle::kMaxRouterId, error = kErrorInvalidArgs);
    mMinRouterId = aMinRouterId;
    mMaxRouterId = aMaxRouterId;

exit:
    return error;
}
#endif

void RouterTable::RouterIdMap::GetAsRouterIdSet(Mle::RouterIdSet &aRouterIdSet) const
{
    aRouterIdSet.Clear();

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (IsAllocated(routerId))
        {
            aRouterIdSet.Add(routerId);
        }
    }
}

void RouterTable::RouterIdMap::HandleTimeTick(void)
{
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        // If Router ID is not allocated the `mIndexes` tracks the
        // remaining reuse delay time in seconds.

        if (!IsAllocated(routerId) && (mIndexes[routerId] > 0))
        {
            mIndexes[routerId]--;
        }
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void RouterTable::LogRouteTable(void) const
{
    static constexpr uint16_t kStringSize = 128;

    LogInfo("Route table");

    for (const Router &router : mRouters)
    {
        String<kStringSize> string;

        string.Append("    %2d 0x%04x", router.GetRouterId(), router.GetRloc16());

        if (router.GetRloc16() == Get<Mle::Mle>().GetRloc16())
        {
            string.Append(" - me");
        }
        else
        {
            if (router.IsStateValid())
            {
                string.Append(" - nbr{lq[i/o]:%d/%d cost:%d}", router.GetLinkQualityIn(), router.GetLinkQualityOut(),
                              GetLinkCost(router));
            }

            if (router.GetNextHop() != Mle::kInvalidRouterId)
            {
                string.Append(" - nexthop{%d cost:%d}", router.GetNextHop(), router.GetCost());
            }
        }

        if (router.GetRouterId() == Get<Mle::Mle>().GetLeaderId())
        {
            string.Append(" - leader");
        }

        LogInfo("%s", string.AsCString());
    }
}
#endif

} // namespace ot

#endif // OPENTHREAD_FTD
