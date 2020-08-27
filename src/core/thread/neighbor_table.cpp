/*
 *  Copyright (c) 2016-2020, The OpenThread Authors.
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
 *   This file includes definitions for Thread neighbor table.
 */

#include "neighbor_table.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "thread/dua_manager.hpp"

namespace ot {

NeighborTable::NeighborTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCallback(nullptr)
{
}

Neighbor *NeighborTable::FindParent(const Neighbor::AddressMatcher &aMatcher)
{
    Neighbor *neighbor = nullptr;
    Mle::Mle &mle      = Get<Mle::Mle>();

    if (mle.GetParent().Matches(aMatcher))
    {
        neighbor = &mle.GetParent();
    }
    else if (mle.GetParentCandidate().Matches(aMatcher))
    {
        neighbor = &mle.GetParentCandidate();
    }

    return neighbor;
}

Neighbor *NeighborTable::FindParent(Mac::ShortAddress aShortAddress, Neighbor::StateFilter aFilter)
{
    return FindParent(Neighbor::AddressMatcher(aShortAddress, aFilter));
}

Neighbor *NeighborTable::FindParent(const Mac::ExtAddress &aExtAddress, Neighbor::StateFilter aFilter)
{
    return FindParent(Neighbor::AddressMatcher(aExtAddress, aFilter));
}

Neighbor *NeighborTable::FindParent(const Mac::Address &aMacAddress, Neighbor::StateFilter aFilter)
{
    return FindParent(Neighbor::AddressMatcher(aMacAddress, aFilter));
}

Neighbor *NeighborTable::FindNeighbor(const Neighbor::AddressMatcher &aMatcher)
{
    Neighbor *neighbor = nullptr;

#if OPENTHREAD_FTD
    if (Get<Mle::Mle>().IsRouterOrLeader())
    {
        neighbor = FindChildOrRouter(aMatcher);
    }

    if (neighbor == nullptr)
#endif
    {
        neighbor = FindParent(aMatcher);
    }

    return neighbor;
}

Neighbor *NeighborTable::FindNeighbor(Mac::ShortAddress aShortAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = nullptr;

    VerifyOrExit((aShortAddress != Mac::kShortAddrBroadcast) && (aShortAddress != Mac::kShortAddrInvalid), OT_NOOP);
    neighbor = FindNeighbor(Neighbor::AddressMatcher(aShortAddress, aFilter));

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindNeighbor(const Mac::ExtAddress &aExtAddress, Neighbor::StateFilter aFilter)
{
    return FindNeighbor(Neighbor::AddressMatcher(aExtAddress, aFilter));
}

Neighbor *NeighborTable::FindNeighbor(const Mac::Address &aMacAddress, Neighbor::StateFilter aFilter)
{
    return FindNeighbor(Neighbor::AddressMatcher(aMacAddress, aFilter));
}

#if OPENTHREAD_FTD

Neighbor *NeighborTable::FindChildOrRouter(const Neighbor::AddressMatcher &aMatcher)
{
    Neighbor *neighbor;

    neighbor = Get<ChildTable>().FindChild(aMatcher);

    if (neighbor == nullptr)
    {
        neighbor = Get<RouterTable>().FindRouter(aMatcher);
    }

    return neighbor;
}

Neighbor *NeighborTable::FindNeighbor(const Ip6::Address &aIp6Address, Neighbor::StateFilter aFilter)
{
    Neighbor *   neighbor = nullptr;
    Mac::Address macAddresss;

    if (aIp6Address.IsLinkLocal())
    {
        aIp6Address.GetIid().ConvertToMacAddress(macAddresss);
    }

    if (Get<Mle::Mle>().IsRoutingLocator(aIp6Address))
    {
        macAddresss.SetShort(aIp6Address.GetIid().GetLocator());
    }

    if (!macAddresss.IsNone())
    {
        neighbor = FindChildOrRouter(Neighbor::AddressMatcher(macAddresss, aFilter));
        ExitNow();
    }

    for (Child &child : Get<ChildTable>().Iterate(aFilter))
    {
        if (child.HasIp6Address(aIp6Address))
        {
            ExitNow(neighbor = &child);
        }
    }

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindRxOnlyNeighborRouter(const Mac::Address &aMacAddress)
{
    Neighbor *neighbor = nullptr;

    VerifyOrExit(Get<Mle::Mle>().IsChild(), OT_NOOP);
    neighbor = Get<RouterTable>().GetNeighbor(aMacAddress);

exit:
    return neighbor;
}

otError NeighborTable::GetNextNeighborInfo(otNeighborInfoIterator &aIterator, Neighbor::Info &aNeighInfo)
{
    otError error = OT_ERROR_NONE;
    int16_t index;

    // Non-negative iterator value gives the Child index into child table

    if (aIterator >= 0)
    {
        for (index = aIterator;; index++)
        {
            Child *child = Get<ChildTable>().GetChildAtIndex(static_cast<uint16_t>(index));

            if (child == nullptr)
            {
                break;
            }

            if (child->IsStateValid())
            {
                aNeighInfo.SetFrom(*child);
                aNeighInfo.mIsChild = true;
                index++;
                aIterator = index;
                ExitNow();
            }
        }

        aIterator = 0;
    }

    // Negative iterator value gives the current index into mRouters array

    for (index = -aIterator; index <= Mle::kMaxRouterId; index++)
    {
        Router *router = Get<RouterTable>().GetRouter(static_cast<uint8_t>(index));

        if (router != nullptr && router->IsStateValid())
        {
            aNeighInfo.SetFrom(*router);
            aNeighInfo.mIsChild = false;
            index++;
            aIterator = -index;
            ExitNow();
        }
    }

    aIterator = -index;
    error     = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

#endif // OPENTHREAD_FTD

#if OPENTHREAD_MTD

otError NeighborTable::GetNextNeighborInfo(otNeighborInfoIterator &aIterator, Neighbor::Info &aNeighInfo)
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(aIterator == OT_NEIGHBOR_INFO_ITERATOR_INIT, OT_NOOP);

    aIterator++;
    VerifyOrExit(Get<Mle::Mle>().GetParent().IsStateValid(), OT_NOOP);

    aNeighInfo.SetFrom(Get<Mle::Mle>().GetParent());
    aNeighInfo.mIsChild = false;
    error               = OT_ERROR_NONE;

exit:
    return error;
}

#endif

void NeighborTable::Signal(Event aEvent, const Neighbor &aNeighbor)
{
    if (mCallback != nullptr)
    {
        EntryInfo info;

        info.mInstance = &GetInstance();

        switch (aEvent)
        {
        case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
            static_cast<Child::Info &>(info.mInfo.mChild).SetFrom(static_cast<const Child &>(aNeighbor));
            break;

        case OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED:
        case OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED:
            static_cast<Neighbor::Info &>(info.mInfo.mRouter).SetFrom(aNeighbor);
            break;
        }

        mCallback(aEvent, &info);
    }

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitNeighborChange(aEvent, aNeighbor);
#endif

    switch (aEvent)
    {
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        Get<Notifier>().Signal(kEventThreadChildAdded);
        break;

    case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
        Get<Notifier>().Signal(kEventThreadChildRemoved);
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
        Get<DuaManager>().UpdateChildDomainUnicastAddress(static_cast<const Child &>(aNeighbor),
                                                          Mle::ChildDuaState::kRemoved);
#endif
        break;

    default:
        break;
    }
}

} // namespace ot
