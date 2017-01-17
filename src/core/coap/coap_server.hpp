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
 *   This file includes definitions for the CoAP server.
 */

#ifndef COAP_SERVER_HPP_
#define COAP_SERVER_HPP_

#include <openthread-coap.h>
#include <coap/coap_base.hpp>
#include <coap/coap_header.hpp>
#include <common/message.hpp>
#include <net/udp6.hpp>

namespace Thread {
namespace Coap {

/**
 * @addtogroup core-coap
 *
 * @{
 *
 */

/**
 * This class implements CoAP resource handling.
 *
 */
class Resource : public otCoapResource
{
    friend class Server;

public:
    enum
    {
        kMaxReceivedUriPath = 32,   ///< Maximum supported URI path on received messages.
    };

    /**
     * This constructor initializes the resource.
     *
     * @param[in]  aUriPath  A pointer to a NULL-terminated string for the Uri-Path.
     * @param[in]  aHandler  A function pointer that is called when receiving a CoAP message for @p aUriPath.
     * @param[in]  aContext  A pointer to arbitrary context information.
     */
    Resource(const char *aUriPath, otCoapRequestHandler aHandler, void *aContext) {
        mUriPath = aUriPath;
        mHandler = aHandler;
        mContext = aContext;
        mNext = NULL;
    }

    /**
     * This method returns a pointer to the next resource.
     *
     * @returns A Pointer to the next resource.
     *
     */
    Resource *GetNext(void) const { return static_cast<Resource *>(mNext); };

private:
    void HandleRequest(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) {
        mHandler(mContext, &aHeader, &aMessage, &aMessageInfo);
    }
};

/**
 * This class implements the CoAP server.
 *
 */
class Server : public CoapBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aUdp      A reference to the UDP object.
     * @param[in]  aPort     The port to listen on.
     * @param[in]  aSender   A pointer to a function for sending messages.
     * @param[in]  aReceiver A pointer to a function for handling received messages.
     *
     */
    Server(Ip6::Udp &aUdp, uint16_t aPort, SenderFunction aSender = &Server::Send,
           ReceiverFunction aReceiver = &Server::Receive);

    /**
     * This method starts the CoAP server.
     *
     * @retval kThreadError_None  Successfully started the CoAP server.
     *
     */
    ThreadError Start(void);

    /**
     * This method stops the CoAP server.
     *
     * @retval kThreadError_None  Successfully stopped the CoAP server.
     *
     */
    ThreadError Stop(void);

    /**
     * This method adds a resource to the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     * @retval kThreadError_None     Successfully added @p aResource.
     * @retval kThreadError_Already  The @p aResource was already added.
     *
     */
    ThreadError AddResource(Resource &aResource);

    /**
     * This method removes a resource from the CoAP server.
     *
     * @param[in]  aResource  A reference to the resource.
     *
     */
    void RemoveResource(Resource &aResource);

    /**
     * This method returns a new UDP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     *
     * @returns A pointer to the message or NULL if no buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved);

    /**
     * This method returns a new MeshCoP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     *
     * @returns A pointer to the MeshCoP message or NULL if no buffers are available.
     *
     */
    Message *NewMeshCoPMessage(uint16_t aReserved);

    /**
     * This method creates a new message with a CoAP header.
     *
     * @param[in]  aHeader  A reference to a CoAP header that is used to create the message.
     *
     * @returns A pointer to the message or NULL if failed to allocate message.
     *
     */
    Message *NewMessage(const Header &aHeader) { return CoapBase::NewMessage(aHeader); };

    /**
     * This method creates a new MeshCoP message with a CoAP header.
     *
     * @param[in]  aHeader  A reference to a CoAP header that is used to create the message.
     *
     * @returns A pointer to the MeshCoP message or NULL if failed to allocate message.
     *
     */
    Message *NewMeshCoPMessage(const Header &aHeader) { return CoapBase::NewMeshCoPMessage(aHeader); };

    /**
      * This method sends a CoAP response from the server.
      *
      * @param[in]  aMessage      The CoAP response to send.
      * @param[in]  aMessageInfo  The message info corresponding to @p aMessage.
      *
      * @retval kThreadError_None    Successfully enqueued the CoAP response message.
      * @retval kThreadError_NoBufs  Insufficient buffers available to send the CoAP response.
      *
      */
    ThreadError SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sends a CoAP ACK message on which a dummy CoAP response is piggybacked.
     *
     * @param[in]  aRequestHeader  A reference to the CoAP Header that was used in CoAP request.
     * @param[in]  aMessageInfo    The message info corresponding to the CoAP request.
     *
     * @retval kThreadError_None         Successfully enqueued the CoAP response message.
     * @retval kThreadError_NoBufs       Insufficient buffers available to send the CoAP response.
     * @retval kThreadError_InvalidArgs  The @p aRequestHeader header is not of confirmable type.
     *
     */
    ThreadError SendEmptyAck(const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sets CoAP server's port number.
     *
     * @param[in]  aPort  A port number to set.
     *
     * @retval kThreadError_None  Binding with a port succeeded.
     *
     */
    ThreadError SetPort(uint16_t aPort);

protected:
    void ProcessReceivedMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

private:
    static ThreadError Send(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) {
        return (static_cast<Server *>(aContext))->mSocket.SendTo(aMessage, aMessageInfo);
    }

    static void Receive(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo) {
        (static_cast<Server *>(aContext))->ProcessReceivedMessage(aMessage, aMessageInfo);
    }

    uint16_t mPort;
    Resource *mResources;
};

/**
 * @}
 *
 */

}  // namespace Coap
}  // namespace Thread

#endif  // COAP_SERVER_HPP_
