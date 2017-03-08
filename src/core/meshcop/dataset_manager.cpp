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
 *   This file implements MeshCoP Datasets manager to process commands.
 *
 */

#define WPP_NAME "dataset_manager.tmh"

#include <stdio.h>

#include "openthread/platform/random.h"
#include "openthread/platform/radio.h"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <coap/coap_header.hpp>
#include <common/debug.hpp>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <common/timer.hpp>
#include <meshcop/dataset.hpp>
#include <meshcop/dataset_manager.hpp>
#include <meshcop/tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>
#include <meshcop/leader.hpp>

namespace Thread {
namespace MeshCoP {

DatasetManager::DatasetManager(ThreadNetif &aThreadNetif, const Tlv::Type aType, const char *aUriSet,
                               const char *aUriGet):
    mLocal(aThreadNetif.GetInstance(), aType),
    mNetwork(aThreadNetif.GetInstance(), aType),
    mNetif(aThreadNetif),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &DatasetManager::HandleTimer, this),
    mUriSet(aUriSet),
    mUriGet(aUriGet)
{
}

ThreadError DatasetManager::ApplyConfiguration(void)
{
    ThreadError error = kThreadError_None;
    Dataset *dataset;
    const Tlv *cur;
    const Tlv *end;

    dataset = mNetif.GetMle().IsAttached() ? &mNetwork : &mLocal;

    cur = reinterpret_cast<const Tlv *>(dataset->GetBytes());
    end = reinterpret_cast<const Tlv *>(dataset->GetBytes() + dataset->GetSize());

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case Tlv::kChannel:
        {
            const ChannelTlv *channel = static_cast<const ChannelTlv *>(cur);
            mNetif.GetMac().SetChannel(static_cast<uint8_t>(channel->GetChannel()));
            break;
        }

        case Tlv::kPanId:
        {
            const PanIdTlv *panid = static_cast<const PanIdTlv *>(cur);
            mNetif.GetMac().SetPanId(panid->GetPanId());
            break;
        }

        case Tlv::kExtendedPanId:
        {
            const ExtendedPanIdTlv *extpanid = static_cast<const ExtendedPanIdTlv *>(cur);
            mNetif.GetMac().SetExtendedPanId(extpanid->GetExtendedPanId());
            break;
        }

        case Tlv::kNetworkName:
        {
            const NetworkNameTlv *name = static_cast<const NetworkNameTlv *>(cur);
            otNetworkName networkName;
            memset(networkName.m8, 0, sizeof(networkName));
            memcpy(networkName.m8, name->GetNetworkName(), name->GetLength());
            mNetif.GetMac().SetNetworkName(networkName.m8);
            break;
        }

        case Tlv::kNetworkMasterKey:
        {
            const NetworkMasterKeyTlv *key = static_cast<const NetworkMasterKeyTlv *>(cur);
            mNetif.GetKeyManager().SetMasterKey(key->GetNetworkMasterKey(), key->GetLength());
            break;
        }

        case Tlv::kMeshLocalPrefix:
        {
            const MeshLocalPrefixTlv *prefix = static_cast<const MeshLocalPrefixTlv *>(cur);
            mNetif.GetMle().SetMeshLocalPrefix(prefix->GetMeshLocalPrefix());
            break;
        }

        case Tlv::kSecurityPolicy:
        {
            const SecurityPolicyTlv *securityPolicy = static_cast<const SecurityPolicyTlv *>(cur);
            mNetif.GetKeyManager().SetKeyRotation(securityPolicy->GetRotationTime());
            mNetif.GetKeyManager().SetSecurityPolicyFlags(securityPolicy->GetFlags());
            break;
        }

        default:
        {
            break;
        }
        }

        cur = cur->GetNext();
    }

    return error;
}

ThreadError DatasetManager::Set(const otOperationalDataset &aDataset, uint8_t &aFlags)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mLocal.Set(aDataset));
    mLocal.Store();
    aFlags = kFlagLocalUpdated;

    switch (mNetif.GetMle().GetDeviceState())
    {
    case Mle::kDeviceStateChild:
    case Mle::kDeviceStateRouter:
        mTimer.Start(1000);
        break;

    case Mle::kDeviceStateLeader:
        mNetwork = mLocal;
        aFlags |= kFlagNetworkUpdated;
        mNetif.GetNetworkDataLeader().IncrementVersion();
        mNetif.GetNetworkDataLeader().IncrementStableVersion();
        break;

    default:
        break;
    }

exit:
    return error;
}

ThreadError DatasetManager::Clear(uint8_t &aFlags, bool aOnlyClearNetwork)
{
    if (!aOnlyClearNetwork && mLocal.Compare(mNetwork) == 0)
    {
        mLocal.Clear(true);
    }

    mNetwork.Clear(false);
    HandleNetworkUpdate(aFlags);
    return kThreadError_None;
}

ThreadError DatasetManager::Set(const Dataset &aDataset)
{
    mNetwork.Set(aDataset);

    if (mLocal.Compare(aDataset) != 0)
    {
        mLocal.Set(aDataset);
        mLocal.Store();
    }

    return kThreadError_None;
}

ThreadError DatasetManager::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                uint16_t aOffset, uint8_t aLength, uint8_t &aFlags)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mNetwork.Set(aMessage, aOffset, aLength));
    mNetwork.SetTimestamp(aTimestamp);
    HandleNetworkUpdate(aFlags);

exit:
    return error;
}

void DatasetManager::HandleNetworkUpdate(uint8_t &aFlags)
{
    int compare = mLocal.Compare(mNetwork);

    aFlags = kFlagNetworkUpdated;

    if (compare > 0)
    {
        mLocal = mNetwork;
        mLocal.Store();
        aFlags |= kFlagLocalUpdated;
    }
    else if (compare < 0)
    {
        mTimer.Start(1000);
    }
}

void DatasetManager::HandleTimer(void *aContext)
{
    static_cast<DatasetManager *>(aContext)->HandleTimer();
}

void DatasetManager::HandleTimer(void)
{
    const Timestamp *pendingActiveTimestamp = NULL;
    const Timestamp *localActiveTimestamp = mNetif.GetActiveDataset().GetLocal().GetTimestamp();
    const ActiveTimestampTlv *tlv = static_cast<const ActiveTimestampTlv *>
                                    (mNetif.GetPendingDataset().GetNetwork().Get(Tlv::kActiveTimestamp));
    pendingActiveTimestamp = static_cast<const Timestamp *>(tlv);

    VerifyOrExit(mNetif.GetMle().IsAttached(),);

    VerifyOrExit(mLocal.Compare(mNetwork) < 0,);

    Register();

    if (pendingActiveTimestamp != NULL &&
        localActiveTimestamp->Compare(*pendingActiveTimestamp) >= 0)
    {
        // stop registration attempts during dataset transition
        ExitNow();
    }
    else
    {
        mTimer.Start(1000);
    }

exit:
    return;
}

ThreadError DatasetManager::Register(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::Address leader;
    Ip6::MessageInfo messageInfo;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriSet);
    header.SetPayloadMarker();

    if (strcmp(mUriSet, OPENTHREAD_URI_PENDING_SET) == 0)
    {
        PendingDatasetBase *pending = static_cast<PendingDatasetBase *>(this);
        pending->UpdateDelayTimer();
    }

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = message->Append(mLocal.GetBytes(), mLocal.GetSize()));

    mNetif.GetMle().GetLeaderAloc(leader);

    messageInfo.SetPeerAddr(leader);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset to leader");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void DatasetManager::Get(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t tlvs[Dataset::kMaxSize];
    uint8_t length = 0;

    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == Tlv::kGet)
        {
            length = tlv.GetLength();
            aMessage.Read(offset + sizeof(Tlv), length, tlvs);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    SendGetResponse(aHeader, aMessageInfo, tlvs, length);
}

ThreadError DatasetManager::Set(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv tlv;
    Timestamp *timestamp;
    uint16_t offset = aMessage.GetOffset();
    Tlv::Type type;
    bool isUpdateFromCommissioner = false;
    bool doesAffectConnectivity = false;
    bool doesAffectMasterKey = false;
    StateTlv::State state = StateTlv::kAccept;

    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    ChannelTlv channel;
    CommissionerSessionIdTlv sessionId;
    MeshLocalPrefixTlv meshLocalPrefix;
    NetworkMasterKeyTlv masterKey;
    PanIdTlv panId;

    activeTimestamp.SetLength(0);
    pendingTimestamp.SetLength(0);
    channel.SetLength(0);
    masterKey.SetLength(0);
    meshLocalPrefix.SetLength(0);
    panId.SetLength(0);
    pendingTimestamp.SetLength(0);
    sessionId.SetLength(0);

    VerifyOrExit(mNetif.GetMle().GetDeviceState() == Mle::kDeviceStateLeader, state = StateTlv::kReject);

    // verify that TLV data size is less than maximum TLV value size
    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize, state = StateTlv::kReject);
        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize, state = StateTlv::kReject);

    type = (strcmp(mUriSet, OPENTHREAD_URI_ACTIVE_SET) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) != kThreadError_None)
    {
        ExitNow(state = StateTlv::kReject);
    }

    VerifyOrExit(activeTimestamp.IsValid(), state = StateTlv::kReject);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), state = StateTlv::kReject);
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    timestamp = (type == Tlv::kActiveTimestamp) ?
                static_cast<Timestamp *>(&activeTimestamp) :
                static_cast<Timestamp *>(&pendingTimestamp);

    VerifyOrExit(mLocal.GetTimestamp() == NULL || mLocal.GetTimestamp()->Compare(*timestamp) > 0,
                 state = StateTlv::kReject);

    // check channel
    if (Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel) == kThreadError_None)
    {
        VerifyOrExit(channel.IsValid() &&
                     channel.GetChannel() >= kPhyMinChannel &&
                     channel.GetChannel() <= kPhyMaxChannel,
                     state = StateTlv::kReject);

        if (channel.GetChannel() != mNetif.GetMac().GetChannel())
        {
            doesAffectConnectivity = true;
        }
    }

    // check PAN ID
    if (Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panId), panId) == kThreadError_None &&
        panId.IsValid() &&
        panId.GetPanId() != mNetif.GetMac().GetPanId())
    {
        doesAffectConnectivity = true;
    }

    // check mesh local prefix
    if (Tlv::GetTlv(aMessage, Tlv::kMeshLocalPrefix, sizeof(meshLocalPrefix), meshLocalPrefix) == kThreadError_None &&
        memcmp(meshLocalPrefix.GetMeshLocalPrefix(), mNetif.GetMle().GetMeshLocalPrefix(),
               meshLocalPrefix.GetLength()))
    {
        doesAffectConnectivity = true;
    }

    // check network master key
    if (Tlv::GetTlv(aMessage, Tlv::kNetworkMasterKey, sizeof(masterKey), masterKey) == kThreadError_None &&
        memcmp(masterKey.GetNetworkMasterKey(), mNetif.GetKeyManager().GetMasterKey(NULL),
               masterKey.GetLength()))
    {
        doesAffectConnectivity = true;
        doesAffectMasterKey = true;
    }

    // check active timestamp rollback
    if (type == Tlv::kPendingTimestamp &&
        (masterKey.GetLength() == 0 ||
         memcmp(masterKey.GetNetworkMasterKey(), mNetif.GetKeyManager().GetMasterKey(NULL),
                masterKey.GetLength()) == 0))
    {
        // no change to master key, active timestamp must be ahead
        const Timestamp *localActiveTimestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

        VerifyOrExit(localActiveTimestamp == NULL || localActiveTimestamp->Compare(activeTimestamp) > 0,
                     state = StateTlv::kReject);
    }

    // check commissioner session id
    if (Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId) == kThreadError_None)
    {
        CommissionerSessionIdTlv *localId;

        isUpdateFromCommissioner = true;

        localId = static_cast<CommissionerSessionIdTlv *>(mNetif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                              Tlv::kCommissionerSessionId));

        VerifyOrExit(sessionId.IsValid() &&
                     localId != NULL &&
                     localId->GetCommissionerSessionId() == sessionId.GetCommissionerSessionId(),
                     state = StateTlv::kReject);
    }

    // verify an MGMT_ACTIVE_SET.req from a Commissioner does not affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || type == Tlv::kPendingTimestamp || !doesAffectConnectivity,
                 state = StateTlv::kReject);

    // update dataset
    if (type == Tlv::kPendingTimestamp && isUpdateFromCommissioner)
    {
        mLocal.Clear(true);
        mLocal.Set(mNetif.GetActiveDataset().GetNetwork());
    }

    if (type == Tlv::kPendingTimestamp || !doesAffectConnectivity)
    {
        offset = aMessage.GetOffset();

        while (offset < aMessage.GetLength())
        {
            OT_TOOL_PACKED_BEGIN
            struct
            {
                Tlv tlv;
                uint8_t value[Dataset::kMaxValueSize];
            } OT_TOOL_PACKED_END data;

            aMessage.Read(offset, sizeof(Tlv), &data.tlv);
            aMessage.Read(offset + sizeof(Tlv), data.tlv.GetLength(), data.value);

            switch (data.tlv.GetType())
            {
            case Tlv::kCommissionerSessionId:
                // do not store Commissioner Session ID TLV
                break;

            case Tlv::kDelayTimer:
            {
                DelayTimerTlv *delayTimerTlv = reinterpret_cast<DelayTimerTlv *>(&tlv);

                if (doesAffectMasterKey && delayTimerTlv->GetDelayTimer() < DelayTimerTlv::kDelayTimerDefault)
                {
                    delayTimerTlv->SetDelayTimer(DelayTimerTlv::kDelayTimerDefault);
                }
                else if (delayTimerTlv->GetDelayTimer() < mNetif.GetLeader().GetDelayTimerMinimal())
                {
                    delayTimerTlv->SetDelayTimer(mNetif.GetLeader().GetDelayTimerMinimal());
                }
            }

            // fall through

            default:
                mLocal.Set(data.tlv);
                break;
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        mLocal.Store();
        mNetwork = mLocal;
        mNetif.GetNetworkDataLeader().IncrementVersion();
        mNetif.GetNetworkDataLeader().IncrementStableVersion();
    }
    else
    {
        mNetif.GetPendingDataset().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        BorderAgentLocatorTlv *borderAgentLocator;
        Ip6::Address destination;

        borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(mNetif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                                      Tlv::kBorderAgentLocator));
        VerifyOrExit(borderAgentLocator != NULL,);

        memset(&destination, 0, sizeof(destination));
        destination = mNetif.GetMle().GetMeshLocal16();
        destination.mFields.m16[4] = HostSwap16(0x0000);
        destination.mFields.m16[5] = HostSwap16(0x00ff);
        destination.mFields.m16[6] = HostSwap16(0xfe00);
        destination.mFields.m16[7] = HostSwap16(borderAgentLocator->GetBorderAgentLocator());

        mNetif.GetLeader().SendDatasetChanged(destination);
    }

exit:

    if (mNetif.GetMle().GetDeviceState() == Mle::kDeviceStateLeader)
    {
        SendSetResponse(aHeader, aMessageInfo, state);
    }

    return state == StateTlv::kAccept ? kThreadError_None : kThreadError_Drop;
}

ThreadError DatasetManager::SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriSet);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

#if OPENTHREAD_ENABLE_COMMISSIONER
    bool isCommissioner;

    isCommissioner = mNetif.GetCommissioner().GetState() != Commissioner::kStateDisabled ? true : false;

    if (isCommissioner)
    {
        const uint8_t *cur = aTlvs;
        const uint8_t *end = aTlvs + aLength;
        bool hasSessionId = false;

        while (cur < end)
        {
            const Tlv *data = reinterpret_cast<const Tlv *>(cur);

            if (data->GetType() == Tlv::kCommissionerSessionId)
            {
                hasSessionId = true;
                break;
            }

            cur += sizeof(Tlv) + data->GetLength();
        }

        if (!hasSessionId)
        {
            CommissionerSessionIdTlv sessionId;
            sessionId.Init();
            sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
            SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));
        }
    }

#endif

    if (aDataset.mIsActiveTimestampSet)
    {
        ActiveTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds(aDataset.mActiveTimestamp);
        static_cast<Timestamp *>(&timestamp)->SetTicks(0);
        SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    }

    if (aDataset.mIsPendingTimestampSet)
    {
        PendingTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds(aDataset.mPendingTimestamp);
        static_cast<Timestamp *>(&timestamp)->SetTicks(0);
        SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    }

    if (aDataset.mIsMasterKeySet)
    {
        NetworkMasterKeyTlv masterkey;
        masterkey.Init();
        masterkey.SetNetworkMasterKey(aDataset.mMasterKey.m8);
        SuccessOrExit(error = message->Append(&masterkey, sizeof(masterkey)));
    }

    if (aDataset.mIsNetworkNameSet)
    {
        NetworkNameTlv networkname;
        networkname.Init();
        networkname.SetNetworkName(aDataset.mNetworkName.m8);
        SuccessOrExit(error = message->Append(&networkname, sizeof(Tlv) + networkname.GetLength()));
    }

    if (aDataset.mIsExtendedPanIdSet)
    {
        ExtendedPanIdTlv extpanid;
        extpanid.Init();
        extpanid.SetExtendedPanId(aDataset.mExtendedPanId.m8);
        SuccessOrExit(error = message->Append(&extpanid, sizeof(extpanid)));
    }

    if (aDataset.mIsMeshLocalPrefixSet)
    {
        MeshLocalPrefixTlv localprefix;
        localprefix.Init();
        localprefix.SetMeshLocalPrefix(aDataset.mMeshLocalPrefix.m8);
        SuccessOrExit(error = message->Append(&localprefix, sizeof(localprefix)));
    }

    if (aDataset.mIsDelaySet)
    {
        DelayTimerTlv delaytimer;
        delaytimer.Init();
        delaytimer.SetDelayTimer(aDataset.mDelay);
        SuccessOrExit(error = message->Append(&delaytimer, sizeof(delaytimer)));
    }

    if (aDataset.mIsPanIdSet)
    {
        PanIdTlv panid;
        panid.Init();
        panid.SetPanId(aDataset.mPanId);
        SuccessOrExit(error = message->Append(&panid, sizeof(panid)));
    }

    if (aDataset.mIsChannelSet)
    {
        ChannelTlv channel;
        channel.Init();
        channel.SetChannelPage(0);
        channel.SetChannel(aDataset.mChannel);
        SuccessOrExit(error = message->Append(&channel, sizeof(channel)));
    }

    if (aDataset.mIsChannelMaskPage0Set)
    {
        ChannelMask0Tlv channelMask;
        channelMask.Init();
        channelMask.SetMask(aDataset.mChannelMaskPage0);
        SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    if (message->GetLength() == header.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset set request to leader");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError DatasetManager::SendGetRequest(const uint8_t *aTlvTypes, const uint8_t aLength,
                                           const otIp6Address *aAddress)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;
    Tlv tlv;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(mUriGet);

    if (aLength > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    if (aLength > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvTypes, aLength));
    }

    if (aAddress != NULL)
    {
        messageInfo.SetPeerAddr(*static_cast<const Ip6::Address *>(aAddress));
    }
    else
    {
        mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    }

    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset get request");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void DatasetManager::SendSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State aState)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message;
    StateTlv state;

    VerifyOrExit((message = mNetif.GetCoapServer().NewMeshCoPMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = mNetif.GetCoapServer().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void DatasetManager::SendGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     uint8_t *aTlvs, uint8_t aLength)
{
    Tlv *tlv;
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message;
    uint8_t index;

    VerifyOrExit((message = mNetif.GetCoapServer().NewMeshCoPMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    if (aLength == 0)
    {
        const Tlv *cur = reinterpret_cast<const Tlv *>(mNetwork.GetBytes());
        const Tlv *end = reinterpret_cast<const Tlv *>(mNetwork.GetBytes() + mNetwork.GetSize());

        while (cur < end)
        {
            if (cur->GetType() != Tlv::kNetworkMasterKey ||
                (mNetif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY))
            {
                SuccessOrExit(error = message->Append(cur, sizeof(Tlv) + cur->GetLength()));
            }

            cur = cur->GetNext();
        }
    }
    else
    {
        for (index = 0; index < aLength; index++)
        {
            if (aTlvs[index] == Tlv::kNetworkMasterKey &&
                !(mNetif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_OBTAIN_MASTER_KEY))
            {
                continue;
            }

            if ((tlv = mNetwork.Get(static_cast<Tlv::Type>(aTlvs[index]))) != NULL)
            {
                SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
            }
        }
    }

    if (message->GetLength() == responseHeader.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    SuccessOrExit(error = mNetif.GetCoapServer().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset get response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

ActiveDatasetBase::ActiveDatasetBase(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, Tlv::kActiveTimestamp, OPENTHREAD_URI_ACTIVE_SET, OPENTHREAD_URI_ACTIVE_GET),
    mResourceGet(OPENTHREAD_URI_ACTIVE_GET, &ActiveDatasetBase::HandleGet, this)
{
    mNetif.GetCoapServer().AddResource(mResourceGet);
}

ThreadError ActiveDatasetBase::Restore(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mLocal.Restore());
    SuccessOrExit(error = DatasetManager::ApplyConfiguration());

exit:
    return error;
}

ThreadError ActiveDatasetBase::Clear(bool aOnlyClearNetwork)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Clear(flags, aOnlyClearNetwork));

exit:
    return error;
}

ThreadError ActiveDatasetBase::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}

ThreadError ActiveDatasetBase::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    DatasetManager::ApplyConfiguration();

    if (mNetif.GetMle().GetDeviceState() == Mle::kDeviceStateLeader)
    {
        mNetif.GetNetworkDataLeader().IncrementVersion();
        mNetif.GetNetworkDataLeader().IncrementStableVersion();
    }

exit:
    return error;
}

ThreadError ActiveDatasetBase::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                   uint16_t aOffset, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength, flags));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}

void ActiveDatasetBase::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                  const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDatasetBase *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDatasetBase::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

PendingDatasetBase::PendingDatasetBase(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, Tlv::kPendingTimestamp, OPENTHREAD_URI_PENDING_SET, OPENTHREAD_URI_PENDING_GET),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &PendingDatasetBase::HandleTimer, this),
    mLocalTime(0),
    mNetworkTime(0),
    mResourceGet(OPENTHREAD_URI_PENDING_GET, &PendingDatasetBase::HandleGet, this)
{
    mNetif.GetCoapServer().AddResource(mResourceGet);
}

ThreadError PendingDatasetBase::Restore(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mLocal.Restore());

    ResetDelayTimer(kFlagLocalUpdated);

exit:
    return error;
}

ThreadError PendingDatasetBase::Clear(bool aOnlyClearNetwork)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Clear(flags, aOnlyClearNetwork));
    ResetDelayTimer(flags);

exit:
    return error;
}

ThreadError PendingDatasetBase::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    ResetDelayTimer(flags);

exit:
    return error;
}

ThreadError PendingDatasetBase::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    ResetDelayTimer(kFlagLocalUpdated | kFlagNetworkUpdated);

exit:
    return error;
}

ThreadError PendingDatasetBase::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                    uint16_t aOffset, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength, flags));
    ResetDelayTimer(flags);

exit:
    return error;
}

void PendingDatasetBase::ResetDelayTimer(uint8_t aFlags)
{
    DelayTimerTlv *delayTimer;

    if (aFlags & kFlagLocalUpdated)
    {
        mLocalTime = Timer::GetNow();
    }

    if (aFlags & kFlagNetworkUpdated)
    {
        mNetworkTime = Timer::GetNow();
        mTimer.Stop();

        if ((delayTimer = static_cast<DelayTimerTlv *>(mNetwork.Get(Tlv::kDelayTimer))) != NULL)
        {
            if (delayTimer->GetDelayTimer() == 0)
            {
                HandleTimer();
            }
            else
            {
                mTimer.Start(delayTimer->GetDelayTimer());
                otLogInfoMeshCoP("delay timer started");
            }
        }
    }
}

void PendingDatasetBase::UpdateDelayTimer(void)
{
    UpdateDelayTimer(mLocal, mLocalTime);
    UpdateDelayTimer(mNetwork, mNetworkTime);
}

void PendingDatasetBase::UpdateDelayTimer(Dataset &aDataset, uint32_t &aStartTime)
{
    DelayTimerTlv *delayTimer;
    uint32_t now = Timer::GetNow();
    uint32_t elapsed;
    uint32_t delay;

    VerifyOrExit((delayTimer = static_cast<DelayTimerTlv *>(aDataset.Get(Tlv::kDelayTimer))) != NULL, ;);

    elapsed = now - aStartTime;

    delay = delayTimer->GetDelayTimer();

    if (delay > elapsed)
    {
        delay -= elapsed;
    }
    else
    {
        delay = 0;
    }

    delayTimer->SetDelayTimer(delay);

    aStartTime = now;

exit:
    return;
}

void PendingDatasetBase::HandleTimer(void *aContext)
{
    static_cast<PendingDatasetBase *>(aContext)->HandleTimer();
}

void PendingDatasetBase::HandleTimer(void)
{
    DelayTimerTlv *delayTimer;

    otLogInfoMeshCoP("pending delay timer expired");

    UpdateDelayTimer();
    delayTimer = static_cast<DelayTimerTlv *>(mNetwork.Get(Tlv::kDelayTimer));
    assert(delayTimer != NULL && delayTimer->GetDelayTimer() == 0);

    mNetif.GetActiveDataset().Set(mNetwork);

    Clear(false);
}

void PendingDatasetBase::ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    DelayTimerTlv delayTimer;
    uint8_t flags;

    VerifyOrExit(mNetif.GetMle().IsAttached(), ;);

    while (offset < aMessage.GetLength())
    {
        OT_TOOL_PACKED_BEGIN
        struct
        {
            Tlv tlv;
            uint8_t value[Dataset::kMaxValueSize];
        } OT_TOOL_PACKED_END data;

        aMessage.Read(offset, sizeof(Tlv), &data.tlv);
        aMessage.Read(offset + sizeof(Tlv), data.tlv.GetLength(), data.value);
        mNetwork.Set(data.tlv);
        offset += sizeof(Tlv) + data.tlv.GetLength();
    }

    // add delay timer tlv
    delayTimer.Init();
    delayTimer.SetDelayTimer(mNetif.GetLeader().GetDelayTimerMinimal());
    mNetwork.Set(delayTimer);

    // add pending timestamp tlv
    mNetwork.SetTimestamp(aTimestamp);
    HandleNetworkUpdate(flags);

    // reset delay timer
    ResetDelayTimer(kFlagNetworkUpdated);

exit:
    {}
}

void PendingDatasetBase::HandleNetworkUpdate(uint8_t &aFlags)
{
    DatasetManager::HandleNetworkUpdate(aFlags);
}

void PendingDatasetBase::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDatasetBase *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDatasetBase::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

}  // namespace MeshCoP
}  // namespace Thread
