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
 *  This file includes definitions for the Joiner Router role.
 */

#ifndef JOINER_ROUTER_HPP_
#define JOINER_ROUTER_HPP_

#include <openthread-types.h>

#include <coap/coap_header.hpp>
#include <coap/coap_client.hpp>
#include <coap/coap_server.hpp>
#include <common/message.hpp>
#include <mac/mac_frame.hpp>
#include <net/udp6.hpp>

namespace Thread {

class ThreadNetif;

namespace MeshCoP {

class JoinerRouter
{
public:
    /**
     * This constructor initializes the Joiner Router object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    JoinerRouter(ThreadNetif &aNetif);

    /**
     * This method returns the Joiner UDP Port.
     *
     * @returns The Joiner UDP Port number .
     *
     */
    uint16_t GetJoinerUdpPort(void);

    /**
     * This method sets the Joiner UDP Port.
     *
     * @param[in]  The Joiner UDP Port number.
     *
     * @retval kThreadError_None    Successfully set the Joiner UDP Port.
     *
     */
    ThreadError SetJoinerUdpPort(uint16_t aJoinerUdpPort);

private:
    static void HandleNetifStateChanged(uint32_t aFlags, void *aContext);
    void HandleNetifStateChanged(uint32_t aFlags);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleRelayTransmit(void *aContext, Coap::Header &aHeader,
                                    Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleRelayTransmit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    ThreadError SendJoinerEntrust(const Ip6::MessageInfo &aMessageInfo);

    ThreadError GetBorderAgentRloc(uint16_t &aRloc);

    Ip6::NetifCallback mNetifCallback;

    Ip6::UdpSocket mSocket;
    Coap::Resource mRelayTransmit;
    Coap::Client &mCoapClient;
    ThreadNetif &mNetif;

    uint16_t mJoinerUdpPort;
    bool mIsJoinerPortConfigured;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // JOINER_ROUTER_HPP_
