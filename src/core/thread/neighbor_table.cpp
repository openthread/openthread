/*
 *  Copyright (c) 2016-2019, The OpenThread Authors.
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

namespace ot {

NeighborTable::NeighborTable(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Neighbor *NeighborTable::FindParentNeighbor(Mac::ShortAddress aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = &Get<Mle::Mle>().GetParent();

    if (neighbor->MatchesFilter(aFilter) && (neighbor->GetRloc16() == aAddress))
    {
        ExitNow();
    }

    neighbor = &Get<Mle::Mle>().GetParentCandidate();

    if (neighbor->MatchesFilter(aFilter) && (neighbor->GetRloc16() == aAddress))
    {
        ExitNow();
    }

    neighbor = NULL;

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindParentNeighbor(const Mac::ExtAddress &aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = &Get<Mle::Mle>().GetParent();

    if (neighbor->MatchesFilter(aFilter) && (neighbor->GetExtAddress() == aAddress))
    {
        ExitNow();
    }

    neighbor = &Get<Mle::Mle>().GetParentCandidate();

    if (neighbor->MatchesFilter(aFilter) && (neighbor->GetExtAddress() == aAddress))
    {
        ExitNow();
    }

    neighbor = NULL;

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindParentNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = NULL;

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        neighbor = FindParentNeighbor(aAddress.GetShort(), aFilter);
        break;

    case Mac::Address::kTypeExtended:
        neighbor = FindParentNeighbor(aAddress.GetExtended(), aFilter);
        break;

    default:
        break;
    }

    return neighbor;
}

#if OPENTHREAD_MTD

Neighbor *NeighborTable::FindNeighbor(Mac::ShortAddress aAddress, Neighbor::StateFilter aFilter)
{
    return FindParentNeighbor(aAddress, aFilter);
}

Neighbor *NeighborTable::FindNeighbor(const Mac::ExtAddress &aAddress, Neighbor::StateFilter aFilter)
{
    return FindParentNeighbor(aAddress, aFilter);
}

#elif OPENTHREAD_FTD

Neighbor *NeighborTable::FindNeighbor(Mac::ShortAddress aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = NULL;

    VerifyOrExit((aAddress != Mac::kShortAddrBroadcast) && (aAddress != Mac::kShortAddrInvalid));

    switch (Get<Mle::MleRouter>().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
        break;

    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_CHILD:
        neighbor = FindParentNeighbor(aAddress, aFilter);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        neighbor = Get<ChildTable>().FindChild(aAddress, aFilter);
        VerifyOrExit(neighbor == NULL);
        neighbor = Get<RouterTable>().FindNeighbor(aAddress, aFilter);
        break;
    }

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindNeighbor(const Mac::ExtAddress &aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = NULL;

    switch (Get<Mle::MleRouter>().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
        break;

    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_CHILD:
        neighbor = FindParentNeighbor(aAddress, aFilter);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        neighbor = Get<ChildTable>().FindChild(aAddress, aFilter);
        VerifyOrExit(neighbor == NULL);
        neighbor = Get<RouterTable>().FindNeighbor(aAddress, aFilter);
        VerifyOrExit(neighbor == NULL);
        // If we are attaching, check if the address matches the parent
        // or the parent candidate.
        VerifyOrExit(Get<Mle::MleRouter>().IsAttaching());
        neighbor = FindParentNeighbor(aAddress, aFilter);
        break;
    }

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindNeighbor(const Ip6::Address &aAddress, Neighbor::StateFilter aFilter)
{
    Mac::Address    macAddr;
    Lowpan::Context context;
    Neighbor *      neighbor = NULL;

    if (aAddress.IsLinkLocal())
    {
        if (aAddress.mFields.m16[4] == HostSwap16(0x0000) && aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00))
        {
            macAddr.SetShort(HostSwap16(aAddress.mFields.m16[7]));
        }
        else
        {
            aAddress.ToExtAddress(macAddr);
        }

        ExitNow(neighbor = FindNeighbor(macAddr, aFilter));
    }

    if (Get<NetworkData::Leader>().GetContext(aAddress, context) != OT_ERROR_NONE)
    {
        context.mContextId = 0xff;
    }

    for (ChildTable::Iterator iter(GetInstance(), aFilter); !iter.IsDone(); iter++)
    {
        Child *child = iter.GetChild();

        if (context.mContextId == Mle::kMeshLocalPrefixContextId && aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) && aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(child->GetRloc16()))
        {
            ExitNow(neighbor = child);
        }

        if (child->HasIp6Address(GetInstance(), aAddress))
        {
            ExitNow(neighbor = child);
        }
    }

    VerifyOrExit(context.mContextId == Mle::kMeshLocalPrefixContextId);

    if (aAddress.mFields.m16[4] == HostSwap16(0x0000) && aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
        aAddress.mFields.m16[6] == HostSwap16(0xfe00))
    {
        neighbor = Get<RouterTable>().FindNeighbor(HostSwap16(aAddress.mFields.m16[7]), aFilter);
    }

exit:
    return neighbor;
}

Neighbor *NeighborTable::FindRxOnlyNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = NULL;

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() == OT_DEVICE_ROLE_CHILD);
    neighbor = Get<RouterTable>().FindNeighbor(aAddress, aFilter);

exit:
    return neighbor;
}

#endif // OPENTHREAD_FTD

Neighbor *NeighborTable::FindNeighbor(const Mac::Address &aAddress, Neighbor::StateFilter aFilter)
{
    Neighbor *neighbor = NULL;

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        neighbor = FindNeighbor(aAddress.GetShort(), aFilter);
        break;

    case Mac::Address::kTypeExtended:
        neighbor = FindNeighbor(aAddress.GetExtended(), aFilter);
        break;

    default:
        break;
    }

    return neighbor;
}

} // namespace ot
