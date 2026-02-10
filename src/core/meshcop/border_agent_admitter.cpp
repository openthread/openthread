/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the Border Agent Admitter.
 */

#include "border_agent_admitter.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

RegisterLogModule("BorderAdmitter");

//---------------------------------------------------------------------------------------------------------------------
// Admitter

const uint8_t Admitter::kEnrollerValidSteeringDataLengths[] = {1, 8, 16};

Admitter::Admitter(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(kEnabledByDefault)
    , mHasAnyEnroller(false)
    , mArbitrator(aInstance)
    , mCommissionerPetitioner(aInstance)
    , mJoinerTimer(aInstance)
    , mReportStateTask(aInstance)
    , mLastSyncedState(kStateUnavailable)
{
}

void Admitter::SetEnabled(bool aEnable)
{
    VerifyOrExit(mEnabled != aEnable);
    mEnabled = aEnable;
    EvaluateOperation();

    // Signal a Border Agent TXT data refresh so that the `StateBitmap`
    // flag indicating `Admitter` function support is updated.

    Get<TxtData>().Refresh();

exit:
    return;
}

void Admitter::EvaluateOperation(void)
{
    // This method is called whenever there is a change or an event that
    // impacts the operation of the `Admitter` or its sub-components.
    // It evaluates the current operational conditions and orchestrates
    // the starting or stopping of the sub-components accordingly.

    if (mEnabled && Get<Manager>().IsRunning())
    {
        mArbitrator.Start();

        if (mArbitrator.IsPrimeAdmitter() && mHasAnyEnroller)
        {
            mCommissionerPetitioner.Start();
        }
        else
        {
            mCommissionerPetitioner.Stop();
        }
    }
    else
    {
        mCommissionerPetitioner.Stop();
        mArbitrator.Stop();
    }

    PostReportStateTask();
}

Admitter::State Admitter::DetermineState(void) const
{
    // The `Admitter::State` is determined from the operational state of
    // its sub-components (`Arbitrator` and `CommissionerPetitioner`).
    // It is not directly tracked by the `Admitter` itself.

    State state = kStateUnavailable;

    VerifyOrExit(mArbitrator.IsPrimeAdmitter());

    switch (mCommissionerPetitioner.GetState())
    {
    case CommissionerPetitioner::kStopped:
    case CommissionerPetitioner::kToPetition:
    case CommissionerPetitioner::kPetitioning:
        state = kStateReady;
        break;
    case CommissionerPetitioner::kAcceptedToSyncData:
    case CommissionerPetitioner::kAcceptedSyncingData:
    case CommissionerPetitioner::kAcceptedDataSynced:
        state = kStateActive;
        break;
    case CommissionerPetitioner::kRejected:
        state = kStateConflictError;
        break;
    }

exit:
    return state;
}

void Admitter::PostReportStateTask(void)
{
    // Posts a task to signal the `Admitter` state change to all registered
    // Enrollers. We track and check against the last synced state to avoid
    // reporting if the state has not changed.

    State state = DetermineState();

    VerifyOrExit(state != mLastSyncedState);
    mReportStateTask.Post();

exit:
    return;
}

void Admitter::HandleReportStateTask(void)
{
    State state = DetermineState();

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        Manager::CoapDtlsSession &coapSession = *iter.GetSessionAs<Manager::CoapDtlsSession>();

        coapSession.SendEnrollerReportState(state);

        if (state == kStateUnavailable)
        {
            coapSession.ResignEnroller();
        }
    }

    mLastSyncedState = state;
}

void Admitter::HandleEnrollerChange(void)
{
    // Handles any changes to Enroller status or properties,
    // such as registration of a new Enroller, removal of an
    // existing one, or modification of registered properties
    // (e.g., steering data).

    EnrollerIterator iter(GetInstance());
    bool             hasAnyEnroller = !iter.IsDone();

    if (hasAnyEnroller != mHasAnyEnroller)
    {
        mHasAnyEnroller = hasAnyEnroller;
        EvaluateOperation();
    }

    mCommissionerPetitioner.HandleEnrollerChange();
}

void Admitter::DetermineSteeringData(SteeringData &aSteeringData) const
{
    uint8_t maxLength = 1;

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        if (iter.GetEnroller()->mSteeringData.PermitsAllJoiners())
        {
            aSteeringData.SetToPermitAllJoiners();
            ExitNow();
        }

        maxLength = Max(maxLength, iter.GetEnroller()->mSteeringData.GetLength());
    }

    IgnoreError(aSteeringData.Init(maxLength));

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        SuccessOrAssert(aSteeringData.MergeBloomFilterWith(iter.GetEnroller()->mSteeringData));
    }

exit:
    return;
}

void Admitter::ForwardJoinerRelayToEnrollers(const Coap::Msg &aMsg)
{
    Ip6::InterfaceIdentifier joinerIid;

    VerifyOrExit(mCommissionerPetitioner.IsActiveCommissioner());

    VerifyOrExit(aMsg.IsNonConfirmablePostRequest());
    SuccessOrExit(Tlv::Find<JoinerIidTlv>(aMsg.mMessage, joinerIid));

    LogInfo("Processing %s from joiner %s", UriToString<kUriRelayRx>(), joinerIid.ToString().AsCString());

    // Check for a specific `Enroller` that accepted this Joiner IID.
    // If found, forward to that specific `Enroller`, otherwise, send
    // to all.

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        Joiner *joiner = iter.GetEnroller()->mJoiners.FindMatching(joinerIid);

        if (joiner != nullptr)
        {
            joiner->UpdateExpirationTime();
            iter.GetSessionAs<Manager::CoapDtlsSession>()->ForwardUdpRelayToEnroller(aMsg.mMessage);
            ExitNow();
        }
    }

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        iter.GetSessionAs<Manager::CoapDtlsSession>()->ForwardUdpRelayToEnroller(aMsg.mMessage);
    }

exit:
    return;
}

void Admitter::ForwardUdpProxyToEnrollers(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mCommissionerPetitioner.IsActiveCommissioner());

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        iter.GetSessionAs<Manager::CoapDtlsSession>()->ForwardUdpProxyToEnroller(aMessage, aMessageInfo);
    }

exit:
    return;
}

void Admitter::HandleJoinerTimer(void)
{
    NextFireTime       nextTime;
    OwningList<Joiner> removedJoiners;

    for (EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        iter.GetEnroller()->mJoiners.RemoveAllMatching(removedJoiners, ExpirationChecker(nextTime.GetNow()));

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
        for (const Joiner &joiner : removedJoiners)
        {
            LogInfo("Removing timed-out joiner %s - previously accepted by enroller - session %u",
                    joiner.mIid.ToString().AsCString(), iter.GetSessionIndex());
        }
#endif

        removedJoiners.Free();

        for (const Joiner &joiner : iter.GetEnroller()->mJoiners)
        {
            nextTime.UpdateIfEarlier(joiner.mExpirationTime);
        }
    }

    mJoinerTimer.FireAt(nextTime);
}

void Admitter::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        mCommissionerPetitioner.HandleNetDataChange();
    }
}

const char *Admitter::EnrollerUriToString(Uri aUri)
{
    const char *uriString = "Unknown";

    switch (aUri)
    {
    case kUriEnrollerRegister:
        uriString = UriToString<kUriEnrollerRegister>();
        break;
    case kUriEnrollerKeepAlive:
        uriString = UriToString<kUriEnrollerKeepAlive>();
        break;
    case kUriEnrollerJoinerAccept:
        uriString = UriToString<kUriEnrollerJoinerAccept>();
        break;
    case kUriEnrollerJoinerRelease:
        uriString = UriToString<kUriEnrollerJoinerRelease>();
        break;
    default:
        break;
    }

    return uriString;
}

//---------------------------------------------------------------------------------------------------------------------
// Admitter::Arbitrator

Admitter::Arbitrator::Arbitrator(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStopped)
    , mTimer(aInstance)
{
}

void Admitter::Arbitrator::SetState(State aState)
{
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    bool shouldUpdateService = (aState == kPrime);
#endif

    VerifyOrExit(mState != aState);
    LogInfo("Arbitrator state: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    // Signal to update the registered MeshCoP service when the prime
    // admitter role changes, i.e. when we transition to or from the
    // `kPrime` state.

    shouldUpdateService |= (mState == kPrime);

    if (shouldUpdateService)
    {
        Get<Manager>().HandlePrimeAdmitterStateChanged();
    }
#endif

exit:
    return;
}

void Admitter::Arbitrator::Start(void)
{
    VerifyOrExit(mState == kStopped);

    SetState(kClaiming);

    Get<NetworkData::Publisher>().PublishBorderAdmitterService();

    if (Get<NetworkData::Publisher>().IsBorderAdmitterServicePublished())
    {
        HandlePublisherEvent(NetworkData::Publisher::kEventEntryAdded);
    }

exit:
    return;
}

void Admitter::Arbitrator::Stop(void)
{
    VerifyOrExit(mState != kStopped);

    SetState(kStopped);
    mTimer.Stop();
    Get<NetworkData::Publisher>().UnpublishBorderAdmitterService();

exit:
    return;
}

void Admitter::Arbitrator::HandlePublisherEvent(NetworkData::Publisher::Event aEvent)
{
    switch (aEvent)
    {
    case NetworkData::Publisher::kEventEntryAdded:
        VerifyOrExit(mState == kClaiming);
        SetState(kCandidate);
        mTimer.Start(kDelayToBecomePrime);
        break;

    case NetworkData::Publisher::kEventEntryRemoved:
        switch (mState)
        {
        case kStopped:
        case kClaiming:
            break;

        case kCandidate:
            mTimer.Stop();
            SetState(kClaiming);
            break;

        case kPrime:
            SetState(kClaiming);
            Get<Admitter>().EvaluateOperation();
            break;
        }

        break;
    }

exit:
    return;
}

void Admitter::Arbitrator::HandleTimer(void)
{
    VerifyOrExit(mState == kCandidate);

    SetState(kPrime);
    Get<Admitter>().EvaluateOperation();

exit:
    return;
}

const char *Admitter::Arbitrator::StateToString(State aState)
{
#define ArbitratorStateMapList(_) \
    _(kStopped, "Stopped")        \
    _(kClaiming, "Claiming")      \
    _(kCandidate, "Candidate")    \
    _(kPrime, "Prime")

    DefineEnumStringArray(ArbitratorStateMapList);

    return kStrings[aState];
}

//---------------------------------------------------------------------------------------------------------------------
// Admitter::CommissionerPetitioner

Admitter::CommissionerPetitioner::CommissionerPetitioner(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStopped)
    , mJoinerUdpPort(kDefaultJoinerUdpPort)
    , mSessionId(0)
    , mRetryTimer(aInstance)
    , mKeepAliveTimer(aInstance)
    , mUdpReceiver(HandleUdpReceive, this)
{
    mSteeringData.Clear();
    mAloc.InitAsThreadOriginMeshLocal();
}

void Admitter::CommissionerPetitioner::SetJoinerUdpPort(uint16_t aUdpPort)
{
    VerifyOrExit(mJoinerUdpPort != aUdpPort);

    LogInfo("Petitioner JoinerUdpPort: %u -> %u", mJoinerUdpPort, aUdpPort);
    mJoinerUdpPort = aUdpPort;

    VerifyOrExit(IsActiveCommissioner());

    SetState(kAcceptedToSyncData);
    ScheduleImmediateDataSync();

exit:
    return;
}

void Admitter::CommissionerPetitioner::Start(void)
{
    VerifyOrExit(mState == kStopped);

    SetState(kToPetition);
    SendPetitionIfNoOtherCommissioner();

exit:
    return;
}

void Admitter::CommissionerPetitioner::Stop(void)
{
    mRetryTimer.Stop();
    mKeepAliveTimer.Stop();

    switch (mState)
    {
    case kStopped:
    case kToPetition:
    case kPetitioning:
    case kRejected:
        break;

    case kAcceptedSyncingData:
    case kAcceptedToSyncData:
    case kAcceptedDataSynced:
        RemoveAlocAndUdpReceiver();
        IgnoreError(SendKeepAlive(StateTlv::kReject));
        break;
    }

    SetState(kStopped);

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDataSetResponse, this));
}

bool Admitter::CommissionerPetitioner::IsActiveCommissioner(void) const
{
    bool isActive = false;

    switch (mState)
    {
    case kStopped:
    case kToPetition:
    case kPetitioning:
    case kRejected:
        break;
    case kAcceptedToSyncData:
    case kAcceptedSyncingData:
    case kAcceptedDataSynced:
        isActive = true;
        break;
    }

    return isActive;
}

void Admitter::CommissionerPetitioner::SetState(State aState)
{
    VerifyOrExit(mState != aState);
    LogInfo("Petitioner state: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

    Get<Admitter>().PostReportStateTask();

exit:
    return;
}

void Admitter::CommissionerPetitioner::HandleNetDataChange(void)
{
    VerifyOrExit(mState == kRejected);
    SendPetitionIfNoOtherCommissioner();

exit:
    return;
}

void Admitter::CommissionerPetitioner::SendPetitionIfNoOtherCommissioner(void)
{
    Error                         error = kErrorNone;
    OwnedPtr<Coap::Message>       message;
    CommissionerIdTlv::StringType commissionerId;
    StringWriter                  writer(commissionerId, sizeof(commissionerId));

    OT_ASSERT(mState == kToPetition || mState == kRejected);

    if (Get<NetworkData::Leader>().FindInCommissioningData<BorderAgentLocatorTlv>() != nullptr)
    {
        SetState(kRejected);
        ExitNow();
    }

    message.Reset(Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriLeaderPetition));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    writer.Append("otAdmitter");
    writer.AppendHexBytes(Get<Mac::Mac>().GetExtAddress().m8, sizeof(Mac::ExtAddress));
    SuccessOrExit(error = Tlv::Append<CommissionerIdTlv>(*message, commissionerId));

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandlePetitionResponse, this));

    SuccessOrExit(error = SendToLeader(message.PassOwnership(), HandlePetitionResponse));

    LogInfo("Send %s", UriToString<kUriLeaderPetition>());
    SetState(kPetitioning);

exit:
    if (error != kErrorNone)
    {
        SchedulePetitionRetry();
    }
}

void Admitter::CommissionerPetitioner::HandlePetitionResponse(Coap::Msg *aMsg, Error aResult)
{
    Error error = ProcessPetitionResponse(aMsg, aResult);

    if (error != kErrorNone)
    {
        VerifyOrExit(mState == kPetitioning);

        SchedulePetitionRetry();
        SetState((error == kErrorRejected) ? kRejected : kToPetition);
        ExitNow();
    }

    // The state can change while awaiting a petition response from
    // the leader. We intentionally do not abort the TMF transaction,
    // ensuring we can process the response. If the petition is
    // accepted, we always update the session ID and ALOC. However,
    // if the state is `kStopped`, we send a keep-alive with
    // `kReject` to resign the active commissioner role.

    switch (mState)
    {
    case kToPetition:
    case kPetitioning:
    case kRejected:
    case kAcceptedToSyncData:
    case kAcceptedSyncingData:
    case kAcceptedDataSynced:
        ScheduleNextKeepAlive();
        AddAlocAndUdpReceiver();
        SetState(kAcceptedToSyncData);
        Get<Admitter>().DetermineSteeringData(mSteeringData);
        SendDataSet();
        break;

    case kStopped:
        IgnoreError(SendKeepAlive(StateTlv::kReject));
        break;
    }

exit:
    return;
}

Error Admitter::CommissionerPetitioner::ProcessPetitionResponse(const Coap::Msg *aMsg, Error aResult)
{
    // Processes a petition response from the leader, returning
    // `kErrorNone` if the petition is valid and accepted,
    // `kErrorRejected` if it is rejected, and other error codes if
    // the response is invalid or cannot be parsed.

    Error   error = aResult;
    uint8_t state;

    LogInfo("Receive %s response, error: %s", UriToString<kUriLeaderPetition>(), ErrorToString(error));

    SuccessOrExit(error);
    VerifyOrExit(aMsg != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(aMsg->GetCode() == Coap::kCodeChanged, error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<StateTlv>(aMsg->mMessage, state));

    VerifyOrExit(state == StateTlv::kAccept, error = kErrorRejected);

    error = Tlv::Find<CommissionerSessionIdTlv>(aMsg->mMessage, mSessionId);

exit:
    return error;
}

void Admitter::CommissionerPetitioner::SchedulePetitionRetry(void)
{
    mRetryTimer.Start(Random::NonCrypto::AddJitter(kPetitionRetryDelay, kPetitionRetryJitter));
}

void Admitter::CommissionerPetitioner::AddAlocAndUdpReceiver(void)
{
    Ip6::Address alocAddr;

    Get<Mle::Mle>().GetCommissionerAloc(mSessionId, alocAddr);

    if (Get<ThreadNetif>().HasUnicastAddress(mAloc))
    {
        VerifyOrExit(mAloc.GetAddress() != alocAddr);
        RemoveAlocAndUdpReceiver();
    }

    mAloc.SetAddress(alocAddr);

    LogInfo("Adding ALOC %s", alocAddr.ToString().AsCString());

    Get<ThreadNetif>().AddUnicastAddress(mAloc);
    IgnoreError(Get<Ip6::Udp>().AddReceiver(mUdpReceiver));

exit:
    return;
}

void Admitter::CommissionerPetitioner::RemoveAlocAndUdpReceiver(void)
{
    LogInfo("Removing ALOC %s", mAloc.GetAddress().ToString().AsCString());
    IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
    Get<ThreadNetif>().RemoveUnicastAddress(mAloc);
}

Error Admitter::CommissionerPetitioner::SendKeepAlive(StateTlv::State aState)
{
    Error                   error = kErrorNone;
    OwnedPtr<Coap::Message> message;

    message.Reset(Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriLeaderKeepAlive));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<StateTlv>(*message, aState));
    SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, mSessionId));

    switch (aState)
    {
    case StateTlv::kAccept:
        SuccessOrExit(error = SendToLeader(message.PassOwnership(), HandleKeepAliveResponse));
        LogInfo("Send %s", UriToString<kUriLeaderKeepAlive>());
        break;
    case StateTlv::kReject:
    default:
        SuccessOrExit(error = SendToLeader(message.PassOwnership(), nullptr));
        LogInfo("Send %s with reject status - resigning the commissioner role", UriToString<kUriLeaderKeepAlive>());
        break;
    }

exit:
    return error;
}

void Admitter::CommissionerPetitioner::HandleKeepAliveResponse(Coap::Msg *aMsg, Error aResult)
{
    Error   error = aResult;
    uint8_t state;

    LogInfo("Receive %s response, error: %s", UriToString<kUriLeaderKeepAlive>(), ErrorToString(error));

    VerifyOrExit(IsActiveCommissioner());

    if (error == kErrorNone)
    {
        error = (aMsg != nullptr) ? Tlv::Find<StateTlv>(aMsg->mMessage, state) : kErrorInvalidArgs;
    }

    if (error != kErrorNone)
    {
        ScheduleKeepAliveRetry();
        ExitNow();
    }

    if (state != StateTlv::kAccept)
    {
        LogInfo("%s response contains reject status", UriToString<kUriLeaderKeepAlive>());

        mKeepAliveTimer.Stop();
        RemoveAlocAndUdpReceiver();

        SetState(kToPetition);
        SchedulePetitionRetry();
        ExitNow();
    }

    ScheduleNextKeepAlive();

exit:
    return;
}

void Admitter::CommissionerPetitioner::ScheduleNextKeepAlive(void)
{
    mKeepAliveTimer.Start(Random::NonCrypto::AddJitter(kKeepAliveTxInterval, kKeepAliveTxJitter));
}

void Admitter::CommissionerPetitioner::ScheduleKeepAliveRetry(void)
{
    mKeepAliveTimer.Start(Random::NonCrypto::AddJitter(kKeepAliveRetryDelay, kKeepAliveRetryJitter));
}

void Admitter::CommissionerPetitioner::HandleKeepAliveTimer(void)
{
    VerifyOrExit(IsActiveCommissioner());

    switch (SendKeepAlive(StateTlv::kAccept))
    {
    case kErrorNone:
        ScheduleNextKeepAlive();
        break;
    default:
        ScheduleKeepAliveRetry();
        break;
    }

exit:
    return;
}

void Admitter::CommissionerPetitioner::SendDataSet(void)
{
    Error                   error = kErrorNone;
    OwnedPtr<Coap::Message> message;

    OT_ASSERT(mState == kAcceptedToSyncData);

    mRetryTimer.Stop();

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDataSetResponse, this));

    message.Reset(Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriCommissionerSet));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(*message, mSessionId));
    SuccessOrExit(error = Tlv::Append<SteeringDataTlv>(*message, mSteeringData.GetData(), mSteeringData.GetLength()));

    if (mJoinerUdpPort != 0)
    {
        SuccessOrExit(error = Tlv::Append<JoinerUdpPortTlv>(*message, mJoinerUdpPort));
    }

    SuccessOrExit(error = SendToLeader(message.PassOwnership(), HandleDataSetResponse));

    LogInfo("Send %s", UriToString<kUriCommissionerSet>());
    SetState(kAcceptedSyncingData);

exit:
    if (error != kErrorNone)
    {
        ScheduleDataSyncRetry();
    }
}

void Admitter::CommissionerPetitioner::HandleDataSetResponse(Coap::Msg *aMsg, Error aResult)
{
    VerifyOrExit(mState == kAcceptedSyncingData);

    if (ProcessDataSetResponse(aMsg, aResult) == kErrorNone)
    {
        SetState(kAcceptedDataSynced);
    }
    else
    {
        SetState(kAcceptedToSyncData);
        ScheduleDataSyncRetry();
    }

exit:
    return;
}

Error Admitter::CommissionerPetitioner::ProcessDataSetResponse(const Coap::Msg *aMsg, Error aResult)
{
    Error   error = aResult;
    uint8_t state;

    SuccessOrExit(error);
    VerifyOrExit(aMsg != nullptr, error = kErrorInvalidArgs);

    VerifyOrExit(aMsg->GetCode() == Coap::kCodeChanged, error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<StateTlv>(aMsg->mMessage, state));

    VerifyOrExit(state == StateTlv::kAccept, error = kErrorRejected);

exit:
    LogInfo("Receive %s response, error: %s", UriToString<kUriCommissionerSet>(), ErrorToString(error));
    return error;
}

void Admitter::CommissionerPetitioner::ScheduleDataSyncRetry(void)
{
    mRetryTimer.Start(Random::NonCrypto::AddJitter(kDataSyncRetryDelay, kDataSyncRetryJitter));
}

void Admitter::CommissionerPetitioner::ScheduleImmediateDataSync(void) { mRetryTimer.Start(0); }

void Admitter::CommissionerPetitioner::HandleEnrollerChange(void)
{
    SteeringData newSteeringData;

    VerifyOrExit(IsActiveCommissioner());

    Get<Admitter>().DetermineSteeringData(newSteeringData);

    VerifyOrExit(newSteeringData != mSteeringData);
    mSteeringData = newSteeringData;

    SetState(kAcceptedToSyncData);
    ScheduleImmediateDataSync();

exit:
    return;
}

Error Admitter::CommissionerPetitioner::SendToLeader(OwnedPtr<Coap::Message> aMessage, Coap::ResponseHandler aHandler)
{
    Error            error;
    Tmf::MessageInfo messageInfo(GetInstance());

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();

    // On success the message ownership is transferred.

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*aMessage, messageInfo, aHandler,
                                                        (aHandler != nullptr) ? this : nullptr));
    aMessage.Release();

exit:
    return error;
}

void Admitter::CommissionerPetitioner::HandleRetryTimer(void)
{
    switch (mState)
    {
    case kToPetition:
    case kRejected:
        SendPetitionIfNoOtherCommissioner();
        break;
    case kAcceptedToSyncData:
        SendDataSet();
        break;

    case kPetitioning:
    case kStopped:
    case kAcceptedSyncingData:
    case kAcceptedDataSynced:
        break;
    }
}

bool Admitter::CommissionerPetitioner::HandleUdpReceive(void                *aContext,
                                                        const otMessage     *aMessage,
                                                        const otMessageInfo *aMessageInfo)
{
    return static_cast<CommissionerPetitioner *>(aContext)->HandleUdpReceive(AsCoreType(aMessage),
                                                                             AsCoreType(aMessageInfo));
}

bool Admitter::CommissionerPetitioner::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool didHandle = false;

    VerifyOrExit(IsActiveCommissioner());
    VerifyOrExit(aMessageInfo.GetSockAddr() == mAloc.GetAddress());

    Get<Admitter>().ForwardUdpProxyToEnrollers(aMessage, aMessageInfo);
    didHandle = true;

exit:
    return didHandle;
}

const char *Admitter::CommissionerPetitioner::StateToString(State aState)
{
#define CommissionerPetitionerStateMapList(_)      \
    _(kStopped, "Stopped")                         \
    _(kToPetition, "ToPetition")                   \
    _(kPetitioning, "Petitioning")                 \
    _(kAcceptedToSyncData, "AcceptedToSyncData")   \
    _(kAcceptedSyncingData, "AcceptedSyncingData") \
    _(kAcceptedDataSynced, "AcceptedDataSynced")   \
    _(kRejected, "Rejected")

    DefineEnumStringArray(CommissionerPetitionerStateMapList);

    return kStrings[aState];
}

//---------------------------------------------------------------------------------------------------------------------
// Admitter::Joiner

Admitter::Joiner::Joiner(Instance &aInstance, const Ip6::InterfaceIdentifier &aIid)
    : InstanceLocator(aInstance)
    , mNext(nullptr)
    , mIid(aIid)
{
    mAcceptUptime = Get<UptimeTracker>().GetUptime();
    UpdateExpirationTime();
}

void Admitter::Joiner::UpdateExpirationTime(void)
{
    mExpirationTime = TimerMilli::GetNow() + kTimeout;

    Get<Admitter>().mJoinerTimer.FireAtIfEarlier(mExpirationTime);
}

bool Admitter::Joiner::Matches(const Ip6::InterfaceIdentifier &aIid) const
{
    // An unspecified `aIid` (all zero) acts as a wildcard.

    return aIid.IsUnspecified() || (aIid == mIid);
}

bool Admitter::Joiner::Matches(const ExpirationChecker &aChecker) const { return aChecker.IsExpired(mExpirationTime); }

//---------------------------------------------------------------------------------------------------------------------
// Admitter::Iterator

void Admitter::Iterator::Init(Instance &aInstance)
{
    SetSession(aInstance.Get<Manager>().mDtlsTransport.GetSessions().GetHead());
    SetJoiner(nullptr);
    SetInitUptime(aInstance.Get<UptimeTracker>().GetUptime());
    SetInitTime(TimerMilli::GetNow());
}

void Admitter::Iterator::SkipToNextEnrollerSession(SecureSession *&aSession)
{
    // Skip over sessions in the list that are not enrollers, starting
    // from the given `aSession` itself. Upon return, the `aSession`
    // pointer is updated to the next session which is an enroller,
    // or it is `nullptr` if none is found.

    for (; aSession != nullptr; aSession = aSession->GetNext())
    {
        if (static_cast<Manager::CoapDtlsSession *>(aSession)->IsEnroller())
        {
            break;
        }
    }
}

Error Admitter::Iterator::GetNextEnrollerInfo(EnrollerInfo &aEnrollerInfo)
{
    Error                     error = kErrorNone;
    SecureSession            *session;
    Manager::CoapDtlsSession *coapSession;
    Enroller                 *enroller;

    session = GetSession();

    SkipToNextEnrollerSession(session);

    VerifyOrExit(session != nullptr, error = kErrorNotFound);

    coapSession = static_cast<Manager::CoapDtlsSession *>(session);
    enroller    = coapSession->mEnroller.Get();

    coapSession->CopyInfoTo(aEnrollerInfo.mSessionInfo, GetInitUptime());

    aEnrollerInfo.mId               = enroller->mId;
    aEnrollerInfo.mSteeringData     = enroller->mSteeringData;
    aEnrollerInfo.mMode             = enroller->mMode;
    aEnrollerInfo.mRegisterDuration = GetInitUptime() - enroller->mRegisterUptime;

    SetJoiner(enroller->mJoiners.GetHead());
    SetSession(session->GetNext());

exit:
    return error;
}

Error Admitter::Iterator::GetNextJoinerInfo(JoinerInfo &aJoinerInfo)
{
    Error   error  = kErrorNone;
    Joiner *joiner = GetJoiner();

    VerifyOrExit(joiner != nullptr, error = kErrorNotFound);

    aJoinerInfo.mIid                = joiner->mIid;
    aJoinerInfo.mMsecSinceAccept    = GetInitUptime() - joiner->mAcceptUptime;
    aJoinerInfo.mMsecTillExpiration = Max(joiner->mExpirationTime, GetInitTime()) - GetInitTime();

    SetJoiner(joiner->GetNext());

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// Admitter::EnrollerIterator

Admitter::EnrollerIterator::EnrollerIterator(Instance &aInstance)
    : mSession(aInstance.Get<Manager>().mDtlsTransport.GetSessions().GetHead())
{
    FindNextEnroller();
}

void Admitter::EnrollerIterator::Advance(void)
{
    VerifyOrExit(mSession != nullptr);

    mSession = mSession->GetNext();
    FindNextEnroller();

exit:
    return;
}

Admitter::Enroller *Admitter::EnrollerIterator::GetEnroller(void) const
{
    Enroller *enroller = nullptr;

    VerifyOrExit(mSession != nullptr);
    enroller = GetSessionAs<Manager::CoapDtlsSession>()->mEnroller.Get();

exit:
    return enroller;
}

uint16_t Admitter::EnrollerIterator::GetSessionIndex(void) const
{
    return (mSession == nullptr) ? 0 : GetSessionAs<Manager::CoapDtlsSession>()->mIndex;
}

//---------------------------------------------------------------------------------------------------------------------
// Manager::CoapDtlSession (enroller/admitter specific methods)

void Manager::CoapDtlsSession::ResignEnroller(void)
{
    VerifyOrExit(mEnroller != nullptr);

    LogInfo("Resigning enroller - session %u", mIndex);
    mEnroller.Reset(nullptr);
    Get<Admitter>().HandleEnrollerChange();

exit:
    return;
}

void Manager::CoapDtlsSession::HandleEnrollerTmf(Uri aUri, const Coap::Msg &aMsg)
{
    Error           error = kErrorNone;
    StateTlv::State responseState;

    VerifyOrExit(aMsg.IsConfirmablePostRequest());

    LogInfo("Receive %s", Admitter::EnrollerUriToString(aUri));

    switch (aUri)
    {
    case kUriEnrollerRegister:
        error = ProcessEnrollerRegister(aMsg.mMessage);
        break;
    case kUriEnrollerKeepAlive:
        error = ProcessEnrollerKeepAlive(aMsg.mMessage);
        break;
    case kUriEnrollerJoinerAccept:
        error = ProcessEnrollerJoinerAccept(aMsg.mMessage);
        break;
    case kUriEnrollerJoinerRelease:
        error = ProcessEnrollerJoinerRelease(aMsg.mMessage);
        break;
    default:
        ExitNow();
    }

    responseState = (error == kErrorNone) ? StateTlv::kAccept : StateTlv::kReject;

    SendEnrollerResponse(aUri, responseState, aMsg.mMessage);

exit:
    return;
}

Error Manager::CoapDtlsSession::ProcessEnrollerRegister(const Coap::Message &aRequest)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<Admitter>().IsPrimeAdmitter(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    VerifyOrExit(!Get<EphemeralKeyManager>().OwnsSession(*this), error = kErrorNotCapable);
#endif

    mEnroller.Reset(Admitter::Enroller::Allocate());
    VerifyOrExit(mEnroller != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Find<EnrollerIdTlv>(aRequest, mEnroller->mId));
    SuccessOrExit(error = Tlv::Find<EnrollerModeTlv>(aRequest, mEnroller->mMode));
    SuccessOrExit(error = ReadSteeringDataTlv(aRequest, mEnroller->mSteeringData));
    mEnroller->mRegisterUptime = Get<UptimeTracker>().GetUptime();

    LogInfo("Registered enroller - session %u", mIndex);
    LogInfo("  id: %s", mEnroller->mId);
    LogInfo("  mode: 0x%02x", mEnroller->mMode);
    LogInfo("  steeringData: %s", mEnroller->mSteeringData.ToString().AsCString());

    mTimer.Start(Admitter::kEnrollerKeepAliveTimeout);
    Get<Admitter>().HandleEnrollerChange();

exit:
    if (error != kErrorNone)
    {
        LogWarn("Failed processing %s - session %u, error:%s", UriToString<kUriEnrollerRegister>(), mIndex,
                ErrorToString(error));
        ResignEnroller();
    }

    return error;
}

Error Manager::CoapDtlsSession::ReadSteeringDataTlv(const Message &aMessage, SteeringData &aSteeringData)
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, Tlv::kSteeringData, offsetRange));

    // Ensure the read steering data has a valid length. A length of
    // one byte is only allowed to indicate `PermitsAllJoiners()`.

    error = kErrorInvalidArgs;

    for (uint8_t validLength : Admitter::kEnrollerValidSteeringDataLengths)
    {
        if (offsetRange.GetLength() == validLength)
        {
            error = kErrorNone;
            break;
        }
    }

    SuccessOrExit(error);

    IgnoreError(aSteeringData.Init(static_cast<uint8_t>(offsetRange.GetLength())));
    aMessage.ReadBytes(offsetRange, aSteeringData.GetData());

    if (aSteeringData.GetLength() == 1)
    {
        VerifyOrExit(aSteeringData.PermitsAllJoiners() || aSteeringData.IsEmpty(), error = kErrorInvalidArgs);
    }

exit:
    return error;
}

Error Manager::CoapDtlsSession::ProcessEnrollerKeepAlive(const Coap::Message &aRequest)
{
    Error        error = kErrorNone;
    uint8_t      state;
    uint8_t      mode;
    SteeringData steeringData;

    VerifyOrExit(IsEnroller(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::Find<StateTlv>(aRequest, state));

    if (state != StateTlv::kAccept)
    {
        ResignEnroller();
        error = kErrorRejected;
        ExitNow();
    }

    // EnrollerKeepAlive can optionally include Enroller Mode TLV
    // or Steering Data TLV.

    error = Tlv::Find<EnrollerModeTlv>(aRequest, mode);

    switch (error)
    {
    case kErrorNone:
        VerifyOrExit(mEnroller->mMode != mode);
        LogInfo("Enroller mode changed: 0x%02x -> 0x%02x, session %u", mEnroller->mMode, mode, mIndex);
        mEnroller->mMode = mode;
        break;
    case kErrorNotFound:
        error = kErrorNone;
        break;
    default:
        ExitNow();
    }

    error = ReadSteeringDataTlv(aRequest, steeringData);

    switch (error)
    {
    case kErrorNone:
        VerifyOrExit(mEnroller->mSteeringData != steeringData);
        mEnroller->mSteeringData = steeringData;
        LogInfo("Enroller steering data changed - session %u", mIndex);
        LogInfo("  steeringData: %s", mEnroller->mSteeringData.ToString().AsCString());
        Get<Admitter>().HandleEnrollerChange();
        break;
    case kErrorNotFound:
        error = kErrorNone;
        break;
    default:
        ExitNow();
    }

    LogInfo("Extending enroller timeout - session %u", mIndex);
    mTimer.Start(Admitter::kEnrollerKeepAliveTimeout);

exit:
    if (error != kErrorNone)
    {
        ResignEnroller();
    }

    return error;
}

Error Manager::CoapDtlsSession::ProcessEnrollerJoinerAccept(const Coap::Message &aRequest)
{
    Error                    error = kErrorNone;
    Ip6::InterfaceIdentifier joinerIid;
    Admitter::Joiner        *joiner;

    VerifyOrExit(IsEnroller(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::Find<JoinerIidTlv>(aRequest, joinerIid));

    VerifyOrExit(!joinerIid.IsUnspecified(), error = kErrorInvalidArgs);

    joiner = mEnroller->mJoiners.FindMatching(joinerIid);

    if (joiner != nullptr)
    {
        joiner->UpdateExpirationTime();
        LogInfo("Enroller re-accepted joiner %s - session %u", joinerIid.ToString().AsCString(), mIndex);
        ExitNow();
    }

    for (Admitter::EnrollerIterator iter(GetInstance()); !iter.IsDone(); iter.Advance())
    {
        if (iter.GetEnroller()->mJoiners.ContainsMatching(joinerIid))
        {
            LogInfo("Joiner %s is already accepted by another Enroller %u, rejecting request - session %u",
                    joinerIid.ToString().AsCString(), iter.GetSessionIndex(), mIndex);
            ExitNow(error = kErrorRejected);
        }
    }

    joiner = Admitter::Joiner::Allocate(GetInstance(), joinerIid);
    VerifyOrExit(joiner != nullptr, error = kErrorNoBufs);

    mEnroller->mJoiners.Push(*joiner);
    LogInfo("Enroller accepted joiner %s - session %u", joinerIid.ToString().AsCString(), mIndex);

exit:
    return error;
}

Error Manager::CoapDtlsSession::ProcessEnrollerJoinerRelease(const Coap::Message &aRequest)
{
    Error                    error = kErrorNone;
    Ip6::InterfaceIdentifier joinerIid;

    VerifyOrExit(IsEnroller(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::Find<JoinerIidTlv>(aRequest, joinerIid));

    VerifyOrExit(mEnroller->mJoiners.RemoveAndFreeAllMatching(joinerIid));

    if (joinerIid.IsUnspecified())
    {
        LogInfo("Enroller released all its previously accepted joiners - session %u", mIndex);
    }
    else
    {
        LogInfo("Enroller released joiner %s - session %u", joinerIid.ToString().AsCString(), mIndex);
    }

exit:
    return error;
}

void Manager::CoapDtlsSession::SendEnrollerResponse(Uri                  aUri,
                                                    StateTlv::State      aResponseState,
                                                    const Coap::Message &aRequest)
{
    OwnedPtr<Coap::Message> response;

    response.Reset(NewPriorityResponseMessage(aRequest));
    VerifyOrExit(response != nullptr);

    SuccessOrExit(Tlv::Append<StateTlv>(*response, static_cast<uint8_t>(aResponseState)));

    switch (aUri)
    {
    case kUriEnrollerRegister:
    case kUriEnrollerKeepAlive:
        SuccessOrExit(AppendAdmitterTlvs(*response, Get<Admitter>().DetermineState()));
        break;
    default:
        break;
    }

    SuccessOrExit(SendMessage(response.PassOwnership()));

    LogInfo("Send %s response (%s) - session %u", Admitter::EnrollerUriToString(aUri),
            StateTlv::StateToString(aResponseState), mIndex);

exit:
    return;
}

void Manager::CoapDtlsSession::SendEnrollerReportState(uint8_t aAdmitterState)
{
    OwnedPtr<Coap::Message> message;

    message.Reset(NewNonConfirmablePostMessage(kUriEnrollerReportState));
    VerifyOrExit(message != nullptr);

    SuccessOrExit(AppendAdmitterTlvs(*message, aAdmitterState));

    SuccessOrExit(SendMessage(message.PassOwnership()));

    LogInfo("Send %s - session %u", UriToString<kUriEnrollerReportState>(), mIndex);

exit:
    return;
}

Error Manager::CoapDtlsSession::AppendAdmitterTlvs(Coap::Message &aMessage, uint8_t aAdmitterState)
{
    Error    error;
    uint16_t joinerUdpPort;

    SuccessOrExit(error = Tlv::Append<AdmitterStateTlv>(aMessage, aAdmitterState));

    VerifyOrExit(aAdmitterState == Admitter::kStateActive);

    SuccessOrExit(error = Tlv::Append<CommissionerSessionIdTlv>(aMessage, Get<Admitter>().GetCommissionerSessionId()));

    joinerUdpPort = Get<Admitter>().GetJoinerUdpPort();

    if (joinerUdpPort != 0)
    {
        SuccessOrExit(error = Tlv::Append<JoinerUdpPortTlv>(aMessage, joinerUdpPort));
    }

exit:
    return error;
}

void Manager::CoapDtlsSession::ForwardUdpRelayToEnroller(const Coap::Message &aMessage)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnroller());
    VerifyOrExit(mEnroller->ShouldForwardJoinerRelay());

    SuccessOrExit(error = ForwardUdpRelay(aMessage));
    LogInfo("Forward %s to enroller - session %u", UriToString<kUriRelayRx>(), mIndex);

exit:
    LogWarnOnError(error, "forward UDP relay to enroller");
}

void Manager::CoapDtlsSession::ForwardUdpProxyToEnroller(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnroller());
    VerifyOrExit(mEnroller->ShouldForwardUdpProxy());

    SuccessOrExit(error = ForwardUdpProxy(aMessage, aMessageInfo));

    LogInfo("Forward %s to enroller - session %u", UriToString<kUriProxyRx>(), mIndex);

exit:
    LogWarnOnError(error, "forward UDP proxy to enroller");
}

} // namespace BorderAgent
} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_ADMITTER_ENABLE
