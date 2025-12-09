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
 *   This file implements the Border Agent Ephemeral Key Manager.
 */

#include "border_agent_ephemeral_key.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

#include "instance/instance.hpp"
#include "utils/verhoeff_checksum.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

RegisterLogModule("BorderAgent");

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
const char EphemeralKeyManager::kServiceType[] = "_meshcop-e._udp";
#endif

EphemeralKeyManager::EphemeralKeyManager(Instance &aInstance)
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

void EphemeralKeyManager::SetEnabled(bool aEnabled)
{
    if (aEnabled)
    {
        VerifyOrExit(mState == kStateDisabled);
        SetState(kStateStopped);
    }
    else
    {
        VerifyOrExit(mState != kStateDisabled);
        Stop();
        SetState(kStateDisabled);
    }

    Get<TxtData>().Refresh();

exit:
    return;
}

Error EphemeralKeyManager::Start(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort)
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
        Get<Manager>().mCounters.mEpskcActivations++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
        Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcActivated);
#endif
        break;
    case kErrorInvalidState:
        Get<Manager>().mCounters.mEpskcInvalidBaStateErrors++;
        break;
    case kErrorInvalidArgs:
        Get<Manager>().mCounters.mEpskcInvalidArgsErrors++;
        break;
    default:
        Get<Manager>().mCounters.mEpskcStartSecureSessionErrors++;
        break;
    }

    return error;
}

void EphemeralKeyManager::Stop(void) { Stop(kReasonLocalDisconnect); }

void EphemeralKeyManager::Stop(DeactivationReason aReason)
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

void EphemeralKeyManager::UpdateCountersAndRecordEvent(DeactivationReason aReason)
{
    struct ReasonToCounterEventEntry
    {
        DeactivationReason mReason;
        uint8_t            mEvent; // Raw values of `HistoryTracker::Local::Epskc` enum.
        uint32_t Counters::*mCounterPtr;
    };

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
#define ReasonEntry(kReason, kCounter, kEvent) \
    {                                          \
        kReason,                               \
        HistoryTracker::Local::kEvent,         \
        &Counters::kCounter,                   \
    }
#else
#define ReasonEntry(kReason, kCounter, kEvent) {kReason, 0, &Counters::kCounter}
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
    HistoryTracker::EpskcEvent event = HistoryTracker::Local::kEpskcDeactivatedUnknown;
#endif

    for (const ReasonToCounterEventEntry &entry : kReasonToCounterEventEntries)
    {
        if (aReason == entry.mReason)
        {
            (Get<Manager>().mCounters.*(entry.mCounterPtr))++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
            event = static_cast<HistoryTracker::EpskcEvent>(entry.mEvent);
#endif
            break;
        }
    }

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<HistoryTracker::Local>().RecordEpskcEvent(event);
#endif
}

void EphemeralKeyManager::SetState(State aState)
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

SecureSession *EphemeralKeyManager::HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<EphemeralKeyManager *>(aContext)->HandleAcceptSession();
}

Manager::CoapDtlsSession *EphemeralKeyManager::HandleAcceptSession(void)
{
    CoapDtlsSession *session = nullptr;

    VerifyOrExit(mCoapDtlsSession == nullptr);

    session = CoapDtlsSession::Allocate(GetInstance(), mDtlsTransport);
    VerifyOrExit(session != nullptr);

    mCoapDtlsSession = session;

exit:
    return session;
}

void EphemeralKeyManager::HandleRemoveSession(void *aContext, SecureSession &aSession)
{
    static_cast<EphemeralKeyManager *>(aContext)->HandleRemoveSession(aSession);
}

void EphemeralKeyManager::HandleRemoveSession(SecureSession &aSession)
{
    CoapDtlsSession &coapSession = static_cast<CoapDtlsSession &>(aSession);

    coapSession.Cleanup();
    coapSession.Free();
    mCoapDtlsSession = nullptr;
}

void EphemeralKeyManager::HandleSessionConnected(void)
{
    SetState(kStateConnected);
    Get<Manager>().mCounters.mEpskcSecureSessionSuccesses++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcConnected);
#endif
}

void EphemeralKeyManager::HandleSessionDisconnected(SecureSession::ConnectEvent aEvent)
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

void EphemeralKeyManager::HandleCommissionerPetitionAccepted(void)
{
    SetState(kStateAccepted);
    Get<Manager>().mCounters.mEpskcCommissionerPetitions++;
#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<HistoryTracker::Local>().RecordEpskcEvent(HistoryTracker::Local::kEpskcPetitioned);
#endif
}

void EphemeralKeyManager::HandleTimer(void) { Stop(kReasonEpskcTimeout); }

void EphemeralKeyManager::HandleTask(void) { mCallback.InvokeIfSet(); }

void EphemeralKeyManager::HandleTransportClosed(void *aContext)
{
    reinterpret_cast<EphemeralKeyManager *>(aContext)->HandleTransportClosed();
}

void EphemeralKeyManager::HandleTransportClosed(void) { Stop(kReasonMaxFailedAttempts); }

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE

bool EphemeralKeyManager::ShouldRegisterService(void) const
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

void EphemeralKeyManager::RegisterOrUnregisterService(void)
{
    Dnssd::Service service;

    VerifyOrExit(Get<Dnssd>().IsReady());

    service.Clear();
    service.mServiceInstance = Get<Manager>().GetServiceName();
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

const char *EphemeralKeyManager::StateToString(State aState)
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

const char *EphemeralKeyManager::DeactivationReasonToString(DeactivationReason aReason)
{
    static const char *const kReasonStrings[] = {
        "LocalDisconnect",   // (0) kReasonLocalDisconnect
        "PeerDisconnect",    // (1) kReasonPeerDisconnect
        "SessionError",      // (2) kReasonSessionError
        "SessionTimeout",    // (3) kReasonSessionTimeout
        "MaxFailedAttempts", // (4) kReasonMaxFailedAttempts
        "EpskcTimeout",      // (5) kReasonEpskcTimeout
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

//---------------------------------------------------------------------------------------------------------------------
// EphemeralKeyManager::Tap

#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

Error EphemeralKeyManager::Tap::GenerateRandom(void)
{
    Error error;
    char  checksum;

    ClearAllBytes(mTap);

    for (uint8_t index = 0; index < kLength - 1; index++)
    {
        SuccessOrExit(error = GenerateRandomDigit(mTap[index]));
    }

    IgnoreError(Utils::VerhoeffChecksum::Calculate(mTap, checksum));
    mTap[kLength - 1] = checksum;

exit:
    return error;
}

Error EphemeralKeyManager::Tap::GenerateRandomDigit(char &aChar)
{
    static constexpr uint8_t kMaxValue = 250;

    Error   error;
    uint8_t byte;

    // To ensure uniform random distribution and avoid bias toward
    // certain digit values, we ignore random `uint8` values of 250 or
    // larger (i.e., values in the range [250-255]). This ensures the
    // random `byte` is uniformly distributed in `[0-249]`, which,
    // when `% 10`, gives us a uniform probability of `[0-9]` values.

    do
    {
        SuccessOrExit(error = Random::Crypto::Fill(byte));
    } while (byte >= kMaxValue);

    aChar = '0' + (byte % 10);

exit:
    return error;
}

Error EphemeralKeyManager::Tap::Validate(void) const
{
    Error error;

    VerifyOrExit(StringLength(mTap, kLength + 1) == kLength, error = kErrorInvalidArgs);
    error = Utils::VerhoeffChecksum::Validate(mTap);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

} // namespace BorderAgent
} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
