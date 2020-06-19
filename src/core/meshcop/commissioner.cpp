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

#include "coap/coap_message.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/string.hpp"
#include "crypto/pbkdf2_cmac.h"
#include "meshcop/joiner.hpp"
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
    , mSessionId(0)
    , mJoinerIndex(0)
    , mTransmitAttempts(0)
    , mJoinerExpirationTimer(aInstance, HandleJoinerExpirationTimer, this)
    , mTimer(aInstance, HandleTimer, this)
    , mRelayReceive(OT_URI_PATH_RELAY_RX, &Commissioner::HandleRelayReceive, this)
    , mDatasetChanged(OT_URI_PATH_DATASET_CHANGED, &Commissioner::HandleDatasetChanged, this)
    , mJoinerFinalize(OT_URI_PATH_JOINER_FINALIZE, &Commissioner::HandleJoinerFinalize, this)
    , mAnnounceBegin(aInstance)
    , mEnergyScan(aInstance)
    , mPanIdQuery(aInstance)
    , mStateCallback(nullptr)
    , mJoinerCallback(nullptr)
    , mCallbackContext(nullptr)
    , mState(OT_COMMISSIONER_STATE_DISABLED)
{
    memset(mJoiners, 0, sizeof(mJoiners));

    mCommissionerAloc.Clear();
    mCommissionerAloc.mPrefixLength       = 64;
    mCommissionerAloc.mPreferred          = true;
    mCommissionerAloc.mValid              = true;
    mCommissionerAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mCommissionerAloc.mScopeOverrideValid = true;

    mProvisioningUrl[0] = '\0';
}

void Commissioner::SetState(otCommissionerState aState)
{
    otCommissionerState oldState = mState;
    OT_UNUSED_VARIABLE(oldState);

    SuccessOrExit(Get<Notifier>().Update(mState, aState, kEventCommissionerStateChanged));

    otLogInfoMeshCoP("CommissionerState: %s -> %s", StateToString(oldState), StateToString(aState));

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

    joinerId.Set(mJoinerIid.m8);
    joinerId.ToggleLocal();

    SignalJoinerEvent(event, joinerId);
}

Commissioner::Joiner *Commissioner::GetUnusedJoinerEntry(void)
{
    Joiner *joiner;

    for (joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (!joiner->mValid)
        {
            ExitNow();
        }
    }

    joiner = nullptr;

exit:
    return joiner;
}

Commissioner::Joiner *Commissioner::FindJoinerEntry(const Mac::ExtAddress *aEui64)
{
    Joiner *joiner;

    for (joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (!joiner->mValid)
        {
            continue;
        }

        if (aEui64 == nullptr)
        {
            if (joiner->mAny)
            {
                ExitNow();
            }
        }
        else
        {
            if (!joiner->mAny && (joiner->mEui64 == *aEui64))
            {
                ExitNow();
            }
        }
    }

    joiner = nullptr;

exit:
    return joiner;
}

Commissioner::Joiner *Commissioner::FindBestMatchingJoinerEntry(const Mac::ExtAddress &aReceivedJoinerId)
{
    Joiner *best = nullptr;

    // Prefer a full Joiner ID match, if not found use the entry
    // accepting any joiner.

    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (!joiner->mValid)
        {
            continue;
        }

        if (!joiner->mAny)
        {
            Mac::ExtAddress joinerId;

            ComputeJoinerId(joiner->mEui64, joinerId);

            if (joinerId == aReceivedJoinerId)
            {
                ExitNow(best = joiner);
            }
        }
        else
        {
            best = joiner;
        }
    }

exit:
    return best;
}

void Commissioner::RemoveJoinerEntry(Commissioner::Joiner &aJoiner)
{
    Mac::ExtAddress joinerId;

    aJoiner.mValid = false;
    UpdateJoinerExpirationTimer();

    SendCommissionerSet();
    LogJoinerEntry("Removed", aJoiner);

    ComputeJoinerId(aJoiner.mEui64, joinerId);
    SignalJoinerEvent(OT_COMMISSIONER_JOINER_REMOVED, joinerId);
}

otError Commissioner::Start(otCommissionerStateCallback  aStateCallback,
                            otCommissionerJoinerCallback aJoinerCallback,
                            void *                       aCallbackContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mState == OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_ALREADY);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    error = Get<MeshCoP::BorderAgent>().Stop();
    VerifyOrExit(error == OT_ERROR_NONE || error == OT_ERROR_ALREADY, OT_NOOP);
#endif

    SuccessOrExit(error = Get<Coap::CoapSecure>().Start(SendRelayTransmit, this));
    Get<Coap::CoapSecure>().SetConnectedCallback(&Commissioner::HandleCoapsConnected, this);

    mStateCallback    = aStateCallback;
    mJoinerCallback   = aJoinerCallback;
    mCallbackContext  = aCallbackContext;
    mTransmitAttempts = 0;

    SuccessOrExit(error = SendPetition());
    SetState(OT_COMMISSIONER_STATE_PETITION);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to start commissioner: %s", otThreadErrorToString(error));
        if (error != OT_ERROR_ALREADY)
        {
            Get<Coap::CoapSecure>().Stop();
        }
    }

    return error;
}

otError Commissioner::Stop(bool aResign)
{
    otError error      = OT_ERROR_NONE;
    bool    needResign = false;

    VerifyOrExit(mState != OT_COMMISSIONER_STATE_DISABLED, error = OT_ERROR_ALREADY);

    Get<Coap::CoapSecure>().Stop();

    if (mState == OT_COMMISSIONER_STATE_ACTIVE)
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        RemoveCoapResources();
        ClearJoiners();
        needResign = true;
    }
    else if (mState == OT_COMMISSIONER_STATE_PETITION)
    {
        mTransmitAttempts = 0;
    }

    mTimer.Stop();

    SetState(OT_COMMISSIONER_STATE_DISABLED);

    if (needResign && aResign)
    {
        SendKeepAlive();
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to stop Commissioner: %s", otThreadErrorToString(error));
    }

    return error;
}

void Commissioner::SendCommissionerSet(void)
{
    otError                error = OT_ERROR_NONE;
    otCommissioningDataset dataset;
    SteeringData &         steeringData = static_cast<SteeringData &>(dataset.mSteeringData);
    Mac::ExtAddress        joinerId;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    memset(&dataset, 0, sizeof(dataset));

    dataset.mSessionId      = mSessionId;
    dataset.mIsSessionIdSet = true;

    // Compute bloom filter
    steeringData.Init();

    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (!joiner->mValid)
        {
            continue;
        }

        if (joiner->mAny)
        {
            steeringData.SetToPermitAllJoiners();
            break;
        }

        ComputeJoinerId(joiner->mEui64, joinerId);
        steeringData.UpdateBloomFilter(joinerId);
    }

    dataset.mIsSteeringDataSet = true;

    error = SendMgmtCommissionerSetRequest(dataset, nullptr, 0);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send MGMT_COMMISSIONER_SET.req: %s", otThreadErrorToString(error));
    }
}

void Commissioner::ClearJoiners(void)
{
    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        joiner->mValid = false;
    }

    SendCommissionerSet();
}

otError Commissioner::AddJoiner(const Mac::ExtAddress *aEui64, const char *aPskd, uint32_t aTimeout)
{
    otError error = OT_ERROR_NONE;
    Joiner *joiner;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(IsPskdValid(aPskd), error = OT_ERROR_INVALID_ARGS);

    joiner = FindJoinerEntry(aEui64);

    if (joiner == nullptr)
    {
        joiner = GetUnusedJoinerEntry();
    }

    VerifyOrExit(joiner != nullptr, error = OT_ERROR_NO_BUFS);

    if (aEui64 != nullptr)
    {
        joiner->mAny   = false;
        joiner->mEui64 = *aEui64;
    }
    else
    {
        joiner->mAny = true;
    }

    strncpy(joiner->mPsk, aPskd, sizeof(joiner->mPsk) - 1);
    joiner->mValid          = true;
    joiner->mExpirationTime = TimerMilli::GetNow() + Time::SecToMsec(aTimeout);

    UpdateJoinerExpirationTimer();

    SendCommissionerSet();

    LogJoinerEntry("Added", *joiner);

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
        strncpy(aJoiner.mPsk, mJoiners[index].mPsk, sizeof(aJoiner.mPsk) - 1);
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
    otError error = OT_ERROR_NONE;
    Joiner *joiner;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    joiner = FindJoinerEntry(aEui64);
    VerifyOrExit(joiner != nullptr, error = OT_ERROR_NOT_FOUND);

    if (aDelay > 0)
    {
        TimeMilli newExpirationTime = TimerMilli::GetNow() + Time::SecToMsec(aDelay);

        if (joiner->mExpirationTime > newExpirationTime)
        {
            joiner->mExpirationTime = newExpirationTime;
            UpdateJoinerExpirationTimer();
        }
    }
    else
    {
        RemoveJoinerEntry(*joiner);
    }

exit:
    return error;
}

otError Commissioner::SetProvisioningUrl(const char *aProvisioningUrl)
{
    otError error = OT_ERROR_NONE;
    uint8_t len;

    if (aProvisioningUrl == nullptr)
    {
        mProvisioningUrl[0] = '\0';
        ExitNow();
    }

    len = static_cast<uint8_t>(StringLength(aProvisioningUrl, sizeof(mProvisioningUrl)));

    VerifyOrExit(len < sizeof(mProvisioningUrl), error = OT_ERROR_INVALID_ARGS);

    memcpy(mProvisioningUrl, aProvisioningUrl, len);
    mProvisioningUrl[len] = '\0';

exit:
    return error;
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
        IgnoreError(SendPetition());
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
    TimeMilli now = TimerMilli::GetNow();

    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (joiner->mValid && (joiner->mExpirationTime <= now))
        {
            otLogDebgMeshCoP("removing joiner due to timeout or successfully joined");
            RemoveJoinerEntry(*joiner);
        }
    }

    UpdateJoinerExpirationTimer();
}

void Commissioner::UpdateJoinerExpirationTimer(void)
{
    TimeMilli now  = TimerMilli::GetNow();
    TimeMilli next = now.GetDistantFuture();

    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (!joiner->mValid)
        {
            continue;
        }

        if (joiner->mExpirationTime <= now)
        {
            next = now;
        }
        else if (joiner->mExpirationTime < next)
        {
            next = joiner->mExpirationTime;
        }
    }

    if (next < now.GetDistantFuture())
    {
        mJoinerExpirationTimer.FireAt(next);
    }
    else
    {
        mJoinerExpirationTimer.Stop();
    }
}

otError Commissioner::SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength)
{
    otError          error = OT_ERROR_NONE;
    Coap::Message *  message;
    Ip6::MessageInfo messageInfo;
    MeshCoP::Tlv     tlv;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != nullptr, error = OT_ERROR_NO_BUFS);

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

    if (error != OT_ERROR_NONE && message != nullptr)
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

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED, OT_NOOP);
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

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_COMMISSIONER_SET));
    SuccessOrExit(error = message->SetPayloadMarker());

    if (aDataset.mIsLocatorSet)
    {
        SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, MeshCoP::Tlv::kBorderAgentLocator, aDataset.mLocator));
    }

    if (aDataset.mIsSessionIdSet)
    {
        SuccessOrExit(error =
                          Tlv::AppendUint16Tlv(*message, MeshCoP::Tlv::kCommissionerSessionId, aDataset.mSessionId));
    }

    if (aDataset.mIsSteeringDataSet)
    {
        SuccessOrExit(error = Tlv::AppendTlv(*message, MeshCoP::Tlv::kSteeringData, aDataset.mSteeringData.m8,
                                             aDataset.mSteeringData.mLength));
    }

    if (aDataset.mIsJoinerUdpPortSet)
    {
        SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerUdpPort, aDataset.mJoinerUdpPort));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->Append(aTlvs, aLength));
    }

    if (message->GetLength() == message->GetOffset())
    {
        // no payload, remove coap payload marker
        IgnoreError(message->SetLength(message->GetLength() - 1));
    }

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleMgmtCommissionerSetResponse, this));

    otLogInfoMeshCoP("sent MGMT_COMMISSIONER_SET.req to leader");

exit:

    if (error != OT_ERROR_NONE && message != nullptr)
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

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED, OT_NOOP);
    otLogInfoMeshCoP("received MGMT_COMMISSIONER_SET response");

exit:
    return;
}

otError Commissioner::SendPetition(void)
{
    otError           error   = OT_ERROR_NONE;
    Coap::Message *   message = nullptr;
    Ip6::MessageInfo  messageInfo;
    CommissionerIdTlv commissionerId;

    mTransmitAttempts++;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_LEADER_PETITION));
    SuccessOrExit(error = message->SetPayloadMarker());

    commissionerId.Init();
    commissionerId.SetCommissionerId("OpenThread Commissioner");

    SuccessOrExit(error = commissionerId.AppendTo(*message));

    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(
        error = Get<Coap::Coap>().SendMessage(*message, messageInfo, Commissioner::HandleLeaderPetitionResponse, this));

    otLogInfoMeshCoP("sent petition");

exit:

    if (error != OT_ERROR_NONE && message != nullptr)
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

    uint8_t state;
    bool    retransmit = false;

    VerifyOrExit(mState != OT_COMMISSIONER_STATE_ACTIVE, OT_NOOP);
    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED,
                 retransmit = (mState == OT_COMMISSIONER_STATE_PETITION));

    otLogInfoMeshCoP("received Leader Petition response");

    SuccessOrExit(Tlv::FindUint8Tlv(*aMessage, Tlv::kState, state));
    VerifyOrExit(state == StateTlv::kAccept, IgnoreError(Stop(/* aResign */ false)));

    SuccessOrExit(Tlv::FindUint16Tlv(*aMessage, Tlv::kCommissionerSessionId, mSessionId));

    // reject this session by sending KeepAlive reject if commissioner is in disabled state
    // this could happen if commissioner is stopped by API during petitioning
    if (mState == OT_COMMISSIONER_STATE_DISABLED)
    {
        SendKeepAlive(mSessionId);
        ExitNow();
    }

    IgnoreError(Get<Mle::MleRouter>().GetCommissionerAloc(mCommissionerAloc.GetAddress(), mSessionId));
    Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);

    AddCoapResources();
    SetState(OT_COMMISSIONER_STATE_ACTIVE);

    mTransmitAttempts = 0;
    mTimer.Start(Time::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (retransmit)
    {
        if (mTransmitAttempts >= kPetitionRetryCount)
        {
            IgnoreError(Stop(/* aResign */ false));
        }
        else
        {
            mTimer.Start(Time::SecToMsec(kPetitionRetryDelay));
        }
    }
}

void Commissioner::SendKeepAlive(void)
{
    SendKeepAlive(mSessionId);
}

void Commissioner::SendKeepAlive(uint16_t aSessionId)
{
    otError          error   = OT_ERROR_NONE;
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_LEADER_KEEP_ALIVE));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(
        error = Tlv::AppendUint8Tlv(*message, Tlv::kState,
                                    (mState == OT_COMMISSIONER_STATE_ACTIVE) ? StateTlv::kAccept : StateTlv::kReject));

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kCommissionerSessionId, aSessionId));

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleLeaderKeepAliveResponse, this));

    otLogInfoMeshCoP("sent keep alive");

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send keep alive: %s", otThreadErrorToString(error));

        if (message != nullptr)
        {
            message->Free();
        }
    }
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

    uint8_t state;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, OT_NOOP);
    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage->GetCode() == OT_COAP_CODE_CHANGED,
                 IgnoreError(Stop(/* aResign */ false)));

    otLogInfoMeshCoP("received Leader keep-alive response");

    SuccessOrExit(Tlv::FindUint8Tlv(*aMessage, Tlv::kState, state));
    VerifyOrExit(state == StateTlv::kAccept, IgnoreError(Stop(/* aResign */ false)));

    mTimer.Start(Time::SecToMsec(kKeepAliveTimeout) / 2);

exit:
    return;
}

void Commissioner::HandleRelayReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Commissioner *>(aContext)->HandleRelayReceive(*static_cast<Coap::Message *>(aMessage),
                                                              *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleRelayReceive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError                  error;
    uint16_t                 joinerPort;
    Ip6::InterfaceIdentifier joinerIid;
    uint16_t                 joinerRloc;
    Ip6::MessageInfo         joinerMessageInfo;
    uint16_t                 offset;
    uint16_t                 length;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(aMessage.IsNonConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, OT_NOOP);

    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kJoinerUdpPort, joinerPort));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kJoinerIid, &joinerIid, sizeof(joinerIid)));
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kJoinerRouterLocator, joinerRloc));

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));
    VerifyOrExit(length <= aMessage.GetLength() - offset, error = OT_ERROR_PARSE);

    if (!Get<Coap::CoapSecure>().IsConnectionActive())
    {
        Mac::ExtAddress receivedId;
        Joiner *        joiner;

        mJoinerIid = joinerIid;

        receivedId.Set(mJoinerIid.m8);
        receivedId.ToggleLocal();

        joiner = FindBestMatchingJoinerEntry(receivedId);
        VerifyOrExit(joiner != nullptr, OT_NOOP);

        SuccessOrExit(error = Get<Coap::CoapSecure>().SetPsk(reinterpret_cast<const uint8_t *>(joiner->mPsk),
                                                             static_cast<uint8_t>(strlen(joiner->mPsk))));
        mJoinerIndex = static_cast<uint8_t>(joiner - mJoiners);

        LogJoinerEntry("Starting new session with", *joiner);
        SignalJoinerEvent(OT_COMMISSIONER_JOINER_START, receivedId);
    }
    else
    {
        VerifyOrExit(mJoinerIid == joinerIid, OT_NOOP);
    }

    mJoinerPort = joinerPort;
    mJoinerRloc = joinerRloc;

    otLogInfoMeshCoP("Remove Relay Receive (%s, 0x%04x)", mJoinerIid.ToString().AsCString(), mJoinerRloc);

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
    VerifyOrExit(aMessage.IsConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, OT_NOOP);

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

    if (Tlv::FindTlv(aMessage, Tlv::kProvisioningUrl, sizeof(provisioningUrl), provisioningUrl) == OT_ERROR_NONE)
    {
        uint8_t len = static_cast<uint8_t>(StringLength(mProvisioningUrl, sizeof(mProvisioningUrl)));

        if ((provisioningUrl.GetProvisioningUrlLength() != len) ||
            !memcmp(provisioningUrl.GetProvisioningUrl(), mProvisioningUrl, len))
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
    otError          error = OT_ERROR_NONE;
    Ip6::MessageInfo joinerMessageInfo;
    Coap::Message *  message;
    Mac::ExtAddress  joinerId;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());
    message->SetOffset(message->GetLength());
    message->SetSubType(Message::kSubTypeJoinerFinalizeResponse);

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, Tlv::kState, static_cast<uint8_t>(aState)));

    joinerMessageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal64());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(message->GetLength() <= sizeof(buf), OT_NOOP);
    message->Read(message->GetOffset(), message->GetLength() - message->GetOffset(), buf);
    otDumpCertMeshCoP("[THCI] direction=send | type=JOIN_FIN.rsp |", buf, message->GetLength() - message->GetOffset());
#endif

    SuccessOrExit(error = Get<Coap::CoapSecure>().SendMessage(*message, joinerMessageInfo));

    joinerId.Set(mJoinerIid.m8);
    joinerId.ToggleLocal();
    SignalJoinerEvent(OT_COMMISSIONER_JOINER_FINALIZE, joinerId);

    if (!mJoiners[mJoinerIndex].mAny)
    {
        // remove after kRemoveJoinerDelay (seconds)
        IgnoreError(RemoveJoiner(&mJoiners[mJoinerIndex].mEui64, kRemoveJoinerDelay));
    }

    otLogInfoMeshCoP("sent joiner finalize response");

exit:

    if (error != OT_ERROR_NONE && message != nullptr)
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

    otError          error = OT_ERROR_NONE;
    ExtendedTlv      tlv;
    Coap::Message *  message;
    uint16_t         offset;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != nullptr, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_RELAY_TX));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerUdpPort, mJoinerPort));
    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kJoinerIid, &mJoinerIid, sizeof(mJoinerIid)));
    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerRouterLocator, mJoinerRloc));

    if (aMessage.GetSubType() == Message::kSubTypeJoinerFinalizeResponse)
    {
        SuccessOrExit(
            error = Tlv::AppendTlv(*message, Tlv::kJoinerRouterKek, Get<KeyManager>().GetKek().GetKey(), Kek::kSize));
    }

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength()));
    aMessage.CopyTo(0, offset, aMessage.GetLength(), *message);

    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().SetLocator(mJoinerRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    aMessage.Free();

exit:

    if (error != OT_ERROR_NONE && message != nullptr)
    {
        message->Free();
    }

    return error;
}

void Commissioner::ApplyMeshLocalPrefix(void)
{
    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, OT_NOOP);

    Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
    mCommissionerAloc.GetAddress().SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);

exit:
    return;
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MESHCOP == 1)

const char *Commissioner::StateToString(otCommissionerState aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case OT_COMMISSIONER_STATE_DISABLED:
        str = "disabled";
        break;
    case OT_COMMISSIONER_STATE_PETITION:
        str = "petition";
        break;
    case OT_COMMISSIONER_STATE_ACTIVE:
        str = "active";
        break;
    default:
        break;
    }

    return str;
}

void Commissioner::LogJoinerEntry(const char *aAction, const Joiner &aJoiner) const
{
    otLogInfoMeshCoP("%s Joiner (%s, %s)", aAction, aJoiner.mAny ? "*" : aJoiner.mEui64.ToString().AsCString(),
                     aJoiner.mPsk);
}

#else

void Commissioner::LogJoinerEntry(const char *, const Joiner &) const
{
}

#endif // (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MESHCOP == 1)

// LCOV_EXCL_STOP

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
