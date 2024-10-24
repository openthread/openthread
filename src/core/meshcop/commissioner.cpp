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

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Commissioner");

Commissioner::Commissioner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mActiveJoiner(nullptr)
    , mJoinerPort(0)
    , mJoinerRloc(0)
    , mSessionId(0)
    , mTransmitAttempts(0)
    , mJoinerExpirationTimer(aInstance)
    , mTimer(aInstance)
    , mJoinerSessionTimer(aInstance)
    , mAnnounceBegin(aInstance)
    , mEnergyScan(aInstance)
    , mPanIdQuery(aInstance)
    , mState(kStateDisabled)
{
    ClearAllBytes(mJoiners);

    mCommissionerAloc.InitAsThreadOriginMeshLocal();
    mCommissionerAloc.mPreferred = true;

    IgnoreError(SetId("OpenThread Commissioner"));

    mProvisioningUrl[0] = '\0';
}

void Commissioner::SetState(State aState)
{
    State oldState = mState;

    OT_UNUSED_VARIABLE(oldState);

    SuccessOrExit(Get<Notifier>().Update(mState, aState, kEventCommissionerStateChanged));

    LogInfo("State: %s -> %s", StateToString(oldState), StateToString(aState));

    mStateCallback.InvokeIfSet(MapEnum(mState));

exit:
    return;
}

void Commissioner::SignalJoinerEvent(JoinerEvent aEvent, const Joiner *aJoiner) const
{
    otJoinerInfo    joinerInfo;
    Mac::ExtAddress joinerId;
    bool            noJoinerId = false;

    VerifyOrExit(mJoinerCallback.IsSet() && (aJoiner != nullptr));

    aJoiner->CopyToJoinerInfo(joinerInfo);

    if (aJoiner->mType == Joiner::kTypeEui64)
    {
        ComputeJoinerId(aJoiner->mSharedId.mEui64, joinerId);
    }
    else if (aJoiner == mActiveJoiner)
    {
        mJoinerIid.ConvertToExtAddress(joinerId);
    }
    else
    {
        noJoinerId = true;
    }

    mJoinerCallback.Invoke(MapEnum(aEvent), &joinerInfo, noJoinerId ? nullptr : &joinerId);

exit:
    return;
}

void Commissioner::HandleSecureAgentConnectEvent(SecureTransport::ConnectEvent aEvent, void *aContext)
{
    static_cast<Commissioner *>(aContext)->HandleSecureAgentConnectEvent(aEvent);
}

void Commissioner::HandleSecureAgentConnectEvent(SecureTransport::ConnectEvent aEvent)
{
    bool isConnected = (aEvent == SecureTransport::kConnected);
    if (!isConnected)
    {
        mJoinerSessionTimer.Stop();
    }

    SignalJoinerEvent(isConnected ? kJoinerEventConnected : kJoinerEventEnd, mActiveJoiner);
}

Commissioner::Joiner *Commissioner::GetUnusedJoinerEntry(void)
{
    Joiner *rval = nullptr;

    for (Joiner &joiner : mJoiners)
    {
        if (joiner.mType == Joiner::kTypeUnused)
        {
            rval = &joiner;
            break;
        }
    }

    return rval;
}

Commissioner::Joiner *Commissioner::FindJoinerEntry(const Mac::ExtAddress *aEui64)
{
    Joiner *rval = nullptr;

    for (Joiner &joiner : mJoiners)
    {
        switch (joiner.mType)
        {
        case Joiner::kTypeUnused:
        case Joiner::kTypeDiscerner:
            break;

        case Joiner::kTypeAny:
            if (aEui64 == nullptr)
            {
                ExitNow(rval = &joiner);
            }
            break;

        case Joiner::kTypeEui64:
            if ((aEui64 != nullptr) && (joiner.mSharedId.mEui64 == *aEui64))
            {
                ExitNow(rval = &joiner);
            }
            break;
        }
    }

exit:
    return rval;
}

Commissioner::Joiner *Commissioner::FindJoinerEntry(const JoinerDiscerner &aDiscerner)
{
    Joiner *rval = nullptr;

    for (Joiner &joiner : mJoiners)
    {
        if ((joiner.mType == Joiner::kTypeDiscerner) && (aDiscerner == joiner.mSharedId.mDiscerner))
        {
            rval = &joiner;
            break;
        }
    }

    return rval;
}

Commissioner::Joiner *Commissioner::FindBestMatchingJoinerEntry(const Mac::ExtAddress &aReceivedJoinerId)
{
    Joiner         *best = nullptr;
    Mac::ExtAddress joinerId;

    // Prefer a full Joiner ID match, if not found use the entry
    // accepting any joiner.

    for (Joiner &joiner : mJoiners)
    {
        switch (joiner.mType)
        {
        case Joiner::kTypeUnused:
            break;

        case Joiner::kTypeAny:
            if (best == nullptr)
            {
                best = &joiner;
            }
            break;

        case Joiner::kTypeEui64:
            ComputeJoinerId(joiner.mSharedId.mEui64, joinerId);
            if (joinerId == aReceivedJoinerId)
            {
                ExitNow(best = &joiner);
            }
            break;

        case Joiner::kTypeDiscerner:
            if (joiner.mSharedId.mDiscerner.Matches(aReceivedJoinerId))
            {
                if ((best == nullptr) ||
                    ((best->mType == Joiner::kTypeDiscerner) &&
                     (best->mSharedId.mDiscerner.GetLength() < joiner.mSharedId.mDiscerner.GetLength())))
                {
                    best = &joiner;
                }
            }
            break;
        }
    }

exit:
    return best;
}

void Commissioner::RemoveJoinerEntry(Commissioner::Joiner &aJoiner)
{
    // Create a copy of `aJoiner` to use for signaling joiner event
    // and logging after the entry is removed. This ensures the joiner
    // event callback is invoked after all states are cleared.

    Joiner joinerCopy = aJoiner;

    aJoiner.mType = Joiner::kTypeUnused;

    if (&aJoiner == mActiveJoiner)
    {
        mActiveJoiner = nullptr;
    }

    SendCommissionerSet();

    LogJoinerEntry("Removed", joinerCopy);
    SignalJoinerEvent(kJoinerEventRemoved, &joinerCopy);
}

Error Commissioner::Start(StateCallback aStateCallback, JoinerCallback aJoinerCallback, void *aCallbackContext)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(mState == kStateDisabled, error = kErrorAlready);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<BorderAgent>().Stop();
#endif

    SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(SendRelayTransmit, this));
    Get<Tmf::SecureAgent>().SetConnectEventCallback(&Commissioner::HandleSecureAgentConnectEvent, this);

    mStateCallback.Set(aStateCallback, aCallbackContext);
    mJoinerCallback.Set(aJoinerCallback, aCallbackContext);
    mTransmitAttempts = 0;

    SuccessOrExit(error = SendPetition());
    SetState(kStatePetition);

    LogInfo("start commissioner %s", mCommissionerId);

exit:
    if ((error != kErrorNone) && (error != kErrorAlready))
    {
        Get<Tmf::SecureAgent>().Stop();
        LogWarnOnError(error, "start commissioner");
    }

    return error;
}

Error Commissioner::Stop(ResignMode aResignMode)
{
    Error error      = kErrorNone;
    bool  needResign = false;

    VerifyOrExit(mState != kStateDisabled, error = kErrorAlready);

    mJoinerSessionTimer.Stop();
    Get<Tmf::SecureAgent>().Stop();

    if (mState == kStateActive)
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        ClearJoiners();
        needResign = true;
    }
    else if (mState == kStatePetition)
    {
        mTransmitAttempts = 0;
    }

    mTimer.Stop();

    SetState(kStateDisabled);

    if (needResign && (aResignMode == kSendKeepAliveToResign))
    {
        SendKeepAlive();
    }

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<BorderAgent>().Start();
#endif

exit:
    if (error != kErrorAlready)
    {
        LogWarnOnError(error, "stop commissioner");
    }

    return error;
}

Error Commissioner::SetId(const char *aId)
{
    Error error = kErrorNone;

    VerifyOrExit(IsDisabled(), error = kErrorInvalidState);
    error = StringCopy(mCommissionerId, aId, kStringCheckUtf8Encoding);

exit:
    return error;
}

void Commissioner::ComputeBloomFilter(SteeringData &aSteeringData) const
{
    Mac::ExtAddress joinerId;

    aSteeringData.Init();

    for (const Joiner &joiner : mJoiners)
    {
        switch (joiner.mType)
        {
        case Joiner::kTypeUnused:
            break;

        case Joiner::kTypeEui64:
            ComputeJoinerId(joiner.mSharedId.mEui64, joinerId);
            aSteeringData.UpdateBloomFilter(joinerId);
            break;

        case Joiner::kTypeDiscerner:
            aSteeringData.UpdateBloomFilter(joiner.mSharedId.mDiscerner);
            break;

        case Joiner::kTypeAny:
            aSteeringData.SetToPermitAllJoiners();
            ExitNow();
        }
    }

exit:
    return;
}

void Commissioner::SendCommissionerSet(void)
{
    Error                error = kErrorNone;
    CommissioningDataset dataset;

    VerifyOrExit(mState == kStateActive, error = kErrorInvalidState);

    dataset.Clear();

    dataset.SetSessionId(mSessionId);
    ComputeBloomFilter(dataset.UpdateSteeringData());

    error = SendMgmtCommissionerSetRequest(dataset, nullptr, 0);

exit:
    LogWarnOnError(error, "send MGMT_COMMISSIONER_SET.req");
    OT_UNUSED_VARIABLE(error);
}

void Commissioner::ClearJoiners(void)
{
    for (Joiner &joiner : mJoiners)
    {
        joiner.mType = Joiner::kTypeUnused;
    }

    SendCommissionerSet();
}

Error Commissioner::AddJoiner(const Mac::ExtAddress *aEui64,
                              const JoinerDiscerner *aDiscerner,
                              const char            *aPskd,
                              uint32_t               aTimeout)
{
    Error   error = kErrorNone;
    Joiner *joiner;

    VerifyOrExit(mState == kStateActive, error = kErrorInvalidState);

    if (aDiscerner != nullptr)
    {
        VerifyOrExit(aDiscerner->IsValid(), error = kErrorInvalidArgs);
        joiner = FindJoinerEntry(*aDiscerner);
    }
    else
    {
        joiner = FindJoinerEntry(aEui64);
    }

    if (joiner == nullptr)
    {
        joiner = GetUnusedJoinerEntry();
    }

    VerifyOrExit(joiner != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = joiner->mPskd.SetFrom(aPskd));

    if (aDiscerner != nullptr)
    {
        joiner->mType                = Joiner::kTypeDiscerner;
        joiner->mSharedId.mDiscerner = *aDiscerner;
    }
    else if (aEui64 != nullptr)
    {
        joiner->mType            = Joiner::kTypeEui64;
        joiner->mSharedId.mEui64 = *aEui64;
    }
    else
    {
        joiner->mType = Joiner::kTypeAny;
    }

    joiner->mExpirationTime = TimerMilli::GetNow() + Time::SecToMsec(aTimeout);

    mJoinerExpirationTimer.FireAtIfEarlier(joiner->mExpirationTime);

    SendCommissionerSet();

    LogJoinerEntry("Added", *joiner);

exit:
    return error;
}

void Commissioner::Joiner::CopyToJoinerInfo(otJoinerInfo &aJoiner) const
{
    ClearAllBytes(aJoiner);

    switch (mType)
    {
    case kTypeAny:
        aJoiner.mType = OT_JOINER_INFO_TYPE_ANY;
        break;

    case kTypeEui64:
        aJoiner.mType            = OT_JOINER_INFO_TYPE_EUI64;
        aJoiner.mSharedId.mEui64 = mSharedId.mEui64;
        break;

    case kTypeDiscerner:
        aJoiner.mType                = OT_JOINER_INFO_TYPE_DISCERNER;
        aJoiner.mSharedId.mDiscerner = mSharedId.mDiscerner;
        break;

    case kTypeUnused:
        ExitNow();
    }

    aJoiner.mPskd           = mPskd;
    aJoiner.mExpirationTime = mExpirationTime - TimerMilli::GetNow();

exit:
    return;
}

Error Commissioner::GetNextJoinerInfo(uint16_t &aIterator, otJoinerInfo &aJoinerInfo) const
{
    Error error = kErrorNone;

    while (aIterator < GetArrayLength(mJoiners))
    {
        const Joiner &joiner = mJoiners[aIterator++];

        if (joiner.mType != Joiner::kTypeUnused)
        {
            joiner.CopyToJoinerInfo(aJoinerInfo);
            ExitNow();
        }
    }

    error = kErrorNotFound;

exit:
    return error;
}

Error Commissioner::RemoveJoiner(const Mac::ExtAddress *aEui64, const JoinerDiscerner *aDiscerner, uint32_t aDelay)
{
    Error   error = kErrorNone;
    Joiner *joiner;

    VerifyOrExit(mState == kStateActive, error = kErrorInvalidState);

    if (aDiscerner != nullptr)
    {
        VerifyOrExit(aDiscerner->IsValid(), error = kErrorInvalidArgs);
        joiner = FindJoinerEntry(*aDiscerner);
    }
    else
    {
        joiner = FindJoinerEntry(aEui64);
    }

    VerifyOrExit(joiner != nullptr, error = kErrorNotFound);

    RemoveJoiner(*joiner, aDelay);

exit:
    return error;
}

void Commissioner::RemoveJoiner(Joiner &aJoiner, uint32_t aDelay)
{
    if (aDelay > 0)
    {
        TimeMilli newExpirationTime = TimerMilli::GetNow() + Time::SecToMsec(aDelay);

        if (aJoiner.mExpirationTime > newExpirationTime)
        {
            aJoiner.mExpirationTime = newExpirationTime;
            mJoinerExpirationTimer.FireAtIfEarlier(newExpirationTime);
        }
    }
    else
    {
        RemoveJoinerEntry(aJoiner);
    }
}

Error Commissioner::SetProvisioningUrl(const char *aProvisioningUrl)
{
    return StringCopy(mProvisioningUrl, aProvisioningUrl, kStringCheckUtf8Encoding);
}

void Commissioner::HandleTimer(void)
{
    switch (mState)
    {
    case kStateDisabled:
        break;

    case kStatePetition:
        IgnoreError(SendPetition());
        break;

    case kStateActive:
        SendKeepAlive();
        break;
    }
}

void Commissioner::HandleJoinerExpirationTimer(void)
{
    NextFireTime nextTime;

    for (Joiner &joiner : mJoiners)
    {
        if (joiner.mType == Joiner::kTypeUnused)
        {
            continue;
        }

        if (joiner.mExpirationTime <= nextTime.GetNow())
        {
            LogDebg("removing joiner due to timeout or successfully joined");
            RemoveJoinerEntry(joiner);
        }
        else
        {
            nextTime.UpdateIfEarlier(joiner.mExpirationTime);
        }
    }

    mJoinerExpirationTimer.FireAtIfEarlier(nextTime);
}

Error Commissioner::SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength)
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());
    Tlv              tlv;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriCommissionerGet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aLength > 0)
    {
        tlv.SetType(Tlv::kGet);
        tlv.SetLength(aLength);
        SuccessOrExit(error = message->Append(tlv));
        SuccessOrExit(error = message->AppendBytes(aTlvs, aLength));
    }

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleMgmtCommissionerGetResponse, this));

    LogInfo("Sent %s to leader", UriToString<kUriCommissionerGet>());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Commissioner::HandleMgmtCommissionerGetResponse(void                *aContext,
                                                     otMessage           *aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     Error                aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissionerGetResponse(AsCoapMessagePtr(aMessage),
                                                                             AsCoreTypePtr(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissionerGetResponse(Coap::Message          *aMessage,
                                                     const Ip6::MessageInfo *aMessageInfo,
                                                     Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrExit(aResult == kErrorNone && aMessage->GetCode() == Coap::kCodeChanged);
    LogInfo("Received %s response", UriToString<kUriCommissionerGet>());

exit:
    return;
}

Error Commissioner::SendMgmtCommissionerSetRequest(const CommissioningDataset &aDataset,
                                                   const uint8_t              *aTlvs,
                                                   uint8_t                     aLength)
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriCommissionerSet);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aDataset.IsLocatorSet())
    {
        SuccessOrExit(error = Tlv::Append<BorderAgentLocatorTlv>(*message, aDataset.GetLocator()));
    }

    if (aDataset.IsSessionIdSet())
    {
        SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, aDataset.GetSessionId()));
    }

    if (aDataset.IsSteeringDataSet())
    {
        const SteeringData &steeringData = aDataset.GetSteeringData();

        SuccessOrExit(error = Tlv::Append<SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));
    }

    if (aDataset.IsJoinerUdpPortSet())
    {
        SuccessOrExit(error = Tlv::Append<JoinerUdpPortTlv>(*message, aDataset.GetJoinerUdpPort()));
    }

    if (aLength > 0)
    {
        SuccessOrExit(error = message->AppendBytes(aTlvs, aLength));
    }

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleMgmtCommissionerSetResponse, this));

    LogInfo("Sent %s to leader", UriToString<kUriCommissionerSet>());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Commissioner::HandleMgmtCommissionerSetResponse(void                *aContext,
                                                     otMessage           *aMessage,
                                                     const otMessageInfo *aMessageInfo,
                                                     Error                aResult)
{
    static_cast<Commissioner *>(aContext)->HandleMgmtCommissionerSetResponse(AsCoapMessagePtr(aMessage),
                                                                             AsCoreTypePtr(aMessageInfo), aResult);
}

void Commissioner::HandleMgmtCommissionerSetResponse(Coap::Message          *aMessage,
                                                     const Ip6::MessageInfo *aMessageInfo,
                                                     Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error   error;
    uint8_t state;

    SuccessOrExit(error = aResult);
    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged && Tlv::Find<StateTlv>(*aMessage, state) == kErrorNone &&
                     state != StateTlv::kPending,
                 error = kErrorParse);

    OT_UNUSED_VARIABLE(error);
exit:
    LogInfo("Received %s response: %s", UriToString<kUriCommissionerSet>(),
            error == kErrorNone ? StateTlv::StateToString(static_cast<StateTlv::State>(state)) : ErrorToString(error));
}

Error Commissioner::SendPetition(void)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    mTransmitAttempts++;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriLeaderPetition);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<CommissionerIdTlv>(*message, mCommissionerId));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrExit(
        error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, Commissioner::HandleLeaderPetitionResponse, this));

    LogInfo("Sent %s", UriToString<kUriLeaderPetition>());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Commissioner::HandleLeaderPetitionResponse(void                *aContext,
                                                otMessage           *aMessage,
                                                const otMessageInfo *aMessageInfo,
                                                Error                aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderPetitionResponse(AsCoapMessagePtr(aMessage),
                                                                        AsCoreTypePtr(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderPetitionResponse(Coap::Message          *aMessage,
                                                const Ip6::MessageInfo *aMessageInfo,
                                                Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t state;
    bool    retransmit = false;

    VerifyOrExit(mState != kStateActive);
    VerifyOrExit(aResult == kErrorNone && aMessage->GetCode() == Coap::kCodeChanged,
                 retransmit = (mState == kStatePetition));

    LogInfo("Received %s response", UriToString<kUriLeaderPetition>());

    SuccessOrExit(Tlv::Find<StateTlv>(*aMessage, state));
    VerifyOrExit(state == StateTlv::kAccept, IgnoreError(Stop(kDoNotSendKeepAlive)));

    SuccessOrExit(Tlv::Find<CommissionerSessionIdTlv>(*aMessage, mSessionId));

    // reject this session by sending KeepAlive reject if commissioner is in disabled state
    // this could happen if commissioner is stopped by API during petitioning
    if (mState == kStateDisabled)
    {
        SendKeepAlive(mSessionId);
        ExitNow();
    }

    Get<Mle::Mle>().GetCommissionerAloc(mSessionId, mCommissionerAloc.GetAddress());
    Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);

    SetState(kStateActive);

    mTransmitAttempts = 0;
    mTimer.Start(Time::SecToMsec(kKeepAliveTimeout) / 2);

exit:

    if (retransmit)
    {
        if (mTransmitAttempts >= kPetitionRetryCount)
        {
            IgnoreError(Stop(kDoNotSendKeepAlive));
        }
        else
        {
            mTimer.Start(Time::SecToMsec(kPetitionRetryDelay));
        }
    }
}

void Commissioner::SendKeepAlive(void) { SendKeepAlive(mSessionId); }

void Commissioner::SendKeepAlive(uint16_t aSessionId)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriLeaderKeepAlive);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(
        error = Tlv::Append<StateTlv>(*message, (mState == kStateActive) ? StateTlv::kAccept : StateTlv::kReject));

    SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, aSessionId));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo,
                                                        Commissioner::HandleLeaderKeepAliveResponse, this));

    LogInfo("Sent %s", UriToString<kUriLeaderKeepAlive>());

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send keep alive");
}

void Commissioner::HandleLeaderKeepAliveResponse(void                *aContext,
                                                 otMessage           *aMessage,
                                                 const otMessageInfo *aMessageInfo,
                                                 Error                aResult)
{
    static_cast<Commissioner *>(aContext)->HandleLeaderKeepAliveResponse(AsCoapMessagePtr(aMessage),
                                                                         AsCoreTypePtr(aMessageInfo), aResult);
}

void Commissioner::HandleLeaderKeepAliveResponse(Coap::Message          *aMessage,
                                                 const Ip6::MessageInfo *aMessageInfo,
                                                 Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t state;

    VerifyOrExit(mState == kStateActive);
    VerifyOrExit(aResult == kErrorNone && aMessage->GetCode() == Coap::kCodeChanged,
                 IgnoreError(Stop(kDoNotSendKeepAlive)));

    LogInfo("Received %s response", UriToString<kUriLeaderKeepAlive>());

    SuccessOrExit(Tlv::Find<StateTlv>(*aMessage, state));
    VerifyOrExit(state == StateTlv::kAccept, IgnoreError(Stop(kDoNotSendKeepAlive)));

    mTimer.Start(Time::SecToMsec(kKeepAliveTimeout) / 2);

exit:
    return;
}

template <> void Commissioner::HandleTmf<kUriRelayRx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error                    error;
    uint16_t                 joinerPort;
    Ip6::InterfaceIdentifier joinerIid;
    uint16_t                 joinerRloc;
    Ip6::MessageInfo         joinerMessageInfo;
    OffsetRange              offsetRange;

    VerifyOrExit(mState == kStateActive, error = kErrorInvalidState);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest());

    SuccessOrExit(error = Tlv::Find<JoinerUdpPortTlv>(aMessage, joinerPort));
    SuccessOrExit(error = Tlv::Find<JoinerIidTlv>(aMessage, joinerIid));
    SuccessOrExit(error = Tlv::Find<JoinerRouterLocatorTlv>(aMessage, joinerRloc));

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, Tlv::kJoinerDtlsEncapsulation, offsetRange));

    if (!Get<Tmf::SecureAgent>().IsConnectionActive())
    {
        Mac::ExtAddress receivedId;
        Joiner         *joiner;

        mJoinerIid = joinerIid;
        mJoinerIid.ConvertToExtAddress(receivedId);

        joiner = FindBestMatchingJoinerEntry(receivedId);
        VerifyOrExit(joiner != nullptr);

        Get<Tmf::SecureAgent>().SetPsk(joiner->mPskd);
        mActiveJoiner = joiner;

        mJoinerSessionTimer.Start(kJoinerSessionTimeoutMillis);

        LogJoinerEntry("Starting new session with", *joiner);
        SignalJoinerEvent(kJoinerEventStart, joiner);
    }
    else
    {
        if (mJoinerIid != joinerIid)
        {
            LogNote("Ignore %s (%s, 0x%04x), session in progress with (%s, 0x%04x)", UriToString<kUriRelayRx>(),
                    joinerIid.ToString().AsCString(), joinerRloc, mJoinerIid.ToString().AsCString(), mJoinerRloc);

            ExitNow();
        }
    }

    mJoinerPort = joinerPort;
    mJoinerRloc = joinerRloc;

    LogInfo("Received %s (%s, 0x%04x)", UriToString<kUriRelayRx>(), mJoinerIid.ToString().AsCString(), mJoinerRloc);

    aMessage.SetOffset(offsetRange.GetOffset());
    SuccessOrExit(error = aMessage.SetLength(offsetRange.GetEndOffset()));

    joinerMessageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocalEid());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

    Get<Tmf::SecureAgent>().HandleUdpReceive(aMessage, joinerMessageInfo);

exit:
    return;
}

void Commissioner::HandleJoinerSessionTimer(void)
{
    if (mActiveJoiner != nullptr)
    {
        LogJoinerEntry("Timed out session with", *mActiveJoiner);
    }

    Get<Tmf::SecureAgent>().Disconnect();
}

template <>
void Commissioner::HandleTmf<kUriDatasetChanged>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mState == kStateActive);
    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    LogInfo("Received %s", UriToString<kUriDatasetChanged>());

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    LogInfo("Sent %s ack", UriToString<kUriDatasetChanged>());

exit:
    return;
}

template <>
void Commissioner::HandleTmf<kUriJoinerFinalize>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    StateTlv::State                state = StateTlv::kAccept;
    ProvisioningUrlTlv::StringType provisioningUrl;

    VerifyOrExit(mState == kStateActive);

    LogInfo("Received %s", UriToString<kUriJoinerFinalize>());

    switch (Tlv::Find<ProvisioningUrlTlv>(aMessage, provisioningUrl))
    {
    case kErrorNone:
        if (!StringMatch(provisioningUrl, mProvisioningUrl))
        {
            state = StateTlv::kReject;
        }
        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow();
    }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (aMessage.GetLength() <= OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE)
    {
        uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

        aMessage.ReadBytes(aMessage.GetOffset(), buf, aMessage.GetLength() - aMessage.GetOffset());
        DumpCert("[THCI] direction=recv | type=JOIN_FIN.req |", buf, aMessage.GetLength() - aMessage.GetOffset());
    }
#endif

    SendJoinFinalizeResponse(aMessage, state);

exit:
    return;
}

void Commissioner::SendJoinFinalizeResponse(const Coap::Message &aRequest, StateTlv::State aState)
{
    Error            error = kErrorNone;
    Ip6::MessageInfo joinerMessageInfo;
    Coap::Message   *message;

    message = Get<Tmf::SecureAgent>().NewPriorityResponseMessage(aRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    message->SetOffset(message->GetLength());
    message->SetSubType(Message::kSubTypeJoinerFinalizeResponse);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));

    joinerMessageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocalEid());
    joinerMessageInfo.GetPeerAddr().SetIid(mJoinerIid);
    joinerMessageInfo.SetPeerPort(mJoinerPort);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t buf[OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE];

    VerifyOrExit(message->GetLength() <= sizeof(buf));
    message->ReadBytes(message->GetOffset(), buf, message->GetLength() - message->GetOffset());
    DumpCert("[THCI] direction=send | type=JOIN_FIN.rsp |", buf, message->GetLength() - message->GetOffset());
#endif

    SuccessOrExit(error = Get<Tmf::SecureAgent>().SendMessage(*message, joinerMessageInfo));

    SignalJoinerEvent(kJoinerEventFinalize, mActiveJoiner);

    if ((mActiveJoiner != nullptr) && (mActiveJoiner->mType != Joiner::kTypeAny))
    {
        // Remove after kRemoveJoinerDelay (seconds)
        RemoveJoiner(*mActiveJoiner, kRemoveJoinerDelay);
    }

    LogInfo("Sent %s response", UriToString<kUriJoinerFinalize>());

exit:
    FreeMessageOnError(message, error);
}

Error Commissioner::SendRelayTransmit(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Commissioner *>(aContext)->SendRelayTransmit(aMessage, aMessageInfo);
}

Error Commissioner::SendRelayTransmit(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error            error = kErrorNone;
    ExtendedTlv      tlv;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());
    Kek              kek;

    Get<KeyManager>().ExtractKek(kek);

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriRelayTx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<JoinerUdpPortTlv>(*message, mJoinerPort));
    SuccessOrExit(error = Tlv::Append<JoinerIidTlv>(*message, mJoinerIid));
    SuccessOrExit(error = Tlv::Append<JoinerRouterLocatorTlv>(*message, mJoinerRloc));

    if (aMessage.GetSubType() == Message::kSubTypeJoinerFinalizeResponse)
    {
        SuccessOrExit(error = Tlv::Append<JoinerRouterKekTlv>(*message, kek));
    }

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength());
    SuccessOrExit(error = message->Append(tlv));
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, 0, aMessage.GetLength()));

    messageInfo.SetSockAddrToRlocPeerAddrTo(mJoinerRloc);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    aMessage.Free();

exit:
    FreeMessageOnError(message, error);
    return error;
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Commissioner::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "disabled", // (0) kStateDisabled
        "petition", // (1) kStatePetition
        "active",   // (2) kStateActive
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateDisabled);
        ValidateNextEnum(kStatePetition);
        ValidateNextEnum(kStateActive);
    };

    return kStateStrings[aState];
}

void Commissioner::LogJoinerEntry(const char *aAction, const Joiner &aJoiner) const
{
    switch (aJoiner.mType)
    {
    case Joiner::kTypeUnused:
        break;

    case Joiner::kTypeAny:
        LogInfo("%s Joiner (any, %s)", aAction, aJoiner.mPskd.GetAsCString());
        break;

    case Joiner::kTypeEui64:
        LogInfo("%s Joiner (eui64:%s, %s)", aAction, aJoiner.mSharedId.mEui64.ToString().AsCString(),
                aJoiner.mPskd.GetAsCString());
        break;

    case Joiner::kTypeDiscerner:
        LogInfo("%s Joiner (disc:%s, %s)", aAction, aJoiner.mSharedId.mDiscerner.ToString().AsCString(),
                aJoiner.mPskd.GetAsCString());
        break;
    }
}

#else

void Commissioner::LogJoinerEntry(const char *, const Joiner &) const {}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

// LCOV_EXCL_STOP

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
