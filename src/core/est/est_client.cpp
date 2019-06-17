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

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include "../common/asn1.hpp"
#include "common/entropy.hpp"
#include "common/random.hpp"
#include "crypto/mbedtls.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sha256.hpp"


/**
 * @file
 *   This file implements the EST client.
 */

namespace ot {
namespace Est {

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
    return mCoapSecure.SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);;
}

otError Client::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    return mCoapSecure.SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}

otError Client::Connect(const Ip6::SockAddr      &aSockAddr,
                        otHandleEstClientConnect  aConnectHandler,
                        otHandleEstClientResponse aResponseHandler,
                        void *                    aContext)
{
    mApplicationContext = aContext;
    mConnectCallback = aConnectHandler;
    mResponseCallback = aResponseHandler;
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

otError Client::SimpleEnroll(void)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

exit:

    return mError;
}

otError Client::SimpleReEnroll(void)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    VerifyOrExit(mIsConnected, mError = OT_ERROR_INVALID_STATE);

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

otError Client::GenerateKeyPair(const uint8_t *aPersonalSeed, uint32_t *aPersonalSeedLength)
{
    otError mError = OT_ERROR_NOT_IMPLEMENTED;

    OT_UNUSED_VARIABLE(aPersonalSeed);
    OT_UNUSED_VARIABLE(aPersonalSeedLength);

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

} // namespace Est
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
