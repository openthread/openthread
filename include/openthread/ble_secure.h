/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *  This file defines the top-level functions for the OpenThread BLE Secure implementation.
 *
 *  @note
 *   The functions in this module require the build-time feature `OPENTHREAD_CONFIG_BLE_TCAT_ENABLE=1`.
 *
 *  @note
 *   To enable cipher suite DTLS_PSK_WITH_AES_128_CCM_8, MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
 *    must be enabled in mbedtls-config.h
 *   To enable cipher suite DTLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
 *    MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED must be enabled in mbedtls-config.h.
 */

#ifndef OPENTHREAD_BLE_SECURE_H_
#define OPENTHREAD_BLE_SECURE_H_

#include <stdint.h>
#include <openthread/message.h>
#include <openthread/tcat.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-ble-secure
 *
 * @brief
 *   This module includes functions that control BLE Secure (TLS over BLE) communication.
 *
 *   The functions in this module are available when BLE Secure API feature
 *   (`OPENTHREAD_CONFIG_BLE_TCAT_ENABLE`) is enabled.
 *
 * @{
 */

/**
 * Pointer to call when ble secure connection state changes.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aConnected           TRUE, if a secure connection was established, FALSE otherwise.
 * @param[in]  aBleConnectionOpen   TRUE if a BLE connection was established to carry a TLS data stream, FALSE
 *                                  otherwise.
 * @param[in]  aContext             A pointer to arbitrary context information.
 */
typedef void (*otHandleBleSecureConnect)(otInstance *aInstance,
                                         bool        aConnected,
                                         bool        aBleConnectionOpen,
                                         void       *aContext);

/**
 * Pointer to call when data was received over a BLE Secure TLS connection.
 */
typedef otHandleTcatApplicationDataReceive otHandleBleSecureReceive;

/**
 * Starts the BLE Secure service.
 * When TLV mode is active, the function @p aReceiveHandler will be called once a complete TLV was received and the
 * message offset points to the TLV value.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aConnectHandler  A pointer to a function that will be called when the connection
 *                              state changes.
 * @param[in]  aReceiveHandler  A pointer to a function that will be called once data has been received
 *                              over the TLS connection.
 * @param[in]  aTlvMode         A boolean value indicating if line mode shall be activated.
 * @param[in]  aContext         A pointer to arbitrary context information. May be NULL if not used.
 *
 * @retval OT_ERROR_NONE        Successfully started the BLE Secure server.
 * @retval OT_ERROR_ALREADY     The service was stated already.
 */
otError otBleSecureStart(otInstance              *aInstance,
                         otHandleBleSecureConnect aConnectHandler,
                         otHandleBleSecureReceive aReceiveHandler,
                         bool                     aTlvMode,
                         void                    *aContext);

/**
 * Sets TCAT vendor info
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aVendorInfo       A pointer to the Vendor Information (must remain valid after the method call.
 *
 * @retval OT_ERROR_NONE         Successfully set value.
 * @retval OT_ERROR_INVALID_ARGS Value not set.
 */
otError otBleSecureSetTcatVendorInfo(otInstance *aInstance, const otTcatVendorInfo *aVendorInfo);

/**
 * Enables the TCAT protocol over BLE Secure.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aHandler          A pointer to a function that is called when the join operation completes.
 *
 * @retval OT_ERROR_NONE              Successfully started the BLE Secure Joiner role.
 * @retval OT_ERROR_INVALID_ARGS      @p aElevationPsk or @p aVendorInfo is invalid.
 * @retval OT_ERROR_INVALID_STATE     The BLE function has not been started or line mode is not selected.
 */
otError otBleSecureTcatStart(otInstance *aInstance, otHandleTcatJoin aHandler);

/**
 * Stops the BLE Secure server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otBleSecureStop(otInstance *aInstance);

/**
 * Sets the Pre-Shared Key (PSK) and cipher suite
 * TLS_PSK_WITH_AES_128_CCM_8.
 *
 * @note Requires the build-time feature `MBEDTLS_KEY_EXCHANGE_PSK_ENABLED` to be enabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPsk          A pointer to the PSK.
 * @param[in]  aPskLength    The PSK length.
 * @param[in]  aPskIdentity  The Identity Name for the PSK.
 * @param[in]  aPskIdLength  The PSK Identity Length.
 */
void otBleSecureSetPsk(otInstance    *aInstance,
                       const uint8_t *aPsk,
                       uint16_t       aPskLength,
                       const uint8_t *aPskIdentity,
                       uint16_t       aPskIdLength);

/**
 * Returns the peer x509 certificate base64 encoded.
 *
 * @note Requires the build-time features `MBEDTLS_BASE64_C` and
 *       `MBEDTLS_SSL_KEEP_PEER_CERTIFICATE` to be enabled.
 *
 * @param[in]       aInstance        A pointer to an OpenThread instance.
 * @param[out]      aPeerCert        A pointer to the base64 encoded certificate buffer.
 * @param[in,out]   aCertLength      On input, the size the max size of @p aPeerCert.
 *                                   On output, the length of the base64 encoded peer certificate.
 *
 * @retval OT_ERROR_NONE            Successfully get the peer certificate.
 * @retval OT_ERROR_INVALID_ARGS    @p aInstance or @p aCertLength is invalid.
 * @retval OT_ERROR_INVALID_STATE   Not connected yet.
 * @retval OT_ERROR_NO_BUFS         Can't allocate memory for certificate.
 */
otError otBleSecureGetPeerCertificateBase64(otInstance *aInstance, unsigned char *aPeerCert, size_t *aCertLength);

/**
 * Returns the DER encoded peer x509 certificate.
 *
 * @note Requires the build-time feature `MBEDTLS_SSL_KEEP_PEER_CERTIFICATE` to
 * be enabled.
 *
 * @param[in]       aInstance        A pointer to an OpenThread instance.
 * @param[out]      aPeerCert        A pointer to the DER encoded certificate
 *                                   buffer.
 * @param[in,out]   aCertLength      On input, the size the max size of @p
 *                                   aPeerCert. On output, the length of the
 *                                   DER encoded peer certificate.
 *
 * @retval OT_ERROR_NONE            Successfully get the peer certificate.
 * @retval OT_ERROR_INVALID_ARGS    @p aInstance or @p aCertLength is invalid.
 * @retval OT_ERROR_INVALID_STATE   Not connected yet.
 * @retval OT_ERROR_NO_BUFS         Can't allocate memory for certificate.
 */
otError otBleSecureGetPeerCertificateDer(otInstance *aInstance, unsigned char *aPeerCert, size_t *aCertLength);

/**
 * Returns an attribute value identified by its OID from the subject
 * of the peer x509 certificate. The peer OID is provided in binary format.
 * The attribute length is set if the attribute was successfully read or zero
 * if unsuccessful. The ASN.1 type as is set as defineded in the ITU-T X.690 standard
 * if the attribute was successfully read.
 *
 * @note Requires the build-time feature
 *       `MBEDTLS_SSL_KEEP_PEER_CERTIFICATE` to be enabled.
 *
 * @param[in]     aInstance           A pointer to an OpenThread instance.
 * @param[in]     aOid                A pointer to the OID to be found.
 * @param[in]     aOidLength          The length of the OID.
 * @param[out]    aAttributeBuffer    A pointer to the attribute buffer.
 * @param[in,out] aAttributeLength    On input, the size the max size of @p aAttributeBuffer.
 *                                    On output, the length of the attribute written to the buffer.
 * @param[out]    aAsn1Type           A pointer to the ASN.1 type of the attribute written to the buffer.
 *
 * @retval OT_ERROR_INVALID_STATE     Not connected yet.
 * @retval OT_ERROR_INVALID_ARGS      Invalid attribute length.
 * @retval OT_ERROR_NONE              Successfully read attribute.
 * @retval OT_ERROR_NO_BUFS           Insufficient memory for storing the attribute value.
 */
otError otBleSecureGetPeerSubjectAttributeByOid(otInstance *aInstance,
                                                const char *aOid,
                                                size_t      aOidLength,
                                                uint8_t    *aAttributeBuffer,
                                                size_t     *aAttributeLength,
                                                int        *aAsn1Type);

/**
 * Returns an attribute value for the OID 1.3.6.1.4.1.44970.x from the v3 extensions of
 * the peer x509 certificate, where the last digit x is set to aThreadOidDescriptor.
 * The attribute length is set if the attribute was successfully read or zero if unsuccessful.
 * Requires a connection to be active.
 *
 * @note Requires the build-time feature
 *       `MBEDTLS_SSL_KEEP_PEER_CERTIFICATE` to be enabled.
 *
 * @param[in]     aInstance             A pointer to an OpenThread instance.
 * @param[in]     aThreadOidDescriptor  The last digit of the Thread attribute OID.
 * @param[out]    aAttributeBuffer      A pointer to the attribute buffer.
 * @param[in,out] aAttributeLength      On input, the size the max size of @p aAttributeBuffer.
 *                                      On output, the length of the attribute written to the buffer.
 *
 * @retval OT_ERROR_NONE                Successfully read attribute.
 * @retval OT_ERROR_INVALID_ARGS        Invalid attribute length.
 * @retval OT_NOT_FOUND                 The requested attribute was not found.
 * @retval OT_ERROR_NO_BUFS             Insufficient memory for storing the attribute value.
 * @retval OT_ERROR_INVALID_STATE       Not connected yet.
 * @retval OT_ERROR_NOT_IMPLEMENTED     The value of aThreadOidDescriptor is >127.
 * @retval OT_ERROR_PARSE               The certificate extensions could not be parsed.
 */
otError otBleSecureGetThreadAttributeFromPeerCertificate(otInstance *aInstance,
                                                         int         aThreadOidDescriptor,
                                                         uint8_t    *aAttributeBuffer,
                                                         size_t     *aAttributeLength);

/**
 * Returns an attribute value for the OID 1.3.6.1.4.1.44970.x from the v3 extensions of
 * the own x509 certificate, where the last digit x is set to aThreadOidDescriptor.
 * The attribute length is set if the attribute was successfully read or zero if unsuccessful.
 * Requires a connection to be active.
 *
 * @param[in]     aInstance             A pointer to an OpenThread instance.
 * @param[in]     aThreadOidDescriptor  The last digit of the Thread attribute OID.
 * @param[out]    aAttributeBuffer      A pointer to the attribute buffer.
 * @param[in,out] aAttributeLength      On input, the size the max size of @p aAttributeBuffer.
 *                                      On output, the length of the attribute written to the buffer.
 *
 * @retval OT_ERROR_NONE                Successfully read attribute.
 * @retval OT_ERROR_INVALID_ARGS        Invalid attribute length.
 * @retval OT_NOT_FOUND                 The requested attribute was not found.
 * @retval OT_ERROR_NO_BUFS             Insufficient memory for storing the attribute value.
 * @retval OT_ERROR_INVALID_STATE       Not connected yet.
 * @retval OT_ERROR_NOT_IMPLEMENTED     The value of aThreadOidDescriptor is >127.
 * @retval OT_ERROR_PARSE               The certificate extensions could not be parsed.
 */
otError otBleSecureGetThreadAttributeFromOwnCertificate(otInstance *aInstance,
                                                        int         aThreadOidDescriptor,
                                                        uint8_t    *aAttributeBuffer,
                                                        size_t     *aAttributeLength);

/**
 * Sets the authentication mode for the BLE secure connection.
 *
 * Disable or enable the verification of peer certificate.
 * Must be called before start.
 *
 * @param[in]   aInstance               A pointer to an OpenThread instance.
 * @param[in]   aVerifyPeerCertificate  true, to verify the peer certificate.
 */
void otBleSecureSetSslAuthMode(otInstance *aInstance, bool aVerifyPeerCertificate);

/**
 * Sets the local device's X509 certificate with corresponding private key for
 * TLS session with TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8.
 *
 * @note Requires `MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED=1`.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aX509Cert          A pointer to the PEM formatted X509 certificate.
 * @param[in]  aX509Length        The length of certificate.
 * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
 * @param[in]  aPrivateKeyLength  The length of the private key.
 */
void otBleSecureSetCertificate(otInstance    *aInstance,
                               const uint8_t *aX509Cert,
                               uint32_t       aX509Length,
                               const uint8_t *aPrivateKey,
                               uint32_t       aPrivateKeyLength);

/**
 * Sets the trusted top level CAs. It is needed for validating the
 * certificate of the peer.
 *
 * TLS mode "ECDHE ECDSA with AES 128 CCM 8" for secure BLE.
 *
 * @note Requires `MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED=1`.
 *
 * @param[in]  aInstance                A pointer to an OpenThread instance.
 * @param[in]  aX509CaCertificateChain  A pointer to the PEM formatted X509 CA chain.
 * @param[in]  aX509CaCertChainLength   The length of chain.
 */
void otBleSecureSetCaCertificateChain(otInstance    *aInstance,
                                      const uint8_t *aX509CaCertificateChain,
                                      uint32_t       aX509CaCertChainLength);

/**
 * Initializes TLS session with a peer using an already open BLE connection.
 *
 * @param[in]  aInstance               A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully started TLS connection.
 */
otError otBleSecureConnect(otInstance *aInstance);

/**
 * Stops the BLE and TLS connection.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otBleSecureDisconnect(otInstance *aInstance);

/**
 * Indicates whether or not the TLS session is active (connected or connecting).
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE  If TLS session is active.
 * @retval FALSE If TLS session is not active.
 */
bool otBleSecureIsConnectionActive(otInstance *aInstance);

/**
 * Indicates whether or not the TLS session is connected.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE   The TLS session is connected.
 * @retval FALSE  The TLS session is not connected.
 */
bool otBleSecureIsConnected(otInstance *aInstance);

/**
 * Indicates whether or not the TCAT agent is enabled.
 *
 * @retval TRUE   The TCAT agent is enabled.
 * @retval FALSE  The TCAT agent is not enabled.
 */
bool otBleSecureIsTcatEnabled(otInstance *aInstance);

/**
 * Indicates whether or not a TCAT command class is authorized.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCommandClass  A command class to check.
 *
 * @retval TRUE   The command class is authorized.
 * @retval FALSE  The command class is not authorized.
 */
bool otBleSecureIsCommandClassAuthorized(otInstance *aInstance, otTcatCommandClass aCommandClass);

/**
 * Sends a secure BLE message.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the message to send.
 *
 * If the return value is OT_ERROR_NONE, OpenThread takes ownership of @p aMessage, and the caller should no longer
 * reference @p aMessage. If the return value is not OT_ERROR_NONE, the caller retains ownership of @p aMessage,
 * including freeing @p aMessage if the message buffer is no longer needed.
 *
 * @retval OT_ERROR_NONE           Successfully sent message.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate buffer memory.
 * @retval OT_ERROR_INVALID_STATE  TLS connection was not initialized.
 */
otError otBleSecureSendMessage(otInstance *aInstance, otMessage *aMessage);

/**
 * Sends a secure BLE data packet.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aBuf          A pointer to the data to send as the Value of the TCAT Send Application Data TLV.
 * @param[in]  aLength       A number indicating the length of the data buffer.
 *
 * @retval OT_ERROR_NONE           Successfully sent data.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate buffer memory.
 * @retval OT_ERROR_INVALID_STATE  TLS connection was not initialized.
 */
otError otBleSecureSend(otInstance *aInstance, uint8_t *aBuf, uint16_t aLength);

/**
 * Sends a secure BLE data packet containing a TCAT Send Application Data TLV.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aBuf            A pointer to the data to send as the Value of the TCAT Send Application Data TLV.
 * @param[in]  aLength         A number indicating the length of the data buffer.
 *
 * @retval OT_ERROR_NONE           Successfully sent data.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate buffer memory.
 * @retval OT_ERROR_INVALID_STATE  TLS connection was not initialized.
 */
otError otBleSecureSendApplicationTlv(otInstance *aInstance, uint8_t *aBuf, uint16_t aLength);

/**
 * Flushes the send buffer.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully flushed output buffer.
 * @retval OT_ERROR_NO_BUFS        Failed to allocate buffer memory.
 * @retval OT_ERROR_INVALID_STATE  TLS connection was not initialized.
 */
otError otBleSecureFlush(otInstance *aInstance);

/**
 * Gets the Install Code Verify Status during the current session.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @retval TRUE  The install code was correctly verified.
 * @retval FALSE The install code was not verified.
 */
bool otBleSecureGetInstallCodeVerifyStatus(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* OPENTHREAD_BLE_SECURE_H_ */
