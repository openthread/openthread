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

#ifndef SECURE_COAP_SERVER_HPP_
#define SECURE_COAP_SERVER_HPP_

#include "coap/coap_server.hpp"

/**
 * @file
 *   This file includes definitions for the secure CoAP server.
 */

namespace Thread {

class ThreadNetif;

namespace Coap {

class SecureServer : public Server
{
public:
    /**
     * This function pointer is called when secure CoAP server want to send encrypted message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef ThreadError(*TransportCallback)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aNetif  A reference to the network interface that secure CoAP server should be assigned to.
     * @param[in]  aPort   The port to listen on.
     *
     */
    SecureServer(ThreadNetif &aNetif, uint16_t aPort);

    /**
     * This method starts the secure CoAP server.
     *
     * @param[in]  aCallback  A pointer to a function for sending messages. If NULL, the message is sent directly to server socket.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     * @retval kThreadError_None  Successfully started the CoAP server.
     *
     */
    ThreadError Start(TransportCallback aCallback = NULL, void *aContext = NULL);

    /**
     * This method stops the secure CoAP server.
     *
     * @retval kThreadError_None  Successfully stopped the CoAP server.
     *
     */
    ThreadError Stop(void);

    /**
     * This method indicates whether or not the DTLS session is active.
     *
     * @retval TRUE  If DTLS session is active.
     * @retval FALSE If DTLS session is not active.
     *
     */
    bool IsConnectionActive(void);

    /**
     * This method is used to pass messages to the secure CoAP server.
     * It can be used when messages are received other way that via server's socket.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    void Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method sets the PSK.
     *
     * @param[in]  aPSK       A pointer to the PSK.
     * @param[in]  aPskLenght A PSK length.
     *
     * @retval kThreadError_None         Successfully set the PSK.
     * @retval kThreadError_InvalidArgs  The PSK is invalid.
     *
     */
    ThreadError SetPsk(const uint8_t *aPsk, uint8_t aPskLength);

private:
    static ThreadError Send(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void Receive(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsConnected(void *aContext, bool aConnected);
    void HandleDtlsConnected(bool aConnected);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static ThreadError HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength);
    ThreadError HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength);

    static void HandleUdpTransmit(void *aContext);
    void HandleUdpTransmit(void);

    Ip6::MessageInfo mPeerAddress;
    TransportCallback mTransmitCallback;
    void *mContext;
    ThreadNetif &mNetif;
    Message *mTransmitMessage;
    Tasklet mTransmitTask;
};

}  // namespace Coap
}  // namespace Thread

#endif  // SECURE_COAP_SERVER_HPP_
