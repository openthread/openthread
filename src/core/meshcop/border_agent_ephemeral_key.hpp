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
 *   This file includes definitions for the Border Agent Ephemeral Key Manager.
 */

#ifndef BORDER_AGENT_EPHEMERAL_KEY_HPP_
#define BORDER_AGENT_EPHEMERAL_KEY_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

#include <openthread/border_agent.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/tasklet.hpp"
#include "common/timer.hpp"
#include "meshcop/border_agent.hpp"
#include "meshcop/secure_transport.hpp"

namespace ot {
namespace MeshCoP {
namespace BorderAgent {

/**
 * Manages the ephemeral key use by Border Agent.
 */
class EphemeralKeyManager : public InstanceLocator, private NonCopyable
{
    friend class Manager;

public:
    static constexpr uint16_t kMinKeyLength   = OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH;      ///< Min key len.
    static constexpr uint16_t kMaxKeyLength   = OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH;      ///< Max key len.
    static constexpr uint32_t kDefaultTimeout = OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT; ///< Default timeout.
    static constexpr uint32_t kMaxTimeout     = OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT;     ///< Max timeout.

    typedef otBorderAgentEphemeralKeyCallback CallbackHandler; ///< Callback function pointer.

    /**
     * Represents the state of the `EphemeralKeyManager`.
     */
    enum State : uint8_t
    {
        kStateDisabled  = OT_BORDER_AGENT_STATE_DISABLED,  ///< Ephemeral key feature is disabled.
        kStateStopped   = OT_BORDER_AGENT_STATE_STOPPED,   ///< Enabled, but the key is not set and started.
        kStateStarted   = OT_BORDER_AGENT_STATE_STARTED,   ///< Key is set and listening to accept connection.
        kStateConnected = OT_BORDER_AGENT_STATE_CONNECTED, ///< Session connected, not full commissioner.
        kStateAccepted  = OT_BORDER_AGENT_STATE_ACCEPTED,  ///< Session connected and accepted as full commissioner.
    };

    /**
     * Initializes the `EphemeralKeyManager`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit EphemeralKeyManager(Instance &aInstance);

    /**
     * Enables/disables Ephemeral Key Manager.
     *
     * If this method is called to disable, while an an ephemeral key is in use, the ephemeral key use will
     * be stopped (as if `Stop()` is called).
     *
     * @param[in] aEnabled  Whether to enable or disable.
     */
    void SetEnabled(bool aEnabled);

    /**
     * Starts using an ephemeral key for a given timeout duration.
     *
     * An ephemeral key can only be set when `GetState()` is `kStateStopped`. Otherwise, `kErrorInvalidState` is
     * returned. This means that setting the ephemeral key again while a previously set key is still in use will
     * fail. Callers can stop the previous key by calling `Stop()` before starting with a new key.
     *
     * The given @p aKeyString is used directly as the ephemeral PSK (excluding the trailing null `\0` character).
     * Its length must be between `kMinKeyLength` and `kMaxKeyLength`, inclusive.
     *
     * The ephemeral key can be used only once by an external commissioner candidate to establish a secure session.
     * After the commissioner candidate disconnects, the use of the ephemeral key is stopped. If the timeout
     * expires, the use of the ephemeral key is also stopped, and any established session using the key is
     * immediately disconnected.
     *
     * @param[in] aKeyString   The ephemeral key.
     * @param[in] aTimeout     The timeout duration, in milliseconds, to use the ephemeral key.
     *                         If zero, the default `kDefaultTimeout` value is used. If the timeout value is
     *                         larger than `kMaxTimeout`, the maximum value is used instead.
     * @param[in] aUdpPort     The UDP port to use with the ephemeral key. If the UDP port is zero, an ephemeral
     *                         port is used. `GetUdpPort()` returns the current UDP port being used.
     *
     * @retval kErrorNone           Successfully started using the ephemeral key.
     * @retval kErrorInvalidState   A previously set ephemeral key is still in use or feature is disabled.
     * @retval kErrorInvalidArgs    The given @p aKeyString is not valid.
     * @retval kErrorFailed         Failed to start (e.g., it could not bind to the given UDP port).
     */
    Error Start(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort);

    /**
     * Stops the ephemeral key use and disconnects any established secure session using it.
     *
     * If there is no ephemeral key in use, calling this method has no effect.
     */
    void Stop(void);

    /**
     * Gets the state of ephemeral key use and its session.
     *
     * @returns The `EphemeralKeyManager` state.
     */
    State GetState(void) const { return mState; }

    /**
     * Gets the UDP port used by ephemeral key DTLS secure transport.
     *
     * @returns  UDP port number.
     */
    uint16_t GetUdpPort(void) const { return mDtlsTransport.GetUdpPort(); }

    /**
     * Sets the callback.
     *
     * @param[in] aCallback   The callback function pointer.
     * @param[in] aContext    The context associated and used with callback handler.
     */
    void SetCallback(CallbackHandler aCallback, void *aContext) { mCallback.Set(aCallback, aContext); }

    /**
     * Converts a given `State` to human-readable string.
     *
     * @param[in] aState  The state to convert.
     *
     * @returns The string corresponding to @p aState.
     */
    static const char *StateToString(State aState);

private:
    static constexpr uint16_t kMaxConnectionAttempts = 10;

    static_assert(kMaxKeyLength <= Dtls::Transport::kPskMaxLength, "Max e-key len is larger than max PSK len");

    using CoapDtlsSession = Manager::CoapDtlsSession;
    using Counters        = Manager::Counters;

    enum DeactivationReason : uint8_t
    {
        kReasonLocalDisconnect,
        kReasonPeerDisconnect,
        kReasonSessionError,
        kReasonSessionTimeout,
        kReasonMaxFailedAttempts,
        kReasonEpskcTimeout,
        kReasonUnknown,
    };

    void SetState(State aState);
    void Stop(DeactivationReason aReason);
    void HandleTimer(void);
    void HandleTask(void);
    bool OwnsSession(CoapDtlsSession &aSession) const { return mCoapDtlsSession == &aSession; }
    void HandleSessionConnected(void);
    void HandleSessionDisconnected(SecureSession::ConnectEvent aEvent);
    void HandleCommissionerPetitionAccepted(void);
    void UpdateCountersAndRecordEvent(DeactivationReason aReason);
#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    bool ShouldRegisterService(void) const;
    void RegisterOrUnregisterService(void);
#endif

    // Session or Transport callbacks
    static SecureSession *HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo);
    CoapDtlsSession      *HandleAcceptSession(void);
    static void           HandleRemoveSession(void *aContext, SecureSession &aSession);
    void                  HandleRemoveSession(SecureSession &aSession);
    static void           HandleTransportClosed(void *aContext);
    void                  HandleTransportClosed(void);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *DeactivationReasonToString(DeactivationReason aReason);
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_MESHCOP_SERVICE_ENABLE
    static const char kServiceType[];
#endif

    using TimeoutTimer = TimerMilliIn<EphemeralKeyManager, &EphemeralKeyManager::HandleTimer>;
    using CallbackTask = TaskletIn<EphemeralKeyManager, &EphemeralKeyManager::HandleTask>;

    State                     mState;
    Dtls::Transport           mDtlsTransport;
    CoapDtlsSession          *mCoapDtlsSession;
    TimeoutTimer              mTimer;
    CallbackTask              mCallbackTask;
    Callback<CallbackHandler> mCallback;
};

} // namespace BorderAgent
} // namespace MeshCoP

DefineMapEnum(otBorderAgentEphemeralKeyState, MeshCoP::BorderAgent::EphemeralKeyManager::State);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE && OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

#endif // BORDER_AGENT_EPHEMERAL_KEY_HPP_
