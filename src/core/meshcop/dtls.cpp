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
 *   This file implements the necessary hooks for mbedTLS.
 */

#define WPP_NAME "dtls.tmh"

#include "dtls.hpp"

#include <mbedtls/debug.h>
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/timer.hpp"
#include "crypto/sha256.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DTLS

namespace ot {
namespace MeshCoP {

Dtls::Dtls(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateStopped)
    , mPskLength(0)
    , mVerifyPeerCertificate(true)
    , mTimer(aInstance, &Dtls::HandleTimer, this)
    , mTimerIntermediate(0)
    , mTimerSet(false)
    , mReceiveMessage(NULL)
    , mReceiveOffset(0)
    , mReceiveLength(0)
    , mConnectedHandler(NULL)
    , mReceiveHandler(NULL)
    , mSendHandler(NULL)
    , mContext(NULL)
    , mMessageSubType(Message::kSubTypeNone)
    , mMessageDefaultSubType(Message::kSubTypeNone)
{
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    mPreSharedKey         = NULL;
    mPreSharedKeyIdentity = NULL;
    mPreSharedKeyIdLength = 0;
    mPreSharedKeyLength   = 0;
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mCaChainSrc       = NULL;
    mCaChainLength    = 0;
    mOwnCertSrc       = NULL;
    mOwnCertLength    = 0;
    mPrivateKeySrc    = NULL;
    mPrivateKeyLength = 0;
    memset(&mCaChain, 0, sizeof(mCaChain));
    memset(&mOwnCert, 0, sizeof(mOwnCert));
    memset(&mPrivateKey, 0, sizeof(mPrivateKey));
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    memset(mCipherSuites, 0, sizeof(mCipherSuites));
    memset(mPsk, 0, sizeof(mPsk));
    memset(&mEntropy, 0, sizeof(mEntropy));
    memset(&mCtrDrbg, 0, sizeof(mCtrDrbg));
    memset(&mSsl, 0, sizeof(mSsl));
    memset(&mConf, 0, sizeof(mConf));

#ifdef MBEDTLS_SSL_COOKIE_C
    memset(&mCookieCtx, 0, sizeof(mCookieCtx));
#endif

    mProvisioningUrl.Init();
}

int Dtls::HandleMbedtlsEntropyPoll(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen)
{
    OT_UNUSED_VARIABLE(aData);

    otError error;
    int     rval = 0;

    error = otPlatRandomGetTrue((uint8_t *)aOutput, (uint16_t)aInLen);
    SuccessOrExit(error);

    if (aOutLen != NULL)
    {
        *aOutLen = aInLen;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    return rval;
}

void Dtls::FreeMbedtls(void)
{
#ifdef MBEDTLS_SSL_COOKIE_C
    mbedtls_ssl_cookie_free(&mCookieCtx);
#endif
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mbedtls_x509_crt_free(&mCaChain);
    mbedtls_x509_crt_free(&mOwnCert);
    mbedtls_pk_free(&mPrivateKey);
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    mbedtls_entropy_free(&mEntropy);
    mbedtls_ctr_drbg_free(&mCtrDrbg);
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSsl);
}

otError Dtls::Start(bool             aClient,
                    ConnectedHandler aConnectedHandler,
                    ReceiveHandler   aReceiveHandler,
                    SendHandler      aSendHandler,
                    void *           aContext)
{
    int rval;

    // do not handle new connection before guard time expired
    VerifyOrExit(mState == kStateStopped, rval = MBEDTLS_ERR_SSL_TIMEOUT);

    mbedtls_ssl_init(&mSsl);
    mbedtls_ssl_config_init(&mConf);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_entropy_init(&mEntropy);
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mbedtls_x509_crt_init(&mCaChain);
    mbedtls_x509_crt_init(&mOwnCert);
    mbedtls_pk_init(&mPrivateKey);
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    rval = mbedtls_entropy_add_source(&mEntropy, &Dtls::HandleMbedtlsEntropyPoll, NULL, MBEDTLS_ENTROPY_MIN_PLATFORM,
                                      MBEDTLS_ENTROPY_SOURCE_STRONG);
    VerifyOrExit(rval == 0);

    {
        otExtAddress eui64;
        otPlatRadioGetIeeeEui64(&GetInstance(), eui64.m8);
        rval = mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, &mEntropy, eui64.m8, sizeof(eui64));
        VerifyOrExit(rval == 0);
    }

    rval = mbedtls_ssl_config_defaults(&mConf, aClient ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
                                       MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
    VerifyOrExit(rval == 0);

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    if (mVerifyPeerCertificate && mCipherSuites[0] == MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8)
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_NONE);
    }
#else
    OT_UNUSED_VARIABLE(mVerifyPeerCertificate);
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    mbedtls_ssl_conf_rng(&mConf, mbedtls_ctr_drbg_random, &mCtrDrbg);
    mbedtls_ssl_conf_min_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_ciphersuites(&mConf, mCipherSuites);
    mbedtls_ssl_conf_export_keys_cb(&mConf, HandleMbedtlsExportKeys, this);
    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, HandleMbedtlsDebug, this);

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    if (!aClient)
    {
        mbedtls_ssl_cookie_init(&mCookieCtx);

        rval = mbedtls_ssl_cookie_setup(&mCookieCtx, mbedtls_ctr_drbg_random, &mCtrDrbg);
        VerifyOrExit(rval == 0);

        mbedtls_ssl_conf_dtls_cookies(&mConf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &mCookieCtx);
    }
#endif // OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    rval = mbedtls_ssl_setup(&mSsl, &mConf);
    VerifyOrExit(rval == 0);

    mbedtls_ssl_set_bio(&mSsl, this, &Dtls::HandleMbedtlsTransmit, HandleMbedtlsReceive, NULL);
    mbedtls_ssl_set_timer_cb(&mSsl, this, &Dtls::HandleMbedtlsSetTimer, HandleMbedtlsGetTimer);

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        rval = mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        rval = SetApplicationCoapSecureKeys();
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    VerifyOrExit(rval == 0);

    mConnectedHandler = aConnectedHandler;
    mReceiveHandler   = aReceiveHandler;
    mSendHandler      = aSendHandler;
    mContext          = aContext;
    mReceiveMessage   = NULL;
    mMessageSubType   = Message::kSubTypeNone;
    mState            = kStateConnecting;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("DTLS started");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("Application Coap Secure DTLS started");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    Process();

exit:
    if (rval != 0)
    {
        FreeMbedtls();
    }

    return MapError(rval);
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
int Dtls::SetApplicationCoapSecureKeys(void)
{
    int rval = 0;

    VerifyOrExit(&mCipherSuites[0] != NULL, rval = MBEDTLS_ERR_SSL_BAD_INPUT_DATA);

    switch (mCipherSuites[0])
    {
    case MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8:
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        if (mCaChainSrc != NULL)
        {
            rval = mbedtls_x509_crt_parse(&mCaChain, (const unsigned char *)mCaChainSrc, (size_t)mCaChainLength);
            VerifyOrExit(rval == 0);
            mbedtls_ssl_conf_ca_chain(&mConf, &mCaChain, NULL);
        }

        if (mOwnCertSrc != NULL && mPrivateKeySrc != NULL)
        {
            rval = mbedtls_x509_crt_parse(&mOwnCert, (const unsigned char *)mOwnCertSrc, (size_t)mOwnCertLength);
            VerifyOrExit(rval == 0);
            rval = mbedtls_pk_parse_key(&mPrivateKey, (const unsigned char *)mPrivateKeySrc, (size_t)mPrivateKeyLength,
                                        NULL, 0);
            VerifyOrExit(rval == 0);
            rval = mbedtls_ssl_conf_own_cert(&mConf, &mOwnCert, &mPrivateKey);
            VerifyOrExit(rval == 0);
        }
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        break;

    case MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8:
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
        rval = mbedtls_ssl_conf_psk(&mConf, (unsigned char *)mPreSharedKey, mPreSharedKeyLength,
                                    (unsigned char *)mPreSharedKeyIdentity, mPreSharedKeyIdLength);
        VerifyOrExit(rval == 0);
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
        break;

    default:
        otLogCritCoap("Application Coap Secure DTLS: Not supported cipher.");
        rval = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
        ExitNow();
        break;
    }

exit:
    return rval;
}

void Dtls::SetSslAuthMode(bool aVerifyPeerCertificate)
{
    mVerifyPeerCertificate = aVerifyPeerCertificate;
}

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

void Dtls::Stop(void)
{
    VerifyOrExit((mState == kStateConnecting) || (mState == kStateConnected));

    mbedtls_ssl_close_notify(&mSsl);
    Close();

exit:
    return;
}

void Dtls::Close(void)
{
    assert((mState == kStateConnecting) || (mState == kStateConnected));

    mState = kStateCloseNotify;
    mTimer.Start(kGuardTimeNewConnectionMilli);

    FreeMbedtls();

    if (mConnectedHandler != NULL)
    {
        mConnectedHandler(mContext, false);
    }
}

otError Dtls::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aPskLength <= sizeof(mPsk), error = OT_ERROR_INVALID_ARGS);

    memcpy(mPsk, aPsk, aPskLength);
    mPskLength       = aPskLength;
    mCipherSuites[0] = MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8;
    mCipherSuites[1] = 0;

exit:
    return error;
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

otError Dtls::SetCertificate(const uint8_t *aX509Certificate,
                             uint32_t       aX509CertLength,
                             const uint8_t *aPrivateKey,
                             uint32_t       aPrivateKeyLength)
{
    otError error = OT_ERROR_NONE;

    assert(aX509CertLength > 0);
    assert(aX509Certificate != NULL);

    assert(aPrivateKeyLength > 0);
    assert(aPrivateKey != NULL);

    mOwnCertSrc       = aX509Certificate;
    mOwnCertLength    = aX509CertLength;
    mPrivateKeySrc    = aPrivateKey;
    mPrivateKeyLength = aPrivateKeyLength;

    mCipherSuites[0] = MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8;
    mCipherSuites[1] = 0;

    return error;
}

otError Dtls::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    otError error = OT_ERROR_NONE;

    assert(aX509CaCertChainLength > 0);
    assert(aX509CaCertificateChain != NULL);

    mCaChainSrc    = aX509CaCertificateChain;
    mCaChainLength = aX509CaCertChainLength;

    return error;
}

#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

otError Dtls::SetPreSharedKey(const uint8_t *aPsk,
                              uint16_t       aPskLength,
                              const uint8_t *aPskIdentity,
                              uint16_t       aPskIdLength)
{
    otError error = OT_ERROR_NONE;

    assert(aPsk != NULL);
    assert(aPskIdentity != NULL);
    assert(aPskLength > 0);
    assert(aPskIdLength > 0);

    mPreSharedKey         = aPsk;
    mPreSharedKeyLength   = aPskLength;
    mPreSharedKeyIdentity = aPskIdentity;
    mPreSharedKeyIdLength = aPskIdLength;

    mCipherSuites[0] = MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8;
    mCipherSuites[1] = 0;

    return error;
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_BASE64_C

otError Dtls::GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kStateConnected, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(mbedtls_base64_encode(aPeerCert, aCertBufferSize, aCertLength, mSsl.session->peer_cert->raw.p,
                                       mSsl.session->peer_cert->raw.len) == 0,
                 error = OT_ERROR_NO_BUFS);

exit:
    return error;
}

#endif // MBEDTLS_BASE64_C
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER
otError Dtls::SetClientId(const uint8_t *aClientId, uint8_t aLength)
{
    int rval = mbedtls_ssl_set_client_transport_id(&mSsl, aClientId, aLength);
    return MapError(rval);
}
#endif // OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER

otError Dtls::Send(Message &aMessage, uint16_t aLength)
{
    otError error = OT_ERROR_NONE;
    uint8_t buffer[kApplicationDataMaxLength];

    VerifyOrExit(aLength <= kApplicationDataMaxLength, error = OT_ERROR_NO_BUFS);

    // Store message specific sub type.
    if (aMessage.GetSubType() != Message::kSubTypeNone)
    {
        mMessageSubType = aMessage.GetSubType();
    }

    aMessage.Read(0, aLength, buffer);

    SuccessOrExit(error = MapError(mbedtls_ssl_write(&mSsl, buffer, aLength)));

    aMessage.Free();

exit:
    return error;
}

void Dtls::Receive(Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    mReceiveMessage = &aMessage;
    mReceiveOffset  = aOffset;
    mReceiveLength  = aLength;

    Process();
}

int Dtls::HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsTransmit(aBuf, aLength);
}

int Dtls::HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength)
{
    otError error;
    int     rval = 0;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("Dtls::HandleMbedtlsTransmit");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("Dtls::ApplicationCoapSecure HandleMbedtlsTransmit");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    error = mSendHandler(mContext, aBuf, static_cast<uint16_t>(aLength), mMessageSubType);

    // Restore default sub type.
    mMessageSubType = mMessageDefaultSubType;

    switch (error)
    {
    case OT_ERROR_NONE:
        rval = static_cast<int>(aLength);
        break;

    case OT_ERROR_NO_BUFS:
        rval = MBEDTLS_ERR_SSL_WANT_WRITE;
        break;

    default:
        otLogWarnMeshCoP("Dtls::HandleMbedtlsTransmit: %s error", otThreadErrorToString(error));
        rval = MBEDTLS_ERR_NET_SEND_FAILED;
        break;
    }

    return rval;
}

int Dtls::HandleMbedtlsReceive(void *aContext, unsigned char *aBuf, size_t aLength)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsReceive(aBuf, aLength);
}

int Dtls::HandleMbedtlsReceive(unsigned char *aBuf, size_t aLength)
{
    int rval;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("Dtls::HandleMbedtlsReceive");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("Dtls:: ApplicationCoapSecure HandleMbedtlsReceive");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    VerifyOrExit(mReceiveMessage != NULL && mReceiveLength != 0, rval = MBEDTLS_ERR_SSL_WANT_READ);

    if (aLength > mReceiveLength)
    {
        aLength = mReceiveLength;
    }

    rval = (int)mReceiveMessage->Read(mReceiveOffset, (uint16_t)aLength, aBuf);
    mReceiveOffset += static_cast<uint16_t>(rval);
    mReceiveLength -= static_cast<uint16_t>(rval);

exit:
    return rval;
}

int Dtls::HandleMbedtlsGetTimer(void *aContext)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsGetTimer();
}

int Dtls::HandleMbedtlsGetTimer(void)
{
    int rval;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("Dtls::HandleMbedtlsGetTimer");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("Dtls:: ApplicationCoapSecure HandleMbedtlsGetTimer");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    if (!mTimerSet)
    {
        rval = -1;
    }
    else if (!mTimer.IsRunning())
    {
        rval = 2;
    }
    else if (static_cast<int32_t>(mTimerIntermediate - TimerMilli::GetNow()) <= 0)
    {
        rval = 1;
    }
    else
    {
        rval = 0;
    }

    return rval;
}

void Dtls::HandleMbedtlsSetTimer(void *aContext, uint32_t aIntermediate, uint32_t aFinish)
{
    static_cast<Dtls *>(aContext)->HandleMbedtlsSetTimer(aIntermediate, aFinish);
}

void Dtls::HandleMbedtlsSetTimer(uint32_t aIntermediate, uint32_t aFinish)
{
    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("Dtls::SetTimer");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("Dtls::ApplicationCoapSecure SetTimer");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    if (aFinish == 0)
    {
        mTimerSet = false;
        mTimer.Stop();
    }
    else
    {
        mTimerSet = true;
        mTimer.Start(aFinish);
        mTimerIntermediate = TimerMilli::GetNow() + aIntermediate;
    }
}

int Dtls::HandleMbedtlsExportKeys(void *               aContext,
                                  const unsigned char *aMasterSecret,
                                  const unsigned char *aKeyBlock,
                                  size_t               aMacLength,
                                  size_t               aKeyLength,
                                  size_t               aIvLength)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsExportKeys(aMasterSecret, aKeyBlock, aMacLength, aKeyLength,
                                                                  aIvLength);
}

int Dtls::HandleMbedtlsExportKeys(const unsigned char *aMasterSecret,
                                  const unsigned char *aKeyBlock,
                                  size_t               aMacLength,
                                  size_t               aKeyLength,
                                  size_t               aIvLength)
{
    OT_UNUSED_VARIABLE(aMasterSecret);

    uint8_t        kek[Crypto::Sha256::kHashSize];
    Crypto::Sha256 sha256;

    sha256.Start();
    sha256.Update(aKeyBlock, 2 * static_cast<uint16_t>(aMacLength + aKeyLength + aIvLength));
    sha256.Finish(kek);

    GetNetif().GetKeyManager().SetKek(kek);

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogInfoMeshCoP("Generated KEK");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogInfoCoap("ApplicationCoapSecure Generated KEK");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    return 0;
}

void Dtls::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Dtls>().HandleTimer();
}

void Dtls::HandleTimer(void)
{
    switch (mState)
    {
    case kStateConnecting:
    case kStateConnected:
        Process();
        break;

    case kStateCloseNotify:
        mState = kStateStopped;
        mTimer.Stop();
        break;

    default:
        assert(false);
        break;
    }
}

void Dtls::Process(void)
{
    uint8_t buf[MBEDTLS_SSL_MAX_CONTENT_LEN];
    bool    shouldClose = false;
    int     rval;

    while ((mState == kStateConnecting) || (mState == kStateConnected))
    {
        if (mState == kStateConnecting)
        {
            rval = mbedtls_ssl_handshake(&mSsl);

            if (mSsl.state == MBEDTLS_SSL_HANDSHAKE_OVER)
            {
                mState = kStateConnected;

                if (mConnectedHandler != NULL)
                {
                    mConnectedHandler(mContext, true);
                }
            }
        }
        else
        {
            rval = mbedtls_ssl_read(&mSsl, buf, sizeof(buf));
        }

        if (rval > 0)
        {
            mReceiveHandler(mContext, buf, static_cast<uint16_t>(rval));
        }
        else if (rval == 0 || rval == MBEDTLS_ERR_SSL_WANT_READ || rval == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            break;
        }
        else
        {
            switch (rval)
            {
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                mbedtls_ssl_close_notify(&mSsl);
                ExitNow(shouldClose = true);
                break;

            case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
                break;

            case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
                mbedtls_ssl_close_notify(&mSsl);
                ExitNow(shouldClose = true);
                break;

            case MBEDTLS_ERR_SSL_INVALID_MAC:
                if (mSsl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC);
                    ExitNow(shouldClose = true);
                }

                break;

            default:
                if (mSsl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE);
                    ExitNow(shouldClose = true);
                }

                break;
            }

            mbedtls_ssl_session_reset(&mSsl);
            if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
            {
                mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
            }
            break;
        }
    }

exit:

    if (shouldClose)
    {
        Close();
    }
}

otError Dtls::MapError(int rval)
{
    otError error = OT_ERROR_NONE;

    switch (rval)
    {
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_PK_TYPE_MISMATCH:
    case MBEDTLS_ERR_PK_FILE_IO_ERROR:
    case MBEDTLS_ERR_PK_KEY_INVALID_VERSION:
    case MBEDTLS_ERR_PK_KEY_INVALID_FORMAT:
    case MBEDTLS_ERR_PK_UNKNOWN_PK_ALG:
    case MBEDTLS_ERR_PK_PASSWORD_REQUIRED:
    case MBEDTLS_ERR_PK_PASSWORD_MISMATCH:
    case MBEDTLS_ERR_PK_INVALID_PUBKEY:
    case MBEDTLS_ERR_PK_INVALID_ALG:
    case MBEDTLS_ERR_PK_UNKNOWN_NAMED_CURVE:
    case MBEDTLS_ERR_PK_BAD_INPUT_DATA:
    case MBEDTLS_ERR_X509_SIG_MISMATCH:
    case MBEDTLS_ERR_X509_BAD_INPUT_DATA:
    case MBEDTLS_ERR_X509_FILE_IO_ERROR:
    case MBEDTLS_ERR_X509_CERT_UNKNOWN_FORMAT:
    case MBEDTLS_ERR_X509_INVALID_VERSION:
    case MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG:
    case MBEDTLS_ERR_X509_INVALID_SERIAL:
    case MBEDTLS_ERR_X509_UNKNOWN_OID:
    case MBEDTLS_ERR_X509_INVALID_FORMAT:
    case MBEDTLS_ERR_X509_INVALID_ALG:
    case MBEDTLS_ERR_X509_INVALID_NAME:
    case MBEDTLS_ERR_X509_INVALID_DATE:
    case MBEDTLS_ERR_X509_INVALID_SIGNATURE:
    case MBEDTLS_ERR_X509_INVALID_EXTENSIONS:
    case MBEDTLS_ERR_X509_UNKNOWN_VERSION:
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_SSL_BAD_INPUT_DATA:
        error = OT_ERROR_INVALID_ARGS;
        break;

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_PK_ALLOC_FAILED:
    case MBEDTLS_ERR_X509_BUFFER_TOO_SMALL:
    case MBEDTLS_ERR_X509_ALLOC_FAILED:
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_SSL_ALLOC_FAILED:
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        error = OT_ERROR_NO_BUFS;
        break;

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE:
    case MBEDTLS_ERR_PK_SIG_LEN_MISMATCH:
    case MBEDTLS_ERR_X509_FEATURE_UNAVAILABLE:
    case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED:
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED:
    case MBEDTLS_ERR_SSL_PEER_VERIFY_FAILED:
        error = OT_ERROR_SECURITY;
        break;

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case MBEDTLS_ERR_X509_FATAL_ERROR:
        error = OT_ERROR_FAILED;
        break;
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

    case MBEDTLS_ERR_SSL_TIMEOUT:
    case MBEDTLS_ERR_SSL_WANT_READ:
        error = OT_ERROR_BUSY;
        break;

    default:
        assert(rval >= 0);
        break;
    }

    return error;
}

void Dtls::HandleMbedtlsDebug(void *ctx, int level, const char *, int, const char *str)
{
    OT_UNUSED_VARIABLE(str);

    Dtls *pThis = static_cast<Dtls *>(ctx);

    if (pThis->mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        switch (level)
        {
        case 1:
            otLogCritMbedTls("%s", str);
            break;

        case 2:
            otLogWarnMbedTls("%s", str);
            break;

        case 3:
            otLogInfoMbedTls("%s", str);
            break;

        case 4:
        default:
            otLogDebgMbedTls("%s", str);
            break;
        }
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        switch (level)
        {
        case 1:
            otLogCritCoap("ApplicationCoapSecure Mbedtls: %s", str);
            break;
        case 2:
            otLogWarnCoap("ApplicationCoapSecure Mbedtls: %s", str);
            break;
        case 3:
            otLogInfoCoap("ApplicationCoapSecure Mbedtls: %s", str);
            break;
        case 4:
        default:
            otLogDebgCoap("ApplicationCoapSecure Mbedtls: %s", str);
            break;
        }
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_ENABLE_DTLS
