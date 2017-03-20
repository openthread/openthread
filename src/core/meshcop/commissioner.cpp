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

#include "openthread/platform/random.h"

#include <coap/coap_header.hpp>
#include <common/crc16.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <crypto/pbkdf2_cmac.h>
#include <meshcop/commissioner.hpp>
#include <meshcop/joiner_router.hpp>
#include <meshcop/tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap64;

namespace Thread {
namespace MeshCoP {

Commissioner::Commissioner(ThreadNetif &aThreadNetif):
    mAnnounceBegin(aThreadNetif),
    mEnergyScan(aThreadNetif),
    mPanIdQuery(aThreadNetif),
    mState(kStateDisabled),
    mJoinerPort(0),
    mJoinerRloc(0),
    mJoinerExpirationTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleJoinerExpirationTimer, this),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, HandleTimer, this),
    mSessionId(0),
    mTransmitAttempts(0),
    mSendKek(false),
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &Commissioner::HandleRelayReceive, this),
    mDatasetChanged(OPENTHREAD_URI_DATASET_CHANGED, &Commissioner::HandleDatasetChanged, this),
    mJoinerFinalize(OPENTHREAD_URI_JOINER_FINALIZE, &Commissioner::HandleJoinerFinalize, this),
    mNetif(aThreadNetif)
{
    memset(mJoiners, 0, sizeof(mJoiners));
    mNetif.GetCoapServer().AddResource(mRelayReceive);
    mNetif.GetCoapServer().AddResource(mDatasetChanged);
    mNetif.GetSecureCoapServer().AddResource(mJoinerFinalize);
}

otInstance *Commissioner::GetInstance()
{
    return mNetif.GetInstance();
}

ThreadError Commissioner::Start(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();
    VerifyOrExit(mState == kStateDisabled, error = kThreadError_InvalidState);

    SuccessOrExit(error = mNetif.GetSecureCoapServer().Start(SendRelayTransmit, this));

    mState = kStatePetition;
    mTransmitAttempts = 0;
    mSendKek = false;

    SendPetition();

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Commissioner::Stop(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();
    VerifyOrExit(mState != kStateDisabled, error = kThreadError_InvalidState);

    mNetif.GetSecureCoapServer().Stop();

    mState = kStateDisabled;
    mTransmitAttempts = 0;
    mSendKek = false;

    mTimer.Stop();

    mNetif.GetDtls().Stop();

    SendKeepAlive();

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Commissioner::SendCommissionerSet(void)
{
    ThreadError error;
    otCommissioningDataset dataset;
    SteeringDataTlv steeringData;

    otLogFuncEntry();
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
    otLogFuncExitErr(error);
    return error;
}

void Commissioner::ClearJoiners(void)
{
    otLogFuncEntry();

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        mJoiners[i].mValid = false;
    }

    SendCommissionerSet();
    otLogFuncExit();
}

ThreadError Commissioner::AddJoiner(const Mac::ExtAddress *aExtAddress, const char *aPSKd, uint32_t aTimeout)
{
    ThreadError error = kThreadError_NoBufs;

    otLogFuncEntryMsg("%llX, %s", (aExtAddress ? HostSwap64(*reinterpret_cast<const uint64_t *>(aExtAddress)) : 0), aPSKd);
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

        strncpy(mJoiners[i].mPsk, aPSKd, sizeof(mJoiners[i].mPsk) - 1);
        mJoiners[i].mValid = true;
        mJoiners[i].mExpirationTime = Timer::GetNow() + Timer::SecToMsec(aTimeout);

        UpdateJoinerExpirationTimer();

        SendCommissionerSet();

        ExitNow(error = kThreadError_None);
    }

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Commissioner::RemoveJoiner(const Mac::ExtAddress *aExtAddress)
{
    ThreadError error = kThreadError_NotFound;

    otLogFuncEntryMsg("%llX", (aExtAddress ? HostSwap64(*reinterpret_cast<const uint64_t *>(aExtAddress)) : 0));

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (aExtAddress != NULL)
        {
            if (memcmp(&mJoiners[i].mExtAddress, aExtAddress, sizeof(mJoiners[i].mExtAddress)))
            {
                continue;
            }
        }
        else if (!mJoiners[i].mAny)
        {
            continue;
        }

        mJoiners[i].mValid = false;

        UpdateJoinerExpirationTimer();

        SendCommissionerSet();

        ExitNow(error = kThreadError_None);
    }

exit:
    otLogFuncExitErr(error);
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

uint8_t Commissioner::GetState(void) const
{
    return mState;
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
        break;

    case kStatePetition:
        SendPetition();
        break;

    case kStateActive:
        SendKeepAlive();
        break;
    }
}

void Commissioner::HandleJoinerExpirationTimer(void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerExpirationTimer();
}

void Commissioner::HandleJoinerExpirationTimer(void)
{
    uint32_t now = Timer::GetNow();

    // Remove expired Joiners.
    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (static_cast<int32_t>(now - mJoiners[i].mExpirationTime) >= 0)
        {
            otLogDebgMeshCoP(GetInstance(), "removing joiner due to timeout");
            RemoveJoiner(&mJoiners[i].mExtAddress);
        }
    }

    UpdateJoinerExpirationTimer();
}

void Commissioner::UpdateJoinerExpirationTimer(void)
{
    uint32_t now = Timer::GetNow();
    uint32_t nextTimeout = 0xffffffff;

    // Check if timer should be set for next Joiner.
    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        {
            if (!mJoiners[i].mValid)
            {
                continue;
            }

            if (mJoiners[i].mExpirationTime - now < nextTimeout)
            {
                nextTimeout = mJoiners[i].mExpirationTime - now;
            }
        }
    }

    if (nextTimeout != 0xffffffff)
    {
        // Update the timer to the timeout of the next Joiner.
        mJoinerExpirationTimer.Start(nextTimeout);
    }
    else
    {
        // No Joiners, stop the timer.
        mJoinerExpirationTimer.Stop();
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

    otLogFuncEntry();

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_GET);

    if (aLength > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    if (aLength > 0)
    {
        tlv.SetType(MeshCoP::Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo,
                                                             Commissioner::HandleMgmtCommissionerGetResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_GET.req to leader");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void Commissioner::HandleMgmtCommissionerGetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                     const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissisonerGetResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissisonerGetResponse(Coap::Header *aHeader, Message *aMessage,
                                                      const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void) aMessage;
    (void) aMessageInfo;

    otLogFuncEntry();

    VerifyOrExit(aResult == kThreadError_None && aHeader->GetCode() == kCoapResponseChanged, ;);
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_GET response");

exit:
    otLogFuncExit();
}

ThreadError Commissioner::SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                                         const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

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

    if (message->GetLength() == header.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo,
                                                             Commissioner::HandleMgmtCommissionerSetResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_SET.req to leader");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void Commissioner::HandleMgmtCommissionerSetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                     const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissisonerSetResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissisonerSetResponse(Coap::Header *aHeader, Message *aMessage,
                                                      const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void) aMessage;
    (void) aMessageInfo;

    otLogFuncEntry();

    VerifyOrExit(aResult == kThreadError_None && aHeader->GetCode() == kCoapResponseChanged, ;);
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_SET response");

exit:
    otLogFuncExit();
}

ThreadError Commissioner::SendPetition(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;
    CommissionerIdTlv commissionerId;

    otLogFuncEntry();

    mTransmitAttempts++;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_PETITION);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);
    commissionerId.Init();
    commissionerId.SetCommissionerId("OpenThread Commissioner");

    SuccessOrExit(error = message->Append(&commissionerId, sizeof(Tlv) + commissionerId.GetLength()));

    mNetif.GetMle().GetLeaderAloc(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo,
                                                             Commissioner::HandleLeaderPetitionResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent petition");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void Commissioner::HandleLeaderPetitionResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderPetitionResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);

}

void Commissioner::HandleLeaderPetitionResponse(Coap::Header *aHeader, Message *aMessage,
                                                const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void) aMessageInfo;

    StateTlv state;
    CommissionerSessionIdTlv sessionId;
    bool retransmit = false;

    otLogFuncEntry();

    VerifyOrExit(mState == kStatePetition, mState = kStateDisabled);
    VerifyOrExit(aResult == kThreadError_None &&
                 aHeader->GetCode() == kCoapResponseChanged, retransmit = true);

    otLogInfoMeshCoP(GetInstance(), "received Leader Petition response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid(), ;);

    VerifyOrExit(state.GetState() == StateTlv::kAccept, mState = kStateDisabled);

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
    VerifyOrExit(sessionId.IsValid(), ;);
    mSessionId = sessionId.GetCommissionerSessionId();

    mState = kStateActive;
    mTransmitAttempts = 0;
    mTimer.Start(Timer::SecToMsec(kKeepAliveTimeout) / 2);

    SendCommissionerSet();

exit:

    if (retransmit)
    {
        if (mTransmitAttempts >= kPetitionRetryCount)
        {
            mState = kStateDisabled;
        }
        else
        {
            mTimer.Start(Timer::SecToMsec(kPetitionRetryDelay));
        }
    }

    otLogFuncExit();
}

ThreadError Commissioner::SendKeepAlive(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;
    StateTlv state;
    CommissionerSessionIdTlv sessionId;

    otLogFuncEntry();

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_KEEP_ALIVE);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    state.Init();
    state.SetState(mState == kStateActive ? StateTlv::kAccept : StateTlv::kReject);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mSessionId);
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    mNetif.GetMle().GetLeaderAloc(*static_cast<Ip6::Address *>(&messageInfo.mPeerAddr));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo,
                                                             Commissioner::HandleLeaderKeepAliveResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent keep alive");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void Commissioner::HandleLeaderKeepAliveResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                 const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderKeepAliveResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderKeepAliveResponse(Coap::Header *aHeader, Message *aMessage,
                                                 const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void) aMessageInfo;

    StateTlv state;

    otLogFuncEntry();

    VerifyOrExit(mState == kStateActive, mState = kStateDisabled);
    VerifyOrExit(aResult == kThreadError_None &&
                 aHeader->GetCode() == kCoapResponseChanged, mState = kStateDisabled);

    otLogInfoMeshCoP(GetInstance(), "received Leader Petition response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid(), ;);

    VerifyOrExit(state.GetState() == StateTlv::kAccept, mState = kStateDisabled);

    mTimer.Start(Timer::SecToMsec(kKeepAliveTimeout) / 2);

exit:
    otLogFuncExit();
}

void Commissioner::HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleRelayReceive(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleRelayReceive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    JoinerUdpPortTlv joinerPort;
    JoinerIidTlv joinerIid;
    JoinerRouterLocatorTlv joinerRloc;
    Ip6::MessageInfo joinerMessageInfo;
    uint16_t offset;
    uint16_t length;
    bool enableJoiner = false;

    otLogFuncEntry();

    VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, ;);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRloc), joinerRloc));
    VerifyOrExit(joinerRloc.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));
    VerifyOrExit(length <= aMessage.GetLength() - offset, error = kThreadError_Parse);

    if (!mNetif.GetSecureCoapServer().IsConnectionActive())
    {
        memcpy(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid));
        mJoinerIid[0] ^= 0x2;

        for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
        {
            if (!mJoiners[i].mValid)
            {
                continue;
            }

            if (mJoiners[i].mAny || !memcmp(&mJoiners[i].mExtAddress, mJoinerIid, sizeof(mJoiners[i].mExtAddress)))
            {

                error = mNetif.GetSecureCoapServer().SetPsk(reinterpret_cast<const uint8_t *>(mJoiners[i].mPsk),
                                                            static_cast<uint8_t>(strlen(mJoiners[i].mPsk)));
                SuccessOrExit(error);
                otLogInfoMeshCoP(GetInstance(), "found joiner, starting new session");
                enableJoiner = true;
                break;
            }
        }

        mJoinerIid[0] ^= 0x2;
    }
    else
    {
        enableJoiner = (memcmp(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid)) == 0);
    }

    VerifyOrExit(enableJoiner, ;);

    mJoinerPort = joinerPort.GetUdpPort();
    mJoinerRloc = joinerRloc.GetJoinerRouterLocator();

    otLogInfoMeshCoP(GetInstance(), "Received relay receive for %llX, rloc:%x", HostSwap64(mJoinerIid64), mJoinerRloc);

    aMessage.SetOffset(offset);
    SuccessOrExit(error = aMessage.SetLength(offset + length));

    joinerMessageInfo.SetPeerAddr(mNetif.GetMle().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

    mNetif.GetSecureCoapServer().Receive(aMessage, joinerMessageInfo);

exit:
    (void)aMessageInfo;
    otLogFuncExit();
}

void Commissioner::HandleDatasetChanged(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                        const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleDatasetChanged(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleDatasetChanged(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otLogFuncEntry();
    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, ;);

    otLogInfoMeshCoP(GetInstance(), "received dataset changed");
    (void)aMessage;

    SuccessOrExit(mNetif.GetCoapServer().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset changed acknowledgment");

exit:
    otLogFuncExit();
}

void Commissioner::HandleJoinerFinalize(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                        const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerFinalize(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleJoinerFinalize(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    (void)aMessageInfo;
    StateTlv::State state = StateTlv::kAccept;
    ProvisioningUrlTlv provisioningUrl;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received joiner finalize");

    if (Tlv::GetTlv(aMessage, Tlv::kProvisioningUrl, sizeof(provisioningUrl), provisioningUrl) == kThreadError_None)
    {
        if (provisioningUrl.GetLength() != mNetif.GetDtls().mProvisioningUrl.GetLength() ||
            memcmp(provisioningUrl.GetProvisioningUrl(), mNetif.GetDtls().mProvisioningUrl.GetProvisioningUrl(),
                   provisioningUrl.GetLength()) != 0)
        {
            state = StateTlv::kReject;
        }
    }

#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(aMessage.GetLength() <= sizeof(buf), ;);
    aMessage.Read(aHeader.GetLength(), aMessage.GetLength() - aHeader.GetLength(), buf);
    otDumpCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.req |", buf, aMessage.GetLength() - aHeader.GetLength());

exit:
#endif

    SendJoinFinalizeResponse(aHeader, state);

    otLogFuncExit();
}


void Commissioner::SendJoinFinalizeResponse(const Coap::Header &aRequestHeader, StateTlv::State aState)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Ip6::MessageInfo joinerMessageInfo;
    MeshCoP::StateTlv stateTlv;
    Message *message;
    Mac::ExtAddress extAddr;

    otLogFuncEntry();

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetSecureCoapServer().NewMeshCoPMessage(responseHeader)) != NULL,
                 error = kThreadError_NoBufs);

    stateTlv.Init();
    stateTlv.SetState(aState);
    SuccessOrExit(error = message->Append(&stateTlv, sizeof(stateTlv)));

    joinerMessageInfo.SetPeerAddr(mNetif.GetMle().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

    mSendKek = true;
#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(message->GetLength() <= sizeof(buf), ;);
    message->Read(responseHeader.GetLength(), message->GetLength() - responseHeader.GetLength(), buf);
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.rsp |", buf,
                      message->GetLength() - responseHeader.GetLength());
#endif

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SendMessage(*message, joinerMessageInfo));

    memcpy(extAddr.m8, mJoinerIid, sizeof(extAddr.m8));
    extAddr.SetLocal(!extAddr.IsLocal());
    RemoveJoiner(&extAddr);

    otLogInfoMeshCoP(GetInstance(), "sent joiner finalize response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        mSendKek = false;
        message->Free();
    }

    otLogFuncExit();
}

ThreadError Commissioner::SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Commissioner *>(aContext)->SendRelayTransmit(aMessage, aMessageInfo);
}

ThreadError Commissioner::SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    (void)aMessageInfo;
    ThreadError error = kThreadError_None;
    Coap::Header header;
    JoinerUdpPortTlv udpPort;
    JoinerIidTlv iid;
    JoinerRouterLocatorTlv rloc;
    ExtendedTlv tlv;
    Message *message;
    uint16_t offset;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();

    header.Init(kCoapTypeNonConfirmable, kCoapRequestPost);
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_TX);
    header.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    udpPort.Init();
    udpPort.SetUdpPort(mJoinerPort);
    SuccessOrExit(error = message->Append(&udpPort, sizeof(udpPort)));

    iid.Init();
    iid.SetIid(mJoinerIid);
    SuccessOrExit(error = message->Append(&iid, sizeof(iid)));

    rloc.Init();
    rloc.SetJoinerRouterLocator(mJoinerRloc);
    SuccessOrExit(error = message->Append(&rloc, sizeof(rloc)));

    if (mSendKek)
    {
        JoinerRouterKekTlv kek;
        kek.Init();
        kek.SetKek(mNetif.GetKeyManager().GetKek());
        SuccessOrExit(error = message->Append(&kek, sizeof(kek)));
        mSendKek = false;
    }

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    message->SetLength(offset + aMessage.GetLength());
    aMessage.CopyTo(0, offset, aMessage.GetLength(), *message);

    messageInfo.SetPeerAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mJoinerRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    aMessage.Free();

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

ThreadError Commissioner::GeneratePSKc(const char *aPassPhrase, const char *aNetworkName, const uint8_t *aExtPanId,
                                       uint8_t *aPSKc)
{
    ThreadError error = kThreadError_None;
    const char *saltPrefix = "Thread";
    uint8_t salt[OT_PBKDF2_SALT_MAX_LEN];
    uint16_t saltLen = 0;

    VerifyOrExit((strlen(aPassPhrase) >= OT_COMMISSIONING_PASSPHRASE_MIN_SIZE) &&
                 (strlen(aPassPhrase) <= OT_COMMISSIONING_PASSPHRASE_MAX_SIZE), error = kThreadError_InvalidArgs);

    memset(salt, 0, sizeof(salt));
    memcpy(salt, saltPrefix, strlen(saltPrefix));
    saltLen += static_cast<uint16_t>(strlen(saltPrefix));

    memcpy(salt + saltLen, aExtPanId, OT_EXT_PAN_ID_SIZE);
    saltLen += OT_EXT_PAN_ID_SIZE;

    memcpy(salt + saltLen, aNetworkName, strlen(aNetworkName));
    saltLen += static_cast<uint16_t>(strlen(aNetworkName));

    otPbkdf2Cmac(reinterpret_cast<const uint8_t *>(aPassPhrase), static_cast<uint16_t>(strlen(aPassPhrase)),
                 reinterpret_cast<const uint8_t *>(salt), saltLen, 16384, OT_PSKC_MAX_SIZE, aPSKc);

exit:
    return error;
}

}  // namespace MeshCoP
}  // namespace Thread
