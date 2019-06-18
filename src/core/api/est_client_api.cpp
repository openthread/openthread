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

/**
 * @file
 *   This file implements the OpenThread EST over CoAP Secure Client API.
 */

#include "openthread-core-config.h"

#include <openthread/est_client.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "est/est_client.hpp"

#if OPENTHREAD_ENABLE_EST_CLIENT

using namespace ot;

otError otEstClientStart(otInstance *aInstance, bool aVerifyPeer)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().Start(aVerifyPeer);
}

void otEstClientStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Est::Client>().Stop();
}

otError otEstClientSetCertificate(otInstance *   aInstance,
                                  const uint8_t *aX509Cert,
                                  uint32_t       aX509Length,
                                  const uint8_t *aPrivateKey,
                                  uint32_t       aPrivateKeyLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
}

otError otEstClientSetCaCertificateChain(otInstance *   aInstance,
                                         const uint8_t *aX509CaCertificateChain,
                                         uint32_t       aX509CaCertChainLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}

otError otEstClientConnect(otInstance *              aInstance,
                           const otSockAddr *        aSockAddr,
                           otHandleEstClientConnect  aConnectHandler,
                           otHandleEstClientResponse aResponseHandler,
                           void *                    aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().Connect(*static_cast<const Ip6::SockAddr *>(aSockAddr), aConnectHandler,
                                               aResponseHandler, aContext);
}

void otEstClientDisconnect(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Est::Client>().Disconnect();
}

bool otEstClientIsConnected(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().IsConnected();
}

otError otEstClientSimpleEnroll(otInstance *   aInstance,
                                const uint8_t *aPrivateKey,
                                uint32_t       aPrivateLeyLength,
                                const uint8_t *aPublicKey,
                                uint32_t       aPublicKeyLength,
                                otMdType       aMdType,
                                uint8_t        aKeyUsageFlags,
                                bool           aPemFormat)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().SimpleEnroll(aPrivateKey, aPrivateLeyLength, aPublicKey, aPublicKeyLength,
                                                    aMdType, aKeyUsageFlags, aPemFormat);
}

otError otEstClientSimpleReEnroll(otInstance *   aInstance,
                                  const uint8_t *aPrivateKey,
                                  uint32_t       aPrivateLeyLength,
                                  const uint8_t *aPublicKey,
                                  uint32_t       aPublicKeyLength,
                                  otMdType       aMdType,
                                  uint8_t        aKeyUsageFlags,
                                  bool           aPemFormat)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().SimpleReEnroll(aPrivateKey, aPrivateLeyLength, aPublicKey, aPublicKeyLength,
                                                      aMdType, aKeyUsageFlags, aPemFormat);
}

otError otEstClientGetCsrAttributes(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().GetCsrAttributes();
}

otError otEstClientGetServerGeneratedKeys(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().GetServerGeneratedKeys();
}

otError otEstClientGetCaCertificates(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.Get<Est::Client>().GetCaCertificates();
}

#endif // OPENTHREAD_ENABLE_EST_CLIENT
