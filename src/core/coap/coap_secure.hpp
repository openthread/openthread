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

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

#include "coap/coap.hpp"
#include "common/callback.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/secure_transport.hpp"

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
     * Function pointer which is called reporting a connection event (when connection established or disconnected)
     */
    typedef otHandleCoapSecureClientConnect ConnectEventCallback;

    /**
     * Callback to notify when the agent is automatically stopped due to reaching the maximum number of connection
     * attempts.
     */
    typedef otCoapSecureAutoStopCallback AutoStopCallback;

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance           A reference to the OpenThread instance.
     * @param[in]  aLayerTwoSecurity   Specifies whether to use layer two security or not.
     */
    explicit CoapSecure(Instance &aInstance, bool aLayerTwoSecurity = false);

    /**
     * Starts the secure CoAP agent.
     *
     * @param[in]  aPort      The local UDP port to bind to.
     *
     * @retval kErrorNone        Successfully started the CoAP agent.
     * @retval kErrorAlready     Already started.
     */
    Error Start(uint16_t aPort);

    /**
     * Starts the secure CoAP agent and sets the maximum number of allowed connection attempts before stopping the
     * agent automatically.
     *
     * @param[in] aPort           The local UDP port to bind to.
     * @param[in] aMaxAttempts    Maximum number of allowed connection request attempts. Zero indicates no limit.
     * @param[in] aCallback       Callback to notify if max number of attempts has reached and agent is stopped.
     * @param[in] aContext        A pointer to arbitrary context to use with `AutoStopCallback`.
     *
     * @retval kErrorNone        Successfully started the CoAP agent.
     * @retval kErrorAlready     Already started.
     */
    Error Start(uint16_t aPort, uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext);

    /**
     * Starts the secure CoAP agent, but do not use socket to transmit/receive messages.
     *
     * @param[in]  aCallback  A pointer to a function for sending messages.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     * @retval kErrorNone        Successfully started the CoAP agent.
     * @retval kErrorAlready     Already started.
     */
    Error Start(MeshCoP::SecureTransport::TransportCallback aCallback, void *aContext);

    /**
     * Sets connected callback of this secure CoAP agent.
     *
     * @param[in]  aCallback  A pointer to a function to get called when connection state changes.
     * @param[in]  aContext   A pointer to arbitrary context information.
     */
    void SetConnectEventCallback(ConnectEventCallback aCallback, void *aContext)
    {
        mConnectEventCallback.Set(aCallback, aContext);
    }

    /**
     * Stops the secure CoAP agent.
     */
    void Stop(void);

    /**
     * Initializes DTLS session with a peer.
     *
     * @param[in]  aSockAddr               A reference to the remote socket address,
     * @param[in]  aCallback               A pointer to a function that will be called once DTLS connection is
     * established.
     *
     * @retval kErrorNone  Successfully started DTLS connection.
     */
    Error Connect(const Ip6::SockAddr &aSockAddr, ConnectEventCallback aCallback, void *aContext);

    /**
     * Indicates whether or not the DTLS session is active.
     *
     * @retval TRUE  If DTLS session is active.
     * @retval FALSE If DTLS session is not active.
     */
    bool IsConnectionActive(void) const { return mDtls.IsConnectionActive(); }

    /**
     * Indicates whether or not the DTLS session is connected.
     *
     * @retval TRUE   The DTLS session is connected.
     * @retval FALSE  The DTLS session is not connected.
     */
    bool IsConnected(void) const { return mDtls.IsConnected(); }

    /**
     * Indicates whether or not the DTLS session is closed.
     *
     * @retval TRUE   The DTLS session is closed
     * @retval FALSE  The DTLS session is not closed.
     */
    bool IsClosed(void) const { return mDtls.IsClosed(); }

    /**
     * Stops the DTLS connection.
     */
    void Disconnect(void) { mDtls.Disconnect(); }

    /**
     * Returns a reference to the DTLS object.
     *
     * @returns  A reference to the DTLS object.
     */
    MeshCoP::SecureTransport &GetDtls(void) { return mDtls; }

    /**
     * Gets the UDP port of this agent.
     *
     * @returns  UDP port number.
     */
    uint16_t GetUdpPort(void) const { return mDtls.GetUdpPort(); }

    /**
     * Sets the PSK.
     *
     * @param[in]  aPsk        A pointer to the PSK.
     * @param[in]  aPskLength  The PSK length.
     *
     * @retval kErrorNone         Successfully set the PSK.
     * @retval kErrorInvalidArgs  The PSK is invalid.
     */
    Error SetPsk(const uint8_t *aPsk, uint8_t aPskLength) { return mDtls.SetPsk(aPsk, aPskLength); }

    /**
     * Sets the PSK.
     *
     * @param[in]  aPskd  A Joiner PSKd.
     */
    void SetPsk(const MeshCoP::JoinerPskd &aPskd);

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    /**
     * Sets the Pre-Shared Key (PSK) for DTLS sessions identified by a PSK.
     *
     * DTLS mode "TLS with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aPsk          A pointer to the PSK.
     * @param[in]  aPskLength    The PSK char length.
     * @param[in]  aPskIdentity  The Identity Name for the PSK.
     * @param[in]  aPskIdLength  The PSK Identity Length.
     */
    void SetPreSharedKey(const uint8_t *aPsk, uint16_t aPskLength, const uint8_t *aPskIdentity, uint16_t aPskIdLength)
    {
        mDtls.SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
    }
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    /**
     * Sets a X509 certificate with corresponding private key for DTLS session.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509Cert          A pointer to the PEM formatted X509 PEM certificate.
     * @param[in]  aX509Length        The length of certificate.
     * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
     * @param[in]  aPrivateKeyLength  The length of the private key.
     */
    void SetCertificate(const uint8_t *aX509Cert,
                        uint32_t       aX509Length,
                        const uint8_t *aPrivateKey,
                        uint32_t       aPrivateKeyLength)
    {
        mDtls.SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
    }

    /**
     * Sets the trusted top level CAs. It is needed for validate the certificate of the peer.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
     * @param[in]  aX509CaCertChainLength   The length of chain.
     */
    void SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
    {
        mDtls.SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
    }
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
    /**
     * Returns the peer x509 certificate base64 encoded.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[out]  aPeerCert        A pointer to the base64 encoded certificate buffer.
     * @param[out]  aCertLength      The length of the base64 encoded peer certificate.
     * @param[in]   aCertBufferSize  The buffer size of aPeerCert.
     *
     * @retval kErrorNone    Successfully get the peer certificate.
     * @retval kErrorNoBufs  Can't allocate memory for certificate.
     */
    Error GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
    {
        return mDtls.GetPeerCertificateBase64(aPeerCert, aCertLength, aCertBufferSize);
    }
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

    /**
     * Sets the authentication mode for the CoAP secure connection. It disables or enables the verification
     * of peer certificate.
     *
     * @param[in]  aVerifyPeerCertificate  true, if the peer certificate should be verified
     */
    void SetSslAuthMode(bool aVerifyPeerCertificate) { mDtls.SetSslAuthMode(aVerifyPeerCertificate); }

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    /**
     * Sends a CoAP message over secure DTLS connection.
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

    /**
     * Sends a CoAP message over secure DTLS connection.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be NULL pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
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
                      const Ip6::MessageInfo     &aMessageInfo,
                      ResponseHandler             aHandler      = nullptr,
                      void                       *aContext      = nullptr,
                      otCoapBlockwiseTransmitHook aTransmitHook = nullptr,
                      otCoapBlockwiseReceiveHook  aReceiveHook  = nullptr);
#else  // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    /**
     * Sends a CoAP message over secure DTLS connection.
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

    /**
     * Sends a CoAP message over secure DTLS connection.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be nullptr pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval kErrorNone          Successfully sent CoAP message.
     * @retval kErrorNoBufs        Failed to allocate retransmission data.
     * @retval kErrorInvalidState  DTLS connection was not initialized.
     */
    Error SendMessage(Message                &aMessage,
                      const Ip6::MessageInfo &aMessageInfo,
                      ResponseHandler         aHandler = nullptr,
                      void                   *aContext = nullptr);
#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

    /**
     * Is used to pass UDP messages to the secure CoAP server.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     */
    void HandleUdpReceive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        return mDtls.HandleReceive(aMessage, aMessageInfo);
    }

    /**
     * Returns the DTLS session's peer address.
     *
     * @return DTLS session's message info.
     */
    const Ip6::MessageInfo &GetMessageInfo(void) const { return mDtls.GetMessageInfo(); }

private:
    Error Open(uint16_t aMaxAttempts, AutoStopCallback aCallback, void *aContext);

    static Error Send(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        return static_cast<CoapSecure &>(aCoapBase).Send(aMessage, aMessageInfo);
    }
    Error Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsConnectEvent(MeshCoP::SecureTransport::ConnectEvent aEvent, void *aContext);
    void        HandleDtlsConnectEvent(MeshCoP::SecureTransport::ConnectEvent aEvent);

    static void HandleDtlsAutoClose(void *aContext);
    void        HandleDtlsAutoClose(void);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void        HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static void HandleTransmit(Tasklet &aTasklet);
    void        HandleTransmit(void);

    MeshCoP::SecureTransport       mDtls;
    Callback<ConnectEventCallback> mConnectEventCallback;
    Callback<AutoStopCallback>     mAutoStopCallback;
    ot::MessageQueue               mTransmitQueue;
    TaskletContext                 mTransmitTask;
};

} // namespace Coap
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

#endif // COAP_SECURE_HPP_
