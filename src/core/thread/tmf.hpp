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
#include "common/locator.hpp"

namespace ot {
namespace Tmf {

constexpr uint16_t kUdpPort = 61631; ///< TMF UDP Port

typedef Coap::Message Message; ///< A TMF message.

/**
 * This class represents message information for a TMF message.
 *
 * This is sub-class of `Ip6::MessageInfo` intended for use when sending TMF messages.
 *
 */
class MessageInfo : public InstanceLocator, public Ip6::MessageInfo
{
public:
    /**
     * This constructor initializes the `MessageInfo`.
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
     * This method sets the local socket port to TMF port.
     *
     */
    void SetSockPortToTmf(void) { SetSockPort(kUdpPort); }

    /**
     * This method sets the local socket address to mesh-local RLOC address.
     *
     */
    void SetSockAddrToRloc(void);

    /**
     * This method sets the local socket address to RLOC address and the peer socket address to leader ALOC.
     *
     * @retval kErrorNone      Successfully set the addresses.
     * @retval kErrorDetached  Cannot set leader ALOC since device is currently detached.
     *
     */
    Error SetSockAddrToRlocPeerAddrToLeaderAloc(void);

    /**
     * This method sets the local socket address to RLOC address and the peer socket address to leader RLOC.
     *
     * @retval kErrorNone      Successfully set the addresses.
     * @retval kErrorDetached  Cannot set leader RLOC since device is currently detached.
     *
     */
    Error SetSockAddrToRlocPeerAddrToLeaderRloc(void);

    /**
     * This method sets the local socket address to RLOC address and the peer socket address to realm-local all
     * routers multicast address.
     *
     */
    void SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast(void);

    /**
     * This method sets the local socket address to RLOC address and the peer socket address to a router RLOC based on
     * a given RLOC16.
     *
     * @param[in] aRloc16     The RLOC16 to use for peer address.
     *
     */
    void SetSockAddrToRlocPeerAddrTo(uint16_t aRloc16);

    /**
     * This method sets the local socket address to RLOC address and the peer socket address to a given address.
     *
     * @param[in] aPeerAddress  The peer address.
     *
     */
    void SetSockAddrToRlocPeerAddrTo(const Ip6::Address &aPeerAddress);
};

/**
 * This class implements functionality of the Thread TMF agent.
 *
 */
class Agent : public Coap::Coap
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit Agent(Instance &aInstance)
        : Coap::Coap(aInstance)
    {
        SetInterceptor(&Filter, this);
    }

    /**
     * This method starts the TMF agent.
     *
     * @retval kErrorNone    Successfully started the CoAP service.
     * @retval kErrorFailed  Failed to start the TMF agent.
     *
     */
    Error Start(void);

    /**
     * This method indicates whether or not a message meets TMF addressing rules.
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

private:
    static Error Filter(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);
};

} // namespace Tmf
} // namespace ot

#endif //  OT_CORE_THREAD_TMF_HPP_
