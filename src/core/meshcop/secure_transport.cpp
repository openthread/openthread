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
const uint16_t SecureTransport::kGroups[] = {MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1, MBEDTLS_SSL_IANA_TLS_GROUP_NONE};
#else
const mbedtls_ecp_group_id SecureTransport::kCurves[] = {MBEDTLS_ECP_DP_SECP256R1, MBEDTLS_ECP_DP_NONE};
#endif

#if defined(MBEDTLS_KEY_EXCHANGE__WITH_CERT__ENABLED) || defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
const uint16_t SecureTransport::kSignatures[] = {MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256, MBEDTLS_TLS1_3_SIG_NONE};
#else
const int SecureTransport::kHashes[] = {MBEDTLS_MD_SHA256, MBEDTLS_MD_NONE};
#endif
#endif

const int SecureTransport::kCipherSuites[][2] = {
    /* kEcjpakeWithAes128Ccm8         */ {MBEDTLS_TLS_ECJPAKE_WITH_AES_128_CCM_8, 0},
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
    /* kPskWithAes128Ccm8             */ {MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8, 0},
#endif
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    /* kEcdheEcdsaWithAes128Ccm8      */ {MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8, 0},
    /* kEcdheEcdsaWithAes128GcmSha256 */ {MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0},
#endif
};

//---------------------------------------------------------------------------------------------------------------------
// SecureTransport

SecureTransport::SecureTransport(Instance &aInstance, LinkSecurityMode aLayerTwoSecurity, bool aDatagramTransport)
    : InstanceLocator(aInstance)
    , mLayerTwoSecurity(aLayerTwoSecurity)
    , mDatagramTransport(aDatagramTransport)
    , mIsOpen(false)
    , mIsServer(true)
    , mTimerSet(false)
    , mVerifyPeerCertificate(true)
    , mSessionState(kSessionDisconnected)
    , mCipherSuite(kUnspecifiedCipherSuite)
    , mMessageSubType(Message::kSubTypeNone)
    , mConnectEvent(kDisconnectedError)
    , mPskLength(0)
    , mMaxConnectionAttempts(0)
    , mRemainingConnectionAttempts(0)
    , mReceiveMessage(nullptr)
    , mSocket(aInstance, *this)
    , mTimer(aInstance, SecureTransport::HandleTimer, this)
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    , mExtension(nullptr)
#endif
{
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
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if (mExtension != nullptr)
    {
        mExtension->mEcdheEcdsaInfo.Free();
    }
#endif
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSsl);
}

void SecureTransport::SetSessionState(SessionState aSessionState)
{
    VerifyOrExit(mSessionState != aSessionState);

    LogInfo("State: %s -> %s", SessionStateToString(mSessionState), SessionStateToString(aSessionState));
    mSessionState = aSessionState;

exit:
    return;
}

Error SecureTransport::Open(ReceiveHandler aReceiveHandler, ConnectedHandler aConnectedHandler, void *aContext)
{
    Error error;

    VerifyOrExit(!mIsOpen, error = kErrorAlready);

    SuccessOrExit(error = mSocket.Open(Ip6::kNetifUnspecified));

    mIsOpen = true;
    mConnectedCallback.Set(aConnectedHandler, aContext);
    mReceiveCallback.Set(aReceiveHandler, aContext);

    mRemainingConnectionAttempts = mMaxConnectionAttempts;

    SetSessionState(kSessionDisconnected);

exit:
    return error;
}

Error SecureTransport::SetMaxConnectionAttempts(uint16_t aMaxAttempts, AutoCloseCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(!mIsOpen, error = kErrorInvalidState);

    mMaxConnectionAttempts = aMaxAttempts;
    mAutoCloseCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error SecureTransport::Connect(const Ip6::SockAddr &aSockAddr)
{
    Error error;

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(IsSessionDisconnected(), error = kErrorInvalidState);

    if (mRemainingConnectionAttempts > 0)
    {
        mRemainingConnectionAttempts--;
    }

    mMessageInfo.SetPeerAddr(aSockAddr.GetAddress());
    mMessageInfo.SetPeerPort(aSockAddr.mPort);

    mIsServer = false;

    error = Setup();

exit:
    return error;
}

void SecureTransport::HandleReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mIsOpen);

    if (IsSessionDisconnected())
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

        SuccessOrExit(Setup());
    }
    else
    {
        // Once DTLS session is started, communicate only with a single peer.
        VerifyOrExit(mMessageInfo.HasSamePeerAddrAndPort(aMessageInfo));
    }

#ifdef MBEDTLS_SSL_SRV_C
    if (IsSessionConnecting())
    {
        mbedtls_ssl_set_client_transport_id(&mSsl, mMessageInfo.GetPeerAddr().GetBytes(), sizeof(Ip6::Address));
    }
#endif

    mReceiveMessage = &aMessage;
    Process();
    mReceiveMessage = nullptr;

exit:
    return;
}

Error SecureTransport::Bind(uint16_t aPort)
{
    Error error;

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(IsSessionDisconnected(), error = kErrorInvalidState);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    SuccessOrExit(error = mSocket.Bind(aPort));
    mIsServer = true;

exit:
    return error;
}

Error SecureTransport::Bind(TransportCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(IsSessionDisconnected(), error = kErrorInvalidState);
    VerifyOrExit(!mSocket.IsBound(), error = kErrorAlready);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    mTransportCallback.Set(aCallback, aContext);
    mIsServer = true;

exit:
    return error;
}

Error SecureTransport::Setup(void)
{
    Error error = kErrorNone;
    int   rval  = 0;

    OT_ASSERT(mCipherSuite != kUnspecifiedCipherSuite);

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(IsSessionDisconnected(), error = kErrorBusy);

    SetSessionState(kSessionInitializing);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the mbedtls_ssl_config `mConf`.

    mbedtls_ssl_config_init(&mConf);

    rval = mbedtls_ssl_config_defaults(
        &mConf, mIsServer ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
        mDatagramTransport ? MBEDTLS_SSL_TRANSPORT_DATAGRAM : MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    VerifyOrExit(rval == 0);

#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if (mVerifyPeerCertificate &&
        (mCipherSuite == kEcdheEcdsaWithAes128Ccm8 || mCipherSuite == kEcdheEcdsaWithAes128GcmSha256))
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

    {
        // We use `kCipherSuites[mCipherSuite]` to look up the cipher
        // suites array to pass to `mbedtls_ssl_conf_ciphersuites()`
        // associated with `mCipherSuite`. We validate that the `enum`
        // values are correct and match the order in the `kCipherSuites[]`
        // array.

        struct EnumCheck
        {
            InitEnumValidatorCounter();
            ValidateNextEnum(kEcjpakeWithAes128Ccm8);
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
            ValidateNextEnum(kPskWithAes128Ccm8);
#endif
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
            ValidateNextEnum(kEcdheEcdsaWithAes128Ccm8);
            ValidateNextEnum(kEcdheEcdsaWithAes128GcmSha256);
#endif
        };

        mbedtls_ssl_conf_ciphersuites(&mConf, kCipherSuites[mCipherSuite]);
    }

    if (mCipherSuite == kEcjpakeWithAes128Ccm8)
    {
#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
        mbedtls_ssl_conf_groups(&mConf, kGroups);
#else
        mbedtls_ssl_conf_curves(&mConf, kCurves);
#endif
#if defined(MBEDTLS_KEY_EXCHANGE__WITH_CERT__ENABLED) || defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
        mbedtls_ssl_conf_sig_algs(&mConf, kSignatures);
#else
        mbedtls_ssl_conf_sig_hashes(&mConf, kHashes);
#endif
#endif
    }

#if (MBEDTLS_VERSION_NUMBER < 0x03000000)
    mbedtls_ssl_conf_export_keys_cb(&mConf, HandleMbedtlsExportKeys, this);
#endif

    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, HandleMbedtlsDebug, this);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the `Extension` components.

#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if (mExtension != nullptr)
    {
        mExtension->mEcdheEcdsaInfo.Init();
    }
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the mbedtls_ssl_cookie_ctx `mCookieCtx`.

#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (mDatagramTransport)
    {
        mbedtls_ssl_cookie_init(&mCookieCtx);

        if (mIsServer)
        {
            rval = mbedtls_ssl_cookie_setup(&mCookieCtx, Crypto::MbedTls::CryptoSecurePrng, nullptr);
            VerifyOrExit(rval == 0);

            mbedtls_ssl_conf_dtls_cookies(&mConf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &mCookieCtx);
        }
    }
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the mbedtls_ssl_context `mSsl`.

    mbedtls_ssl_init(&mSsl);

    rval = mbedtls_ssl_setup(&mSsl, &mConf);
    VerifyOrExit(rval == 0);

    mbedtls_ssl_set_bio(&mSsl, this, HandleMbedtlsTransmit, HandleMbedtlsReceive, /* RecvTimeoutFn */ nullptr);

    if (mDatagramTransport)
    {
        mbedtls_ssl_set_timer_cb(&mSsl, this, HandleMbedtlsSetTimer, HandleMbedtlsGetTimer);
    }

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_ssl_set_export_keys_cb(&mSsl, HandleMbedtlsExportKeys, this);
#endif

    if (mCipherSuite == kEcjpakeWithAes128Ccm8)
    {
        rval = mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
    }
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    else if (mExtension != nullptr)
    {
        rval = mExtension->SetApplicationSecureKeys();
    }
#endif
    VerifyOrExit(rval == 0);

    mReceiveMessage = nullptr;
    mMessageSubType = Message::kSubTypeNone;

    SetSessionState(kSessionConnecting);

    Process();

exit:
    if (mIsOpen && IsSessionInitializing())
    {
        error = Crypto::MbedTls::MapError(rval);

        if ((mMaxConnectionAttempts > 0) && (mRemainingConnectionAttempts == 0))
        {
            Close();
            mAutoCloseCallback.InvokeIfSet();
        }
        else
        {
            SetSessionState(kSessionDisconnected);
            FreeMbedtls();
        }
    }

    return error;
}

void SecureTransport::Close(void)
{
    VerifyOrExit(mIsOpen);

    Disconnect(kDisconnectedLocalClosed);
    SetSessionState(kSessionDisconnected);

    mIsOpen   = false;
    mTimerSet = false;
    mTransportCallback.Clear();
    IgnoreError(mSocket.Close());
    mTimer.Stop();

exit:
    return;
}

void SecureTransport::Disconnect(ConnectEvent aEvent)
{
    VerifyOrExit(mIsOpen);
    VerifyOrExit(IsSessionConnectingOrConnected());

    mbedtls_ssl_close_notify(&mSsl);
    SetSessionState(kSessionDisconnecting);
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
    mPskLength   = aPskLength;
    mCipherSuite = kEcjpakeWithAes128Ccm8;

exit:
    return error;
}

Error SecureTransport::Send(Message &aMessage)
{
    Error    error  = kErrorNone;
    uint16_t length = aMessage.GetLength();
    uint8_t  buffer[kApplicationDataMaxLength];

    VerifyOrExit(length <= sizeof(buffer), error = kErrorNoBufs);

    mMessageSubType = aMessage.GetSubType();
    aMessage.ReadBytes(0, buffer, length);

    SuccessOrExit(error = Crypto::MbedTls::MapError(mbedtls_ssl_write(&mSsl, buffer, length)));

    aMessage.Free();

exit:
    return error;
}

bool SecureTransport::IsMbedtlsHandshakeOver(mbedtls_ssl_context *aSslContext)
{
    return
#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        mbedtls_ssl_is_handshake_over(aSslContext);
#else
        (aSslContext->MBEDTLS_PRIVATE(state) == MBEDTLS_SSL_HANDSHAKE_OVER);
#endif
}

int SecureTransport::HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsTransmit(aBuf, aLength);
}

int SecureTransport::HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength)
{
    Error    error   = kErrorNone;
    Message *message = mSocket.NewMessage();
    int      rval;

    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    message->SetSubType(mMessageSubType);
    message->SetLinkSecurityEnabled(mLayerTwoSecurity);

    SuccessOrExit(error = message->AppendBytes(aBuf, static_cast<uint16_t>(aLength)));

    if (mTransportCallback.IsSet())
    {
        error = mTransportCallback.Invoke(*message, mMessageInfo);
    }
    else
    {
        error = mSocket.SendTo(*message, mMessageInfo);
    }

exit:
    FreeMessageOnError(message, error);
    mMessageSubType = Message::kSubTypeNone;

    switch (error)
    {
    case kErrorNone:
        rval = static_cast<int>(aLength);
        break;

    case kErrorNoBufs:
        rval = MBEDTLS_ERR_SSL_WANT_WRITE;
        break;

    default:
        LogWarnOnError(error, "HandleMbedtlsTransmit");
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
    int      rval = MBEDTLS_ERR_SSL_WANT_READ;
    uint16_t readLength;

    VerifyOrExit(mReceiveMessage != nullptr);

    readLength = mReceiveMessage->ReadBytes(mReceiveMessage->GetOffset(), aBuf, static_cast<uint16_t>(aLength));
    VerifyOrExit(readLength > 0);

    mReceiveMessage->MoveOffset(readLength);
    rval = static_cast<int>(readLength);

exit:
    return rval;
}

int SecureTransport::HandleMbedtlsGetTimer(void *aContext)
{
    return static_cast<SecureTransport *>(aContext)->HandleMbedtlsGetTimer();
}

int SecureTransport::HandleMbedtlsGetTimer(void)
{
    int rval = 0;

    // `mbedtls_ssl_get_timer_t` return values:
    //   -1 if cancelled
    //    0 if none of the delays have passed,
    //    1 if only the intermediate delay has passed,
    //    2 if the final delay has passed.

    if (!mTimerSet)
    {
        rval = -1;
    }
    else
    {
        TimeMilli now = TimerMilli::GetNow();

        if (now >= mTimerFinish)
        {
            rval = 2;
        }
        else if (now >= mTimerIntermediate)
        {
            rval = 1;
        }
    }

    return rval;
}

void SecureTransport::HandleMbedtlsSetTimer(void *aContext, uint32_t aIntermediate, uint32_t aFinish)
{
    static_cast<SecureTransport *>(aContext)->HandleMbedtlsSetTimer(aIntermediate, aFinish);
}

void SecureTransport::HandleMbedtlsSetTimer(uint32_t aIntermediate, uint32_t aFinish)
{
    if (aFinish == 0)
    {
        mTimerSet = false;
        mTimer.Stop();
    }
    else
    {
        TimeMilli now = TimerMilli::GetNow();

        mTimerSet          = true;
        mTimerIntermediate = now + aIntermediate;
        mTimerFinish       = now + aFinish;

        mTimer.FireAt(mTimerFinish);
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

    VerifyOrExit(mCipherSuite == kEcjpakeWithAes128Ccm8);
    VerifyOrExit(aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET);

    memcpy(randBytes, aServerRandom, kSecureTransportRandomBufferSize);
    memcpy(randBytes + kSecureTransportRandomBufferSize, aClientRandom, kSecureTransportRandomBufferSize);

    // Retrieve the Key block from Master secret
    mbedtls_ssl_tls_prf(aTlsPrfType, aMasterSecret, aMasterSecretLen, "key expansion", randBytes, sizeof(randBytes),
                        keyBlock, sizeof(keyBlock));

    sha256.Start();
    sha256.Update(keyBlock, kSecureTransportKeyBlockSize);
    sha256.Finish(kek);

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

    VerifyOrExit(mCipherSuite == kEcjpakeWithAes128Ccm8);

    sha256.Start();
    sha256.Update(aKeyBlock, 2 * static_cast<uint16_t>(aMacLength + aKeyLength + aIvLength));
    sha256.Finish(kek);

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
    VerifyOrExit(mIsOpen);

    if (IsSessionConnectingOrConnected())
    {
        Process();
        ExitNow();
    }

    if (IsSessionDisconnecting())
    {
        if ((mMaxConnectionAttempts > 0) && (mRemainingConnectionAttempts == 0))
        {
            Close();
            mConnectEvent = kDisconnectedMaxAttempts;
            mAutoCloseCallback.InvokeIfSet();
        }
        else
        {
            SetSessionState(kSessionDisconnected);
            mTimer.Stop();
        }

        mConnectedCallback.InvokeIfSet(mConnectEvent);
    }

exit:
    return;
}

void SecureTransport::Process(void)
{
    uint8_t      buf[kMaxContentLen];
    int          rval;
    ConnectEvent disconnectEvent;
    bool         shouldReset;

    while (IsSessionConnectingOrConnected())
    {
        if (IsSessionConnecting())
        {
            rval = mbedtls_ssl_handshake(&mSsl);

            if (IsMbedtlsHandshakeOver(&mSsl))
            {
                SetSessionState(kSessionConnected);
                mConnectEvent = kConnected;
                mConnectedCallback.InvokeIfSet(mConnectEvent);
            }
        }
        else
        {
            rval = mbedtls_ssl_read(&mSsl, buf, sizeof(buf));

            if (rval > 0)
            {
                mReceiveCallback.InvokeIfSet(buf, static_cast<uint16_t>(rval));
                continue;
            }
        }

        // Check `rval` to determine if the connection should be
        // disconnected, reset, or if we should wait.

        disconnectEvent = kConnected;
        shouldReset     = true;

        switch (rval)
        {
        case 0:
        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            shouldReset = false;
            break;

        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
            disconnectEvent = kDisconnectedPeerClosed;
            break;

        case MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED:
            break;

        case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE:
            disconnectEvent = kDisconnectedError;
            break;

        case MBEDTLS_ERR_SSL_INVALID_MAC:
            if (!IsMbedtlsHandshakeOver(&mSsl))
            {
                mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                               MBEDTLS_SSL_ALERT_MSG_BAD_RECORD_MAC);
                disconnectEvent = kDisconnectedError;
            }
            break;

        default:
            if (!IsMbedtlsHandshakeOver(&mSsl))
            {
                mbedtls_ssl_send_alert_message(&mSsl, MBEDTLS_SSL_ALERT_LEVEL_FATAL,
                                               MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE);
                disconnectEvent = kDisconnectedError;
            }

            break;
        }

        if (disconnectEvent != kConnected)
        {
            Disconnect(disconnectEvent);
        }
        else if (shouldReset)
        {
            mbedtls_ssl_session_reset(&mSsl);

            if (mCipherSuite == kEcjpakeWithAes128Ccm8)
            {
                mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
            }
        }

        break; // from `while()` loop
    }
}

void SecureTransport::HandleMbedtlsDebug(void *aContext, int aLevel, const char *aFile, int aLine, const char *aStr)
{
    static_cast<SecureTransport *>(aContext)->HandleMbedtlsDebug(aLevel, aFile, aLine, aStr);
}

void SecureTransport::HandleMbedtlsDebug(int aLevel, const char *aFile, int aLine, const char *aStr)
{
    LogLevel logLevel = kLogLevelDebg;

    switch (aLevel)
    {
    case 1:
        logLevel = kLogLevelCrit;
        break;

    case 2:
        logLevel = kLogLevelWarn;
        break;

    case 3:
        logLevel = kLogLevelInfo;
        break;

    case 4:
    default:
        break;
    }

    LogAt(logLevel, "[%u] %s", mSocket.GetSockName().mPort, aStr);

    OT_UNUSED_VARIABLE(aStr);
    OT_UNUSED_VARIABLE(aFile);
    OT_UNUSED_VARIABLE(aLine);
    OT_UNUSED_VARIABLE(logLevel);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *SecureTransport::SessionStateToString(SessionState aState)
{
    static const char *const kSessionStrings[] = {
        "Disconnected",  // (0) kSessionDisconnected
        "Initializing",  // (1) kSessionInitializing
        "Connecting",    // (2) kSessionConnecting
        "Connected",     // (3) kSessionConnected
        "Disconnecting", // (4) kSessionDisconnecting
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kSessionDisconnected);
        ValidateNextEnum(kSessionInitializing);
        ValidateNextEnum(kSessionConnecting);
        ValidateNextEnum(kSessionConnected);
        ValidateNextEnum(kSessionDisconnecting);
    };

    return kSessionStrings[aState];
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// SecureTransport::Extension

#if OPENTHREAD_CONFIG_TLS_API_ENABLE

int SecureTransport::Extension::SetApplicationSecureKeys(void)
{
    int rval = 0;

    switch (mSecureTransport.mCipherSuite)
    {
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case kEcdheEcdsaWithAes128Ccm8:
    case kEcdheEcdsaWithAes128GcmSha256:
        rval = mEcdheEcdsaInfo.SetSecureKeys(mSecureTransport.mConf);
        VerifyOrExit(rval == 0);
        break;
#endif

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    case kPskWithAes128Ccm8:
        rval = mPskInfo.SetSecureKeys(mSecureTransport.mConf);
        VerifyOrExit(rval == 0);
        break;
#endif

    default:
        LogCrit("Application Coap Secure: Not supported cipher.");
        rval = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
        ExitNow();
    }

exit:
    return rval;
}

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

void SecureTransport::Extension::EcdheEcdsaInfo::Init(void)
{
    mbedtls_x509_crt_init(&mCaChain);
    mbedtls_x509_crt_init(&mOwnCert);
    mbedtls_pk_init(&mPrivateKey);
}

void SecureTransport::Extension::EcdheEcdsaInfo::Free(void)
{
    mbedtls_x509_crt_free(&mCaChain);
    mbedtls_x509_crt_free(&mOwnCert);
    mbedtls_pk_free(&mPrivateKey);
}

int SecureTransport::Extension::EcdheEcdsaInfo::SetSecureKeys(mbedtls_ssl_config &aConfig)
{
    int rval = 0;

    if (mCaChainSrc != nullptr)
    {
        rval = mbedtls_x509_crt_parse(&mCaChain, static_cast<const unsigned char *>(mCaChainSrc),
                                      static_cast<size_t>(mCaChainLength));
        VerifyOrExit(rval == 0);
        mbedtls_ssl_conf_ca_chain(&aConfig, &mCaChain, nullptr);
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
        rval = mbedtls_ssl_conf_own_cert(&aConfig, &mOwnCert, &mPrivateKey);
    }

exit:
    return rval;
}

void SecureTransport::Extension::SetCertificate(const uint8_t *aX509Certificate,
                                                uint32_t       aX509CertLength,
                                                const uint8_t *aPrivateKey,
                                                uint32_t       aPrivateKeyLength)
{
    OT_ASSERT(aX509CertLength > 0);
    OT_ASSERT(aX509Certificate != nullptr);

    OT_ASSERT(aPrivateKeyLength > 0);
    OT_ASSERT(aPrivateKey != nullptr);

    mEcdheEcdsaInfo.mOwnCertSrc       = aX509Certificate;
    mEcdheEcdsaInfo.mOwnCertLength    = aX509CertLength;
    mEcdheEcdsaInfo.mPrivateKeySrc    = aPrivateKey;
    mEcdheEcdsaInfo.mPrivateKeyLength = aPrivateKeyLength;

    mSecureTransport.mCipherSuite =
        mSecureTransport.mDatagramTransport ? kEcdheEcdsaWithAes128Ccm8 : kEcdheEcdsaWithAes128GcmSha256;
}

void SecureTransport::Extension::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain,
                                                       uint32_t       aX509CaCertChainLength)
{
    OT_ASSERT(aX509CaCertChainLength > 0);
    OT_ASSERT(aX509CaCertificateChain != nullptr);

    mEcdheEcdsaInfo.mCaChainSrc    = aX509CaCertificateChain;
    mEcdheEcdsaInfo.mCaChainLength = aX509CaCertChainLength;
}

#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

int SecureTransport::Extension::PskInfo::SetSecureKeys(mbedtls_ssl_config &aConfig) const
{
    return mbedtls_ssl_conf_psk(&aConfig, static_cast<const unsigned char *>(mPreSharedKey), mPreSharedKeyLength,
                                static_cast<const unsigned char *>(mPreSharedKeyIdentity), mPreSharedKeyIdLength);
}

void SecureTransport::Extension::SetPreSharedKey(const uint8_t *aPsk,
                                                 uint16_t       aPskLength,
                                                 const uint8_t *aPskIdentity,
                                                 uint16_t       aPskIdLength)
{
    OT_ASSERT(aPsk != nullptr);
    OT_ASSERT(aPskIdentity != nullptr);
    OT_ASSERT(aPskLength > 0);
    OT_ASSERT(aPskIdLength > 0);

    mPskInfo.mPreSharedKey         = aPsk;
    mPskInfo.mPreSharedKeyLength   = aPskLength;
    mPskInfo.mPreSharedKeyIdentity = aPskIdentity;
    mPskInfo.mPreSharedKeyIdLength = aPskIdLength;

    mSecureTransport.mCipherSuite = kPskWithAes128Ccm8;
}

#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
Error SecureTransport::Extension::GetPeerCertificateBase64(unsigned char *aPeerCert,
                                                           size_t        *aCertLength,
                                                           size_t         aCertBufferSize)
{
    Error error = kErrorNone;

    VerifyOrExit(mSecureTransport.IsSessionConnected(), error = kErrorInvalidState);

#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
    VerifyOrExit(
        mbedtls_base64_encode(aPeerCert, aCertBufferSize, aCertLength,
                              mSecureTransport.mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.p,
                              mSecureTransport.mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.len) == 0,
        error = kErrorNoBufs);
#else
    VerifyOrExit(mbedtls_base64_encode(aPeerCert, aCertBufferSize, aCertLength,
                                       mSecureTransport.mSsl.MBEDTLS_PRIVATE(session)
                                           ->MBEDTLS_PRIVATE(peer_cert)
                                           ->MBEDTLS_PRIVATE(raw)
                                           .MBEDTLS_PRIVATE(p),
                                       mSecureTransport.mSsl.MBEDTLS_PRIVATE(session)
                                           ->MBEDTLS_PRIVATE(peer_cert)
                                           ->MBEDTLS_PRIVATE(raw)
                                           .MBEDTLS_PRIVATE(len)) == 0,
                 error = kErrorNoBufs);
#endif

exit:
    return error;
}
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
Error SecureTransport::Extension::GetPeerSubjectAttributeByOid(const char *aOid,
                                                               size_t      aOidLength,
                                                               uint8_t    *aAttributeBuffer,
                                                               size_t     *aAttributeLength,
                                                               int        *aAsn1Type)
{
    Error                          error = kErrorNone;
    const mbedtls_asn1_named_data *data;
    size_t                         length;
    size_t                         attributeBufferSize;
    mbedtls_x509_crt *peerCert = const_cast<mbedtls_x509_crt *>(mbedtls_ssl_get_peer_cert(&mSecureTransport.mSsl));

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

Error SecureTransport::Extension::GetThreadAttributeFromPeerCertificate(int      aThreadOidDescriptor,
                                                                        uint8_t *aAttributeBuffer,
                                                                        size_t  *aAttributeLength)
{
    const mbedtls_x509_crt *cert = mbedtls_ssl_get_peer_cert(&mSecureTransport.mSsl);

    return GetThreadAttributeFromCertificate(cert, aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}

#endif // defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

Error SecureTransport::Extension::GetThreadAttributeFromOwnCertificate(int      aThreadOidDescriptor,
                                                                       uint8_t *aAttributeBuffer,
                                                                       size_t  *aAttributeLength)
{
    const mbedtls_x509_crt *cert = &mEcdheEcdsaInfo.mOwnCert;

    return GetThreadAttributeFromCertificate(cert, aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}

Error SecureTransport::Extension::GetThreadAttributeFromCertificate(const mbedtls_x509_crt *aCert,
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

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
