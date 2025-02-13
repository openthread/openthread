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
 *   This file includes definitions for the BorderAgent role.
 */

#ifndef BORDER_AGENT_HPP_
#define BORDER_AGENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include <openthread/border_agent.h>

#include "common/as_core_type.hpp"
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owned_ptr.hpp"
#include "common/tasklet.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/secure_transport.hpp"
#include "net/socket.hpp"
#include "net/udp6.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

namespace MeshCoP {

#if !OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
#error "Border Agent feature requires `OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE`"
#endif

#if !OPENTHREAD_CONFIG_UPTIME_ENABLE
#error "Border Agent feature requires `OPENTHREAD_CONFIG_UPTIME_ENABLE`"
#endif

class BorderAgent : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class Tmf::Agent;

    class CoapDtlsSession;

public:
    typedef otBorderAgentId          Id;          ///< Border Agent ID.
    typedef otBorderAgentCounters    Counters;    ///< Border Agent Counters.
    typedef otBorderAgentSessionInfo SessionInfo; ///< A session info.

    /**
     * Represents an iterator for secure sessions.
     */
    class SessionIterator : public otBorderAgentSessionIterator
    {
    public:
        /**
         * Initializes the `SessionIterator`.
         *
         * @param[in] aInstance  The OpenThread instance.
         */
        void Init(Instance &aInstance);

        /**
         * Retrieves the next session information.
         *
         * @param[out] aSessionInfo     A `SessionInfo` to populate.
         *
         * @retval kErrorNone        Successfully retrieved the next session. @p aSessionInfo is updated.
         * @retval kErrorNotFound    No more sessions are available. The end of the list has been reached.
         */
        Error GetNextSessionInfo(SessionInfo &aSessionInfo);

    private:
        CoapDtlsSession *GetSession(void) const { return static_cast<CoapDtlsSession *>(mPtr); }
        void             SetSession(CoapDtlsSession *aSession) { mPtr = aSession; }
        uint64_t         GetInitTime(void) const { return mData; }
        void             SetInitTime(uint64_t aInitTime) { mData = aInitTime; }
    };

    /**
     * Initializes the `BorderAgent` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit BorderAgent(Instance &aInstance);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    /**
     * Gets the randomly generated Border Agent ID.
     *
     * The ID is saved in persistent storage and survives reboots. The typical use case of the ID is to
     * be published in the MeshCoP mDNS service as the `id` TXT value for the client to identify this
     * Border Router/Agent device.
     *
     * @param[out] aId  Reference to return the Border Agent ID.
     *
     * @retval kErrorNone  If successfully retrieved the Border Agent ID.
     * @retval ...         If failed to retrieve the Border Agent ID.
     */
    Error GetId(Id &aId);

    /**
     * Sets the Border Agent ID.
     *
     * The Border Agent ID will be saved in persistent storage and survive reboots. It's required
     * to set the ID only once after factory reset. If the ID has never been set by calling this
     * method, a random ID will be generated and returned when `GetId()` is called.
     *
     * @param[out] aId  specifies the Border Agent ID.
     *
     * @retval kErrorNone  If successfully set the Border Agent ID.
     * @retval ...         If failed to set the Border Agent ID.
     */
    Error SetId(const Id &aId);
#endif

    /**
     * Gets the UDP port of this service.
     *
     * @returns  UDP port number.
     */
    uint16_t GetUdpPort(void) const;

    /**
     * Indicates whether the Border Agent service is running.
     *
     * @retval TRUE  Border Agent service is running.
     * @retval FALSE Border Agent service is not running.
     */
    bool IsRunning(void) const { return mIsRunning; }

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    /**
     * Manages the ephemeral key use by Border Agent.
     */
    class EphemeralKeyManager : public InstanceLocator, private NonCopyable
    {
        friend class BorderAgent;

    public:
        static constexpr uint16_t kMinKeyLength   = OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH;      ///< Min key len.
        static constexpr uint16_t kMaxKeyLength   = OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH;      ///< Max key len.
        static constexpr uint32_t kDefaultTimeout = OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT; //< Default timeout.
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
         * @returns The `EmpheralKeyManager` state.
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

        enum StopReason : uint8_t
        {
            kReasonLocalDisconnect,
            kReasonPeerDisconnect,
            kReasonSessionError,
            kReasonMaxFailedAttempts,
            kReasonTimeout,
            kReasonUnknown,
        };

        explicit EphemeralKeyManager(Instance &aInstance);

        void SetState(State aState);
        void Stop(StopReason aReason);
        void HandleTimer(void);
        void HandleTask(void);
        bool OwnsSession(CoapDtlsSession &aSession) const { return mCoapDtlsSession == &aSession; }
        void HandleSessionConnected(void);
        void HandleSessionDisconnected(SecureSession::ConnectEvent aEvent);
        void HandleCommissionerPetitionAccepted(void);

        // Session or Transport callbacks
        static SecureSession *HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo);
        CoapDtlsSession      *HandleAcceptSession(void);
        static void           HandleRemoveSession(void *aContext, SecureSession &aSession);
        void                  HandleRemoveSession(SecureSession &aSession);
        static void           HandleTransportClosed(void *aContext);
        void                  HandleTransportClosed(void);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
        static const char *StopReasonToString(StopReason aReason);
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

    /**
     * Gets the `EphemeralKeyManager` instance.
     *
     * @returns A reference to the `EphemeralKeyManager`.
     */
    EphemeralKeyManager &GetEphemeralKeyManager(void) { return mEphemeralKeyManager; }

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

    /**
     * Gets the set of border agent counters.
     *
     * @returns The border agent counters.
     */
    const Counters &GetCounters(void) { return mCounters; }

private:
    static constexpr uint16_t kUdpPort          = OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT;
    static constexpr uint32_t kKeepAliveTimeout = 50 * 1000; // Timeout to reject a commissioner (in msec)

    class CoapDtlsSession : public Coap::SecureSession, public Heap::Allocatable<CoapDtlsSession>
    {
        friend Heap::Allocatable<CoapDtlsSession>;

    public:
        Error    ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage);
        void     Cleanup(void);
        bool     IsActiveCommissioner(void) const { return mIsActiveCommissioner; }
        uint64_t GetAllocationTime(void) const { return mAllocationTime; }

    private:
        class ForwardContext : public ot::LinkedListEntry<ForwardContext>,
                               public Heap::Allocatable<ForwardContext>,
                               private ot::NonCopyable
        {
            friend class Heap::Allocatable<ForwardContext>;

        public:
            Error ToHeader(Coap::Message &aMessage, uint8_t aCode) const;

            CoapDtlsSession &mSession;
            ForwardContext  *mNext;
            uint16_t         mMessageId;
            bool             mPetition : 1;
            bool             mSeparate : 1;
            uint8_t          mTokenLength : 4;
            uint8_t          mType : 2;
            uint8_t          mToken[Coap::Message::kMaxTokenLength];

        private:
            ForwardContext(CoapDtlsSession &aSession, const Coap::Message &aMessage, bool aPetition, bool aSeparate);
        };

        CoapDtlsSession(Instance &aInstance, Dtls::Transport &aDtlsTransport);

        void  HandleTmfCommissionerKeepAlive(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        void  HandleTmfRelayTx(Coap::Message &aMessage);
        void  HandleTmfProxyTx(Coap::Message &aMessage);
        void  HandleTmfDatasetGet(Coap::Message &aMessage, Uri aUri);
        Error ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri);
        void  SendErrorMessage(const ForwardContext &aForwardContext, Error aError);
        void  SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError);

        static void HandleConnected(ConnectEvent aEvent, void *aContext);
        void        HandleConnected(ConnectEvent aEvent);
        static void HandleCoapResponse(void                *aContext,
                                       otMessage           *aMessage,
                                       const otMessageInfo *aMessageInfo,
                                       otError              aResult);
        void HandleCoapResponse(const ForwardContext &aForwardContext, const Coap::Message *aResponse, Error aResult);
        static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
        bool        HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        static bool HandleResource(CoapBase               &aCoapBase,
                                   const char             *aUriPath,
                                   Coap::Message          &aMessage,
                                   const Ip6::MessageInfo &aMessageInfo);
        bool        HandleResource(const char *aUriPath, Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);

        bool                       mIsActiveCommissioner;
        LinkedList<ForwardContext> mForwardContexts;
        TimerMilliContext          mTimer;
        Ip6::Udp::Receiver         mUdpReceiver;
        Ip6::Netif::UnicastAddress mCommissionerAloc;
        uint64_t                   mAllocationTime;
    };

    void Start(void);
    void Stop(void);
    void HandleNotifierEvents(Events aEvents);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static SecureSession *HandleAcceptSession(void *aContext, const Ip6::MessageInfo &aMessageInfo);
    CoapDtlsSession      *HandleAcceptSession(void);
    static void           HandleRemoveSession(void *aContext, SecureSession &aSession);
    void                  HandleRemoveSession(SecureSession &aSession);
    CoapDtlsSession      *FindActiveCommissionerSession(void);

    void HandleSessionConnected(CoapDtlsSession &aSession);
    void HandleSessionDisconnected(CoapDtlsSession &aSession, CoapDtlsSession::ConnectEvent aEvent);
    void HandleCommissionerPetitionAccepted(CoapDtlsSession &aSession);

    static Coap::Message::Code CoapCodeFromError(Error aError);

    bool            mIsRunning;
    Dtls::Transport mDtlsTransport;
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    Id   mId;
    bool mIdInitialized;
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    EphemeralKeyManager mEphemeralKeyManager;
#endif
    Counters mCounters;
};

DeclareTmfHandler(BorderAgent, kUriRelayRx);

} // namespace MeshCoP

DefineCoreType(otBorderAgentId, MeshCoP::BorderAgent::Id);
DefineCoreType(otBorderAgentSessionIterator, MeshCoP::BorderAgent::SessionIterator);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
DefineMapEnum(otBorderAgentEphemeralKeyState, MeshCoP::BorderAgent::EphemeralKeyManager::State);
#endif

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#endif // BORDER_AGENT_HPP_
