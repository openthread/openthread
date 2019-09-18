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

#include "commissioner.hpp"

#include <stdio.h>
#include "utils/wrap_string.h"

#include "coap/coap_message.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "crypto/pbkdf2_cmac.h"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

namespace ot {
namespace MeshCoP {

Commissioner::Commissioner(Instance &aInstance)
    : InstanceLocator(aInstance)
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
    , mStateCallback(NULL)
    , mJoinerCallback(NULL)
    , mCallbackContext(NULL)
    , mState(OT_COMMISSIONER_STATE_DISABLED)
{
    memset(mJoiners, 0, sizeof(mJoiners));

    mCommissionerAloc.mPrefixLength       = 64;
    mCommissionerAloc.mPreferred          = true;
    mCommissionerAloc.mValid              = true;
    mCommissionerAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mCommissionerAloc.mScopeOverrideValid = true;
    mProvisioningUrl.Init();
}

void Commissioner::SetState(otCommissionerState aState)
{
    VerifyOrExit(mState != aState);

    mState = aState;

    if (mStateCallback)
    {
        mStateCallback(mState, mCallbackContext);
    }

exit:
    return;
}

void Commissioner::SignalJoinerEvent(otCommissionerJoinerEvent aEvent, const Mac::ExtAddress &aJoinerId)
{
    if (mJoinerCallback)
    {
        mJoinerCallback(aEvent, &aJoinerId, mCallbackContext);
    }
}

void Commissioner::AddCoapResources(void)
{
    Get<Coap::Coap>().AddResource(mRelayReceive);
    Get<Coap::Coap>().AddResource(mDatasetChanged);
    Get<Coap::CoapSecure>().AddResource(mJoinerFinalize);
}

void Commissioner::RemoveCoapResources(void)
{
    Get<Coap::Coap>().RemoveResource(mRelayReceive);
    Get<Coap::Coap>().RemoveResource(mDatasetChanged);
    Get<Coap::CoapSecure>().RemoveResource(mJoinerFinalize);
}

void Commissioner::HandleCoapsConnected(bool aConnected, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleCoapsConnected(aConnected);
}

void Commissioner::HandleCoapsConnected(bool aConnected)
{
    otCommissionerJoinerEvent event;
    Mac::ExtAddress           joinerId;

    event = aConnected ? OT_COMMISSIONER_JOINER_CONNECTED : OT_COMMISSIONER_JOINER_END;

    joinerId.Set(mJoinerIid);
    joinerId.ToggleLocal();

    SignalJoinerEvent(event, joinerId);
}

otError Commissioner::Start(otCommissionerStateCallback  aStateCallback,
                            otCommissionerJoinerCallback aJoinerCallback,
                            void *                       aCallbackContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mState == OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = Get<Coap::CoapSecure>().Start(SendRelayTransmit, this));
    Get<Coap::CoapSecure>().SetConnectedCallback(&Commissioner::HandleCoapsConnected, this);

    mStateCallback    = aStateCallback;
    mJoinerCallback   = aJoinerCallback;
    mCallbackContext  = aCallbackContext;
    mTransmitAttempts = 0;

    SuccessOrExit(error = SendPetition());
    SetState(OT_COMMISSIONER_STATE_PETITION);

exit:
    return error;
}

otError Commissioner::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState != OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    Get<Coap::CoapSecure>().Stop();

    Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
    RemoveCoapResources();
    ClearJoiners();
    mTransmitAttempts = 0;

    mTimer.Stop();
    Get<Coap::CoapSecure>().Stop();

    SetState(OT_COMMISSIONER_STATE_DISABLED);

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

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
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
    dataset.mSteeringData.mLength = steeringData.GetSteeringDataLength();
    memcpy(dataset.mSteeringData.m8, steeringData.GetValue(), dataset.mSteeringData.mLength);
    dataset.mIsSteeringDataSet = true;

    SuccessOrExit(error = SendMgmtCommissionerSetRequest(dataset, NULL, 0));

exit:
    return error;
}

void Commissioner::ClearJoiners(void)
{
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
    {
        mJoiners[i].mValid = false;
    }

    SendCommissionerSet();
}

otError Commissioner::AddJoiner(const Mac::ExtAddress *aEui64, const char *aPskd, uint32_t aTimeout)
{
    otError error = OT_ERROR_NO_BUFS;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(strlen(aPskd) <= Dtls::kPskMaxLength, error = OT_ERROR_INVALID_ARGS);
    RemoveJoiner(aEui64, 0); // remove immediately

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
    {
        if (mJoiners[i].mValid)
        {
            continue;
        }

        if (aEui64 != NULL)
        {
            mJoiners[i].mEui64 = *aEui64;
            mJoiners[i].mAny   = false;
        }
        else
        {
            mJoiners[i].mAny = true;
        }

        (void)strlcpy(mJoiners[i].mPsk, aPskd, sizeof(mJoiners[i].mPsk));
        mJoiners[i].mValid          = true;
        mJoiners[i].mExpirationTime = TimerMilli::GetNow() + TimerMilli::SecToMsec(aTimeout);

        UpdateJoinerExpirationTimer();

        SendCommissionerSet();

        otLogInfoMeshCoP("Added Joiner (%s, %s)", (aEui64 != NULL) ? aEui64->ToString().AsCString() : "*", aPskd);

        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError Commissioner::GetNextJoinerInfo(uint16_t &aIterator, otJoinerInfo &aJoiner) const
{
    otError error = OT_ERROR_NONE;
    size_t  index;

    for (index = aIterator; index < OT_ARRAY_LENGTH(mJoiners); index++)
    {
        if (!mJoiners[index].mValid)
        {
            continue;
        }

        memset(&aJoiner, 0, sizeof(aJoiner));

        aJoiner.mAny   = mJoiners[index].mAny;
        aJoiner.mEui64 = mJoiners[index].mEui64;
        strlcpy(aJoiner.mPsk, mJoiners[index].mPsk, sizeof(aJoiner.mPsk));
        aJoiner.mExpirationTime = mJoiners[index].mExpirationTime - TimerMilli::GetNow();
        aIterator               = static_cast<uint16_t>(index) + 1;
        ExitNow();
    }

    error = OT_ERROR_NOT_FOUND;

exit:
    return error;
}

otError Commissioner::RemoveJoiner(const Mac::ExtAddress *aEui64, uint32_t aDelay)
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (aEui64 != NULL)
        {
            if (mJoiners[i].mEui64 != *aEui64)
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
            Mac::ExtAddress joinerId;

            mJoiners[i].mValid = false;
            UpdateJoinerExpirationTimer();
            SendCommissionerSet();

            otLogInfoMeshCoP("Removed Joiner (%s)", (aEui64 != NULL) ? aEui64->ToString().AsCString() : "*");

            ComputeJoinerId(mJoiners[i].mEui64, joinerId);
            SignalJoinerEvent(OT_COMMISSIONER_JOINER_REMOVED, joinerId);
        }

        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

const char *Commissioner::GetProvisioningUrl(uint16_t &aLength) const
{
    aLength = mProvisioningUrl.GetLength();

    return mProvisioningUrl.GetProvisioningUrl();
}

otError Commissioner::SetProvisioningUrl(const char *aProvisioningUrl)
{
    otError error = OT_ERROR_NONE;

    if (aProvisioningUrl != NULL)
    {
        size_t len = strnlen(aProvisioningUrl, MeshCoP::ProvisioningUrlTlv::kMaxLength + 1);
        VerifyOrExit(len <= MeshCoP::ProvisioningUrlTlv::kMaxLength, error = OT_ERROR_INVALID_ARGS);
    }

    mProvisioningUrl.SetProvisioningUrl(aProvisioningUrl);

exit:
    return error;
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
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
    {
        if (!mJoiners[i].mValid)
        {
            continue;
        }

        if (static_cast<int32_t>(now - mJoiners[i].mExpirationTime) >= 0)
        {
            otLogDebgMeshCoP("removing joiner due to timeout or successfully joined");
            RemoveJoiner(&mJoiners[i].mEui64, 0); // remove immediately
        }
    }

    UpdateJoinerExpirationTimer();
}

void Commissioner::UpdateJoinerExpirationTimer(void)
{
    uint32_t now         = TimerMilli::GetNow();
    uint32_t nextTimeout = TimerMilli::kForeverDt;

    // Check if timer should be set for next Joiner.
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
    {
        int32_t diff;

        if (!mJoiners[i].mValid)
        {
            continue;
        }

        diff = TimerMilli::Diff(now, mJoiners[i].mExpirationTime);
        if (diff <= 0)
        {
            nextTimeout = 0;
            break;
        }
        else if (static_cast<uint32_t>(diff) < nextTimeout)
        {
            nextTimeout = static_cast<uint32_t>(diff);
        }
    }

    if (nextTimeout != TimerMilli::kForeverDt)
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
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;
    MeshCoP::Tlv     tlv;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_COMMISSIONER_GET));

    if (aLength > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    if (aLength > 0)
    {
        tlv.SetType(MeshCoP::Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleMgmtCommissionerGetResponse, this));

    otLogInfoMeshCoP("sent MGMT_COMMISSIONER_GET.req to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleMgmtCommissionerGetResponse(void *               aContext,
                                                     otMessage *          aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissionerGetResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissionerGetResponse(Coap::Message *         aMessage,
                                                     const Ip6::MessageInfo *aMessageInfo,
                                                     otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED);
    otLogInfoMeshCoP("received MGMT_COMMISSIONER_GET response");

exit:
    return;
}

otError Commissioner::SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                                     const uint8_t *               aTlvs,
                                                     uint8_t                       aLength)
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_COMMISSIONER_SET));
    SuccessOrExit(error = message->SetPayloadMarker());

    if (aDataset.mIsLocatorSet)
    {
        MeshCoP::BorderAgentLocatorTlv locator;
        locator.Init();
        locator.SetBorderAgentLocator(aDataset.mLocator);
        SuccessOrExit(error = message->AppendTlv(locator));
    }

    if (aDataset.mIsSessionIdSet)
    {
        MeshCoP::CommissionerSessionIdTlv sessionId;
        sessionId.Init();
        sessionId.SetCommissionerSessionId(aDataset.mSessionId);
        SuccessOrExit(error = message->AppendTlv(sessionId));
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
        SuccessOrExit(error = message->AppendTlv(joinerUdpPort));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    if (message->GetLength() == message->GetOffset())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleMgmtCommissionerSetResponse, this));

    otLogInfoMeshCoP("sent MGMT_COMMISSIONER_SET.req to leader");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleMgmtCommissionerSetResponse(void *               aContext,
                                                     otMessage *          aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissionerSetResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissionerSetResponse(Coap::Message *         aMessage,
                                                     const Ip6::MessageInfo *aMessageInfo,
                                                     otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED);
    otLogInfoMeshCoP("received MGMT_COMMISSIONER_SET response");

exit:
    return;
}

otError Commissioner::SendPetition(void)
{
    otError           error   = OT_ERROR_NONE;
    Coap::Message *   message = NULL;
    Ip6::MessageInfo  messageInfo;
    CommissionerIdTlv commissionerId;

    mTransmitAttempts++;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_LEADER_PETITION));
    SuccessOrExit(error = message->SetPayloadMarker());

    commissionerId.Init();
    commissionerId.SetCommissionerId("OpenThread Commissioner");

    SuccessOrExit(error = message->AppendTlv(commissionerId));

    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(
        error = Get<Coap::Coap>().SendMessage(*message, messageInfo, Commissioner::HandleLeaderPetitionResponse, this));

    otLogInfoMeshCoP("sent petition");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleLeaderPetitionResponse(void *               aContext,
                                                otMessage *          aMessage,
                                                const otMessageInfo *aMessageInfo,
                                                otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderPetitionResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderPetitionResponse(Coap::Message *         aMessage,
                                                const Ip6::MessageInfo *aMessageInfo,
                                                otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;
    bool                     retransmit = false;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_PETITION, SetState(OT_COMMISSIONER_STATE_DISABLED));
    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED, retransmit = true);

    otLogInfoMeshCoP("received Leader Petition response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    VerifyOrExit(state.GetState() == StateTlv::kAccept, SetState(OT_COMMISSIONER_STATE_DISABLED));

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kCommissionerSessionId, sizeof(sessionId), sessionId));
    VerifyOrExit(sessionId.IsValid());
    mSessionId = sessionId.GetCommissionerSessionId();

    Get<Mle::MleRouter>().GetCommissionerAloc(mCommissionerAloc.GetAddress(), mSessionId);
    Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);

    AddCoapResources();
    SetState(OT_COMMISSIONER_STATE_ACTIVE);

    mTransmitAttempts = 0;
    mTimer.Start(TimerMilli::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (retransmit)
    {
        if (mTransmitAttempts >= kPetitionRetryCount)
        {
            SetState(OT_COMMISSIONER_STATE_DISABLED);
        }
        else
        {
            mTimer.Start(TimerMilli::SecToMsec(kPetitionRetryDelay));
        }
    }
}

otError Commissioner::SendKeepAlive(void)
{
    otError                  error   = OT_ERROR_NONE;
    Coap::Message *          message = NULL;
    Ip6::MessageInfo         messageInfo;
    StateTlv                 state;
    CommissionerSessionIdTlv sessionId;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_LEADER_KEEP_ALIVE));
    SuccessOrExit(error = message->SetPayloadMarker());

    state.Init();
    state.SetState(mState == OT_COMMISSIONER_STATE_ACTIVE ? StateTlv::kAccept : StateTlv::kReject);
    SuccessOrExit(error = message->AppendTlv(state));

    sessionId.Init();
    sessionId.SetCommissionerSessionId(mSessionId);
    SuccessOrExit(error = message->AppendTlv(sessionId));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleLeaderKeepAliveResponse, this));

    otLogInfoMeshCoP("sent keep alive");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Commissioner::HandleLeaderKeepAliveResponse(void *               aContext,
                                                 otMessage *          aMessage,
                                                 const otMessageInfo *aMessageInfo,
                                                 otError              aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderKeepAliveResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderKeepAliveResponse(Coap::Message *         aMessage,
                                                 const Ip6::MessageInfo *aMessageInfo,
                                                 otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    StateTlv state;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, SetState(OT_COMMISSIONER_STATE_DISABLED));
    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED,
                 SetState(OT_COMMISSIONER_STATE_DISABLED));

    otLogInfoMeshCoP("received Leader keep-alive response");

    SuccessOrExit(Tlv::GetTlv(*aMessage, Tlv::kState, sizeof(state), state));
    VerifyOrExit(state.IsValid());

    VerifyOrExit(state.GetState() == StateTlv::kAccept, SetState(OT_COMMISSIONER_STATE_DISABLED));

    mTimer.Start(TimerMilli::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (mState != OT_COMMISSIONER_STATE_ACTIVE)
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        RemoveCoapResources();
    }
}

void Commissioner::HandleRelayReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleRelayReceive(*static_cast<Coap::Message *>(aMessage),
                                                              *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleRelayReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError                error;
    JoinerUdpPortTlv       joinerPort;
    JoinerIidTlv           joinerIid;
    JoinerRouterLocatorTlv joinerRloc;
    Ip6::MessageInfo       joinerMessageInfo;
    uint16_t               offset;
    uint16_t               length;
    bool                   enableJoiner = false;
    Mac::ExtAddress        receivedId;
    Mac::ExtAddress        joinerId;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRloc), joinerRloc));
    VerifyOrExit(joinerRloc.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));
    VerifyOrExit(length <= aMessage.GetLength() - offset, error = OT_ERROR_PARSE);

    if (!Get<Coap::CoapSecure>().IsConnectionActive())
    {
        memcpy(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid));

        receivedId.Set(mJoinerIid);
        receivedId.ToggleLocal();

        for (uint8_t i = 0; i < OT_ARRAY_LENGTH(mJoiners); i++)
        {
            if (!mJoiners[i].mValid)
            {
                continue;
            }

            ComputeJoinerId(mJoiners[i].mEui64, joinerId);

            if (mJoiners[i].mAny || (joinerId == receivedId))
            {
                error = Get<Coap::CoapSecure>().SetPsk(reinterpret_cast<const uint8_t *>(mJoiners[i].mPsk),
                                                       static_cast<uint8_t>(strlen(mJoiners[i].mPsk)));
                SuccessOrExit(error);
                mJoinerIndex = i;
                enableJoiner = true;

                otLogInfoMeshCoP("found joiner, starting new session");
                SignalJoinerEvent(OT_COMMISSIONER_JOINER_START, joinerId);

                break;
            }
        }
    }
    else
    {
        enableJoiner = (memcmp(mJoinerIid, joinerIid.GetIid(), sizeof(mJoinerIid)) == 0);
    }

    VerifyOrExit(enableJoiner);

    mJoinerPort = joinerPort.GetUdpPort();
    mJoinerRloc = joinerRloc.GetJoinerRouterLocator();

    otLogInfoMeshCoP("Remove Relay Receive (%02x%02x%02x%02x%02x%02x%02x%02x, 0x%04x)", mJoinerIid[0], mJoinerIid[1],
                     mJoinerIid[2], mJoinerIid[3], mJoinerIid[4], mJoinerIid[5], mJoinerIid[6], mJoinerIid[7],
                     mJoinerRloc);

    aMessage.SetOffset(offset);
    SuccessOrExit(error = aMessage.SetLength(offset + length));

    joinerMessageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

    Get<Coap::CoapSecure>().HandleUdpReceive(aMessage, joinerMessageInfo);

exit:
    return;
}

void Commissioner::HandleDatasetChanged(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleDatasetChanged(*static_cast<Coap::Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleDatasetChanged(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMeshCoP("received dataset changed");

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    otLogInfoMeshCoP("sent dataset changed acknowledgment");

exit:
    return;
}

void Commissioner::HandleJoinerFinalize(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleJoinerFinalize(*static_cast<Coap::Message *>(aMessage),
                                                                *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleJoinerFinalize(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    StateTlv::State    state = StateTlv::kAccept;
    ProvisioningUrlTlv provisioningUrl;

    otLogInfoMeshCoP("received joiner finalize");

    if (Tlv::GetTlv(aMessage, Tlv::kProvisioningUrl, sizeof(provisioningUrl), provisioningUrl) == OT_ERROR_NONE)
    {
        if (provisioningUrl.GetProvisioningUrlLength() != mProvisioningUrl.GetProvisioningUrlLength() ||
            memcmp(provisioningUrl.GetProvisioningUrl(), mProvisioningUrl.GetProvisioningUrl(),
                   provisioningUrl.GetProvisioningUrlLength()) != 0)
        {
            state = StateTlv::kReject;
        }
    }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (aMessage.GetLength() <= OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE)
    {
        uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

        aMessage.Read(aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset(), buf);
        otDumpCertMeshCoP("[THCI] direction=recv | type=JOIN_FIN.req |", buf,
                          aMessage.GetLength() - aMessage.GetOffset());
    }
#endif

    SendJoinFinalizeResponse(aMessage, state);
}

void Commissioner::SendJoinFinalizeResponse(const Coap::Message &aRequest, StateTlv::State aState)
{
    otError           error = OT_ERROR_NONE;
    Ip6::MessageInfo  joinerMessageInfo;
    MeshCoP::StateTlv stateTlv;
    Coap::Message *   message;
    Mac::ExtAddress   joinerId;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());
    message->SetOffset(message->GetLength());
    message->SetSubType(Message::kSubTypeJoinerFinalizeResponse);

    stateTlv.Init();
    stateTlv.SetState(aState);
    SuccessOrExit(error = message->AppendTlv(stateTlv));

    joinerMessageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(message->GetLength() <= sizeof(buf));
    message->Read(message->GetOffset(), message->GetLength() - message->GetOffset(), buf);
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.rsp |", buf, message->GetLength() - message->GetOffset());
#endif

    SuccessOrExit(error = Get<Coap::CoapSecure>().SendMessage(*message, joinerMessageInfo));

    joinerId.Set(mJoinerIid);
    joinerId.ToggleLocal();
    SignalJoinerEvent(OT_COMMISSIONER_JOINER_FINALIZE, joinerId);

    if (!mJoiners[mJoinerIndex].mAny)
    {
        // remove after kRemoveJoinerDelay (seconds)
        RemoveJoiner(&mJoiners[mJoinerIndex].mEui64, kRemoveJoinerDelay);
    }

    otLogInfoMeshCoP("sent joiner finalize response");

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
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError                error = OT_ERROR_NONE;
    JoinerUdpPortTlv       udpPort;
    JoinerIidTlv           iid;
    JoinerRouterLocatorTlv rloc;
    ExtendedTlv            tlv;
    Coap::Message *        message;
    uint16_t               offset;
    Ip6::MessageInfo       messageInfo;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_RELAY_TX));
    SuccessOrExit(error = message->SetPayloadMarker());

    udpPort.Init();
    udpPort.SetUdpPort(mJoinerPort);
    SuccessOrExit(error = message->AppendTlv(udpPort));

    iid.Init();
    iid.SetIid(mJoinerIid);
    SuccessOrExit(error = message->AppendTlv(iid));

    rloc.Init();
    rloc.SetJoinerRouterLocator(mJoinerRloc);
    SuccessOrExit(error = message->AppendTlv(rloc));

    if (aMessage.GetSubType() == Message::kSubTypeJoinerFinalizeResponse)
    {
        JoinerRouterKekTlv kek;
        kek.Init();
        kek.SetKek(Get<KeyManager>().GetKek());
        SuccessOrExit(error = message->AppendTlv(kek));
    }

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength()));
    aMessage.CopyTo(0, offset, aMessage.GetLength(), *message);

    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mJoinerRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    aMessage.Free();

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Commissioner::GeneratePskc(const char *              aPassPhrase,
                                   const char *              aNetworkName,
                                   const Mac::ExtendedPanId &aExtPanId,
                                   uint8_t *                 aPskc)
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

    memcpy(salt + saltLen, aExtPanId.m8, sizeof(aExtPanId));
    saltLen += OT_EXT_PAN_ID_SIZE;

    memcpy(salt + saltLen, aNetworkName, strlen(aNetworkName));
    saltLen += static_cast<uint16_t>(strlen(aNetworkName));

    otPbkdf2Cmac(reinterpret_cast<const uint8_t *>(aPassPhrase), static_cast<uint16_t>(strlen(aPassPhrase)),
                 reinterpret_cast<const uint8_t *>(salt), saltLen, 16384, OT_PSKC_MAX_SIZE, aPskc);

exit:
    return error;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
