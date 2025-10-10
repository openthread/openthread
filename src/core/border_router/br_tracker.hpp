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
class NetDataBrTracker : public InstanceLocator
{
    friend class ot::Notifier;

public:
    /**
     * Represents an iterator to iterate through the tracked BR entries.
     */
    using TableIterator = otBorderRoutingPrefixTableIterator;

    /**
     * Represents information about a Border Router found in the Network Data.
     */
    using BorderRouterEntry = otBorderRoutingPeerBorderRouterEntry;

    /**
     * Specified the filter to apply when counting or retrieving the tracked Border Routers.
     */
    enum Filter : uint8_t
    {
        kAllBorderRouters,  ///< Include all Border Routers.
        kExcludeThisDevice, ///< Exclude this device itself if acting as BR.
    };

    /**
     * Initializes a `NetDataBrTracker`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit NetDataBrTracker(Instance &aInstance);

    /**
     * Counts the number of tracked Border Routers.
     *
     * The @p aFilter specifies which BRs to include in the count, e.g., if `kExcludeThisDevice` is used then the
     * count does not include this device itself (when it itself is acting as a BR).
     *
     * @param[in]  aFilter   The filter to use when counting BRs.
     * @param[out] aMinAge   Reference to an `uint32_t` to return the minimum age among all peer BRs.
     *                       Age is represented as seconds since appearance of the BR entry in the Network Data.
     *
     * @returns The number of BRs.
     */
    uint16_t CountBrs(Filter aFilter, uint32_t &aMinAge) const;

    /**
     * Iterates over the tracked Border Routers.
     *
     * @param[in]     aFilter    Specifies the filter to apply when retrieving the tracked BRs.
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next BR info, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more BRs in the list.
     */
    Error GetNext(Filter aFilter, TableIterator &aIterator, BorderRouterEntry &aEntry) const;

private:
    struct BorderRouter : LinkedListEntry<BorderRouter>, Heap::Allocatable<BorderRouter>
    {
        struct RlocFilter
        {
            RlocFilter(const NetworkData::Rlocs &aRlocs)
                : mExcludeRlocs(aRlocs)
            {
            }

            const NetworkData::Rlocs &mExcludeRlocs;
        };

        uint32_t GetAge(uint32_t aUptime) const { return aUptime - mDiscoverTime; }
        bool     Matches(uint16_t aRloc16) const { return mRloc16 == aRloc16; }
        bool     Matches(const RlocFilter &aFilter) const { return !aFilter.mExcludeRlocs.Contains(mRloc16); }

        BorderRouter *mNext;
        uint32_t      mDiscoverTime;
        uint16_t      mRloc16;
    };

    bool BrMatchesFilter(const BorderRouter &aEntry, Filter aFilter) const;
    void HandleNotifierEvents(Events aEvents);

    OwningList<BorderRouter> mBorderRouters;
};

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // BR_TRACKER_HPP_
