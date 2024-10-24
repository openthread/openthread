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

#include "address_resolver.hpp"

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("AddrResolver");

AddressResolver::AddressResolver(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_FTD
    , mCacheEntryPool(aInstance)
    , mIcmpHandler(&AddressResolver::HandleIcmpReceive, this)
#endif
{
#if OPENTHREAD_FTD
    IgnoreError(Get<Ip6::Icmp>().RegisterHandler(mIcmpHandler));
#endif
}

#if OPENTHREAD_FTD

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
                Get<MeshForwarder>().HandleResolved(entry->GetTarget(), kErrorDrop);
            }

            mCacheEntryPool.Free(*entry);
        }
    }
}

Error AddressResolver::GetNextCacheEntry(EntryInfo &aInfo, Iterator &aIterator) const
{
    Error                 error = kErrorNone;
    const CacheEntryList *list  = aIterator.GetList();
    const CacheEntry     *entry = aIterator.GetEntry();

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
            ExitNow(error = kErrorNotFound);
        }

        entry = list->GetHead();
    }

    // Update the iterator then populate the `aInfo`.

    aIterator.SetEntry(entry->GetNext());
    aIterator.SetList(list);

    aInfo.Clear();
    aInfo.mTarget = entry->GetTarget();
    aInfo.mRloc16 = entry->GetRloc16();

    if (list == &mCachedList)
    {
        aInfo.mState          = MapEnum(EntryInfo::kStateCached);
        aInfo.mCanEvict       = true;
        aInfo.mValidLastTrans = entry->IsLastTransactionTimeValid();

        VerifyOrExit(entry->IsLastTransactionTimeValid());

        aInfo.mLastTransTime = entry->GetLastTransactionTime();
        AsCoreType(&aInfo.mMeshLocalEid).SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        AsCoreType(&aInfo.mMeshLocalEid).SetIid(entry->GetMeshLocalIid());

        ExitNow();
    }

    if (list == &mSnoopedList)
    {
        aInfo.mState = MapEnum(EntryInfo::kStateSnooped);
    }
    else if (list == &mQueryList)
    {
        aInfo.mState = MapEnum(EntryInfo::kStateQuery);
    }
    else
    {
        aInfo.mState    = MapEnum(EntryInfo::kStateRetryQuery);
        aInfo.mRampDown = entry->IsInRampDown();
    }

    aInfo.mCanEvict   = entry->CanEvict();
    aInfo.mTimeout    = entry->GetTimeout();
    aInfo.mRetryDelay = entry->GetRetryDelay();

exit:
    return error;
}

void AddressResolver::RemoveEntriesForRouterId(uint8_t aRouterId)
{
    Remove(Mle::Rloc16FromRouterId(aRouterId), /* aMatchRouterId */ true);
}

void AddressResolver::RemoveEntriesForRloc16(uint16_t aRloc16) { Remove(aRloc16, /* aMatchRouterId */ false); }

AddressResolver::CacheEntry *AddressResolver::GetEntryAfter(CacheEntry *aPrev, CacheEntryList &aList)
{
    return (aPrev == nullptr) ? aList.GetHead() : aPrev->GetNext();
}

void AddressResolver::Remove(uint16_t aRloc16, bool aMatchRouterId)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList};

    for (CacheEntryList *list : lists)
    {
        CacheEntry *prev = nullptr;
        CacheEntry *entry;

        while ((entry = GetEntryAfter(prev, *list)) != nullptr)
        {
            if ((aMatchRouterId && Mle::RouterIdMatch(entry->GetRloc16(), aRloc16)) ||
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
                                                             CacheEntryList    *&aList,
                                                             CacheEntry        *&aPrevEntry)
{
    CacheEntry     *entry   = nullptr;
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList, &mQueryList, &mQueryRetryList};

    for (CacheEntryList *list : lists)
    {
        aList = list;
        entry = aList->FindMatching(aEid, aPrevEntry);
        VerifyOrExit(entry == nullptr);
    }

exit:
    return entry;
}

void AddressResolver::RemoveEntryForAddress(const Ip6::Address &aEid) { Remove(aEid, kReasonRemovingEid); }

void AddressResolver::Remove(const Ip6::Address &aEid, Reason aReason)
{
    CacheEntry     *entry;
    CacheEntry     *prev;
    CacheEntryList *list;

    entry = FindCacheEntry(aEid, list, prev);
    VerifyOrExit(entry != nullptr);

    RemoveCacheEntry(*entry, *list, prev, aReason);
    mCacheEntryPool.Free(*entry);

exit:
    return;
}

void AddressResolver::ReplaceEntriesForRloc16(uint16_t aOldRloc16, uint16_t aNewRloc16)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList};

    for (CacheEntryList *list : lists)
    {
        for (CacheEntry &entry : *list)
        {
            if (entry.GetRloc16() == aOldRloc16)
            {
                entry.SetRloc16(aNewRloc16);
            }
        }
    }
}

AddressResolver::CacheEntry *AddressResolver::NewCacheEntry(bool aSnoopedEntry)
{
    CacheEntry     *newEntry  = nullptr;
    CacheEntry     *prevEntry = nullptr;
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
    VerifyOrExit(newEntry == nullptr);

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

            VerifyOrExit(numNonEvictable < kMaxNonEvictableSnoopedEntries);
        }
    }

exit:
    return newEntry;
}

void AddressResolver::RemoveCacheEntry(CacheEntry     &aEntry,
                                       CacheEntryList &aList,
                                       CacheEntry     *aPrevEntry,
                                       Reason          aReason)
{
    aList.PopAfter(aPrevEntry);

    if (&aList == &mQueryList)
    {
        Get<MeshForwarder>().HandleResolved(aEntry.GetTarget(), kErrorDrop);
    }

    LogCacheEntryChange(kEntryRemoved, aReason, aEntry, &aList);
}

Error AddressResolver::UpdateCacheEntry(const Ip6::Address &aEid, uint16_t aRloc16)
{
    // This method updates an existing cache entry for the EID (if any).
    // Returns `kErrorNone` if entry is found and successfully updated,
    // `kErrorNotFound` if no matching entry.

    Error           error = kErrorNone;
    CacheEntryList *list;
    CacheEntry     *entry;
    CacheEntry     *prev;

    entry = FindCacheEntry(aEid, list, prev);
    VerifyOrExit(entry != nullptr, error = kErrorNotFound);

    if ((list == &mCachedList) || (list == &mSnoopedList))
    {
        VerifyOrExit(entry->GetRloc16() != aRloc16);
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

        Get<MeshForwarder>().HandleResolved(aEid, kErrorNone);
    }

    LogCacheEntryChange(kEntryUpdated, kReasonSnoop, *entry);

exit:
    return error;
}

void AddressResolver::UpdateSnoopedCacheEntry(const Ip6::Address &aEid, uint16_t aRloc16, uint16_t aDest)
{
    uint16_t    numNonEvictable = 0;
    CacheEntry *entry;

    VerifyOrExit(Get<Mle::MleRouter>().IsFullThreadDevice());

#if OPENTHREAD_CONFIG_TMF_ALLOW_ADDRESS_RESOLUTION_USING_NET_DATA_SERVICES
    {
        uint16_t rloc16;

        VerifyOrExit(ResolveUsingNetDataServices(aEid, rloc16) != kErrorNone);
    }
#endif

    VerifyOrExit(UpdateCacheEntry(aEid, aRloc16) != kErrorNone);

    // Skip if the `aRloc16` (i.e., the source of the snooped message)
    // is this device or an MTD (minimal) child of the device itself.

    VerifyOrExit(!Get<Mle::Mle>().HasRloc16(aRloc16) && !Get<ChildTable>().HasMinimalChild(aRloc16));

    // Ensure that the destination of the snooped message is this device
    // or a minimal child of this device.

    VerifyOrExit(Get<Mle::Mle>().HasRloc16(aDest) || Get<ChildTable>().HasMinimalChild(aDest));

    entry = NewCacheEntry(/* aSnoopedEntry */ true);
    VerifyOrExit(entry != nullptr);

    for (CacheEntry &snooped : mSnoopedList)
    {
        if (!snooped.CanEvict())
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

    for (CacheEntry &entry : mQueryList)
    {
        IgnoreError(SendAddressQuery(entry.GetTarget()));

        entry.SetTimeout(kAddressQueryTimeout);
        entry.SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry.SetCanEvict(false);
    }
}

uint16_t AddressResolver::LookUp(const Ip6::Address &aEid)
{
    uint16_t rloc16 = Mle::kInvalidRloc16;

    IgnoreError(Resolve(aEid, rloc16, /* aAllowAddressQuery */ false));
    return rloc16;
}

Error AddressResolver::Resolve(const Ip6::Address &aEid, uint16_t &aRloc16, bool aAllowAddressQuery)
{
    Error           error = kErrorNone;
    CacheEntry     *entry;
    CacheEntry     *prev = nullptr;
    CacheEntryList *list;

#if OPENTHREAD_CONFIG_TMF_ALLOW_ADDRESS_RESOLUTION_USING_NET_DATA_SERVICES
    VerifyOrExit(ResolveUsingNetDataServices(aEid, aRloc16) != kErrorNone);
#endif

    entry = FindCacheEntry(aEid, list, prev);

    if ((entry != nullptr) && ((list == &mCachedList) || (list == &mSnoopedList)))
    {
        bool isFresh;

        list->PopAfter(prev);

        // If the `entry->GetRloc16()` is unreachable (there is no
        // valid next hop towards it), it may be a stale entry. We
        // clear the entry to allow new address query to be sent for
        // it, unless the entry has been recently updated, i.e., we
        // have recently received an `AddressNotify` for it and its
        // `FreshnessTimeout` has not expired yet.
        //
        // The `FreshnessTimeout` check prevents repeated address
        // query transmissions when mesh routes are not yet
        // discovered (e.g., after initial attach) or if there is a
        // temporary link issue.

        isFresh = (list == &mCachedList) && !entry->IsFreshnessTimeoutZero();

        if (!isFresh && (Get<RouterTable>().GetNextHop(entry->GetRloc16()) == Mle::kInvalidRloc16))
        {
            mCacheEntryPool.Free(*entry);
            entry = nullptr;
        }

        if (entry != nullptr)
        {
            // Push the entry at the head of cached list.

            if (list == &mSnoopedList)
            {
                entry->MarkLastTransactionTimeAsInvalid();
            }

            mCachedList.Push(*entry);
            aRloc16 = entry->GetRloc16();
            ExitNow();
        }
    }

    if (entry == nullptr)
    {
        // If the entry is not present in any of the lists, try to
        // allocate a new entry and perform address query. We do not
        // allow first-time address query entries to be evicted till
        // timeout.

        VerifyOrExit(aAllowAddressQuery, error = kErrorNotFound);

        entry = NewCacheEntry(/* aSnoopedEntry */ false);
        VerifyOrExit(entry != nullptr, error = kErrorNoBufs);

        entry->SetTarget(aEid);
        entry->SetRloc16(Mle::kInvalidRloc16);
        entry->SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry->SetCanEvict(false);
        list = nullptr;
    }

    // Note that if `aAllowAddressQuery` is `false` then the `entry`
    // is definitely already in a list, i.e., we cannot not get here
    // with `aAllowAddressQuery` being `false` and `entry` being a
    // newly allocated one, due to the `VerifyOrExit` check that
    // `aAllowAddressQuery` is `true` before allocating a new cache
    // entry.
    VerifyOrExit(aAllowAddressQuery, error = kErrorNotFound);

    if (list == &mQueryList)
    {
        ExitNow(error = kErrorAddressQuery);
    }

    if (list == &mQueryRetryList)
    {
        // Allow an entry in query-retry mode to resend an Address
        // Query again only if it is in ramp down mode, i.e., the
        // retry delay timeout is expired.

        VerifyOrExit(entry->IsInRampDown(), error = kErrorDrop);
        mQueryRetryList.PopAfter(prev);
    }

    entry->SetTimeout(kAddressQueryTimeout);

    error = SendAddressQuery(aEid);
    VerifyOrExit(error == kErrorNone, mCacheEntryPool.Free(*entry));

    if (list == nullptr)
    {
        LogCacheEntryChange(kEntryAdded, kReasonQueryRequest, *entry);
    }

    mQueryList.Push(*entry);
    error = kErrorAddressQuery;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TMF_ALLOW_ADDRESS_RESOLUTION_USING_NET_DATA_SERVICES

Error AddressResolver::ResolveUsingNetDataServices(const Ip6::Address &aEid, uint16_t &aRloc16)
{
    // Tries to resolve `aEid` Network Data DNS/SRP Unicast address
    // service entries.  Returns `kErrorNone` and updates `aRloc16`
    // if successful, otherwise returns `kErrorNotFound`.

    Error                                   error = kErrorNotFound;
    NetworkData::Service::Manager::Iterator iterator;
    NetworkData::Service::DnsSrpUnicastInfo unicastInfo;
    NetworkData::Service::DnsSrpUnicastType type = NetworkData::Service::kAddrInServerData;

    VerifyOrExit(Get<Mle::Mle>().GetDeviceMode().GetNetworkDataType() == NetworkData::kFullSet);

    while (Get<NetworkData::Service::Manager>().GetNextDnsSrpUnicastInfo(iterator, type, unicastInfo) == kErrorNone)
    {
        if (aEid == unicastInfo.mSockAddr.GetAddress())
        {
            aRloc16 = unicastInfo.mRloc16;
            error   = kErrorNone;
            ExitNow();
        }
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_TMF_ALLOW_ADDRESS_RESOLUTION_USING_NET_DATA_SERVICES

Error AddressResolver::SendAddressQuery(const Ip6::Address &aEid)
{
    Error            error;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriAddressQuery);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aEid));

    messageInfo.SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s for %s", UriToString<kUriAddressQuery>(), aEid.ToString().AsCString());

exit:

    Get<TimeTicker>().RegisterReceiver(TimeTicker::kAddressResolver);
    FreeMessageOnError(message, error);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    if (Get<BackboneRouter::Local>().IsPrimary() && Get<BackboneRouter::Leader>().IsDomainUnicast(aEid))
    {
        uint16_t selfRloc16 = Get<Mle::MleRouter>().GetRloc16();

        LogInfo("Extending %s to %s for target %s, rloc16=%04x(self)", UriToString<kUriAddressQuery>(),
                UriToString<kUriBackboneQuery>(), aEid.ToString().AsCString(), selfRloc16);
        IgnoreError(Get<BackboneRouter::Manager>().SendBackboneQuery(aEid, selfRloc16));
    }
#endif

    return error;
}

template <>
void AddressResolver::HandleTmf<kUriAddressNotify>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::Address             target;
    Ip6::InterfaceIdentifier meshLocalIid;
    uint16_t                 rloc16;
    uint32_t                 lastTransactionTime;
    CacheEntryList          *list;
    CacheEntry              *entry;
    CacheEntry              *prev;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    SuccessOrExit(Tlv::Find<ThreadTargetTlv>(aMessage, target));
    SuccessOrExit(Tlv::Find<ThreadMeshLocalEidTlv>(aMessage, meshLocalIid));
    SuccessOrExit(Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16));

    switch (Tlv::Find<ThreadLastTransactionTimeTlv>(aMessage, lastTransactionTime))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        lastTransactionTime = 0;
        break;
    default:
        ExitNow();
    }

    LogInfo("Received %s from 0x%04x for %s to 0x%04x", UriToString<kUriAddressNotify>(),
            aMessageInfo.GetPeerAddr().GetIid().GetLocator(), target.ToString().AsCString(), rloc16);

    entry = FindCacheEntry(target, list, prev);
    VerifyOrExit(entry != nullptr);

    if (list == &mCachedList)
    {
        if (entry->IsLastTransactionTimeValid())
        {
            // Receiving multiple Address Notification for an EID from
            // different mesh-local IIDs indicates address is in use
            // by more than one device. Try to resolve the duplicate
            // address by sending an Address Error message.

            VerifyOrExit(entry->GetMeshLocalIid() == meshLocalIid, SendAddressError(target, meshLocalIid, nullptr));

            VerifyOrExit(lastTransactionTime < entry->GetLastTransactionTime());
        }
    }

    entry->SetRloc16(rloc16);
    entry->SetMeshLocalIid(meshLocalIid);
    entry->SetLastTransactionTime(lastTransactionTime);
    entry->ResetFreshnessTimeout();
    Get<TimeTicker>().RegisterReceiver(TimeTicker::kAddressResolver);

    list->PopAfter(prev);
    mCachedList.Push(*entry);

    LogCacheEntryChange(kEntryUpdated, kReasonReceivedNotification, *entry);

    if (Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo) == kErrorNone)
    {
        LogInfo("Sent %s ack", UriToString<kUriAddressNotify>());
    }

    Get<MeshForwarder>().HandleResolved(target, kErrorNone);

exit:
    return;
}

void AddressResolver::SendAddressError(const Ip6::Address             &aTarget,
                                       const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                       const Ip6::Address             *aDestination)
{
    Error            error;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit((message = Get<Tmf::Agent>().NewMessage()) != nullptr, error = kErrorNoBufs);

    message->Init(aDestination == nullptr ? Coap::kTypeNonConfirmable : Coap::kTypeConfirmable, Coap::kCodePost);
    SuccessOrExit(error = message->AppendUriPathOptions(PathForUri(kUriAddressError)));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aTarget));
    SuccessOrExit(error = Tlv::Append<ThreadMeshLocalEidTlv>(*message, aMeshLocalIid));

    if (aDestination == nullptr)
    {
        messageInfo.SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast();
    }
    else
    {
        messageInfo.SetSockAddrToRlocPeerAddrTo(*aDestination);
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s for target %s", UriToString<kUriAddressError>(), aTarget.ToString().AsCString());

exit:

    if (error != kErrorNone)
    {
        FreeMessage(message);
        LogInfo("Failed to send %s: %s", UriToString<kUriAddressError>(), ErrorToString(error));
    }
}

#endif // OPENTHREAD_FTD

template <>
void AddressResolver::HandleTmf<kUriAddressError>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                    error = kErrorNone;
    Ip6::Address             target;
    Ip6::InterfaceIdentifier meshLocalIid;
#if OPENTHREAD_FTD
    Mac::ExtAddress extAddr;
    Ip6::Address    destination;
#endif

    VerifyOrExit(aMessage.IsPostRequest(), error = kErrorDrop);

    LogInfo("Received %s", UriToString<kUriAddressError>());

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        if (Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo) == kErrorNone)
        {
            LogInfo("Sent %s ack", UriToString<kUriAddressError>());
        }
    }

    SuccessOrExit(error = Tlv::Find<ThreadTargetTlv>(aMessage, target));
    SuccessOrExit(error = Tlv::Find<ThreadMeshLocalEidTlv>(aMessage, meshLocalIid));

    for (Ip6::Netif::UnicastAddress &address : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (address.GetAddress() == target && Get<Mle::MleRouter>().GetMeshLocalEid().GetIid() != meshLocalIid)
        {
            // Target EID matches address and Mesh Local EID differs
#if OPENTHREAD_CONFIG_DUA_ENABLE
            if (Get<BackboneRouter::Leader>().IsDomainUnicast(address.GetAddress()))
            {
                Get<DuaManager>().NotifyDuplicateDomainUnicastAddress();
            }
            else
#endif
            {
                Get<ThreadNetif>().RemoveUnicastAddress(address);
            }

            ExitNow();
        }
    }

#if OPENTHREAD_FTD
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

            if (child.RemoveIp6Address(target) == kErrorNone)
            {
                destination.SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), child.GetRloc16());

                SendAddressError(target, meshLocalIid, &destination);
                ExitNow();
            }
        }
    }
#endif // OPENTHREAD_FTD

exit:

    if (error != kErrorNone)
    {
        LogWarn("Error %s when processing %s", ErrorToString(error), UriToString<kUriAddressError>());
    }
}

#if OPENTHREAD_FTD

template <>
void AddressResolver::HandleTmf<kUriAddressQuery>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::Address target;
    uint32_t     lastTransactionTime;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest());

    SuccessOrExit(Tlv::Find<ThreadTargetTlv>(aMessage, target));

    LogInfo("Received %s from 0x%04x for target %s", UriToString<kUriAddressQuery>(),
            aMessageInfo.GetPeerAddr().GetIid().GetLocator(), target.ToString().AsCString());

    if (Get<ThreadNetif>().HasUnicastAddress(target))
    {
        SendAddressQueryResponse(target, Get<Mle::MleRouter>().GetMeshLocalEid().GetIid(), nullptr,
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

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    if (Get<BackboneRouter::Local>().IsPrimary() && Get<BackboneRouter::Leader>().IsDomainUnicast(target))
    {
        uint16_t srcRloc16 = aMessageInfo.GetPeerAddr().GetIid().GetLocator();

        LogInfo("Extending %s to %s for target %s rloc16=%04x", UriToString<kUriAddressQuery>(),
                UriToString<kUriBackboneQuery>(), target.ToString().AsCString(), srcRloc16);
        IgnoreError(Get<BackboneRouter::Manager>().SendBackboneQuery(target, srcRloc16));
    }
#endif

exit:
    return;
}

void AddressResolver::SendAddressQueryResponse(const Ip6::Address             &aTarget,
                                               const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                               const uint32_t                 *aLastTransactionTime,
                                               const Ip6::Address             &aDestination)
{
    Error            error;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriAddressNotify);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aTarget));
    SuccessOrExit(error = Tlv::Append<ThreadMeshLocalEidTlv>(*message, aMeshLocalIid));
    SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, Get<Mle::MleRouter>().GetRloc16()));

    if (aLastTransactionTime != nullptr)
    {
        SuccessOrExit(error = Tlv::Append<ThreadLastTransactionTimeTlv>(*message, *aLastTransactionTime));
    }

    messageInfo.SetSockAddrToRlocPeerAddrTo(aDestination);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent %s for target %s", UriToString<kUriAddressNotify>(), aTarget.ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
}

void AddressResolver::HandleTimeTick(void)
{
    bool continueRxingTicks = false;

    for (CacheEntry &entry : mCachedList)
    {
        if (!entry.IsFreshnessTimeoutZero())
        {
            entry.DecrementFreshnessTimeout();
            continueRxingTicks = true;
        }
    }

    for (CacheEntry &entry : mSnoopedList)
    {
        if (entry.IsTimeoutZero())
        {
            continue;
        }

        continueRxingTicks = true;
        entry.DecrementTimeout();

        if (entry.IsTimeoutZero())
        {
            entry.SetCanEvict(true);
        }
    }

    for (CacheEntry &entry : mQueryRetryList)
    {
        if (entry.IsTimeoutZero())
        {
            continue;
        }

        continueRxingTicks = true;
        entry.DecrementTimeout();

        if (entry.IsTimeoutZero())
        {
            if (!entry.IsInRampDown())
            {
                entry.SetRampDown(true);
                entry.SetTimeout(kAddressQueryMaxRetryDelay);

                LogInfo("Starting ramp down of %s retry-delay:%u", entry.GetTarget().ToString().AsCString(),
                        entry.GetTimeout());
            }
            else
            {
                uint16_t retryDelay = entry.GetRetryDelay();

                retryDelay >>= 1;
                retryDelay = Max(retryDelay, kAddressQueryInitialRetryDelay);

                if (retryDelay != entry.GetRetryDelay())
                {
                    entry.SetRetryDelay(retryDelay);
                    entry.SetTimeout(kAddressQueryMaxRetryDelay);

                    LogInfo("Ramping down %s retry-delay:%u", entry.GetTarget().ToString().AsCString(), retryDelay);
                }
            }
        }
    }

    {
        CacheEntry *prev = nullptr;
        CacheEntry *entry;

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
                retryDelay = Min(retryDelay, kAddressQueryMaxRetryDelay);

                entry->SetRetryDelay(retryDelay);
                entry->SetCanEvict(true);
                entry->SetRampDown(false);

                // Move the entry from `mQueryList` to `mQueryRetryList`
                mQueryList.PopAfter(prev);
                mQueryRetryList.Push(*entry);

                LogInfo("Timed out waiting for %s for %s, retry: %d", UriToString<kUriAddressNotify>(),
                        entry->GetTarget().ToString().AsCString(), entry->GetTimeout());

                Get<MeshForwarder>().HandleResolved(entry->GetTarget(), kErrorDrop);

                // When the entry is removed from `mQueryList`
                // we keep the `prev` pointer same as before.
            }
            else
            {
                prev = entry;
            }
        }
    }

    if (!continueRxingTicks)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kAddressResolver);
    }
}

void AddressResolver::HandleIcmpReceive(void                *aContext,
                                        otMessage           *aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        const otIcmp6Header *aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    static_cast<AddressResolver *>(aContext)->HandleIcmpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo),
                                                                AsCoreType(aIcmpHeader));
}

void AddressResolver::HandleIcmpReceive(Message                 &aMessage,
                                        const Ip6::MessageInfo  &aMessageInfo,
                                        const Ip6::Icmp::Header &aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Ip6::Header ip6Header;

    VerifyOrExit(aIcmpHeader.GetType() == Ip6::Icmp::Header::kTypeDstUnreach);
    VerifyOrExit(aIcmpHeader.GetCode() == Ip6::Icmp::Header::kCodeDstUnreachNoRoute);
    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), ip6Header));

    Remove(ip6Header.GetDestination(), kReasonReceivedIcmpDstUnreachNoRoute);

exit:
    return;
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void AddressResolver::LogCacheEntryChange(EntryChange       aChange,
                                          Reason            aReason,
                                          const CacheEntry &aEntry,
                                          CacheEntryList   *aList)
{
    static const char *const kChangeStrings[] = {
        "added",   // (0) kEntryAdded
        "updated", // (1) kEntryUpdated
        "removed", // (2) kEntryRemoved
    };

    static const char *const kReasonStrings[] = {
        "query request",          // (0) kReasonQueryRequest
        "snoop",                  // (1) kReasonSnoop
        "rx notification",        // (2) kReasonReceivedNotification
        "removing router id",     // (3) kReasonRemovingRouterId
        "removing rloc16",        // (4) kReasonRemovingRloc16
        "rx icmp no route",       // (5) kReasonReceivedIcmpDstUnreachNoRoute
        "evicting for new entry", // (6) kReasonEvictingForNewEntry
        "removing eid",           // (7) kReasonRemovingEid
    };

    struct ChangeEnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kEntryAdded);
        ValidateNextEnum(kEntryUpdated);
        ValidateNextEnum(kEntryRemoved);
    };

    struct ReasonEnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kReasonQueryRequest);
        ValidateNextEnum(kReasonSnoop);
        ValidateNextEnum(kReasonReceivedNotification);
        ValidateNextEnum(kReasonRemovingRouterId);
        ValidateNextEnum(kReasonRemovingRloc16);
        ValidateNextEnum(kReasonReceivedIcmpDstUnreachNoRoute);
        ValidateNextEnum(kReasonEvictingForNewEntry);
        ValidateNextEnum(kReasonRemovingEid);
    };

    LogInfo("Cache entry %s: %s, 0x%04x%s%s - %s", kChangeStrings[aChange], aEntry.GetTarget().ToString().AsCString(),
            aEntry.GetRloc16(), (aList == nullptr) ? "" : ", list:", ListToString(aList), kReasonStrings[aReason]);
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

#else // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void AddressResolver::LogCacheEntryChange(EntryChange, Reason, const CacheEntry &, CacheEntryList *) {}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

//---------------------------------------------------------------------------------------------------------------------
// AddressResolver::CacheEntry

void AddressResolver::CacheEntry::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);
    mNextIndex        = kNoNextIndex;
    mFreshnessTimeout = 0;
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

#endif // OPENTHREAD_FTD

} // namespace ot
