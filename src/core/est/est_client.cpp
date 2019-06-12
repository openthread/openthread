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

    mStarted = true;
    mVerifyEstServerCertificate = aVerifyPeer;

    error = mCoapSecure.Start(54234);
    VerifyOrExit(error);

exit:

    return error;
}

void Client::Stop(void)
{
    mStarted = false;
}

otError Client::SetCertificate(const uint8_t *aX509Cert, uint32_t aX509Length, const uint8_t *aPrivateKey, uint32_t aPrivateKeyLength)
{
    OT_UNUSED_VARIABLE(aX509Cert);
    OT_UNUSED_VARIABLE(aX509Length);
    OT_UNUSED_VARIABLE(aPrivateKey);
    OT_UNUSED_VARIABLE(aPrivateKeyLength);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength)
{
    OT_UNUSED_VARIABLE(aX509CaCertificateChain);
    OT_UNUSED_VARIABLE(aX509CaCertChainLength);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::Connect(const otSockAddr * aSockAddr, otHandleEstClientConnect aConnectHandler, otHandleEstClientResponse aResponseHandler, void *aContext)
{
    OT_UNUSED_VARIABLE(aSockAddr);
    OT_UNUSED_VARIABLE(aConnectHandler);
    OT_UNUSED_VARIABLE(aResponseHandler);
    OT_UNUSED_VARIABLE(aContext);

    return OT_ERROR_NONE;
}

void Client::Disconnect(void)
{
    return;
}

bool Client::IsConnected(void)
{
    return mIsConnected;
}

otError Client::SimpleEnroll(void *aContext)
{
    mApplicationContext = aContext;
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::SimpleReEnroll(void *aContext)
{
    mApplicationContext = aContext;
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::GetCsrAttributes(void *aContext)
{
    mApplicationContext = aContext;
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::GetServerGeneratedKeys(void *aContext)
{
    mApplicationContext = aContext;
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError Client::GetCaCertificates(void *aContext)
{
    mApplicationContext = aContext;
    return OT_ERROR_NOT_IMPLEMENTED;
}

} // namespace Est
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
