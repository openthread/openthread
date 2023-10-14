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
    typedef otBorderAgentId Id; ///< Border Agent ID.

    /**
     * Defines the Border Agent state.
     *
     */
    enum State : uint8_t
    {
        kStateStopped = OT_BORDER_AGENT_STATE_STOPPED, ///< Border agent is stopped/disabled.
        kStateStarted = OT_BORDER_AGENT_STATE_STARTED, ///< Border agent is started.
        kStateActive  = OT_BORDER_AGENT_STATE_ACTIVE,  ///< Border agent is connected with external commissioner.
    };

    /**
     * Initializes the `BorderAgent` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
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
     *
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
     *
     */
    Error SetId(const Id &aId);
#endif

    /**
     * Gets the UDP port of this service.
     *
     * @returns  UDP port number.
     *
     */
    uint16_t GetUdpPort(void) const;

    /**
     * Starts the Border Agent service.
     *
     */
    void Start(void);

    /**
     * Stops the Border Agent service.
     *
     */
    void Stop(void);

    /**
     * Gets the state of the Border Agent service.
     *
     * @returns The state of the Border Agent service.
     *
     */
    State GetState(void) const { return mState; }

    /**
     * Returns the UDP Proxy port to which the commissioner is currently
     * bound.
     *
     * @returns  The current UDP Proxy port or 0 if no Proxy Transmit has been received yet.
     *
     */
    uint16_t GetUdpProxyPort(void) const { return mUdpProxyPort; }

private:
    static constexpr uint16_t kUdpPort          = OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT;
    static constexpr uint32_t kKeepAliveTimeout = 50 * 1000; // Timeout to reject a commissioner (in msec)

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

    void HandleNotifierEvents(Events aEvents);

    Coap::Message::Code CoapCodeFromError(Error aError);
    Error               SendMessage(Coap::Message &aMessage);
    void                SendErrorMessage(const ForwardContext &aForwardContext, Error aError);
    void                SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError);

    static void HandleConnected(bool aConnected, void *aContext);
    void        HandleConnected(bool aConnected);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void HandleTimeout(void);

    static void HandleCoapResponse(void                *aContext,
                                   otMessage           *aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   Error                aResult);
    void  HandleCoapResponse(const ForwardContext &aForwardContext, const Coap::Message *aResponse, Error aResult);
    Error ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri);
    Error ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage);
    static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
    bool        HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    void LogError(const char *aActionText, Error aError);
#else
    void LogError(const char *, Error) {}
#endif

    using TimeoutTimer = TimerMilliIn<BorderAgent, &BorderAgent::HandleTimeout>;

    State                      mState;
    uint16_t                   mUdpProxyPort;
    Ip6::Udp::Receiver         mUdpReceiver;
    Ip6::Netif::UnicastAddress mCommissionerAloc;
    TimeoutTimer               mTimer;
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    Id   mId;
    bool mIdInitialized;
#endif
};

DeclareTmfHandler(BorderAgent, kUriRelayRx);
DeclareTmfHandler(BorderAgent, kUriCommissionerPetition);
DeclareTmfHandler(BorderAgent, kUriCommissionerKeepAlive);
DeclareTmfHandler(BorderAgent, kUriRelayTx);
DeclareTmfHandler(BorderAgent, kUriCommissionerGet);
DeclareTmfHandler(BorderAgent, kUriCommissionerSet);
DeclareTmfHandler(BorderAgent, kUriActiveGet);
DeclareTmfHandler(BorderAgent, kUriActiveSet);
DeclareTmfHandler(BorderAgent, kUriPendingGet);
DeclareTmfHandler(BorderAgent, kUriPendingSet);
DeclareTmfHandler(BorderAgent, kUriProxyTx);

} // namespace MeshCoP

DefineMapEnum(otBorderAgentState, MeshCoP::BorderAgent::State);
DefineCoreType(otBorderAgentId, MeshCoP::BorderAgent::Id);

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#endif // BORDER_AGENT_HPP_
