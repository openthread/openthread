/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the BorderAgent service.
 */

#include "border_agent.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("BorderAgent");

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent`

const char BorderAgent::kTxtDataRecordVersion[] = "1";
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
const char BorderAgent::kServiceType[]            = "_meshcop._udp";
const char BorderAgent::kDefaultBaseServiceName[] = OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME;
#endif

BorderAgent::BorderAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(true)
    , mIsRunning(false)
    , mDtlsTransport(aInstance, kNoLinkSecurity)
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    , mIdInitialized(false)
#endif
    , mServiceTask(aInstance)
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    , mEphemeralKeyManager(aInstance)
#endif
{
    ClearAllBytes(mCounters);

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    ClearAllBytes(mServiceName);
    PostServiceTask();

    static_assert(sizeof(kDefaultBaseServiceName) - 1 <= kBaseServiceNameMaxLen,
                  "OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_BASE_NAME is too long");
#endif
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
Error BorderAgent::GetId(Id &aId)
{
    Error error = kErrorNone;

    if (mIdInitialized)
    {
        aId = mId;
        ExitNow();
    }

    if (Get<Settings>().Read<Settings::BorderAgentId>(mId) != kErrorNone)
    {
        mId.GenerateRandom();
        SuccessOrExit(error = Get<Settings>().Save<Settings::BorderAgentId>(mId));
    }

    mIdInitialized = true;
    aId            = mId;

exit:
    return error;
}

Error BorderAgent::SetId(const Id &aId)
{
    Error error = kErrorNone;

    if (mIdInitialized)
    {
        VerifyOrExit(aId != mId);
    }

    SuccessOrExit(error = Get<Settings>().Save<Settings::BorderAgentId>(aId));
    mId            = aId;
    mIdInitialized = true;
    PostServiceTask();

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

void BorderAgent::SetEnabled(bool aEnabled)
{
    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;
    LogInfo("%sabling Border Agent", mEnabled ? "En" : "Dis");
    UpdateState();

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    if (!mEnabled)
    {
        UnregisterService();
    }
#endif

exit:
    return;
}

void BorderAgent::UpdateState(void)
{
    if (mEnabled && Get<Mle::Mle>().IsAttached())
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void BorderAgent::Start(void)
{
    Error error = kErrorNone;
    Pskc  pskc;

    VerifyOrExit(!mIsRunning);

    mDtlsTransport.SetAcceptCallback(BorderAgent::HandleAcceptSession, this);
    mDtlsTransport.SetRemoveSessionCallback(BorderAgent::HandleRemoveSession, this);

    SuccessOrExit(error = mDtlsTransport.Open());
    SuccessOrExit(error = mDtlsTransport.Bind(kUdpPort));

    Get<KeyManager>().GetPskc(pskc);
    SuccessOrExit(error = mDtlsTransport.SetPsk(pskc.m8, Pskc::kSize));
    pskc.Clear();

    mIsRunning = true;
    PostServiceTask();

    LogInfo("Border Agent start listening on port %u", GetUdpPort());

exit:
    if (!mIsRunning)
    {
        mDtlsTransport.Close();
    }

    LogWarnOnError(error, "start agent");
}

void BorderAgent::Stop(void)
{
    VerifyOrExit(mIsRunning);

    mDtlsTransport.Close();
    mIsRunning = false;
    PostServiceTask();

    LogInfo("Border Agent stopped");

exit:
    return;
}

uint16_t BorderAgent::GetUdpPort(void) const { return mDtlsTransport.GetUdpPort(); }

void BorderAgent::SetServiceChangedCallback(ServiceChangedCallback aCallback, void *aContext)
{
    mServiceChangedCallback.Set(aCallback, aContext);

    PostServiceTask();
}

void BorderAgent::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        UpdateState();
    }

    VerifyOrExit(mEnabled);

    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadExtPanIdChanged | kEventThreadNetworkNameChanged |
                            kEventThreadBackboneRouterStateChanged | kEventActiveDatasetChanged))
    {
        PostServiceTask();
    }

    if (aEvents.ContainsAny(kEventPskcChanged))
    {
        Pskc pskc;

        VerifyOrExit(mIsRunning);

        Get<KeyManager>().GetPskc(pskc);

        // If there is secure session already established, it won't be impacted,
        // new pskc will be applied for next connection.
        SuccessOrExit(mDtlsTransport.SetPsk(pskc.m8, Pskc::kSize));
        pskc.Clear();
    }

exit:
    return;
}

SecureSession *BorderAgent::HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<BorderAgent *>(aContext)->HandleAcceptSession();
}

BorderAgent::CoapDtlsSession *BorderAgent::HandleAcceptSession(void)
{
    return CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport);
}

void BorderAgent::HandleRemoveSession(void *aContext, SecureSession &aSession)
{
    static_cast<BorderAgent *>(aContext)->HandleRemoveSession(aSession);
}

void BorderAgent::HandleRemoveSession(SecureSession &aSession)
{
    CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(aSession);

    coapSession.Cleanup();
    coapSession.Free();
}

void BorderAgent::HandleSessionConnected(CoapDtlsSession &aSession)
{
    OT_UNUSED_VARIABLE(aSession);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mEphemeralKeyManager.OwnsSession(aSession))
    {
        mEphemeralKeyManager.HandleSessionConnected();
    }
    else
#endif
    {
        mCounters.mPskcSecureSessionSuccesses++;
    }
}

void BorderAgent::HandleSessionDisconnected(CoapDtlsSession &aSession, CoapDtlsSession::ConnectEvent aEvent)
{
    OT_UNUSED_VARIABLE(aSession);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mEphemeralKeyManager.OwnsSession(aSession))
    {
        mEphemeralKeyManager.HandleSessionDisconnected(aEvent);
    }
    else
#endif
    {
        if (aEvent == CoapDtlsSession::kDisconnectedError)
        {
            mCounters.mPskcSecureSessionFailures++;
        }
    }
}

void BorderAgent::HandleCommissionerPetitionAccepted(CoapDtlsSession &aSession)
{
    OT_UNUSED_VARIABLE(aSession);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mEphemeralKeyManager.OwnsSession(aSession))
    {
        mEphemeralKeyManager.HandleCommissionerPetitionAccepted();
    }
    else
#endif
    {
        mCounters.mPskcCommissionerPetitions++;
    }
}

BorderAgent::CoapDtlsSession *BorderAgent::FindActiveCommissionerSession(void)
{
    CoapDtlsSession *commissionerSession = nullptr;

    for (SecureSession &session : mDtlsTransport.GetSessions())
    {
        CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(session);

        if (coapSession.IsActiveCommissioner())
        {
            commissionerSession = &coapSession;
            break;
        }
    }

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if ((mEphemeralKeyManager.mCoapDtlsSession != nullptr) &&
        mEphemeralKeyManager.mCoapDtlsSession->IsActiveCommissioner())
    {
        commissionerSession = mEphemeralKeyManager.mCoapDtlsSession;
    }
#endif

    return commissionerSession;
}

Coap::Message::Code BorderAgent::CoapCodeFromError(Error aError)
{
    Coap::Message::Code code;

    switch (aError)
    {
    case kErrorNone:
        code = Coap::kCodeChanged;
        break;

    case kErrorParse:
        code = Coap::kCodeBadRequest;
        break;

    default:
        code = Coap::kCodeInternalError;
        break;
    }

    return code;
}

template <> void BorderAgent::HandleTmf<kUriRelayRx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    // This is from TMF agent.

    OT_UNUSED_VARIABLE(aMessageInfo);

    Coap::Message   *message = nullptr;
    Error            error   = kErrorNone;
    CoapDtlsSession *session;

    VerifyOrExit(mIsRunning);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = kErrorDrop);

    session = FindActiveCommissionerSession();
    VerifyOrExit(session != nullptr);

    message = session->NewPriorityNonConfirmablePostMessage(kUriRelayRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = session->ForwardToCommissioner(*message, aMessage));
    LogInfo("Sent to commissioner on RelayRx (c/rx)");

exit:
    FreeMessageOnError(message, error);
}

void BorderAgent::PostServiceTask(void)
{
    VerifyOrExit(mEnabled);

#if !OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    VerifyOrExit(mServiceChangedCallback.IsSet());
#endif

    mServiceTask.Post();

exit:
    return;
}

void BorderAgent::HandleServiceTask(void)
{
    VerifyOrExit(mEnabled);

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    RegisterService();
#endif
    mServiceChangedCallback.InvokeIfSet();

exit:
    return;
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

Error BorderAgent::SetServiceBaseName(const char *aBaseName)
{
    Error                  error = kErrorNone;
    Dns::Name::LabelBuffer newName;

    VerifyOrExit(StringLength(aBaseName, kBaseServiceNameMaxLen + 1) <= kBaseServiceNameMaxLen,
                 error = kErrorInvalidArgs);

    ConstrcutServiceName(aBaseName, newName);

    VerifyOrExit(!StringMatch(newName, mServiceName));

    UnregisterService();
    IgnoreError(StringCopy(mServiceName, newName));
    RegisterService();

exit:
    return error;
}

void BorderAgent::SetVendorTxtData(const uint8_t *aVendorData, uint16_t aVendorDataLength)
{
    VerifyOrExit(!mVendorTxtData.Matches(aVendorData, aVendorDataLength));

    SuccessOrAssert(mVendorTxtData.SetFrom(aVendorData, aVendorDataLength));
    PostServiceTask();

exit:
    return;
}

const char *BorderAgent::GetServiceName(void)
{
    if (IsServiceNameEmpty())
    {
        ConstrcutServiceName(kDefaultBaseServiceName, mServiceName);
    }

    return mServiceName;
}

void BorderAgent::ConstrcutServiceName(const char *aBaseName, Dns::Name::LabelBuffer &aNameBuffer)
{
    StringWriter writer(aNameBuffer, sizeof(Dns::Name::LabelBuffer));

    writer.Append("%.*s%s", kBaseServiceNameMaxLen, aBaseName, Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
}

void BorderAgent::RegisterService(void)
{
    Dnssd::Service service;
    uint8_t       *txtDataBuffer;
    uint16_t       txtDataBufferSize;
    uint16_t       txtDataLength;

    VerifyOrExit(Get<Dnssd>().IsReady());

    // Allocate a large enough buffer to fit both the TXT data
    // generated by Border Agent itself and the vendor extra
    // TXT data. The vendor TXT Data is appended at the
    // end.

    txtDataBufferSize = kTxtDataMaxSize + mVendorTxtData.GetLength();
    txtDataBuffer     = reinterpret_cast<uint8_t *>(Heap::CAlloc(txtDataBufferSize, sizeof(uint8_t)));
    OT_ASSERT(txtDataBuffer != nullptr);

    SuccessOrAssert(PrepareServiceTxtData(txtDataBuffer, txtDataBufferSize, txtDataLength));

    if (mVendorTxtData.GetLength() != 0)
    {
        mVendorTxtData.CopyBytesTo(txtDataBuffer + txtDataLength);
        txtDataLength += mVendorTxtData.GetLength();
    }

    service.Clear();
    service.mServiceInstance = GetServiceName();
    service.mServiceType     = kServiceType;
    service.mPort            = IsRunning() ? GetUdpPort() : kDummyUdpPort;
    service.mTxtData         = txtDataBuffer;
    service.mTxtDataLength   = txtDataLength;

    Get<Dnssd>().RegisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);

    Heap::Free(txtDataBuffer);

exit:
    return;
}

void BorderAgent::UnregisterService(void)
{
    Dnssd::Service service;

    VerifyOrExit(Get<Dnssd>().IsReady());
    VerifyOrExit(!IsServiceNameEmpty());

    service.Clear();
    service.mServiceInstance = GetServiceName();
    service.mServiceType     = kServiceType;

    Get<Dnssd>().UnregisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

Error BorderAgent::PrepareServiceTxtData(ServiceTxtData &aTxtData)
{
    return PrepareServiceTxtData(aTxtData.mData, sizeof(aTxtData.mData), aTxtData.mLength);
}

Error BorderAgent::PrepareServiceTxtData(uint8_t *aBuffer, uint16_t aBufferSize, uint16_t &aLength)
{
    Error               error = kErrorNone;
    Dns::TxtDataEncoder encoder(aBuffer, aBufferSize);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    {
        Id id;

        if (GetId(id) == kErrorNone)
        {
            SuccessOrExit(error = encoder.AppendEntry("id", id));
        }
    }
#endif
    SuccessOrExit(error = encoder.AppendStringEntry("rv", kTxtDataRecordVersion));
    SuccessOrExit(error = encoder.AppendNameEntry("nn", Get<NetworkNameManager>().GetNetworkName().GetAsData()));
    SuccessOrExit(error = encoder.AppendEntry("xp", Get<ExtendedPanIdManager>().GetExtPanId()));
    SuccessOrExit(error = encoder.AppendStringEntry("tv", kThreadVersionString));
    SuccessOrExit(error = encoder.AppendEntry("xa", Get<Mac::Mac>().GetExtAddress()));
    SuccessOrExit(error = encoder.AppendBigEndianUintEntry("sb", DetermineStateBitmap()));

    if (Get<Mle::Mle>().IsAttached())
    {
        SuccessOrExit(error = encoder.AppendBigEndianUintEntry("pt", Get<Mle::Mle>().GetLeaderData().GetPartitionId()));

        if (Get<MeshCoP::ActiveDatasetManager>().GetTimestamp().IsValid())
        {
            SuccessOrExit(error = encoder.AppendEntry("at", Get<MeshCoP::ActiveDatasetManager>().GetTimestamp()));
        }
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (Get<Mle::Mle>().IsAttached() && Get<BackboneRouter::Local>().IsEnabled())
    {
        BackboneRouter::Config bbrConfig;

        Get<BackboneRouter::Local>().GetConfig(bbrConfig);
        SuccessOrExit(error = encoder.AppendEntry("sq", bbrConfig.mSequenceNumber));
        SuccessOrExit(error = encoder.AppendBigEndianUintEntry("bb", BackboneRouter::kBackboneUdpPort));
    }

    SuccessOrExit(error =
                      encoder.AppendNameEntry("dn", Get<MeshCoP::NetworkNameManager>().GetDomainName().GetAsData()));
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    {
        Ip6::Prefix                                   prefix;
        BorderRouter::RoutingManager::RoutePreference preference;

        if (Get<BorderRouter::RoutingManager>().GetFavoredOmrPrefix(prefix, preference) == kErrorNone &&
            prefix.GetLength() > 0)
        {
            uint8_t omrData[Ip6::NetworkPrefix::kSize + 1];

            omrData[0] = prefix.GetLength();
            memcpy(omrData + 1, prefix.GetBytes(), prefix.GetBytesSize());

            SuccessOrExit(error = encoder.AppendEntry("omr", omrData));
        }
    }
#endif

    aLength = encoder.GetLength();

exit:
    return error;
}

uint32_t BorderAgent::DetermineStateBitmap(void) const
{
    uint32_t bitmap = 0;

    bitmap |= (IsRunning() ? StateBitmap::kConnectionModePskc : StateBitmap::kConnectionModeDisabled);
    bitmap |= StateBitmap::kAvailabilityHigh;

    switch (Get<Mle::Mle>().GetRole())
    {
    case Mle::DeviceRole::kRoleDisabled:
        bitmap |= (StateBitmap::kThreadIfStatusNotInitialized | StateBitmap::kThreadRoleDisabledOrDetached);
        break;
    case Mle::DeviceRole::kRoleDetached:
        bitmap |= (StateBitmap::kThreadIfStatusInitialized | StateBitmap::kThreadRoleDisabledOrDetached);
        break;
    case Mle::DeviceRole::kRoleChild:
        bitmap |= (StateBitmap::kThreadIfStatusActive | StateBitmap::kThreadRoleChild);
        break;
    case Mle::DeviceRole::kRoleRouter:
        bitmap |= (StateBitmap::kThreadIfStatusActive | StateBitmap::kThreadRoleRouter);
        break;
    case Mle::DeviceRole::kRoleLeader:
        bitmap |= (StateBitmap::kThreadIfStatusActive | StateBitmap::kThreadRoleLeader);
        break;
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (Get<Mle::Mle>().IsAttached())
    {
        bitmap |= (Get<BackboneRouter::Local>().IsEnabled() ? StateBitmap::kFlagBbrIsActive : 0);
        bitmap |= (Get<BackboneRouter::Local>().IsPrimary() ? StateBitmap::kFlagBbrIsPrimary : 0);
    }
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    if (mEphemeralKeyManager.GetState() != EphemeralKeyManager::kStateDisabled)
    {
        bitmap |= StateBitmap::kFlagEpskcSupported;
    }
#endif

    return bitmap;
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgent::SessionIterator

void BorderAgent::SessionIterator::Init(Instance &aInstance)
{
    SetSession(static_cast<CoapDtlsSession *>(aInstance.Get<BorderAgent>().mDtlsTransport.GetSessions().GetHead()));
    SetInitTime(aInstance.Get<Uptime>().GetUptime());
}

Error BorderAgent::SessionIterator::GetNextSessionInfo(SessionInfo &aSessionInfo)
{
    Error            error   = kErrorNone;
    CoapDtlsSession *session = GetSession();

    VerifyOrExit(session != nullptr, error = kErrorNotFound);

    SetSession(static_cast<CoapDtlsSession *>(session->GetNext()));

    aSessionInfo.mPeerSockAddr.mAddress = session->GetMessageInfo().GetPeerAddr();
    aSessionInfo.mPeerSockAddr.mPort    = session->GetMessageInfo().GetPeerPort();
    aSessionInfo.mIsConnected           = session->IsConnected();
    aSessionInfo.mIsCommissioner        = session->IsActiveCommissioner();
    aSessionInfo.mLifetime              = GetInitTime() - session->GetAllocationTime();

exit:
    return error;
}

//----------------------------------------------------------------------------------------------------------------------
// BorderAgent::EphemeralKeyManager

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
const char BorderAgent::EphemeralKeyManager::kServiceType[] = "_meshcop-e._udp";
#endif

BorderAgent::EphemeralKeyManager::EphemeralKeyManager(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_FEATURE_ENABLED_BY_DEFAULT
    , mState(kStateStopped)
#else
    , mState(kStateDisabled)
#endif
    , mDtlsTransport(aInstance, kNoLinkSecurity)
    , mCoapDtlsSession(nullptr)
    , mTimer(aInstance)
    , mCallbackTask(aInstance)
{
}

void BorderAgent::EphemeralKeyManager::SetEnabled(bool aEnabled)
{
    if (aEnabled)
    {
        VerifyOrExit(mState == kStateDisabled);
        SetState(kStateStopped);
        Get<BorderAgent>().PostServiceTask();
    }
    else
    {
        VerifyOrExit(mState != kStateDisabled);
        Stop();
        SetState(kStateDisabled);
        Get<BorderAgent>().PostServiceTask();
    }

exit:
    return;
}

Error BorderAgent::EphemeralKeyManager::Start(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort)
{
    Error    error = kErrorNone;
    uint16_t length;

    VerifyOrExit(mState == kStateStopped, error = kErrorInvalidState);

    length = StringLength(aKeyString, kMaxKeyLength + 1);
    VerifyOrExit((length >= kMinKeyLength) && (length <= kMaxKeyLength), error = kErrorInvalidArgs);

    IgnoreError(mDtlsTransport.SetMaxConnectionAttempts(kMaxConnectionAttempts, HandleTransportClosed, this));

    mDtlsTransport.SetAcceptCallback(EphemeralKeyManager::HandleAcceptSession, this);
    mDtlsTransport.SetRemoveSessionCallback(EphemeralKeyManager::HandleRemoveSession, this);

    SuccessOrExit(error = mDtlsTransport.Open());
    SuccessOrExit(error = mDtlsTransport.Bind(aUdpPort));

    SuccessOrExit(
        error = mDtlsTransport.SetPsk(reinterpret_cast<const uint8_t *>(aKeyString), static_cast<uint8_t>(length)));

    aTimeout = Min((aTimeout == 0) ? kDefaultTimeout : aTimeout, kMaxTimeout);
    mTimer.Start(aTimeout);

    LogInfo("Allow ephemeral key for %lu msec on port %u", ToUlong(aTimeout), GetUdpPort());

    SetState(kStateStarted);

exit:
    switch (error)
    {
    case kErrorNone:
        Get<BorderAgent>().mCounters.mEpskcActivations++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcActivated);
#endif
        break;
    case kErrorInvalidState:
        Get<BorderAgent>().mCounters.mEpskcInvalidBaStateErrors++;
        break;
    case kErrorInvalidArgs:
        Get<BorderAgent>().mCounters.mEpskcInvalidArgsErrors++;
        break;
    default:
        Get<BorderAgent>().mCounters.mEpskcStartSecureSessionErrors++;
        break;
    }

    return error;
}

void BorderAgent::EphemeralKeyManager::Stop(void) { Stop(kReasonLocalDisconnect); }

void BorderAgent::EphemeralKeyManager::Stop(DeactivationReason aReason)
{
    switch (mState)
    {
    case kStateStarted:
    case kStateConnected:
    case kStateAccepted:
        break;
    case kStateDisabled:
    case kStateStopped:
        ExitNow();
    }

    LogInfo("Stopping ephemeral key use - reason: %s", DeactivationReasonToString(aReason));
    SetState(kStateStopped);

    mTimer.Stop();
    mDtlsTransport.Close();

    UpdateCountersAndRecordEvent(aReason);

exit:
    return;
}

void BorderAgent::EphemeralKeyManager::UpdateCountersAndRecordEvent(DeactivationReason aReason)
{
    struct ReasonToCounterEventEntry
    {
        DeactivationReason mReason;
        uint8_t            mEvent; // Raw values of `Utils::HistoryTracker::Epskc` enum.
        uint32_t Counters::*mCounterPtr;
    };

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
#define ReasonEntry(kReason, kCounter, kEvent)                       \
    {                                                                \
        kReason, Utils::HistoryTracker::kEvent, &Counters::kCounter, \
    }
#else
#define ReasonEntry(kReason, kCounter, kEvent) \
    {                                          \
        kReason, 0, &Counters::kCounter        \
    }
#endif

    static const ReasonToCounterEventEntry kReasonToCounterEventEntries[] = {
        ReasonEntry(kReasonLocalDisconnect, mEpskcDeactivationClears, kEpskcDeactivatedLocalClose),
        ReasonEntry(kReasonSessionTimeout, mEpskcDeactivationClears, kEpskcDeactivatedSessionTimeout),
        ReasonEntry(kReasonPeerDisconnect, mEpskcDeactivationDisconnects, kEpskcDeactivatedRemoteClose),
        ReasonEntry(kReasonSessionError, mEpskcStartSecureSessionErrors, kEpskcDeactivatedSessionError),
        ReasonEntry(kReasonMaxFailedAttempts, mEpskcDeactivationMaxAttempts, kEpskcDeactivatedMaxAttempts),
        ReasonEntry(kReasonEpskcTimeout, mEpskcDeactivationTimeouts, kEpskcDeactivatedEpskcTimeout),
    };

#undef ReasonEntry

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Utils::HistoryTracker::EpskcEvent event = Utils::HistoryTracker::kEpskcDeactivatedUnknown;
#endif

    for (const ReasonToCounterEventEntry &entry : kReasonToCounterEventEntries)
    {
        if (aReason == entry.mReason)
        {
            (Get<BorderAgent>().mCounters.*(entry.mCounterPtr))++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
            event = static_cast<Utils::HistoryTracker::EpskcEvent>(entry.mEvent);
#endif
            break;
        }
    }

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordEpskcEvent(event);
#endif
}

void BorderAgent::EphemeralKeyManager::SetState(State aState)
{
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    bool isServiceRegistered = ShouldRegisterService();
#endif

    VerifyOrExit(mState != aState);
    LogInfo("Ephemeral key - state: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;
    mCallbackTask.Post();

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    VerifyOrExit(isServiceRegistered != ShouldRegisterService());
    RegisterOrUnregisterService();
#endif

exit:
    return;
}

SecureSession *BorderAgent::EphemeralKeyManager::HandleAcceptSession(void                   *aContext,
                                                                     const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<EphemeralKeyManager *>(aContext)->HandleAcceptSession();
}

BorderAgent::CoapDtlsSession *BorderAgent::EphemeralKeyManager::HandleAcceptSession(void)
{
    CoapDtlsSession *session = nullptr;

    VerifyOrExit(mCoapDtlsSession == nullptr);

    session = CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport);
    VerifyOrExit(session != nullptr);

    mCoapDtlsSession = session;

exit:
    return session;
}

void BorderAgent::EphemeralKeyManager::HandleRemoveSession(void *aContext, SecureSession &aSession)
{
    static_cast<EphemeralKeyManager *>(aContext)->HandleRemoveSession(aSession);
}

void BorderAgent::EphemeralKeyManager::HandleRemoveSession(SecureSession &aSession)
{
    CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(aSession);

    coapSession.Cleanup();
    coapSession.Free();
    mCoapDtlsSession = nullptr;
}

void BorderAgent::EphemeralKeyManager::HandleSessionConnected(void)
{
    SetState(kStateConnected);
    Get<BorderAgent>().mCounters.mEpskcSecureSessionSuccesses++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcConnected);
#endif
}

void BorderAgent::EphemeralKeyManager::HandleSessionDisconnected(SecureSession::ConnectEvent aEvent)
{
    DeactivationReason reason = kReasonUnknown;

    // The ephemeral key can be used once
    VerifyOrExit((mState == kStateConnected) || (mState == kStateAccepted));

    switch (aEvent)
    {
    case SecureSession::kDisconnectedError:
        reason = kReasonSessionError;
        break;
    case SecureSession::kDisconnectedPeerClosed:
        reason = kReasonPeerDisconnect;
        break;
    case SecureSession::kDisconnectedMaxAttempts:
        reason = kReasonMaxFailedAttempts;
        break;
    case SecureSession::kDisconnectedTimeout:
        reason = kReasonSessionTimeout;
        break;
    default:
        break;
    }

    Stop(reason);

exit:
    return;
}

void BorderAgent::EphemeralKeyManager::HandleCommissionerPetitionAccepted(void)
{
    SetState(kStateAccepted);
    Get<BorderAgent>().mCounters.mEpskcCommissionerPetitions++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcPetitioned);
#endif
}

void BorderAgent::EphemeralKeyManager::HandleTimer(void) { Stop(kReasonEpskcTimeout); }

void BorderAgent::EphemeralKeyManager::HandleTask(void) { mCallback.InvokeIfSet(); }

void BorderAgent::EphemeralKeyManager::HandleTransportClosed(void *aContext)
{
    reinterpret_cast<EphemeralKeyManager *>(aContext)->HandleTransportClosed();
}

void BorderAgent::EphemeralKeyManager::HandleTransportClosed(void)
{
    Stop(kReasonMaxFailedAttempts);
    ;
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

bool BorderAgent::EphemeralKeyManager::ShouldRegisterService(void) const
{
    bool shouldRegister = false;

    switch (mState)
    {
    case kStateDisabled:
    case kStateStopped:
        break;
    case kStateStarted:
    case kStateConnected:
    case kStateAccepted:
        shouldRegister = true;
        break;
    }

    return shouldRegister;
}

void BorderAgent::EphemeralKeyManager::RegisterOrUnregisterService(void)
{
    Dnssd::Service service;

    VerifyOrExit(Get<Dnssd>().IsReady());

    service.Clear();
    service.mServiceInstance = Get<BorderAgent>().GetServiceName();
    service.mServiceType     = kServiceType;
    service.mPort            = GetUdpPort();

    if (ShouldRegisterService())
    {
        Get<Dnssd>().RegisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);
    }
    else
    {
        Get<Dnssd>().UnregisterService(service, /* aRequestId */ 0, /* aCallback */ nullptr);
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

const char *BorderAgent::EphemeralKeyManager::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disabled",  // (0) kStateDisabled
        "Stopped",   // (1) kStateStopped
        "Started",   // (2) kStateStarted
        "Connected", // (3) kStateConnected
        "Accepted",  // (4) kStateAccepted
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateDisabled);
        ValidateNextEnum(kStateStopped);
        ValidateNextEnum(kStateStarted);
        ValidateNextEnum(kStateConnected);
        ValidateNextEnum(kStateAccepted);
    };

    return kStateStrings[aState];
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *BorderAgent::EphemeralKeyManager::DeactivationReasonToString(DeactivationReason aReason)
{
    static const char *const kReasonStrings[] = {
        "LocalDisconnect",   // (0) kReasonLocalDisconnect
        "PeerDisconnect",    // (1) kReasonPeerDisconnect
        "SessionError",      // (2) kReasonSessionError
        "SessionTimeout",    // (3) kReasonSessionTimeout
        "MaxFailedAttempts", // (4) kReasonMaxFailedAttempts
        "EpskcTimeout",      // (5) kReasonTimeout
        "Unknown",           // (6) kReasonUnknown
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kReasonLocalDisconnect);
        ValidateNextEnum(kReasonPeerDisconnect);
        ValidateNextEnum(kReasonSessionError);
        ValidateNextEnum(kReasonSessionTimeout);
        ValidateNextEnum(kReasonMaxFailedAttempts);
        ValidateNextEnum(kReasonEpskcTimeout);
        ValidateNextEnum(kReasonUnknown);
    };

    return kReasonStrings[aReason];
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::CoapDtlsSession

BorderAgent::CoapDtlsSession::CoapDtlsSession(Instance &aInstance, Dtls::Transport &aDtlsTransport)
    : Coap::SecureSession(aInstance, aDtlsTransport)
    , mIsActiveCommissioner(false)
    , mTimer(aInstance, HandleTimer, this)
    , mUdpReceiver(HandleUdpReceive, this)
    , mAllocationTime(aInstance.Get<Uptime>().GetUptime())
{
    mCommissionerAloc.InitAsThreadOriginMeshLocal();

    SetResourceHandler(&HandleResource);
    SetConnectCallback(&HandleConnected, this);
}

void BorderAgent::CoapDtlsSession::Cleanup(void)
{
    while (!mForwardContexts.IsEmpty())
    {
        ForwardContext *forwardContext = mForwardContexts.Pop();

        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleCoapResponse, forwardContext));
    }

    mTimer.Stop();
    IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
    Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

    Coap::SecureSession::Cleanup();
}

bool BorderAgent::CoapDtlsSession::HandleResource(CoapBase               &aCoapBase,
                                                  const char             *aUriPath,
                                                  Coap::Message          &aMessage,
                                                  const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<CoapDtlsSession &>(aCoapBase).HandleResource(aUriPath, aMessage, aMessageInfo);
}

bool BorderAgent::CoapDtlsSession::HandleResource(const char             *aUriPath,
                                                  Coap::Message          &aMessage,
                                                  const Ip6::MessageInfo &aMessageInfo)
{
    bool didHandle = true;
    Uri  uri       = UriFromPath(aUriPath);

    switch (uri)
    {
    case kUriCommissionerPetition:
        IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderPetition));
        break;
    case kUriCommissionerKeepAlive:
        HandleTmfCommissionerKeepAlive(aMessage, aMessageInfo);
        break;
    case kUriRelayTx:
        HandleTmfRelayTx(aMessage);
        break;
    case kUriCommissionerGet:
    case kUriActiveGet:
    case kUriPendingGet:
        HandleTmfDatasetGet(aMessage, uri);
        break;
    case kUriProxyTx:
        HandleTmfProxyTx(aMessage);
        break;
    default:
        didHandle = false;
        break;
    }

    return didHandle;
}

void BorderAgent::CoapDtlsSession::HandleConnected(ConnectEvent aEvent, void *aContext)
{
    static_cast<CoapDtlsSession *>(aContext)->HandleConnected(aEvent);
}

void BorderAgent::CoapDtlsSession::HandleConnected(ConnectEvent aEvent)
{
    if (aEvent == kConnected)
    {
        LogInfo("SecureSession connected");
        mTimer.Start(kKeepAliveTimeout);
        Get<BorderAgent>().HandleSessionConnected(*this);
    }
    else
    {
        LogInfo("SecureSession disconnected");
        IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);

        Get<BorderAgent>().HandleSessionDisconnected(*this, aEvent);
    }
}

void BorderAgent::CoapDtlsSession::HandleTmfCommissionerKeepAlive(Coap::Message          &aMessage,
                                                                  const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mIsActiveCommissioner);
    SuccessOrExit(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderKeepAlive));
    mTimer.Start(kKeepAliveTimeout);
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    if (Get<EphemeralKeyManager>().OwnsSession(*this))
    {
        Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcKeepAlive);
    }
#endif

exit:
    return;
}

Error BorderAgent::CoapDtlsSession::ForwardToLeader(const Coap::Message    &aMessage,
                                                    const Ip6::MessageInfo &aMessageInfo,
                                                    Uri                     aUri)
{
    Error                    error = kErrorNone;
    OwnedPtr<ForwardContext> forwardContext;
    Tmf::MessageInfo         messageInfo(GetInstance());
    Coap::Message           *message  = nullptr;
    bool                     petition = false;
    bool                     separate = false;
    OffsetRange              offsetRange;

    switch (aUri)
    {
    case kUriLeaderPetition:
        petition = true;
        separate = true;
        break;
    case kUriLeaderKeepAlive:
        separate = true;
        break;
    default:
        break;
    }

    if (separate)
    {
        SuccessOrExit(error = SendAck(aMessage, aMessageInfo));
    }

    forwardContext.Reset(ForwardContext::Allocate(*this, aMessage, petition, separate));
    VerifyOrExit(!forwardContext.IsNull(), error = kErrorNoBufs);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(aUri);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error =
                      Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleCoapResponse, forwardContext.Get()));

    // Release the ownership of `forwardContext` since `SendMessage()`
    // will own it. We take back ownership from `HandleCoapResponse()`
    // callback.

    mForwardContexts.Push(*forwardContext.Release());

    LogInfo("Forwarded request to leader on %s", PathForUri(aUri));

exit:
    LogWarnOnError(error, "forward to leader");

    if (error != kErrorNone)
    {
        FreeMessage(message);
        SendErrorMessage(aMessage, separate, error);
    }

    return error;
}

void BorderAgent::CoapDtlsSession::HandleCoapResponse(void                *aContext,
                                                      otMessage           *aMessage,
                                                      const otMessageInfo *aMessageInfo,
                                                      otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    OwnedPtr<ForwardContext> forwardContext(static_cast<ForwardContext *>(aContext));

    forwardContext->mSession.HandleCoapResponse(*forwardContext.Get(), AsCoapMessagePtr(aMessage), aResult);
}

void BorderAgent::CoapDtlsSession::HandleCoapResponse(const ForwardContext &aForwardContext,
                                                      const Coap::Message  *aResponse,
                                                      Error                 aResult)
{
    Coap::Message *message = nullptr;
    Error          error;

    IgnoreError(mForwardContexts.Remove(aForwardContext));

    SuccessOrExit(error = aResult);
    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aForwardContext.mPetition && aResponse->GetCode() == Coap::kCodeChanged)
    {
        uint8_t state;

        SuccessOrExit(error = Tlv::Find<StateTlv>(*aResponse, state));

        if (state == StateTlv::kAccept)
        {
            uint16_t sessionId;

            SuccessOrExit(error = Tlv::Find<CommissionerSessionIdTlv>(*aResponse, sessionId));

            Get<Mle::Mle>().GetCommissionerAloc(sessionId, mCommissionerAloc.GetAddress());
            Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);
            IgnoreError(Get<Ip6::Udp>().AddReceiver(mUdpReceiver));
            mIsActiveCommissioner = true;
            Get<BorderAgent>().HandleCommissionerPetitionAccepted(*this);

            LogInfo("Commissioner accepted - SessionId:%u ALOC:%s", sessionId,
                    mCommissionerAloc.GetAddress().ToString().AsCString());
        }
        else
        {
            LogInfo("Commissioner rejected");
        }
    }

    SuccessOrExit(error = aForwardContext.ToHeader(*message, aResponse->GetCode()));

    if (aResponse->GetLength() > aResponse->GetOffset())
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    SuccessOrExit(error = ForwardToCommissioner(*message, *aResponse));

exit:

    if (error != kErrorNone)
    {
        FreeMessage(message);

        LogWarn("Commissioner request[%u] failed: %s", aForwardContext.mMessageId, ErrorToString(error));

        SendErrorMessage(aForwardContext, error);
    }
}

bool BorderAgent::CoapDtlsSession::HandleUdpReceive(void                *aContext,
                                                    const otMessage     *aMessage,
                                                    const otMessageInfo *aMessageInfo)
{
    return static_cast<CoapDtlsSession *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

bool BorderAgent::CoapDtlsSession::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                     error     = kErrorNone;
    Coap::Message            *message   = nullptr;
    bool                      didHandle = false;
    ExtendedTlv               extTlv;
    UdpEncapsulationTlvHeader udpEncapHeader;
    OffsetRange               offsetRange;

    VerifyOrExit(aMessageInfo.GetSockAddr() == mCommissionerAloc.GetAddress());

    didHandle = true;

    VerifyOrExit(aMessage.GetLength() > 0);

    message = NewPriorityNonConfirmablePostMessage(kUriProxyRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    extTlv.SetType(Tlv::kUdpEncapsulation);
    extTlv.SetLength(sizeof(UdpEncapsulationTlvHeader) + offsetRange.GetLength());

    udpEncapHeader.SetSourcePort(aMessageInfo.GetPeerPort());
    udpEncapHeader.SetDestinationPort(aMessageInfo.GetSockPort());

    SuccessOrExit(error = message->Append(extTlv));
    SuccessOrExit(error = message->Append(udpEncapHeader));
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    SuccessOrExit(error = Tlv::Append<Ip6AddressTlv>(*message, aMessageInfo.GetPeerAddr()));

    SuccessOrExit(error = SendMessage(*message));

    LogInfo("Sent ProxyRx (c/ur) to commissioner");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send ProxyRx (c/ur)");

    return didHandle;
}

Error BorderAgent::CoapDtlsSession::ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage)
{
    Error       error = kErrorNone;
    OffsetRange offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = aForwardMessage.AppendBytesFromMessage(aMessage, offsetRange));

    SuccessOrExit(error = SendMessage(aForwardMessage));

    LogInfo("Sent to commissioner");

exit:
    LogWarnOnError(error, "send to commissioner");
    return error;
}

void BorderAgent::CoapDtlsSession::SendErrorMessage(const ForwardContext &aForwardContext, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = aForwardContext.ToHeader(*message, CoapCodeFromError(aError)));
    SuccessOrExit(error = SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
}

void BorderAgent::CoapDtlsSession::SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aRequest.IsNonConfirmable() || aSeparate)
    {
        message->Init(Coap::kTypeNonConfirmable, CoapCodeFromError(aError));
    }
    else
    {
        message->Init(Coap::kTypeAck, CoapCodeFromError(aError));
    }

    if (!aSeparate)
    {
        message->SetMessageId(aRequest.GetMessageId());
    }

    SuccessOrExit(error = message->SetTokenFromMessage(aRequest));

    SuccessOrExit(error = SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send error CoAP message");
}

void BorderAgent::CoapDtlsSession::HandleTmfProxyTx(Coap::Message &aMessage)
{
    Error                     error   = kErrorNone;
    Message                  *message = nullptr;
    Ip6::MessageInfo          messageInfo;
    OffsetRange               offsetRange;
    UdpEncapsulationTlvHeader udpEncapHeader;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, Tlv::kUdpEncapsulation, offsetRange));

    SuccessOrExit(error = aMessage.Read(offsetRange, udpEncapHeader));
    offsetRange.AdvanceOffset(sizeof(UdpEncapsulationTlvHeader));

    VerifyOrExit(udpEncapHeader.GetSourcePort() > 0 && udpEncapHeader.GetDestinationPort() > 0, error = kErrorDrop);

    VerifyOrExit((message = Get<Ip6::Udp>().NewMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockPort(udpEncapHeader.GetSourcePort());
    messageInfo.SetSockAddr(mCommissionerAloc.GetAddress());
    messageInfo.SetPeerPort(udpEncapHeader.GetDestinationPort());

    SuccessOrExit(error = Tlv::Find<Ip6AddressTlv>(aMessage, messageInfo.GetPeerAddr()));

    SuccessOrExit(error = Get<Ip6::Udp>().SendDatagram(*message, messageInfo));

    LogInfo("Proxy transmit sent to %s", messageInfo.GetPeerAddr().ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send proxy stream");
}

void BorderAgent::CoapDtlsSession::HandleTmfRelayTx(Coap::Message &aMessage)
{
    Error            error = kErrorNone;
    uint16_t         joinerRouterRloc;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    OffsetRange      offsetRange;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest());

    SuccessOrExit(error = Tlv::Find<JoinerRouterLocatorTlv>(aMessage, joinerRouterRloc));

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriRelayTx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aMessage);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offsetRange));

    messageInfo.SetSockAddrToRlocPeerAddrTo(joinerRouterRloc);
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent to joiner router request on RelayTx (c/tx)");

exit:
    FreeMessageOnError(message, error);
    LogWarnOnError(error, "send to joiner router request RelayTx (c/tx)");
}

void BorderAgent::CoapDtlsSession::HandleTmfDatasetGet(Coap::Message &aMessage, Uri aUri)
{
    Error          error    = kErrorNone;
    Coap::Message *response = nullptr;

    // When processing `MGMT_GET` request directly on Border Agent,
    // the Security Policy flags (O-bit) should be ignored to allow
    // the commissioner candidate to get the full Operational Dataset.

    switch (aUri)
    {
    case kUriActiveGet:
        response = Get<ActiveDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        Get<BorderAgent>().mCounters.mMgmtActiveGets++;
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        if (Get<EphemeralKeyManager>().OwnsSession(*this))
        {
            Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcRetrievedActiveDataset);
        }
#endif
        break;

    case kUriPendingGet:
        response = Get<PendingDatasetManager>().ProcessGetRequest(aMessage, DatasetManager::kIgnoreSecurityPolicyFlags);
        Get<BorderAgent>().mCounters.mMgmtPendingGets++;
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE && OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        if (Get<EphemeralKeyManager>().OwnsSession(*this))
        {
            Get<Utils::HistoryTracker>().RecordEpskcEvent(Utils::HistoryTracker::kEpskcRetrievedPendingDataset);
        }
#endif
        break;

    case kUriCommissionerGet:
        response = Get<NetworkData::Leader>().ProcessCommissionerGetRequest(aMessage);
        break;

    default:
        break;
    }

    VerifyOrExit(response != nullptr, error = kErrorParse);

    SuccessOrExit(error = SendMessage(*response));

    LogInfo("Sent %s response to non-active commissioner", PathForUri(aUri));

exit:
    LogWarnOnError(error, "send Active/Pending/CommissionerGet response");
    FreeMessageOnError(response, error);
}

void BorderAgent::CoapDtlsSession::HandleTimer(Timer &aTimer)
{
    static_cast<CoapDtlsSession *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void BorderAgent::CoapDtlsSession::HandleTimer(void)
{
    if (IsConnected())
    {
        LogInfo("Session timed out - disconnecting");
        DisconnectTimeout();
    }
}

//----------------------------------------------------------------------------------------------------------------------
// `BorderAgent::CoapDtlsSession::ForwardContext`

BorderAgent::CoapDtlsSession::ForwardContext::ForwardContext(CoapDtlsSession     &aSession,
                                                             const Coap::Message &aMessage,
                                                             bool                 aPetition,
                                                             bool                 aSeparate)
    : mSession(aSession)
    , mMessageId(aMessage.GetMessageId())
    , mPetition(aPetition)
    , mSeparate(aSeparate)
    , mTokenLength(aMessage.GetTokenLength())
    , mType(aMessage.GetType())
{
    memcpy(mToken, aMessage.GetToken(), mTokenLength);
}

Error BorderAgent::CoapDtlsSession::ForwardContext::ToHeader(Coap::Message &aMessage, uint8_t aCode) const
{
    if ((mType == Coap::kTypeNonConfirmable) || mSeparate)
    {
        aMessage.Init(Coap::kTypeNonConfirmable, static_cast<Coap::Code>(aCode));
    }
    else
    {
        aMessage.Init(Coap::kTypeAck, static_cast<Coap::Code>(aCode));
    }

    if (!mSeparate)
    {
        aMessage.SetMessageId(mMessageId);
    }

    return aMessage.SetToken(mToken, mTokenLength);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
