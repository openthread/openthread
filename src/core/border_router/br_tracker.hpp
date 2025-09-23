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

#ifndef BR_TRACKER_HPP_
#define BR_TRACKER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <openthread/border_routing.h>

#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/owning_list.hpp"
#include "thread/network_data.hpp"

namespace ot {
namespace BorderRouter {

class RoutingManager;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

/**
 * Represents a Network Data BR tracker which discovers and tracks BRs in the Thread Network Data.
 */
class NetDataPeerBrTracker : public InstanceLocator
{
    friend class ot::Notifier;

public:
    /**
     * Represents an iterator to iterate through the tracked BR entries.
     */
    using TableIterator = otBorderRoutingPrefixTableIterator;

    /**
     * Represents information about a peer Border Router found in the Network Data.
     */
    using PeerBrEntry = otBorderRoutingPeerBorderRouterEntry;

    /**
     * Initializes a `NetDataPeerBrTracker`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit NetDataPeerBrTracker(Instance &aInstance);

    /**
     * Counts the number of peer BRs found in the Network Data.
     *
     * The count does not include this device itself (when it itself is acting as a BR).
     *
     * @param[out] aMinAge   Reference to an `uint32_t` to return the minimum age among all peer BRs.
     *                       Age is represented as seconds since appearance of the BR entry in the Network Data.
     *
     * @returns The number of peer BRs.
     */
    uint16_t CountPeerBrs(uint32_t &aMinAge) const;

    /**
     * Iterates over the peer BRs found in the Network Data.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next peer BR info, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more peer BRs in the list.
     */
    Error GetNext(TableIterator &aIterator, PeerBrEntry &aEntry) const;

private:
    struct PeerBr : LinkedListEntry<PeerBr>, Heap::Allocatable<PeerBr>
    {
        struct Filter
        {
            Filter(const NetworkData::Rlocs &aRlocs)
                : mExcludeRlocs(aRlocs)
            {
            }

            const NetworkData::Rlocs &mExcludeRlocs;
        };

        uint32_t GetAge(uint32_t aUptime) const { return aUptime - mDiscoverTime; }
        bool     Matches(uint16_t aRloc16) const { return mRloc16 == aRloc16; }
        bool     Matches(const Filter &aFilter) const { return !aFilter.mExcludeRlocs.Contains(mRloc16); }

        PeerBr  *mNext;
        uint16_t mRloc16;
        uint32_t mDiscoverTime;
    };

    void HandleNotifierEvents(Events aEvents);

    OwningList<PeerBr> mPeerBrs;
};

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // BR_TRACKER_HPP_
