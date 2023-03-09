/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for TMF functionality.
 */

#ifndef OT_CORE_THREAD_TMF_HPP_
#define OT_CORE_THREAD_TMF_HPP_

#include "openthread-core-config.h"

#include "coap/coap.hpp"
#include "coap/coap_secure.hpp"
#include "common/locator.hpp"

namespace ot {
namespace Tmf {

/**
 * Declares a TMF handler (a full template specialization of `HandleTmf<Uri>` method) in a given `Type`.
 *
 * The class `Type` MUST declare a template method of the following format:
 *
 *  template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
 *
 * @param[in] Type      The `Type` in which the TMF handler is declared.
 * @param[in] kUri      The `Uri` which is handled.
 *
 */
#define DeclareTmfHandler(Type, kUri) \
    template <> void Type::HandleTmf<kUri>(Coap::Message & aMessage, const Ip6::MessageInfo &aMessageInfo)

constexpr uint16_t kUdpPort = 61631; ///< TMF UDP Port

typedef Coap::Message Message; ///< A TMF message.

/**
 * Represents message information for a TMF message.
 *
 * This is sub-class of `Ip6::MessageInfo` intended for use when sending TMF messages.
 *
 */
class MessageInfo : public InstanceLocator, public Ip6::MessageInfo
{
public:
    /**
     * Initializes the `MessageInfo`.
     *
     * The peer port is set to `Tmf::kUdpPort` and all other properties are cleared (set to zero).
     *
     * @param[in] aInstance    The OpenThread instance.
     *
     */
    explicit MessageInfo(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
        SetPeerPort(kUdpPort);
    }

    /**
     * Sets the local socket port to TMF port.
     *
     */
    void SetSockPortToTmf(void) { SetSockPort(kUdpPort); }

    /**
     * Sets the local socket address to mesh-local RLOC address.
     *
     */
    void SetSockAddrToRloc(void);

    /**
     * Sets the local socket address to RLOC address and the peer socket address to leader ALOC.
     *
     * @retval kErrorNone      Successfully set the addresses.
     * @retval kErrorDetached  Cannot set leader ALOC since device is currently detached.
     *
     */
    Error SetSockAddrToRlocPeerAddrToLeaderAloc(void);

    /**
     * Sets the local socket address to RLOC address and the peer socket address to leader RLOC.
     *
     * @retval kErrorNone      Successfully set the addresses.
     * @retval kErrorDetached  Cannot set leader RLOC since device is currently detached.
     *
     */
    Error SetSockAddrToRlocPeerAddrToLeaderRloc(void);

    /**
     * Sets the local socket address to RLOC address and the peer socket address to realm-local all
     * routers multicast address.
     *
     */
    void SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast(void);

    /**
     * Sets the local socket address to RLOC address and the peer socket address to a router RLOC based on
     * a given RLOC16.
     *
     * @param[in] aRloc16     The RLOC16 to use for peer address.
     *
     */
    void SetSockAddrToRlocPeerAddrTo(uint16_t aRloc16);

    /**
     * Sets the local socket address to RLOC address and the peer socket address to a given address.
     *
     * @param[in] aPeerAddress  The peer address.
     *
     */
    void SetSockAddrToRlocPeerAddrTo(const Ip6::Address &aPeerAddress);
};

/**
 * Implements functionality of the Thread TMF agent.
 *
 */
class Agent : public Coap::Coap
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit Agent(Instance &aInstance);

    /**
     * Starts the TMF agent.
     *
     * @retval kErrorNone    Successfully started the CoAP service.
     * @retval kErrorFailed  Failed to start the TMF agent.
     *
     */
    Error Start(void);

    /**
     * Indicates whether or not a message meets TMF addressing rules.
     *
     * A TMF message MUST comply with following rules:
     *
     * - The destination port is `Tmf::kUdpPort`.
     * - Both source and destination addresses are Link-Local, or
     * - Source is Mesh Local and then destination is Mesh Local or Link-Local Multicast or Realm-Local Multicast.
     *
     * @param[in] aSourceAddress   Source IPv6 address.
     * @param[in] aDestAddress     Destination IPv6 address.
     * @param[in] aDestPort        Destination port number.
     *
     * @retval TRUE   if TMF addressing rules are met.
     * @retval FALSE  if TMF addressing rules are not met.
     *
     */
    bool IsTmfMessage(const Ip6::Address &aSourceAddress, const Ip6::Address &aDestAddress, uint16_t aDestPort) const;

    /**
     * Converts a TMF message priority to IPv6 header DSCP value.
     *
     * @param[in] aPriority  The message priority to convert.
     *
     * @returns The DSCP value corresponding to @p aPriority.
     *
     */
    static uint8_t PriorityToDscp(Message::Priority aPriority);

    /**
     * Converts a IPv6 header DSCP value to message priority for TMF message.
     *
     * @param[in] aDscp      The IPv6 header DSCP value in a TMF message.
     *
     * @returns The message priority corresponding to the @p aDscp.
     *
     */
    static Message::Priority DscpToPriority(uint8_t aDscp);

private:
    template <Uri kUri> void HandleTmf(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static bool HandleResource(CoapBase               &aCoapBase,
                               const char             *aUriPath,
                               Message                &aMessage,
                               const Ip6::MessageInfo &aMessageInfo);
    bool        HandleResource(const char *aUriPath, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static Error Filter(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);
};

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

/**
 * Implements functionality of the secure TMF agent.
 *
 */
class SecureAgent : public Coap::CoapSecure
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit SecureAgent(Instance &aInstance);

private:
    static bool HandleResource(CoapBase               &aCoapBase,
                               const char             *aUriPath,
                               Message                &aMessage,
                               const Ip6::MessageInfo &aMessageInfo);
    bool        HandleResource(const char *aUriPath, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
};

#endif

} // namespace Tmf
} // namespace ot

#endif //  OT_CORE_THREAD_TMF_HPP_
