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

#ifndef SECURE_COAP_CLIENT_HPP_
#define SECURE_COAP_CLIENT_HPP_

#include <coap/coap_client.hpp>
#include <meshcop/dtls.hpp>

/**
 * @file
 *   This file includes definitions for the secure CoAP client.
 */

namespace Thread {

class ThreadNetif;

namespace Coap {

class SecureClient: public Client
{
public:
    /**
      * This function pointer is called once DTLS connection is established.
      *
      * @param[in]  aConnected  TRUE if a connection was established, FALSE otherwise.
      * @param[in]  aContext    A pointer to arbitrary context information.
      *
      */
    typedef void (*ConnectedCallback)(bool aConnected, void *aContext);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aNetif  A reference to the network interface that secure CoAP client should be assigned to.
     *
     */
    SecureClient(ThreadNetif &aNetif);

    /**
     * This method stops the secure CoAP client.
     *
     * @retval kThreadError_None  Successfully stopped the secure CoAP client.
     *
     */
    ThreadError Stop(void);

    /**
     * This method initializes DTLS session with a peer.
     *
     * @param[in]  aMessageInfo  A reference to an address of the peer.
     * @param[in]  aCallback     A pointer to a function that will be called once DTLS connection is established.
     *
     * @retval kThreadError_None  Successfully started DTLS connection.
     *
     */
    ThreadError Connect(const Ip6::MessageInfo &aMessageInfo, ConnectedCallback aCallback, void *aContext);

    /**
     * This method indicates whether or not the DTLS session is active.
     *
     * @retval TRUE  If DTLS session is active.
     * @retval FALSE If DTLS session is not active.
     *
     */
    bool IsConnectionActive(void);

    /**
     * This method indicates whether or not the DTLS session is connected.
     *
     * @retval TRUE   The DTLS session is connected.
     * @retval FALSE  The DTLS session is not connected.
     *
     */
    bool IsConnected(void);

    /**
     * This method stops the DTLS connection.
     *
     * @retval kThreadError_None  Successfully stopped the DTLS connection.
     *
     */
    ThreadError Disconnect(void);

    /**
     * This method returns a reference to the DTLS object.
     *
     * @returns  A reference to the DTLS object.
     *
     */
    MeshCoP::Dtls &GetDtls(void);

    /**
     * This method sends a CoAP message over secure DTLS connection.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be NULL pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kThreadError_None         Successfully sent CoAP message.
     * @retval kThreadError_NoBufs       Failed to allocate retransmission data.
     * @retvak kThreadError_InvalidState DTLS connection was not initialized.
     *
     */
    ThreadError SendMessage(Message &aMessage, otCoapResponseHandler aHandler = NULL, void *aContext = NULL);

private:
    static ThreadError Send(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void Receive(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsConnected(void *aContext, bool aConnected);
    void HandleDtlsConnected(bool aConnected);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static ThreadError HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType);
    ThreadError HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType);

    static void HandleUdpTransmit(void *aContext);
    void HandleUdpTransmit(void);

    Ip6::MessageInfo mPeerAddress;
    ConnectedCallback mConnectedCallback;
    void *mContext;
    ThreadNetif &mNetif;
    Message *mTransmitMessage;
    Tasklet mTransmitTask;
};

}  // namespace Coap
}  // namespace Thread

#endif  // SECURE_COAP_CLIENT_HPP_
