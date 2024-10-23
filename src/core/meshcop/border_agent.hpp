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
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/tasklet.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/secure_transport.hpp"
#include "net/udp6.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

namespace MeshCoP {

class BorderAgent : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class Tmf::Agent;
    friend class Tmf::SecureAgent;

public:
    /**
     * Minimum length of the ephemeral key string.
     */
    static constexpr uint16_t kMinEphemeralKeyLength = OT_BORDER_AGENT_MIN_EPHEMERAL_KEY_LENGTH;

    /**
     * Maximum length of the ephemeral key string.
     */
    static constexpr uint16_t kMaxEphemeralKeyLength = OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_LENGTH;

    /**
     * Default ephemeral key timeout interval in milliseconds.
     */
    static constexpr uint32_t kDefaultEphemeralKeyTimeout = OT_BORDER_AGENT_DEFAULT_EPHEMERAL_KEY_TIMEOUT;

    /**
     * Maximum ephemeral key timeout interval in milliseconds.
     */
    static constexpr uint32_t kMaxEphemeralKeyTimeout = OT_BORDER_AGENT_MAX_EPHEMERAL_KEY_TIMEOUT;

    typedef otBorderAgentId Id; ///< Border Agent ID.

    /**
     * Defines the Border Agent state.
     */
    enum State : uint8_t
    {
        kStateStopped,   ///< Stopped/disabled.
        kStateStarted,   ///< Started and listening for connections.
        kStateConnected, ///< Connected to an external commissioner candidate, petition pending.
        kStateAccepted,  ///< Connected to and accepted an external commissioner.
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
     * Starts the Border Agent service.
     */
    void Start(void) { IgnoreError(Start(kUdpPort)); }

    /**
     * Stops the Border Agent service.
     */
    void Stop(void);

    /**
     * Gets the state of the Border Agent service.
     *
     * @returns The state of the Border Agent service.
     */
    State GetState(void) const { return mState; }

    /**
     * Disconnects the Border Agent from any active secure sessions.
     *
     * If Border Agent is connected to a commissioner candidate with ephemeral key, calling this API
     * will cause the ephemeral key to be cleared after the session is disconnected.
     *
     * The Border Agent state may not change immediately upon calling this method, the state will be
     * updated when the connection update is notified by `HandleConnected()`.
     */
    void Disconnect(void);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    /**
     * Sets the ephemeral key for a given timeout duration.
     *
     * The ephemeral key can be set when the Border Agent is already running and is not currently connected to any
     * external commissioner (i.e., it is in `kStateStarted` state). To terminate active commissioner sessions,
     * use the `Disconnect()` function.
     *
     * The given @p aKeyString is directly used as the ephemeral PSK (excluding the trailing null `\0` character). Its
     * length must be between `kMinEphemeralKeyLength` and `kMaxEphemeralKeyLength`, inclusive.
     *
     * Setting the ephemeral key again before a previously set one is timed out will replace the previous one and will
     * reset the timeout.
     *
     * During the timeout interval, the ephemeral key can be used only once by an external commissioner to establish a
     * connection. After the commissioner disconnects, the ephemeral key is cleared, and the Border Agent reverts to
     * using PSKc. If the timeout expires while a commissioner is still connected, the session will be terminated, and
     * the Border Agent will cease using the ephemeral key and revert to PSKc.
     *
     * @param[in] aKeyString   The ephemeral key.
     * @param[in] aTimeout     The timeout duration in milliseconds to use the ephemeral key.
     *                         If zero, the default `kDefaultEphemeralKeyTimeout` value will be used.
     *                         If the timeout value is larger than `kMaxEphemeralKeyTimeout`, the max value will be
     *                         used instead.
     * @param[in] aUdpPort     The UDP port to use with ephemeral key. If UDP port is zero, an ephemeral port will be
     *                         used. `GetUdpPort()` will return the current UDP port being used.
     *
     * @retval kErrorNone           Successfully set the ephemeral key.
     * @retval kErrorInvalidState   Agent is not running or connected to external commissioner.
     * @retval kErrorInvalidArgs    The given @p aKeyString is not valid.
     * @retval kErrorFailed         Failed to set the key (e.g., could not bind to UDP port).
     */
    Error SetEphemeralKey(const char *aKeyString, uint32_t aTimeout, uint16_t aUdpPort);

    /**
     * Cancels the ephemeral key in use if any.
     *
     * Can be used to cancel a previously set ephemeral key before it times out. If the Border Agent is not running or
     * there is no ephemeral key in use, calling this function has no effect.
     *
     * If a commissioner is connected using the ephemeral key and is currently active, calling this method does not
     * change its state. In this case the `IsEphemeralKeyActive()` will continue to return `true` until the commissioner
     * disconnects, or the ephemeral key timeout expires. To terminate active commissioner sessions, use the
     * `Disconnect()` function.
     */
    void ClearEphemeralKey(void);

    /**
     * Indicates whether or not an ephemeral key is currently active.
     *
     * @retval TRUE    An ephemeral key is active.
     * @retval FALSE   No ephemeral key is active.
     */
    bool IsEphemeralKeyActive(void) const { return mUsingEphemeralKey; }

    /**
     * Callback function pointer to notify when there is any changes related to use of ephemeral key by Border Agent.
     */
    typedef otBorderAgentEphemeralKeyCallback EphemeralKeyCallback;

    void SetEphemeralKeyCallback(EphemeralKeyCallback aCallback, void *aContext)
    {
        mEphemeralKeyCallback.Set(aCallback, aContext);
    }

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE

    /**
     * Gets the set of border agent counters.
     *
     * @returns The border agent counters.
     */
    const otBorderAgentCounters *GetCounters(void) { return &mCounters; }

    /**
     * Returns the UDP Proxy port to which the commissioner is currently
     * bound.
     *
     * @returns  The current UDP Proxy port or 0 if no Proxy Transmit has been received yet.
     */
    uint16_t GetUdpProxyPort(void) const { return mUdpProxyPort; }

private:
    static_assert(kMaxEphemeralKeyLength <= SecureTransport::kPskMaxLength,
                  "Max ephemeral key length is larger than max PSK len");

    static constexpr uint16_t kUdpPort          = OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT;
    static constexpr uint32_t kKeepAliveTimeout = 50 * 1000; // Timeout to reject a commissioner (in msec)

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    static constexpr uint16_t kMaxEphemeralKeyConnectionAttempts = 10;
#endif

    class ForwardContext : public InstanceLocatorInit, public Heap::Allocatable<ForwardContext>
    {
    public:
        Error    Init(Instance &aInstance, const Coap::Message &aMessage, bool aPetition, bool aSeparate);
        bool     IsPetition(void) const { return mPetition; }
        uint16_t GetMessageId(void) const { return mMessageId; }
        Error    ToHeader(Coap::Message &aMessage, uint8_t aCode) const;

    private:
        uint16_t mMessageId;                             // The CoAP Message ID of the original request.
        bool     mPetition : 1;                          // Whether the forwarding request is leader petition.
        bool     mSeparate : 1;                          // Whether the original request expects separate response.
        uint8_t  mTokenLength : 4;                       // The CoAP Token Length of the original request.
        uint8_t  mType : 2;                              // The CoAP Type of the original request.
        uint8_t  mToken[Coap::Message::kMaxTokenLength]; // The CoAP Token of the original request.
    };

    Error Start(uint16_t aUdpPort);
    Error Start(uint16_t aUdpPort, const uint8_t *aPsk, uint8_t aPskLength);

    void HandleNotifierEvents(Events aEvents);

    Coap::Message::Code CoapCodeFromError(Error aError);
    Error               SendMessage(Coap::Message &aMessage);
    void                SendErrorMessage(const ForwardContext &aForwardContext, Error aError);
    void                SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError);

    static void HandleConnected(SecureTransport::ConnectEvent aEvent, void *aContext);
    void        HandleConnected(SecureTransport::ConnectEvent aEvent);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleTmfDatasetGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri);
    void HandleTimeout(void);

#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    void        RestartAfterRemovingEphemeralKey(void);
    void        HandleEphemeralKeyTimeout(void);
    void        InvokeEphemeralKeyCallback(void);
    static void HandleSecureAgentStopped(void *aContext);
    void        HandleSecureAgentStopped(void);
#endif

    static void HandleCoapResponse(void                *aContext,
                                   otMessage           *aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   Error                aResult);
    void  HandleCoapResponse(const ForwardContext &aForwardContext, const Coap::Message *aResponse, Error aResult);
    Error ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri);
    Error ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage);
    static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
    bool        HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    using TimeoutTimer = TimerMilliIn<BorderAgent, &BorderAgent::HandleTimeout>;
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    using EphemeralKeyTimer = TimerMilliIn<BorderAgent, &BorderAgent::HandleEphemeralKeyTimeout>;
    using EphemeralKeyTask  = TaskletIn<BorderAgent, &BorderAgent::InvokeEphemeralKeyCallback>;
#endif

    State                      mState;
    uint16_t                   mUdpProxyPort;
    Ip6::Udp::Receiver         mUdpReceiver;
    Ip6::Netif::UnicastAddress mCommissionerAloc;
    TimeoutTimer               mTimer;
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    Id   mId;
    bool mIdInitialized;
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
    bool                           mUsingEphemeralKey;
    uint16_t                       mOldUdpPort;
    EphemeralKeyTimer              mEphemeralKeyTimer;
    EphemeralKeyTask               mEphemeralKeyTask;
    Callback<EphemeralKeyCallback> mEphemeralKeyCallback;
#endif
    otBorderAgentCounters mCounters;
};

DeclareTmfHandler(BorderAgent, kUriRelayRx);
DeclareTmfHandler(BorderAgent, kUriCommissionerPetition);
DeclareTmfHandler(BorderAgent, kUriCommissionerKeepAlive);
DeclareTmfHandler(BorderAgent, kUriRelayTx);
DeclareTmfHandler(BorderAgent, kUriCommissionerGet);
DeclareTmfHandler(BorderAgent, kUriActiveGet);
DeclareTmfHandler(BorderAgent, kUriPendingGet);
DeclareTmfHandler(BorderAgent, kUriProxyTx);

} // namespace MeshCoP

DefineCoreType(otBorderAgentId, MeshCoP::BorderAgent::Id);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#endif // BORDER_AGENT_HPP_
