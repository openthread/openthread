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

/**
 * @file
 *   This file includes definitions for using mbedTLS.
 */

#ifndef DTLS_HPP_
#define DTLS_HPP_

#include "openthread-core-config.h"

#include <mbedtls/certs.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#include <mbedtls/base64.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "crypto/sha256.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

namespace MeshCoP {

class Dtls : public InstanceLocator
{
public:
    enum
    {
        kPskMaxLength                = 32,
        kGuardTimeNewConnectionMilli = 2000,
#if !OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
        kApplicationDataMaxLength = 512,
#else
        kApplicationDataMaxLength = OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH,
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    };

    /**
     * This constructor initializes the DTLS object.
     *
     * @param[in]  aNetif  A reference to the Thread network interface.
     *
     */
    explicit Dtls(Instance &aInstance);

    /**
     * This function pointer is called when a connection is established or torn down.
     *
     * @param[in]  aContext    A pointer to application-specific context.
     * @param[in]  aConnected  TRUE if a connection was established, FALSE otherwise.
     *
     */
    typedef void (*ConnectedHandler)(void *aContext, bool aConnected);

    /**
     * This function pointer is called when data is received from the DTLS session.
     *
     * @param[in]  aContext  A pointer to application-specific context.
     * @param[in]  aBuf      A pointer to the received data buffer.
     * @param[in]  aLength   Number of bytes in the received data buffer.
     *
     */
    typedef void (*ReceiveHandler)(void *aContext, uint8_t *aBuf, uint16_t aLength);

    /**
     * This function pointer is called when data is ready to transmit for the DTLS session.
     *
     * @param[in]  aContext         A pointer to application-specific context.
     * @param[in]  aBuf             A pointer to the transmit data buffer.
     * @param[in]  aLength          Number of bytes in the transmit data buffer.
     * @param[in]  aMessageSubtype  A message sub type information for the sender.
     *
     */
    typedef otError (*SendHandler)(void *aContext, const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType);

    /**
     * This method starts the DTLS service.
     *
     * For CoAP Secure API do first:
     * Set X509 Pk and Cert for use DTLS mode ECDHE ECDSA with AES 128 CCM 8 or
     * set PreShared Key for use DTLS mode PSK with AES 128 CCM 8.
     *
     * @param[in]  aClient                 TRUE if operating as a client, FALSE if operating as a server.
     * @param[in]  aConnectedHandler       A pointer to the connected handler.
     * @param[in]  aReceiveHandler         A pointer to the receive handler.
     * @param[in]  aSendHandler            A pointer to the send handler.
     * @param[in]  aContext                A pointer to application-specific context.
     *
     * @retval OT_ERROR_NONE      Successfully started the DTLS service.
     *
     */
    otError Start(bool             aClient,
                  ConnectedHandler aConnectedHandler,
                  ReceiveHandler   aReceiveHandler,
                  SendHandler      aSendHandler,
                  void *           aContext);

    /**
     * This method stops the DTLS service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the DTLS service.
     *
     */
    otError Stop(void);

    /**
     * This method indicates whether or not the DTLS service is active.
     *
     * @returns true if the DTLS service is active, false otherwise.
     *
     */
    bool IsStarted(void);

    /**
     * This method sets the PSK.
     *
     * @param[in]  aPSK  A pointer to the PSK.
     *
     * @retval OT_ERROR_NONE          Successfully set the PSK.
     * @retval OT_ERROR_INVALID_ARGS  The PSK is invalid.
     *
     */
    otError SetPsk(const uint8_t *aPsk, uint8_t aPskLength);

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    /**
     * This method sets the Pre-Shared Key (PSK) for DTLS sessions-
     * identified by a PSK.
     *
     * DTLS mode "PSK with AES 128 CCM 8" for Application CoAPS.
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
     * This method sets a reference to the own x509 certificate with corresponding private key.
     *
     * DTLS mode "ECDHE ECDSA with AES 128 CCM 8" for Application CoAPS.
     *
     * @param[in]  aX509Certificate   A pointer to the PEM formatted X509 certificate.
     * @param[in]  aX509CertLength    The length of certificate.
     * @param[in]  aPrivateKey        A pointer to the PEM formatted private key.
     * @param[in]  aPrivateKeyLength  The length of the private key.
     *
     * @retval OT_ERROR_NONE  Successfully set the x509 certificate with his private key.
     *
     */
    otError SetCertificate(const uint8_t *aX509Certificate,
                           uint32_t       aX509CertLength,
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
     * This method set the authentication mode for a dtls connection.
     *
     * Disable or enable the verification of peer certificate.
     * Must called before start.
     *
     * @param[in]  aVerifyPeerCertificate  true, if the peer certificate should verify.
     *
     */
    void SetSslAuthMode(bool aVerifyPeerCertificate);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER
    /**
     * This method sets the Client ID used for generating the Hello Cookie.
     *
     * @param[in]  aClientId  A pointer to the Client ID.
     * @param[in]  aLength    Number of bytes in the Client ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the Client ID.
     *
     */
    otError SetClientId(const uint8_t *aClientId, uint8_t aLength);
#endif // OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER

    /**
     * This method indicates whether or not the DTLS session is connected.
     *
     * @retval TRUE   The DTLS session is connected.
     * @retval FALSE  The DTLS session is not connected.
     *
     */
    bool IsConnected(void);

    /**
     * This method sends data within the DTLS session.
     *
     * @param[in]  aMessage  A message to send via DTLS.
     * @param[in]  aLength   Number of bytes in the data buffer.
     *
     * @retval OT_ERROR_NONE     Successfully sent the data via the DTLS session.
     * @retval OT_ERROR_NO_BUFS  A message is too long.
     *
     */
    otError Send(Message &aMessage, uint16_t aLength);

    /**
     * This method provides a received DTLS message to the DTLS object.
     *
     * @param[in]  aMessage  A reference to the message.
     * @param[in]  aOffset   The offset within @p aMessage where the DTLS message starts.
     * @param[in]  aLength   The size of the DTLS message (bytes).
     *
     * @retval OT_ERROR_NONE  Successfully processed the received DTLS message.
     *
     */
    otError Receive(Message &aMessage, uint16_t aOffset, uint16_t aLength);

    /**
     * This method sets the default message sub-type that will be used for all messages without defined
     * sub-type.
     *
     * @param[in]  aMessageSubType  The default message sub-type.
     *
     */
    void SetDefaultMessageSubType(uint8_t aMessageSubType) { mMessageDefaultSubType = aMessageSubType; }

    /**
     * The provisioning URL is placed here so that both the Commissioner and Joiner can share the same object.
     *
     */
    ProvisioningUrlTlv mProvisioningUrl;

private:
    void FreeMbedtls(void);

    static otError MapError(int rval);

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    /**
     * Set keys and/or certificates for dtls session dependent of used cipher suite.
     *
     * @retval mbedtls error, 0 if successfully.
     *
     */
    int SetApplicationCoapSecureKeys(void);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    static void HandleMbedtlsDebug(void *ctx, int level, const char *file, int line, const char *str);

    static int HandleMbedtlsGetTimer(void *aContext);
    int        HandleMbedtlsGetTimer(void);

    static void HandleMbedtlsSetTimer(void *aContext, uint32_t aIntermediate, uint32_t aFinish);
    void        HandleMbedtlsSetTimer(uint32_t aIntermediate, uint32_t aFinish);

    static int HandleMbedtlsReceive(void *aContext, unsigned char *aBuf, size_t aLength);
    int        HandleMbedtlsReceive(unsigned char *aBuf, size_t aLength);

    static int HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength);
    int        HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength);

    static int HandleMbedtlsExportKeys(void *               aContext,
                                       const unsigned char *aMasterSecret,
                                       const unsigned char *aKeyBlock,
                                       size_t               aMacLength,
                                       size_t               aKeyLength,
                                       size_t               aIvLength);
    int        HandleMbedtlsExportKeys(const unsigned char *aMasterSecret,
                                       const unsigned char *aKeyBlock,
                                       size_t               aMacLength,
                                       size_t               aKeyLength,
                                       size_t               aIvLength);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static int HandleMbedtlsEntropyPoll(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen);

    void Close(void);
    void Process(void);

    int     mCipherSuites[2];
    uint8_t mPsk[kPskMaxLength];
    uint8_t mPskLength;

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

    const uint8_t *    mCaChainSrc;
    uint32_t           mCaChainLength;
    const uint8_t *    mOwnCertSrc;
    uint32_t           mOwnCertLength;
    const uint8_t *    mPrivateKeySrc;
    uint32_t           mPrivateKeyLength;
    mbedtls_x509_crt   mCaChain;
    mbedtls_x509_crt   mOwnCert;
    mbedtls_pk_context mPrivateKey;
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    const uint8_t *mPreSharedKey;
    const uint8_t *mPreSharedKeyIdentity;
    uint16_t       mPreSharedKeyLength;
    uint16_t       mPreSharedKeyIdLength;
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    bool mVerifyPeerCertificate;

    mbedtls_entropy_context  mEntropy;
    mbedtls_ctr_drbg_context mCtrDrbg;
    mbedtls_ssl_context      mSsl;
    mbedtls_ssl_config       mConf;

#ifdef MBEDTLS_SSL_COOKIE_C
    mbedtls_ssl_cookie_ctx mCookieCtx;
#endif

    bool mStarted;

    TimerMilli mTimer;
    uint32_t   mTimerIntermediate;
    bool       mTimerSet;

    Message *mReceiveMessage;
    uint16_t mReceiveOffset;
    uint16_t mReceiveLength;

    ConnectedHandler mConnectedHandler;
    ReceiveHandler   mReceiveHandler;
    SendHandler      mSendHandler;
    void *           mContext;
    bool             mGuardTimerSet;

    uint8_t mMessageSubType;
    uint8_t mMessageDefaultSubType;
};

} // namespace MeshCoP
} // namespace ot

#endif // DTLS_HPP_
