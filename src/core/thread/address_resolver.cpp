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
 *   This file implements Thread's EID-to-RLOC mapping and caching.
 */

#if OPENTHREAD_FTD

#include "address_resolver.hpp"

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/time.hpp"
#include "mac/mac_types.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

AddressResolver::AddressResolver(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mAddressError(UriPath::kAddressError, &AddressResolver::HandleAddressError, this)
    , mAddressQuery(UriPath::kAddressQuery, &AddressResolver::HandleAddressQuery, this)
    , mAddressNotification(UriPath::kAddressNotify, &AddressResolver::HandleAddressNotification, this)
    , mCacheEntryPool(aInstance)
    , mCachedList()
    , mSnoopedList()
    , mQueryList()
    , mQueryRetryList()
    , mIcmpHandler(&AddressResolver::HandleIcmpReceive, this)
{
    Get<Tmf::TmfAgent>().AddResource(mAddressError);
    Get<Tmf::TmfAgent>().AddResource(mAddressQuery);
    Get<Tmf::TmfAgent>().AddResource(mAddressNotification);

    IgnoreError(Get<Ip6::Icmp>().RegisterHandler(mIcmpHandler));
}

void AddressResolver::Clear(void)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList, &mQueryList, &mQueryRetryList};

    for (CacheEntryList *list : lists)
    {
        CacheEntry *entry;

        while ((entry = list->Pop()) != nullptr)
        {
            if (list == &mQueryList)
            {
                Get<MeshForwarder>().HandleResolved(entry->GetTarget(), OT_ERROR_DROP);
            }

            mCacheEntryPool.Free(*entry);
        }
    }
}

otError AddressResolver::GetNextCacheEntry(EntryInfo &aInfo, Iterator &aIterator) const
{
    otError               error = OT_ERROR_NONE;
    const CacheEntryList *list;
    const CacheEntry *    entry;

    list  = reinterpret_cast<const CacheEntryList *>(aIterator.mData[kIteratorListIndex]);
    entry = reinterpret_cast<const CacheEntry *>(aIterator.mData[kIteratorEntryIndex]);

    while (entry == nullptr)
    {
        if (list == nullptr)
        {
            list = &mCachedList;
        }
        else if (list == &mCachedList)
        {
            list = &mSnoopedList;
        }
        else if (list == &mSnoopedList)
        {
            list = &mQueryList;
        }
        else if (list == &mQueryList)
        {
            list = &mQueryRetryList;
        }
        else
        {
            ExitNow(error = OT_ERROR_NOT_FOUND);
        }

        entry = list->GetHead();
    }

    // Update the iterator then populate the `aInfo`.

    aIterator.mData[kIteratorEntryIndex] = entry->GetNext();
    aIterator.mData[kIteratorListIndex]  = list;

    memset(&aInfo, 0, sizeof(aInfo));
    aInfo.mTarget = entry->GetTarget();
    aInfo.mRloc16 = entry->GetRloc16();

    if (list == &mCachedList)
    {
        aInfo.mState          = OT_CACHE_ENTRY_STATE_CACHED;
        aInfo.mCanEvict       = true;
        aInfo.mValidLastTrans = entry->IsLastTransactionTimeValid();

        VerifyOrExit(entry->IsLastTransactionTimeValid(), OT_NOOP);

        aInfo.mLastTransTime = entry->GetLastTransactionTime();
        static_cast<Ip6::Address &>(aInfo.mMeshLocalEid).SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        static_cast<Ip6::Address &>(aInfo.mMeshLocalEid).SetIid(entry->GetMeshLocalIid());

        ExitNow();
    }

    if (list == &mSnoopedList)
    {
        aInfo.mState = OT_CACHE_ENTRY_STATE_SNOOPED;
    }
    else if (list == &mQueryList)
    {
        aInfo.mState = OT_CACHE_ENTRY_STATE_QUERY;
    }
    else
    {
        aInfo.mState = OT_CACHE_ENTRY_STATE_RETRY_QUERY;
    }

    aInfo.mCanEvict   = entry->CanEvict();
    aInfo.mTimeout    = entry->GetTimeout();
    aInfo.mRetryDelay = entry->GetRetryDelay();

exit:
    return error;
}

void AddressResolver::Remove(uint8_t aRouterId)
{
    Remove(Mle::Mle::Rloc16FromRouterId(aRouterId), /* aMatchRouterId */ true);
}

void AddressResolver::Remove(uint16_t aRloc16)
{
    Remove(aRloc16, /* aMatchRouterId */ false);
}

AddressResolver::CacheEntry *AddressResolver::GetEntryAfter(CacheEntry *aPrev, CacheEntryList &aList)
{
    return (aPrev == nullptr) ? aList.GetHead() : aPrev->GetNext();
}

void AddressResolver::Remove(Mac::ShortAddress aRloc16, bool aMatchRouterId)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList};

    for (CacheEntryList *list : lists)
    {
        CacheEntry *prev = nullptr;
        CacheEntry *entry;

        while ((entry = GetEntryAfter(prev, *list)) != nullptr)
        {
            if ((aMatchRouterId && Mle::Mle::RouterIdMatch(entry->GetRloc16(), aRloc16)) ||
                (!aMatchRouterId && (entry->GetRloc16() == aRloc16)))
            {
                RemoveCacheEntry(*entry, *list, prev, aMatchRouterId ? kReasonRemovingRouterId : kReasonRemovingRloc16);
                mCacheEntryPool.Free(*entry);

                // If the entry is removed from list, we keep the same
                // `prev` pointer.
            }
            else
            {
                prev = entry;
            }
        }
    }
}

AddressResolver::CacheEntry *AddressResolver::FindCacheEntry(const Ip6::Address &aEid,
                                                             CacheEntryList *&   aList,
                                                             CacheEntry *&       aPrevEntry)
{
    CacheEntry *    entry   = nullptr;
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList, &mQueryList, &mQueryRetryList};

    for (CacheEntryList *list : lists)
    {
        aList = list;
        entry = aList->FindMatching(aEid, aPrevEntry);
        VerifyOrExit(entry == nullptr, OT_NOOP);
    }

exit:
    return entry;
}

void AddressResolver::Remove(const Ip6::Address &aEid)
{
    Remove(aEid, kReasonRemovingEid);
}

void AddressResolver::Remove(const Ip6::Address &aEid, Reason aReason)
{
    CacheEntry *    entry;
    CacheEntry *    prev;
    CacheEntryList *list;

    entry = FindCacheEntry(aEid, list, prev);
    VerifyOrExit(entry != nullptr, OT_NOOP);

    RemoveCacheEntry(*entry, *list, prev, aReason);
    mCacheEntryPool.Free(*entry);

exit:
    return;
}

AddressResolver::CacheEntry *AddressResolver::NewCacheEntry(bool aSnoopedEntry)
{
    CacheEntry *    newEntry  = nullptr;
    CacheEntry *    prevEntry = nullptr;
    CacheEntryList *lists[]   = {&mSnoopedList, &mQueryRetryList, &mQueryList, &mCachedList};

    // The following order is used when trying to allocate a new cache
    // entry: First the cache pool is checked, followed by the list
    // of snooped entries, then query-retry list (entries in delay
    // retry timeout wait due to a prior query failing to get a
    // response), then the query list (entries actively querying and
    // waiting for address notification response), and finally the
    // cached (in-use) list. Within each list the oldest entry is
    // reclaimed first (the list's tail). We also make sure the entry
    // can be evicted (e.g., first time query entries can not be
    // evicted till timeout).

    newEntry = mCacheEntryPool.Allocate();
    VerifyOrExit(newEntry == nullptr, OT_NOOP);

    for (CacheEntryList *list : lists)
    {
        CacheEntry *prev;
        CacheEntry *entry;
        uint16_t    numNonEvictable = 0;

        for (prev = nullptr; (entry = GetEntryAfter(prev, *list)) != nullptr; prev = entry)
        {
            if ((list != &mCachedList) && !entry->CanEvict())
            {
                numNonEvictable++;
                continue;
            }

            newEntry  = entry;
            prevEntry = prev;
        }

        if (newEntry != nullptr)
        {
            RemoveCacheEntry(*newEntry, *list, prevEntry, kReasonEvictingForNewEntry);
            ExitNow();
        }

        if (aSnoopedEntry && (list == &mSnoopedList))
        {
            // Check if the new entry is being requested for "snoop
            // optimization" (i.e., inspection of a received message).
            // When a new snooped entry is added, we do not allow it
            // to be evicted for a short timeout. This allows some
            // delay for a response message to use the entry (if entry
            // is used it will be moved to the cached list). If a
            // snooped entry is not used after the timeout, we allow
            // it to be evicted. To ensure snooped entries do not
            // overwrite other cached entries, we limit the number of
            // snooped entries that are in timeout mode and cannot be
            // evicted by `kMaxNonEvictableSnoopedEntries`.

            VerifyOrExit(numNonEvictable < kMaxNonEvictableSnoopedEntries, OT_NOOP);
        }
    }

exit:
    return newEntry;
}

void AddressResolver::RemoveCacheEntry(CacheEntry &    aEntry,
                                       CacheEntryList &aList,
                                       CacheEntry *    aPrevEntry,
                                       Reason          aReason)
{
    aList.PopAfter(aPrevEntry);

    if (&aList == &mQueryList)
    {
        Get<MeshForwarder>().HandleResolved(aEntry.GetTarget(), OT_ERROR_DROP);
    }

    LogCacheEntryChange(kEntryRemoved, aReason, aEntry, &aList);
}

otError AddressResolver::UpdateCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    otError         error = OT_ERROR_NONE;
    CacheEntryList *list;
    CacheEntry *    entry;
    CacheEntry *    prev;

    entry = FindCacheEntry(aEid, list, prev);
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NOT_FOUND);

    if ((list == &mCachedList) || (list == &mSnoopedList))
    {
        VerifyOrExit(entry->GetRloc16() != aRloc16, OT_NOOP);
        entry->SetRloc16(aRloc16);
    }
    else
    {
        // Entry is in `mQueryList` or `mQueryRetryList`. Remove it
        // from its current list, update it, and then add it to the
        // `mCachedList`.

        list->PopAfter(prev);

        entry->SetRloc16(aRloc16);
        entry->MarkLastTransactionTimeAsInvalid();
        mCachedList.Push(*entry);

        Get<MeshForwarder>().HandleResolved(aEid, OT_ERROR_NONE);
    }

    LogCacheEntryChange(kEntryUpdated, kReasonSnoop, *entry);

exit:
    return error;
}

void AddressResolver::AddSnoopedCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    uint16_t    numNonEvictable = 0;
    CacheEntry *entry;

    entry = NewCacheEntry(/* aSnoopedEntry */ true);
    VerifyOrExit(entry != nullptr, OT_NOOP);

    for (CacheEntry *snooped = mSnoopedList.GetHead(); snooped != nullptr; snooped = snooped->GetNext())
    {
        if (!snooped->CanEvict())
        {
            numNonEvictable++;
        }
    }

    entry->SetTarget(aEid);
    entry->SetRloc16(aRloc16);

    if (numNonEvictable < kMaxNonEvictableSnoopedEntries)
    {
        entry->SetCanEvict(false);
        entry->SetTimeout(kSnoopBlockEvictionTimeout);

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kAddressResolver);
    }
    else
    {
        entry->SetCanEvict(true);
        entry->SetTimeout(0);
    }

    mSnoopedList.Push(*entry);

    LogCacheEntryChange(kEntryAdded, kReasonSnoop, *entry);

exit:
    return;
}

void AddressResolver::RestartAddressQueries(void)
{
    CacheEntry *tail;

    // We move all entries from `mQueryRetryList` at the tail of
    // `mQueryList` and then (re)send Address Query for all entries in
    // the updated `mQueryList`.

    tail = mQueryList.GetTail();

    if (tail == nullptr)
    {
        mQueryList.SetHead(mQueryRetryList.GetHead());
    }
    else
    {
        tail->SetNext(mQueryRetryList.GetHead());
    }

    mQueryRetryList.Clear();

    for (CacheEntry *entry = mQueryList.GetHead(); entry != nullptr; entry = entry->GetNext())
    {
        IgnoreError(SendAddressQuery(entry->GetTarget()));

        entry->SetTimeout(kAddressQueryTimeout);
        entry->SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry->SetCanEvict(false);
    }
}

otError AddressResolver::Resolve(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16, bool aAllowAddressQuery)
{
    otError         error = OT_ERROR_NONE;
    CacheEntry *    entry;
    CacheEntry *    prev = nullptr;
    CacheEntryList *list;

    entry = FindCacheEntry(aEid, list, prev);

    if (entry == nullptr)
    {
        // If the entry is not present in any of the lists, try to
        // allocate a new entry and perform address query. We do not
        // allow first-time address query entries to be evicted till
        // timeout.
        VerifyOrExit(aAllowAddressQuery, error = OT_ERROR_NOT_FOUND);

        entry = NewCacheEntry(/* aSnoopedEntry */ false);
        VerifyOrExit(entry != nullptr, error = OT_ERROR_NO_BUFS);

        entry->SetTarget(aEid);
        entry->SetRloc16(Mac::kShortAddrInvalid);
        entry->SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry->SetCanEvict(false);
        list = nullptr;
    }

    if ((list == &mCachedList) || (list == &mSnoopedList))
    {
        // Remove the entry from its current list and push it at the
        // head of cached list.

        list->PopAfter(prev);

        if (list == &mSnoopedList)
        {
            entry->MarkLastTransactionTimeAsInvalid();
        }

        mCachedList.Push(*entry);
        aRloc16 = entry->GetRloc16();
        ExitNow();
    }

    // Note that if `aAllowAddressQuery` is `false` then the `entry` is definitely already in a list, i.e., we cannot
    // not get here with `aAllowAddressQuery` being `false` and `entry` being a newly allocated one, due to the
    // `VerifyOrExit` check that `aAllowAddressQuery` is `true` before allocating a new cache entry.
    VerifyOrExit(aAllowAddressQuery, error = OT_ERROR_NOT_FOUND);

    if (list == &mQueryList)
    {
        ExitNow(error = OT_ERROR_ADDRESS_QUERY);
    }

    if (list == &mQueryRetryList)
    {
        // Allow an entry in query-retry mode to resend an Address
        // Query again only if the timeout (retry delay interval) is
        // expired.

        VerifyOrExit(entry->IsTimeoutZero(), error = OT_ERROR_DROP);
        mQueryRetryList.PopAfter(prev);
    }

    entry->SetTimeout(kAddressQueryTimeout);

    error = SendAddressQuery(aEid);
    VerifyOrExit(error == OT_ERROR_NONE, mCacheEntryPool.Free(*entry));

    if (list == nullptr)
    {
        LogCacheEntryChange(kEntryAdded, kReasonQueryRequest, *entry);
    }

    mQueryList.Push(*entry);
    error = OT_ERROR_ADDRESS_QUERY;

exit:
    return error;
}

otError AddressResolver::SendAddressQuery(const Ip6::Address &aEid)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewPriorityMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->InitAsNonConfirmablePost();
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kAddressQuery));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &aEid, sizeof(aEid)));

    messageInfo.GetPeerAddr().SetToRealmLocalAllRoutersMulticast();

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address query for %s", aEid.ToString().AsCString());

exit:

    Get<TimeTicker>().RegisterReceiver(TimeTicker::kAddressResolver);
    FreeMessageOnError(message, error);
    return error;
}

void AddressResolver::HandleAddressNotification(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressNotification(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressNotification(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::Address             target;
    Ip6::InterfaceIdentifier meshLocalIid;
    uint16_t                 rloc16;
    uint32_t                 lastTransactionTime;
    CacheEntryList *         list;
    CacheEntry *             entry;
    CacheEntry *             prev;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), OT_NOOP);

    SuccessOrExit(Tlv::FindTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));
    SuccessOrExit(Tlv::FindTlv(aMessage, ThreadTlv::kMeshLocalEid, &meshLocalIid, sizeof(meshLocalIid)));
    SuccessOrExit(Tlv::FindUint16Tlv(aMessage, ThreadTlv::kRloc16, rloc16));

    switch (Tlv::FindUint32Tlv(aMessage, ThreadTlv::kLastTransactionTime, lastTransactionTime))
    {
    case OT_ERROR_NONE:
        break;
    case OT_ERROR_NOT_FOUND:
        lastTransactionTime = 0;
        break;
    default:
        ExitNow();
    }

    otLogInfoArp("Received address notification from 0x%04x for %s to 0x%04x",
                 aMessageInfo.GetPeerAddr().GetIid().GetLocator(), target.ToString().AsCString(), rloc16);

    entry = FindCacheEntry(target, list, prev);
    VerifyOrExit(entry != nullptr, OT_NOOP);

    if (list == &mCachedList)
    {
        if (entry->IsLastTransactionTimeValid())
        {
            // Receiving multiple Address Notification for an EID from
            // different mesh-local IIDs indicates address is in use
            // by more than one device. Try to resolve the duplicate
            // address by sending an Address Error message.

            VerifyOrExit(entry->GetMeshLocalIid() == meshLocalIid, SendAddressError(target, meshLocalIid, nullptr));

            VerifyOrExit(lastTransactionTime < entry->GetLastTransactionTime(), OT_NOOP);
        }
    }

    entry->SetRloc16(rloc16);
    entry->SetMeshLocalIid(meshLocalIid);
    entry->SetLastTransactionTime(lastTransactionTime);

    list->PopAfter(prev);
    mCachedList.Push(*entry);

    LogCacheEntryChange(kEntryUpdated, kReasonReceivedNotification, *entry);

    if (Get<Tmf::TmfAgent>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
    {
        otLogInfoArp("Sending address notification acknowledgment");
    }

    Get<MeshForwarder>().HandleResolved(target, OT_ERROR_NONE);

exit:
    return;
}

void AddressResolver::SendAddressError(const Ip6::Address &            aTarget,
                                       const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                       const Ip6::Address *            aDestination)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->Init(aDestination == nullptr ? Coap::kTypeNonConfirmable : Coap::kTypeConfirmable, Coap::kCodePost);
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kAddressError));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &aTarget, sizeof(aTarget)));
    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, &aMeshLocalIid, sizeof(aMeshLocalIid)));

    if (aDestination == nullptr)
    {
        messageInfo.GetPeerAddr().SetToRealmLocalAllRoutersMulticast();
    }
    else
    {
        messageInfo.SetPeerAddr(*aDestination);
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address error for target %s", aTarget.ToString().AsCString());

exit:

    if (error != OT_ERROR_NONE)
    {
        FreeMessage(message);
        otLogInfoArp("Failed to send address error: %s", otThreadErrorToString(error));
    }
}

void AddressResolver::HandleAddressError(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressError(*static_cast<Coap::Message *>(aMessage),
                                                                 *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressError(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                  error = OT_ERROR_NONE;
    Ip6::Address             target;
    Ip6::InterfaceIdentifier meshLocalIid;
    Mac::ExtAddress          extAddr;
    Ip6::Address             destination;

    VerifyOrExit(aMessage.IsPostRequest(), error = OT_ERROR_DROP);

    otLogInfoArp("Received address error notification");

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        if (Get<Tmf::TmfAgent>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoArp("Sent address error notification acknowledgment");
        }
    }

    SuccessOrExit(error = Tlv::FindTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, ThreadTlv::kMeshLocalEid, &meshLocalIid, sizeof(meshLocalIid)));

    for (const Ip6::NetifUnicastAddress *address = Get<ThreadNetif>().GetUnicastAddresses(); address;
         address                                 = address->GetNext())
    {
        if (address->GetAddress() == target && Get<Mle::MleRouter>().GetMeshLocal64().GetIid() != meshLocalIid)
        {
            // Target EID matches address and Mesh Local EID differs
            Get<ThreadNetif>().RemoveUnicastAddress(*address);
            ExitNow();
        }
    }

    meshLocalIid.ConvertToExtAddress(extAddr);

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsFullThreadDevice())
        {
            continue;
        }

        if (child.GetExtAddress() != extAddr)
        {
            // Mesh Local EID differs, so check whether Target EID
            // matches a child address and if so remove it.

            if (child.RemoveIp6Address(target) == OT_ERROR_NONE)
            {
                SuccessOrExit(error = Get<Mle::Mle>().GetLocatorAddress(destination, child.GetRloc16()));

                SendAddressError(target, meshLocalIid, &destination);
                ExitNow();
            }
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnArp("Error while processing address error notification: %s", otThreadErrorToString(error));
    }
}

void AddressResolver::HandleAddressQuery(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressQuery(*static_cast<Coap::Message *>(aMessage),
                                                                 *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressQuery(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::Address target;
    uint32_t     lastTransactionTime;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), OT_NOOP);

    SuccessOrExit(Tlv::FindTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));

    otLogInfoArp("Received address query from 0x%04x for target %s", aMessageInfo.GetPeerAddr().GetIid().GetLocator(),
                 target.ToString().AsCString());

    if (Get<ThreadNetif>().HasUnicastAddress(target))
    {
        SendAddressQueryResponse(target, Get<Mle::MleRouter>().GetMeshLocal64().GetIid(), nullptr,
                                 aMessageInfo.GetPeerAddr());
        ExitNow();
    }

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsFullThreadDevice() || child.GetLinkFailures() >= Mle::kFailedChildTransmissions)
        {
            continue;
        }

        if (child.HasIp6Address(target))
        {
            lastTransactionTime = Time::MsecToSec(TimerMilli::GetNow() - child.GetLastHeard());
            SendAddressQueryResponse(target, child.GetMeshLocalIid(), &lastTransactionTime, aMessageInfo.GetPeerAddr());
            ExitNow();
        }
    }

exit:
    return;
}

void AddressResolver::SendAddressQueryResponse(const Ip6::Address &            aTarget,
                                               const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                               const uint32_t *                aLastTransactionTime,
                                               const Ip6::Address &            aDestination)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewPriorityMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->InitAsConfirmablePost();
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kAddressNotify));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &aTarget, sizeof(aTarget)));
    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, &aMeshLocalIid, sizeof(aMeshLocalIid)));
    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, ThreadTlv::kRloc16, Get<Mle::MleRouter>().GetRloc16()));

    if (aLastTransactionTime != nullptr)
    {
        SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, ThreadTlv::kLastTransactionTime, *aLastTransactionTime));
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address notification for target %s", aTarget.ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
}

void AddressResolver::HandleTimeTick(void)
{
    bool        continueRxingTicks = false;
    CacheEntry *prev;
    CacheEntry *entry;

    for (entry = mSnoopedList.GetHead(); entry != nullptr; entry = entry->GetNext())
    {
        if (entry->IsTimeoutZero())
        {
            continue;
        }

        continueRxingTicks = true;
        entry->DecrementTimeout();

        if (entry->IsTimeoutZero())
        {
            entry->SetCanEvict(true);
        }
    }

    for (entry = mQueryRetryList.GetHead(); entry != nullptr; entry = entry->GetNext())
    {
        if (entry->IsTimeoutZero())
        {
            continue;
        }

        continueRxingTicks = true;
        entry->DecrementTimeout();
    }

    prev = nullptr;

    while ((entry = GetEntryAfter(prev, mQueryList)) != nullptr)
    {
        OT_ASSERT(!entry->IsTimeoutZero());

        continueRxingTicks = true;
        entry->DecrementTimeout();

        if (entry->IsTimeoutZero())
        {
            uint16_t retryDelay = entry->GetRetryDelay();

            entry->SetTimeout(retryDelay);

            retryDelay <<= 1;

            if (retryDelay > kAddressQueryMaxRetryDelay)
            {
                retryDelay = kAddressQueryMaxRetryDelay;
            }

            entry->SetRetryDelay(retryDelay);
            entry->SetCanEvict(true);

            // Move the entry from `mQueryList` to `mQueryRetryList`
            mQueryList.PopAfter(prev);
            mQueryRetryList.Push(*entry);

            otLogInfoArp("Timed out waiting for address notification for %s, retry: %d",
                         entry->GetTarget().ToString().AsCString(), entry->GetTimeout());

            Get<MeshForwarder>().HandleResolved(entry->GetTarget(), OT_ERROR_DROP);

            // When the entry is removed from `mQueryList`
            // we keep the `prev` pointer same as before.
        }
        else
        {
            prev = entry;
        }
    }

    if (!continueRxingTicks)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kAddressResolver);
    }
}

void AddressResolver::HandleIcmpReceive(void *               aContext,
                                        otMessage *          aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        const otIcmp6Header *aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    static_cast<AddressResolver *>(aContext)->HandleIcmpReceive(*static_cast<Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                *static_cast<const Ip6::Icmp::Header *>(aIcmpHeader));
}

void AddressResolver::HandleIcmpReceive(Message &                aMessage,
                                        const Ip6::MessageInfo & aMessageInfo,
                                        const Ip6::Icmp::Header &aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Ip6::Header ip6Header;

    VerifyOrExit(aIcmpHeader.GetType() == Ip6::Icmp::Header::kTypeDstUnreach, OT_NOOP);
    VerifyOrExit(aIcmpHeader.GetCode() == Ip6::Icmp::Header::kCodeDstUnreachNoRoute, OT_NOOP);
    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(ip6Header), &ip6Header) == sizeof(ip6Header), OT_NOOP);

    Remove(ip6Header.GetDestination(), kReasonReceivedIcmpDstUnreachNoRoute);

exit:
    return;
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_ARP == 1)

void AddressResolver::LogCacheEntryChange(EntryChange       aChange,
                                          Reason            aReason,
                                          const CacheEntry &aEntry,
                                          CacheEntryList *  aList)
{
    const char *change = "";
    const char *reason = "";

    switch (aChange)
    {
    case kEntryAdded:
        change = "added";
        break;
    case kEntryUpdated:
        change = "updated";
        break;
    case kEntryRemoved:
        change = "removed";
        break;
    }

    switch (aReason)
    {
    case kReasonQueryRequest:
        reason = "query request";
        break;
    case kReasonSnoop:
        reason = "snoop";
        break;
    case kReasonReceivedNotification:
        reason = "rx notification";
        break;
    case kReasonRemovingRouterId:
        reason = "removing router id";
        break;
    case kReasonRemovingRloc16:
        reason = "removing rloc16";
        break;
    case kReasonReceivedIcmpDstUnreachNoRoute:
        reason = "rx icmp no route";
        break;
    case kReasonEvictingForNewEntry:
        reason = "evicting for new entry";
        break;
    case kReasonRemovingEid:
        reason = "removing eid";
        break;
    }

    otLogNoteArp("Cache entry %s: %s, 0x%04x%s%s - %s", change, aEntry.GetTarget().ToString().AsCString(),
                 aEntry.GetRloc16(), (aList == nullptr) ? "" : ", list:", ListToString(aList), reason);
}

const char *AddressResolver::ListToString(const CacheEntryList *aList) const
{
    const char *str = "";

    VerifyOrExit(aList != &mCachedList, str = "cached");
    VerifyOrExit(aList != &mSnoopedList, str = "snooped");
    VerifyOrExit(aList != &mQueryList, str = "query");
    VerifyOrExit(aList != &mQueryRetryList, str = "query-retry");

exit:
    return str;
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_ARP == 1)

void AddressResolver::LogCacheEntryChange(EntryChange, Reason, const CacheEntry &, CacheEntryList *)
{
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_ARP == 1)

// LCOV_EXCL_STOP

//---------------------------------------------------------------------------------------------------------------------
// AddressResolver::CacheEntry

void AddressResolver::CacheEntry::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);
    mNextIndex = kNoNextIndex;
}

AddressResolver::CacheEntry *AddressResolver::CacheEntry::GetNext(void)
{
    return (mNextIndex == kNoNextIndex) ? nullptr : &Get<AddressResolver>().GetCacheEntryPool().GetEntryAt(mNextIndex);
}

const AddressResolver::CacheEntry *AddressResolver::CacheEntry::GetNext(void) const
{
    return (mNextIndex == kNoNextIndex) ? nullptr : &Get<AddressResolver>().GetCacheEntryPool().GetEntryAt(mNextIndex);
}

void AddressResolver::CacheEntry::SetNext(CacheEntry *aEntry)
{
    VerifyOrExit(aEntry != nullptr, mNextIndex = kNoNextIndex);
    mNextIndex = Get<AddressResolver>().GetCacheEntryPool().GetIndexOf(*aEntry);

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_FTD
