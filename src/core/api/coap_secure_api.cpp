/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the OpenThread CoAP Secure API.
 */

#include "openthread-core-config.h"

#include <openthread/coap_secure.h>
#include <openthread/ip6.h>

#include "coap/coap_message.hpp"
#include "coap/coap_secure.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

using namespace ot;

otError otCoapSecureStart(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().Start(aPort);
}

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
void otCoapSecureSetCertificate(otInstance *   aInstance,
                                const uint8_t *aX509Cert,
                                uint32_t       aX509Length,
                                const uint8_t *aPrivateKey,
                                uint32_t       aPrivateKeyLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aX509Cert != nullptr && aX509Length != 0 && aPrivateKey != nullptr && aPrivateKeyLength != 0);

    instance.GetApplicationCoapSecure().SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
}

void otCoapSecureSetCaCertificateChain(otInstance *   aInstance,
                                       const uint8_t *aX509CaCertificateChain,
                                       uint32_t       aX509CaCertChainLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aX509CaCertificateChain != nullptr && aX509CaCertChainLength != 0);

    instance.GetApplicationCoapSecure().SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
void otCoapSecureSetPsk(otInstance *   aInstance,
                        const uint8_t *aPsk,
                        uint16_t       aPskLength,
                        const uint8_t *aPskIdentity,
                        uint16_t       aPskIdLength)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aPsk != nullptr && aPskLength != 0 && aPskIdentity != nullptr && aPskIdLength != 0);

    instance.GetApplicationCoapSecure().SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_BASE64_C
otError otCoapSecureGetPeerCertificateBase64(otInstance *   aInstance,
                                             unsigned char *aPeerCert,
                                             size_t *       aCertLength,
                                             size_t         aCertBufferSize)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().GetPeerCertificateBase64(aPeerCert, aCertLength, aCertBufferSize);
}
#endif // MBEDTLS_BASE64_C

void otCoapSecureSetSslAuthMode(otInstance *aInstance, bool aVerifyPeerCertificate)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().SetSslAuthMode(aVerifyPeerCertificate);
}

otError otCoapSecureConnect(otInstance *                    aInstance,
                            const otSockAddr *              aSockAddr,
                            otHandleCoapSecureClientConnect aHandler,
                            void *                          aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().Connect(*static_cast<const Ip6::SockAddr *>(aSockAddr), aHandler,
                                                       aContext);
}

void otCoapSecureDisconnect(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().Disconnect();
}

bool otCoapSecureIsConnected(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().IsConnected();
}

bool otCoapSecureIsConnectionActive(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().IsConnectionActive();
}

void otCoapSecureStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().Stop();
}

otError otCoapSecureSendRequest(otInstance *          aInstance,
                                otMessage *           aMessage,
                                otCoapResponseHandler aHandler,
                                void *                aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().SendMessage(*static_cast<Coap::Message *>(aMessage), aHandler, aContext);
}

void otCoapSecureAddResource(otInstance *aInstance, otCoapResource *aResource)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().AddResource(*static_cast<Coap::Resource *>(aResource));
}

void otCoapSecureRemoveResource(otInstance *aInstance, otCoapResource *aResource)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().RemoveResource(*static_cast<Coap::Resource *>(aResource));
}

void otCoapSecureSetClientConnectedCallback(otInstance *                    aInstance,
                                            otHandleCoapSecureClientConnect aHandler,
                                            void *                          aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().SetClientConnectedCallback(aHandler, aContext);
}

void otCoapSecureSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoapSecure().SetDefaultHandler(aHandler, aContext);
}

otError otCoapSecureSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoapSecure().SendMessage(*static_cast<Coap::Message *>(aMessage),
                                                           *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
