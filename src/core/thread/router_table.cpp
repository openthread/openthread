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
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/timer.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

RouterTable::Iterator::Iterator(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRouter(NULL)
{
    Reset();
}

void RouterTable::Iterator::Reset(void)
{
    mRouter = Get<RouterTable>().GetFirstEntry();
}

void RouterTable::Iterator::Advance(void)
{
    mRouter = Get<RouterTable>().GetNextEntry(mRouter);
}

RouterTable::RouterTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRouterIdSequenceLastUpdated(0)
    , mRouterIdSequence(Random::NonCrypto::GetUint8())
    , mActiveRouterCount(0)
{
    Clear();
}

const Router *RouterTable::GetFirstEntry(void) const
{
    const Router *router = &mRouters[0];
    VerifyOrExit(router->GetRloc16() != 0xffff, router = NULL);

exit:
    return router;
}

const Router *RouterTable::GetNextEntry(const Router *aRouter) const
{
    VerifyOrExit(aRouter != NULL);
    aRouter++;
    VerifyOrExit(aRouter < &mRouters[Mle::kMaxRouters], aRouter = NULL);
    VerifyOrExit(aRouter->GetRloc16() != 0xffff, aRouter = NULL);

exit:
    return aRouter;
}

void RouterTable::Clear(void)
{
    mAllocatedRouterIds.Clear();
    memset(mRouterIdReuseDelay, 0, sizeof(mRouterIdReuseDelay));
    UpdateAllocation();
}

void RouterTable::ClearNeighbors(void)
{
    for (uint8_t index = 0; index < Mle::kMaxRouters; index++)
    {
        Router &router = mRouters[index];

        if (router.IsStateValid())
        {
            Get<Mle::MleRouter>().Signal(OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED, router);
        }

        router.SetState(Neighbor::kStateInvalid);
    }
}

bool RouterTable::IsAllocated(uint8_t aRouterId) const
{
    return mAllocatedRouterIds.Contains(aRouterId);
}

void RouterTable::UpdateAllocation(void)
{
    uint8_t indexMap[Mle::kMaxRouterId + 1];

    mActiveRouterCount = 0;

    // build index map
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (IsAllocated(routerId) && mActiveRouterCount < Mle::kMaxRouters)
        {
            indexMap[routerId] = mActiveRouterCount++;
        }
        else
        {
            indexMap[routerId] = Mle::kInvalidRouterId;
        }
    }

    // shift entries forward
    for (int index = Mle::kMaxRouters - 2; index >= 0; index--)
    {
        uint8_t routerId = mRouters[index].GetRouterId();
        uint8_t newIndex;

        if (routerId > Mle::kMaxRouterId || indexMap[routerId] == Mle::kInvalidRouterId)
        {
            continue;
        }

        newIndex = indexMap[routerId];

        if (newIndex > index)
        {
            mRouters[newIndex] = mRouters[index];
        }
    }

    // shift entries backward
    for (uint8_t index = 1; index < Mle::kMaxRouters; index++)
    {
        uint8_t routerId = mRouters[index].GetRouterId();
        uint8_t newIndex;

        if (routerId > Mle::kMaxRouterId || indexMap[routerId] == Mle::kInvalidRouterId)
        {
            continue;
        }

        newIndex = indexMap[routerId];

        if (newIndex < index)
        {
            mRouters[newIndex] = mRouters[index];
        }
    }

    // fix replaced entries
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        uint8_t index = indexMap[routerId];

        if (index != Mle::kInvalidRouterId)
        {
            Router &router = mRouters[index];

            if (router.GetRouterId() != routerId)
            {
                router.Clear();
                router.SetRloc16(Mle::Mle::GetRloc16(routerId));
                router.SetNextHop(Mle::kInvalidRouterId);
            }
        }
    }

    // clear unused entries
    for (uint8_t index = mActiveRouterCount; index < Mle::kMaxRouters; index++)
    {
        Router &router = mRouters[index];
        router.Clear();
        router.SetRloc16(0xffff);
    }
}

Router *RouterTable::Allocate(void)
{
    Router *rval         = NULL;
    uint8_t numAvailable = 0;
    uint8_t freeBit;

    // count available router ids
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (!IsAllocated(routerId) && mRouterIdReuseDelay[routerId] == 0)
        {
            numAvailable++;
        }
    }

    VerifyOrExit(mActiveRouterCount < Mle::kMaxRouters && numAvailable > 0);

    // choose available router id at random
    freeBit = Random::NonCrypto::GetUint8InRange(0, numAvailable);

    // allocate router
    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (IsAllocated(routerId) || mRouterIdReuseDelay[routerId] > 0)
        {
            continue;
        }

        if (freeBit == 0)
        {
            rval = Allocate(routerId);
            assert(rval != NULL);
            ExitNow();
        }

        freeBit--;
    }

exit:
    return rval;
}

Router *RouterTable::Allocate(uint8_t aRouterId)
{
    Router *rval = NULL;

    VerifyOrExit(aRouterId <= Mle::kMaxRouterId && mActiveRouterCount < Mle::kMaxRouters && !IsAllocated(aRouterId) &&
                 mRouterIdReuseDelay[aRouterId] == 0);

    mAllocatedRouterIds.Add(aRouterId);
    UpdateAllocation();

    rval = GetRouter(aRouterId);
    rval->SetLastHeard(TimerMilli::GetNow());

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();
    Get<Mle::MleRouter>().ResetAdvertiseInterval();

    otLogNoteMle("Allocate router id %d", aRouterId);

exit:
    return rval;
}

otError RouterTable::Release(uint8_t aRouterId)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t rloc16 = Mle::Mle::GetRloc16(aRouterId);

    assert(aRouterId <= Mle::kMaxRouterId);

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(IsAllocated(aRouterId), error = OT_ERROR_NOT_FOUND);

    mAllocatedRouterIds.Remove(aRouterId);
    UpdateAllocation();

    mRouterIdReuseDelay[aRouterId] = Mle::kRouterIdReuseDelay;

    for (Router *router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->GetNextHop() == rloc16)
        {
            router->SetNextHop(Mle::kInvalidRouterId);
            router->SetCost(0);
        }
    }

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    Get<AddressResolver>().Remove(aRouterId);
    Get<NetworkData::Leader>().RemoveBorderRouter(rloc16, NetworkData::Leader::kMatchModeRouterId);
    Get<Mle::MleRouter>().ResetAdvertiseInterval();

    otLogNoteMle("Release router id %d", aRouterId);

exit:
    return error;
}

void RouterTable::RemoveNeighbor(Router &aRouter)
{
    aRouter.SetLinkQualityOut(0);
    aRouter.SetLastHeard(TimerMilli::GetNow());

    for (Router *cur = GetFirstEntry(); cur != NULL; cur = GetNextEntry(cur))
    {
        if (cur->GetNextHop() == aRouter.GetRouterId())
        {
            cur->SetNextHop(Mle::kInvalidRouterId);
            cur->SetCost(0);

            if (GetLinkCost(*cur) >= Mle::kMaxRouteCost)
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

uint8_t RouterTable::GetActiveLinkCount(void) const
{
    uint8_t activeLinks = 0;

    for (const Router *router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->IsStateValid())
        {
            activeLinks++;
        }
    }

    return activeLinks;
}

Router *RouterTable::GetNeighbor(uint16_t aRloc16)
{
    Router *router = NULL;

    VerifyOrExit(aRloc16 != Get<Mle::MleRouter>().GetRloc16());

    for (router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->IsStateValid() && router->GetRloc16() == aRloc16)
        {
            ExitNow();
        }
    }

exit:
    return router;
}

Router *RouterTable::GetNeighbor(const Mac::ExtAddress &aExtAddress)
{
    Router *router = NULL;

    VerifyOrExit(aExtAddress != Get<Mac::Mac>().GetExtAddress());

    for (router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->IsStateValid() && router->GetExtAddress() == aExtAddress)
        {
            ExitNow();
        }
    }

exit:
    return router;
}

const Router *RouterTable::GetRouter(uint8_t aRouterId) const
{
    const Router *router = NULL;
    uint16_t      rloc16 = Mle::Mle::GetRloc16(aRouterId);

    for (router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->GetRloc16() == rloc16)
        {
            break;
        }
    }

    return router;
}

Router *RouterTable::GetRouter(const Mac::ExtAddress &aExtAddress)
{
    Router *router = NULL;

    for (router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->GetExtAddress() == aExtAddress)
        {
            break;
        }
    }

    return router;
}

otError RouterTable::GetRouterInfo(uint16_t aRouterId, otRouterInfo &aRouterInfo)
{
    otError error = OT_ERROR_NONE;
    Router *router;
    uint8_t routerId;

    if (aRouterId <= Mle::kMaxRouterId)
    {
        routerId = static_cast<uint8_t>(aRouterId);
    }
    else
    {
        VerifyOrExit(Mle::Mle::IsActiveRouter(aRouterId), error = OT_ERROR_INVALID_ARGS);
        routerId = Mle::Mle::GetRouterId(aRouterId);
        VerifyOrExit(routerId <= Mle::kMaxRouterId, error = OT_ERROR_INVALID_ARGS);
    }

    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = OT_ERROR_NOT_FOUND);

    memset(&aRouterInfo, 0, sizeof(aRouterInfo));
    aRouterInfo.mRouterId        = routerId;
    aRouterInfo.mRloc16          = Mle::Mle::GetRloc16(routerId);
    aRouterInfo.mExtAddress      = router->GetExtAddress();
    aRouterInfo.mAllocated       = true;
    aRouterInfo.mNextHop         = router->GetNextHop();
    aRouterInfo.mLinkEstablished = router->IsStateValid();
    aRouterInfo.mPathCost        = router->GetCost();
    aRouterInfo.mLinkQualityIn   = router->GetLinkInfo().GetLinkQuality();
    aRouterInfo.mLinkQualityOut  = router->GetLinkQualityOut();
    aRouterInfo.mAge             = static_cast<uint8_t>(Time::MsecToSec(TimerMilli::GetNow() - router->GetLastHeard()));

exit:
    return error;
}

Router *RouterTable::GetLeader(void)
{
    return GetRouter(Get<Mle::MleRouter>().GetLeaderId());
}

uint32_t RouterTable::GetLeaderAge(void) const
{
    return (mActiveRouterCount > 0) ? Time::MsecToSec(TimerMilli::GetNow() - mRouterIdSequenceLastUpdated) : 0xffffffff;
}

uint8_t RouterTable::GetNeighborCount(void) const
{
    uint8_t count = 0;

    for (const Router *router = GetFirstEntry(); router != NULL; router = GetNextEntry(router))
    {
        if (router->IsStateValid())
        {
            count++;
        }
    }

    return count;
}

uint8_t RouterTable::GetLinkCost(Router &aRouter)
{
    uint8_t rval = Mle::kMaxRouteCost;

    VerifyOrExit(aRouter.GetRloc16() != Get<Mle::MleRouter>().GetRloc16() && aRouter.IsStateValid());

    rval = aRouter.GetLinkInfo().GetLinkQuality();

    if (rval > aRouter.GetLinkQualityOut())
    {
        rval = aRouter.GetLinkQualityOut();
    }

    rval = Mle::MleRouter::LinkQualityToCost(rval);

exit:
    return rval;
}

void RouterTable::ProcessTlv(const Mle::RouteTlv &aTlv)
{
    bool allocationChanged = false;

    mRouterIdSequence            = aTlv.GetRouterIdSequence();
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (aTlv.IsRouterIdSet(routerId) == IsAllocated(routerId))
        {
            continue;
        }

        allocationChanged = true;

        if (aTlv.IsRouterIdSet(routerId))
        {
            mAllocatedRouterIds.Add(routerId);
        }
        else
        {
            Router *router = GetRouter(routerId);

            assert(router != NULL);
            router->SetNextHop(Mle::kInvalidRouterId);
            RemoveNeighbor(*router);

            mAllocatedRouterIds.Remove(routerId);
        }
    }

    if (allocationChanged)
    {
        UpdateAllocation();
        Get<Mle::MleRouter>().ResetAdvertiseInterval();
    }
}

void RouterTable::ProcessTlv(const ThreadRouterMaskTlv &aTlv)
{
    bool allocationChanged = false;

    mRouterIdSequence            = aTlv.GetIdSequence();
    mRouterIdSequenceLastUpdated = TimerMilli::GetNow();

    for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
    {
        if (aTlv.IsAssignedRouterIdSet(routerId) == IsAllocated(routerId))
        {
            continue;
        }

        allocationChanged = true;

        if (aTlv.IsAssignedRouterIdSet(routerId))
        {
            mAllocatedRouterIds.Add(routerId);
        }
        else
        {
            mAllocatedRouterIds.Remove(routerId);
        }
    }

    if (allocationChanged)
    {
        UpdateAllocation();
        Get<Mle::MleRouter>().ResetAdvertiseInterval();
    }
}

void RouterTable::ProcessTimerTick(void)
{
    Mle::MleRouter &mle = Get<Mle::MleRouter>();

    if (mle.GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        // update router id sequence
        if (GetLeaderAge() >= Mle::kRouterIdSequencePeriod)
        {
            mRouterIdSequence++;
            mRouterIdSequenceLastUpdated = TimerMilli::GetNow();
        }

        for (uint8_t routerId = 0; routerId <= Mle::kMaxRouterId; routerId++)
        {
            if (mRouterIdReuseDelay[routerId] > 0)
            {
                mRouterIdReuseDelay[routerId]--;
            }
        }
    }
}

} // namespace ot

#endif // OPENTHREAD_FTD
