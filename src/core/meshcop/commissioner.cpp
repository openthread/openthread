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
 *   This file implements a Commissioner role.
 */

#define WPP_NAME "commissioner.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <coap/coap_header.hpp>
#include <common/crc16.hpp>
#include <common/logging.hpp>
#include <meshcop/commissioner.hpp>
#include <meshcop/joiner_router.hpp>
#include <platform/random.h>
#include <thread/meshcop_tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {
namespace MeshCoP {

Commissioner::Commissioner(ThreadNetif &aThreadNetif):
    mAnnounceBegin(aThreadNetif),
    mEnergyScan(aThreadNetif),
    mPanIdQuery(aThreadNetif),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this),
    mTransmitTask(aThreadNetif.GetIp6().mTaskletScheduler, &Commissioner::HandleUdpTransmit, this),
    mSendKek(false),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &Commissioner::HandleRelayReceive, this),
    mDatasetChanged(OPENTHREAD_URI_DATASET_CHANGED, &Commissioner::HandleDatasetChanged, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mNetif(aThreadNetif),
    mIsSendMgmtCommRequest(false)
{
    memset(mJoiners, 0, sizeof(mJoiners));
    mCoapServer.AddResource(mRelayReceive);
    mCoapServer.AddResource(mDatasetChanged);
}

ThreadError Commissioner::Start(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState == kStateDisabled, error = kThreadError_InvalidState);

    SuccessOrExit(error = mSocket.Open(HandleUdpReceive, this));

    mState = kStatePetition;
    mTransmitAttempts = 0;

    SendPetition();

exit:
    return error;
}

ThreadError Commissioner::Stop(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState != kStateDisabled, error = kThreadError_InvalidState);

    mState = kStateDisabled;
    mTransmitAttempts = 0;

    SendKeepAlive();

exit:
    return error;
}

ThreadError Commissioner::SendCommissionerSet(void)
{
    ThreadError error;
    otCommissioningDataset dataset;
    SteeringDataTlv steeringData;

    VerifyOrExit(mState == kStateActive, error = kThreadError_InvalidState);

    memset(&dataset, 0, sizeof(dataset));

    // session id
    dataset.mSessionId = mSessionId;
    dataset.mIsSessionIdSet = true;

    // compute bloom filter
    steeringData.Init();
    steeringData.Clear();

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        Crc16 ccitt(Crc16::kCcitt);
        Crc16 ansi(Crc16::kAnsi);

        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (mJoiners[i].mAny)
        {
            steeringData.SetLength(1);
            steeringData.Set();
            break;
        }

        for (size_t j = 0; j < sizeof(mJoiners[i].mExtAddress.m8); j++)
        {
            uint8_t byte = mJoiners[i].mExtAddress.m8[j];
            ccitt.Update(byte);
            ansi.Update(byte);
        }

        steeringData.SetBit(ccitt.Get() % steeringData.GetNumBits());
        steeringData.SetBit(ansi.Get() % steeringData.GetNumBits());
    }

    // set bloom filter
    memcpy(dataset.mSteeringData.m8, steeringData.GetValue(), steeringData.GetLength());
    dataset.mSteeringData.mLength = steeringData.GetLength();
    dataset.mIsSteeringDataSet = true;

    SuccessOrExit(error = SendMgmtCommissionerSetRequest(dataset, NULL, 0));

exit:
    return error;
}

void Commissioner::ClearJoiners(void)
{
    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        mJoiners[i].mValid = false;
    }

    SendCommissionerSet();
}

ThreadError Commissioner::AddJoiner(const Mac::ExtAddress *aExtAddress, const char *aPSKd)
{
    ThreadError error = kThreadError_NoBufs;

    VerifyOrExit(strlen(aPSKd) <= Dtls::kPskMaxLength, error = kThreadError_InvalidArgs);
    RemoveJoiner(aExtAddress);

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (mJoiners[i].mValid)
        {
            continue;
        }

        if (aExtAddress != NULL)
        {
            mJoiners[i].mExtAddress = *aExtAddress;
            mJoiners[i].mAny = false;
        }
        else
        {
            mJoiners[i].mAny = true;
        }

        strncpy(mJoiners[i].mPsk, aPSKd, sizeof(mJoiners[i].mPsk));
        mJoiners[i].mValid = true;

        SendCommissionerSet();

        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

ThreadError Commissioner::RemoveJoiner(const Mac::ExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_NotFound;

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (aExtAddress != NULL)
        {
            if (memcmp(&mJoiners[i].mExtAddress, &aExtAddress, sizeof(mJoiners[i].mExtAddress)))
            {
                continue;
            }
        }
        else if (!mJoiners[i].mAny)
        {
            continue;
        }

        mJoiners[i].mValid = false;

        SendCommissionerSet();

        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

ThreadError Commissioner::SetProvisioningUrl(const char *aProvisioningUrl)
{
    return mNetif.GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);
}

uint16_t Commissioner::GetSessionId(void) const
{
    return mSessionId;
}

void Commissioner::HandleTimer(void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleTimer();
}

void Commissioner::HandleTimer(void)
{
    switch (mState)
    {
    case kStateDisabled:
        mSocket.Close();
        break;

    case kStatePetition:
        SendPetition();
        break;

    case kStateActive:
        SendKeepAlive();
        break;
    }
}

ThreadError Commissioner::SendMgmtCommissionerGetRequest(const uint8_t *aTlvs,
                                                         uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;
    MeshCoP::Tlv tlv;

    mIsSendMgmtCommRequest = true;

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_GET);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    if (aLength > 0)
    {
        tlv.SetType(MeshCoP::Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent MGMT_COMMISSIONER_GET.req to leader\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        mIsSendMgmtCommRequest = false;
        message->Free();
    }

    return error;
}

ThreadError Commissioner::SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                                         const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    mIsSendMgmtCommRequest = true;

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    if (aDataset.mIsLocatorSet)
    {
        MeshCoP::BorderAgentLocatorTlv locator;
        locator.Init();
        locator.SetBorderAgentLocator(aDataset.mLocator);
        SuccessOrExit(error = message->Append(&locator, sizeof(locator)));
    }

    if (aDataset.mIsSessionIdSet)
    {
        MeshCoP::CommissionerSessionIdTlv sessionId;
        sessionId.Init();
        sessionId.SetCommissionerSessionId(aDataset.mSessionId);
        SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));
    }

    if (aDataset.mIsSteeringDataSet)
    {
        MeshCoP::SteeringDataTlv steeringData;
        steeringData.Init();
        steeringData.SetLength(aDataset.mSteeringData.mLength);
        SuccessOrExit(error = message->Append(&steeringData, sizeof(MeshCoP::Tlv)));
        SuccessOrExit(error = message->Append(&aDataset.mSteeringData.m8, aDataset.mSteeringData.mLength));
    }

    if (aDataset.mIsJoinerUdpPortSet)
    {
        MeshCoP::JoinerUdpPortTlv joinerUdpPort;
        joinerUdpPort.Init();
        joinerUdpPort.SetUdpPort(aDataset.mJoinerUdpPort);
        SuccessOrExit(error = message->Append(&joinerUdpPort, sizeof(joinerUdpPort)));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent MGMT_COMMISSIONER_SET.req to leader\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        mIsSendMgmtCommRequest = false;
        message->Free();
    }

    return error;
}

ThreadError Commissioner::SendPetition(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;
    CommissionerIdTlv commissionerId;

    if (mTransmitAttempts >= kPetitionRetryCount)
    {
        mState = kStateDisabled;
        ExitNow();
    }

    mTimer.Start(Timer::SecToMsec(kPetitionRetryDelay));
    mTransmitAttempts++;

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = otPlatRandomGet() & 0xff;
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_PETITION);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    commissionerId.Init();
    commissionerId.SetCommissionerId("OpenThread Commissioner");

    SuccessOrExit(error = message->Append(&commissionerId, sizeof(Tlv) + commissionerId.GetLength()));

    memset(&messageInfo, 0, sizeof(messageInfo));
    mNetif.GetMle().GetLeaderAloc(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent petition\r\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError Commissioner::SendKeepAlive(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;

    if (mTransmitAttempts >= kPetitionRetryCount)
    {
        mState = kStateDisabled;
        ExitNow();
    }

    mTimer.Start(Timer::SecToMsec(kPetitionRetryDelay));
    mTransmitAttempts++;

    for (size_t i = 0; i < sizeof(mCoapToken); i++)
    {
        mCoapToken[i] = otPlatRandomGet() & 0xff;
    }

    header.Init();
    header.SetType(Coap::Header::kTypeConfirmable);
    header.SetCode(Coap::Header::kCodePost);
    header.SetMessageId(++mCoapMessageId);
    header.SetToken(mCoapToken, sizeof(mCoapToken));
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_KEEP_ALIVE);
    header.Finalize();

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->Append(header.GetBytes(), header.GetLength()));

    state.Init();
    state.SetState(mState == kStateActive ? StateTlv::kAccept : StateTlv::kReject);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mSessionId);
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    mNetif.GetMle().GetLeaderAloc(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
    messageInfo.mPeerPort = kCoapUdpPort;
    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    otLogInfoMeshCoP("sent keep alive\r\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleRelayReceive(void *aContext, Coap::Header &aHeader,
                                      Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleRelayReceive(aHeader, aMessage, aMessageInfo);
}

void Commissioner::HandleRelayReceive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    JoinerUdpPortTlv joinerPort;
    JoinerIidTlv joinerIid;
    JoinerRouterLocatorTlv joinerRloc;
    uint16_t offset;
    uint16_t length;

    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeNonConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodePost, ;);

    otLogInfoMeshCoP("Received relay receive\r\n");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRloc), joinerRloc));
    VerifyOrExit(joinerRloc.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    memcpy(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid));
    mJoinerPort = joinerPort.GetUdpPort();
    mJoinerRloc = joinerRloc.GetJoinerRouterLocator();

    if (!mNetif.GetDtls().IsStarted())
    {
        mJoinerIid[0] ^= 0x2;

        for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
        {
            if (!mJoiners[i].mValid)
            {
                continue;
            }

            if (mJoiners[i].mAny || !memcmp(&mJoiners[i].mExtAddress, mJoinerIid, sizeof(mJoiners[i].mExtAddress)))
            {
                SuccessOrExit(error = mNetif.GetDtls().SetPsk(reinterpret_cast<const uint8_t *>(mJoiners[i].mPsk),
                                                              static_cast<uint8_t>(strlen(mJoiners[i].mPsk))));
                SuccessOrExit(error = mNetif.GetDtls().Start(false, HandleDtlsReceive, HandleDtlsSend, this));
                otLogInfoMeshCoP("found joiner, starting new session\r\n");
                break;
            }
        }

        mJoinerIid[0] ^= 0x2;
    }

    VerifyOrExit(mNetif.GetDtls().IsStarted() && memcmp(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid)) == 0,);

    SuccessOrExit(error = mNetif.GetDtls().SetClientId(mJoinerIid, sizeof(mJoinerIid)));
    mNetif.GetDtls().Receive(aMessage, offset, length);

exit:
    (void)aMessageInfo;
    return;
}

void Commissioner::HandleDatasetChanged(void *aContext, Coap::Header &aHeader,
                                        Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleDatasetChanged(aHeader, aMessage, aMessageInfo);
}

void Commissioner::HandleDatasetChanged(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aHeader.GetType() == Coap::Header::kTypeConfirmable &&
                 aHeader.GetCode() == Coap::Header::kCodePost, ;);

    otLogInfoMeshCoP("received dataset changed\r\n");
    (void)aMessage;

    SendDatasetChangedResponse(aHeader, aMessageInfo);

exit:
    return;
}

void Commissioner::SendDatasetChangedResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    responseHeader.Init();
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));
    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("Sent dataset changed acknowledgment\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void Commissioner::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;

    SuccessOrExit(header.FromMessage(aMessage));
    VerifyOrExit(header.GetType() == Coap::Header::kTypeAcknowledgment &&
                 header.GetCode() == Coap::Header::kCodeChanged &&
                 header.GetMessageId() == mCoapMessageId &&
                 header.GetTokenLength() == sizeof(mCoapToken) &&
                 memcmp(mCoapToken, header.GetToken(), sizeof(mCoapToken)) == 0, ;);
    aMessage.MoveOffset(header.GetLength());

    if (mIsSendMgmtCommRequest)
    {
        mIsSendMgmtCommRequest = false;

        otLogInfoMeshCoP("received MGMT_COMMISSIONER_SET and MGMT_COMMISSIONER_GET response\r\n");

        ExitNow();
    }

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid(), ;);

    VerifyOrExit(state.GetState() == StateTlv::kAccept, mState = kStateDisabled);

    switch (mState)
    {
    case kStateDisabled:
        break;

    case kStatePetition:
        SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
        VerifyOrExit(sessionId.IsValid(), ;);
        mSessionId = sessionId.GetCommissionerSessionId();

        otLogInfoMeshCoP("received petition response\r\n");

        mState = kStateActive;
        mTransmitAttempts = 0;
        mTimer.Start(Timer::SecToMsec(kKeepAliveTimeout) / 2);
        break;

    case kStateActive:
        otLogInfoMeshCoP("received keep alive response\r\n");
        mTransmitAttempts = 0;
        mTimer.Start(Timer::SecToMsec(kKeepAliveTimeout) / 2);
        break;
    }

exit:
    (void)aMessageInfo;
    return;
}

ThreadError Commissioner::HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength)
{
    otLogInfoMeshCoP("Commissioner::HandleDtlsTransmit\r\n");
    return static_cast<Commissioner *>(aContext)->HandleDtlsSend(aBuf, aLength);
}

ThreadError Commissioner::HandleDtlsSend(const unsigned char *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;

    if (mTransmitMessage == NULL)
    {
        Coap::Header header;
        JoinerUdpPortTlv udpPort;
        JoinerIidTlv iid;
        JoinerRouterLocatorTlv rloc;
        ExtendedTlv tlv;

        VerifyOrExit((mTransmitMessage = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

        header.Init();
        header.SetType(Coap::Header::kTypeNonConfirmable);
        header.SetCode(Coap::Header::kCodePost);
        header.SetMessageId(0);
        header.SetToken(NULL, 0);
        header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_TX);
        header.Finalize();
        SuccessOrExit(error = mTransmitMessage->Append(header.GetBytes(), header.GetLength()));

        udpPort.Init();
        udpPort.SetUdpPort(mJoinerPort);
        SuccessOrExit(error = mTransmitMessage->Append(&udpPort, sizeof(udpPort)));

        iid.Init();
        iid.SetIid(mJoinerIid);
        SuccessOrExit(error = mTransmitMessage->Append(&iid, sizeof(iid)));

        rloc.Init();
        rloc.SetJoinerRouterLocator(mJoinerRloc);
        SuccessOrExit(error = mTransmitMessage->Append(&rloc, sizeof(rloc)));

        if (mSendKek)
        {
            JoinerRouterKekTlv kek;
            kek.Init();
            kek.SetKek(mNetif.GetKeyManager().GetKek());
            SuccessOrExit(error = mTransmitMessage->Append(&kek, sizeof(kek)));
        }

        mTransmitMessage->SetOffset(mTransmitMessage->GetLength());

        tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
        tlv.SetLength(0);
        SuccessOrExit(error = mTransmitMessage->Append(&tlv, sizeof(tlv)));
    }

    VerifyOrExit(mTransmitMessage->Append(aBuf, aLength) == kThreadError_None, error = kThreadError_NoBufs);

    mTransmitTask.Post();

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    return error;
}

void Commissioner::HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength)
{
    otLogInfoMeshCoP("Commissioner::HandleDtlsReceive\r\n");
    static_cast<Commissioner *>(aContext)->HandleDtlsReceive(aBuf, aLength);
}

void Commissioner::HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength)
{
    ReceiveJoinerFinalize(aBuf, aLength);
}

void Commissioner::HandleUdpTransmit(void *aContext)
{
    otLogInfoMeshCoP("Commissioner::HandleUdpTransmit\r\n");
    static_cast<Commissioner *>(aContext)->HandleUdpTransmit();
}

void Commissioner::HandleUdpTransmit(void)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;
    ExtendedTlv tlv;

    VerifyOrExit(mTransmitMessage != NULL, error = kThreadError_NoBufs);

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(mTransmitMessage->GetLength() - mTransmitMessage->GetOffset() - sizeof(tlv));
    mTransmitMessage->Write(mTransmitMessage->GetOffset(), sizeof(tlv), &tlv);

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = *mNetif.GetMle().GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mJoinerRloc);
    messageInfo.mPeerPort = kCoapUdpPort;
    messageInfo.mInterfaceId = mNetif.GetInterfaceId();

    SuccessOrExit(error = mSocket.SendTo(*mTransmitMessage, messageInfo));

exit:

    if (error != kThreadError_None && mTransmitMessage != NULL)
    {
        mTransmitMessage->Free();
    }

    mTransmitMessage = NULL;
}

void Commissioner::ReceiveJoinerFinalize(uint8_t *buf, uint16_t length)
{
    Message *message = NULL;
    Coap::Header header;
    char uriPath[16];
    char *curUriPath = uriPath;
    const Coap::Header::Option *coapOption;

    StateTlv::State state = StateTlv::kAccept;
    ProvisioningUrlTlv provisioningUrl;

    otLogInfoMeshCoP("receive joiner finalize 1\r\n");

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL, ;);
    SuccessOrExit(message->Append(buf, length));
    SuccessOrExit(header.FromMessage(*message));
    SuccessOrExit(message->SetOffset(header.GetLength()));

    coapOption = header.GetCurrentOption();

    while (coapOption != NULL)
    {
        switch (coapOption->mNumber)
        {
        case Coap::Header::Option::kOptionUriPath:
            VerifyOrExit(coapOption->mLength < sizeof(uriPath) - static_cast<uint16_t>(curUriPath - uriPath), ;);
            memcpy(curUriPath, coapOption->mValue, coapOption->mLength);
            curUriPath[coapOption->mLength] = '/';
            curUriPath += coapOption->mLength + 1;
            break;

        case Coap::Header::Option::kOptionContentFormat:
            break;

        default:
            ExitNow();
        }

        coapOption = header.GetNextOption();
    }

    if (curUriPath > uriPath)
    {
        curUriPath[-1] = '\0';
    }

    VerifyOrExit(strcmp(uriPath, OPENTHREAD_URI_JOINER_FINALIZE) == 0,);

    if (Tlv::GetTlv(*message, Tlv::kProvisioningUrl, sizeof(provisioningUrl), provisioningUrl) == kThreadError_None)
    {
        if (provisioningUrl.GetLength() != mNetif.GetDtls().mProvisioningUrl.GetLength() ||
            memcmp(provisioningUrl.GetProvisioningUrl(), mNetif.GetDtls().mProvisioningUrl.GetProvisioningUrl(),
                   provisioningUrl.GetLength()) != 0)
        {
            state = StateTlv::kReject;
        }
    }

    otDumpCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.req |", buf + header.GetLength(), length - header.GetLength());

    SendJoinFinalizeResponse(header, state);

exit:

    if (message != NULL)
    {
        message->Free();
    }
}

void Commissioner::SendJoinFinalizeResponse(const Coap::Header &aRequestHeader, StateTlv::State aState)
{
    Coap::Header responseHeader;
    MeshCoP::StateTlv *stateTlv;
    uint8_t buf[128];
    uint8_t *cur = buf;

    responseHeader.Init();
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.Finalize();
    memcpy(cur, responseHeader.GetBytes(), responseHeader.GetLength());
    cur += responseHeader.GetLength();

    stateTlv = reinterpret_cast<MeshCoP::StateTlv *>(cur);
    stateTlv->Init();
    stateTlv->SetState(aState);
    cur += sizeof(*stateTlv);

    mSendKek = true;
    mNetif.GetDtls().Send(buf, static_cast<uint16_t>(cur - buf));
    mSendKek = false;

    otLogInfoMeshCoP("sent joiner finalize response\r\n");
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.rsp\r\n");
}

}  // namespace MeshCoP
}  // namespace Thread
