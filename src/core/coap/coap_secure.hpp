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

#include "coap/coap.hpp"
#include "meshcop/dtls.hpp"

#include <openthread/coap_secure.h>

/**
 * @file
 *   This file includes definitions for the secure CoAP agent.
 */

namespace ot {

namespace Coap {

class CoapSecure : public CoapBase
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
     * This function pointer is called when secure CoAP server want to send encrypted message.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    typedef otError (*TransportCallback)(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit CoapSecure(Instance &aInstance);

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    /**
     * This constructor initializes the object.
     * (Used for Application CoAPS)
     *
     * @param[in]  aInstance             A reference to the OpenThread instance.
     * @param[in]  aUdpTransmitHandle    Handler for udp transmit.
     * @param[in]  aRetransmissionTimer  Handler for retransmission.
     * @param[in]  aResponsesQueueTimer  Handler for Queue Responses.
     *
     */
    explicit CoapSecure(Instance &       aInstance,
                        Tasklet::Handler aUdpTransmitHandle,
                        Timer::Handler   aRetransmissionTimer,
                        Timer::Handler   aResponsesQueueTimer);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    /**
     * This method starts the secure CoAP agent.
     *
     * @param[in]  aPort      The local UDP port to bind to.
     * @param[in]  aCallback  A pointer to a function for sending messages.
     *                        If NULL, the message is sent directly to the socket.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE  Successfully started the CoAP agent.
     *
     */
    otError Start(uint16_t aPort, TransportCallback aCallback = NULL, void *aContext = NULL);

    /**
     * This method sets connected callback of this secure CoAP agent.
     *
     * @param[in]  aCallback  A pointer to a function to get called when connection state changes.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     */
    void SetConnectedCallback(ConnectedCallback aCallback, void *aContext);

    /**
     * This method stops the secure CoAP agent.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the secure CoAP agent.
     *
     */
    otError Stop(void);

    /**
     * This method initializes DTLS session with a peer.
     *
     * @param[in]  aMessageInfo            A pointer to a message info structure.
     * @param[in]  aCallback               A pointer to a function that will be called once DTLS connection is
     * established.
     *
     * @retval OT_ERROR_NONE  Successfully started DTLS connection.
     *
     */
    otError Connect(const Ip6::MessageInfo &aMessageInfo, ConnectedCallback aCallback, void *aContext);

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
     * @retval OT_ERROR_NONE  Successfully stopped the DTLS connection.
     *
     */
    otError Disconnect(void);

    /**
     * This method returns a reference to the DTLS object.
     *
     * @returns  A reference to the DTLS object.
     *
     */
    MeshCoP::Dtls &GetDtls(void);

    /**
     * This method sets the PSK.
     *
     * @param[in]  aPSK        A pointer to the PSK.
     * @param[in]  aPskLength  The PSK length.
     *
     * @retval OT_ERROR_NONE          Successfully set the PSK.
     * @retval OT_ERROR_INVALID_ARGS  The PSK is invalid.
     *
     */
    otError SetPsk(const uint8_t *aPsk, uint8_t aPskLength);

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    /**
     * This method sets the Pre Shared Key (PSK) for DTLS sessions
     * identified by a PSK.
     * DTLS mode "TLS with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aPsk          A pointer to the PSK.
     * @param[in]  aPskLength    The PSK char length.
     * @param[in]  aPskIdentity  The Identity Name for the PSK.
     * @param[in]  aPskIdLength  The PSK Identity Length.
     *
     * @retval OT_ERROR_NONE  Successfully set the PSK.
     *
     */
    otError SetPreSharedKey(const uint8_t *aPsk,
                            uint16_t       aPskLength,
                            const uint8_t *aPskIdentity,
                            uint16_t       aPskIdLength);
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    /**
     * This method sets a X509 certificate with corresponding private key for DTLS session.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509Certificate   A pointer to the PEM formatted X509 PEM certificate.
     * @param[in]  aX509CertLength    The length of certificate.
     * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
     * @param[in]  aPrivateKeyLength  The length of the private key.
     *
     * @retval OT_ERROR_NONE  Successfully set the x509 certificate with his private key.
     *
     */
    otError SetCertificate(const uint8_t *aX509Cert,
                           uint32_t       aX509Length,
                           const uint8_t *aPrivateKey,
                           uint32_t       aPrivateKeyLength);

    /**
     * This method sets the trusted top level CAs. It is needed for validate the
     * certificate of the peer.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
     * @param[in]  aX509CaCertChainLength   The length of chain.
     *
     * @retval OT_ERROR_NONE  Successfully set the the trusted top level CAs.
     *
     */
    otError SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength);
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_BASE64_C
    /**
     * This method returns the peer x509 certificate base64 encoded.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[out]  aPeerCert        A pointer to the base64 encoded certificate buffer.
     * @param[out]  aCertLength      The length of the base64 encoded peer certificate.
     * @param[in]   aCertBufferSize  The buffer size of aPeerCert.
     *
     * @retval OT_ERROR_NONE     Successfully get the peer certificate.
     * @retval OT_ERROR_NO_BUFS  Can't allocate memory for certificate.
     *
     */
    otError GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize);
#endif // MBEDTLS_BASE64_C

    /**
     * This method sets the connected callback to indicate, when
     * a Client connect to the CoAP Secure server.
     *
     * @param[in]  aCallback     A pointer to a function that will be called once DTLS connection is
     * established.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     */
    void SetClientConnectedCallback(ConnectedCallback aCallback, void *aContext);

    /**
     * This method set the authentication mode for the coap secure connection.
     * Disable or enable the verification of peer certificate.
     *
     * @param[in]  aVerifyPeerCertificate  true, if the peer certificate should verify.
     *
     */
    void SetSslAuthMode(bool aVerifyPeerCertificate);

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

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
     * @retval OT_ERROR_NONE           Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retvak OT_ERROR_INVALID_STATE  DTLS connection was not initialized.
     *
     */
    otError SendMessage(Message &aMessage, otCoapResponseHandler aHandler = NULL, void *aContext = NULL);

    /**
     * This method sends a CoAP message over secure DTLS connection.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be NULL pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE           Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retvak OT_ERROR_INVALID_STATE  DTLS connection was not initialized.
     *
     */
    otError SendMessage(Message &               aMessage,
                        const Ip6::MessageInfo &aMessageInfo,
                        otCoapResponseHandler   aHandler = NULL,
                        void *                  aContext = NULL);

    /**
     * This method is used to pass messages to the secure CoAP server.
     * It can be used when messages are received other way that via server's socket.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    virtual void Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method returns the DTLS session's peer address.
     *
     * @return DTLS session's message info.
     *
     */
    const Ip6::MessageInfo &GetPeerMessageInfo(void) const { return mPeerAddress; }

protected:
    void HandleUdpTransmit(void);

private:
    virtual otError Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsConnected(void *aContext, bool aConnected);
    void        HandleDtlsConnected(bool aConnected);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void        HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static otError HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType);
    otError        HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType);

    static void HandleUdpTransmit(Tasklet &aTasklet);

    static void HandleRetransmissionTimer(Timer &aTimer);
    static void HandleResponsesQueueTimer(Timer &aTimer);

    Ip6::MessageInfo  mPeerAddress;
    ConnectedCallback mConnectedCallback;
    void *            mConnectedContext;
    TransportCallback mTransportCallback;
    void *            mTransportContext;
    Message *         mTransmitMessage;
    Tasklet           mTransmitTask;

    bool mLayerTwoSecurity : 1;
};

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

/**
 * This class implements the application CoAP Secure client and server.
 *
 */
class ApplicationCoapSecure : public CoapSecure
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit ApplicationCoapSecure(Instance &aInstance);

private:
    static void HandleRetransmissionTimer(Timer &aTimer);
    static void HandleResponsesQueueTimer(Timer &aTimer);
    static void HandleUdpTransmit(Tasklet &aTasklet);
};

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

} // namespace Coap
} // namespace ot

#endif // COAP_SECURE_HPP_
