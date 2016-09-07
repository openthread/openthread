/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdio.h>

#include <openthread-types.h>
#include <coap/coap_header.hpp>
#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <common/timer.hpp>
#include <platform/random.h>
#include <thread/meshcop_dataset.hpp>
#include <thread/meshcop_dataset_manager.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {
namespace MeshCoP {

DatasetManager::DatasetManager(ThreadNetif &aThreadNetif, const char *aUriSet, const char *aUriGet):
    mMle(aThreadNetif.GetMle()),
    mNetif(aThreadNetif),
    mNetworkDataLeader(aThreadNetif.GetNetworkDataLeader()),
    mResourceSet(aUriSet, &HandleSet, this),
    mResourceGet(aUriGet, &HandleGet, this),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &HandleTimer, this),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mUriSet(aUriSet),
    mUriGet(aUriGet),
    mCoapServer(aThreadNetif.GetCoapServer())
{
    mCoapServer.AddResource(mResourceSet);
    mCoapServer.AddResource(mResourceGet);
}

ThreadError DatasetManager::Set(const Dataset &aDataset, uint8_t &aFlags)
{
    ThreadError error = kThreadError_None;

    aFlags = 0;

    VerifyOrExit(mNetwork.GetTimestamp().Compare(aDataset.GetTimestamp()) > 0, error = kThreadError_InvalidArgs);

    mLocal = aDataset;
    aFlags |= kFlagLocalUpdated;

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

ThreadError DatasetManager::Set(const Timestamp &aTimestamp, const Message &aMessage,
                                uint16_t aOffset, uint8_t aLength, uint8_t &aFlags)
{
    ThreadError error = kThreadError_None;
    int compare;

    aFlags = 0;

    SuccessOrExit(error = mNetwork.Set(aMessage, aOffset, aLength));
    mNetwork.SetTimestamp(aTimestamp);
    aFlags |= kFlagNetworkUpdated;

    compare = mLocal.GetTimestamp().Compare(aTimestamp);

    if (compare > 0)
    {
        SuccessOrExit(error = mLocal.Set(aMessage, aOffset, aLength));
        mLocal.SetTimestamp(aTimestamp);
        aFlags |= kFlagLocalUpdated;
    }
    else if (compare < 0)
    {
        mTimer.Start(1000);
    }

exit:
    return error;
}

ThreadError DatasetManager::ApplyLocalToNetwork(void)
{
    mNetwork = mLocal;
    return kThreadError_None;
}

void DatasetManager::HandleTimer(void *aContext)
{
    DatasetManager *obj = static_cast<DatasetManager *>(aContext);
    obj->HandleTimer();
}

void DatasetManager::HandleTimer(void)
{
    VerifyOrExit(mMle.IsAttached() && mNetwork.GetTimestamp().Compare(mLocal.GetTimestamp()) > 0, ;);
    VerifyOrExit(mLocal.Get(Tlv::kDelayTimer) != NULL, ;);

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
    ActiveTimestampTlv timestamp;

    mSocket.Open(&HandleUdpReceive, this);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetVersion(1);
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(mUriSet);
    header.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    timestamp.Init();
    *static_cast<Timestamp *>(&timestamp) = mLocal.GetTimestamp();
    SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    SuccessOrExit(error = message->Append(mLocal.GetBytes(), mLocal.GetSize()));

    mMle.GetLeaderAddress(leader);

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(&messageInfo.mPeerAddr, &leader, sizeof(messageInfo.mPeerAddr));
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset to leader\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void DatasetManager::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    DatasetManager *obj = static_cast<DatasetManager *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void DatasetManager::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;
    (void)aMessageInfo;

    SuccessOrExit(header.FromMessage(aMessage));
    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == mCoapMessageId &&
                 header.GetTokenLength() == sizeof(mCoapToken) &&
                 memcmp(mCoapToken, header.GetToken(), sizeof(mCoapToken)) == 0, ;);

    otLogInfoMeshCoP("received response from leader\n");

exit:
    return;
}

void DatasetManager::HandleSet(void *aContext, Coap::Header &aHeader, Message &aMessage,
                               const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager *obj = static_cast<DatasetManager *>(aContext);
    obj->HandleSet(aHeader, aMessage, aMessageInfo);
}

void DatasetManager::HandleSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Tlv tlv;
    Timestamp timestamp;
    uint16_t offset = aMessage.GetOffset();
    uint8_t type;
    StateTlv::State state = StateTlv::kAccept;

    VerifyOrExit(mMle.GetDeviceState() == Mle::kDeviceStateLeader, state = StateTlv::kReject);

    type = strcmp(mUriSet, OPENTHREAD_URI_ACTIVE_SET) == 0 ? Tlv::kActiveTimestamp : Tlv::kPendingTimestamp;

    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == type)
        {
            aMessage.Read(offset + sizeof(Tlv), sizeof(timestamp), &timestamp);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    // verify the request includes a timestamp that is ahead of the locally stored value
    VerifyOrExit(offset < aMessage.GetLength() && mLocal.GetTimestamp().Compare(timestamp) > 0, state = StateTlv::kReject);

    mLocal.Set(aMessage, aMessage.GetOffset(), static_cast<uint8_t>(aMessage.GetLength() - aMessage.GetOffset()));
    mNetwork = mLocal;
    mNetworkDataLeader.IncrementVersion();
    mNetworkDataLeader.IncrementStableVersion();

exit:

    if (mMle.GetDeviceState() == Mle::kDeviceStateLeader)
    {
        SendSetResponse(aHeader, aMessageInfo, state);
    }

    return;
}

void DatasetManager::HandleGet(void *aContext, Coap::Header &aHeader, Message &aMessage,
                               const Ip6::MessageInfo &aMessageInfo)
{
    DatasetManager *obj = static_cast<DatasetManager *>(aContext);
    obj->HandleGet(aHeader, aMessage, aMessageInfo);
}

void DatasetManager::HandleGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
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

ThreadError DatasetManager::SendSetRequest(const otOperationalDataset &aDataset, const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    mSocket.Open(&HandleUdpReceive, this);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetVersion(1);
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(mUriSet);
    header.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    if (aDataset.mIsActiveTimestampSet)
    {
        ActiveTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds((aDataset.mActiveTimestamp) >> 16);
        static_cast<Timestamp *>(&timestamp)->SetTicks((aDataset.mActiveTimestamp) & 0xffff);
        SuccessOrExit(error = message->Append(&timestamp, sizeof(timestamp)));
    }

    if (aDataset.mIsPendingTimestampSet)
    {
        PendingTimestampTlv timestamp;
        timestamp.Init();
        static_cast<Timestamp *>(&timestamp)->SetSeconds((aDataset.mPendingTimestamp) >> 16);
        static_cast<Timestamp *>(&timestamp)->SetTicks((aDataset.mPendingTimestamp) & 0xffff);
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
        channel.SetChannel(aDataset.mChannel);
        SuccessOrExit(error = message->Append(&channel, sizeof(channel)));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    mMle.GetLeaderAddress(messageInfo.GetPeerAddr());
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset set request to leader\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError DatasetManager::SendGetRequest(const uint8_t *aTlvTypes, const uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;
    Tlv tlv;

    mSocket.Open(&HandleUdpReceive, this);

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetVersion(1);
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(mUriGet);
    header.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    if (aLength > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvTypes, aLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    mMle.GetLeaderAddress(messageInfo.GetPeerAddr());
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent dataset get request to leader\n");

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
    responseHeader.Init();
    responseHeader.SetVersion(1);
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset set response\n");

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
    responseHeader.Init();
    responseHeader.SetVersion(1);
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    if (aLength == 0)
    {
        SuccessOrExit(error = message->Append(mNetwork.GetBytes(), mNetwork.GetSize()));
    }
    else
    {
        for (index = 0; index < aLength; index++)
        {
            if ((tlv = mNetwork.Get(static_cast<Tlv::Type>(aTlvs[index]))) != NULL)
            {
                SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
            }
        }
    }

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent dataset get response\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

ActiveDataset::ActiveDataset(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, OPENTHREAD_URI_ACTIVE_SET, OPENTHREAD_URI_ACTIVE_GET)
{
}

void ActiveDataset::Get(otOperationalDataset &aDataset)
{
    memset(&aDataset, 0, sizeof(aDataset));
    mLocal.Get(aDataset);
    aDataset.mActiveTimestamp = mLocal.GetTimestamp().GetSeconds();
    aDataset.mIsActiveTimestampSet = true;
}

ThreadError ActiveDataset::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    ApplyConfiguration();

exit:
    return error;
}

ThreadError ActiveDataset::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    Dataset dataset;

    SuccessOrExit(error = dataset.Set(aDataset, true));
    SuccessOrExit(error = Set(dataset));

exit:
    return error;
}

ThreadError ActiveDataset::Set(const Timestamp &aTimestamp, const Message &aMessage,
                               uint16_t aOffset, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aTimestamp, aMessage, aOffset, aLength, flags));
    ApplyConfiguration();

exit:
    return error;
}

ThreadError ActiveDataset::ApplyConfiguration(void)
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
            const NetworkNameTlv *extpanid = static_cast<const NetworkNameTlv *>(cur);
            mNetif.GetMac().SetNetworkName(extpanid->GetNetworkName());
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

        default:
        {
            break;
        }
        }

        cur = cur->GetNext();
    }

    return error;
}

PendingDataset::PendingDataset(ThreadNetif &aThreadNetif):
    DatasetManager(aThreadNetif, OPENTHREAD_URI_PENDING_SET, OPENTHREAD_URI_PENDING_GET),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this)
{
}

void PendingDataset::Get(otOperationalDataset &aDataset)
{
    memset(&aDataset, 0, sizeof(aDataset));
    mLocal.Get(aDataset);
    aDataset.mPendingTimestamp = mLocal.GetTimestamp().GetSeconds();
    aDataset.mIsPendingTimestampSet = true;
}

ThreadError PendingDataset::Set(const Dataset &aDataset)
{
    ThreadError error = kThreadError_None;
    uint8_t flags;

    SuccessOrExit(error = DatasetManager::Set(aDataset, flags));
    ResetDelayTimer(flags);

exit:
    return error;
}

ThreadError PendingDataset::Set(const otOperationalDataset &aDataset)
{
    ThreadError error = kThreadError_None;
    Dataset dataset;

    SuccessOrExit(error = dataset.Set(aDataset, false));
    SuccessOrExit(error = Set(dataset));

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

void PendingDataset::ApplyLocalToNetwork(void)
{
    DatasetManager::ApplyLocalToNetwork();
    mNetworkTime = mLocalTime;
}

void PendingDataset::ResetDelayTimer(uint8_t aFlags)
{
    DelayTimerTlv *delayTimer;

    if (aFlags & kFlagLocalUpdated)
    {
        mLocalTime = Timer::GetNow();

        mTimer.Stop();

        if ((delayTimer = static_cast<DelayTimerTlv *>(mLocal.Get(Tlv::kDelayTimer))) != NULL)
        {
            mTimer.Start(delayTimer->GetDelayTimer());
            otLogInfoMeshCoP("delay timer started\n");
        }
    }

    if (aFlags & kFlagNetworkUpdated)
    {
        mNetworkTime = Timer::GetNow();

        // if partition is up to date and delay timer already expired
        if ((mNetwork.GetTimestamp().Compare(mLocal.GetTimestamp()) == 0) &&
            (delayTimer = static_cast<DelayTimerTlv *>(mLocal.Get(Tlv::kDelayTimer))) != NULL &&
            (delayTimer->GetDelayTimer() == 0))
        {
            HandleTimer();
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
    PendingDataset *obj = static_cast<PendingDataset *>(aContext);
    obj->HandleTimer();
}

void PendingDataset::HandleTimer(void)
{
    ActiveTimestampTlv *activeTimestamp;

    otLogInfoMeshCoP("pending delay timer expired\n");

    UpdateDelayTimer();

    // update only if one of the following is true
    // 1) not attached
    // 2) partition's pending dataset is up to date
    VerifyOrExit(!mMle.IsAttached() || mNetwork.GetTimestamp().Compare(mLocal.GetTimestamp()) == 0, ;);

    mLocal.Remove(Tlv::kDelayTimer);

    VerifyOrExit((activeTimestamp = static_cast<ActiveTimestampTlv *>(mLocal.Get(Tlv::kActiveTimestamp))) != NULL,
                 ;);

    mNetif.GetActiveDataset().GetLocal() = mLocal;
    mNetif.GetActiveDataset().GetLocal().SetTimestamp(*activeTimestamp);
    mNetif.GetActiveDataset().GetLocal().Remove(Tlv::kActiveTimestamp);
    mNetif.GetActiveDataset().GetNetwork() = mNetif.GetActiveDataset().GetLocal();
    mNetif.GetActiveDataset().ApplyConfiguration();
    mNetworkDataLeader.IncrementVersion();
    mNetworkDataLeader.IncrementStableVersion();

exit:
    return;
}

}  // namespace MeshCoP
}  // namespace Thread
