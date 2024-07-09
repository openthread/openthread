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

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("RouterTable");

RouterTable::RouterTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRouters(aInstance)
    , mChangedTask(aInstance)
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
    SignalTableChanged();
}

bool RouterTable::IsRouteTlvIdSequenceMoreRecent(const Mle::RouteTlv &aRouteTlv) const
{
    return (GetActiveRouterCount() == 0) ||
           SerialNumber::IsGreater(aRouteTlv.GetRouterIdSequence(), GetRouterIdSequence());
}

void RouterTable::ClearNeighbors(void)
{
    for (Router &router : mRouters)
    {
        if (router.IsStateValid())
        {
            Get<NeighborTable>().Signal(NeighborTable::kRouterRemoved, router);
            SignalTableChanged();
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
    router->SetNextHopToInvalid();

    mRouterIdMap.SetIndex(aRouterId, mRouters.IndexOf(*router));
    SignalTableChanged();

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

    SignalTableChanged();
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
            otherRouter.SetNextHopToInvalid();
        }
    }

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    Get<AddressResolver>().RemoveEntriesForRouterId(aRouterId);
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
        SignalTableChanged();
    }

    for (Router &router : mRouters)
    {
        if (router.GetNextHop() == aRouter.GetRouterId())
        {
            router.SetNextHopToInvalid();
            SignalTableChanged();

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
        Get<AddressResolver>().RemoveEntriesForRouterId(aRouter.GetRouterId());
    }
}

const Router *RouterTable::FindRouter(const Router::AddressMatcher &aMatcher) const
{
    return mRouters.FindMatching(aMatcher);
}

Router *RouterTable::FindNeighbor(uint16_t aRloc16)
{
    Router *router = nullptr;

    VerifyOrExit(!Get<Mle::Mle>().HasRloc16(aRloc16));
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
        VerifyOrExit(Mle::IsRouterRloc16(aRouterId), error = kErrorInvalidArgs);
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

uint8_t RouterTable::GetNeighborCount(LinkQuality aLinkQuality) const
{
    uint8_t count = 0;

    for (const Router &router : mRouters)
    {
        if (router.IsStateValid() && (router.GetLinkQualityIn() >= aLinkQuality))
        {
            count++;
        }
    }

    return count;
}

uint8_t RouterTable::GetLinkCost(const Router &aRouter) const
{
    uint8_t rval = Mle::kMaxRouteCost;

    VerifyOrExit(!Get<Mle::Mle>().HasRloc16(aRouter.GetRloc16()) && aRouter.IsStateValid());

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
    uint8_t  pathCost;
    uint16_t nextHopRloc16;

    GetNextHopAndPathCost(aDestRloc16, nextHopRloc16, pathCost);

    return pathCost;
}

uint8_t RouterTable::GetPathCostToLeader(void) const { return GetPathCost(Get<Mle::Mle>().GetLeaderRloc16()); }

void RouterTable::GetNextHopAndPathCost(uint16_t aDestRloc16, uint16_t &aNextHopRloc16, uint8_t &aPathCost) const
{
    const Router *router;
    const Router *nextHop;

    aPathCost      = Mle::kMaxRouteCost;
    aNextHopRloc16 = Mle::kInvalidRloc16;

    VerifyOrExit(Get<Mle::Mle>().IsAttached());

    if (Get<Mle::Mle>().HasRloc16(aDestRloc16))
    {
        // Destination is this device, return cost as zero.
        aPathCost      = 0;
        aNextHopRloc16 = aDestRloc16;
        ExitNow();
    }

    router  = FindRouterById(Mle::RouterIdFromRloc16(aDestRloc16));
    nextHop = (router != nullptr) ? FindNextHopOf(*router) : nullptr;

    if (Get<Mle::MleRouter>().IsChild())
    {
        const Router &parent = Get<Mle::Mle>().GetParent();
        bool          destIsParentOrItsChild;

        if (parent.IsStateValid())
        {
            aNextHopRloc16 = parent.GetRloc16();
        }

        // If destination is our parent or another child of our
        // parent, we use the link cost to our parent. Otherwise we
        // check if we have a next hop towards the destination and
        // add its cost to the link cost to parent.

        destIsParentOrItsChild = Mle::RouterIdMatch(aDestRloc16, parent.GetRloc16());

        VerifyOrExit(destIsParentOrItsChild || (nextHop != nullptr));

        aPathCost = CostForLinkQuality(parent.GetLinkQualityIn());

        if (!destIsParentOrItsChild)
        {
            aPathCost += router->GetCost();
        }

        // The case where destination itself is a child is handled at
        // the end (after `else` block).
    }
    else // Role is router or leader
    {
        if (Get<Mle::Mle>().HasMatchingRouterIdWith(aDestRloc16))
        {
            // Destination is a one of our children.

            const Child *child = Get<ChildTable>().FindChild(aDestRloc16, Child::kInStateAnyExceptInvalid);

            VerifyOrExit(child != nullptr);
            aNextHopRloc16 = aDestRloc16;
            aPathCost      = CostForLinkQuality(child->GetLinkQualityIn());
            ExitNow();
        }

        VerifyOrExit(router != nullptr);

        aPathCost = GetLinkCost(*router);

        if (aPathCost < Mle::kMaxRouteCost)
        {
            aNextHopRloc16 = router->GetRloc16();
        }

        if (nextHop != nullptr)
        {
            // Determine whether direct link or forwarding hop link
            // through `nextHop` has a lower path cost.

            uint8_t nextHopPathCost = router->GetCost() + GetLinkCost(*nextHop);

            if (nextHopPathCost < aPathCost)
            {
                aPathCost      = nextHopPathCost;
                aNextHopRloc16 = nextHop->GetRloc16();
            }
        }
    }

    if (Mle::IsChildRloc16(aDestRloc16))
    {
        // Destination is a child. we assume best link quality
        // between destination and its parent router.

        aPathCost += kCostForLinkQuality3;
    }

exit:
    return;
}

uint16_t RouterTable::GetNextHop(uint16_t aDestRloc16) const
{
    uint8_t  pathCost;
    uint16_t nextHopRloc16;

    GetNextHopAndPathCost(aDestRloc16, nextHopRloc16, pathCost);

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
            router->SetNextHopToInvalid();
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

void RouterTable::UpdateRoutes(const Mle::RouteTlv &aRouteTlv, uint8_t aNeighborId)
{
    Router          *neighbor;
    Mle::RouterIdSet finitePathCostIdSet;
    uint8_t          linkCostToNeighbor;

    neighbor = FindRouterById(aNeighborId);
    VerifyOrExit(neighbor != nullptr);

    // Before updating the routes, we track which routers have finite
    // path cost. After the update we check again to see if any path
    // cost changed from finite to infinite or vice versa to decide
    // whether to reset the  MLE Advertisement interval.

    finitePathCostIdSet.Clear();

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (GetPathCost(Mle::Rloc16FromRouterId(routerId)) < Mle::kMaxRouteCost)
        {
            finitePathCostIdSet.Add(routerId);
        }
    }

    // Find the entry corresponding to our Router ID in the received
    // `aRouteTlv` to get the `LinkQualityIn` from the perspective of
    // neighbor. We use this to update our `LinkQualityOut` to the
    // neighbor.

    for (uint8_t routerId = 0, index = 0; routerId <= Mle::kMaxRouterId;
         index += aRouteTlv.IsRouterIdSet(routerId) ? 1 : 0, routerId++)
    {
        if (!Get<Mle::Mle>().MatchesRouterId(routerId))
        {
            continue;
        }

        if (aRouteTlv.IsRouterIdSet(routerId))
        {
            LinkQuality linkQuality = aRouteTlv.GetLinkQualityIn(index);

            if (neighbor->GetLinkQualityOut() != linkQuality)
            {
                neighbor->SetLinkQualityOut(linkQuality);
                SignalTableChanged();
            }
        }

        break;
    }

    linkCostToNeighbor = GetLinkCost(*neighbor);

    for (uint8_t routerId = 0, index = 0; routerId <= Mle::kMaxRouterId;
         index += aRouteTlv.IsRouterIdSet(routerId) ? 1 : 0, routerId++)
    {
        Router *router;
        Router *nextHop;
        uint8_t cost;

        if (!aRouteTlv.IsRouterIdSet(routerId))
        {
            continue;
        }

        router = FindRouterById(routerId);

        if (router == nullptr || Get<Mle::Mle>().HasRloc16(router->GetRloc16()) || router == neighbor)
        {
            continue;
        }

        nextHop = FindNextHopOf(*router);

        cost = aRouteTlv.GetRouteCost(index);
        cost = (cost == 0) ? Mle::kMaxRouteCost : cost;

        if ((nextHop == nullptr) || (nextHop == neighbor))
        {
            // `router` has no next hop or next hop is neighbor (sender)

            if (cost + linkCostToNeighbor < Mle::kMaxRouteCost)
            {
                if (router->SetNextHopAndCost(aNeighborId, cost))
                {
                    SignalTableChanged();
                }
            }
            else if (nextHop == neighbor)
            {
                router->SetNextHopToInvalid();
                router->SetLastHeard(TimerMilli::GetNow());
                SignalTableChanged();
            }
        }
        else
        {
            uint8_t curCost = router->GetCost() + GetLinkCost(*nextHop);
            uint8_t newCost = cost + linkCostToNeighbor;

            if (newCost < curCost)
            {
                router->SetNextHopAndCost(aNeighborId, cost);
                SignalTableChanged();
            }
        }
    }

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        bool oldCostFinite = finitePathCostIdSet.Contains(routerId);
        bool newCostFinite = (GetPathCost(Mle::Rloc16FromRouterId(routerId)) < Mle::kMaxRouteCost);

        if (newCostFinite != oldCostFinite)
        {
            Get<Mle::MleRouter>().ResetAdvertiseInterval();
            break;
        }
    }

exit:
    return;
}

void RouterTable::UpdateRouterOnFtdChild(const Mle::RouteTlv &aRouteTlv, uint8_t aParentId)
{
    for (uint8_t routerId = 0, index = 0; routerId <= Mle::kMaxRouterId;
         index += aRouteTlv.IsRouterIdSet(routerId) ? 1 : 0, routerId++)
    {
        Router *router;
        uint8_t cost;
        uint8_t nextHopId;

        if (!aRouteTlv.IsRouterIdSet(routerId) || (routerId == aParentId))
        {
            continue;
        }

        router = FindRouterById(routerId);

        if (router == nullptr)
        {
            continue;
        }

        cost      = aRouteTlv.GetRouteCost(index);
        nextHopId = (cost == 0) ? Mle::kInvalidRouterId : aParentId;

        if (router->SetNextHopAndCost(nextHopId, cost))
        {
            SignalTableChanged();
        }
    }
}

void RouterTable::FillRouteTlv(Mle::RouteTlv &aRouteTlv, const Neighbor *aNeighbor) const
{
    uint8_t          routerIdSequence = mRouterIdSequence;
    Mle::RouterIdSet routerIdSet;
    uint8_t          routerIndex;

    mRouterIdMap.GetAsRouterIdSet(routerIdSet);

    if ((aNeighbor != nullptr) && Mle::IsRouterRloc16(aNeighbor->GetRloc16()))
    {
        // Sending a Link Accept message that may require truncation
        // of Route64 TLV.

        uint8_t routerCount = mRouters.GetLength();

        if (routerCount > kMaxRoutersInRouteTlvForLinkAccept)
        {
            for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
            {
                if (routerCount <= kMaxRoutersInRouteTlvForLinkAccept)
                {
                    break;
                }

                if (Get<Mle::Mle>().MatchesRouterId(routerId) || (routerId == aNeighbor->GetRouterId()) ||
                    (routerId == Get<Mle::Mle>().GetLeaderId()))
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
            routerIdSequence -= kLinkAcceptSequenceRollback;
        }
    }

    aRouteTlv.SetRouterIdSequence(routerIdSequence);
    aRouteTlv.SetRouterIdMask(routerIdSet);

    routerIndex = 0;

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        uint16_t routerRloc16;

        if (!routerIdSet.Contains(routerId))
        {
            continue;
        }

        routerRloc16 = Mle::Rloc16FromRouterId(routerId);

        if (Get<Mle::Mle>().HasRloc16(routerRloc16))
        {
            aRouteTlv.SetRouteData(routerIndex, kLinkQuality0, kLinkQuality0, 1);
        }
        else
        {
            const Router *router = FindRouterById(routerId);
            uint8_t       pathCost;

            OT_ASSERT(router != nullptr);

            pathCost = GetPathCost(routerRloc16);

            if (pathCost >= Mle::kMaxRouteCost)
            {
                pathCost = 0;
            }

            aRouteTlv.SetRouteData(routerIndex, router->GetLinkQualityIn(), router->GetLinkQualityOut(), pathCost);
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
    if (GetLeaderAge() >= kRouterIdSequencePeriod)
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

void RouterTable::SignalTableChanged(void) { mChangedTask.Post(); }

void RouterTable::HandleTableChanged(void)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    LogRouteTable();
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordRouterTableChange();
#endif

    Get<Mle::MleRouter>().UpdateAdvertiseInterval();
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

        if (Get<Mle::Mle>().HasRloc16(router.GetRloc16()))
        {
            string.Append(" - me");
        }
        else if (Get<Mle::Mle>().IsChild() && (router.GetRloc16() == Get<Mle::Mle>().GetParent().GetRloc16()))
        {
            string.Append(" - parent");
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
