/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#if OPENTHREAD_MTD

#include "sed_capable_neighbor_table.hpp"

#include "common/locator-getters.hpp"

namespace ot {

SedCapableNeighborTable::Iterator::Iterator(Instance &aInstance, SedCapableNeighbor::StateFilter aFilter)
    : InstanceLocator(aInstance)
    , ItemPtrIterator(nullptr)
    , mFilter(aFilter)
{
    Reset();
}

void SedCapableNeighborTable::Iterator::Reset(void)
{
    mItem = &Get<SedCapableNeighborTable>().mNeighbors[0];

    if (!mItem->MatchesFilter(mFilter))
    {
        Advance();
    }
}

void SedCapableNeighborTable::Iterator::Advance(void)
{
    VerifyOrExit(mItem != nullptr);

    do
    {
        mItem++;
        VerifyOrExit(mItem < &Get<SedCapableNeighborTable>().mNeighbors[kMaxRxOffNeighbor], mItem = nullptr);
    } while (!mItem->MatchesFilter(mFilter));

exit:
    return;
}

SedCapableNeighborTable::SedCapableNeighborTable(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    for (SedCapableNeighbor &neighbor : mNeighbors)
    {
        neighbor.Init(aInstance);
        neighbor.Clear();
    }
}

uint16_t SedCapableNeighborTable::GetSedCapableNeighborIndex(const SedCapableNeighbor &aSedCapableNeighbor) const
{
    return static_cast<uint16_t>(&aSedCapableNeighbor - mNeighbors);
}

SedCapableNeighbor *SedCapableNeighborTable::FindSedCapableNeighbor(uint16_t                        aRloc16,
                                                                    SedCapableNeighbor::StateFilter aFilter)
{
    return FindSedCapableNeighbor(SedCapableNeighbor::AddressMatcher(aRloc16, aFilter));
}

SedCapableNeighbor *SedCapableNeighborTable::FindSedCapableNeighbor(const Mac::Address &            aMacAddress,
                                                                    SedCapableNeighbor::StateFilter aFilter)
{
    return FindSedCapableNeighbor(SedCapableNeighbor::AddressMatcher(aMacAddress, aFilter));
}

const SedCapableNeighbor *SedCapableNeighborTable::FindSedCapableNeighbor(
    const SedCapableNeighbor::AddressMatcher &aMatcher) const
{
    const SedCapableNeighbor *neighbor = mNeighbors;

    for (uint16_t num = kMaxRxOffNeighbor; num != 0; num--, neighbor++)
    {
        if (neighbor->Matches(aMatcher))
        {
            ExitNow();
        }
    }

    neighbor = nullptr;

exit:
    return neighbor;
}

SedCapableNeighbor *SedCapableNeighborTable::FindSedCapableNeighbor(const Ip6::Address &            aIp6Address,
                                                                    SedCapableNeighbor::StateFilter aFilter)
{
    SedCapableNeighbor *neighbor = mNeighbors;
    Mac::Address        macAddress;

    if (aIp6Address.IsLinkLocal())
    {
        aIp6Address.GetIid().ConvertToMacAddress(macAddress);
    }

    if (!macAddress.IsNone())
    {
        neighbor = FindSedCapableNeighbor(SedCapableNeighbor::AddressMatcher(macAddress, aFilter));
        ExitNow();
    }

    for (uint16_t num = kMaxRxOffNeighbor; num != 0; num--, neighbor++)
    {
        if (neighbor->MatchesFilter(aFilter) && neighbor->HasIp6Address(aIp6Address))
        {
            ExitNow();
        }
    }

    neighbor = nullptr;

exit:
    return neighbor;
}

} // namespace ot

#endif // OPENTHREAD_MTD
