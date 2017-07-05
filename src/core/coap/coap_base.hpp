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

#ifndef COAP_BASE_HPP_
#define COAP_BASE_HPP_

#include <openthread-types.h>
#include <coap/coap_header.hpp>
#include <common/message.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace Thread {
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 *
 */

enum
{
    kMeshCoPMessagePriority = Message::kPriorityHigh, // The priority for MeshCoP message
};

/**
 * This class implements a common code base for CoAP client/server.
 *
 */
class CoapBase
{
public:

    /**
     * This function pointer is called when CoAP client/server wants to send a message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef ThreadError(* SenderFunction)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This function pointer is called when CoAP client/server receives a message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef void (* ReceiverFunction)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aUdp      A reference to the UDP object.
     * @param[in]  aSender   A pointer to a function for sending messages.
     * @param[in]  aReceiver A pointer to a function for handling received messages.
     *
     */
    CoapBase(Ip6::Udp &aUdp, SenderFunction aSender, ReceiverFunction aReceiver):
        mSocket(aUdp),
        mSender(aSender),
        mReceiver(aReceiver) {};

    /**
     * This method creates a new message with a CoAP header.
     *
     * @param[in]  aHeader  A reference to a CoAP header that is used to create the message.
     *
     * @returns A pointer to the message or NULL if failed to allocate message.
     *
     */
    Message *NewMessage(const Header &aHeader);

    /**
     * This method creates a new MeshCoP message with a CoAP header.
     *
     * @param[in]  aHeader  A reference to a CoAP header that is used to create the message.
     *
     * @returns A pointer to the MeshCoP message or NULL if failed to allocate message.
     *
     */
    Message *NewMeshCoPMessage(const Header &aHeader);

    /**
     * This method returns a port number used by CoAP client.
     *
     * @returns A port number.
     *
     */
    uint16_t GetPort(void) { return mSocket.GetSockName().mPort; };

protected:
    ThreadError Start(const Ip6::SockAddr &aSockAddr);
    ThreadError Stop(void);

    Ip6::UdpSocket mSocket;
    SenderFunction mSender;
    ReceiverFunction mReceiver;

private:
    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
};

}  // namespace Coap
}  // namespace Thread

#endif  // COAP_BASE_HPP_
