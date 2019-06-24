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

/**
 * @file
 * @brief
 *  This file defines the top-level functions for the OpenThread EST over CoAP Client
 *  implementation according to draft-ietf-ace-coap-est-06 based on EST - RFC 7030.
 */

#ifndef OPENTHREAD_EST_CLIENT_H_
#define OPENTHREAD_EST_CLIENT_H_

#include <stdint.h>

#include <mbedtls/x509.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-est-client
 *
 * @brief
 *   This module includes functions that control the EST-coaps (EST over CoAPS) client.
 *
 *   The functions in this module are available when est-client feature
 *   (`OPENTHREAD_ENABLE_EST_CLIENT`) is enabled.
 *
 * @{
 *
 */

#define OT_EST_COAPS_DEFAULT_EST_SERVER_IP6 "2001:620:190:ffa1:21b:21ff:fe70:9240"
#define OT_EST_COAPS_DEFAULT_EST_SERVER_PORT 5684

#define OT_EST_COAPS_SHORT_URI_CA_CERTS ".well-known/est/crts"        ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SIMPLE_ENROLL ".well-known/est/sen"    ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SIMPLE_REENROLL ".well-known/est/sren" ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_CSR_ATTRS ".well-known/est/att"        ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SERVER_KEY_GEN ".well-known/est/skg"   ///< Specified in draft-ietf-ace-coap-est-12

/**
 * Key usage type for the X.509 certificate used in the EST client.
 */
#define OT_EST_KEY_USAGE_DIGITAL_SIGNATURE MBEDTLS_X509_KU_DIGITAL_SIGNATURE
#define OT_EST_KEY_USAGE_NON_REPUTATION MBEDTLS_X509_KU_NON_REPUDIATION
#define OT_EST_KEY_USAGE_KEY_ENCIPHERMENT MBEDTLS_X509_KU_KEY_ENCIPHERMENT
#define OT_EST_KEY_USAGE_DATA_ENCIPHERMENT MBEDTLS_X509_KU_DATA_ENCIPHERMENT
#define OT_EST_KEY_USAGE_KEY_AGREEMENT MBEDTLS_X509_KU_KEY_AGREEMENT
#define OT_EST_KEY_USAGE_KEY_CERT_SIGN MBEDTLS_X509_KU_KEY_CERT_SIGN
#define OT_EST_KEY_USAGE_CRL_SIGN MBEDTLS_X509_KU_CRL_SIGN
#define OT_EST_KEY_USAGE_ENCIPHER_ONLY MBEDTLS_X509_KU_ENCIPHER_ONLY
#define OT_EST_KEY_USAGE_DECIPHER_ONLY MBEDTLS_X509_KU_DECIPHER_ONLY

/**
 * Type description for the EST response handle.
 */
typedef enum otEstType
{
    OT_EST_TYPE_NONE,
    OT_EST_TYPE_SIMPLE_ENROLL,
    OT_EST_TYPE_SIMPLE_REENROLL,
    OT_EST_TYPE_CA_CERTS,
    OT_EST_TYPE_SERVER_SIDE_KEY,
    OT_EST_TYPE_CSR_ATTR,
    OT_EST_TYPE_INVALID_CERT,
    OT_EST_TYPE_INVALID_KEY
} otEstType;

/**
 * Supported message digest.
 */
typedef enum otMdType
{
    OT_MD_TYPE_NONE      = 0,
    OT_MD_TYPE_MD5       = MBEDTLS_MD_MD5,
    OT_MD_TYPE_SHA256    = MBEDTLS_MD_SHA256,
    OT_MD_TYPE_SHA384    = MBEDTLS_MD_SHA384,
    OT_MD_TYPE_SHA512    = MBEDTLS_MD_SHA512,
    OT_MD_TYPE_RIPEMD160 = MBEDTLS_MD_RIPEMD160
} otMdType;

/**
 * This function pointer is called when the connection state to the EST-coaps server changes.
 *
 * @param[in]  aConnected  true, if a connection was established, false otherwise.
 * @param[in]  aContext    A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleEstClientConnect)(bool aConnected, void *aContext);

/**
 * This function pointer is called when the server responds after an EST operation.
 *
 * @param[in]  aError           OT_ERROR_NONE if the EST operation was successful
 * @param[in]  aType            The response type of the EST operation that called the handler.
 * @param[in]  aPayload         The payload associated with @p aType.
 * @param[in]  aPayloadLength   The length of the payload @p aPayload.
 * @param[in]  aContext         A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleEstClientResponse)(otError   aError,
                                          otEstType aType,
                                          uint8_t * aPayload,
                                          uint32_t  aPayloadLength,
                                          void *    aContext);

/**
 * This function starts the EST over CoAP Secure client service.
 *
 * @param[in]  aInstance                A pointer to an OpenThread instance.
 * @param[in]  aVerifyPeer              true, if it is possible to verify the EST server with an installed CA
 *                                      certificate, otherwise false. E.g. for a re-enrollment.
 *
 * @retval OT_ERROR_NONE  Successfully started the EST client.
 *
 */
otError otEstClientStart(otInstance *aInstance, bool aVerifyPeer);

/**
 * This function stops the EST over CoAP Secure client.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
void otEstClientStop(otInstance *aInstance);

/**
 * This method sets the local device's X509 certificate with corresponding private key for
 * DTLS session with DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 to connect to the EST server.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
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
otError otEstClientSetCertificate(otInstance *   aInstance,
                                  const uint8_t *aX509Cert,
                                  uint32_t       aX509Length,
                                  const uint8_t *aPrivateKey,
                                  uint32_t       aPrivateKeyLength);

/**
 * This method sets the trusted top level CAs. It is needed for validating the
 * certificate of the EST Server, if available. Else start the EST Client without verification.
 *
 * @param[in]  aInstance                A pointer to an OpenThread instance.
 * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
 * @param[in]  aX509CaCertChainLength   The length of chain.
 *
 * @retval OT_ERROR_NONE  Successfully set the trusted top level CAs.
 *
 */
otError otEstClientSetCaCertificateChain(otInstance *   aInstance,
                                         const uint8_t *aX509CaCertificateChain,
                                         uint32_t       aX509CaCertChainLength);

/**
 * This method initializes CoAP Secure session with a EST over CoAP Secure server.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aSockAddr        A pointer to the EST Server sockaddr.
 * @param[in]  aConnectHandler  A pointer to a function that will be called when the DTLS connection state changes.
 * @param[in]  aResponseHandler A pointer to a function that will be called by a response form the EST Server or
 * time-out.
 * @param[in]  aContext         A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE  Successfully started CoAP Secure connection.
 *
 */
otError otEstClientConnect(otInstance *              aInstance,
                           const otSockAddr *        aSockAddr,
                           otHandleEstClientConnect  aConnectHandler,
                           otHandleEstClientResponse aResponseHandler,
                           void *                    aContext);

/**
 * This method terminates the secure connection to the EST server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
void otEstClientDisconnect(otInstance *aInstance);

/**
 * This method indicates whether or not the EST client is connected to a EST server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval true   EST client is connected.
 * @retval false  EST client is not connected.
 *
 */
bool otEstClientIsConnected(otInstance *aInstance);

/**
 * This method process a simple enrollment over CoAP Secure.
 * The response callback should return the signed certificate after execution this step.
 * Note: With the function otCryptoEcpGenenrateKey a new EC key pair can be generated.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aPrivateKey        A pointer to a buffer that contains the EC private key.
 * @param[in]  aPrivateLeyLength  The length of the buffer @p aPrivateKey
 * @param[in]  aMdType            The message digest type to be used for simple enrollment.
 * @param[in]  aKeyUsageFlags     Flags for defining key usage. Refer OT_EST_KEY_USAGE_*
 *
 * @retval OT_ERROR_NONE           Successfully sent request.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
 * @retval OT_ERROR_INVALID_STATE  EST client not connected.
 *
 */
otError otEstClientSimpleEnroll(otInstance *   aInstance,
                                const uint8_t *aPrivateKey,
                                uint32_t       aPrivateLeyLength,
                                otMdType       aMdType,
                                uint8_t        aKeyUsageFlags);

/**
 * This method process a simple re-enrollment over CoAP Secure.
 * The response callback should return the renewed signed certificate after execution this step.
 * Note: With the function otCryptoEcpGenenrateKey a new EC key pair can be generated.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aPrivateKey        A pointer to a buffer that contains the EC private key.
 * @param[in]  aPrivateLeyLength  The length of the buffer @p aPrivateKey
 * @param[in]  aMdType            The message digest type to be used for simple enrollment.
 * @param[in]  aKeyUsageFlags     Flags for defining key usage. Refer OT_EST_KEY_USAGE_*
 *
 * @retval OT_ERROR_NONE           Successfully sent request.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
 * @retval OT_ERROR_INVALID_STATE  EST client not connected.
 *
 */
otError otEstClientSimpleReEnroll(otInstance *   aInstance,
                                  const uint8_t *aPrivateKey,
                                  uint32_t       aPrivateLeyLength,
                                  otMdType       aMdType,
                                  uint8_t        aKeyUsageFlags);

/**
 * ToDo: Optionally
 */
otError otEstClientGetCsrAttributes(otInstance *aInstance);

/**
 * ToDo: Optionally
 */
otError otEstClientGetServerGeneratedKeys(otInstance *aInstance);

/**
 * ToDo: Optionally
 */
otError otEstClientGetCaCertificates(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OPENTHREAD_EST_CLIENT_H_ */
