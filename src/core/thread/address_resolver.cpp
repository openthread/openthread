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
    , mIcmpHandler(&AddressResolver::HandleIcmpReceive, this)
    , mTimer(aInstance, &AddressResolver::HandleTimer, this)
{
    Init();

    Get<Coap::Coap>().AddResource(mAddressError);
    Get<Coap::Coap>().AddResource(mAddressQuery);
    Get<Coap::Coap>().AddResource(mAddressNotification);

    Get<Ip6::Icmp>().RegisterHandler(mIcmpHandler);
}

void AddressResolver::Init(void)
{
    memset(&mCache, 0, sizeof(mCache));

    for (uint8_t i = 0; i < kCacheEntries; i++)
    {
        mCache[i].mAge = i;
    }
}

void AddressResolver::Clear(void)
{
    for (uint8_t i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState != Cache::kStateQuery)
        {
            continue;
        }

        Get<MeshForwarder>().HandleResolved(mCache[i].mTarget, OT_ERROR_DROP);
    }

    Init();
}

otError AddressResolver::GetEntry(uint8_t aIndex, otEidCacheEntry &aEntry) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIndex < kCacheEntries, error = OT_ERROR_INVALID_ARGS);

    aEntry.mTarget = mCache[aIndex].mTarget;
    aEntry.mRloc16 = mCache[aIndex].mRloc16;
    aEntry.mAge    = mCache[aIndex].mAge;
    aEntry.mValid  = mCache[aIndex].mState == Cache::kStateCached;

exit:
    return error;
}

void AddressResolver::Remove(uint8_t aRouterId)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (Mle::Mle::RouterIdFromRloc16(mCache[i].mRloc16) == aRouterId)
        {
            InvalidateCacheEntry(mCache[i], kReasonRemovingRouterId);
        }
    }
}

void AddressResolver::Remove(uint16_t aRloc16)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mRloc16 == aRloc16)
        {
            InvalidateCacheEntry(mCache[i], kReasonRemovingRloc16);
        }
    }
}

void AddressResolver::Remove(const Ip6::Address &aEid)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState == Cache::kStateInvalid || mCache[i].mTarget != aEid)
        {
            continue;
        }

        InvalidateCacheEntry(mCache[i], kReasonRemovingEid);
        break;
    }
}

AddressResolver::Cache *AddressResolver::NewCacheEntry(void)
{
    Cache *rval = NULL;

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState == Cache::kStateQuery && mCache[i].mFailures == 0)
        {
            continue;
        }

        if (rval == NULL || rval->mAge < mCache[i].mAge)
        {
            rval = &mCache[i];
        }
    }

    if (rval != NULL)
    {
        InvalidateCacheEntry(*rval, kReasonEvictingForNewEntry);
    }

    return rval;
}

void AddressResolver::MoveCacheEntryAt(Cache &aEntry, uint8_t aAge)
{
    VerifyOrExit(aEntry.mAge != aAge);

    if (aEntry.mAge > aAge)
    {
        for (int i = 0; i < kCacheEntries; i++)
        {
            if (mCache[i].mAge >= aAge && mCache[i].mAge < aEntry.mAge)
            {
                mCache[i].mAge++;
            }
        }
    }
    else
    {
        for (int i = 0; i < kCacheEntries; i++)
        {
            if (mCache[i].mAge > aEntry.mAge && mCache[i].mAge <= aAge)
            {
                mCache[i].mAge--;
            }
        }
    }

    aEntry.mAge = aAge;

exit:
    return;
}

void AddressResolver::MoveCacheEntryBehindCached(Cache &aEntry)
{
    bool    found             = false;
    uint8_t lastInCachedState = 0;

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState != Cache::kStateCached || mCache[i].mAge == aEntry.mAge)
        {
            continue;
        }

        if (mCache[i].mAge >= lastInCachedState)
        {
            found             = true;
            lastInCachedState = mCache[i].mAge;
        }
    }

    if (!found)
    {
        MoveCacheEntryAt(aEntry, 0);
    }
    else
    {
        uint8_t newAge = lastInCachedState + 1;

        if (newAge == kCacheEntries)
        {
            newAge = kCacheEntries - 1;
        }

        MoveCacheEntryAt(aEntry, newAge);
    }
}

const char *AddressResolver::InvalidationReasonToString(InvalidationReason aReason)
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

void AddressResolver::InvalidateCacheEntry(Cache &aEntry, InvalidationReason aReason)
{
    OT_UNUSED_VARIABLE(aReason);

    switch (aEntry.mState)
    {
    case Cache::kStateCached:
        otLogNoteArp("Cache entry removed: %s, 0x%04x - %s", aEntry.mTarget.ToString().AsCString(), aEntry.mRloc16,
                     InvalidationReasonToString(aReason));
        break;

    case Cache::kStateQuery:
        otLogNoteArp("Cache entry (query mode) removed: %s, timeout:%d, retry:%d - %s",
                     aEntry.mTarget.ToString().AsCString(), aEntry.mTimeout, aEntry.mRetryTimeout,
                     InvalidationReasonToString(aReason));

        Get<MeshForwarder>().HandleResolved(aEntry.mTarget, OT_ERROR_DROP);
        break;

    default:
        break;
    }

    aEntry.mState = Cache::kStateInvalid;
    MoveCacheEntryAtBack(aEntry);
}

otError AddressResolver::UpdateCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState == Cache::kStateInvalid || mCache[i].mTarget != aEid)
        {
            continue;
        }

        if (mCache[i].mRloc16 != aRloc16)
        {
            // not updating the age here is intentional because this cache entry is not actually being used
            mCache[i].mRloc16 = aRloc16;

            if (mCache[i].mState != Cache::kStateCached)
            {
                mCache[i].mLastTransactionTime = static_cast<uint32_t>(kLastTransactionTimeInvalid);
                mCache[i].mRetryTimeout        = 0;
                mCache[i].mTimeout             = 0;
                mCache[i].mFailures            = 0;
                mCache[i].mState               = Cache::kStateCached;
                MoveCacheEntryAtFront(mCache[i]);

                Get<MeshForwarder>().HandleResolved(aEid, OT_ERROR_NONE);
            }

            otLogNoteArp("Cache entry updated (snoop): %s, 0x%04x", aEid.ToString().AsCString(), aRloc16);
        }

        error = OT_ERROR_NONE;
    }

    return error;
}

otError AddressResolver::AddCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    otError error = OT_ERROR_NONE;
    Cache * entry = NewCacheEntry();

    VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);

    entry->mTarget              = aEid;
    entry->mLastTransactionTime = static_cast<uint32_t>(kLastTransactionTimeInvalid);
    entry->mRloc16              = aRloc16;
    entry->mRetryTimeout        = 0;
    entry->mTimeout             = 0;
    entry->mFailures            = 0;
    entry->mState               = Cache::kStateCached;
    MoveCacheEntryBehindCached(*entry);

    otLogNoteArp("Cache entry added (snoop): %s, 0x%04x", aEid.ToString().AsCString(), aRloc16);

exit:
    return error;
}

void AddressResolver::RestartAddressQueries(void)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        Cache &entry = mCache[i];

        if (entry.mState != Cache::kStateQuery)
        {
            continue;
        }

        SendAddressQuery(entry.mTarget);

        entry.mTimeout      = kAddressQueryTimeout;
        entry.mFailures     = 0;
        entry.mRetryTimeout = kAddressQueryInitialRetryDelay;
    }
}

otError AddressResolver::Resolve(const Ip6::Address &aEid, uint16_t &aRloc16)
{
    otError error = OT_ERROR_NONE;
    Cache * entry = NULL;

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState != Cache::kStateInvalid)
        {
            if (mCache[i].mTarget == aEid)
            {
                entry = &mCache[i];
                break;
            }
        }
    }

    if (entry == NULL)
    {
        entry = NewCacheEntry();
    }

    VerifyOrExit(entry != NULL, error = OT_ERROR_NO_BUFS);

    switch (entry->mState)
    {
    case Cache::kStateInvalid:
        SuccessOrExit(error = SendAddressQuery(aEid));
        entry->mTarget       = aEid;
        entry->mRloc16       = Mac::kShortAddrInvalid;
        entry->mTimeout      = kAddressQueryTimeout;
        entry->mFailures     = 0;
        entry->mRetryTimeout = kAddressQueryInitialRetryDelay;
        entry->mState        = Cache::kStateQuery;
        error                = OT_ERROR_ADDRESS_QUERY;
        break;

    case Cache::kStateQuery:
        if (entry->mTimeout > 0)
        {
            error = OT_ERROR_ADDRESS_QUERY;
        }
        else if (entry->mTimeout == 0 && entry->mRetryTimeout == 0)
        {
            SuccessOrExit(error = SendAddressQuery(aEid));
            entry->mTimeout = kAddressQueryTimeout;
            error           = OT_ERROR_ADDRESS_QUERY;
        }
        else
        {
            error = OT_ERROR_DROP;
        }

        break;

    case Cache::kStateCached:
        aRloc16 = entry->mRloc16;
        MoveCacheEntryAtFront(*entry);
        break;
    }

exit:
    return error;
}

otError AddressResolver::SendAddressQuery(const Ip6::Address &aEid)
{
    otError          error;
    Coap::Message *  message;
    ThreadTargetTlv  targetTlv;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_QUERY));
    SuccessOrExit(error = message->SetPayloadMarker());

    targetTlv.Init();
    targetTlv.SetTarget(aEid);
    SuccessOrExit(error = targetTlv.AppendTo(*message));

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
    ThreadTargetTlv              targetTlv;
    ThreadMeshLocalEidTlv        mlIidTlv;
    ThreadRloc16Tlv              rloc16Tlv;
    ThreadLastTransactionTimeTlv lastTransactionTimeTlv;
    uint32_t                     lastTransactionTime;

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid());
    targetTlv.Init(); // reset TLV length

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kMeshLocalEid, sizeof(mlIidTlv), mlIidTlv));
    VerifyOrExit(mlIidTlv.IsValid());
    mlIidTlv.Init(); // reset TLV length

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rloc16Tlv), rloc16Tlv));
    VerifyOrExit(rloc16Tlv.IsValid());

    lastTransactionTime = 0;

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kLastTransactionTime, sizeof(lastTransactionTimeTlv),
                          lastTransactionTimeTlv) == OT_ERROR_NONE)
    {
        VerifyOrExit(lastTransactionTimeTlv.IsValid());
        lastTransactionTime = lastTransactionTimeTlv.GetTime();
    }

    otLogInfoArp("Received address notification from 0x%04x for %s to 0x%04x",
                 HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]), targetTlv.GetTarget().ToString().AsCString(),
                 rloc16Tlv.GetRloc16());

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mTarget != targetTlv.GetTarget())
        {
            continue;
        }

        switch (mCache[i].mState)
        {
        case Cache::kStateInvalid:
            break;

        case Cache::kStateCached:
            if (mCache[i].mLastTransactionTime != kLastTransactionTimeInvalid)
            {
                if (memcmp(mCache[i].mMeshLocalIid, mlIidTlv.GetIid(), sizeof(mCache[i].mMeshLocalIid)) != 0)
                {
                    SendAddressError(targetTlv, mlIidTlv, NULL);
                    ExitNow();
                }

                if (lastTransactionTime >= mCache[i].mLastTransactionTime)
                {
                    ExitNow();
                }
            }

            // fall through

        case Cache::kStateQuery:
            memcpy(mCache[i].mMeshLocalIid, mlIidTlv.GetIid(), sizeof(mCache[i].mMeshLocalIid));
            mCache[i].mRloc16              = rloc16Tlv.GetRloc16();
            mCache[i].mRetryTimeout        = 0;
            mCache[i].mLastTransactionTime = lastTransactionTime;
            mCache[i].mTimeout             = 0;
            mCache[i].mFailures            = 0;
            mCache[i].mState               = Cache::kStateCached;
            MoveCacheEntryAtFront(mCache[i]);

            otLogNoteArp("Cache entry updated (notification): %s, 0x%04x, lastTrans:%d",
                         targetTlv.GetTarget().ToString().AsCString(), rloc16Tlv.GetRloc16(), lastTransactionTime);

            if (Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
            {
                otLogInfoArp("Sending address notification acknowledgment");
            }

            Get<MeshForwarder>().HandleResolved(targetTlv.GetTarget(), OT_ERROR_NONE);
            break;
        }
    }

exit:
    return;
}

otError AddressResolver::SendAddressError(const ThreadTargetTlv &      aTarget,
                                          const ThreadMeshLocalEidTlv &aEid,
                                          const Ip6::Address *         aDestination)
{
    otError          error;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(aDestination == NULL ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_ERROR));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = aTarget.AppendTo(*message));
    SuccessOrExit(error = aEid.AppendTo(*message));

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

    otLogInfoArp("Sending address error for target %s", aTarget.GetTarget().ToString().AsCString());

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
    otError               error = OT_ERROR_NONE;
    ThreadTargetTlv       targetTlv;
    ThreadMeshLocalEidTlv mlIidTlv;
    Mac::ExtAddress       macAddr;
    Ip6::Address          destination;

    VerifyOrExit(aMessage.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoArp("Received address error notification");

    if (aMessage.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        if (Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoArp("Sent address error notification acknowledgment");
        }
    }

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid(), error = OT_ERROR_PARSE);
    targetTlv.Init(); // reset TLV length

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kMeshLocalEid, sizeof(mlIidTlv), mlIidTlv));
    VerifyOrExit(mlIidTlv.IsValid(), error = OT_ERROR_PARSE);
    mlIidTlv.Init(); // reset TLV length

    for (const Ip6::NetifUnicastAddress *address = Get<ThreadNetif>().GetUnicastAddresses(); address;
         address                                 = address->GetNext())
    {
        if (address->GetAddress() == targetTlv.GetTarget() &&
            memcmp(Get<Mle::MleRouter>().GetMeshLocal64().GetIid(), mlIidTlv.GetIid(), 8))
        {
            // Target EID matches address and Mesh Local EID differs
            Get<ThreadNetif>().RemoveUnicastAddress(*address);
            ExitNow();
        }
    }

    macAddr.Set(mlIidTlv.GetIid());
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

            if (child.RemoveIp6Address(GetInstance(), targetTlv.GetTarget()) == OT_ERROR_NONE)
            {
                destination.Clear();
                destination.mFields.m16[0] = HostSwap16(0xfe80);
                destination.SetIid(child.GetExtAddress());

                SendAddressError(targetTlv, mlIidTlv, &destination);
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
    ThreadTargetTlv              targetTlv;
    ThreadMeshLocalEidTlv        mlIidTlv;
    ThreadLastTransactionTimeTlv lastTransactionTimeTlv;

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid());
    targetTlv.Init(); // reset TLV length

    mlIidTlv.Init();

    lastTransactionTimeTlv.Init();

    otLogInfoArp("Received address query from 0x%04x for target %s",
                 HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]), targetTlv.GetTarget().ToString().AsCString());

    if (Get<ThreadNetif>().IsUnicastAddress(targetTlv.GetTarget()))
    {
        mlIidTlv.SetIid(Get<Mle::MleRouter>().GetMeshLocal64().GetIid());
        SendAddressQueryResponse(targetTlv, mlIidTlv, NULL, aMessageInfo.GetPeerAddr());
        ExitNow();
    }

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (child.IsFullThreadDevice() || child.GetLinkFailures() >= Mle::kFailedChildTransmissions)
        {
            continue;
        }

        if (child.HasIp6Address(GetInstance(), targetTlv.GetTarget()))
        {
            mlIidTlv.SetIid(child.GetExtAddress());
            lastTransactionTimeTlv.SetTime(TimerMilli::GetNow() - child.GetLastHeard());
            SendAddressQueryResponse(targetTlv, mlIidTlv, &lastTransactionTimeTlv, aMessageInfo.GetPeerAddr());
            ExitNow();
        }
    }

exit:
    return;
}

void AddressResolver::SendAddressQueryResponse(const ThreadTargetTlv &             aTargetTlv,
                                               const ThreadMeshLocalEidTlv &       aMlEidTlv,
                                               const ThreadLastTransactionTimeTlv *aLastTransactionTimeTlv,
                                               const Ip6::Address &                aDestination)
{
    otError          error;
    Coap::Message *  message;
    ThreadRloc16Tlv  rloc16Tlv;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_ADDRESS_NOTIFY));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = aTargetTlv.AppendTo(*message));
    SuccessOrExit(error = aMlEidTlv.AppendTo(*message));

    rloc16Tlv.Init();
    rloc16Tlv.SetRloc16(Get<Mle::MleRouter>().GetRloc16());
    SuccessOrExit(error = rloc16Tlv.AppendTo(*message));

    if (aLastTransactionTimeTlv != NULL)
    {
        SuccessOrExit(error = aLastTransactionTimeTlv->AppendTo(*message));
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoArp("Sending address notification for target %s", aTargetTlv.GetTarget().ToString().AsCString());

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
    bool continueTimer = false;

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState != Cache::kStateQuery)
        {
            continue;
        }

        continueTimer = true;

        if (mCache[i].mTimeout > 0)
        {
            mCache[i].mTimeout--;

            if (mCache[i].mTimeout == 0)
            {
                mCache[i].mRetryTimeout =
                    static_cast<uint16_t>(kAddressQueryInitialRetryDelay * (1 << mCache[i].mFailures));

                if (mCache[i].mRetryTimeout < kAddressQueryMaxRetryDelay)
                {
                    mCache[i].mFailures++;
                }
                else
                {
                    mCache[i].mRetryTimeout = kAddressQueryMaxRetryDelay;
                }

                otLogInfoArp("Timed out waiting for address notification for %s, retry: %d",
                             mCache[i].mTarget.ToString().AsCString(), mCache[i].mRetryTimeout);

                Get<MeshForwarder>().HandleResolved(mCache[i].mTarget, OT_ERROR_DROP);
            }
        }
        else if (mCache[i].mRetryTimeout > 0)
        {
            mCache[i].mRetryTimeout--;
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

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState != Cache::kStateInvalid && mCache[i].mTarget == ip6Header.GetDestination())
        {
            InvalidateCacheEntry(mCache[i], kReasonReceivedIcmpDstUnreachNoRoute);
            break;
        }
    }

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_FTD
