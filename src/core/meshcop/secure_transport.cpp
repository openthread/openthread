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

//---------------------------------------------------------------------------------------------------------------------
// SecureSession

SecureSession::SecureSession(SecureTransport &aTransport)
    : mTransport(aTransport)
{
    Init();
}

void SecureSession::Init(void)
{
    mTimerSet       = false;
    mIsServer       = false;
    mState          = kStateDisconnected;
    mMessageSubType = Message::kSubTypeNone;
    mConnectEvent   = kDisconnectedError;
    mReceiveMessage = nullptr;
    mMessageInfo.Clear();

    MarkAsNotUsed();
    ClearAllBytes(mSsl);
    ClearAllBytes(mConf);
#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    ClearAllBytes(mCookieCtx);
#endif
}

void SecureSession::FreeMbedtls(void)
{
#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (mTransport.mDatagramTransport)
    {
        mbedtls_ssl_cookie_free(&mCookieCtx);
    }
#endif
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if (mTransport.mExtension != nullptr)
    {
        mTransport.mExtension->mEcdheEcdsaInfo.Free();
    }
#endif
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ssl_free(&mSsl);
}

void SecureSession::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("Session state: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

exit:
    return;
}

Error SecureSession::Connect(const Ip6::SockAddr &aSockAddr)
{
    Error error;

    VerifyOrExit(mTransport.mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(!IsSessionInUse(), error = kErrorInvalidState);

    Init();
    mMessageInfo.SetPeerAddr(aSockAddr.GetAddress());
    mMessageInfo.SetPeerPort(aSockAddr.mPort);

    SuccessOrExit(error = Setup());

    mTransport.mSessions.Push(*this);

exit:
    return error;
}

void SecureSession::Accept(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    mMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());
    mMessageInfo.SetPeerPort(aMessageInfo.GetPeerPort());
    mMessageInfo.SetIsHostInterface(aMessageInfo.IsHostInterface());
    mMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
    mMessageInfo.SetSockPort(aMessageInfo.GetSockPort());

    mIsServer = true;

    if (Setup() == kErrorNone)
    {
        HandleTransportReceive(aMessage);
    }
}

void SecureSession::HandleTransportReceive(Message &aMessage)
{
    VerifyOrExit(!IsDisconnected());

#ifdef MBEDTLS_SSL_SRV_C
    if (IsConnecting())
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

Error SecureSession::Setup(void)
{
    Error error = kErrorNone;
    int   rval  = 0;

    OT_ASSERT(mTransport.mCipherSuite != SecureTransport::kUnspecifiedCipherSuite);

    SetState(kStateInitializing);

    if (mTransport.HasNoRemainingConnectionAttempts())
    {
        mConnectEvent = kDisconnectedMaxAttempts;
        error         = kErrorNoBufs;
        ExitNow();
    }

    mTransport.DecremenetRemainingConnectionAttempts();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the mbedtls_ssl_config `mConf`.

    mbedtls_ssl_config_init(&mConf);

    rval = mbedtls_ssl_config_defaults(&mConf, mIsServer ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
                                       mTransport.mDatagramTransport ? MBEDTLS_SSL_TRANSPORT_DATAGRAM
                                                                     : MBEDTLS_SSL_TRANSPORT_STREAM,
                                       MBEDTLS_SSL_PRESET_DEFAULT);
    VerifyOrExit(rval == 0);

#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if (mTransport.mVerifyPeerCertificate &&
        (mTransport.mCipherSuite == SecureTransport::kEcdheEcdsaWithAes128Ccm8 ||
         mTransport.mCipherSuite == SecureTransport::kEcdheEcdsaWithAes128GcmSha256))
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else
    {
        mbedtls_ssl_conf_authmode(&mConf, MBEDTLS_SSL_VERIFY_NONE);
    }
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
            ValidateNextEnum(SecureTransport::kEcjpakeWithAes128Ccm8);
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
            ValidateNextEnum(SecureTransport::kPskWithAes128Ccm8);
#endif
#if OPENTHREAD_CONFIG_TLS_API_ENABLE && defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
            ValidateNextEnum(SecureTransport::kEcdheEcdsaWithAes128Ccm8);
            ValidateNextEnum(SecureTransport::kEcdheEcdsaWithAes128GcmSha256);
#endif
        };

        mbedtls_ssl_conf_ciphersuites(&mConf, SecureTransport::kCipherSuites[mTransport.mCipherSuite]);
    }

    if (mTransport.mCipherSuite == SecureTransport::kEcjpakeWithAes128Ccm8)
    {
#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
        mbedtls_ssl_conf_groups(&mConf, SecureTransport::kGroups);
#else
        mbedtls_ssl_conf_curves(&mConf, SecureTransport::kCurves);
#endif
#if defined(MBEDTLS_KEY_EXCHANGE__WITH_CERT__ENABLED) || defined(MBEDTLS_KEY_EXCHANGE_WITH_CERT_ENABLED)
#if (MBEDTLS_VERSION_NUMBER >= 0x03020000)
        mbedtls_ssl_conf_sig_algs(&mConf, SecureTransport::kSignatures);
#else
        mbedtls_ssl_conf_sig_hashes(&mConf, SecureTransport::kHashes);
#endif
#endif
    }

#if (MBEDTLS_VERSION_NUMBER < 0x03000000)
    mbedtls_ssl_conf_export_keys_cb(&mConf, SecureTransport::HandleMbedtlsExportKeys, &mTransport);
#endif

    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, SecureTransport::HandleMbedtlsDebug, &mTransport);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the `Extension` components.

#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    if (mTransport.mExtension != nullptr)
    {
#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
        mTransport.mExtension->mEcdheEcdsaInfo.Init();
#endif
        rval = mTransport.mExtension->SetApplicationSecureKeys(mConf);
        VerifyOrExit(rval == 0);
    }
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup the mbedtls_ssl_cookie_ctx `mCookieCtx`.

#if defined(MBEDTLS_SSL_SRV_C) && defined(MBEDTLS_SSL_COOKIE_C)
    if (mTransport.mDatagramTransport)
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

    if (mTransport.mDatagramTransport)
    {
        mbedtls_ssl_set_timer_cb(&mSsl, this, HandleMbedtlsSetTimer, HandleMbedtlsGetTimer);
    }

#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_ssl_set_export_keys_cb(&mSsl, SecureTransport::HandleMbedtlsExportKeys, &mTransport);
#endif

    if (mTransport.mCipherSuite == SecureTransport::kEcjpakeWithAes128Ccm8)
    {
        rval = mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mTransport.mPsk, mTransport.mPskLength);
        VerifyOrExit(rval == 0);
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (!mIsServer)
    {
        mbedtls_ssl_set_hostname(&mSsl, nullptr);
    }
#endif

    mReceiveMessage = nullptr;
    mMessageSubType = Message::kSubTypeNone;

    SetState(kStateConnecting);

    Process();

exit:
    if (IsInitializing())
    {
        error = (error == kErrorNone) ? Crypto::MbedTls::MapError(rval) : error;

        SetState(kStateDisconnected);
        FreeMbedtls();
        mTransport.mUpdateTask.Post();
    }

    return error;
}

void SecureSession::Disconnect(ConnectEvent aEvent)
{
    VerifyOrExit(mTransport.mIsOpen);
    VerifyOrExit(IsConnectingOrConnected());

    mbedtls_ssl_close_notify(&mSsl);
    SetState(kStateDisconnecting);
    mConnectEvent = aEvent;

    mTimerSet    = false;
    mTimerFinish = TimerMilli::GetNow() + kGuardTimeNewConnectionMilli;
    mTransport.mTimer.FireAtIfEarlier(mTimerFinish);

    FreeMbedtls();

exit:
    return;
}

Error SecureSession::Send(Message &aMessage)
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

bool SecureSession::IsMbedtlsHandshakeOver(mbedtls_ssl_context *aSslContext)
{
    return
#if (MBEDTLS_VERSION_NUMBER >= 0x03000000)
        mbedtls_ssl_is_handshake_over(aSslContext);
#else
        (aSslContext->MBEDTLS_PRIVATE(state) == MBEDTLS_SSL_HANDSHAKE_OVER);
#endif
}

int SecureSession::HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength)
{
    return static_cast<SecureSession *>(aContext)->HandleMbedtlsTransmit(aBuf, aLength);
}

int SecureSession::HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength)
{
    Message::SubType msgSubType = mMessageSubType;

    mMessageSubType = Message::kSubTypeNone;

    return mTransport.Transmit(aBuf, aLength, mMessageInfo, msgSubType);
}

int SecureSession::HandleMbedtlsReceive(void *aContext, unsigned char *aBuf, size_t aLength)
{
    return static_cast<SecureSession *>(aContext)->HandleMbedtlsReceive(aBuf, aLength);
}

int SecureSession::HandleMbedtlsReceive(unsigned char *aBuf, size_t aLength)
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

int SecureSession::HandleMbedtlsGetTimer(void *aContext)
{
    return static_cast<SecureSession *>(aContext)->HandleMbedtlsGetTimer();
}

int SecureSession::HandleMbedtlsGetTimer(void)
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

void SecureSession::HandleMbedtlsSetTimer(void *aContext, uint32_t aIntermediate, uint32_t aFinish)
{
    static_cast<SecureSession *>(aContext)->HandleMbedtlsSetTimer(aIntermediate, aFinish);
}

void SecureSession::HandleMbedtlsSetTimer(uint32_t aIntermediate, uint32_t aFinish)
{
    if (aFinish == 0)
    {
        mTimerSet = false;
    }
    else
    {
        TimeMilli now = TimerMilli::GetNow();

        mTimerSet          = true;
        mTimerIntermediate = now + aIntermediate;
        mTimerFinish       = now + aFinish;

        mTransport.mTimer.FireAtIfEarlier(mTimerFinish);
    }
}

void SecureSession::HandleTimer(TimeMilli aNow)
{
    if (IsConnectingOrConnected())
    {
        VerifyOrExit(mTimerSet);

        if (aNow < mTimerFinish)
        {
            mTransport.mTimer.FireAtIfEarlier(mTimerFinish);
            ExitNow();
        }

        Process();
        ExitNow();
    }

    if (IsDisconnecting())
    {
        if (aNow < mTimerFinish)
        {
            mTransport.mTimer.FireAtIfEarlier(mTimerFinish);
            ExitNow();
        }

        SetState(kStateDisconnected);
        mTransport.mUpdateTask.Post();
    }

exit:
    return;
}

void SecureSession::Process(void)
{
    uint8_t      buf[kMaxContentLen];
    int          rval;
    ConnectEvent disconnectEvent;
    bool         shouldReset;

    while (IsConnectingOrConnected())
    {
        if (IsConnecting())
        {
            rval = mbedtls_ssl_handshake(&mSsl);

            if (IsMbedtlsHandshakeOver(&mSsl))
            {
                SetState(kStateConnected);
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

            if (mTransport.mCipherSuite == SecureTransport::kEcjpakeWithAes128Ccm8)
            {
                mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mTransport.mPsk, mTransport.mPskLength);
            }
        }

        break; // from `while()` loop
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *SecureSession::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disconnected",  // (0) kStateDisconnected
        "Initializing",  // (1) kStateInitializing
        "Connecting",    // (2) kStateConnecting
        "Connected",     // (3) kStateConnected
        "Disconnecting", // (4) kStateDisconnecting
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateDisconnected);
        ValidateNextEnum(kStateInitializing);
        ValidateNextEnum(kStateConnecting);
        ValidateNextEnum(kStateConnected);
        ValidateNextEnum(kStateDisconnecting);
    };

    return kStateStrings[aState];
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// SecureTransport

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

SecureTransport::SecureTransport(Instance &aInstance, LinkSecurityMode aLayerTwoSecurity, bool aDatagramTransport)
    : mLayerTwoSecurity(aLayerTwoSecurity)
    , mDatagramTransport(aDatagramTransport)
    , mIsOpen(false)
    , mIsClosing(false)
    , mVerifyPeerCertificate(true)
    , mCipherSuite(kUnspecifiedCipherSuite)
    , mPskLength(0)
    , mMaxConnectionAttempts(0)
    , mRemainingConnectionAttempts(0)
    , mSocket(aInstance, *this)
    , mTimer(aInstance, HandleTimer, this)
    , mUpdateTask(aInstance, HandleUpdateTask, this)
#if OPENTHREAD_CONFIG_TLS_API_ENABLE
    , mExtension(nullptr)
#endif
{
    ClearAllBytes(mPsk);
    OT_UNUSED_VARIABLE(mVerifyPeerCertificate);
}

Error SecureTransport::Open(Ip6::NetifIdentifier aNetifIdentifier)
{
    Error error;

    VerifyOrExit(!mIsOpen, error = kErrorAlready);

    SuccessOrExit(error = mSocket.Open(aNetifIdentifier));
    mIsOpen                      = true;
    mRemainingConnectionAttempts = mMaxConnectionAttempts;

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

void SecureTransport::HandleReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    SecureSession *session;

    VerifyOrExit(mIsOpen);

    session = mSessions.FindMatching(aMessageInfo);

    if (session != nullptr)
    {
        session->HandleTransportReceive(aMessage);
        ExitNow();
    }

    // A new connection request

    VerifyOrExit(mAcceptCallback.IsSet());

    session = mAcceptCallback.Invoke(aMessageInfo);
    VerifyOrExit(session != nullptr);

    session->Init();
    mSessions.Push(*session);

    session->Accept(aMessage, aMessageInfo);

exit:
    return;
}

Error SecureTransport::Bind(uint16_t aPort)
{
    Error error;

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    VerifyOrExit(mSessions.IsEmpty(), error = kErrorInvalidState);

    error = mSocket.Bind(aPort);

exit:
    return error;
}

Error SecureTransport::Bind(TransportCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsOpen, error = kErrorInvalidState);
    VerifyOrExit(!mSocket.IsBound(), error = kErrorAlready);
    VerifyOrExit(!mTransportCallback.IsSet(), error = kErrorAlready);

    VerifyOrExit(mSessions.IsEmpty(), error = kErrorInvalidState);

    mTransportCallback.Set(aCallback, aContext);

exit:
    return error;
}

void SecureTransport::Close(void)
{
    VerifyOrExit(mIsOpen);
    VerifyOrExit(!mIsClosing);

    // `mIsClosing` is used to protect against multiple
    // calls to `Close()` and re-entry. As the transport is closed,
    // all existing sessions are disconnected, which can trigger
    // connect and remove callbacks to be invoked. These callbacks
    // may call `Close()` again.

    mIsClosing = true;

    for (SecureSession &session : mSessions)
    {
        session.Disconnect(SecureSession::kDisconnectedLocalClosed);
        session.SetState(SecureSession::kStateDisconnected);
    }

    RemoveDisconnectedSessions();

    mIsOpen    = false;
    mIsClosing = false;
    mTransportCallback.Clear();
    IgnoreError(mSocket.Close());
    mTimer.Stop();

exit:
    return;
}

void SecureTransport::RemoveDisconnectedSessions(void)
{
    LinkedList<SecureSession> disconnectedSessions;
    SecureSession            *session;

    mSessions.RemoveAllMatching(disconnectedSessions, SecureSession::kStateDisconnected);

    while ((session = disconnectedSessions.Pop()) != nullptr)
    {
        session->mConnectedCallback.InvokeIfSet(session->mConnectEvent);
        session->MarkAsNotUsed();
        session->mMessageInfo.Clear();
        mRemoveSessionCallback.InvokeIfSet(*session);
    }
}

void SecureTransport::DecremenetRemainingConnectionAttempts(void)
{
    if (mRemainingConnectionAttempts > 0)
    {
        mRemainingConnectionAttempts--;
    }
}

bool SecureTransport::HasNoRemainingConnectionAttempts(void) const
{
    return (mMaxConnectionAttempts > 0) && (mRemainingConnectionAttempts == 0);
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

void SecureTransport::SetPsk(const JoinerPskd &aPskd)
{
    static_assert(JoinerPskd::kMaxLength <= kPskMaxLength, "The max DTLS PSK length is smaller than joiner PSKd");

    IgnoreError(SetPsk(aPskd.GetBytes(), aPskd.GetLength()));
}

int SecureTransport::Transmit(const unsigned char    *aBuf,
                              size_t                  aLength,
                              const Ip6::MessageInfo &aMessageInfo,
                              Message::SubType        aMessageSubType)
{
    Error    error   = kErrorNone;
    Message *message = mSocket.NewMessage();
    int      rval;

    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    message->SetSubType(aMessageSubType);
    message->SetLinkSecurityEnabled(mLayerTwoSecurity);

    SuccessOrExit(error = message->AppendBytes(aBuf, static_cast<uint16_t>(aLength)));

    if (mTransportCallback.IsSet())
    {
        error = mTransportCallback.Invoke(*message, aMessageInfo);
    }
    else
    {
        error = mSocket.SendTo(*message, aMessageInfo);
    }

exit:
    FreeMessageOnError(message, error);

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

    mTimer.Get<KeyManager>().SetKek(kek.GetBytes());

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

    mTimer.Get<KeyManager>().SetKek(kek.GetBytes());

exit:
    return 0;
}

#endif // (MBEDTLS_VERSION_NUMBER >= 0x03000000)

void SecureTransport::HandleUpdateTask(Tasklet &aTasklet)
{
    static_cast<SecureTransport *>(static_cast<TaskletContext &>(aTasklet).GetContext())->HandleUpdateTask();
}

void SecureTransport::HandleUpdateTask(void)
{
    RemoveDisconnectedSessions();

    if (mSessions.IsEmpty() && HasNoRemainingConnectionAttempts())
    {
        Close();
        mAutoCloseCallback.InvokeIfSet();
    }
}

void SecureTransport::HandleTimer(Timer &aTimer)
{
    static_cast<SecureTransport *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void SecureTransport::HandleTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();

    VerifyOrExit(mIsOpen);

    for (SecureSession &session : mSessions)
    {
        session.HandleTimer(now);
    }

exit:
    return;
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

//---------------------------------------------------------------------------------------------------------------------
// SecureTransport::Extension

#if OPENTHREAD_CONFIG_TLS_API_ENABLE

int SecureTransport::Extension::SetApplicationSecureKeys(mbedtls_ssl_config &aConfig)
{
    int rval = 0;

    switch (mSecureTransport.mCipherSuite)
    {
    case kEcjpakeWithAes128Ccm8:
        // PSK will be set on `mbedtls_ssl_context` when set up.
        break;

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    case kEcdheEcdsaWithAes128Ccm8:
    case kEcdheEcdsaWithAes128GcmSha256:
        rval = mEcdheEcdsaInfo.SetSecureKeys(aConfig);
        VerifyOrExit(rval == 0);
        break;
#endif

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    case kPskWithAes128Ccm8:
        rval = mPskInfo.SetSecureKeys(aConfig);
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
    Error          error   = kErrorNone;
    SecureSession *session = mSecureTransport.mSessions.GetHead();

    VerifyOrExit(session != nullptr, error = kErrorInvalidState);
    VerifyOrExit(session->IsConnected(), error = kErrorInvalidState);

#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
    VerifyOrExit(mbedtls_base64_encode(aPeerCert, aCertBufferSize, aCertLength,
                                       session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.p,
                                       session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.len) ==
                     0,
                 error = kErrorNoBufs);
#else
    VerifyOrExit(
        mbedtls_base64_encode(
            aPeerCert, aCertBufferSize, aCertLength,
            session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(p),
            session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(
                len)) == 0,
        error = kErrorNoBufs);
#endif

exit:
    return error;
}
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
Error SecureTransport::Extension::GetPeerCertificateDer(uint8_t *aPeerCert, size_t *aCertLength, size_t aCertBufferSize)
{
    Error          error   = kErrorNone;
    SecureSession *session = mSecureTransport.mSessions.GetHead();

    VerifyOrExit(session->IsConnected(), error = kErrorInvalidState);

#if (MBEDTLS_VERSION_NUMBER >= 0x03010000)
    VerifyOrExit(session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.len < aCertBufferSize,
                 error = kErrorNoBufs);

    *aCertLength = session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.len;
    memcpy(aPeerCert, session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->raw.p, *aCertLength);

#else
    VerifyOrExit(
        session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(len) <
            aCertBufferSize,
        error = kErrorNoBufs);

    *aCertLength =
        session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(len);
    memcpy(aPeerCert,
           session->mSsl.MBEDTLS_PRIVATE(session)->MBEDTLS_PRIVATE(peer_cert)->MBEDTLS_PRIVATE(raw).MBEDTLS_PRIVATE(p),
           *aCertLength);
#endif

exit:
    return error;
}

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
    SecureSession                 *session;
    mbedtls_x509_crt              *peerCert;

    session = mSecureTransport.mSessions.GetHead();
    VerifyOrExit(session != nullptr, error = kErrorInvalidState);

    peerCert = const_cast<mbedtls_x509_crt *>(mbedtls_ssl_get_peer_cert(&session->mSsl));

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
    Error                   error;
    SecureSession          *session = mSecureTransport.mSessions.GetHead();
    const mbedtls_x509_crt *cert;

    VerifyOrExit(session != nullptr, error = kErrorInvalidState);
    cert  = mbedtls_ssl_get_peer_cert(&session->mSsl);
    error = GetThreadAttributeFromCertificate(cert, aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);

exit:
    return error;
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

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Tls

SecureSession *Tls::HandleAccept(void *aContext, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return static_cast<Tls *>(aContext)->HandleAccept();
}

SecureSession *Tls::HandleAccept(void) { return IsSessionInUse() ? nullptr : static_cast<SecureSession *>(this); }

#endif

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE
