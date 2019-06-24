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

#include "est_client.hpp"

#include <mbedtls/asn1.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_csr.h>

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

#include "openthread/entropy.h"
#include "openthread/random_crypto.h"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include "../common/asn1.hpp"
#include "common/entropy.hpp"
#include "common/random.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/mbedtls.hpp"
#include "crypto/sha256.hpp"

/**
 * @file
 *   This file implements the EST client.
 */

namespace ot {
namespace Est {

#define CSR_RANDOM_THRESHOLD    32
#define CSR_SEED_LENGTH         8
#define CSR_BUFFER_SIZE         1024



#define EST_ASN1_OID_PKCS7_DATA        MBEDTLS_OID_PKCS "\x07" "\x01"   //[RFC3369]
#define EST_ASN1_OID_PKCS7_SIGNEDATA   MBEDTLS_OID_PKCS "\x07" "\x02"   //[RFC3369]

Client::Client(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsConnected(false)
    , mStarted(false)
    , mVerifyEstServerCertificate(false)
    , mApplicationContext(NULL)
    , mConnectCallback(NULL)
    , mResponseCallback(NULL)
    , mCoapSecure(aInstance, true)
{
    OT_UNUSED_VARIABLE(mIsConnected);
    OT_UNUSED_VARIABLE(mStarted);
    OT_UNUSED_VARIABLE(mVerifyEstServerCertificate);
    OT_UNUSED_VARIABLE(mApplicationContext);
    OT_UNUSED_VARIABLE(mConnectCallback);
    OT_UNUSED_VARIABLE(mResponseCallback);
}

otError Client::Start(bool aVerifyPeer)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mStarted == false, error = OT_ERROR_ALREADY);

    mStarted                    = true;
    mVerifyEstServerCertificate = aVerifyPeer;

    error = mCoapSecure.Start(kLocalPort);
    VerifyOrExit(error);

exit:

    return error;
}

void Client::Stop(void)
{
    mCoapSecure.Stop();
    mStarted = false;
}

otError Client::SetCertificate(const uint8_t *aX509Cert,
                               uint32_t       aX509Length,
                               const uint8_t *aPrivateKey,
                               uint32_t       aPrivateKeyLength)
{
    return mCoapSecure.SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
    ;
}

otError Client::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    return mCoapSecure.SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}

otError Client::Connect(const Ip6::SockAddr &     aSockAddr,
                        otHandleEstClientConnect  aConnectHandler,
                        otHandleEstClientResponse aResponseHandler,
                        void *                    aContext)
{
    mApplicationContext = aContext;
    mConnectCallback    = aConnectHandler;
    mResponseCallback   = aResponseHandler;
    mCoapSecure.Connect(aSockAddr, &Client::CoapSecureConnectedHandle, this);

    return OT_ERROR_NONE;
}

void Client::Disconnect(void)
{
    mCoapSecure.Disconnect();
}

bool Client::IsConnected(void)
{
    return mIsConnected;
}

otError Client::SimpleEnroll(const uint8_t *aPrivateKey,
                             uint32_t       aPrivateLeyLength,
                             const uint8_t *aPublicKey,
                             uint32_t       aPublicKeyLength,
                             otMdType       aMdType,
                             uint8_t        aKeyUsageFlags,
                             bool           aPemFormat)
{
    otError error = OT_ERROR_NONE;
    mbedtls_entropy_context *entropyCtx;
    mbedtls_ctr_drbg_context *ctrDrbgCtx;
    mbedtls_x509write_csr csr;
    mbedtls_pk_context pkCtx;
    uint8_t seed[CSR_SEED_LENGTH];
    uint16_t seedLength = CSR_SEED_LENGTH;
    uint8_t nsCertType = 0;
    uint8_t buffer[CSR_BUFFER_SIZE] = {0};
    size_t bufferLength = CSR_BUFFER_SIZE;
    uint8_t *bufferPointer = NULL;
    Coap::Message *mCoapMessage = NULL;

    entropyCtx = otEntropyMbedTlsContextGet();
    ctrDrbgCtx = otRandomCryptoMbedTlsContextGet();
    mbedtls_x509write_csr_init(&csr);
    mbedtls_pk_init(&pkCtx);

    VerifyOrExit(mIsConnected, error = OT_ERROR_INVALID_STATE);

    // Parse key pair
    VerifyOrExit(mbedtls_pk_parse_key(&pkCtx, aPrivateKey, aPrivateLeyLength, NULL, 0) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mbedtls_pk_parse_public_key(&pkCtx, aPublicKey, aPublicKeyLength) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    // Setup entropy
    VerifyOrExit(mbedtls_entropy_add_source(entropyCtx, &EntropyPollHandle,
                                            NULL, CSR_RANDOM_THRESHOLD,
                                            MBEDTLS_ENTROPY_SOURCE_STRONG) == 0,
                 error = OT_ERROR_FAILED);

    // Setup CTR_DRBG context
    mbedtls_ctr_drbg_set_prediction_resistance(ctrDrbgCtx, MBEDTLS_CTR_DRBG_PR_ON);

    VerifyOrExit(otRandomCryptoFillBuffer(seed, seedLength) == OT_ERROR_NONE,
                 error = OT_ERROR_FAILED);

    VerifyOrExit(mbedtls_ctr_drbg_seed(ctrDrbgCtx, mbedtls_entropy_func, entropyCtx,
                                       seed, seedLength) == 0,
                 error = OT_ERROR_FAILED);

    // Create PKCS#10
    mbedtls_x509write_csr_set_md_alg(&csr, (mbedtls_md_type_t)aMdType);

    VerifyOrExit(mbedtls_x509write_csr_set_key_usage(&csr, aKeyUsageFlags) == 0,
                 error = OT_ERROR_INVALID_ARGS);

    nsCertType |= MBEDTLS_X509_NS_CERT_TYPE_SSL_CLIENT;
    VerifyOrExit(mbedtls_x509write_csr_set_ns_cert_type(&csr, nsCertType) == 0,
                 error = OT_ERROR_FAILED);

    mbedtls_x509write_csr_set_key(&csr, &pkCtx);

    if(aPemFormat)
    {
        // Write CSR in PEM format
        VerifyOrExit((bufferLength = mbedtls_x509write_csr_pem(&csr, buffer, bufferLength,
                                                               mbedtls_ctr_drbg_random, ctrDrbgCtx)) > 0,
                     error = OT_ERROR_NO_BUFS);

        bufferPointer = buffer;
    }
    else
    {
        // Write CSR in DER format
        VerifyOrExit((bufferLength = mbedtls_x509write_csr_der(&csr, buffer, bufferLength,
                                                               mbedtls_ctr_drbg_random, ctrDrbgCtx)) > 0,
                     error = OT_ERROR_NO_BUFS);

        bufferPointer = buffer + (CSR_BUFFER_SIZE - bufferLength);
    }

    // Send CSR
    VerifyOrExit((mCoapMessage = mCoapSecure.NewMessage(NULL)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = mCoapMessage->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST,
                                             OT_EST_COAPS_SHORT_URI_SIMPLE_ENROLL));

    SuccessOrExit(error = mCoapMessage->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_PKCS10));

    SuccessOrExit(error = mCoapMessage->SetPayloadMarker());

    SuccessOrExit(error = mCoapMessage->Append(bufferPointer, bufferLength));

    mCoapSecure.SendMessage(*mCoapMessage, &Client::SimpleEnrollResponseHandler, this);

exit:
    mbedtls_x509write_csr_free(&csr);
    mbedtls_pk_free(&pkCtx);

    return error;
}

otError Client::SimpleReEnroll(const uint8_t *aPrivateKey,
                               uint32_t       aPrivateLeyLength,
                               const uint8_t *aPublicKey,
                               uint32_t       aPublicKeyLength,
                               otMdType       aMdType,
                               uint8_t        aKeyUsageFlags,
                               bool           aPemFormat)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

    OT_UNUSED_VARIABLE(aPrivateKey);
    OT_UNUSED_VARIABLE(aPrivateLeyLength);
    OT_UNUSED_VARIABLE(aPublicKey);
    OT_UNUSED_VARIABLE(aPublicKeyLength);
    OT_UNUSED_VARIABLE(aMdType);
    OT_UNUSED_VARIABLE(aKeyUsageFlags);
    OT_UNUSED_VARIABLE(aPemFormat);

exit:

    return mError;
}

otError Client::GetCsrAttributes(void)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

exit:

    return mError;
}

otError Client::GetServerGeneratedKeys(void)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

exit:

    return mError;
}

otError Client::GetCaCertificates(void)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

exit:

    return mError;
}

void Client::CoapSecureConnectedHandle(bool aConnected, void *aContext)
{
    return static_cast<Client *>(aContext)->CoapSecureConnectedHandle(aConnected);
}

void Client::CoapSecureConnectedHandle(bool aConnected)
{
    mIsConnected = aConnected;

    if (mConnectCallback != NULL)
    {
        mConnectCallback(aConnected, mApplicationContext);
    }
}

otError Client::CmsReadSignedData(uint8_t       *aMessage,
                                  uint32_t       aMessageLength,
                                  uint8_t      **aPayload,
                                  uint32_t      *aPayloadLength)
{
    otError mError = OT_ERROR_NONE;
    uint8_t *mMessagePointer = NULL;
    uint8_t *mMessageEnd = NULL;
    size_t mSequenceLength = 0;

    mMessagePointer = aMessage;
    mMessageEnd = aMessage + aMessageLength;

    VerifyOrExit(mbedtls_asn1_get_tag(&mMessagePointer, mMessageEnd,
                                      &mSequenceLength, MBEDTLS_ASN1_OID) == 0,
                 mError = OT_ERROR_SECURITY);

    VerifyOrExit(memcmp(mMessagePointer,
                        EST_ASN1_OID_PKCS7_SIGNEDATA,
                        mSequenceLength) == 0,
                 mError = OT_ERROR_SECURITY);

    VerifyOrExit(mbedtls_asn1_get_tag(&mMessagePointer, mMessageEnd,
                                      &mSequenceLength, MBEDTLS_ASN1_OID) == 0,
                 mError = OT_ERROR_SECURITY);

    VerifyOrExit(memcmp(mMessagePointer,
                        EST_ASN1_OID_PKCS7_DATA,
                        mSequenceLength) == 0,
                 mError = OT_ERROR_SECURITY);

    VerifyOrExit(mbedtls_asn1_get_tag(&mMessagePointer, mMessageEnd,
                                      &mSequenceLength, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC) == 0,
                 mError = OT_ERROR_SECURITY);

    *aPayload = mMessagePointer;
    *aPayloadLength = mSequenceLength;

exit:
    return mError;
}

void Client::SimpleEnrollResponseHandler(void *               aContext,
                                         otMessage *          aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aResult)
{
    return static_cast<Client *>(aContext)->SimpleEnrollResponseHandler(aMessage, aMessageInfo, aResult);
}

void Client::SimpleEnrollResponseHandler(otMessage *          aMessage,
                                         const otMessageInfo *aMessageInfo,
                                         otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otCoapCode mCoapCode = otCoapMessageGetCode(aMessage);
    uint8_t mMessage[CSR_BUFFER_SIZE + 1] = {0};
    uint32_t mMessageLength = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    uint8_t *mPayload = NULL;
    uint32_t mPayloadLength = 0;

    VerifyOrExit(aResult == OT_ERROR_NONE, mMessageLength = 0);

    switch(mCoapCode)
    {
        case OT_COAP_CODE_CREATED:
            // Check if message is too long for buffer
            VerifyOrExit(mMessageLength <= sizeof(mMessage), aResult = OT_ERROR_NO_BUFS;);

            // Parse message
            mMessage[mMessageLength] = '\0';
            otMessageRead(aMessage, otMessageGetOffset(aMessage), mMessage, mMessageLength);

            aResult = Client::CmsReadSignedData(mMessage, mMessageLength, &mPayload, &mPayloadLength);
            break;

        default:
            aResult = OT_ERROR_FAILED;
            mMessageLength = 0;
            break;
    }

exit:
    mResponseCallback(aResult, OT_EST_TYPE_SIMPLE_ENROLL, mPayload, mPayloadLength, mApplicationContext);
}

void Client::SimpleReEnrollResponseHandler(void *               aContext,
                                           otMessage *          aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aResult)
{
    return static_cast<Client *>(aContext)->SimpleReEnrollResponseHandler(aMessage, aMessageInfo, aResult);
}

void Client::SimpleReEnrollResponseHandler(otMessage *          aMessage,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otCoapCode mCoapCode = otCoapMessageGetCode(aMessage);
    uint8_t mMessage[CSR_BUFFER_SIZE + 1] = {0};
    uint32_t mMessageLength = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    uint8_t *mPayload = NULL;
    uint32_t mPayloadLength = 0;

    VerifyOrExit(aResult == OT_ERROR_NONE, mMessageLength = 0);

    switch(mCoapCode)
    {
        case OT_COAP_CODE_CREATED:
            // Check if message is too long for buffer
            VerifyOrExit(mMessageLength <= sizeof(mMessage), aResult = OT_ERROR_NO_BUFS;);

            // Parse message
            mMessage[mMessageLength] = '\0';
            otMessageRead(aMessage, otMessageGetOffset(aMessage), mMessage, mMessageLength);

            aResult = Client::CmsReadSignedData(mMessage, mMessageLength, &mPayload, &mPayloadLength);
            break;

        default:
            aResult = OT_ERROR_FAILED;
            mMessageLength = 0;
            break;
    }

exit:
    mResponseCallback(aResult, OT_EST_TYPE_SIMPLE_REENROLL, mPayload, mPayloadLength, mApplicationContext);
}

int Client::EntropyPollHandle(void *         aData,
                              unsigned char *aOutput,
                              size_t         aInLen,
                              size_t *       aOutLen)
{
    int error = 0;
    OT_UNUSED_VARIABLE(aData);

    VerifyOrExit(otRandomCryptoFillBuffer((uint8_t*)aOutput, (uint16_t)aInLen) == OT_ERROR_NONE,
                 error = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);

    if(aOutLen != NULL)
    {
        *aOutLen = aInLen;
    }

exit:

    return error;
}

} // namespace Est
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
