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
#include "mac/mac_types.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

class RouterTable : public InstanceLocator, private NonCopyable
{
    friend class NeighborTable;

public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit RouterTable(Instance &aInstance);

    /**
     * This method clears the router table.
     *
     */
    void Clear(void);

    /**
     * This method removes all neighbor links to routers.
     *
     */
    void ClearNeighbors(void);

    /**
     * This method allocates a router with a random Router ID.
     *
     * @returns A pointer to the allocated router or `nullptr` if a Router ID is not available.
     *
     */
    Router *Allocate(void);

    /**
     * This method allocates a router with a specified Router ID.
     *
     * @param[in] aRouterId   The Router ID to try to allocate.
     *
     * @returns A pointer to the allocated router or `nullptr` if the ID @p aRouterId could not be allocated.
     *
     */
    Router *Allocate(uint8_t aRouterId);

    /**
     * This method releases a Router ID.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @retval kErrorNone          Successfully released the Router ID @p aRouterId.
     * @retval kErrorInvalidState  The device is not currently operating as a leader.
     * @retval kErrorNotFound      The Router ID @p aRouterId is not currently allocated.
     *
     */
    Error Release(uint8_t aRouterId);

    /**
     * This method removes a router link.
     *
     * @param[in]  aRouter  A reference to the router.
     *
     */
    void RemoveRouterLink(Router &aRouter);

    /**
     * This method returns the number of active routers in the Thread network.
     *
     * @returns The number of active routers in the Thread network.
     *
     */
    uint8_t GetActiveRouterCount(void) const { return mRouters.GetLength(); }

    /**
     * This method returns the leader in the Thread network.
     *
     * @returns A pointer to the Leader in the Thread network.
     *
     */
    Router *GetLeader(void) { return AsNonConst(AsConst(this)->GetLeader()); }

    /**
     * This method returns the leader in the Thread network.
     *
     * @returns A pointer to the Leader in the Thread network.
     *
     */
    const Router *GetLeader(void) const;

    /**
     * This method returns the leader's age in seconds, i.e., seconds since the last Router ID Sequence update.
     *
     * @returns The leader's age.
     *
     */
    uint32_t GetLeaderAge(void) const;

    /**
     * This method returns the link cost for a neighboring router.
     *
     * @param[in]  aRouter   A router.
     *
     * @returns The link cost to @p aRouter.
     *
     */
    uint8_t GetLinkCost(const Router &aRouter) const;

    /**
     * This method returns the link cost to the given Router.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     * @returns The link cost to the Router.
     *
     */
    uint8_t GetLinkCost(uint8_t aRouterId) const;

    /**
     * This method returns the minimum mesh path cost to the given RLOC16
     *
     * @param[in]  aDestRloc16  The RLOC16 of destination
     *
     * @returns The minimum mesh path cost to @p aDestRloc16 (via direct link or forwarding).
     *
     */
    uint8_t GetPathCost(uint16_t aDestRloc16) const;

    /**
     * This method returns the mesh path cost to leader.
     *
     * @returns The path cost to leader.
     *
     */
    uint8_t GetPathCostToLeader(void) const;

    /**
     * This method determines the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestRloc16  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, `Mle::kInvalidRloc16` otherwise.
     *
     */
    uint16_t GetNextHop(uint16_t aDestRloc16) const;

    /**
     * This method finds the router for a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    Router *FindRouterById(uint8_t aRouterId) { return AsNonConst(AsConst(this)->FindRouterById(aRouterId)); }

    /**
     * This method finds the router for a given Router ID.
     *
     * @param[in]  aRouterId  The Router ID to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    const Router *FindRouterById(uint8_t aRouterId) const;

    /**
     * This method finds the router for a given RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    Router *FindRouterByRloc16(uint16_t aRloc16) { return AsNonConst(AsConst(this)->FindRouterByRloc16(aRloc16)); }

    /**
     * This method finds the router for a given RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 to search for.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    const Router *FindRouterByRloc16(uint16_t aRloc16) const;

    /**
     * This method finds the router that is the next hop of a given router.
     *
     * @param[in]  aRouter  The router to find next hop of.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    Router *FindNextHopOf(const Router &aRouter) { return AsNonConst(AsConst(this)->FindNextHopOf(aRouter)); }

    /**
     * This method finds the router that is the next hop of a given router.
     *
     * @param[in]  aRouter  The router to find next hop of.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    const Router *FindNextHopOf(const Router &aRouter) const;

    /**
     * This method find the router for a given MAC Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the MAC Extended Address.
     *
     * @returns A pointer to the router or `nullptr` if the router could not be found.
     *
     */
    Router *FindRouter(const Mac::ExtAddress &aExtAddress);

    /**
     * This method indicates whether the router table contains a given `Neighbor` instance.
     *
     * @param[in]  aNeighbor  A reference to a `Neighbor`.
     *
     * @retval TRUE  if @p aNeighbor is a `Router` in the router table.
     * @retval FALSE if @p aNeighbor is not a `Router` in the router table
     *               (i.e. it can be the parent or parent candidate, or a `Child` of the child table).
     *
     */
    bool Contains(const Neighbor &aNeighbor) const
    {
        return mRouters.IsInArrayBuffer(&static_cast<const Router &>(aNeighbor));
    }

    /**
     * This method retains diagnostic information for a given router.
     *
     * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
     * @param[out]  aRouterInfo  The router information.
     *
     * @retval kErrorNone          Successfully retrieved the router info for given id.
     * @retval kErrorInvalidArgs   @p aRouterId is not a valid value for a router.
     * @retval kErrorNotFound      No router entry with the given id.
     *
     */
    Error GetRouterInfo(uint16_t aRouterId, Router::Info &aRouterInfo);

    /**
     * This method returns the Router ID Sequence.
     *
     * @returns The Router ID Sequence.
     *
     */
    uint8_t GetRouterIdSequence(void) const { return mRouterIdSequence; }

    /**
     * This method returns the local time when the Router ID Sequence was last updated.
     *
     * @returns The local time when the Router ID Sequence was last updated.
     *
     */
    TimeMilli GetRouterIdSequenceLastUpdated(void) const { return mRouterIdSequenceLastUpdated; }

    /**
     * This method returns the number of neighbor links.
     *
     * @returns The number of neighbor links.
     *
     */
    uint8_t GetNeighborCount(void) const;

    /**
     * This method indicates whether or not a Router ID is allocated.
     *
     * @param[in] aRouterId  The Router ID.
     *
     * @retval TRUE  if @p aRouterId is allocated.
     * @retval FALSE if @p aRouterId is not allocated.
     *
     */
    bool IsAllocated(uint8_t aRouterId) const { return mRouterIdMap.IsAllocated(aRouterId); }

    /**
     * This method updates the Router ID allocation set.
     *
     * @param[in]  aRouterIdSequence  The Router ID Sequence.
     * @param[in]  aRouterIdSet       The Router ID Set.
     *
     */
    void UpdateRouterIdSet(uint8_t aRouterIdSequence, const Mle::RouterIdSet &aRouterIdSet);

    /**
     * This method updates the routes based on a received `RouteTlv` from a neighboring router.
     *
     * @param[in]  aRouteTlv    The received `RouteTlv`
     * @param[in]  aRouterId    The router ID of neighboring router from which @p aRouteTlv is received.
     *
     */
    void UpdateRoutes(const Mle::RouteTlv &aRouteTlv, uint8_t aRouterId);

    /**
     * This method gets the allocated Router ID set.
     *
     * @returns The allocated Router ID set.
     *
     */
    void GetRouterIdSet(Mle::RouterIdSet &aRouterIdSet) const { return mRouterIdMap.GetAsRouterIdSet(aRouterIdSet); }

    /**
     * This method fills a Route TLV.
     *
     * When @p aNeighbor is not `nullptr`, we limit the number of router entries to `Mle::kLinkAcceptMaxRouters` when
     * populating `aRouteTlv`, so that the TLV can be appended in a Link Accept message. In this case, we ensure to
     * include router entries associated with @p aNeighbor, leader, and this device itself.
     *
     * @param[out] aRouteTlv    A Route TLV to be filled.
     * @param[in]  aNeighbor    A pointer to the receiver (in case TLV is for a Link Accept message).
     *
     */
    void FillRouteTlv(Mle::RouteTlv &aRouteTlv, const Neighbor *aNeighbor = nullptr) const;

    /**
     * This method updates the router table and must be called with a one second period.
     *
     */
    void HandleTimeTick(void);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * This method gets the range of Router IDs.
     *
     * All the Router IDs in the range `[aMinRouterId, aMaxRouterId]` are allowed. This is intended for testing only.
     *
     * @param[out]  aMinRouterId   Reference to return the minimum Router ID.
     * @param[out]  aMaxRouterId   Reference to return the maximum Router ID.
     *
     */
    void GetRouterIdRange(uint8_t &aMinRouterId, uint8_t &aMaxRouterId) const;

    /**
     * This method sets the range of Router IDs.
     *
     * All the Router IDs in the range `[aMinRouterId, aMaxRouterId]` are allowed. This is intended for testing only.
     *
     * @param[in]  aMinRouterId   The minimum Router ID.
     * @param[in]  aMaxRouterId   The maximum Router ID.
     *
     * @retval kErrorNone          Successfully set the Router ID range.
     * @retval kErrorInvalidArgs   The given range is not valid.
     *
     */
    Error SetRouterIdRange(uint8_t aMinRouterId, uint8_t aMaxRouterId);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    /**
     * This method logs the route table.
     *
     */
    void LogRouteTable(void) const;
#else
    void LogRouteTable(void) const {}
#endif

    // The following methods are intended to support range-based `for`
    // loop iteration over the router and should not be used
    // directly.

    Router       *begin(void) { return mRouters.begin(); }
    Router       *end(void) { return mRouters.end(); }
    const Router *begin(void) const { return mRouters.begin(); }
    const Router *end(void) const { return mRouters.end(); }

private:
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

    bool UpdateLinkQualityOut(const Mle::RouteTlv &aRouteTlv, Router &aNeighbor, bool &aResetAdvInterval);

    class RouterIdMap
    {
    public:
        // The `RouterIdMap` tracks which Router IDs are allocated.
        // For allocated IDs, tracks the index of the `Router` entry
        // in `mRouters` array. For unallocated IDs, tracks the
        // remaining reuse delay time (in seconds).

        RouterIdMap(void) { Clear(); }
        void    Clear(void) { memset(mIndexes, 0, sizeof(mIndexes)); }
        bool    IsAllocated(uint8_t aRouterId) const { return (mIndexes[aRouterId] & kAllocatedFlag); }
        uint8_t GetIndex(uint8_t aRouterId) const { return (mIndexes[aRouterId] & kIndexMask); }
        void    SetIndex(uint8_t aRouterId, uint8_t aIndex) { mIndexes[aRouterId] = kAllocatedFlag | aIndex; }
        bool    CanAllocate(uint8_t aRouterId) const { return (mIndexes[aRouterId] == 0); }
        void    Release(uint8_t aRouterId) { mIndexes[aRouterId] = Mle::kRouterIdReuseDelay; }
        void    GetAsRouterIdSet(Mle::RouterIdSet &aRouterIdSet) const;
        void    HandleTimeTick(void);

    private:
        // The high bit in `mIndexes[aRouterId]` indicates whether or
        // not the router ID is allocated. The lower 7 bits give either
        // the index in `mRouter` array or remaining reuse delay time.

        static constexpr uint8_t kAllocatedFlag = 1 << 7;
        static constexpr uint8_t kIndexMask     = 0x7f;

        static_assert(Mle::kRouterIdReuseDelay <= kIndexMask, "Mle::kRouterIdReuseDelay does not fit in 7 bits");

        uint8_t mIndexes[Mle::kMaxRouterId + 1];
    };

    Array<Router, Mle::kMaxRouters> mRouters;
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
