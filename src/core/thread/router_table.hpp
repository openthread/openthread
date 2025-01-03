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

#ifndef ROUTER_TABLE_HPP_
#define ROUTER_TABLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include "common/array.hpp"
#include "common/const_cast.hpp"
#include "common/encoding.hpp"
#include "common/iterator_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/serial_number.hpp"
#include "common/tasklet.hpp"
#include "mac/mac_types.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/router.hpp"
#include "thread/thread_tlvs.hpp"

namespace ot {

class RouterTable : public InstanceLocator, private NonCopyable
{
    friend class NeighborTable;

public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit RouterTable(Instance &aInstance);

    /**
     * Clears the router table.
     */
    void Clear(void);

    /**
     * Removes all neighbor links to routers.
     */
    void ClearNeighbors(void);

    /**
     * Allocates a router with a random Router ID.
     *
     * @returns A pointer to the allocated router or `nullptr` if a Router ID is not available.
     */
    Router *Allocate(void);

    /**
     * Allocates a router with a specified Router ID.
     *
     * @param[in] aRouterId   The Router ID to try to allocate.
     *
     * @returns A pointer to the allocated router or `nullptr` if the ID @p aRouterId could not be allocated.
     */
    Router *Allocate(uint8_t aRouterId);

    /**
     * Releases a Router ID.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval kErrorNone          Successfully released the Router ID @p aRouterId.
     * @retval kErrorInvalidState  The device is not currently operating as a leader.
     * @retval kErrorNotFound      The Router ID @p aRouterId is not currently allocated.
     */
    Error Release(uint8_t aRouterId);

    /**
     * Removes a router link.
     *
     * @param[in]  aRouter  A reference to the router.
     */
    void RemoveRouterLink(Router &aRouter);

    /**
     * Returns the number of active routers in the Thread network.
     *
     * @returns The number of active routers in the Thread network.
     */
    uint8_t GetActiveRouterCount(void) const { return mRouters.GetLength(); }

    /**
     * Returns the leader in the Thread network.
     *
     * @returns A pointer to the Leader in the Thread network.
     */
    Router *GetLeader(void) { return AsNonConst(AsConst(this)->GetLeader()); }

    /**
     * Returns the leader in the Thread network.
     *
     * @returns A pointer to the Leader in the Thread network.
     */
    const Router *GetLeader(void) const;

    /**
     * Returns the leader's age in seconds, i.e., seconds since the last Router ID Sequence update.
     *
     * @returns The leader's age.
     */
    uint32_t GetLeaderAge(void) const;

    /**
     * Returns the link cost for a neighboring router.
     *
     * @param[in]  aRouter   A router.
     *
     * @returns The link cost to @p aRouter.
     */
    uint8_t GetLinkCost(const Router &aRouter) const;

    /**
     * Returns the link cost to the given Router.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @returns The link cost to the Router.
     */
    uint8_t GetLinkCost(uint8_t aRouterId) const;

    /**
     * Returns the minimum mesh path cost to the given RLOC16
     *
     * @param[in]  aDestRloc16  The RLOC16 of destination
     *
     * @returns The minimum mesh path cost to @p aDestRloc16 (via direct link or forwarding).
     */
    uint8_t GetPathCost(uint16_t aDestRloc16) const;

    /**
     * Returns the mesh path cost to leader.
     *
     * @returns The path cost to leader.
     */
    uint8_t GetPathCostToLeader(void) const;

    /**
     * Determines the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestRloc16  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, `Mle::kInvalidRloc16` otherwise.
     */
    uint16_t GetNextHop(uint16_t aDestRloc16) const;

    /**
     * Determines the next hop and the path cost towards an RLOC16 destination.
     *
     * @param[in]  aDestRloc16      The RLOC16 of the destination.
     * @param[out] aNextHopRloc16   A reference to return the RLOC16 of next hop if known, or `Mle::kInvalidRloc16`.
     * @param[out] aPathCost        A reference to return the path cost.
     */
    void GetNextHopAndPathCost(uint16_t aDestRloc16, uint16_t &aNextHopRloc16, uint8_t &aPathCost) const;

    /**
     * Finds the router for a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    Router *FindRouterById(uint8_t aRouterId) { return AsNonConst(AsConst(this)->FindRouterById(aRouterId)); }

    /**
     * Finds the router for a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    const Router *FindRouterById(uint8_t aRouterId) const;

    /**
     * Finds the router for a given RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    Router *FindRouterByRloc16(uint16_t aRloc16) { return AsNonConst(AsConst(this)->FindRouterByRloc16(aRloc16)); }

    /**
     * Finds the router for a given RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    const Router *FindRouterByRloc16(uint16_t aRloc16) const;

    /**
     * Finds the router that is the next hop of a given router.
     *
     * @param[in]  aRouter  The router to find next hop of.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    Router *FindNextHopOf(const Router &aRouter) { return AsNonConst(AsConst(this)->FindNextHopOf(aRouter)); }

    /**
     * Finds the router that is the next hop of a given router.
     *
     * @param[in]  aRouter  The router to find next hop of.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    const Router *FindNextHopOf(const Router &aRouter) const;

    /**
     * Find the router for a given MAC Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the MAC Extended Address.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     */
    Router *FindRouter(const Mac::ExtAddress &aExtAddress);

    /**
     * Indicates whether the router table contains a given `Neighbor` instance.
     *
     * @param[in]  aNeighbor  A reference to a `Neighbor`.
     *
     * @retval TRUE  if @p aNeighbor is a `Router` in the router table.
     * @retval FALSE if @p aNeighbor is not a `Router` in the router table
     *               (i.e. it can be the parent or parent candidate, or a `Child` of the child table).
     */
    bool Contains(const Neighbor &aNeighbor) const
    {
        return mRouters.IsInArrayBuffer(&static_cast<const Router &>(aNeighbor));
    }

    /**
     * Retains diagnostic information for a given router.
     *
     * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
     * @param[out]  aRouterInfo  The router information.
     *
     * @retval kErrorNone          Successfully retrieved the router info for given id.
     * @retval kErrorInvalidArgs   @p aRouterId is not a valid value for a router.
     * @retval kErrorNotFound      No router entry with the given id.
     */
    Error GetRouterInfo(uint16_t aRouterId, Router::Info &aRouterInfo);

    /**
     * Returns the Router ID Sequence.
     *
     * @returns The Router ID Sequence.
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * Returns the local time when the Router ID Sequence was last updated.
     *
     * @returns The local time when the Router ID Sequence was last updated.
     */
    TimeMilli GetRouterIdSequenceLastUpdated(void) const { return mRouterIdSequenceLastUpdated; }

    /**
     * Determines whether the Router ID Sequence in a received Route TLV is more recent than the current
     * Router ID Sequence being used by `RouterTable`.
     *
     * @param[in] aRouteTlv   The Route TLV to compare.
     *
     * @retval TRUE    The Router ID Sequence in @p aRouteTlv is more recent.
     * @retval FALSE   The Router ID Sequence in @p aRouteTlv is not more recent.
     */
    bool IsRouteTlvIdSequenceMoreRecent(const Mle::RouteTlv &aRouteTlv) const;

    /**
     * Gets the number of router neighbors with `GetLinkQualityIn()` better than or equal to a given threshold.
     *
     * @param[in] aLinkQuality  Link quality threshold.
     *
     * @returns Number of router neighbors with link quality of @o aLinkQuality or better.
     */
    uint8_t GetNeighborCount(LinkQuality aLinkQuality) const;

    /**
     * Indicates whether or not a Router ID is allocated.
     *
     * @param[in] aRouterId  The Router ID.
     *
     * @retval TRUE  if @p aRouterId is allocated.
     * @retval FALSE if @p aRouterId is not allocated.
     */
    bool IsAllocated(uint8_t aRouterId) const { return mRouterIdMap.IsAllocated(aRouterId); }

    /**
     * Updates the Router ID allocation set.
     *
     * @param[in]  aRouterIdSequence  The Router ID Sequence.
     * @param[in]  aRouterIdSet       The Router ID Set.
     */
    void UpdateRouterIdSet(uint8_t aRouterIdSequence, const Mle::RouterIdSet &aRouterIdSet);

    /**
     * Updates the routes based on a received `RouteTlv` from a neighboring router.
     *
     * @param[in]  aRouteTlv    The received `RouteTlv`
     * @param[in]  aNeighborId  The router ID of neighboring router from which @p aRouteTlv is received.
     */
    void UpdateRoutes(const Mle::RouteTlv &aRouteTlv, uint8_t aNeighborId);

    /**
     * Updates the routes on an FTD child based on a received `RouteTlv` from the parent.
     *
     * MUST be called when device is an FTD child and @p aRouteTlv is received from its current parent.
     *
     * @param[in]  aRouteTlv    The received `RouteTlv` from parent.
     * @param[in]  aParentId    The Router ID of parent.
     */
    void UpdateRouterOnFtdChild(const Mle::RouteTlv &aRouteTlv, uint8_t aParentId);

    /**
     * Gets the allocated Router ID set.
     *
     * @param[out]  aRouterIdSet   A reference to output the allocated Router ID set.
     */
    void GetRouterIdSet(Mle::RouterIdSet &aRouterIdSet) const { return mRouterIdMap.GetAsRouterIdSet(aRouterIdSet); }

    /**
     * Fills a Route TLV.
     *
     * When @p aNeighbor is not `nullptr`, we limit the number of router entries to `kMaxRoutersInRouteTlvForLinkAccept`
     * when populating `aRouteTlv`, so that the TLV can be appended in a Link Accept message. In this case, we ensure
     * to include router entries associated with @p aNeighbor, leader, and this device itself.
     *
     * @param[out] aRouteTlv    A Route TLV to be filled.
     * @param[in]  aNeighbor    A pointer to the receiver (in case TLV is for a Link Accept message).
     */
    void FillRouteTlv(Mle::RouteTlv &aRouteTlv, const Neighbor *aNeighbor = nullptr) const;

    /**
     * Updates the router table and must be called with a one second period.
     */
    void HandleTimeTick(void);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Gets the range of Router IDs.
     *
     * All the Router IDs in the range `[aMinRouterId, aMaxRouterId]` are allowed. This is intended for testing only.
     *
     * @param[out]  aMinRouterId   Reference to return the minimum Router ID.
     * @param[out]  aMaxRouterId   Reference to return the maximum Router ID.
     */
    void GetRouterIdRange(uint8_t &aMinRouterId, uint8_t &aMaxRouterId) const;

    /**
     * Sets the range of Router IDs.
     *
     * All the Router IDs in the range `[aMinRouterId, aMaxRouterId]` are allowed. This is intended for testing only.
     *
     * @param[in]  aMinRouterId   The minimum Router ID.
     * @param[in]  aMaxRouterId   The maximum Router ID.
     *
     * @retval kErrorNone          Successfully set the Router ID range.
     * @retval kErrorInvalidArgs   The given range is not valid.
     */
    Error SetRouterIdRange(uint8_t aMinRouterId, uint8_t aMaxRouterId);
#endif

    // The following methods are intended to support range-based `for`
    // loop iteration over the router and should not be used
    // directly.

    Router       *begin(void) { return mRouters.begin(); }
    Router       *end(void) { return mRouters.end(); }
    const Router *begin(void) const { return mRouters.begin(); }
    const Router *end(void) const { return mRouters.end(); }

private:
    static constexpr uint32_t kRouterIdSequencePeriod     = 10; // in sec
    static constexpr uint8_t  kLinkAcceptSequenceRollback = 64;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    static constexpr uint8_t kMaxRoutersInRouteTlvForLinkAccept = 3;
#else
    static constexpr uint8_t kMaxRoutersInRouteTlvForLinkAccept = 20;
#endif

    Router       *AddRouter(uint8_t aRouterId);
    void          RemoveRouter(Router &aRouter);
    Router       *FindNeighbor(uint16_t aRloc16);
    Router       *FindNeighbor(const Mac::ExtAddress &aExtAddress);
    Router       *FindNeighbor(const Mac::Address &aMacAddress);
    const Router *FindRouter(const Router::AddressMatcher &aMatcher) const;
    Router       *FindRouter(const Router::AddressMatcher &aMatcher)
    {
        return AsNonConst(AsConst(this)->FindRouter(aMatcher));
    }

    void SignalTableChanged(void);
    void HandleTableChanged(void);
    void LogRouteTable(void) const;

    class RouterIdMap : public Clearable<RouterIdMap>
    {
    public:
        // The `RouterIdMap` tracks which Router IDs are allocated.
        // For allocated IDs, tracks the index of the `Router` entry
        // in `mRouters` array. For unallocated IDs, tracks the
        // remaining reuse delay time (in seconds).

        RouterIdMap(void) { Clear(); }
        bool    IsAllocated(uint8_t aRouterId) const { return (mIndexes[aRouterId] & kAllocatedFlag); }
        uint8_t GetIndex(uint8_t aRouterId) const { return (mIndexes[aRouterId] & kIndexMask); }
        void    SetIndex(uint8_t aRouterId, uint8_t aIndex) { mIndexes[aRouterId] = kAllocatedFlag | aIndex; }
        bool    CanAllocate(uint8_t aRouterId) const { return (mIndexes[aRouterId] == 0); }
        void    Release(uint8_t aRouterId) { mIndexes[aRouterId] = kReuseDelay; }
        void    GetAsRouterIdSet(Mle::RouterIdSet &aRouterIdSet) const;
        void    HandleTimeTick(void);

    private:
        // The high bit in `mIndexes[aRouterId]` indicates whether or
        // not the router ID is allocated. The lower 7 bits give either
        // the index in `mRouter` array or remaining reuse delay time.

        static constexpr uint8_t kReuseDelay    = 100; // in sec
        static constexpr uint8_t kAllocatedFlag = 1 << 7;
        static constexpr uint8_t kIndexMask     = 0x7f;

        static_assert(kReuseDelay <= kIndexMask, "kReuseDelay does not fit in 7 bits");

        uint8_t mIndexes[Mle::kMaxRouterId + 1];
    };

    using ChangedTask = TaskletIn<RouterTable, &RouterTable::HandleTableChanged>;

    Array<Router, Mle::kMaxRouters> mRouters;
    ChangedTask                     mChangedTask;
    RouterIdMap                     mRouterIdMap;
    TimeMilli                       mRouterIdSequenceLastUpdated;
    uint8_t                         mRouterIdSequence;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t mMinRouterId;
    uint8_t mMaxRouterId;
#endif
};

} // namespace ot

#endif // OPENTHREAD_FTD

#endif // ROUTER_TABLE_HPP_
