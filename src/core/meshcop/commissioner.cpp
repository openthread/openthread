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

#include "commissioner.hpp"

#include <stdio.h>
#include "utils/wrap_string.h"

#include <openthread/types.h>
#include <openthread/platform/random.h>

#include "coap/coap_header.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "crypto/pbkdf2_cmac.h"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER

namespace ot {
namespace MeshCoP {

Commissioner::Commissioner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(OT_COMMISSIONER_STATE_DISABLED)
    , mJoinerPort(0)
    , mJoinerRloc(0)
    , mJoinerExpirationTimer(aInstance, HandleJoinerExpirationTimer, this)
    , mTimer(aInstance, HandleTimer, this)
    , mSessionId(0)
    , mTransmitAttempts(0)
    , mRelayReceive(OT_URI_PATH_RELAY_RX, &Commissioner::HandleRelayReceive, this)
    , mDatasetChanged(OT_URI_PATH_DATASET_CHANGED, &Commissioner::HandleDatasetChanged, this)
    , mJoinerFinalize(OT_URI_PATH_JOINER_FINALIZE, &Commissioner::HandleJoinerFinalize, this)
    , mAnnounceBegin(aInstance)
    , mEnergyScan(aInstance)
    , mPanIdQuery(aInstance)
{
    memset(mJoiners, 0, sizeof(mJoiners));
}

void Commissioner::AddCoapResources(void)
{
    ThreadNetif &netif = GetNetif();

    netif.GetCoap().AddResource(mRelayReceive);
    netif.GetCoap().AddResource(mDatasetChanged);
    netif.GetCoapSecure().AddResource(mJoinerFinalize);
}

void Commissioner::RemoveCoapResources(void)
{
    ThreadNetif &netif = GetNetif();

    netif.GetCoap().RemoveResource(mRelayReceive);
    netif.GetCoap().RemoveResource(mDatasetChanged);
    netif.GetCoapSecure().RemoveResource(mJoinerFinalize);
}

otError Commissioner::Start(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = GetNetif().GetCoapSecure().Start(OPENTHREAD_CONFIG_JOINER_UDP_PORT, SendRelayTransmit, this));

    mState            = OT_COMMISSIONER_STATE_PETITION;
    mTransmitAttempts = 0;

    SendPetition();

exit:
    return error;
}

otError Commissioner::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    GetNetif().GetCoapSecure().Stop();

    mState = OT_COMMISSIONER_STATE_DISABLED;
    GetNotifier().SetFlags(OT_CHANGED_COMMISSIONER_STATE);
    RemoveCoapResources();
    ClearJoiners();
    mTransmitAttempts = 0;

    mTimer.Stop();

    GetNetif().GetDtls().Stop();

    SendKeepAlive();

exit:
    return error;
}

otError Commissioner::SendCommissionerSet(void)
{
    otError                error;
    otCommissioningDataset dataset;
    SteeringDataTlv        steeringData;
    Mac::ExtAddress        joinerId;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    memset(&dataset, 0, sizeof(dataset));

    // session id
    dataset.mSessionId      = mSessionId;
    dataset.mIsSessionIdSet = true;

    // compute bloom filter
    steeringData.Init();
    steeringData.Clear();

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
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

        ComputeJoinerId(mJoiners[i].mEui64, joinerId);
        steeringData.ComputeBloomFilter(joinerId);
    }

    // set bloom filter
    memcpy(dataset.mSteeringData.m8, steeringData.GetValue(), steeringData.GetLength());
    dataset.mSteeringData.mLength = steeringData.GetLength();
    dataset.mIsSteeringDataSet    = true;

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

otError Commissioner::AddJoiner(const Mac::ExtAddress *aEui64, const char *aPSKd, uint32_t aTimeout)
{
    otError error = OT_ERROR_NO_BUFS;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(strlen(aPSKd) <= Dtls::kPskMaxLength, error = OT_ERROR_INVALID_ARGS);
    RemoveJoiner(aEui64, 0); // remove immediately

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (mJoiners[i].mValid)
        {
            continue;
        }

        if (aEui64 != NULL)
        {
            memcpy(&mJoiners[i].mEui64, aEui64, sizeof(mJoiners[i].mEui64));
            mJoiners[i].mAny = false;
        }
        else
        {
            mJoiners[i].mAny = true;
        }

        (void)strlcpy(mJoiners[i].mPsk, aPSKd, sizeof(mJoiners[i].mPsk));
        mJoiners[i].mValid          = true;
        mJoiners[i].mExpirationTime = TimerMilli::GetNow() + TimerMilli::SecToMsec(aTimeout);

        UpdateJoinerExpirationTimer();

        SendCommissionerSet();

        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        if (aEui64)
        {
            char logString[Mac::Address::kAddressStringSize];

            otLogInfoMeshCoP(GetInstance(), "Added Joiner (%s, %s)", aEui64->ToString(logString, sizeof(logString)),
                             aPSKd);

            OT_UNUSED_VARIABLE(logString);
        }
        else
        {
            otLogInfoMeshCoP(GetInstance(), "Added Joiner (*, %s)", aPSKd);
        }
    }

    return error;
}

otError Commissioner::RemoveJoiner(const Mac::ExtAddress *aEui64, uint32_t aDelay)
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (aEui64 != NULL)
        {
            if (memcmp(&mJoiners[i].mEui64, aEui64, sizeof(mJoiners[i].mEui64)))
            {
                continue;
            }
        }
        else if (!mJoiners[i].mAny)
        {
            continue;
        }

        if (aDelay > 0)
        {
            uint32_t now = TimerMilli::GetNow();

            if ((static_cast<int32_t>(mJoiners[i].mExpirationTime - now) > 0) &&
                (static_cast<uint32_t>(mJoiners[i].mExpirationTime - now) > TimerMilli::SecToMsec(aDelay)))
            {
                mJoiners[i].mExpirationTime = now + TimerMilli::SecToMsec(aDelay);
                UpdateJoinerExpirationTimer();
            }
        }
        else
        {
            mJoiners[i].mValid = false;
            UpdateJoinerExpirationTimer();
            SendCommissionerSet();
        }

        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        if (aEui64)
        {
            char logString[Mac::Address::kAddressStringSize];

            otLogInfoMeshCoP(GetInstance(), "Removed Joiner (%s)", aEui64->ToString(logString, sizeof(logString)));

            OT_UNUSED_VARIABLE(logString);
        }
        else
        {
            otLogInfoMeshCoP(GetInstance(), "Removed Joiner (*)");
        }
    }

    return error;
}

otError Commissioner::SetProvisioningUrl(const char *aProvisioningUrl)
{
    return GetNetif().GetDtls().mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);
}

uint16_t Commissioner::GetSessionId(void) const
{
    return mSessionId;
}

otCommissionerState Commissioner::GetState(void) const
{
    return mState;
}

void Commissioner::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Commissioner>().HandleTimer();
}

void Commissioner::HandleTimer(void)
{
    switch (mState)
    {
    case OT_COMMISSIONER_STATE_DISABLED:
        break;

    case OT_COMMISSIONER_STATE_PETITION:
        SendPetition();
        break;

    case OT_COMMISSIONER_STATE_ACTIVE:
        SendKeepAlive();
        break;
    }
}

void Commissioner::HandleJoinerExpirationTimer(Timer &aTimer)
{
    aTimer.GetOwner<Commissioner>().HandleJoinerExpirationTimer();
}

void Commissioner::HandleJoinerExpirationTimer(void)
{
    uint32_t now = TimerMilli::GetNow();

    // Remove Joiners.
    for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (static_cast<int32_t>(now - mJoiners[i].mExpirationTime) >= 0)
        {
            otLogDebgMeshCoP(GetInstance(), "removing joiner due to timeout or successfully joined");
            RemoveJoiner(&mJoiners[i].mEui64, 0); // remove immediately
        }
    }

    UpdateJoinerExpirationTimer();
}

void Commissioner::UpdateJoinerExpirationTimer(void)
{
    uint32_t now         = TimerMilli::GetNow();
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

otError Commissioner::SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Header     header;
    Message *        message;
    Ip6::MessageInfo messageInfo;
    MeshCoP::Tlv     tlv;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_COMMISSIONER_GET);

    if (aLength > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    if (aLength > 0)
    {
        tlv.SetType(MeshCoP::Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo,
                                                      Commissioner::HandleMgmtCommissionerGetResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_GET.req to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleMgmtCommissionerGetResponse(void *               aContext,
                                                     otCoapHeader *       aHeader,
                                                     otMessage *          aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissisonerGetResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissisonerGetResponse(Coap::Header *          aHeader,
                                                      Message *               aMessage,
                                                      const Ip6::MessageInfo *aMessageInfo,
                                                      otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == OT_ERROR_NONE && aHeader->GetCode() == OT_COAP_CODE_CHANGED);
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_GET response");

exit:
    return;
}

otError Commissioner::SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                                     const uint8_t *               aTlvs,
                                                     uint8_t                       aLength)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Coap::Header     header;
    Message *        message;
    Ip6::MessageInfo messageInfo;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_COMMISSIONER_SET);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

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

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo,
                                                      Commissioner::HandleMgmtCommissionerSetResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_SET.req to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleMgmtCommissionerSetResponse(void *               aContext,
                                                     otCoapHeader *       aHeader,
                                                     otMessage *          aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissisonerSetResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissisonerSetResponse(Coap::Header *          aHeader,
                                                      Message *               aMessage,
                                                      const Ip6::MessageInfo *aMessageInfo,
                                                      otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == OT_ERROR_NONE && aHeader->GetCode() == OT_COAP_CODE_CHANGED);
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_SET response");

exit:
    return;
}

otError Commissioner::SendPetition(void)
{
    ThreadNetif &     netif = GetNetif();
    otError           error = OT_ERROR_NONE;
    Coap::Header      header;
    Message *         message = NULL;
    Ip6::MessageInfo  messageInfo;
    CommissionerIdTlv commissionerId;

    mTransmitAttempts++;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_LEADER_PETITION);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    commissionerId.Init();
    commissionerId.SetCommissionerId("OpenThread Commissioner");

    SuccessOrExit(error = message->Append(&commissionerId, sizeof(Tlv) + commissionerId.GetLength()));

    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    SuccessOrExit(
        error = netif.GetCoap().SendMessage(*message, messageInfo, Commissioner::HandleLeaderPetitionResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent petition");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleLeaderPetitionResponse(void *               aContext,
                                                otCoapHeader *       aHeader,
                                                otMessage *          aMessage,
                                                const otMessageInfo *aMessageInfo,
                                                otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderPetitionResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderPetitionResponse(Coap::Header *          aHeader,
                                                Message *               aMessage,
                                                const Ip6::MessageInfo *aMessageInfo,
                                                otError                 aResult)
{
    (void)aMessageInfo;

    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;
    bool                     retransmit = false;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_PETITION, mState = OT_COMMISSIONER_STATE_DISABLED);
    VerifyOrExit(aResult == OT_ERROR_NONE && aHeader->GetCode() == OT_COAP_CODE_CHANGED, retransmit = true);

    otLogInfoMeshCoP(GetInstance(), "received Leader Petition response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    VerifyOrExit(state.GetState() == StateTlv::kAccept, mState = OT_COMMISSIONER_STATE_DISABLED);

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
    VerifyOrExit(sessionId.IsValid());
    mSessionId = sessionId.GetCommissionerSessionId();

    AddCoapResources();
    mState = OT_COMMISSIONER_STATE_ACTIVE;

    mTransmitAttempts = 0;
    mTimer.Start(TimerMilli::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (retransmit)
    {
        if (mTransmitAttempts >= kPetitionRetryCount)
        {
            mState = OT_COMMISSIONER_STATE_DISABLED;
        }
        else
        {
            mTimer.Start(TimerMilli::SecToMsec(kPetitionRetryDelay));
        }
    }

    GetNotifier().SetFlags(OT_CHANGED_COMMISSIONER_STATE);
}

otError Commissioner::SendKeepAlive(void)
{
    ThreadNetif &            netif = GetNetif();
    otError                  error = OT_ERROR_NONE;
    Coap::Header             header;
    Message *                message = NULL;
    Ip6::MessageInfo         messageInfo;
    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_LEADER_KEEP_ALIVE);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    state.Init();
    state.SetState(mState == OT_COMMISSIONER_STATE_ACTIVE ? StateTlv::kAccept : StateTlv::kReject);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mSessionId);
    SuccessOrExit(error = message->Append(&sessionId, sizeof(sessionId)));

    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(
        error = netif.GetCoap().SendMessage(*message, messageInfo, Commissioner::HandleLeaderKeepAliveResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent keep alive");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleLeaderKeepAliveResponse(void *               aContext,
                                                 otCoapHeader *       aHeader,
                                                 otMessage *          aMessage,
                                                 const otMessageInfo *aMessageInfo,
                                                 otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderKeepAliveResponse(
        static_cast<Coap::Header *>(aHeader), static_cast<Message *>(aMessage),
        static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderKeepAliveResponse(Coap::Header *          aHeader,
                                                 Message *               aMessage,
                                                 const Ip6::MessageInfo *aMessageInfo,
                                                 otError                 aResult)
{
    (void)aMessageInfo;

    StateTlv state;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, mState = OT_COMMISSIONER_STATE_DISABLED);
    VerifyOrExit(aResult == OT_ERROR_NONE && aHeader->GetCode() == OT_COAP_CODE_CHANGED,
                 mState = OT_COMMISSIONER_STATE_DISABLED);

    otLogInfoMeshCoP(GetInstance(), "received Leader keep-alive response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    VerifyOrExit(state.GetState() == StateTlv::kAccept, mState = OT_COMMISSIONER_STATE_DISABLED);

    mTimer.Start(TimerMilli::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (mState != OT_COMMISSIONER_STATE_ACTIVE)
    {
        RemoveCoapResources();
    }
}

void Commissioner::HandleRelayReceive(void *               aContext,
                                      otCoapHeader *       aHeader,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleRelayReceive(*static_cast<Coap::Header *>(aHeader),
                                                              *static_cast<Message *>(aMessage),
                                                              *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleRelayReceive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &          netif = GetNetif();
    otError                error;
    JoinerUdpPortTlv       joinerPort;
    JoinerIidTlv           joinerIid;
    JoinerRouterLocatorTlv joinerRloc;
    Ip6::MessageInfo       joinerMessageInfo;
    uint16_t               offset;
    uint16_t               length;
    bool                   enableJoiner = false;
    Mac::ExtAddress        joinerId;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aHeader.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRloc), joinerRloc));
    VerifyOrExit(joinerRloc.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));
    VerifyOrExit(length <= aMessage.GetLength() - offset, error = OT_ERROR_PARSE);

    if (!netif.GetCoapSecure().IsConnectionActive())
    {
        memcpy(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid));
        mJoinerIid[0] ^= 0x2;

        for (size_t i = 0; i < sizeof(mJoiners) / sizeof(mJoiners[0]); i++)
        {
            if (!mJoiners[i].mValid)
            {
                continue;
            }

            ComputeJoinerId(mJoiners[i].mEui64, joinerId);

            if (mJoiners[i].mAny || !memcmp(&joinerId, mJoinerIid, sizeof(joinerId)))
            {
                error = netif.GetCoapSecure().SetPsk(reinterpret_cast<const uint8_t *>(mJoiners[i].mPsk),
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

    VerifyOrExit(enableJoiner);

    mJoinerPort = joinerPort.GetUdpPort();
    mJoinerRloc = joinerRloc.GetJoinerRouterLocator();

    otLogInfoMeshCoP(GetInstance(), "Remove Relay Receive (%02x%02x%02x%02x%02x%02x%02x%02x, 0x%04x)", mJoinerIid[0],
                     mJoinerIid[1], mJoinerIid[2], mJoinerIid[3], mJoinerIid[4], mJoinerIid[5], mJoinerIid[6],
                     mJoinerIid[7], mJoinerRloc);

    aMessage.SetOffset(offset);
    SuccessOrExit(error = aMessage.SetLength(offset + length));

    joinerMessageInfo.SetPeerAddr(netif.GetMle().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

    netif.GetCoapSecure().Receive(aMessage, joinerMessageInfo);

exit:
    OT_UNUSED_VARIABLE(aMessageInfo);

    return;
}

void Commissioner::HandleDatasetChanged(void *               aContext,
                                        otCoapHeader *       aHeader,
                                        otMessage *          aMessage,
                                        const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleDatasetChanged(*static_cast<Coap::Header *>(aHeader),
                                                                *static_cast<Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleDatasetChanged(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE && aHeader.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMeshCoP(GetInstance(), "received dataset changed");
    OT_UNUSED_VARIABLE(aMessage);

    SuccessOrExit(GetNetif().GetCoap().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent dataset changed acknowledgment");

exit:
    return;
}

void Commissioner::HandleJoinerFinalize(void *               aContext,
                                        otCoapHeader *       aHeader,
                                        otMessage *          aMessage,
                                        const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerFinalize(*static_cast<Coap::Header *>(aHeader),
                                                                *static_cast<Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleJoinerFinalize(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    StateTlv::State    state = StateTlv::kAccept;
    ProvisioningUrlTlv provisioningUrl;

    otLogInfoMeshCoP(GetInstance(), "received joiner finalize");

    if (Tlv::GetTlv(aMessage, Tlv::kProvisioningUrl, sizeof(provisioningUrl), provisioningUrl) == OT_ERROR_NONE)
    {
        if (provisioningUrl.GetLength() != GetNetif().GetDtls().mProvisioningUrl.GetLength() ||
            memcmp(provisioningUrl.GetProvisioningUrl(), GetNetif().GetDtls().mProvisioningUrl.GetProvisioningUrl(),
                   provisioningUrl.GetLength()) != 0)
        {
            state = StateTlv::kReject;
        }
    }

#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(aMessage.GetLength() <= sizeof(buf));
    aMessage.Read(aHeader.GetLength(), aMessage.GetLength() - aHeader.GetLength(), buf);
    otDumpCertMeshCoP(GetInstance(), "[THCI] direction=recv | type=JOIN_FIN.req |", buf,
                      aMessage.GetLength() - aHeader.GetLength());

exit:
#endif

    SendJoinFinalizeResponse(aHeader, state);
}

void Commissioner::SendJoinFinalizeResponse(const Coap::Header &aRequestHeader, StateTlv::State aState)
{
    ThreadNetif &     netif = GetNetif();
    otError           error = OT_ERROR_NONE;
    Coap::Header      responseHeader;
    Ip6::MessageInfo  joinerMessageInfo;
    MeshCoP::StateTlv stateTlv;
    Message *         message;
    Mac::ExtAddress   extAddr;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure(), responseHeader)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    message->SetSubType(Message::kSubTypeJoinerFinalizeResponse);

    stateTlv.Init();
    stateTlv.SetState(aState);
    SuccessOrExit(error = message->Append(&stateTlv, sizeof(stateTlv)));

    joinerMessageInfo.SetPeerAddr(netif.GetMle().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

#if OPENTHREAD_ENABLE_CERT_LOG
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];
    VerifyOrExit(message->GetLength() <= sizeof(buf));
    message->Read(responseHeader.GetLength(), message->GetLength() - responseHeader.GetLength(), buf);
    otDumpCertMeshCoP(GetInstance(), "[THCI] direction=send | type=JOIN_FIN.rsp |", buf,
                      message->GetLength() - responseHeader.GetLength());
#endif

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, joinerMessageInfo));

    memcpy(extAddr.m8, mJoinerIid, sizeof(extAddr.m8));
    extAddr.SetLocal(!extAddr.IsLocal());
    RemoveJoiner(&extAddr, kRemoveJoinerDelay); // remove after kRemoveJoinerDelay (seconds)

    otLogInfoMeshCoP(GetInstance(), "sent joiner finalize response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError Commissioner::SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Commissioner *>(aContext)->SendRelayTransmit(aMessage, aMessageInfo);
}

otError Commissioner::SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &          netif = GetNetif();
    otError                error = OT_ERROR_NONE;
    Coap::Header           header;
    JoinerUdpPortTlv       udpPort;
    JoinerIidTlv           iid;
    JoinerRouterLocatorTlv rloc;
    ExtendedTlv            tlv;
    Message *              message;
    uint16_t               offset;
    Ip6::MessageInfo       messageInfo;

    OT_UNUSED_VARIABLE(aMessageInfo);

    header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_RELAY_TX);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    udpPort.Init();
    udpPort.SetUdpPort(mJoinerPort);
    SuccessOrExit(error = message->Append(&udpPort, sizeof(udpPort)));

    iid.Init();
    iid.SetIid(mJoinerIid);
    SuccessOrExit(error = message->Append(&iid, sizeof(iid)));

    rloc.Init();
    rloc.SetJoinerRouterLocator(mJoinerRloc);
    SuccessOrExit(error = message->Append(&rloc, sizeof(rloc)));

    if (aMessage.GetSubType() == Message::kSubTypeJoinerFinalizeResponse)
    {
        JoinerRouterKekTlv kek;
        kek.Init();
        kek.SetKek(netif.GetKeyManager().GetKek());
        SuccessOrExit(error = message->Append(&kek, sizeof(kek)));
    }

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength()));
    aMessage.CopyTo(0, offset, aMessage.GetLength(), *message);

    messageInfo.SetPeerAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mJoinerRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    aMessage.Free();

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Commissioner::GeneratePSKc(const char *   aPassPhrase,
                                   const char *   aNetworkName,
                                   const uint8_t *aExtPanId,
                                   uint8_t *      aPSKc)
{
    otError     error      = OT_ERROR_NONE;
    const char *saltPrefix = "Thread";
    uint8_t     salt[OT_PBKDF2_SALT_MAX_LEN];
    uint16_t    saltLen = 0;

    VerifyOrExit((strlen(aPassPhrase) >= OT_COMMISSIONING_PASSPHRASE_MIN_SIZE) &&
                     (strlen(aPassPhrase) <= OT_COMMISSIONING_PASSPHRASE_MAX_SIZE),
                 error = OT_ERROR_INVALID_ARGS);

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

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER
