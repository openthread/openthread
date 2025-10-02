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

#include "border_router/br_tracker.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

RegisterLogModule("BrTracker");

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

NetDataBrTracker::NetDataBrTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

bool NetDataBrTracker::BrMatchesFilter(const BorderRouter &aEntry, Filter aFilter) const
{
    bool matches = true;

    switch (aFilter)
    {
    case kAllBorderRouters:
        break;
    case kExcludeThisDevice:
        matches = !Get<Mle::Mle>().HasRloc16(aEntry.mRloc16);
        break;
    }

    return matches;
}

uint16_t NetDataBrTracker::CountBrs(Filter aFilter, uint32_t &aMinAge) const
{
    uint32_t uptime = Get<Uptime>().GetUptimeInSeconds();
    uint16_t count  = 0;

    SetToUintMax(aMinAge);

    for (const BorderRouter &entry : mBorderRouters)
    {
        if (!BrMatchesFilter(entry, aFilter))
        {
            continue;
        }

        count++;
        aMinAge = Min(aMinAge, entry.GetAge(uptime));
    }

    if (count == 0)
    {
        aMinAge = 0;
    }

    return count;
}

Error NetDataBrTracker::GetNext(Filter aFilter, TableIterator &aIterator, BorderRouterEntry &aEntry) const
{
    using Iterator = RxRaTracker::Iterator;

    Iterator           &iterator = static_cast<Iterator &>(aIterator);
    Error               error    = kErrorNone;
    const BorderRouter *entry;

    do
    {
        if (iterator.GetType() == Iterator::kUnspecified)
        {
            iterator.SetType(Iterator::kNetDataBrIterator);
            entry = mBorderRouters.GetHead();
        }
        else
        {
            VerifyOrExit(iterator.GetType() == Iterator::kNetDataBrIterator, error = kErrorInvalidArgs);
            entry = static_cast<const BorderRouter *>(iterator.GetEntry());
            VerifyOrExit(entry != nullptr, error = kErrorNotFound);
            entry = entry->GetNext();
        }

        VerifyOrExit(entry != nullptr, error = kErrorNotFound);

        iterator.SetEntry(entry);

    } while (!BrMatchesFilter(*entry, aFilter));

    aEntry.mRloc16 = entry->mRloc16;
    aEntry.mAge    = entry->GetAge(iterator.GetInitUptime());

exit:
    return error;
}

void NetDataBrTracker::HandleNotifierEvents(Events aEvents)
{
    NetworkData::Rlocs rlocs;

    VerifyOrExit(aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged));

    Get<NetworkData::Leader>().FindRlocs(NetworkData::kBrProvidingExternalIpConn, NetworkData::kAnyRole, rlocs);

    // Remove `BorderRouter` entries no longer found in Network Data
    // Then allocate and add entries for newly discovered BRs.

    mBorderRouters.RemoveAndFreeAllMatching(BorderRouter::RlocFilter(rlocs));

    for (uint16_t rloc16 : rlocs)
    {
        BorderRouter *newEntry;

        if (mBorderRouters.ContainsMatching(rloc16))
        {
            continue;
        }

        newEntry = BorderRouter::Allocate();
        VerifyOrExit(newEntry != nullptr, LogWarn("Failed to allocate `BorderRouter` entry"));

        newEntry->mRloc16       = rloc16;
        newEntry->mDiscoverTime = Get<Uptime>().GetUptimeInSeconds();

        mBorderRouters.Push(*newEntry);
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE
    Get<RoutingManager>().mMultiAilDetector.Evaluate();
#endif

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
