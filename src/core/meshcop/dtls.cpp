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

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <common/timer.hpp>
#include <crypto/sha256.hpp>
#include <meshcop/dtls.hpp>
#include <thread/thread_netif.hpp>

#include <mbedtls/debug.h>

namespace Thread {
namespace MeshCoP {

Dtls::Dtls(ThreadNetif &aNetif):
    mPskLength(0),
    mStarted(false),
    mTimer(aNetif.GetIp6().mTimerScheduler, &Dtls::HandleTimer, this),
    mTimerIntermediate(0),
    mTimerSet(false),
    mReceiveMessage(NULL),
    mReceiveOffset(0),
    mReceiveLength(0),
    mConnectedHandler(NULL),
    mReceiveHandler(NULL),
    mSendHandler(NULL),
    mContext(NULL),
    mClient(false),
    mNetif(aNetif)
{
    memset(mPsk, 0, sizeof(mPsk));
    memset(&mEntropy, 0, sizeof(mEntropy));
    memset(&mCtrDrbg, 0, sizeof(mCtrDrbg));
    memset(&mSsl, 0, sizeof(mSsl));
    memset(&mConf, 0, sizeof(mConf));
    memset(&mCookieCtx, 0, sizeof(mCookieCtx));
    mProvisioningUrl.Init();
}

otInstance *Dtls::GetInstance()
{
    return mNetif.GetInstance();
}

ThreadError Dtls::Start(bool aClient, ConnectedHandler aConnectedHandler, ReceiveHandler aReceiveHandler,
                        SendHandler aSendHandler, void *aContext)
{
    static const int ciphersuites[2] = {0xC0FF, 0}; // EC-JPAKE cipher suite
    int rval;

    mConnectedHandler = aConnectedHandler;
    mReceiveHandler = aReceiveHandler;
    mSendHandler = aSendHandler;
    mContext = aContext;
    mClient = aClient;
    mReceiveMessage = NULL;

    mbedtls_ssl_init(&mSsl);
    mbedtls_ssl_config_init(&mConf);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_entropy_init(&mEntropy);

    mbedtls_debug_set_threshold(4);

    // XXX: should set personalization data to hardware address
    rval = mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, &mEntropy, NULL, 0);
    VerifyOrExit(rval == 0, ;);

    rval = mbedtls_ssl_config_defaults(&mConf, mClient ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER,
                                       MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
    VerifyOrExit(rval == 0, ;);

    mbedtls_ssl_conf_rng(&mConf, mbedtls_ctr_drbg_random, &mCtrDrbg);
    mbedtls_ssl_conf_min_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&mConf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_ciphersuites(&mConf, ciphersuites);
    mbedtls_ssl_conf_export_keys_cb(&mConf, HandleMbedtlsExportKeys, this);
    mbedtls_ssl_conf_handshake_timeout(&mConf, 8000, 60000);
    mbedtls_ssl_conf_dbg(&mConf, HandleMbedtlsDebug, this);

    if (!mClient)
    {
        mbedtls_ssl_cookie_init(&mCookieCtx);

        rval = mbedtls_ssl_cookie_setup(&mCookieCtx, mbedtls_ctr_drbg_random, &mCtrDrbg);
        VerifyOrExit(rval == 0, ;);

        mbedtls_ssl_conf_dtls_cookies(&mConf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &mCookieCtx);
    }

    rval = mbedtls_ssl_setup(&mSsl, &mConf);
    VerifyOrExit(rval == 0, ;);

    mbedtls_ssl_set_bio(&mSsl, this, &Dtls::HandleMbedtlsTransmit, HandleMbedtlsReceive, NULL);
    mbedtls_ssl_set_timer_cb(&mSsl, this, &Dtls::HandleMbedtlsSetTimer, HandleMbedtlsGetTimer);

    rval = mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
    VerifyOrExit(rval == 0, ;);

    mStarted = true;
    Process();

    otLogInfoMeshCoP(GetInstance(), "DTLS started");

exit:
    return MapError(rval);
}

ThreadError Dtls::Stop(void)
{
    mbedtls_ssl_close_notify(&mSsl);
    Close();
    return kThreadError_None;
}

void Dtls::Close(void)
{
    VerifyOrExit(mStarted,);

    mStarted = false;
    mbedtls_ssl_free(&mSsl);
    mbedtls_ssl_config_free(&mConf);
    mbedtls_ctr_drbg_free(&mCtrDrbg);
    mbedtls_entropy_free(&mEntropy);
    mbedtls_ssl_cookie_free(&mCookieCtx);

    if (mConnectedHandler != NULL)
    {
        mConnectedHandler(mContext, false);
    }

exit:
    return;
}

bool Dtls::IsStarted(void)
{
    return mStarted;
}

ThreadError Dtls::SetPsk(const uint8_t *aPsk, uint8_t aPskLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aPskLength <= sizeof(mPsk), error = kThreadError_InvalidArgs);

    memcpy(mPsk, aPsk, aPskLength);
    mPskLength = aPskLength;

exit:
    return error;
}

ThreadError Dtls::SetClientId(const uint8_t *aClientId, uint8_t aLength)
{
    int rval = mbedtls_ssl_set_client_transport_id(&mSsl, aClientId, aLength);
    return MapError(rval);
}

bool Dtls::IsConnected(void)
{
    return mSsl.state == MBEDTLS_SSL_HANDSHAKE_OVER;
}

ThreadError Dtls::Send(Message &aMessage, uint16_t aLength)
{
    ThreadError error = kThreadError_None;
    uint8_t buffer[kApplicationDataMaxLength];

    VerifyOrExit(aLength <= kApplicationDataMaxLength, error = kThreadError_NoBufs);

    aMessage.Read(0, aLength, buffer);

    SuccessOrExit(error = MapError(mbedtls_ssl_write(&mSsl, buffer, aLength)));

    aMessage.Free();

exit:
    return error;
}

ThreadError Dtls::Receive(Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    mReceiveMessage = &aMessage;
    mReceiveOffset = aOffset;
    mReceiveLength = aLength;

    Process();

    return kThreadError_None;
}

int Dtls::HandleMbedtlsTransmit(void *aContext, const unsigned char *aBuf, size_t aLength)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsTransmit(aBuf, aLength);
}

int Dtls::HandleMbedtlsTransmit(const unsigned char *aBuf, size_t aLength)
{
    ThreadError error;
    int rval = 0;

    otLogInfoMeshCoP(GetInstance(), "Dtls::HandleMbedtlsTransmit");

    error = mSendHandler(mContext, aBuf, (uint16_t)aLength);

    switch (error)
    {
    case kThreadError_None:
        rval = static_cast<int>(aLength);
        break;

    case kThreadError_NoBufs:
        rval = MBEDTLS_ERR_SSL_WANT_WRITE;
        break;

    default:
        assert(false);
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

    otLogInfoMeshCoP(GetInstance(), "Dtls::HandleMbedtlsReceive");

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

    otLogInfoMeshCoP(GetInstance(), "Dtls::HandleMbedtlsGetTimer");

    if (!mTimerSet)
    {
        rval = -1;
    }
    else if (!mTimer.IsRunning())
    {
        rval = 2;
    }
    else if (static_cast<int32_t>(mTimerIntermediate - Timer::GetNow()) <= 0)
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
    otLogInfoMeshCoP(GetInstance(), "Dtls::SetTimer");

    if (aFinish == 0)
    {
        mTimerSet = false;
        mTimer.Stop();
    }
    else
    {
        mTimerSet = true;
        mTimer.Start(aFinish);
        mTimerIntermediate = Timer::GetNow() + aIntermediate;
    }
}

int Dtls::HandleMbedtlsExportKeys(void *aContext, const unsigned char *aMasterSecret, const unsigned char *aKeyBlock,
                                  size_t aMacLength, size_t aKeyLength, size_t aIvLength)
{
    return static_cast<Dtls *>(aContext)->HandleMbedtlsExportKeys(aMasterSecret, aKeyBlock,
                                                                  aMacLength, aKeyLength, aIvLength);
}

int Dtls::HandleMbedtlsExportKeys(const unsigned char *aMasterSecret, const unsigned char *aKeyBlock,
                                  size_t aMacLength, size_t aKeyLength, size_t aIvLength)
{
    uint8_t kek[Crypto::Sha256::kHashSize];
    Crypto::Sha256 sha256;

    sha256.Start();
    sha256.Update(aKeyBlock, 2 * static_cast<uint16_t>(aMacLength + aKeyLength + aIvLength));
    sha256.Finish(kek);

    mNetif.GetKeyManager().SetKek(kek);

    otLogInfoMeshCoP(GetInstance(), "Generated KEK");

    (void)aMasterSecret;
    return 0;
}

void Dtls::HandleTimer(void *aContext)
{
    static_cast<Dtls *>(aContext)->HandleTimer();
}

void Dtls::HandleTimer(void)
{
    Process();
}

void Dtls::Process(void)
{
    uint8_t buf[MBEDTLS_SSL_MAX_CONTENT_LEN];
    bool shouldClose = false;
    int rval;

    while (mStarted)
    {
        if (mSsl.state != MBEDTLS_SSL_HANDSHAKE_OVER)
        {
            rval = mbedtls_ssl_handshake(&mSsl);

            if ((mSsl.state == MBEDTLS_SSL_HANDSHAKE_OVER) && (mConnectedHandler != NULL))
            {
                mConnectedHandler(mContext, true);
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
            mbedtls_ssl_set_hs_ecjpake_password(&mSsl, mPsk, mPskLength);
            break;
        }
    }

exit:

    if (shouldClose)
    {
        Close();
    }
}

ThreadError Dtls::MapError(int rval)
{
    ThreadError error = kThreadError_None;

    switch (rval)
    {
    case MBEDTLS_ERR_SSL_BAD_INPUT_DATA:
        error = kThreadError_InvalidArgs;
        break;

    case MBEDTLS_ERR_SSL_ALLOC_FAILED:
        error = kThreadError_NoBufs;
        break;

    default:
        assert(rval >= 0);
        break;
    }

    return error;
}

void Dtls::HandleMbedtlsDebug(void *ctx, int level, const char *, int , const char *str)
{
    Dtls *pThis = static_cast<Dtls *>(ctx);
    (void)pThis;

    switch (level)
    {
    case 1:
        otLogCritMbedTls(pThis->GetInstance(), "%s", str);
        break;

    case 2:
        otLogWarnMbedTls(pThis->GetInstance(), "%s", str);
        break;

    case 3:
        otLogInfoMbedTls(pThis->GetInstance(), "%s", str);
        break;

    case 4:
    default:
        otLogDebgMbedTls(pThis->GetInstance(), "%s", str);
        break;
    }
}

}  // namespace MeshCoP
}  // namespace Thread
