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

#define WPP_NAME "address_resolver.tmh"

#include "address_resolver.hpp"

#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "mac/mac_frame.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

AddressResolver::AddressResolver(Instance &aInstance) :
    InstanceLocator(aInstance),
    mAddressError(OT_URI_PATH_ADDRESS_ERROR, &AddressResolver::HandleAddressError, this),
    mAddressQuery(OT_URI_PATH_ADDRESS_QUERY, &AddressResolver::HandleAddressQuery, this),
    mAddressNotification(OT_URI_PATH_ADDRESS_NOTIFY, &AddressResolver::HandleAddressNotification, this),
    mIcmpHandler(&AddressResolver::HandleIcmpReceive, this),
    mTimer(aInstance, &AddressResolver::HandleTimer, this)
{
    Clear();

    GetNetif().GetCoap().AddResource(mAddressError);
    GetNetif().GetCoap().AddResource(mAddressQuery);
    GetNetif().GetCoap().AddResource(mAddressNotification);

    GetNetif().GetIp6().GetIcmp().RegisterHandler(mIcmpHandler);
}

void AddressResolver::Clear()
{
    memset(&mCache, 0, sizeof(mCache));

    for (uint8_t i = 0; i < kCacheEntries; i++)
    {
        mCache[i].mAge = i;
    }
}

otError AddressResolver::GetEntry(uint8_t aIndex, otEidCacheEntry &aEntry) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIndex < kCacheEntries, error = OT_ERROR_INVALID_ARGS);
    memcpy(&aEntry.mTarget, &mCache[aIndex].mTarget, sizeof(aEntry.mTarget));
    aEntry.mRloc16 = mCache[aIndex].mRloc16;
    aEntry.mValid = mCache[aIndex].mState == Cache::kStateCached;

exit:
    return error;
}

void AddressResolver::Remove(uint8_t aRouterId)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (Mle::Mle::GetRouterId(mCache[i].mRloc16) == aRouterId)
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

void AddressResolver::MarkCacheEntryAsUsed(Cache &aEntry)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mAge < aEntry.mAge)
        {
            mCache[i].mAge++;
        }
    }

    aEntry.mAge = 0;
}

const char *AddressResolver::ConvertInvalidationReasonToString(InvalidationReason aReason)
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
    }

    return str;
}

void AddressResolver::InvalidateCacheEntry(Cache &aEntry, InvalidationReason aReason)
{
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mAge > aEntry.mAge)
        {
            mCache[i].mAge--;
        }
    }

    switch (aEntry.mState)
    {
    case Cache::kStateCached:
        otLogInfoArp(GetInstance(), "Cache entry removed: %s, 0x%04x - %s",
                     aEntry.mTarget.ToString(stringBuffer, sizeof(stringBuffer)), aEntry.mRloc16,
                     ConvertInvalidationReasonToString(aReason));
        break;

    case Cache::kStateQuery:
        otLogInfoArp(GetInstance(), "Cache entry (query mode) removed: %s, timeout:%d, retry:%d - %s",
                     aEntry.mTarget.ToString(stringBuffer, sizeof(stringBuffer)), aEntry.mTimeout,
                     aEntry.mRetryTimeout, ConvertInvalidationReasonToString(aReason));
        break;

    default:
        break;
    }

    aEntry.mAge = kCacheEntries - 1;
    aEntry.mState = Cache::kStateInvalid;

    OT_UNUSED_VARIABLE(stringBuffer);
    OT_UNUSED_VARIABLE(aReason);
}

void AddressResolver::UpdateCacheEntry(const Ip6::Address &aEid, Mac::ShortAddress aRloc16)
{
    for (int i = 0; i < kCacheEntries; i++)
    {
        if (mCache[i].mState == Cache::kStateInvalid || mCache[i].mTarget != aEid)
        {
            continue;
        }

        if (mCache[i].mRloc16 != aRloc16)
        {
            char stringBuffer[Ip6::Address::kIp6AddressStringSize];

            // not updating the age here is intentional because this cache entry is not actually being used
            mCache[i].mRloc16 = aRloc16;

            if (mCache[i].mState != Cache::kStateCached)
            {
                mCache[i].mRetryTimeout = 0;
                mCache[i].mLastTransactionTime = static_cast<uint32_t>(kLastTransactionTimeInvalid);
                mCache[i].mTimeout = 0;
                mCache[i].mFailures = 0;
                mCache[i].mState = Cache::kStateCached;

                GetNetif().GetMeshForwarder().HandleResolved(aEid, OT_ERROR_NONE);
            }

            otLogInfoArp(GetInstance(), "Cache entry updated (snoop): %s, 0x%04x",
                         aEid.ToString(stringBuffer, sizeof(stringBuffer)), aRloc16);

            OT_UNUSED_VARIABLE(stringBuffer);
        }

        ExitNow();
    }

exit:
    return;
}

otError AddressResolver::Resolve(const Ip6::Address &aEid, uint16_t &aRloc16)
{
    otError error = OT_ERROR_NONE;
    Cache *entry = NULL;

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
        entry->mTarget = aEid;
        entry->mRloc16 = Mac::kShortAddrInvalid;
        entry->mTimeout = kAddressQueryTimeout;
        entry->mFailures = 0;
        entry->mRetryTimeout = kAddressQueryInitialRetryDelay;
        entry->mState = Cache::kStateQuery;
        error = OT_ERROR_ADDRESS_QUERY;
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
            error = OT_ERROR_ADDRESS_QUERY;
        }
        else
        {
            error = OT_ERROR_DROP;
        }

        break;

    case Cache::kStateCached:
        aRloc16 = entry->mRloc16;
        MarkCacheEntryAsUsed(*entry);
        break;
    }

exit:
    return error;
}

otError AddressResolver::SendAddressQuery(const Ip6::Address &aEid)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    Message *message;
    Coap::Header header;
    ThreadTargetTlv targetTlv;
    Ip6::MessageInfo messageInfo;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_ADDRESS_QUERY);
    header.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    targetTlv.Init();
    targetTlv.SetTarget(aEid);
    SuccessOrExit(error = message->Append(&targetTlv, sizeof(targetTlv)));

    messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x0002);
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoArp(GetInstance(), "Sending address query for %s", aEid.ToString(stringBuffer, sizeof(stringBuffer)));

    OT_UNUSED_VARIABLE(stringBuffer);

exit:

    if (mTimer.IsRunning() == false)
    {
        mTimer.Start(kStateUpdatePeriod);
    }

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void AddressResolver::HandleAddressNotification(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressNotification(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressNotification(Coap::Header &aHeader, Message &aMessage,
                                                const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    ThreadTargetTlv targetTlv;
    ThreadMeshLocalEidTlv mlIidTlv;
    ThreadRloc16Tlv rloc16Tlv;
    ThreadLastTransactionTimeTlv lastTransactionTimeTlv;
    uint32_t lastTransactionTime;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid());

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kMeshLocalEid, sizeof(mlIidTlv), mlIidTlv));
    VerifyOrExit(mlIidTlv.IsValid());

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rloc16Tlv), rloc16Tlv));
    VerifyOrExit(rloc16Tlv.IsValid());

    lastTransactionTime = 0;

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kLastTransactionTime, sizeof(lastTransactionTimeTlv),
                          lastTransactionTimeTlv) == OT_ERROR_NONE)
    {
        VerifyOrExit(lastTransactionTimeTlv.IsValid());
        lastTransactionTime = lastTransactionTimeTlv.GetTime();
    }

    otLogInfoArp(GetInstance(), "Received address notification from 0x%04x for %s to 0x%04x",
                 HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]),
                 targetTlv.GetTarget().ToString(stringBuffer, sizeof(stringBuffer)), rloc16Tlv.GetRloc16());

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
            mCache[i].mRloc16 = rloc16Tlv.GetRloc16();
            mCache[i].mRetryTimeout = 0;
            mCache[i].mLastTransactionTime = lastTransactionTime;
            mCache[i].mTimeout = 0;
            mCache[i].mFailures = 0;
            mCache[i].mState = Cache::kStateCached;
            MarkCacheEntryAsUsed(mCache[i]);

            otLogInfoArp(GetInstance(), "Cache entry updated (notification): %s, 0x%04x, lastTrans:%d",
                         stringBuffer, rloc16Tlv.GetRloc16(), lastTransactionTime);

            if (netif.GetCoap().SendEmptyAck(aHeader, aMessageInfo) == OT_ERROR_NONE)
            {
                otLogInfoArp(GetInstance(), "Sending address notification acknowledgment");
            }

            netif.GetMeshForwarder().HandleResolved(targetTlv.GetTarget(), OT_ERROR_NONE);
            break;
        }
    }

    OT_UNUSED_VARIABLE(stringBuffer);

exit:
    return;
}

otError AddressResolver::SendAddressError(const ThreadTargetTlv &aTarget, const ThreadMeshLocalEidTlv &aEid,
                                          const Ip6::Address *aDestination)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    Message *message;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    header.Init(aDestination == NULL ? OT_COAP_TYPE_NON_CONFIRMABLE : OT_COAP_TYPE_CONFIRMABLE,
                OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_ADDRESS_ERROR);
    header.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(&aTarget, sizeof(aTarget)));
    SuccessOrExit(error = message->Append(&aEid, sizeof(aEid)));

    if (aDestination == NULL)
    {
        messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
        messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x0002);
    }
    else
    {
        memcpy(&messageInfo.GetPeerAddr(), aDestination, sizeof(messageInfo.GetPeerAddr()));
    }

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoArp(GetInstance(), "Sending address error for target %s",
                 aTarget.GetTarget().ToString(stringBuffer, sizeof(stringBuffer)));

    OT_UNUSED_VARIABLE(stringBuffer);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void AddressResolver::HandleAddressError(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                         const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressError(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressError(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    ThreadTargetTlv targetTlv;
    ThreadMeshLocalEidTlv mlIidTlv;
    Child *children;
    uint8_t numChildren;
    Mac::ExtAddress macAddr;
    Ip6::Address destination;

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoArp(GetInstance(), "Received address error notification");

    if (aHeader.IsConfirmable() && !aMessageInfo.GetSockAddr().IsMulticast())
    {
        if (netif.GetCoap().SendEmptyAck(aHeader, aMessageInfo) == OT_ERROR_NONE)
        {
            otLogInfoArp(GetInstance(), "Sent address error notification acknowledgment");
        }
    }

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kMeshLocalEid, sizeof(mlIidTlv), mlIidTlv));
    VerifyOrExit(mlIidTlv.IsValid(), error = OT_ERROR_PARSE);

    for (const Ip6::NetifUnicastAddress *address = netif.GetUnicastAddresses(); address; address = address->GetNext())
    {
        if (address->GetAddress() == targetTlv.GetTarget() &&
            memcmp(netif.GetMle().GetMeshLocal64().GetIid(), mlIidTlv.GetIid(), 8))
        {
            // Target EID matches address and Mesh Local EID differs
            netif.RemoveUnicastAddress(*address);
            ExitNow();
        }
    }

    children = netif.GetMle().GetChildren(&numChildren);

    memcpy(&macAddr, mlIidTlv.GetIid(), sizeof(macAddr));
    macAddr.m8[0] ^= 0x2;

    for (int i = 0; i < numChildren; i++)
    {
        Child &child = children[i];

        if (child.GetState() != Neighbor::kStateValid || child.IsFullThreadDevice())
        {
            continue;
        }

        if (child.GetExtAddress() != macAddr)
        {
            // Mesh Local EID differs, so check whether Target EID
            // matches a child address and if so remove it.

            if (child.RemoveIp6Address(GetInstance(), targetTlv.GetTarget()) == OT_ERROR_NONE)
            {
                memset(&destination, 0, sizeof(destination));
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
        otLogWarnArp(GetInstance(), "Error while processing address error notification: %s",
                     otThreadErrorToString(error));
    }

    return;
}

void AddressResolver::HandleAddressQuery(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                         const otMessageInfo *aMessageInfo)
{
    static_cast<AddressResolver *>(aContext)->HandleAddressQuery(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void AddressResolver::HandleAddressQuery(Coap::Header &aHeader, Message &aMessage,
                                         const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    ThreadTargetTlv targetTlv;
    ThreadMeshLocalEidTlv mlIidTlv;
    ThreadLastTransactionTimeTlv lastTransactionTimeTlv;
    Child *children;
    uint8_t numChildren;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kTarget, sizeof(targetTlv), targetTlv));
    VerifyOrExit(targetTlv.IsValid());

    mlIidTlv.Init();

    lastTransactionTimeTlv.Init();

    otLogInfoArp(GetInstance(), "Received address query from 0x%04x for target %s",
                 HostSwap16(aMessageInfo.GetPeerAddr().mFields.m16[7]),
                 targetTlv.GetTarget().ToString(stringBuffer, sizeof(stringBuffer)));

    if (netif.IsUnicastAddress(targetTlv.GetTarget()))
    {
        mlIidTlv.SetIid(netif.GetMle().GetMeshLocal64().GetIid());
        SendAddressQueryResponse(targetTlv, mlIidTlv, NULL, aMessageInfo.GetPeerAddr());
        ExitNow();
    }

    children = netif.GetMle().GetChildren(&numChildren);

    for (int i = 0; i < numChildren; i++)
    {
        Child &child = children[i];

        if (child.GetState() != Neighbor::kStateValid ||
            child.IsFullThreadDevice() ||
            child.GetLinkFailures() >= Mle::kFailedChildTransmissions)
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

    OT_UNUSED_VARIABLE(stringBuffer);

exit:
    return;
}

void AddressResolver::SendAddressQueryResponse(const ThreadTargetTlv &aTargetTlv,
                                               const ThreadMeshLocalEidTlv &aMlIidTlv,
                                               const ThreadLastTransactionTimeTlv *aLastTransactionTimeTlv,
                                               const Ip6::Address &aDestination)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    Message *message;
    Coap::Header header;
    ThreadRloc16Tlv rloc16Tlv;
    Ip6::MessageInfo messageInfo;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_ADDRESS_NOTIFY);
    header.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Append(&aTargetTlv, sizeof(aTargetTlv)));
    SuccessOrExit(error = message->Append(&aMlIidTlv, sizeof(aMlIidTlv)));

    rloc16Tlv.Init();
    rloc16Tlv.SetRloc16(netif.GetMle().GetRloc16());
    SuccessOrExit(error = message->Append(&rloc16Tlv, sizeof(rloc16Tlv)));

    if (aLastTransactionTimeTlv != NULL)
    {
        SuccessOrExit(error = message->Append(aLastTransactionTimeTlv, sizeof(*aLastTransactionTimeTlv)));
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoArp(GetInstance(), "Sending address notification for target %s",
                 aTargetTlv.GetTarget().ToString(stringBuffer, sizeof(stringBuffer)));

    OT_UNUSED_VARIABLE(stringBuffer);

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
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

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

                otLogInfoArp(GetInstance(), "Timed out waiting for address notification for %s, retry: %d",
                             mCache[i].mTarget.ToString(stringBuffer, sizeof(stringBuffer)), mCache[i].mRetryTimeout);

                GetNetif().GetMeshForwarder().HandleResolved(mCache[i].mTarget, OT_ERROR_DROP);
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

    OT_UNUSED_VARIABLE(stringBuffer);
}

void AddressResolver::HandleIcmpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                        const otIcmp6Header *aIcmpHeader)
{
    static_cast<AddressResolver *>(aContext)->HandleIcmpReceive(*static_cast<Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                *static_cast<const Ip6::IcmpHeader *>(aIcmpHeader));

    OT_UNUSED_VARIABLE(aMessageInfo);
}

void AddressResolver::HandleIcmpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                        const Ip6::IcmpHeader &aIcmpHeader)
{
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
    OT_UNUSED_VARIABLE(aMessageInfo);
}

}  // namespace ot

#endif // OPENTHREAD_FTD
