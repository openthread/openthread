/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for Thread EID-to-RLOC mapping and caching.
 */

#ifndef ADDRESS_RESOLVER_HPP_
#define ADDRESS_RESOLVER_HPP_

#include "openthread-core-config.h"

#include "coap/coap.hpp"
#include "common/as_core_type.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "net/icmp6.hpp"
#include "net/udp6.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {

/**
 * @addtogroup core-arp
 *
 * @brief
 *   This module includes definitions for Thread EID-to-RLOC mapping and caching.
 *
 * @{
 */

/**
 * Implements the EID-to-RLOC mapping and caching.
 *
 */
class AddressResolver : public InstanceLocator, private NonCopyable
{
    friend class TimeTicker;
    friend class Tmf::Agent;

    class CacheEntry;
    class CacheEntryList;

public:
    /**
     * Represents an iterator used for iterating through the EID cache table entries.
     *
     */
    class Iterator : public otCacheEntryIterator, public Clearable<Iterator>
    {
        friend class AddressResolver;

        static constexpr uint8_t kListIndex  = 0;
        static constexpr uint8_t kEntryIndex = 1;

        const CacheEntry     *GetEntry(void) const { return static_cast<const CacheEntry *>(mData[kEntryIndex]); }
        void                  SetEntry(const CacheEntry *aEntry) { mData[kEntryIndex] = aEntry; }
        const CacheEntryList *GetList(void) const { return static_cast<const CacheEntryList *>(mData[kListIndex]); }
        void                  SetList(const CacheEntryList *aList) { mData[kListIndex] = aList; }
    };

    /**
     * Represents an EID cache entry.
     *
     */
    class EntryInfo : public otCacheEntryInfo, public Clearable<EntryInfo>
    {
    public:
        enum State : uint8_t ///< Entry state.
        {
            kStateCached     = OT_CACHE_ENTRY_STATE_CACHED,      ///< Cached and in-use.
            kStateSnooped    = OT_CACHE_ENTRY_STATE_SNOOPED,     ///< Created by snoop optimization.
            kStateQuery      = OT_CACHE_ENTRY_STATE_QUERY,       ///< Ongoing query for the EID.
            kStateRetryQuery = OT_CACHE_ENTRY_STATE_RETRY_QUERY, ///< In retry wait mode.
        };
    };

    /**
     * Initializes the object.
     *
     */
    explicit AddressResolver(Instance &aInstance);

#if OPENTHREAD_FTD
    /**
     * Clears the EID-to-RLOC cache.
     *
     */
    void Clear(void);

    /**
     * Gets the information about the next EID cache entry (using an iterator).
     *
     * @param[out]    aInfo      An `EntryInfo` where the EID cache entry information is placed.
     * @param[in,out] aIterator  An iterator. It will be updated to point to the next entry on success.
     *                           To get the first entry, initialize the iterator by setting all its fields to zero.
     *                           e.g., `memset` the the iterator structure to zero.
     *
     * @retval kErrorNone      Successfully populated @p aInfo with the info for the next EID cache entry.
     * @retval kErrorNotFound  No more entries in the address cache table.
     *
     */
    Error GetNextCacheEntry(EntryInfo &aInfo, Iterator &aIterator) const;

    /**
     * Removes the EID-to-RLOC cache entries corresponding to an RLOC16.
     *
     * @param[in]  aRloc16  The RLOC16 address.
     *
     */
    void RemoveEntriesForRloc16(Mac::ShortAddress aRloc16);

    /**
     * Removes all EID-to-RLOC cache entries associated with a Router ID.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     */
    void RemoveEntriesForRouterId(uint8_t aRouterId);

    /**
     * Removes the cache entry for the EID.
     *
     * @param[in]  aEid               A reference to the EID.
     *
     */
    void RemoveEntryForAddress(const Ip6::Address &aEid);

    /**
     * Replaces all EID-to-RLOC cache entries corresponding to an old RLOC16 with a new RLOC16.
     *
     * @param[in] aOldRloc16    The old RLOC16.
     * @param[in] aNewRloc16    The new RLOC16.
     *
     */
    void ReplaceEntriesForRloc16(uint16_t aOldRloc16, uint16_t aNewRloc16);

    /**
     * Updates an existing entry or adds a snooped cache entry for a given EID.
     *
     * The method is intended to add an entry for snoop optimization (inspection of a received message to create a
     * cache entry mapping an EID to a RLOC).
     *
     * @param[in] aEid             A reference to the EID.
     * @param[in] aRloc16          The RLOC16 corresponding to @p aEid.
     * @param[in] aDest            The short MAC address destination of the received snooped message.
     *
     */
    void UpdateSnoopedCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16, Mac::ShortAddress aDest);

    /**
     * Returns the RLOC16 for a given EID, initiates an Address Query if the mapping is not known.
     *
     * @param[in]   aEid                A reference to the EID.
     * @param[out]  aRloc16             The RLOC16 corresponding to @p aEid.
     *
     * @retval kErrorNone           Successfully provided the RLOC16.
     * @retval kErrorAddressQuery   Initiated an Address Query if allowed.
     * @retval kErrorDrop           Earlier Address Query for the EID timed out. In retry timeout interval.
     * @retval kErrorNoBufs         Insufficient buffer space available to send Address Query.
     *
     */
    Error Resolve(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16)
    {
        return Resolve(aEid, aRloc16, /* aAllowAddressQuery */ true);
    }

    /**
     * Looks up the RLOC16 for a given EID in the address cache.
     *
     * @param[in]   aEid                A reference to the EID.
     *
     * @returns The RLOC16 mapping to @p aEid or `Mac::kShortAddrInvalid` if it is not found in the address cache.
     *
     */
    Mac::ShortAddress LookUp(const Ip6::Address &aEid);

    /**
     * Restarts any ongoing address queries.
     *
     * Any existing address queries will be restarted as if they are being sent for the first time.
     *
     */
    void RestartAddressQueries(void);

    /**
     * Sends an Address Notification (ADDR_NTF.ans) message.
     *
     * @param[in]  aTarget                  The target address of the ADDR_NTF.ans message.
     * @param[in]  aMeshLocalIid            The ML-IID of the ADDR_NTF.ans message.
     * @param[in]  aLastTransactionTimeTlv  A pointer to the Last Transaction Time if the ADDR_NTF.ans message contains
     *                                      a Last Transaction Time TLV.
     * @param[in]  aDestination             The destination to send the ADDR_NTF.ans message.
     *
     */
    void SendAddressQueryResponse(const Ip6::Address             &aTarget,
                                  const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                  const uint32_t                 *aLastTransactionTimeTlv,
                                  const Ip6::Address             &aDestination);

    /**
     * Sends an Address Error Notification (ADDR_ERR.ntf) message.
     *
     * @param aTarget        The target address of the ADDR_ERR.ntf message.
     * @param aMeshLocalIid  The ML-IID of the ADDR_ERR.ntf message.
     * @param aDestination   The destination to send the ADDR_ERR.ntf message.
     *
     */
    void SendAddressError(const Ip6::Address             &aTarget,
                          const Ip6::InterfaceIdentifier &aMeshLocalIid,
                          const Ip6::Address             *aDestination);

private:
    static constexpr uint16_t kCacheEntries = OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES;
    static constexpr uint16_t kMaxNonEvictableSnoopedEntries =
        OT_MAX(1, OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES);

    // All time/delay values are in seconds
    static constexpr uint16_t kAddressQueryTimeout           = OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_TIMEOUT;
    static constexpr uint16_t kAddressQueryInitialRetryDelay = OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_INITIAL_RETRY_DELAY;
    static constexpr uint16_t kAddressQueryMaxRetryDelay     = OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY;
    static constexpr uint16_t kSnoopBlockEvictionTimeout     = OPENTHREAD_CONFIG_TMF_SNOOP_CACHE_ENTRY_TIMEOUT;

    class CacheEntry : public InstanceLocatorInit
    {
    public:
        void Init(Instance &aInstance);

        CacheEntry       *GetNext(void);
        const CacheEntry *GetNext(void) const;
        void              SetNext(CacheEntry *aEntry);

        const Ip6::Address &GetTarget(void) const { return mTarget; }
        void                SetTarget(const Ip6::Address &aTarget) { mTarget = aTarget; }

        Mac::ShortAddress GetRloc16(void) const { return mRloc16; }
        void              SetRloc16(Mac::ShortAddress aRloc16) { mRloc16 = aRloc16; }

        const Ip6::InterfaceIdentifier &GetMeshLocalIid(void) const { return mInfo.mCached.mMeshLocalIid; }
        void SetMeshLocalIid(const Ip6::InterfaceIdentifier &aIid) { mInfo.mCached.mMeshLocalIid = aIid; }

        uint32_t GetLastTransactionTime(void) const { return mInfo.mCached.mLastTransactionTime; }
        void     SetLastTransactionTime(uint32_t aTime) { mInfo.mCached.mLastTransactionTime = aTime; }
        bool     IsLastTransactionTimeValid(void) const { return GetLastTransactionTime() != kInvalidLastTransTime; }
        void     MarkLastTransactionTimeAsInvalid(void) { SetLastTransactionTime(kInvalidLastTransTime); }

        void     DecrementTimeout(void) { mInfo.mOther.mTimeout--; }
        bool     IsTimeoutZero(void) const { return mInfo.mOther.mTimeout == 0; }
        uint16_t GetTimeout(void) const { return mInfo.mOther.mTimeout; }
        void     SetTimeout(uint16_t aTimeout) { mInfo.mOther.mTimeout = aTimeout; }

        uint16_t GetRetryDelay(void) const { return mInfo.mOther.mRetryDelay; }
        void     SetRetryDelay(uint16_t aDelay) { mInfo.mOther.mRetryDelay = aDelay; }

        bool CanEvict(void) const { return mInfo.mOther.mCanEvict; }
        void SetCanEvict(bool aCanEvict) { mInfo.mOther.mCanEvict = aCanEvict; }

        bool IsInRampDown(void) const { return mInfo.mOther.mRampDown; }
        void SetRampDown(bool aRampDown) { mInfo.mOther.mRampDown = aRampDown; }

        bool Matches(const Ip6::Address &aEid) const { return GetTarget() == aEid; }

    private:
        static constexpr uint16_t kNoNextIndex          = 0xffff;     // `mNextIndex` value when at end of list.
        static constexpr uint32_t kInvalidLastTransTime = 0xffffffff; // Value when `mLastTransactionTime` is invalid.

        Ip6::Address      mTarget;
        Mac::ShortAddress mRloc16;
        uint16_t          mNextIndex;

        union
        {
            struct
            {
                uint32_t                 mLastTransactionTime;
                Ip6::InterfaceIdentifier mMeshLocalIid;
            } mCached;

            struct
            {
                uint16_t mTimeout;
                uint16_t mRetryDelay;
                bool     mCanEvict;
                bool     mRampDown;
            } mOther;

        } mInfo;
    };

    typedef Pool<CacheEntry, kCacheEntries> CacheEntryPool;

    class CacheEntryList : public LinkedList<CacheEntry>
    {
    };

    enum EntryChange : uint8_t
    {
        kEntryAdded,
        kEntryUpdated,
        kEntryRemoved,
    };

    enum Reason : uint8_t
    {
        kReasonQueryRequest,
        kReasonSnoop,
        kReasonReceivedNotification,
        kReasonRemovingRouterId,
        kReasonRemovingRloc16,
        kReasonReceivedIcmpDstUnreachNoRoute,
        kReasonEvictingForNewEntry,
        kReasonRemovingEid,
    };

    CacheEntryPool &GetCacheEntryPool(void) { return mCacheEntryPool; }

    Error       Resolve(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16, bool aAllowAddressQuery);
    void        Remove(Mac::ShortAddress aRloc16, bool aMatchRouterId);
    void        Remove(const Ip6::Address &aEid, Reason aReason);
    CacheEntry *FindCacheEntry(const Ip6::Address &aEid, CacheEntryList *&aList, CacheEntry *&aPrevEntry);
    CacheEntry *NewCacheEntry(bool aSnoopedEntry);
    void        RemoveCacheEntry(CacheEntry &aEntry, CacheEntryList &aList, CacheEntry *aPrevEntry, Reason aReason);
    Error       UpdateCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16);
    Error       SendAddressQuery(const Ip6::Address &aEid);
#if OPENTHREAD_CONFIG_TMF_ALLOW_ADDRESS_RESOLUTION_USING_NET_DATA_SERVICES
    Error ResolveUsingNetDataServices(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16);
#endif

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

#endif // OPENTHREAD_FTD

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_FTD

    static void HandleIcmpReceive(void                *aContext,
                                  otMessage           *aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  const otIcmp6Header *aIcmpHeader);
    void        HandleIcmpReceive(Message                 &aMessage,
                                  const Ip6::MessageInfo  &aMessageInfo,
                                  const Ip6::Icmp::Header &aIcmpHeader);

    void        HandleTimeTick(void);
    void        LogCacheEntryChange(EntryChange       aChange,
                                    Reason            aReason,
                                    const CacheEntry &aEntry,
                                    CacheEntryList   *aList = nullptr);
    const char *ListToString(const CacheEntryList *aList) const;

    static AddressResolver::CacheEntry *GetEntryAfter(CacheEntry *aPrev, CacheEntryList &aList);

    CacheEntryPool     mCacheEntryPool;
    CacheEntryList     mCachedList;
    CacheEntryList     mSnoopedList;
    CacheEntryList     mQueryList;
    CacheEntryList     mQueryRetryList;
    Ip6::Icmp::Handler mIcmpHandler;

#endif // OPENTHREAD_FTD
};

DeclareTmfHandler(AddressResolver, kUriAddressError);
#if OPENTHREAD_FTD
DeclareTmfHandler(AddressResolver, kUriAddressQuery);
DeclareTmfHandler(AddressResolver, kUriAddressNotify);
#endif

/**
 * @}
 */

DefineCoreType(otCacheEntryIterator, AddressResolver::Iterator);
DefineCoreType(otCacheEntryInfo, AddressResolver::EntryInfo);
DefineMapEnum(otCacheEntryState, AddressResolver::EntryInfo::State);

} // namespace ot

#endif // ADDRESS_RESOLVER_HPP_
