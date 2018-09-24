/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 * @brief
 *  This file defines the top-level functions for the OpenThread CoAP Secure implementation.
 *
 *  @note
 *   To enable cipher suite DTLS_PSK_WITH_AES_128_CCM_8, MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
 *    must be enabled in mbedtls-config.h
 *   To enable cipher suite DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
 *    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED must be enabled in mbedtls-config.h.
 */

#ifndef OPENTHREAD_COAP_SECURE_H_
#define OPENTHREAD_COAP_SECURE_H_

#include <stdint.h>

#include <openthread/coap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-coap-secure
 *
 * @brief
 *   This module includes functions that control CoAP Secure (CoAP over DTLS) communication.
 *
 *   The functions in this module are available when application-coap-secure feature
 *   (`OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE`) is enabled.
 *
 * @{
 *
 */

#define OT_DEFAULT_COAP_SECURE_PORT 5684 ///< Default CoAP Secure port, as specified in RFC 7252

/**
 * This function pointer is called when the DTLS connection state changes.
 *
 * @param[in]  aConnected  true, if a connection was established, false otherwise.
 * @param[in]  aContext    A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleCoapSecureClientConnect)(bool aConnected, void *aContext);

/**
 * This function starts the CoAP Secure service.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aPort      The local UDP port to bind to.
 * @param[in]  aContext   A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE  Successfully started the CoAP Secure server.
 *
 */
otError otCoapSecureStart(otInstance *aInstance, uint16_t aPort, void *aContext);

/**
 * This function stops the CoAP Secure server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully stopped the CoAP Secure server.
 */
otError otCoapSecureStop(otInstance *aInstance);

/**
 * This method sets the Pre-Shared Key (PSK) and cipher suite
 * DTLS_PSK_WITH_AES_128_CCM_8.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPSK          A pointer to the PSK.
 * @param[in]  aPskLength    The PSK length.
 * @param[in]  aPskIdentity  The Identity Name for the PSK.
 * @param[in]  aPskIdLength  The PSK Identity Length.
 *
 * @retval OT_ERROR_NONE              Successfully set the PSK.
 * @retval OT_ERROR_INVALID_ARGS      The PSK is invalid.
 * @retval OT_ERROR_DISABLED_FEATURE  Mbedtls config not enabled
 *                                    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
 *
 */
otError otCoapSecureSetPsk(otInstance *   aInstance,
                           const uint8_t *aPsk,
                           uint16_t       aPskLength,
                           const uint8_t *aPskIdentity,
                           uint16_t       aPskIdLength);

/**
 * This method returns the peer x509 certificate base64 encoded.
 *
 * @param[in]   aInstance        A pointer to an OpenThread instance.
 * @param[out]  aPeerCert        A pointer to the base64 encoded certificate buffer.
 * @param[out]  aCertLength      The length of the base64 encoded peer certificate.
 * @param[in]   aCertBufferSize  The buffer size of aPeerCert.
 *
 * @retval OT_ERROR_NONE              Successfully get the peer certificate.
 * @retval OT_ERROR_DISABLED_FEATURE  Mbedtls config not enabled MBEDTLS_BASE64_C.
 *
 */
otError otCoapSecureGetPeerCertificateBase64(otInstance *   aInstance,
                                             unsigned char *aPeerCert,
                                             uint64_t *     aCertLength,
                                             uint64_t       aCertBufferSize);

/**
 * This method sets the authentication mode for the coap secure connection.
 *
 * Disable or enable the verification of peer certificate.
 * Must be called before start.
 *
 * @param[in]   aInstance               A pointer to an OpenThread instance.
 * @param[in]   aVerifyPeerCertificate  true, to verify the peer certificate.
 *
 */
void otCoapSecureSetSslAuthMode(otInstance *aInstance, bool aVerifyPeerCertificate);

/**
 * This method sets the local device's X509 certificate with corresponding private key for
 * DTLS session with DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aX509Certificate   A pointer to the PEM formatted X509 certificate.
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
otError otCoapSecureSetCertificate(otInstance *   aInstance,
                                   const uint8_t *aX509Cert,
                                   uint32_t       aX509Length,
                                   const uint8_t *aPrivateKey,
                                   uint32_t       aPrivateKeyLength);

/**
 * This method sets the trusted top level CAs. It is needed for validating the
 * certificate of the peer.
 *
 * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
 *
 * @param[in]  aInstance                A pointer to an OpenThread instance.
 * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
 * @param[in]  aX509CaCertChainLength   The length of chain.
 *
 * @retval OT_ERROR_NONE  Successfully set the the trusted top level CAs.
 *
 */
otError otCoapSecureSetCaCertificateChain(otInstance *   aInstance,
                                          const uint8_t *aX509CaCertificateChain,
                                          uint32_t       aX509CaCertChainLength);

/**
 * This method initializes DTLS session with a peer.
 *
 * @param[in]  aInstance               A pointer to an OpenThread instance.
 * @param[in]  aMessageInfo            A pointer to a message info structure.
 * @param[in]  aCallback               A pointer to a function that will be called when the DTLS connection
 *                                     state changes.
 * @param[in]  aContext                A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE  Successfully started DTLS connection.
 *
 */
otError otCoapSecureConnect(otInstance *                    aInstance,
                            const otMessageInfo *           aMessageInfo,
                            otHandleCoapSecureClientConnect aHandler,
                            void *                          aContext);

/**
 * This method stops the DTLS connection.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully stopped the DTLS connection.
 *
 */
otError otCoapSecureDisconnect(otInstance *aInstance);

/**
 * This method indicates whether or not the DTLS session is connected.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE   The DTLS session is connected.
 * @retval FALSE  The DTLS session is not connected.
 *
 */
bool otCoapSecureIsConnected(otInstance *aInstance);

/**
 * This method indicates whether or not the DTLS session is active.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE  If DTLS session is active.
 * @retval FALSE If DTLS session is not active.
 *
 */
bool otCoapSecureIsConncetionActive(otInstance *aInstance);

/**
 * This method sends a CoAP request over secure DTLS connection.
 *
 * If a response for a request is expected, respective function and context information should be provided.
 * If no response is expected, these arguments should be NULL pointers.
 * If Message Id was not set in the header (equal to 0), this function will assign unique Message Id to the message.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A reference to the message to send.
 * @param[in]  aHandler      A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext      A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE           Successfully sent CoAP message.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
 * @retvak OT_ERROR_INVALID_STATE  DTLS connection was not initialized.
 *
 */
otError otCoapSecureSendRequest(otInstance *          aInstance,
                                otMessage *           aMessage,
                                otCoapResponseHandler aHandler,
                                void *                aContext);

/**
 * This function adds a resource to the CoAP Secure server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 * @retval OT_ERROR_NONE     Successfully added @p aResource.
 * @retval OT_ERROR_ALREADY  The @p aResource was already added.
 *
 */
otError otCoapSecureAddResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function removes a resource from the CoAP Secure server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aResource  A pointer to the resource.
 *
 */
void otCoapSecureRemoveResource(otInstance *aInstance, otCoapResource *aResource);

/**
 * This function sets the default handler for unhandled CoAP Secure requests.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aHandler   A function pointer that shall be called when an unhandled request arrives.
 * @param[in]  aContext   A pointer to arbitrary context information. May be NULL if not used.
 *
 */
void otCoapSecureSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext);

/**
 * This method sets the connected callback to indicate, when
 * a Client connect to the CoAP Secure server.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessageInfo  A pointer to a message info structure.
 * @param[in]  aHandler      A pointer to a function that will be called once DTLS connection is established.
 *
 */
void otCoapSecureSetClientConnectedCallback(otInstance *                    aInstance,
                                            otHandleCoapSecureClientConnect aHandler,
                                            void *                          aContext);

/**
 * This function sends a CoAP response from the CoAP Secure server.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the CoAP response to send.
 * @param[in]  aMessageInfo  A pointer to the message info associated with @p aMessage.
 *
 * @retval OT_ERROR_NONE     Successfully enqueued the CoAP response message.
 * @retval OT_ERROR_NO_BUFS  Insufficient buffers available to send the CoAP response.
 *
 */
otError otCoapSecureSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OPENTHREAD_COAP_SECURE_H_ */
