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

#ifndef COAP_SECURE_HPP_
#define COAP_SECURE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE && !OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
#error "CoAP Secure API feature requires `OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE`"
#endif

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

#include "coap/coap.hpp"
#include "common/callback.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/secure_transport.hpp"

#include <openthread/coap_secure.h>

/**
 * @file
 *   This file includes definitions for the secure CoAP.
 */

namespace ot {
namespace Coap {

typedef MeshCoP::Dtls Dtls;

/**
 * Represents a secure CoAP session.
 */
class SecureSession : public CoapBase, public Dtls::Session
{
public:
    /**
     * Dequeues and frees all queued messages (requests and responses) and stops all timers and tasklets.
     */
    void Cleanup(void);

    /**
     * Sets the connection event callback.
     *
     * @param[in]  aHandler   A pointer to a function that is called when connected or disconnected.
     * @param[in]  aContext   A pointer to arbitrary context information.
     */
    void SetConnectCallback(ConnectHandler aHandler, void *aContext) { mConnectCallback.Set(aHandler, aContext); }

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    /**
     * Sends a CoAP message over secure DTLS session.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be NULL pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aTransmitHook A pointer to a hook function for outgoing block-wise transfer.
     * @param[in]  aReceiveHook  A pointer to a hook function for incoming block-wise transfer.
     *
     * @retval kErrorNone          Successfully sent CoAP message.
     * @retval kErrorNoBufs        Failed to allocate retransmission data.
     * @retval kErrorInvalidState  DTLS connection was not initialized.
     */
    Error SendMessage(Message                    &aMessage,
                      ResponseHandler             aHandler      = nullptr,
                      void                       *aContext      = nullptr,
                      otCoapBlockwiseTransmitHook aTransmitHook = nullptr,
                      otCoapBlockwiseReceiveHook  aReceiveHook  = nullptr);

#else
    /**
     * Sends a CoAP message over secure DTLS session.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be nullptr pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone          Successfully sent CoAP message.
     * @retval kErrorNoBufs        Failed to allocate retransmission data.
     * @retval kErrorInvalidState  DTLS connection was not initialized.
     */
    Error SendMessage(Message &aMessage, ResponseHandler aHandler = nullptr, void *aContext = nullptr);
#endif

protected:
    SecureSession(Instance &aInstance, Dtls::Transport &aDtlsTransport);

private:
    static Error Transmit(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    Error        Transmit(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void  HandleTransmitTask(Tasklet &aTasklet);
    void         HandleTransmitTask(void);
    static void  HandleDtlsConnectEvent(ConnectEvent aEvent, void *aContext);
    void         HandleDtlsConnectEvent(ConnectEvent aEvent);
    static void  HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void         HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    Callback<ConnectHandler> mConnectCallback;
    ot::MessageQueue         mTransmitQueue;
    TaskletContext           mTransmitTask;
};

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

/**
 * Represents an Application CoAPS.
 */
class ApplicationCoapSecure : public Dtls::Transport, public Dtls::Transport::Extension, public SecureSession
{
public:
    /**
     * Initializes the `ApplicationCoapSecure`
     *
     * @param[in]  aInstance           A reference to the OpenThread instance.
     * @param[in]  aLayerTwoSecurity   Specifies whether to use layer two security or not.
     */
    ApplicationCoapSecure(Instance &aInstance, LinkSecurityMode aLayerTwoSecurity)
        : Dtls::Transport(aInstance, aLayerTwoSecurity)
        , Dtls::Transport::Extension(static_cast<Dtls::Transport &>(*this))
        , SecureSession(aInstance, static_cast<Dtls::Transport &>(*this))
    {
        Dtls::Transport::SetAcceptCallback(HandleDtlsAccept, this);
        Dtls::Transport::SetExtension(static_cast<Dtls::Transport::Extension &>(*this));
    }

private:
    static MeshCoP::SecureSession *HandleDtlsAccept(void *aContext, const Ip6::MessageInfo &aMessageInfo);
    SecureSession                 *HandleDtlsAccept(void);
};

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

#endif // COAP_SECURE_HPP_
