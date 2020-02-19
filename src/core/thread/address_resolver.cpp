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
#include "mac/mac_types.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

AddressResolver::AddressResolver(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mAddressError(OT_URI_PATH_ADDRESS_ERROR, &AddressResolver::HandleAddressError, this)
    , mAddressQuery(OT_URI_PATH_ADDRESS_QUERY, &AddressResolver::HandleAddressQuery, this)
    , mAddressNotification(OT_URI_PATH_ADDRESS_NOTIFY, &AddressResolver::HandleAddressNotification, this)
    , mCachedList()
    , mSnoopedList()
    , mQueryList()
    , mQueryRetryList()
    , mUnusedList()
    , mIcmpHandler(&AddressResolver::HandleIcmpReceive, this)
    , mTimer(aInstance, &AddressResolver::HandleTimer, this)
{
    for (CacheEntry *entry = &mCacheEntries[0]; entry < OT_ARRAY_END(mCacheEntries); entry++)
    {
        entry->Init(GetInstance());
        mUnusedList.Push(*entry);
    }

    Get<Coap::Coap>().AddResource(mAddressError);
    Get<Coap::Coap>().AddResource(mAddressQuery);
    Get<Coap::Coap>().AddResource(mAddressNotification);

    Get<Ip6::Icmp>().RegisterHandler(mIcmpHandler);
}

void AddressResolver::Clear(void)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList, &mQueryList, &mQueryRetryList};

    for (uint8_t index = 0; index < OT_ARRAY_LENGTH(lists); index++)
    {
        CacheEntryList *list = lists[index];
        CacheEntry *    entry;

        while ((entry = list->Pop()) != NULL)
        {
            if (list == &mQueryList)
            {
                Get<MeshForwarder>().HandleResolved(entry->GetTarget(), OT_ERROR_DROP);
            }

            mUnusedList.Push(*entry);
        }
    }
}

otError AddressResolver::GetEntry(uint8_t aIndex, otEidCacheEntry &aEntry) const
{
    otError               error   = OT_ERROR_NONE;
    uint8_t               age     = aIndex;
    const CacheEntryList *lists[] = {&mCachedList, &mSnoopedList};
    const CacheEntry *    entry;

    for (uint8_t index = 0; index < OT_ARRAY_LENGTH(lists); index++)
    {
        for (entry = lists[index]->GetHead(); (aIndex != 0) && (entry != NULL); aIndex--, entry = entry->GetNext())
            ;

        if (entry != NULL)
        {
            break;
        }
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

    aEntry.mTarget = entry->GetTarget();
    aEntry.mRloc16 = entry->GetRloc16();
    aEntry.mAge    = age;
    aEntry.mValid  = true;

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

void AddressResolver::Remove(Mac::ShortAddress aRloc16, bool aMatchRouterId)
{
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList};

    for (uint8_t index = 0; index < OT_ARRAY_LENGTH(lists); index++)
    {
        CacheEntryList *list = lists[index];
        CacheEntry *    prev;
        CacheEntry *    entry;
        CacheEntry *    next;

        for (prev = NULL, entry = list->GetHead(); entry != NULL; prev = entry, entry = next)
        {
            // We keep track of the next entry in the list before
            // checking the entry's RLOC16 or Router Id since the
            // entry may be removed from the list if it matches.

            next = entry->GetNext();

            if ((aMatchRouterId && Mle::Mle::RouterIdMatch(entry->GetRloc16(), aRloc16)) ||
                (!aMatchRouterId && (entry->GetRloc16() == aRloc16)))
            {
                RemoveCacheEntry(*entry, *list, prev, aMatchRouterId ? kReasonRemovingRouterId : kReasonRemovingRloc16);
                mUnusedList.Push(*entry);
            }
        }
    }
}

AddressResolver::CacheEntry *AddressResolver::FindCacheEntryInList(CacheEntryList &    aList,
                                                                   const Ip6::Address &aEid,
                                                                   CacheEntry *&       aPrevEntry)
{
    CacheEntry *entry;
    CacheEntry *prev;

    for (prev = NULL, entry = aList.GetHead(); entry != NULL; prev = entry, entry = entry->GetNext())
    {
        if (entry->GetTarget() == aEid)
        {
            aPrevEntry = prev;
            break;
        }
    }

    return entry;
}

AddressResolver::CacheEntry *AddressResolver::FindCacheEntry(const Ip6::Address &aEid,
                                                             CacheEntryList *&   aList,
                                                             CacheEntry *&       aPrevEntry)
{
    CacheEntry *    entry   = NULL;
    CacheEntryList *lists[] = {&mCachedList, &mSnoopedList, &mQueryList, &mQueryRetryList};

    for (uint8_t index = 0; index < OT_ARRAY_LENGTH(lists); index++)
    {
        aList = lists[index];
        entry = FindCacheEntryInList(*aList, aEid, aPrevEntry);
        VerifyOrExit(entry == NULL);
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
    VerifyOrExit(entry != NULL);

    RemoveCacheEntry(*entry, *list, prev, aReason);
    mUnusedList.Push(*entry);

exit:
    return;
}

AddressResolver::CacheEntry *AddressResolver::NewCacheEntry(bool aSnoopedEntry)
{
    CacheEntry *    newEntry  = NULL;
    CacheEntry *    prevEntry = NULL;
    CacheEntryList *lists[]   = {&mSnoopedList, &mQueryRetryList, &mQueryList, &mCachedList};

    // The following order is used when trying to allocate a new cache
    // entry: First the unused list is checked, followed by the list
    // of snooped entries, then query-retry list (entries in delay
    // retry timeout wait due to a prior query failing to get a
    // response), then the query list (entries actively querying and
    // waiting for address notification response), and finally the
    // cached (in-use) list. Within each list the oldest entry is
    // reclaimed first (the list's tail). We also make sure the entry
    // can be evicted (e.g., first time query entries can not be
    // evicted till timeout).

    newEntry = mUnusedList.Pop();
    VerifyOrExit(newEntry == NULL);

    for (uint8_t index = 0; index < OT_ARRAY_LENGTH(lists); index++)
    {
        CacheEntryList *list = lists[index];
        CacheEntry *    prev;
        CacheEntry *    entry;
        uint16_t        numNonEvictable = 0;

        for (prev = NULL, entry = list->GetHead(); entry != NULL; prev = entry, entry = entry->GetNext())
        {
            if ((list != &mCachedList) && !entry->CanEvict())
            {
                numNonEvictable++;
                continue;
            }

            newEntry  = entry;
            prevEntry = prev;
        }

        if (newEntry != NULL)
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

void AddressResolver::RemoveCacheEntry(CacheEntry &    aEntry,
                                       CacheEntryList &aList,
                                       CacheEntry *    aPrevEntry,
                                       Reason          aReason)
{
    OT_UNUSED_VARIABLE(aReason);

    aList.PopAfter(aPrevEntry);

    if (&aList == &mQueryList)
    {
        Get<MeshForwarder>().HandleResolved(aEntry.GetTarget(), OT_ERROR_DROP);
    }

    otLogNoteArp("Cache entry removed: %s, 0x%04x (%s) - %s", aEntry.GetTarget().ToString().AsCString(),
                 aEntry.GetRloc16(), ListToString(aList), ReasonToString(aReason));
}

otError AddressResolver::UpdateCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    otError         error = OT_ERROR_NONE;
    CacheEntryList *list;
    CacheEntry *    entry;
    CacheEntry *    prev;

    entry = FindCacheEntry(aEid, list, prev);
    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);

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

        Get<MeshForwarder>().HandleResolved(aEid, OT_ERROR_NONE);
    }

    otLogNoteArp("Cache entry updated (snoop): %s, 0x%04x", aEid.ToString().AsCString(), aRloc16);

exit:
    return error;
}

otError AddressResolver::AddSnoopedCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    otError     error           = OT_ERROR_NONE;
    uint16_t    numNonEvictable = 0;
    CacheEntry *entry;

    entry = NewCacheEntry(/* aSnoopedEntry */ true);
    VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);

    for (CacheEntry *snooped = mSnoopedList.GetHead(); snooped != NULL; snooped = snooped->GetNext())
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

        if (!mTimer.IsRunning())
        {
            mTimer.Start(kStateUpdatePeriod);
        }
    }
    else
    {
        entry->SetCanEvict(true);
        entry->SetTimeout(0);
    }

    mSnoopedList.Push(*entry);

    otLogNoteArp("Cache entry added (snoop): %s, 0x%04x", aEid.ToString().AsCString(), aRloc16);

exit:
    return error;
}

void AddressResolver::RestartAddressQueries(void)
{
    CacheEntry *tail;

    // We move all entries from `mQueryRetryList` at the tail of
    // `mQueryList` and then (re)send Address Query for all entries in
    // the updated `mQueryList`.

    tail = mQueryList.GetTail();

    if (tail == NULL)
    {
        mQueryList.SetHead(mQueryRetryList.GetHead());
    }
    else
    {
        tail->SetNext(mQueryRetryList.GetHead());
    }

    mQueryRetryList.Clear();

    for (CacheEntry *entry = mQueryList.GetHead(); entry != NULL; entry = entry->GetNext())
    {
        SendAddressQuery(entry->GetTarget());

        entry->SetTimeout(kAddressQueryTimeout);
        entry->SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry->SetCanEvict(false);
    }
}

otError AddressResolver::Resolve(const Ip6::Address &aEid, uint16_t &aRloc16)
{
    otError         error = OT_ERROR_NONE;
    CacheEntry *    entry;
    CacheEntry *    prev;
    CacheEntryList *list;

    entry = FindCacheEntry(aEid, list, prev);

    if (entry == NULL)
    {
        // If the entry is not present in any of the lists, try to
        // allocate a new entry and perform address query. We do not
        // allow first-time address query entries to be evicted till
        // timeout.

        entry = NewCacheEntry(/* aSnoopedEntry */ false);
        VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);

        entry->SetTarget(aEid);
        entry->SetRloc16(Mac::kShortAddrInvalid);
        entry->SetRetryDelay(kAddressQueryInitialRetryDelay);
        entry->SetCanEvict(false);
        list = NULL;
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
    VerifyOrExit(error == OT_ERROR_NONE, mUnusedList.Push(*entry));

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

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_QUERY));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, aEid.mFields.m8, sizeof(aEid)));

    messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x0002);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address query for %s", aEid.ToString().AsCString());

exit:

    if (!mTimer.IsRunning())
    {
        mTimer.Start(kStateUpdatePeriod);
    }

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void AddressResolver::HandleAddressNotification(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressNotification(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressNotification(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Ip6::Address    target;
    uint8_t         meshLocalIid[Ip6::Address::kInterfaceIdentifierSize];
    uint16_t        rloc16;
    uint32_t        lastTransactionTime;
    CacheEntryList *list;
    CacheEntry *    entry;
    CacheEntry *    prev;

    VerifyOrExit(aMessage.IsConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(Tlv::ReadTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));
    SuccessOrExit(Tlv::ReadTlv(aMessage, ThreadTlv::kMeshLocalEid, meshLocalIid, sizeof(meshLocalIid)));
    SuccessOrExit(Tlv::ReadUint16Tlv(aMessage, ThreadTlv::kRloc16, rloc16));

    switch (Tlv::ReadUint32Tlv(aMessage, ThreadTlv::kLastTransactionTime, lastTransactionTime))
    {
    case OT_ERROR_NONE:
        break;
    case OT_ERROR_NOT_FOUND:
        lastTransactionTime = 0;
        break;
    default:
        ExitNow();
    }

    otLogInfoArp("Received address notification from 0x%04x for %s to 0x%04x", aMessageInfo.GetPeerAddr().GetLocator(),
                 target.ToString().AsCString(), rloc16);

    entry = FindCacheEntry(target, list, prev);
    VerifyOrExit(entry != NULL);

    if (list == &mCachedList)
    {
        if (entry->IsLastTransactionTimeValid())
        {
            // Receiving multiple Address Notification for an EID from
            // different mesh-local IIDs indicates address is in use
            // by more than one device. Try to resolve the duplicate
            // address by sending an Address Error message.

            VerifyOrExit(entry->HasMeshLocalIid(meshLocalIid), SendAddressError(target, meshLocalIid, NULL));

            VerifyOrExit(lastTransactionTime < entry->GetLastTransactionTime());
        }
    }

    entry->SetRloc16(rloc16);
    entry->SetMeshLocalIid(meshLocalIid);
    entry->SetLastTransactionTime(lastTransactionTime);

    list->PopAfter(prev);
    mCachedList.Push(*entry);

    otLogNoteArp("Cache entry updated (notification): %s, 0x%04x, lastTrans:%d", target.ToString().AsCString(), rloc16,
                 lastTransactionTime);

    if (Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
    {
        otLogInfoArp("Sending address notification acknowledgment");
    }

    Get<MeshForwarder>().HandleResolved(target, OT_ERROR_NONE);

exit:
    return;
}

otError AddressResolver::SendAddressError(const Ip6::Address &aTarget,
                                          const uint8_t *     aMeshLocalIid,
                                          const Ip6::Address *aDestination)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(aDestination == NULL ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_ERROR));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, aTarget.mFields.m8, sizeof(aTarget)));
    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, aMeshLocalIid,
                                         Ip6::Address::kInterfaceIdentifierSize));

    if (aDestination == NULL)
    {
        messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
        messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x0002);
    }
    else
    {
        messageInfo.SetPeerAddr(*aDestination);
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address error for target %s", aTarget.ToString().AsCString());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void AddressResolver::HandleAddressError(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressError(*static_cast<Coap::Message *>(aMessage),
                                                                 *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressError(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError         error = OT_ERROR_NONE;
    Ip6::Address    target;
    uint8_t         meshLocalIid[Ip6::Address::kInterfaceIdentifierSize];
    Mac::ExtAddress macAddr;
    Ip6::Address    destination;

    VerifyOrExit(aMessage.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoArp("Received address error notification");

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        if (Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoArp("Sent address error notification acknowledgment");
        }
    }

    SuccessOrExit(error = Tlv::ReadTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));
    SuccessOrExit(error = Tlv::ReadTlv(aMessage, ThreadTlv::kMeshLocalEid, meshLocalIid, sizeof(meshLocalIid)));

    for (const Ip6::NetifUnicastAddress *address = Get<ThreadNetif>().GetUnicastAddresses(); address;
         address                                 = address->GetNext())
    {
        if (address->GetAddress() == target &&
            memcmp(Get<Mle::MleRouter>().GetMeshLocal64().GetIid(), meshLocalIid, sizeof(meshLocalIid)))
        {
            // Target EID matches address and Mesh Local EID differs
            Get<ThreadNetif>().RemoveUnicastAddress(*address);
            ExitNow();
        }
    }

    macAddr.Set(meshLocalIid);
    macAddr.ToggleLocal();

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (child.IsFullThreadDevice())
        {
            continue;
        }

        if (child.GetExtAddress() != macAddr)
        {
            // Mesh Local EID differs, so check whether Target EID
            // matches a child address and if so remove it.

            if (child.RemoveIp6Address(target) == OT_ERROR_NONE)
            {
                destination.Clear();
                destination.mFields.m16[0] = HostSwap16(0xfe80);
                destination.SetIid(child.GetExtAddress());

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

    VerifyOrExit(aMessage.IsNonConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(Tlv::ReadTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));

    otLogInfoArp("Received address query from 0x%04x for target %s", aMessageInfo.GetPeerAddr().GetLocator(),
                 target.ToString().AsCString());

    if (Get<ThreadNetif>().IsUnicastAddress(target))
    {
        SendAddressQueryResponse(target, Get<Mle::MleRouter>().GetMeshLocal64().GetIid(), NULL,
                                 aMessageInfo.GetPeerAddr());
        ExitNow();
    }

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (child.IsFullThreadDevice() || child.GetLinkFailures() >= Mle::kFailedChildTransmissions)
        {
            continue;
        }

        if (child.HasIp6Address(target))
        {
            Mac::ExtAddress addr;

            // Convert extended address to IID.
            addr = child.GetExtAddress();
            addr.ToggleLocal();
            lastTransactionTime = TimerMilli::GetNow() - child.GetLastHeard();
            SendAddressQueryResponse(target, addr.m8, &lastTransactionTime, aMessageInfo.GetPeerAddr());
            ExitNow();
        }
    }

exit:
    return;
}

void AddressResolver::SendAddressQueryResponse(const Ip6::Address &aTarget,
                                               const uint8_t *     aMeshLocalIid,
                                               const uint32_t *    aLastTransactionTime,
                                               const Ip6::Address &aDestination)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_NOTIFY));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, aTarget.mFields.m8, sizeof(aTarget)));
    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, aMeshLocalIid,
                                         Ip6::Address::kInterfaceIdentifierSize));
    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, ThreadTlv::kRloc16, Get<Mle::MleRouter>().GetRloc16()));

    if (aLastTransactionTime != NULL)
    {
        SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, ThreadTlv::kLastTransactionTime, *aLastTransactionTime));
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address notification for target %s", aTarget.ToString().AsCString());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void AddressResolver::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<AddressResolver>().HandleTimer();
}

void AddressResolver::HandleTimer(void)
{
    bool        continueTimer = false;
    CacheEntry *prev;
    CacheEntry *entry;
    CacheEntry *next;

    for (entry = mSnoopedList.GetHead(); entry != NULL; entry = entry->GetNext())
    {
        if (entry->IsTimeoutZero())
        {
            continue;
        }

        continueTimer = true;
        entry->DecrementTimeout();

        if (entry->IsTimeoutZero())
        {
            entry->SetCanEvict(true);
        }
    }

    for (entry = mQueryRetryList.GetHead(); entry != NULL; entry = entry->GetNext())
    {
        if (entry->IsTimeoutZero())
        {
            continue;
        }

        continueTimer = true;
        entry->DecrementTimeout();
    }

    for (prev = NULL, entry = mQueryList.GetHead(); entry != NULL; entry = next)
    {
        next = entry->GetNext();

        OT_ASSERT(!entry->IsTimeoutZero());

        continueTimer = true;
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
        }
    }

    if (continueTimer)
    {
        mTimer.Start(kStateUpdatePeriod);
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
                                                                *static_cast<const Ip6::IcmpHeader *>(aIcmpHeader));
}

void AddressResolver::HandleIcmpReceive(Message &               aMessage,
                                        const Ip6::MessageInfo &aMessageInfo,
                                        const Ip6::IcmpHeader & aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Ip6::Header ip6Header;

    VerifyOrExit(aIcmpHeader.GetType() == Ip6::IcmpHeader::kTypeDstUnreach);
    VerifyOrExit(aIcmpHeader.GetCode() == Ip6::IcmpHeader::kCodeDstUnreachNoRoute);
    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(ip6Header), &ip6Header) == sizeof(ip6Header));

    Remove(ip6Header.GetDestination(), kReasonReceivedIcmpDstUnreachNoRoute);

exit:
    return;
}

// LCOV_EXCL_START

const char *AddressResolver::ReasonToString(Reason aReason)
{
    const char *str = "";

    switch (aReason)
    {
    case kReasonRemovingRouterId:
        str = "removing router id";
        break;

    case kReasonRemovingRloc16:
        str = "removing rloc16";
        break;

    case kReasonReceivedIcmpDstUnreachNoRoute:
        str = "received icmp no route";
        break;

    case kReasonEvictingForNewEntry:
        str = "evicting for new entry";
        break;

    case kReasonRemovingEid:
        str = "removing eid";
        break;
    }

    return str;
}

const char *AddressResolver::ListToString(const CacheEntryList &aList) const
{
    const char *str;

    VerifyOrExit(&aList != &mCachedList, str = "cached");
    VerifyOrExit(&aList != &mSnoopedList, str = "snooped");
    VerifyOrExit(&aList != &mQueryList, str = "query");
    VerifyOrExit(&aList != &mQueryRetryList, str = "query-retry");
    str = "unused";

exit:
    return str;
}

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
    return (mNextIndex == kNoNextIndex) ? NULL : &Get<AddressResolver>().mCacheEntries[mNextIndex];
}

const AddressResolver::CacheEntry *AddressResolver::CacheEntry::GetNext(void) const
{
    return (mNextIndex == kNoNextIndex) ? NULL : &Get<AddressResolver>().mCacheEntries[mNextIndex];
}

void AddressResolver::CacheEntry::SetNext(CacheEntry *aEntry)
{
    VerifyOrExit(aEntry != NULL, mNextIndex = kNoNextIndex);
    mNextIndex = static_cast<uint16_t>(aEntry - Get<AddressResolver>().mCacheEntries);

exit:
    return;
}

bool AddressResolver::CacheEntry::HasMeshLocalIid(const uint8_t *aIid) const
{
    return memcmp(mInfo.mCached.mMeshLocalIid, aIid, Ip6::Address::kInterfaceIdentifierSize) == 0;
}

void AddressResolver::CacheEntry::SetMeshLocalIid(const uint8_t *aIid)
{
    memcpy(mInfo.mCached.mMeshLocalIid, aIid, Ip6::Address::kInterfaceIdentifierSize);
}

} // namespace ot

#endif // OPENTHREAD_FTD
