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

RegisterLogModule("BorderRouting");

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

NetDataPeerBrTracker::NetDataPeerBrTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

uint16_t NetDataPeerBrTracker::CountPeerBrs(uint32_t &aMinAge) const
{
    uint32_t uptime = Get<Uptime>().GetUptimeInSeconds();
    uint16_t count  = 0;

    aMinAge = NumericLimits<uint16_t>::kMax;

    for (const PeerBr &peerBr : mPeerBrs)
    {
        count++;
        aMinAge = Min(aMinAge, peerBr.GetAge(uptime));
    }

    if (count == 0)
    {
        aMinAge = 0;
    }

    return count;
}

Error NetDataPeerBrTracker::GetNext(TableIterator &aIterator, PeerBrEntry &aEntry) const
{
    using Iterator = RoutingManager::RxRaTracker::Iterator;

    Iterator     &iterator = static_cast<Iterator &>(aIterator);
    Error         error    = kErrorNone;
    const PeerBr *entry;

    if (iterator.GetType() == Iterator::kUnspecified)
    {
        iterator.SetType(Iterator::kNetDataBrIterator);
        entry = mPeerBrs.GetHead();
    }
    else
    {
        VerifyOrExit(iterator.GetType() == Iterator::kNetDataBrIterator, error = kErrorInvalidArgs);
        entry = static_cast<const PeerBr *>(iterator.GetEntry());
        VerifyOrExit(entry != nullptr, error = kErrorNotFound);
        entry = entry->GetNext();
    }

    VerifyOrExit(entry != nullptr, error = kErrorNotFound);

    iterator.SetEntry(entry);

    aEntry.mRloc16 = entry->mRloc16;
    aEntry.mAge    = entry->GetAge(iterator.GetInitUptime());

exit:
    return error;
}

void NetDataPeerBrTracker::HandleNotifierEvents(Events aEvents)
{
    NetworkData::Rlocs rlocs;

    VerifyOrExit(aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged));

    Get<NetworkData::Leader>().FindRlocs(NetworkData::kBrProvidingExternalIpConn, NetworkData::kAnyRole, rlocs);

    // Remove `PeerBr` entries no longer found in Network Data,
    // or they match the device RLOC16. Then allocate and add
    // entries for newly discovered peers.

    mPeerBrs.RemoveAndFreeAllMatching(PeerBr::Filter(rlocs));
    mPeerBrs.RemoveAndFreeAllMatching(Get<Mle::Mle>().GetRloc16());

    for (uint16_t rloc16 : rlocs)
    {
        PeerBr *newEntry;

        if (Get<Mle::Mle>().HasRloc16(rloc16) || mPeerBrs.ContainsMatching(rloc16))
        {
            continue;
        }

        newEntry = PeerBr::Allocate();
        VerifyOrExit(newEntry != nullptr, LogWarn("Failed to allocate `PeerBr` entry"));

        newEntry->mRloc16       = rloc16;
        newEntry->mDiscoverTime = Get<Uptime>().GetUptimeInSeconds();

        mPeerBrs.Push(*newEntry);
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
