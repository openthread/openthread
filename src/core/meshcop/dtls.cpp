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
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#include <mbedtls/pem.h>
#endif
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "common/timer.hpp"
#include "crypto/mbedtls.hpp"
#include "crypto/sha256.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DTLS

namespace ot {
namespace MeshCoP {

Dtls::Dtls(Instance &aInstance, bool aLayerTwoSecurity)
    : InstanceLocator(aInstance)
    , mState(kStateClosed)
    , mPskLength(0)
    , mVerifyPeerCertificate(true)
    , mTimer(aInstance, &Dtls::HandleTimer, this)
    , mTimerIntermediate(0)
    , mTimerSet(false)
    , mLayerTwoSecurity(aLayerTwoSecurity)
    , mReceiveMessage(NULL)
    , mReceiveOffset(0)
    , mReceiveLength(0)
    , mConnectedHandler(NULL)
    , mReceiveHandler(NULL)
    , mSendHandler(NULL)
    , mContext(NULL)
    , mSocket(Get<Ip6::Udp>())
    , mTransportCallback(NULL)
    , mTransportContext(NULL)
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
    memset(&mSsl, 0, sizeof(mSsl));
    memset(&mConf, 0, sizeof(mConf));

#ifdef MBEDTLS_SSL_COOKIE_C
    memset(&mCookieCtx, 0, sizeof(mCookieCtx));
#endif
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
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSsl);
}

otError Dtls::Open(ReceiveHandler aReceiveHandler, ConnectedHandler aConnectedHandler, void *aContext)
{
    otError error;

    VerifyOrExit(mState == kStateClosed, error = OT_ERROR_ALREADY);

    SuccessOrExit(error = mSocket.Open(&Dtls::HandleUdpReceive, this));

    mReceiveHandler   = aReceiveHandler;
    mConnectedHandler = aConnectedHandler;
    mContext          = aContext;
    mState            = kStateOpen;

exit:
    return error;
}

otError Dtls::Connect(const Ip6::SockAddr &aSockAddr)
{
    otError error;

    VerifyOrExit(mState == kStateOpen, error = OT_ERROR_INVALID_STATE);

    memcpy(&mPeerAddress.mPeerAddr, &aSockAddr.mAddress, sizeof(mPeerAddress.mPeerAddr));
    mPeerAddress.mPeerPort = aSockAddr.mPort;

    if (aSockAddr.GetAddress().IsLinkLocal() || aSockAddr.GetAddress().IsMulticast())
    {
        mPeerAddress.mInterfaceId = aSockAddr.mScopeId;
    }
    else
    {
        mPeerAddress.mInterfaceId = 0;
    }

    error = Setup(true);

exit:
    return error;
}

void Dtls::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Dtls *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                    *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Dtls::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    switch (mState)
    {
    case MeshCoP::Dtls::kStateClosed:
        ExitNow();

    case MeshCoP::Dtls::kStateOpen:
    {
        Ip6::SockAddr sockAddr;

        sockAddr.mAddress = aMessageInfo.GetPeerAddr();
        sockAddr.mPort    = aMessageInfo.GetPeerPort();
        mSocket.Connect(sockAddr);

        mPeerAddress.SetPeerAddr(aMessageInfo.GetPeerAddr());
        mPeerAddress.SetPeerPort(aMessageInfo.GetPeerPort());
        mPeerAddress.SetInterfaceId(aMessageInfo.GetInterfaceId());

        if (Get<ThreadNetif>().IsUnicastAddress(aMessageInfo.GetSockAddr()))
        {
            mPeerAddress.SetSockAddr(aMessageInfo.GetSockAddr());
        }

        mPeerAddress.SetSockPort(aMessageInfo.GetSockPort());

        SuccessOrExit(Setup(false));
        break;
    }

    default:
        // Once DTLS session is started, communicate only with a peer.
        VerifyOrExit((mPeerAddress.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                     (mPeerAddress.GetPeerPort() == aMessageInfo.GetPeerPort()));
        break;
    }

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER
    if (mState == MeshCoP::Dtls::kStateConnecting)
    {
        SetClientId(mPeerAddress.GetPeerAddr().mFields.m8, sizeof(mPeerAddress.GetPeerAddr().mFields));
    }
#endif

    Receive(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

exit:
    return;
}

otError Dtls::Bind(uint16_t aPort)
{
    otError       error;
    Ip6::SockAddr sockaddr;

    VerifyOrExit(mState == kStateOpen, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mTransportCallback == NULL, error = OT_ERROR_ALREADY);

    sockaddr.mPort = aPort;
    SuccessOrExit(error = mSocket.Bind(sockaddr));

exit:
    return error;
}

otError Dtls::Bind(TransportCallback aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == kStateOpen, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!mSocket.IsBound(), error = OT_ERROR_ALREADY);
    VerifyOrExit(mTransportCallback == NULL, error = OT_ERROR_ALREADY);

    mTransportCallback = aCallback;
    mTransportContext  = aContext;

exit:
    return error;
}

otError Dtls::Setup(bool aClient)
{
    int rval;

    // do not handle new connection before guard time expired
    VerifyOrExit(mState == kStateOpen, rval = MBEDTLS_ERR_SSL_TIMEOUT);

    mState = kStateInitializing;

    mbedtls_ssl_init(&mSsl);
    mbedtls_ssl_config_init(&mConf);
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mbedtls_x509_crt_init(&mCaChain);
    mbedtls_x509_crt_init(&mOwnCert);
    mbedtls_pk_init(&mPrivateKey);
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

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

    mbedtls_ssl_conf_rng(&mConf, mbedtls_ctr_drbg_random, Random::Crypto::MbedTlsContextGet());
    mbedtls_ssl_conf_min_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    assert(mCipherSuites[1] == 0);
    mbedtls_ssl_conf_ciphersuites(&mConf, mCipherSuites);
    mbedtls_ssl_conf_export_keys_cb(&mConf, HandleMbedtlsExportKeys, this);
    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, HandleMbedtlsDebug, this);

#if OPENTHREAD_ENABLE_BORDER_AGENT || OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    if (!aClient)
    {
        mbedtls_ssl_cookie_init(&mCookieCtx);

        rval = mbedtls_ssl_cookie_setup(&mCookieCtx, mbedtls_ctr_drbg_random, Random::Crypto::MbedTlsContextGet());
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

    mReceiveMessage = NULL;
    mMessageSubType = Message::kSubTypeNone;
    mState          = kStateConnecting;

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

    mState = kStateConnecting;

    Process();

exit:
    if ((mState == kStateInitializing) && (rval != 0))
    {
        mState = kStateOpen;
        FreeMbedtls();
    }

    return Crypto::MbedTls::MapError(rval);
}

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
int Dtls::SetApplicationCoapSecureKeys(void)
{
    int rval = 0;

    switch (mCipherSuites[0])
    {
    case MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8:
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        if (mCaChainSrc != NULL)
        {
            rval = mbedtls_x509_crt_parse(&mCaChain, static_cast<const unsigned char *>(mCaChainSrc),
                                          static_cast<size_t>(mCaChainLength));
            VerifyOrExit(rval == 0);
            mbedtls_ssl_conf_ca_chain(&mConf, &mCaChain, NULL);
        }

        if (mOwnCertSrc != NULL && mPrivateKeySrc != NULL)
        {
            rval = mbedtls_x509_crt_parse(&mOwnCert, static_cast<const unsigned char *>(mOwnCertSrc),
                                          static_cast<size_t>(mOwnCertLength));
            VerifyOrExit(rval == 0);
            rval = mbedtls_pk_parse_key(&mPrivateKey, static_cast<const unsigned char *>(mPrivateKeySrc),
                                        static_cast<size_t>(mPrivateKeyLength), NULL, 0);
            VerifyOrExit(rval == 0);
            rval = mbedtls_ssl_conf_own_cert(&mConf, &mOwnCert, &mPrivateKey);
            VerifyOrExit(rval == 0);
        }
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        break;

    case MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8:
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
        rval = mbedtls_ssl_conf_psk(&mConf, static_cast<const unsigned char *>(mPreSharedKey), mPreSharedKeyLength,
                                    static_cast<const unsigned char *>(mPreSharedKeyIdentity), mPreSharedKeyIdLength);
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

void Dtls::Close(void)
{
    Disconnect();

    mState             = kStateClosed;
    mTransportCallback = NULL;
    mTransportContext  = NULL;
    mTimerSet          = false;

    mSocket.Close();
    mTimer.Stop();
}

void Dtls::Disconnect(void)
{
    VerifyOrExit(mState == kStateConnecting || mState == kStateConnected);

    mbedtls_ssl_close_notify(&mSsl);
    mState = kStateCloseNotify;
    mTimer.Start(kGuardTimeNewConnectionMilli);

    new (&mPeerAddress) Ip6::MessageInfo();
    mSocket.Connect(Ip6::SockAddr());

    if (mConnectedHandler != NULL)
    {
        mConnectedHandler(mContext, false);
    }

    FreeMbedtls();

exit:
    return;
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
    return Crypto::MbedTls::MapError(rval);
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

    SuccessOrExit(error = Crypto::MbedTls::MapError(mbedtls_ssl_write(&mSsl, buffer, aLength)));

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
        otLogDebgMeshCoP("Dtls::HandleMbedtlsTransmit");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogDebgCoap("Dtls::ApplicationCoapSecure HandleMbedtlsTransmit");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    error = HandleDtlsSend(aBuf, static_cast<uint16_t>(aLength), mMessageSubType);

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
        otLogDebgMeshCoP("Dtls::HandleMbedtlsReceive");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogDebgCoap("Dtls:: ApplicationCoapSecure HandleMbedtlsReceive");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

    VerifyOrExit(mReceiveMessage != NULL && mReceiveLength != 0, rval = MBEDTLS_ERR_SSL_WANT_READ);

    if (aLength > mReceiveLength)
    {
        aLength = mReceiveLength;
    }

    rval = mReceiveMessage->Read(mReceiveOffset, static_cast<uint16_t>(aLength), aBuf);
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
        otLogDebgMeshCoP("Dtls::HandleMbedtlsGetTimer");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogDebgCoap("Dtls:: ApplicationCoapSecure HandleMbedtlsGetTimer");
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
        otLogDebgMeshCoP("Dtls::SetTimer");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogDebgCoap("Dtls::ApplicationCoapSecure SetTimer");
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

    Get<KeyManager>().SetKek(kek);

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        otLogDebgMeshCoP("Generated KEK");
    }
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    else
    {
        otLogDebgCoap("ApplicationCoapSecure Generated KEK");
    }
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
    return 0;
}

void Dtls::HandleTimer(Timer &aTimer)
{
    static_cast<Dtls *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
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
        mState = kStateOpen;
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
    bool    shouldDisconnect = false;
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
                ExitNow(shouldDisconnect = true);
                break;

            case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
                break;

            case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
                mbedtls_ssl_close_notify(&mSsl);
                ExitNow(shouldDisconnect = true);
                break;

            case MBEDTLS_ERR_SSL_INVALID_MAC:
                if (mSsl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC);
                    ExitNow(shouldDisconnect = true);
                }

                break;

            default:
                if (mSsl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE);
                    ExitNow(shouldDisconnect = true);
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

    if (shouldDisconnect)
    {
        Disconnect();
    }
}

void Dtls::HandleMbedtlsDebug(void *ctx, int level, const char *, int, const char *str)
{
    OT_UNUSED_VARIABLE(str);

    Dtls *pThis = static_cast<Dtls *>(ctx);
    OT_UNUSED_VARIABLE(pThis);

    switch (level)
    {
    case 1:
        otLogCritMbedTls("[%hu] %s", pThis->mSocket.GetSockName().mPort, str);
        break;

    case 2:
        otLogWarnMbedTls("[%hu] %s", pThis->mSocket.GetSockName().mPort, str);
        break;

    case 3:
        otLogInfoMbedTls("[%hu] %s", pThis->mSocket.GetSockName().mPort, str);
        break;

    case 4:
    default:
        otLogDebgMbedTls("[%hu] %s", pThis->mSocket.GetSockName().mPort, str);
        break;
    }
}

otError Dtls::HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength, uint8_t aMessageSubType)
{
    otError      error   = OT_ERROR_NONE;
    ot::Message *message = NULL;

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(aMessageSubType);
    message->SetLinkSecurityEnabled(mLayerTwoSecurity);

    SuccessOrExit(error = message->Append(aBuf, aLength));

    // Set message sub type in case Joiner Finalize Response is appended to the message.
    if (aMessageSubType != Message::kSubTypeNone)
    {
        message->SetSubType(aMessageSubType);
    }

    if (mTransportCallback)
    {
        SuccessOrExit(error = mTransportCallback(mTransportContext, *message, mPeerAddress));
    }
    else
    {
        SuccessOrExit(error = mSocket.SendTo(*message, mPeerAddress));
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_ENABLE_DTLS
