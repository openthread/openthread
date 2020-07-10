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
#include "meshcop/meshcop.hpp"

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
     * This constructor initializes the object.
     *
     * @param[in]  aInstance           A reference to the OpenThread instance.
     * @param[in]  aLayerTwoSecurity   Specifies whether to use layer two security or not.
     *
     */
    explicit CoapSecure(Instance &aInstance, bool aLayerTwoSecurity = false);

    /**
     * This method starts the secure CoAP agent.
     *
     * @param[in]  aPort      The local UDP port to bind to.
     *
     * @retval OT_ERROR_NONE        Successfully started the CoAP agent.
     * @retval OT_ERROR_ALREADY     Already started.
     *
     */
    otError Start(uint16_t aPort);

    /**
     * This method starts the secure CoAP agent, but do not use socket to transmit/receive messages.
     *
     * @param[in]  aCallback  A pointer to a function for sending messages.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE        Successfully started the CoAP agent.
     * @retval OT_ERROR_ALREADY     Already started.
     *
     */
    otError Start(MeshCoP::Dtls::TransportCallback aCallback, void *aContext);

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
     */
    void Stop(void);

    /**
     * This method initializes DTLS session with a peer.
     *
     * @param[in]  aSockAddr               A reference to the remote socket address,
     * @param[in]  aCallback               A pointer to a function that will be called once DTLS connection is
     * established.
     *
     * @retval OT_ERROR_NONE  Successfully started DTLS connection.
     *
     */
    otError Connect(const Ip6::SockAddr &aSockAddr, ConnectedCallback aCallback, void *aContext);

    /**
     * This method indicates whether or not the DTLS session is active.
     *
     * @retval TRUE  If DTLS session is active.
     * @retval FALSE If DTLS session is not active.
     *
     */
    bool IsConnectionActive(void) const { return mDtls.IsConnectionActive(); }

    /**
     * This method indicates whether or not the DTLS session is connected.
     *
     * @retval TRUE   The DTLS session is connected.
     * @retval FALSE  The DTLS session is not connected.
     *
     */
    bool IsConnected(void) const { return mDtls.IsConnected(); }

    /**
     * This method stops the DTLS connection.
     *
     */
    void Disconnect(void) { mDtls.Disconnect(); }

    /**
     * This method returns a reference to the DTLS object.
     *
     * @returns  A reference to the DTLS object.
     *
     */
    MeshCoP::Dtls &GetDtls(void) { return mDtls; }

    /**
     * This method sets the PSK.
     *
     * @param[in]  aPsk        A pointer to the PSK.
     * @param[in]  aPskLength  The PSK length.
     *
     * @retval OT_ERROR_NONE          Successfully set the PSK.
     * @retval OT_ERROR_INVALID_ARGS  The PSK is invalid.
     *
     */
    otError SetPsk(const uint8_t *aPsk, uint8_t aPskLength);

    /**
     * This method sets the PSK.
     *
     * @param[in]  aPskd  A Joiner PSKd.
     *
     */
    void SetPsk(const MeshCoP::JoinerPskd &aPskd);

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    /**
     * This method sets the Pre-Shared Key (PSK) for DTLS sessions identified by a PSK.
     *
     * DTLS mode "TLS with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aPsk          A pointer to the PSK.
     * @param[in]  aPskLength    The PSK char length.
     * @param[in]  aPskIdentity  The Identity Name for the PSK.
     * @param[in]  aPskIdLength  The PSK Identity Length.
     *
     */
    void SetPreSharedKey(const uint8_t *aPsk, uint16_t aPskLength, const uint8_t *aPskIdentity, uint16_t aPskIdLength)
    {
        mDtls.SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
    }
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
     */
    void SetCertificate(const uint8_t *aX509Cert,
                        uint32_t       aX509Length,
                        const uint8_t *aPrivateKey,
                        uint32_t       aPrivateKeyLength);

    /**
     * This method sets the trusted top level CAs. It is needed for validate the certificate of the peer.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
     * @param[in]  aX509CaCertChainLength   The length of chain.
     *
     */
    void SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength);
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
     * This method sets the connected callback to indicate, when a Client connect to the CoAP Secure server.
     *
     * @param[in]  aCallback     A pointer to a function that will be called once DTLS connection is established.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     */
    void SetClientConnectedCallback(ConnectedCallback aCallback, void *aContext);

    /**
     * This method sets the authentication mode for the CoAP secure connection. It disables or enables the verification
     * of peer certificate.
     *
     * @param[in]  aVerifyPeerCertificate  true, if the peer certificate should be verified
     *
     */
    void SetSslAuthMode(bool aVerifyPeerCertificate);

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

    /**
     * This method sends a CoAP message over secure DTLS connection.
     *
     * If a response for a request is expected, respective function and context information should be provided.
     * If no response is expected, these arguments should be nullptr pointers.
     * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
     *
     * @param[in]  aMessage      A reference to the message to send.
     * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext      A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE           Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  DTLS connection was not initialized.
     *
     */
    otError SendMessage(Message &aMessage, ResponseHandler aHandler = nullptr, void *aContext = nullptr);

    /**
     * This method sends a CoAP message over secure DTLS connection.
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
     * @retval OT_ERROR_NONE           Successfully sent CoAP message.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  DTLS connection was not initialized.
     *
     */
    otError SendMessage(Message &               aMessage,
                        const Ip6::MessageInfo &aMessageInfo,
                        ResponseHandler         aHandler = nullptr,
                        void *                  aContext = nullptr);

    /**
     * This method is used to pass UDP messages to the secure CoAP server.
     *
     * @param[in]  aMessage      A reference to the received message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    void HandleUdpReceive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        return mDtls.HandleUdpReceive(aMessage, aMessageInfo);
    }

    /**
     * This method returns the DTLS session's peer address.
     *
     * @return DTLS session's message info.
     *
     */
    const Ip6::MessageInfo &GetMessageInfo(void) const { return mDtls.GetMessageInfo(); }

private:
    static otError Send(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        return static_cast<CoapSecure &>(aCoapBase).Send(aMessage, aMessageInfo);
    }
    otError Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsConnected(void *aContext, bool aConnected);
    void        HandleDtlsConnected(bool aConnected);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void        HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static void HandleTransmit(Tasklet &aTasklet);
    void        HandleTransmit(void);

    MeshCoP::Dtls     mDtls;
    ConnectedCallback mConnectedCallback;
    void *            mConnectedContext;
    ot::MessageQueue  mTransmitQueue;
    TaskletContext    mTransmitTask;
};

} // namespace Coap
} // namespace ot

#endif // COAP_SECURE_HPP_
