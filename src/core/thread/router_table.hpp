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

#include "common/encoding.hpp"
#include "common/locator.hpp"
#include "mac/mac_frame.hpp"
#include "thread/mle_constants.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

#if OPENTHREAD_FTD

class RouterTable : public InstanceLocator
{
public:
    /**
     * This class represents an iterator for iterating through entries in the router table.
     *
     */
    class Iterator : public InstanceLocator
    {
    public:
        /**
         * This constructor initializes an `Iterator` instance to start from beginning of the router table.
         *
         * @param[in] aInstance  A reference to the OpenThread instance.
         *
         */
        Iterator(Instance &aInstance);

        /**
         * This method resets the iterator to start over.
         *
         */
        void Reset(void);

        /**
         * This method indicates if the iterator has reached the end of the list.
         *
         * @retval TRUE   The iterator has reached the end of the list.
         * @retval FALSE  The iterator currently points to a valid entry.
         *
         */
        bool IsDone(void) const { return (mRouter == NULL); }

        /**
         * This method advances the iterator.
         *
         * The iterator is moved to point to the next entry.  If there are no more entries matching the iterator
         * becomes empty (i.e., `GetRouter()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void Advance(void);

        /**
         * This method overloads `++` operator (pre-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next entry.  If there are no more entries matching the iterator
         * becomes empty (i.e., `GetRouter()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void operator++(void) { Advance(); }

        /**
         * This method overloads `++` operator (post-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next entry.  If there are no more entries matching the iterator
         * becomes empty (i.e., `GetRouter()` returns `NULL` and `IsDone()` returns `true`).
         *
         */
        void operator++(int) { Advance(); }

        /**
         * This method gets the entry to which the iterator is currently pointing.
         *
         * @returns A pointer to the current entry, or `NULL` if the iterator is done/empty.
         *
         */
        Router *GetRouter(void) { return mRouter; }

    private:
        Router *mRouter;
    };

    /**
     * Constructor.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    RouterTable(Instance &aInstance);

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
     * This method allocates a router with a random router id.
     *
     * @returns A pointer to the allocated router or NULL if a router ID is not available.
     *
     */
    Router *Allocate(void);

    /**
     * This method allocates a router with a specified router id.
     *
     * @returns A pointer to the allocated router or NULL if the router id could not be allocated.
     *
     */
    Router *Allocate(uint8_t aRouterId);

    /**
     * This method releases a router id.
     *
     * @param[in]  aRouterId  The router id.
     *
     * @retval OT_ERROR_NONE           Successfully released the router id.
     * @retval OT_ERROR_INVALID_STATE  The device is not currently operating as a leader.
     * @retval OT_ERROR_NOT_FOUND      The router id is not currently allocated.
     *
     */
    otError Release(uint8_t aRouterId);

    /**
     * This method removes a neighboring router link.
     *
     * @param[in]  aRouter  A reference to the router.
     *
     */
    void RemoveNeighbor(Router &aRouter);

    /**
     * This method returns the number of active routers in the Thread network.
     *
     * @returns The number of active routers in the Thread network.
     *
     */
    uint8_t GetActiveRouterCount(void) const { return mActiveRouterCount; }

    /**
     * This method returns the number of active links with neighboring routers.
     *
     * @returns The number of active links with neighboring routers.
     *
     */
    uint8_t GetActiveLinkCount(void) const;

    /**
     * This method returns the leader in the Thread network.
     *
     * @returns A pointer to the Leader in the Thread network.
     *
     */
    Router *GetLeader(void);

    /**
     * This method returns the time in seconds since the last Router ID Sequence update.
     *
     * @returns The time in seconds since the last Router ID Sequence update.
     *
     */
    uint32_t GetLeaderAge(void) const;

    /**
     * This method returns the link cost for a neighboring router.
     *
     * @param[in]  aRouter  A reference to the router.
     *
     * @returns The link cost.
     *
     */
    uint8_t GetLinkCost(Router &aRouter);

    /**
     * This method returns the neighbor for a given RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 value.
     *
     * @returns A pointer to the router or NULL if the router could not be found.
     *
     */
    Router *GetNeighbor(uint16_t aRloc16);

    /**
     * This method returns the neighbor for a given IEEE Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the IEEE Extended Address.
     *
     * @returns A pointer to the router or NULL if the router could not be found.
     *
     */
    Router *GetNeighbor(const Mac::ExtAddress &aExtAddress);

    /**
     * This method returns the router for a given router id.
     *
     * @param[in]  aRouterId  The router id.
     *
     * @returns A pointer to the router or NULL if the router could not be found.
     *
     */
    Router *GetRouter(uint8_t aRouterId)
    {
        return const_cast<Router *>(const_cast<const RouterTable *>(this)->GetRouter(aRouterId));
    }

    /**
     * This method returns the router for a given router id.
     *
     * @param[in]  aRouterId  The router id.
     *
     * @returns A pointer to the router or NULL if the router could not be found.
     *
     */
    const Router *GetRouter(uint8_t aRouterId) const;

    /**
     * This method returns the router for a given IEEE Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the IEEE Extended Address.
     *
     * @returns A pointer to the router or NULL if the router could not be found.
     *
     */
    Router *GetRouter(const Mac::ExtAddress &aExtAddress);

    /**
     * This method retains diagnostic information for a given router.
     *
     * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
     * @param[out]  aRouterInfo  The router information.
     *
     * @retval OT_ERROR_NONE          Successfully retrieved the router info for given id.
     * @retval OT_ERROR_INVALID_ARGS  @p aRouterId is not a valid value for a router.
     * @retval OT_ERROR_NOT_FOUND     No router entry with the given id.
     *
     */
    otError GetRouterInfo(uint16_t aRouterId, otRouterInfo &aRouterInfo);

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
    uint32_t GetRouterIdSequenceLastUpdated(void) const { return mRouterIdSequenceLastUpdated; }

    /**
     * This method returns the number of neighbor links.
     *
     * @returns The number of neighbor links.
     *
     */
    uint8_t GetNeighborCount(void) const;

    /**
     * This method indicates whether or not @p aRouterId is allocated.
     *
     * @retval TRUE if @p aRouterId is allocated.
     * @retval FALSE if @p aRouterId is not allocated.
     *
     */
    bool IsAllocated(uint8_t aRouterId) const;

    /**
     * This method updates the router table with a received Route TLV.
     *
     * @param[in]  aRoute  A reference to the Route TLV.
     *
     */
    void ProcessTlv(const Mle::RouteTlv &aRoute);

    /**
     * This method updates the router table with a received Router Mask TLV.
     *
     * @param[in]  aTlv  A reference to the Router Mask TLV.
     *
     */
    void ProcessTlv(const ThreadRouterMaskTlv &Tlv);

    /**
     * This method updates the router table and must be called with a one second period.
     *
     */
    void ProcessTimerTick(void);

private:
    void          UpdateAllocation(void);
    const Router *GetFirstEntry(void) const;
    const Router *GetNextEntry(const Router *aRouter) const;
    Router *GetFirstEntry(void) { return const_cast<Router *>(const_cast<const RouterTable *>(this)->GetFirstEntry()); }
    Router *GetNextEntry(Router *aRouter)
    {
        return const_cast<Router *>(const_cast<const RouterTable *>(this)->GetNextEntry(aRouter));
    }

    Router   mRouters[Mle::kMaxRouters];
    uint8_t  mAllocatedRouterIds[BitVectorBytes(Mle::kMaxRouterId + 1)];
    uint8_t  mRouterIdReuseDelay[Mle::kMaxRouterId + 1];
    uint32_t mRouterIdSequenceLastUpdated;
    uint8_t  mRouterIdSequence;
    uint8_t  mActiveRouterCount;
};

#endif // OPENTHREAD_FTD

#if OPENTHREAD_MTD

class RouterTable
{
public:
    class Iterator
    {
    public:
        Iterator(Instance &) {}
        void    Reset(void) {}
        bool    IsDone(void) const { return true; }
        void    Advance(void) {}
        void    operator++(void) {}
        void    operator++(int) {}
        Router *GetRouter(void) { return NULL; }
    };

    explicit RouterTable(Instance &) {}

    uint8_t GetNeighborCount(void) const { return 0; }
};

#endif // OPENTHREAD_MTD

} // namespace ot

#endif // ROUTER_TABLE_HPP_
