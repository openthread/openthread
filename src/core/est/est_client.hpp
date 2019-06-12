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

#ifndef EST_CLIENT_HPP_
#define EST_CLIENT_HPP_

#include "openthread-core-config.h"

#include <openthread/est_client.h>

#include "common/message.hpp"
#include "common/timer.hpp"
#include "coap/coap_secure.hpp"

/**
 * @file
 *   This file includes definitions for the EST-coaps client.
 */

namespace ot {
namespace Est {

/**
 * This class implements EST client.
 *
 */
class Client : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit Client(Instance &aInstance);

    /**
     * This method starts the EST client.
     *
     * @retval OT_ERROR_NONE     Successfully started the EST client.
     * @retval OT_ERROR_ALREADY  The client is already started.
     */
    otError Start(bool aVerifyPeer);

    void Stop(void);

    otError SetCertificate(const uint8_t *aX509Cert, uint32_t aX509Length, const uint8_t *aPrivateKey, uint32_t aPrivateKeyLength);

    otError SetCaCertificateChain(const uint8_t *aX509CaCertificateChain, uint32_t aX509CaCertChainLength);

    otError Connect(const otSockAddr * aSockAddr, otHandleEstClientConnect aConnectHandler, otHandleEstClientResponse aResponseHandler, void *aContext);

    void Disconnect(void);

    bool IsConnected(void);

    otError SimpleEnroll(void *aContext);

    otError SimpleReEnroll(void *aContext);

    otError GetCsrAttributes(void *aContext);

    otError GetServerGeneratedKeys(void *aContext);

    otError GetCaCertificates(void *aContext);

private:

    bool                      mIsConnected;
    bool                      mStarted;
    bool                      mVerifyEstServerCertificate;
    void *                    mApplicationContext;
    otHandleEstClientConnect  mConnectCallback;
    otHandleEstClientResponse mResponseCallback;
    Coap::CoapSecure          mCoapSecure;

};

} // namespace Est
} // namespace ot

#endif // EST_CLIENT_HPP_
