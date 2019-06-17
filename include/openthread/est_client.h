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
 *  implementation according to draft-ietf-ace-coap-est-06 (EST - RFC 7030).
 */

#ifndef OPENTHREAD_EST_CLIENT_H_
#define OPENTHREAD_EST_CLIENT_H_

#include <stdint.h>

#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-coap-secure
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

#define OT_EST_COAPS_DEFAULT_EST_SERVER_IP6     "2001:620:190:ffa1::1234:4321"
#define OT_EST_COAPS_DEFAULT_EST_SERVER_PORT    5684

/**
 * ToDo: bring down to a lower layer
 */
#define OT_EST_COAPS_SHORT_URI_CA_CERTS "/crts"        ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SIMPLE_ENROLL "/sen"    ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SIMPLE_REENROLL "/sren" ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_CSR_ATTRS "/att"        ///< Specified in draft-ietf-ace-coap-est-12
#define OT_EST_COAPS_SHORT_URI_SERVER_KEY_GEN "/skg"   ///< Specified in draft-ietf-ace-coap-est-12

typedef enum otEstType
{
    OT_EST_TYPE_SIMPLE_ENROLL,
    OT_EST_TYPE_SIMPLE_REENROLL,
    OT_EST_TYPE_CA_CERTS,
    OT_EST_TYPE_SERVER_SIDE_KEY,
    OT_EST_TYPE_CSR_ATTR,
    OT_EST_TYPE_INVALID_CERT,
    OT_EST_TYPE_INVALID_KEY
} otEstType;

/**
 * This function pointer is called when the CoAPS connection state changes.
 *
 * @param[in]  aConnected  true, if a connection was established, false otherwise.
 * @param[in]  aContext    A pointer to arbitrary context information.
 *
 */
typedef void (*otHandleEstClientConnect)(bool aConnected, void *aContext);

/**
 *
 */
typedef void (*otHandleEstClientResponse)(otError aError, otEstType aType, uint8_t *aPayload, uint32_t aPayloadLength, void *aContext);

/**
 * This function starts the EST client service.
 *
 * @param[in]  aInstance                A pointer to an OpenThread instance.
 * @param[in]  aUseCoapsAsTrasportLayer If true, CoAP Secure is used for the trasport layer,
 *                                      else the raw data for EST is returned by callback.
 *
 * @retval OT_ERROR_NONE  Successfully started the EST client.
 *
 */
otError otEstClientStart(otInstance *aInstance, bool aVerifyPeer);

/**
 * This function stops the EST client.
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
 * certificate of the EST Server.
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
 * This method initializes CoAP Secure session with a EST server.
 *
 * @param[in]  aInstance               A pointer to an OpenThread instance.
 * @param[in]  aSockAddr               A pointer to the remote sockaddr.
 * @param[in]  aHandler                A pointer to a function that will be called when the DTLS connection
 *                                     state changes.
 * @param[in]  aContext                A pointer to arbitrary context information.
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
 * This method stops the EST client connection.
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
 * @retval TRUE   EST client is connected.
 * @retval FALSE  EST client is not connected.
 *
 */
bool otEstClientIsConnected(otInstance *aInstance);

/**
 * This method process a simple enrollment over CoAP Secure.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aResponseHandler     A function pointer that shall be called on response reception or time-out.
 * @param[in]  otEstRawDataHandler  A function pointer that shall be called, if not the CoAP Secure is used for
 * transportation.
 * @param[in]  aContext             A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE           Successfully sent request.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
 * @retval OT_ERROR_INVALID_STATE  EST client not connected.
 *
 */
otError otEstClientSimpleEnroll(otInstance *aInstance);

/**
 * This method process a simple re-enrollment over CoAP Secure.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aResponseHandler     A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext             A pointer to arbitrary context information.
 *
 * @retval OT_ERROR_NONE           Successfully sent request.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate retransmission data.
 * @retval OT_ERROR_INVALID_STATE  EST client not connected.
 *
 */
otError otEstClientSimpleReEnroll(otInstance *aInstance);

/**
 *
 */
otError otEstClientGetCsrAttributes(otInstance *aInstance);

/**
 *
 */
otError otEstClientGetServerGeneratedKeys(otInstance *aInstance);

/**
 *
 */
otError otEstClientGetCaCertificates(otInstance *aInstance);

/**
 *
 */
otError otEstClientGenerateKeyPair(otInstance *aInstance, const uint8_t* aPersonalSeed, uint32_t *aPersonalSeedLength);



/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OPENTHREAD_EST_CLIENT_H_ */
