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

#define WPP_NAME "meshcop_dataset_manager.tmh"

#include <stdio.h>

#include <openthread-types.h>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <coap/coap_header.hpp>
#include <common/debug.hpp>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <common/timer.hpp>
#include <platform/random.h>
#include <platform/radio.h>
#include <thread/meshcop_dataset.hpp>
#include <thread/meshcop_dataset_manager.hpp>
#include <thread/meshcop_tlvs.hpp>
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
    mCoapServer(aThreadNetif.GetCoapServer()),
    mMle(aThreadNetif.GetMle()),
    mNetif(aThreadNetif),
    mNetworkDataLeader(aThreadNetif.GetNetworkDataLeader()),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &DatasetManager::HandleTimer, this),
    mCoapClient(aThreadNetif.GetCoapClient()),
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

    dataset = mMle.IsAttached() ? &mNetwork : &mLocal;

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
            mMle.SetMeshLocalPrefix(prefix->GetMeshLocalPrefix());
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

    switch (mMle.GetDeviceState())
    {
    case Mle::kDeviceStateChild:
    case Mle::kDeviceStateRouter:
        mTimer.Start(1000);
        break;

    case Mle::kDeviceStateLeader:
        mNetwork = mLocal;
        aFlags |= kFlagNetworkUpdated;
        mNetworkDataLeader.IncrementVersion();
        mNetworkDataLeader.IncrementStableVersion();
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
    VerifyOrExit(mMle.IsAttached(),);

    VerifyOrExit(mLocal.Compare(mNetwork) < 0,);

    Register();
    mTimer.Start(1000);

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
        PendingDataset *pending = static_cast<PendingDataset *>(this);
        pending->UpdateDelayTimer();
    }

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = message->Append(mLocal.GetBytes(), mLocal.GetSize()));

    mMle.GetLeaderAloc(leader);

    messageInfo.SetPeerAddr(leader);
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

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
    Timestamp timestamp;
    uint16_t offset = aMessage.GetOffset();
    Tlv::Type type;
    bool isUpdateFromCommissioner = false;
    bool isUpdateAffectConnectivity = false;
    StateTlv::State state = StateTlv::kAccept;

    ActiveTimestampTlv activeTimestamp;
    NetworkMasterKeyTlv masterKey;

    activeTimestamp.SetLength(0);
    masterKey.SetLength(0);

    VerifyOrExit(mMle.GetDeviceState() == Mle::kDeviceStateLeader, state = StateTlv::kReject);

    type = (strcmp(mUriSet, OPENTHREAD_URI_ACTIVE_SET) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp);

    while (offset < aMessage.GetLength())
    {
        Tlv::Type tlvType;

        aMessage.Read(offset, sizeof(tlv), &tlv);
        tlvType = tlv.GetType();

        if (tlvType == type)
        {
            aMessage.Read(offset + sizeof(Tlv), sizeof(timestamp), &timestamp);
        }

        switch (tlvType)
        {
        case Tlv::kActiveTimestamp:
            aMessage.Read(offset, sizeof(activeTimestamp), &activeTimestamp);
            break;

        case Tlv::kNetworkMasterKey:
            aMessage.Read(offset, sizeof(masterKey), &masterKey);
            break;

        default:
            break;
        }

        // verify the request does not include fields that affect connectivity
        if ((type == Tlv::kActiveTimestamp) &&
            (tlvType == Tlv::kChannel || tlvType == Tlv::kMeshLocalPrefix ||
             tlvType == Tlv::kPanId || tlvType == Tlv::kNetworkMasterKey))
        {
            isUpdateAffectConnectivity = true;
        }

        // verify session id is the same
        if (tlvType == Tlv::kCommissionerSessionId)
        {
            uint8_t *cur;
            uint8_t *end;
            uint8_t length;

            isUpdateFromCommissioner = true;
            cur = mNetworkDataLeader.GetCommissioningData(length);
            end = cur + length;

            while (cur < end)
            {
                Tlv *data = reinterpret_cast<Tlv *>(cur);

                if (data->GetType() == Tlv::kCommissionerSessionId)
                {
                    uint16_t sessionId;
                    uint16_t rxSessionId;

                    sessionId = static_cast<CommissionerSessionIdTlv *>(data)->GetCommissionerSessionId();
                    aMessage.Read(offset + sizeof(tlv), sizeof(rxSessionId), &rxSessionId);
                    VerifyOrExit(sessionId == rxSessionId, state = StateTlv::kReject);
                    break;
                }

                cur += sizeof(Tlv) + data->GetLength();
            }
        }

        // verify that TLV data size is less than maximum TLV value size
        VerifyOrExit(tlv.GetLength() <= Dataset::kMaxValueSize, state = StateTlv::kReject);

        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify the update from commissioner should not contain tlv would affect connectivity
    VerifyOrExit(!isUpdateFromCommissioner || !isUpdateAffectConnectivity, state = StateTlv::kReject);

    // verify the request includes a timestamp that is ahead of the locally stored value
    VerifyOrExit(offset == aMessage.GetLength() && (mLocal.GetTimestamp() == NULL ||
                                                    mLocal.GetTimestamp()->Compare(timestamp) > 0), state = StateTlv::kReject);

    // verify network master key if active timestamp is behind
    if (type == Tlv::kPendingTimestamp)
    {
        const Timestamp *localActiveTimestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

        if (localActiveTimestamp != NULL && localActiveTimestamp->Compare(activeTimestamp) <= 0)
        {
            VerifyOrExit(masterKey.GetLength() != 0, state = StateTlv::kReject);
        }
    }

    // verify that does not overflow dataset buffer
    VerifyOrExit((offset - aMessage.GetOffset()) <= Dataset::kMaxSize, state = StateTlv::kReject);

    // update dataset
    if (type == Tlv::kPendingTimestamp && isUpdateFromCommissioner)
    {
        mLocal.Clear(true);
        mLocal.Set(mNetif.GetActiveDataset().GetNetwork());
    }

    if (!isUpdateAffectConnectivity)
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

            if (data.tlv.GetType() != Tlv::kCommissionerSessionId)
            {
                mLocal.Set(data.tlv);
            }

            offset += sizeof(Tlv) + data.tlv.GetLength();
        }

        mLocal.Store();
        mNetwork = mLocal;
        mNetworkDataLeader.IncrementVersion();
        mNetworkDataLeader.IncrementStableVersion();
    }
    else
    {
        mNetif.GetPendingDataset().ApplyActiveDataset(activeTimestamp, aMessage);
    }

    // notify commissioner if update is from thread device
    if (!isUpdateFromCommissioner)
    {
        uint8_t *cur;
        uint8_t *end;
        uint8_t length;
        uint16_t locator = 0xffff;
        Ip6::Address destination;

        cur = mNetworkDataLeader.GetCommissioningData(length);
        VerifyOrExit(cur != NULL && length != 0, ;);
        end = cur + length;

        while (cur < end)
        {
            Tlv *data = reinterpret_cast<Tlv *>(cur);

            if (data->GetType() == Tlv::kBorderAgentLocator)
            {
                locator = static_cast<BorderAgentLocatorTlv *>(data)->GetBorderAgentLocator();
                break;
            }

            cur += sizeof(Tlv) + data->GetLength();
        }

        // verify that Border Agent Locator should be available
        VerifyOrExit(locator != 0xffff, ;);

        memset(&destination, 0, sizeof(destination));
        destination = mNetif.GetMle().GetMeshLocal16();
        destination.mFields.m16[4] = HostSwap16(0x0000);
        destination.mFields.m16[5] = HostSwap16(0x00ff);
        destination.mFields.m16[6] = HostSwap16(0xfe00);
        destination.mFields.m16[7] = HostSwap16(locator);

        mNetif.GetLeader().SendDatasetChanged(destination);
    }

exit:

    if (mMle.GetDeviceState() == Mle::kDeviceStateLeader)
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

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

#if OPENTHREAD_ENABLE_COMMISSIONER
    bool isCommissioner;

    isCommissioner = mNetif.GetCommissioner().GetState() != Commissioner::kStateDisabled ? true : false;

    if (isCommissioner)
    {
        CommissionerSessionIdTlv sessionId;
        sessionId.Init();
        sessionId.SetCommissionerSessionId(mNetif.GetCommissioner().GetSessionId());
        SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));
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

    mMle.GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

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
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

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
        mMle.GetLeaderAloc(messageInfo.GetPeerAddr());
    }

    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

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

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

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

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

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

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset get response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

ActiveDataset::ActiveDataset(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, Tlv::kActiveTimestamp, OPENTHREAD_URI_ACTIVE_SET, OPENTHREAD_URI_ACTIVE_GET),
    mResourceGet(OPENTHREAD_URI_ACTIVE_GET, &ActiveDataset::HandleGet, this),
    mResourceSet(OPENTHREAD_URI_ACTIVE_SET, &ActiveDataset::HandleSet, this)
{
    mCoapServer.AddResource(mResourceGet);
}

ThreadError ActiveDataset::Restore(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mLocal.Restore());
    SuccessOrExit(error = DatasetManager::ApplyConfiguration());

exit:
    return error;
}

void ActiveDataset::StartLeader(void)
{
    if (mLocal.GetTimestamp() == NULL)
    {
        otOperationalDataset dataset;

        memset(&dataset, 0, sizeof(dataset));

        // Active Timestamp
        dataset.mActiveTimestamp = 0;
        dataset.mIsActiveTimestampSet = true;

        // Channel
        dataset.mChannel = mNetif.GetMac().GetChannel();
        dataset.mIsChannelSet = true;

        // channelMask
        dataset.mChannelMaskPage0 = kPhySupportedChannelMask;
        dataset.mIsChannelMaskPage0Set = true;

        // Extended PAN ID
        memcpy(dataset.mExtendedPanId.m8, mNetif.GetMac().GetExtendedPanId(), sizeof(dataset.mExtendedPanId));
        dataset.mIsExtendedPanIdSet = true;

        // Mesh-Local Prefix
        memcpy(dataset.mMeshLocalPrefix.m8, mNetif.GetMle().GetMeshLocalPrefix(), sizeof(dataset.mMeshLocalPrefix));
        dataset.mIsMeshLocalPrefixSet = true;

        // Master Key
        const uint8_t *key;
        uint8_t keyLength;
        key = mNetif.GetKeyManager().GetMasterKey(&keyLength);
        memcpy(dataset.mMasterKey.m8, key, keyLength);
        dataset.mIsMasterKeySet = true;

        // Network Name
        const char *name;
        name = mNetif.GetMac().GetNetworkName();
        memcpy(dataset.mNetworkName.m8, name, strlen(name));
        dataset.mIsNetworkNameSet = true;

        // Pan ID
        dataset.mPanId = mNetif.GetMac().GetPanId();
        dataset.mIsPanIdSet = true;

        // PSKc
        memset(dataset.mPSKc.m8, 0, OT_PSKC_MAX_SIZE);
        dataset.mIsPSKcSet = true;

        // Security Policy
        dataset.mSecurityPolicy.mRotationTime = static_cast<uint16_t>(mNetif.GetKeyManager().GetKeyRotation());
        dataset.mSecurityPolicy.mFlags = mNetif.GetKeyManager().GetSecurityPolicyFlags();
        dataset.mIsSecurityPolicySet = true;

        mLocal.Set(dataset);
    }

    mLocal.Store();
    mNetwork = mLocal;
    mCoapServer.AddResource(mResourceSet);
}

void ActiveDataset::StopLeader(void)
{
    mCoapServer.RemoveResource(mResourceSet);
}

ThreadError ActiveDataset::Clear(bool aOnlyClearNetwork)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Clear(flags, aOnlyClearNetwork));

exit:
    return error;
}

ThreadError ActiveDataset::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}

ThreadError ActiveDataset::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    DatasetManager::ApplyConfiguration();

    if (mMle.GetDeviceState() == Mle::kDeviceStateLeader)
    {
        mNetworkDataLeader.IncrementVersion();
        mNetworkDataLeader.IncrementStableVersion();
    }

exit:
    return error;
}

ThreadError ActiveDataset::Set(const Timestamp &aTimestamp, const Message &aMessage,
                               uint16_t aOffset, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength, flags));
    DatasetManager::ApplyConfiguration();

exit:
    return error;
}

void ActiveDataset::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void ActiveDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<ActiveDataset *>(aContext)->HandleSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void ActiveDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    ApplyConfiguration();

exit:
    return;
}

PendingDataset::PendingDataset(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, Tlv::kPendingTimestamp, OPENTHREAD_URI_PENDING_SET, OPENTHREAD_URI_PENDING_GET),
    mResourceGet(OPENTHREAD_URI_PENDING_GET, &PendingDataset::HandleGet, this),
    mResourceSet(OPENTHREAD_URI_PENDING_SET, &PendingDataset::HandleSet, this),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &PendingDataset::HandleTimer, this)
{
    mCoapServer.AddResource(mResourceGet);
}

ThreadError PendingDataset::Restore(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = mLocal.Restore());

    ResetDelayTimer(kFlagLocalUpdated);

exit:
    return error;
}

void PendingDataset::StartLeader(void)
{
    UpdateDelayTimer(mLocal, mLocalTime);
    mLocal.Store();
    mNetwork = mLocal;
    ResetDelayTimer(kFlagNetworkUpdated);

    mCoapServer.AddResource(mResourceSet);
}

void PendingDataset::StopLeader(void)
{
    mCoapServer.RemoveResource(mResourceSet);
}

ThreadError PendingDataset::Clear(bool aOnlyClearNetwork)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Clear(flags, aOnlyClearNetwork));
    ResetDelayTimer(flags);

exit:
    return error;
}

ThreadError PendingDataset::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    ResetDelayTimer(flags);

exit:
    return error;
}

ThreadError PendingDataset::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = DatasetManager::Set(aDataset));
    ResetDelayTimer(kFlagLocalUpdated | kFlagNetworkUpdated);

exit:
    return error;
}

ThreadError PendingDataset::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                uint16_t aOffset, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength, flags));
    ResetDelayTimer(flags);

exit:
    return error;
}

void PendingDataset::HandleGet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager::Get(aHeader, aMessage, aMessageInfo);
}

void PendingDataset::HandleSet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                               const otMessageInfo *aMessageInfo)
{
    static_cast<PendingDataset *>(aContext)->HandleSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void PendingDataset::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SuccessOrExit(DatasetManager::Set(aHeader, aMessage, aMessageInfo));
    ResetDelayTimer(kFlagLocalUpdated | kFlagNetworkUpdated);

exit:
    return;
}

void PendingDataset::ResetDelayTimer(uint8_t aFlags)
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

void PendingDataset::UpdateDelayTimer(void)
{
    UpdateDelayTimer(mLocal, mLocalTime);
    UpdateDelayTimer(mNetwork, mNetworkTime);
}

void PendingDataset::UpdateDelayTimer(Dataset &aDataset, uint32_t &aStartTime)
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

void PendingDataset::HandleTimer(void *aContext)
{
    static_cast<PendingDataset *>(aContext)->HandleTimer();
}

void PendingDataset::HandleTimer(void)
{
    DelayTimerTlv *delayTimer;

    otLogInfoMeshCoP("pending delay timer expired");

    UpdateDelayTimer();
    delayTimer = static_cast<DelayTimerTlv *>(mNetwork.Get(Tlv::kDelayTimer));
    assert(delayTimer != NULL && delayTimer->GetDelayTimer() == 0);

    mNetif.GetActiveDataset().Set(mNetwork);

    Clear(false);
}

void PendingDataset::ApplyActiveDataset(const Timestamp &aTimestamp, Message &aMessage)
{
    uint16_t offset = aMessage.GetOffset();
    DelayTimerTlv delayTimer;
    uint8_t flags;

    VerifyOrExit(mMle.IsAttached(), ;);

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
    delayTimer.SetDelayTimer(Timer::SecToMsec(DelayTimerTlv::kMinDelayTimer));
    mNetwork.Set(delayTimer);

    // add pending timestamp tlv
    mNetwork.SetTimestamp(aTimestamp);
    HandleNetworkUpdate(flags);

    // reset delay timer
    ResetDelayTimer(kFlagNetworkUpdated);

exit:
    {}
}

void PendingDataset::HandleNetworkUpdate(uint8_t &aFlags)
{
    DatasetManager::HandleNetworkUpdate(aFlags);
}

}  // namespace MeshCoP
}  // namespace Thread
