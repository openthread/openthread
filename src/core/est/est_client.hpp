/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#ifndef EST_CLIENT_HPP_
#define EST_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

#include <openthread/est_client.h>

#include "coap/coap_secure.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"

/**
 * @file
 *   This file includes definitions for the EST over CoAP Secure client.
 */

namespace ot {
namespace Est {

/**
 * This class implements the EST over CoAP Secure client.
 *
 */
class Client : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit Client(Instance &aInstance);

    /**
     * This method starts the EST over CoAP Secure client.
     *
     * aVerifyPeer              true, if it is possible to verify the EST server with an installed CA
     *                          certificate, otherwise false. E.g. for a re-enrollment.
     *
     * @retval OT_ERROR_NONE     Successfully started the EST client.
     * @retval OT_ERROR_ALREADY  The client is already started.
     */
    otError Start(bool aVerifyPeer);

    /**
     * This method stops the EST over CoAP Secure client.
     *
     */
    void Stop(void);

    /**
     * This method sets the local device's X509 certificate with corresponding private key for
     * DTLS session with DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 to connect to the EST server.
     *
     * @param[in]  aX509Cert          A pointer to the PEM formatted X509 certificate.
     * @param[in]  aX509Length        The length of certificate.
     * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
     * @param[in]  aPrivateKeyLength  The length of the private key.
     *
     * @retval OT_ERROR_NONE              Successfully set the x509 certificate
     *                                    with his private key.
     * @retval OT_ERROR_DISABLED_FEATURE  Mbedtls config not enabled
     *                                    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED.
     *
     */
    otError SetCertificate(const uint8_t *aX509Cert,
                           uint32_t       aX509Length,
                           const uint8_t *aPrivateKey,
                           uint32_t       aPrivateKeyLength);

    /**
     * This method sets the trusted top level CAs. It is needed for validating the
     * certificate of the EST Server, if available. Else start the EST Client without verification.
     *
     * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
     * @param[in]  aX509CaCertChainLength   The length of chain.
     *
     * @retval OT_ERROR_NONE  Successfully set the trusted top level CAs.
     *
     */
    otError SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength);

    /**
     * This method initializes CoAP Secure session with a EST over CoAP Secure server.
     *
     * @param[in]  aSockAddr        A pointer to the EST Server socket address.
     * @param[in]  aConnectHandler  A pointer to a function that will be called when the DTLS connection state changes.
     * @param[in]  aResponseHandler A pointer to a function that will be called by a response form the EST Server or
     * time-out.
     * @param[in]  aContext         A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE  Successfully started CoAP Secure connection.
     *
     */
    otError Connect(const Ip6::SockAddr &     aSockAddr,
                    otHandleEstClientConnect  aConnectHandler,
                    otHandleEstClientResponse aResponseHandler,
                    void *                    aContext);

    /**
     * This method prints CSR attributes in ASN.1 format to a human readable string.
     *
     * @param[in]   aData           A pointer to the beginning of the ASN.1 formatted CSR attributes data.
     * @param[in]   aDataEnd        A pointer to the end of the ASN.1 formatted CSR attributes data.
     * @param[in]   aString         A pointer to a string buffer.
     * @param[in]   aStringLength   Length of the string buffer.
     *
     * @retval OT_ERROR_NONE    Successfully printed CSR attributes to string.
     * @retval OT_ERROR_PARSE   ASN.1 parsing error.
     * @retval OT_ERROR_NO_BUFS Buffer too small.
     */
    otError CsrAttributesToString(uint8_t *aData, const uint8_t *aDataEnd, char *aString, uint32_t aStringLength);

    /**
     * This method terminates the secure connection to the EST server.
     *
     */
    void Disconnect(void);

    /**
     * This method indicates whether or not the EST client is connected to a EST server.
     *
     * @retval true   EST client is connected.
     * @retval false  EST client is not connected.
     *
     */
    bool IsConnected(void);

    /**
     * This method process a simple enrollment over CoAP Secure.
     *
     * @param[in]  aPrivateKey              A pointer to a buffer that contains the EC private key.
     * @param[in]  aPrivateLeyLength        The length of the buffer @p aPrivateKey
     * @param[in]  aMdType                  The message digest type to be used for simple enrollment.
     * @param[in]  aKeyUsageFlags           Flags for defining key usage. Refer OT_EST_KEY_USAGE_*
     * @param[in]  aX509Extensions          A pointer to ASN.1 DER encoded data that contain the X.509
     *                                      extensions to set.
     * @param[in]  aX509ExtensionsLength    The length of the data @p aX509Extensions
     *
     * @retval OT_ERROR_NONE           Successfully sent request.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  EST client not connected.
     *
     */
    otError SimpleEnroll(const uint8_t *aPrivateKey,
                         uint32_t       aPrivateLeyLength,
                         otMdType       aMdType,
                         uint8_t        aKeyUsageFlags,
                         uint8_t *      aX509Extensions,
                         uint32_t       aX509ExtensionsLength);

    /**
     * This method process a simple re-enrollment over CoAP Secure.
     *
     * @param[in]  aPrivateKey              A pointer to a buffer that contains the EC private key.
     * @param[in]  aPrivateLeyLength        The length of the buffer @p aPrivateKey
     * @param[in]  aMdType                  The message digest type to be used for simple enrollment.
     * @param[in]  aKeyUsageFlags           Flags for defining key usage. Refer OT_EST_KEY_USAGE_*
     * @param[in]  aX509Extensions          A pointer to ASN.1 DER encoded data that contain the X.509
     *                                      extensions to set.
     * @param[in]  aX509ExtensionsLength    The length of the data @p aX509Extensions
     *
     * @retval OT_ERROR_NONE           Successfully sent request.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  EST client not connected.
     *
     */
    otError SimpleReEnroll(const uint8_t *aPrivateKey,
                           uint32_t       aPrivateLeyLength,
                           otMdType       aMdType,
                           uint8_t        aKeyUsageFlags,
                           uint8_t *      aX509Extensions,
                           uint32_t       aX509ExtensionsLength);

    /**
     * This method requests the CSR attributes from the EST server.
     *
     * @retval OT_ERROR_NONE           Successfully sent request.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  EST client not connected.
     */
    otError GetCsrAttributes(void);

    /**
     * ToDo: Optionally
     */
    otError GetServerGeneratedKeys(void);

    /**
     * This method requests the EST CA certificate from the EST server.
     * The response callback should return the public key certificate with which the client can
     * verify object singed by the EST CA [RFC7030, draft-ietf-ace-coap-est-12].
     *
     * @retval OT_ERROR_NONE           Successfully sent request.
     * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_STATE  EST client not connected.
     */
    otError GetCaCertificates(void);

private:
    enum
    {
        kLocalPort = 12345, // the devices local port to use for the EST over CoAPS client
    };

    static void CoapSecureConnectedHandle(bool aConnected, void *aContext);
    void        CoapSecureConnectedHandle(bool aConnected);
    otError CmsReadSignedData(uint8_t *aMessage, uint32_t aMessageLength, uint8_t **aPayload, uint32_t *aPayloadLength);
    otError WriteCsr(const uint8_t *aPrivateKey,
                     size_t         aPrivateLeyLength,
                     otMdType       aMdType,
                     uint8_t        aKeyUsageFlags,
                     uint8_t *      aX509Extensions,
                     uint32_t       aX509ExtensionsLength,
                     uint8_t *      aOutput,
                     size_t *       aOutputLength);
    static void SimpleEnrollResponseHandler(void *               aContext,
                                            otMessage *          aMessage,
                                            const otMessageInfo *aMessageInfo,
                                            otError              aResult);
    void        SimpleEnrollResponseHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult);
    static void GetCaCertificatesResponseHandler(void *               aContext,
                                                 otMessage *          aMessage,
                                                 const otMessageInfo *aMessageInfo,
                                                 otError              aResult);
    void GetCaCertificatesResponseHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult);
    static void GetCsrAttributesResponseHandler(void *               aContext,
                                                otMessage *          aMessage,
                                                const otMessageInfo *aMessageInfo,
                                                otError              aResult);
    void GetCsrAttributesResponseHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult);

    bool                      mIsConnected;
    bool                      mStarted;
    bool                      mVerifyEstServerCertificate;
    bool                      mIsEnroll;
    bool                      mIsEnrolled;
    void *                    mApplicationContext;
    otHandleEstClientConnect  mConnectCallback;
    otHandleEstClientResponse mResponseCallback;
    Coap::CoapSecure          mCoapSecure;
};

} // namespace Est
} // namespace ot

#endif // OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

#endif // EST_CLIENT_HPP_
