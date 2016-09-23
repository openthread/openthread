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

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>

#include <coap/coap_header.hpp>
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
    mEnergyScan(aThreadNetif),
    mPanIdQuery(aThreadNetif),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this),
    mTransmitTask(aThreadNetif.GetIp6().mTaskletScheduler, &Commissioner::HandleUdpTransmit, this),
    mSendKek(false),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &Commissioner::HandleRelayReceive, this),
    mNetif(aThreadNetif)
{
    aThreadNetif.GetCoapServer().AddResource(mRelayReceive);
}

ThreadError Commissioner::Start(const char *aPSKd, const char *aProvisioningUrl)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mState == kStateDisabled, error = kThreadError_InvalidState);

    SuccessOrExit(error = mNetif.GetDtls().SetPsk(reinterpret_cast<const uint8_t *>(aPSKd),
                                                  static_cast<uint8_t>(strlen(aPSKd))));
    SuccessOrExit(error = mNetif.GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl));
    SuccessOrExit(error = mSocket.Open(HandleUdpReceive, this));
    mState = kStatePetition;
    SendPetition();

exit:
    return error;
}

ThreadError Commissioner::Stop(void)
{
    mState = kStateDisabled;
    SendKeepAlive();
    mTimer.Start(1000);
    return kThreadError_None;
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
        mTimer.Start(5000);
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
    mNetif.GetMle().GetLeaderAddress(messageInfo.GetPeerAddr());
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
    mNetif.GetMle().GetLeaderAddress(messageInfo.GetPeerAddr());
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
    Message *message;
    Ip6::MessageInfo messageInfo;
    CommissionerIdTlv commissionerId;

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
    mNetif.GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
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
    Message *message;
    Ip6::MessageInfo messageInfo;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;

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
    mNetif.GetMle().GetLeaderAddress(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
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
    mNetif.GetDtls().SetClientId(mJoinerIid, sizeof(mJoinerIid));
    mJoinerPort = joinerPort.GetUdpPort();
    mJoinerRloc = joinerRloc.GetJoinerRouterLocator();

    mNetif.GetDtls().Receive(aMessage, offset, length);

exit:
    (void)aMessageInfo;
    return;
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
        mState = kStateActive;
        mTimer.Start(5000);

        mNetif.GetDtls().Start(false, HandleDtlsReceive, HandleDtlsSend, this);

        otLogInfoMeshCoP("received petition response\r\n");
        break;

    case kStateActive:
        otLogInfoMeshCoP("received keep alive response\r\n");
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
}

}  // namespace MeshCoP
}  // namespace Thread
