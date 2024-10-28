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

#include "secure_transport.hpp"

#include <mbedtls/debug.h>
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#include <mbedtls/pem.h>
#endif

#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

namespace ot {
namespace MeshCoP {

RegisterLogModule("SecTransport");

#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
const uint16_t SecureTransport::sGroups[] = {MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1, MBEDTLS_SSL_IANA_TLS_GROUP_NONE};
#else
const mbedtls_ecp_group_id SecureTransport::sCurves[] = {MBEDTLS_ECP_DP_SECP256R1, MBEDTLS_ECP_DP_NONE};
#endif

#if defined(MBEDTLS_KEY_EXCHANGE__WITH_CERT__ENABLED) || defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
const uint16_t SecureTransport::sSignatures[] = {MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256, MBEDTLS_TLS1_3_SIG_NONE};
#else
const int SecureTransport::sHashes[] = {MBEDTLS_MD_SHA256, MBEDTLS_MD_NONE};
#endif
#endif

SecureTransport::SecureTransport(Instance &aInstance, bool aLayerTwoSecurity, bool aDatagramTransport)
    : InstanceLocator(aInstance)
    , mState(kStateClosed)
    , mPskLength(0)
    , mVerifyPeerCertificate(true)
    , mTimer(aInstance, SecureTransport::HandleTimer, this)
    , mTimerIntermediate(0)
    , mTimerSet(false)
    , mLayerTwoSecurity(aLayerTwoSecurity)
    , mDatagramTransport(aDatagramTransport)
    , mMaxConnectionAttempts(0)
    , mRemainingConnectionAttempts(0)
    , mReceiveMessage(nullptr)
    , mSocket(aInstance, *this)
    , mMessageSubType(Message::kSubTypeNone)
    , mMessageDefaultSubType(Message::kSubTypeNone)
{
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    mPreSharedKey         = nullptr;
    mPreSharedKeyIdentity = nullptr;
    mPreSharedKeyIdLength = 0;
    mPreSharedKeyLength   = 0;
#endif

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mCaChainSrc       = nullptr;
    mCaChainLength    = 0;
    mOwnCertSrc       = nullptr;
    mOwnCertLength    = 0;
    mPrivateKeySrc    = nullptr;
    mPrivateKeyLength = 0;
    ClearAllBytes(mCaChain);
    ClearAllBytes(mOwnCert);
    ClearAllBytes(mPrivateKey);
#endif
#endif

    ClearAllBytes(mCipherSuites);
    ClearAllBytes(mPsk);
    ClearAllBytes(mSsl);
    ClearAllBytes(mConf);

#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    ClearAllBytes(mCookieCtx);
#endif
}

void SecureTransport::FreeMbedtls(void)
{
#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (mDatagramTransport)
    {
        mbedtls_ssl_cookie_free(&mCookieCtx);
    }
#endif
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mbedtls_x509_crt_free(&mCaChain);
    mbedtls_x509_crt_free(&mOwnCert);
    mbedtls_pk_free(&mPrivateKey);
#endif
#endif
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSsl);
}

void SecureTransport::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("State: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

exit:
    return;
}

Error SecureTransport::Open(ReceiveHandler aReceiveHandler, ConnectedHandler aConnectedHandler, void *aContext)
{
    Error error;

    VerifyOrExit(IsStateClosed(), error = kErrorAlready);

    SuccessOrExit(error = mSocket.Open());

    mConnectedCallback.Set(aConnectedHandler, aContext);
    mReceiveCallback.Set(aReceiveHandler, aContext);

    mRemainingConnectionAttempts = mMaxConnectionAttempts;

    SetState(kStateOpen);

exit:
    return error;
}

Error SecureTransport::SetMaxConnectionAttempts(uint16_t aMaxAttempts, AutoCloseCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsStateClosed(), error = kErrorInvalidState);

    mMaxConnectionAttempts = aMaxAttempts;
    mAutoCloseCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error SecureTransport::Connect(const Ip6::SockAddr &aSockAddr)
{
    Error error;

    VerifyOrExit(IsStateOpen(), error = kErrorInvalidState);

    if (mRemainingConnectionAttempts > 0)
    {
        mRemainingConnectionAttempts--;
    }

    mMessageInfo.SetPeerAddr(aSockAddr.GetAddress());
    mMessageInfo.SetPeerPort(aSockAddr.mPort);

    error = Setup(true);

exit:
    return error;
}

void SecureTransport::HandleReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(!IsStateClosed());

    if (IsStateOpen())
    {
        if (mRemainingConnectionAttempts > 0)
        {
            mRemainingConnectionAttempts--;
        }

        mMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());
        mMessageInfo.SetPeerPort(aMessageInfo.GetPeerPort());
        mMessageInfo.SetIsHostInterface(aMessageInfo.IsHostInterface());

        mMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
        mMessageInfo.SetSockPort(aMessageInfo.GetSockPort());

        SuccessOrExit(Setup(false));
    }
    else
    {
        // Once DTLS session is started, communicate only with a single peer.
        VerifyOrExit((mMessageInfo.GetPeerAddr() == aMessageInfo.GetPeerAddr()) &&
                     (mMessageInfo.GetPeerPort() == aMessageInfo.GetPeerPort()));
    }

#ifdef MBEDTLS_SSL_SRV_C
    if (IsStateConnecting())
    {
        IgnoreError(SetClientId(mMessageInfo.GetPeerAddr().mFields.m8, sizeof(mMessageInfo.GetPeerAddr().mFields)));
    }
#endif

    Receive(aMessage);

exit:
    return;
}

uint16_t SecureTransport::GetUdpPort(void) const { return mSocket.GetSockName().GetPort(); }

Error SecureTransport::Bind(uint16_t aPort)
{
    Error error;

    VerifyOrExit(IsStateOpen(), error = kErrorInvalidState);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    SuccessOrExit(error = mSocket.Bind(aPort, Ip6::kNetifUnspecified));

exit:
    return error;
}

Error SecureTransport::Bind(TransportCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(IsStateOpen(), error = kErrorInvalidState);
    VerifyOrExit(!mSocket.IsBound(), error = kErrorAlready);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    mTransportCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error SecureTransport::Setup(bool aClient)
{
    int rval;

    // do not handle new connection before guard time expired
    VerifyOrExit(IsStateOpen(), rval = MBEDTLS_ERR_SSL_TIMEOUT);

    SetState(kStateInitializing);

    mbedtls_ssl_init(&mSsl);
    mbedtls_ssl_config_init(&mConf);
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    mbedtls_x509_crt_init(&mCaChain);
    mbedtls_x509_crt_init(&mOwnCert);
    mbedtls_pk_init(&mPrivateKey);
#endif
#endif
#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (mDatagramTransport)
    {
        mbedtls_ssl_cookie_init(&mCookieCtx);
    }
#endif

    rval = mbedtls_ssl_config_defaults(
        &mConf, aClient ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
        mDatagramTransport ? MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    VerifyOrExit(rval == 0);

#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    if (mVerifyPeerCertificate && (mCipherSuites[0] == MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 ||
                                   mCipherSuites[0] == MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256))
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_NONE);
    }
#else
    OT_UNUSED_VARIABLE(mVerifyPeerCertificate);
#endif

    mbedtls_ssl_conf_rng(&mConf, Crypto::MbedTls::CryptoSecurePrng, nullptr);
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
    mbedtls_ssl_conf_min_tls_version(&mConf, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&mConf, MBEDTLS_SSL_VERSION_TLS1_2);
#else
    mbedtls_ssl_conf_min_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
#endif

    OT_ASSERT(mCipherSuites[1] == 0);
    mbedtls_ssl_conf_ciphersuites(&mConf, mCipherSuites);
    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
        mbedtls_ssl_conf_groups(&mConf, sGroups);
#else
        mbedtls_ssl_conf_curves(&mConf, sCurves);
#endif
#if defined(MBEDTLS_KEY_EXCHANGE__WITH_CERT__ENABLED) || defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
        mbedtls_ssl_conf_sig_algs(&mConf, sSignatures);
#else
        mbedtls_ssl_conf_sig_hashes(&mConf, sHashes);
#endif
#endif
    }

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_ssl_set_export_keys_cb(&mSsl, HandleMbedtlsExportKeys, this);
#else
    mbedtls_ssl_conf_export_keys_cb(&mConf, HandleMbedtlsExportKeys, this);
#endif

    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, HandleMbedtlsDebug, this);

#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (!aClient && mDatagramTransport)
    {
        rval = mbedtls_ssl_cookie_setup(&mCookieCtx, Crypto::MbedTls::CryptoSecurePrng, nullptr);
        VerifyOrExit(rval == 0);

        mbedtls_ssl_conf_dtls_cookies(&mConf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &mCookieCtx);
    }
#endif

    rval = mbedtls_ssl_setup(&mSsl, &mConf);
    VerifyOrExit(rval == 0);

    mbedtls_ssl_set_bio(&mSsl, this, &SecureTransport::HandleMbedtlsTransmit, HandleMbedtlsReceive, nullptr);

    if (mDatagramTransport)
    {
        mbedtls_ssl_set_timer_cb(&mSsl, this, &SecureTransport::HandleMbedtlsSetTimer, HandleMbedtlsGetTimer);
    }

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        rval = mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        rval = SetApplicationSecureKeys();
    }
#endif
    VerifyOrExit(rval == 0);

    mReceiveMessage = nullptr;
    mMessageSubType = Message::kSubTypeNone;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        LogInfo("DTLS started");
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        LogInfo("Application Secure (D)TLS started");
    }
#endif

    SetState(kStateConnecting);

    Process();

exit:
    if (IsStateInitializing() && (rval != 0))
    {
        if ((mMaxConnectionAttempts > 0) && (mRemainingConnectionAttempts == 0))
        {
            Close();
            mAutoCloseCallback.InvokeIfSet();
        }
        else
        {
            SetState(kStateOpen);
            FreeMbedtls();
        }
    }

    return Crypto::MbedTls::MapError(rval);
}

#if OPENTHREAD_CONFIG_TLS_API_ENABLE
int SecureTransport::SetApplicationSecureKeys(void)
{
    int rval = 0;

    switch (mCipherSuites[0])
    {
    case MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8:
    case MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        if (mCaChainSrc != nullptr)
        {
            rval = mbedtls_x509_crt_parse(&mCaChain, static_cast<const unsigned char *>(mCaChainSrc),
                                          static_cast<size_t>(mCaChainLength));
            VerifyOrExit(rval == 0);
            mbedtls_ssl_conf_ca_chain(&mConf, &mCaChain, nullptr);
        }

        if (mOwnCertSrc != nullptr && mPrivateKeySrc != nullptr)
        {
            rval = mbedtls_x509_crt_parse(&mOwnCert, static_cast<const unsigned char *>(mOwnCertSrc),
                                          static_cast<size_t>(mOwnCertLength));
            VerifyOrExit(rval == 0);

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
            rval = mbedtls_pk_parse_key(&mPrivateKey, static_cast<const unsigned char *>(mPrivateKeySrc),
                                        static_cast<size_t>(mPrivateKeyLength), nullptr, 0,
                                        Crypto::MbedTls::CryptoSecurePrng, nullptr);
#else
            rval = mbedtls_pk_parse_key(&mPrivateKey, static_cast<const unsigned char *>(mPrivateKeySrc),
                                        static_cast<size_t>(mPrivateKeyLength), nullptr, 0);
#endif
            VerifyOrExit(rval == 0);
            rval = mbedtls_ssl_conf_own_cert(&mConf, &mOwnCert, &mPrivateKey);
            VerifyOrExit(rval == 0);
        }
#endif
        break;

    case MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8:
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
        rval = mbedtls_ssl_conf_psk(&mConf, static_cast<const unsigned char *>(mPreSharedKey), mPreSharedKeyLength,
                                    static_cast<const unsigned char *>(mPreSharedKeyIdentity), mPreSharedKeyIdLength);
        VerifyOrExit(rval == 0);
#endif
        break;

    default:
        LogCrit("Application Coap Secure: Not supported cipher.");
        rval = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
        ExitNow();
        break;
    }

exit:
    return rval;
}

#endif // OPENTHREAD_CONFIG_TLS_API_ENABLE

void SecureTransport::Close(void)
{
    Disconnect(kDisconnectedLocalClosed);

    SetState(kStateClosed);
    mTimerSet = false;
    mTransportCallback.Clear();

    IgnoreError(mSocket.Close());
    mTimer.Stop();
}

void SecureTransport::Disconnect(void) { Disconnect(kDisconnectedLocalClosed); }

void SecureTransport::Disconnect(ConnectEvent aEvent)
{
    VerifyOrExit(IsStateConnectingOrConnected());

    mbedtls_ssl_close_notify(&mSsl);
    SetState(kStateCloseNotify);
    mConnectEvent = aEvent;
    mTimer.Start(kGuardTimeNewConnectionMilli);

    mMessageInfo.Clear();

    FreeMbedtls();

exit:
    return;
}

Error SecureTransport::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aPskLength <= sizeof(mPsk), error = kErrorInvalidArgs);

    memcpy(mPsk, aPsk, aPskLength);
    mPskLength       = aPskLength;
    mCipherSuites[0] = MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8;
    mCipherSuites[1] = 0;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TLS_API_ENABLE
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

void SecureTransport::SetCertificate(const uint8_t *aX509Certificate,
                                     uint32_t       aX509CertLength,
                                     const uint8_t *aPrivateKey,
                                     uint32_t       aPrivateKeyLength)
{
    OT_ASSERT(aX509CertLength > 0);
    OT_ASSERT(aX509Certificate != nullptr);

    OT_ASSERT(aPrivateKeyLength > 0);
    OT_ASSERT(aPrivateKey != nullptr);

    mOwnCertSrc       = aX509Certificate;
    mOwnCertLength    = aX509CertLength;
    mPrivateKeySrc    = aPrivateKey;
    mPrivateKeyLength = aPrivateKeyLength;

    if (mDatagramTransport)
    {
        mCipherSuites[0] = MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8;
    }
    else
    {
        mCipherSuites[0] = MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
    }

    mCipherSuites[1] = 0;
}

void SecureTransport::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    OT_ASSERT(aX509CaCertChainLength > 0);
    OT_ASSERT(aX509CaCertificateChain != nullptr);

    mCaChainSrc    = aX509CaCertificateChain;
    mCaChainLength = aX509CaCertChainLength;
}

#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
void SecureTransport::SetPreSharedKey(const uint8_t *aPsk,
                                      uint16_t       aPskLength,
                                      const uint8_t *aPskIdentity,
                                      uint16_t       aPskIdLength)
{
    OT_ASSERT(aPsk != nullptr);
    OT_ASSERT(aPskIdentity != nullptr);
    OT_ASSERT(aPskLength > 0);
    OT_ASSERT(aPskIdLength > 0);

    mPreSharedKey         = aPsk;
    mPreSharedKeyLength   = aPskLength;
    mPreSharedKeyIdentity = aPskIdentity;
    mPreSharedKeyIdLength = aPskIdLength;

    mCipherSuites[0] = MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8;
    mCipherSuites[1] = 0;
}
#endif

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
Error SecureTransport::GetPeerCertificateBase64(unsigned char *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
{
    Error error = kErrorNone;

    VerifyOrExit(IsStateConnected(), error = kErrorInvalidState);

#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
    VerifyOrExit(mbedtls_base64_encode(aPeerCert, aCertBufferSize, aCertLength,
                                       mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.p,
                                       mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.len) == 0,
                 error = kErrorNoBufs);
#else
    VerifyOrExit(
        mbedtls_base64_encode(
            aPeerCert, aCertBufferSize, aCertLength,
            mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(p),
            mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(len)) == 0,
        error = kErrorNoBufs);
#endif

exit:
    return error;
}
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
Error SecureTransport::GetPeerSubjectAttributeByOid(const char *aOid,
                                                    size_t      aOidLength,
                                                    uint8_t    *aAttributeBuffer,
                                                    size_t     *aAttributeLength,
                                                    int        *aAsn1Type)
{
    Error                          error = kErrorNone;
    const mbedtls_asn1_named_data *data;
    size_t                         length;
    size_t                         attributeBufferSize;
    mbedtls_x509_crt              *peerCert = const_cast<mbedtls_x509_crt *>(mbedtls_ssl_get_peer_cert(&mSsl));

    VerifyOrExit(aAttributeLength != nullptr, error = kErrorInvalidArgs);
    attributeBufferSize = *aAttributeLength;
    *aAttributeLength   = 0;

    VerifyOrExit(aAttributeBuffer != nullptr, error = kErrorNoBufs);
    VerifyOrExit(peerCert != nullptr, error = kErrorInvalidState);

    data = mbedtls_asn1_find_named_data(&peerCert->subject, aOid, aOidLength);
    VerifyOrExit(data != nullptr, error = kErrorNotFound);

    length = data->val.len;
    VerifyOrExit(length <= attributeBufferSize, error = kErrorNoBufs);
    *aAttributeLength = length;

    if (aAsn1Type != nullptr)
    {
        *aAsn1Type = data->val.tag;
    }

    memcpy(aAttributeBuffer, data->val.p, length);

exit:
    return error;
}

Error SecureTransport::GetThreadAttributeFromPeerCertificate(int      aThreadOidDescriptor,
                                                             uint8_t *aAttributeBuffer,
                                                             size_t  *aAttributeLength)
{
    const mbedtls_x509_crt *cert = mbedtls_ssl_get_peer_cert(&mSsl);

    return GetThreadAttributeFromCertificate(cert, aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}

#endif // defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

Error SecureTransport::GetThreadAttributeFromOwnCertificate(int      aThreadOidDescriptor,
                                                            uint8_t *aAttributeBuffer,
                                                            size_t  *aAttributeLength)
{
    const mbedtls_x509_crt *cert = &mOwnCert;

    return GetThreadAttributeFromCertificate(cert, aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}

Error SecureTransport::GetThreadAttributeFromCertificate(const mbedtls_x509_crt *aCert,
                                                         int                     aThreadOidDescriptor,
                                                         uint8_t                *aAttributeBuffer,
                                                         size_t                 *aAttributeLength)
{
    Error            error  = kErrorNotFound;
    char             oid[9] = {0x2B, 0x06, 0x01, 0x04, 0x01, static_cast<char>(0x82), static_cast<char>(0xDF),
                               0x2A, 0x00}; // 1.3.6.1.4.1.44970.0
    mbedtls_x509_buf v3_ext;
    unsigned char   *p, *end, *endExtData;
    size_t           len;
    size_t           attributeBufferSize;
    mbedtls_x509_buf extnOid;
    int              ret, isCritical;

    VerifyOrExit(aAttributeLength != nullptr, error = kErrorInvalidArgs);
    attributeBufferSize = *aAttributeLength;
    *aAttributeLength   = 0;

    VerifyOrExit(aCert != nullptr, error = kErrorInvalidState);
    v3_ext = aCert->v3_ext;
    p      = v3_ext.p;
    VerifyOrExit(p != nullptr, error = kErrorInvalidState);
    end = p + v3_ext.len;
    VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) == 0,
                 error = kErrorParse);
    VerifyOrExit(end == p + len, error = kErrorParse);

    VerifyOrExit(aThreadOidDescriptor < 128, error = kErrorNotImplemented);
    oid[sizeof(oid) - 1] = static_cast<char>(aThreadOidDescriptor);

    while (p < end)
    {
        isCritical = 0;
        VerifyOrExit(mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) == 0,
                     error = kErrorParse);
        endExtData = p + len;

        // Get extension ID
        VerifyOrExit(mbedtls_asn1_get_tag(&p, endExtData, &extnOid.len, MBEDTLS_ASN1_OID) == 0, error = kErrorParse);
        extnOid.tag = MBEDTLS_ASN1_OID;
        extnOid.p   = p;
        p += extnOid.len;

        // Get optional critical
        ret = mbedtls_asn1_get_bool(&p, endExtData, &isCritical);
        VerifyOrExit(ret == 0 || ret == MBEDTLS_ERR_ASN1_UNEXPECTED_TAG, error = kErrorParse);

        // Data must be octet string type, see https://datatracker.ietf.org/doc/html/rfc5280#section-4.1
        VerifyOrExit(mbedtls_asn1_get_tag(&p, endExtData, &len, MBEDTLS_ASN1_OCTET_STRING) == 0, error = kErrorParse);
        VerifyOrExit(endExtData == p + len, error = kErrorParse);

        // TODO: extensions with isCritical == 1 that are unknown should lead to rejection of the entire cert.
        if (extnOid.len == sizeof(oid) && memcmp(extnOid.p, oid, sizeof(oid)) == 0)
        {
            // per RFC 5280, octet string must contain ASN.1 Type Length Value octets
            VerifyOrExit(len >= 2, error = kErrorParse);
            VerifyOrExit(*(p + 1) == len - 2, error = kErrorParse); // check TLV Length, not Type.
            *aAttributeLength = len - 2; // strip the ASN.1 Type Length bytes from embedded TLV

            if (aAttributeBuffer != nullptr)
            {
                VerifyOrExit(*aAttributeLength <= attributeBufferSize, error = kErrorNoBufs);
                memcpy(aAttributeBuffer, p + 2, *aAttributeLength);
            }

            error = kErrorNone;
            break;
        }
        p += len;
    }

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_TLS_API_ENABLE

#ifdef MBEDTLS_SSL_SRV_C
Error SecureTransport::SetClientId(const uint8_t *aClientId, uint8_t aLength)
{
    int rval = mbedtls_ssl_set_client_transport_id(&mSsl, aClientId, aLength);
    return Crypto::MbedTls::MapError(rval);
}
#endif

Error SecureTransport::Send(Message &aMessage, uint16_t aLength)
{
    Error   error = kErrorNone;
    uint8_t buffer[kApplicationDataMaxLength];

    VerifyOrExit(aLength <= kApplicationDataMaxLength, error = kErrorNoBufs);

    // Store message specific sub type.
    if (aMessage.GetSubType() != Message::kSubTypeNone)
    {
        mMessageSubType = aMessage.GetSubType();
    }

    aMessage.ReadBytes(0, buffer, aLength);

    SuccessOrExit(error = Crypto::MbedTls::MapError(mbedtls_ssl_write(&mSsl, buffer, aLength)));

    aMessage.Free();

exit:
    return error;
}

void SecureTransport::Receive(Message &aMessage)
{
    mReceiveMessage = &aMessage;

    Process();

    mReceiveMessage = nullptr;
}

int SecureTransport::HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsTransmit(aBuf, aLength);
}

int SecureTransport::HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength)
{
    Error error;
    int   rval = 0;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        LogDebg("HandleMbedtlsTransmit DTLS");
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        LogDebg("HandleMbedtlsTransmit TLS");
    }
#endif

    error = HandleSecureTransportSend(aBuf, static_cast<uint16_t>(aLength), mMessageSubType);

    // Restore default sub type.
    mMessageSubType = mMessageDefaultSubType;

    switch (error)
    {
    case kErrorNone:
        rval = static_cast<int>(aLength);
        break;

    case kErrorNoBufs:
        rval = MBEDTLS_ERR_SSL_WANT_WRITE;
        break;

    default:
        LogWarn("HandleMbedtlsTransmit: %s error", ErrorToString(error));
        rval = MBEDTLS_ERR_NET_SEND_FAILED;
        break;
    }

    return rval;
}

int SecureTransport::HandleMbedtlsReceive(void *aContext, unsigned char *aBuf, size_t aLength)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsReceive(aBuf, aLength);
}

int SecureTransport::HandleMbedtlsReceive(unsigned char *aBuf, size_t aLength)
{
    int rval;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        LogDebg("HandleMbedtlsReceive DTLS");
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        LogDebg("HandleMbedtlsReceive TLS");
    }
#endif

    VerifyOrExit(mReceiveMessage != nullptr && (rval = mReceiveMessage->GetLength() - mReceiveMessage->GetOffset()) > 0,
                 rval = MBEDTLS_ERR_SSL_WANT_READ);

    if (aLength > static_cast<size_t>(rval))
    {
        aLength = static_cast<size_t>(rval);
    }

    rval = mReceiveMessage->ReadBytes(mReceiveMessage->GetOffset(), aBuf, static_cast<uint16_t>(aLength));
    mReceiveMessage->MoveOffset(rval);

exit:
    return rval;
}

int SecureTransport::HandleMbedtlsGetTimer(void *aContext)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsGetTimer();
}

int SecureTransport::HandleMbedtlsGetTimer(void)
{
    int rval;

    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        LogDebg("HandleMbedtlsGetTimer");
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        LogDebg("HandleMbedtlsGetTimer");
    }
#endif

    if (!mTimerSet)
    {
        rval = -1;
    }
    else if (!mTimer.IsRunning())
    {
        rval = 2;
    }
    else if (mTimerIntermediate <= TimerMilli::GetNow())
    {
        rval = 1;
    }
    else
    {
        rval = 0;
    }

    return rval;
}

void SecureTransport::HandleMbedtlsSetTimer(void *aContext, uint32_t aIntermediate, uint32_t aFinish)
{
    static_cast<SecureTransport *>(aContext)->HandleMbedtlsSetTimer(aIntermediate, aFinish);
}

void SecureTransport::HandleMbedtlsSetTimer(uint32_t aIntermediate, uint32_t aFinish)
{
    if (mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8)
    {
        LogDebg("SetTimer DTLS");
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else
    {
        LogDebg("SetTimer TLS");
    }
#endif

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

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)

void SecureTransport::HandleMbedtlsExportKeys(void                       *aContext,
                                              mbedtls_ssl_key_export_type aType,
                                              const unsigned char        *aMasterSecret,
                                              size_t                      aMasterSecretLen,
                                              const unsigned char         aClientRandom[32],
                                              const unsigned char         aServerRandom[32],
                                              mbedtls_tls_prf_types       aTlsPrfType)
{
    static_cast<SecureTransport *>(aContext)->HandleMbedtlsExportKeys(aType, aMasterSecret, aMasterSecretLen,
                                                                      aClientRandom, aServerRandom, aTlsPrfType);
}

void SecureTransport::HandleMbedtlsExportKeys(mbedtls_ssl_key_export_type aType,
                                              const unsigned char        *aMasterSecret,
                                              size_t                      aMasterSecretLen,
                                              const unsigned char         aClientRandom[32],
                                              const unsigned char         aServerRandom[32],
                                              mbedtls_tls_prf_types       aTlsPrfType)
{
    Crypto::Sha256::Hash kek;
    Crypto::Sha256       sha256;
    unsigned char        keyBlock[kSecureTransportKeyBlockSize];
    unsigned char        randBytes[2 * kSecureTransportRandomBufferSize];

    VerifyOrExit(mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8);
    VerifyOrExit(aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET);

    memcpy(randBytes, aServerRandom, kSecureTransportRandomBufferSize);
    memcpy(randBytes + kSecureTransportRandomBufferSize, aClientRandom, kSecureTransportRandomBufferSize);

    // Retrieve the Key block from Master secret
    mbedtls_ssl_tls_prf(aTlsPrfType, aMasterSecret, aMasterSecretLen, "key expansion", randBytes, sizeof(randBytes),
                        keyBlock, sizeof(keyBlock));

    sha256.Start();
    sha256.Update(keyBlock, kSecureTransportKeyBlockSize);
    sha256.Finish(kek);

    LogDebg("Generated KEK");
    Get<KeyManager>().SetKek(kek.GetBytes());

exit:
    return;
}

#else

int SecureTransport::HandleMbedtlsExportKeys(void                *aContext,
                                             const unsigned char *aMasterSecret,
                                             const unsigned char *aKeyBlock,
                                             size_t               aMacLength,
                                             size_t               aKeyLength,
                                             size_t               aIvLength)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsExportKeys(aMasterSecret, aKeyBlock, aMacLength,
                                                                             aKeyLength, aIvLength);
}

int SecureTransport::HandleMbedtlsExportKeys(const unsigned char *aMasterSecret,
                                             const unsigned char *aKeyBlock,
                                             size_t               aMacLength,
                                             size_t               aKeyLength,
                                             size_t               aIvLength)
{
    OT_UNUSED_VARIABLE(aMasterSecret);

    Crypto::Sha256::Hash kek;
    Crypto::Sha256       sha256;

    VerifyOrExit(mCipherSuites[0] == MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8);

    sha256.Start();
    sha256.Update(aKeyBlock, 2 * static_cast<uint16_t>(aMacLength + aKeyLength + aIvLength));
    sha256.Finish(kek);

    LogDebg("Generated KEK");
    Get<KeyManager>().SetKek(kek.GetBytes());

exit:
    return 0;
}

#endif // (MBEDTLS_VERSION_NUMBER >= 0x03000000)

void SecureTransport::HandleTimer(Timer &aTimer)
{
    static_cast<SecureTransport *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void SecureTransport::HandleTimer(void)
{
    if (IsStateConnectingOrConnected())
    {
        Process();
    }
    else if (IsStateCloseNotify())
    {
        if ((mMaxConnectionAttempts > 0) && (mRemainingConnectionAttempts == 0))
        {
            Close();
            mConnectEvent = kDisconnectedMaxAttempts;
            mAutoCloseCallback.InvokeIfSet();
        }
        else
        {
            SetState(kStateOpen);
            mTimer.Stop();
        }
        mConnectedCallback.InvokeIfSet(mConnectEvent);
    }
}

void SecureTransport::Process(void)
{
    uint8_t      buf[OPENTHREAD_CONFIG_DTLS_MAX_CONTENT_LEN];
    bool         shouldDisconnect = false;
    int          rval;
    ConnectEvent event;

    while (IsStateConnectingOrConnected())
    {
        if (IsStateConnecting())
        {
            rval = mbedtls_ssl_handshake(&mSsl);

            if (mSsl.MBEDTLS_PRIVATE(state) == MBEDTLS_SSL_HANDSHAKE_OVER)
            {
                SetState(kStateConnected);
                mConnectEvent = kConnected;
                mConnectedCallback.InvokeIfSet(mConnectEvent);
            }
        }
        else
        {
            rval = mbedtls_ssl_read(&mSsl, buf, sizeof(buf));
        }

        if (rval > 0)
        {
            mReceiveCallback.InvokeIfSet(buf, static_cast<uint16_t>(rval));
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
                event = kDisconnectedPeerClosed;
                ExitNow(shouldDisconnect = true);
                OT_UNREACHABLE_CODE(break);

            case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
                break;

            case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
                mbedtls_ssl_close_notify(&mSsl);
                event = kDisconnectedError;
                ExitNow(shouldDisconnect = true);
                OT_UNREACHABLE_CODE(break);

            case MBEDTLS_ERR_SSL_INVALID_MAC:
                if (mSsl.MBEDTLS_PRIVATE(state) != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC);
                    event = kDisconnectedError;
                    ExitNow(shouldDisconnect = true);
                }

                break;

            default:
                if (mSsl.MBEDTLS_PRIVATE(state) != MBEDTLS_SSL_HANDSHAKE_OVER)
                {
                    mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                                   MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE);
                    event = kDisconnectedError;
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
        Disconnect(event);
    }
}

void SecureTransport::HandleMbedtlsDebug(void *aContext, int aLevel, const char *aFile, int aLine, const char *aStr)
{
    static_cast<SecureTransport *>(aContext)->HandleMbedtlsDebug(aLevel, aFile, aLine, aStr);
}

void SecureTransport::HandleMbedtlsDebug(int aLevel, const char *aFile, int aLine, const char *aStr)
{
    OT_UNUSED_VARIABLE(aStr);
    OT_UNUSED_VARIABLE(aFile);
    OT_UNUSED_VARIABLE(aLine);

    switch (aLevel)
    {
    case 1:
        LogCrit("[%u] %s", mSocket.GetSockName().mPort, aStr);
        break;

    case 2:
        LogWarn("[%u] %s", mSocket.GetSockName().mPort, aStr);
        break;

    case 3:
        LogInfo("[%u] %s", mSocket.GetSockName().mPort, aStr);
        break;

    case 4:
    default:
        LogDebg("[%u] %s", mSocket.GetSockName().mPort, aStr);
        break;
    }
}

Error SecureTransport::HandleSecureTransportSend(const uint8_t   *aBuf,
                                                 uint16_t         aLength,
                                                 Message::SubType aMessageSubType)
{
    Error        error   = kErrorNone;
    ot::Message *message = nullptr;

    VerifyOrExit((message = mSocket.NewMessage()) != nullptr, error = kErrorNoBufs);
    message->SetSubType(aMessageSubType);
    message->SetLinkSecurityEnabled(mLayerTwoSecurity);

    SuccessOrExit(error = message->AppendBytes(aBuf, aLength));

    // Set message sub type in case Joiner Finalize Response is appended to the message.
    if (aMessageSubType != Message::kSubTypeNone)
    {
        message->SetSubType(aMessageSubType);
    }

    if (mTransportCallback.IsSet())
    {
        SuccessOrExit(error = mTransportCallback.Invoke(*message, mMessageInfo));
    }
    else
    {
        SuccessOrExit(error = mSocket.SendTo(*message, mMessageInfo));
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *SecureTransport::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Closed",       // (0) kStateClosed
        "Open",         // (1) kStateOpen
        "Initializing", // (2) kStateInitializing
        "Connecting",   // (3) kStateConnecting
        "Connected",    // (4) kStateConnected
        "CloseNotify",  // (5) kStateCloseNotify
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateClosed);
        ValidateNextEnum(kStateOpen);
        ValidateNextEnum(kStateInitializing);
        ValidateNextEnum(kStateConnecting);
        ValidateNextEnum(kStateConnected);
        ValidateNextEnum(kStateCloseNotify);
    };

    return kStateStrings[aState];
}

#endif

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
