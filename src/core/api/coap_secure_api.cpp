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

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#include "instance/instance.hpp"

using namespace ot;

otError otCoapSecureStart(otInstance *aInstance, uint16_t aPort)
{
    otError error;

    SuccessOrExit(error = AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().Open());
    error = AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().Bind(aPort);

exit:
    return error;
}

otError otCoapSecureStartWithMaxConnAttempts(otInstance                  *aInstance,
                                             uint16_t                     aPort,
                                             uint16_t                     aMaxAttempts,
                                             otCoapSecureAutoStopCallback aCallback,
                                             void                        *aContext)
{
    Error error = kErrorAlready;

    SuccessOrExit(AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetMaxConnectionAttempts(
        aMaxAttempts, aCallback, aContext));
    error = otCoapSecureStart(aInstance, aPort);

exit:
    return error;
}

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
void otCoapSecureSetCertificate(otInstance    *aInstance,
                                const uint8_t *aX509Cert,
                                uint32_t       aX509Length,
                                const uint8_t *aPrivateKey,
                                uint32_t       aPrivateKeyLength)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetCertificate(aX509Cert, aX509Length, aPrivateKey,
                                                                            aPrivateKeyLength);
}

void otCoapSecureSetCaCertificateChain(otInstance    *aInstance,
                                       const uint8_t *aX509CaCertificateChain,
                                       uint32_t       aX509CaCertChainLength)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetCaCertificateChain(aX509CaCertificateChain,
                                                                                   aX509CaCertChainLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
void otCoapSecureSetPsk(otInstance    *aInstance,
                        const uint8_t *aPsk,
                        uint16_t       aPskLength,
                        const uint8_t *aPskIdentity,
                        uint16_t       aPskIdLength)
{
    AssertPointerIsNotNull(aPsk);
    AssertPointerIsNotNull(aPskIdentity);

    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetPreSharedKey(aPsk, aPskLength, aPskIdentity,
                                                                             aPskIdLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
otError otCoapSecureGetPeerCertificateBase64(otInstance    *aInstance,
                                             unsigned char *aPeerCert,
                                             size_t        *aCertLength,
                                             size_t         aCertBufferSize)
{
    AssertPointerIsNotNull(aPeerCert);

    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().GetPeerCertificateBase64(aPeerCert, aCertLength,
                                                                                             aCertBufferSize);
}
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

void otCoapSecureSetSslAuthMode(otInstance *aInstance, bool aVerifyPeerCertificate)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetSslAuthMode(aVerifyPeerCertificate);
}

otError otCoapSecureConnect(otInstance                     *aInstance,
                            const otSockAddr               *aSockAddr,
                            otHandleCoapSecureClientConnect aHandler,
                            void                           *aContext)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetConnectCallback(aHandler, aContext);

    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().Connect(AsCoreType(aSockAddr));
}

void otCoapSecureDisconnect(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().Disconnect();
}

bool otCoapSecureIsConnected(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().IsConnected();
}

bool otCoapSecureIsConnectionActive(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().IsConnectionActive();
}

bool otCoapSecureIsClosed(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().IsClosed();
}

void otCoapSecureStop(otInstance *aInstance) { AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().Close(); }

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
otError otCoapSecureSendRequestBlockWise(otInstance                 *aInstance,
                                         otMessage                  *aMessage,
                                         otCoapResponseHandler       aHandler,
                                         void                       *aContext,
                                         otCoapBlockwiseTransmitHook aTransmitHook,
                                         otCoapBlockwiseReceiveHook  aReceiveHook)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SendMessage(AsCoapMessage(aMessage), aHandler,
                                                                                aContext, aTransmitHook, aReceiveHook);
}
#endif

otError otCoapSecureSendRequest(otInstance           *aInstance,
                                otMessage            *aMessage,
                                otCoapResponseHandler aHandler,
                                void                 *aContext)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SendMessage(AsCoapMessage(aMessage), aHandler,
                                                                                aContext);
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
void otCoapSecureAddBlockWiseResource(otInstance *aInstance, otCoapBlockwiseResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().AddBlockWiseResource(AsCoreType(aResource));
}

void otCoapSecureRemoveBlockWiseResource(otInstance *aInstance, otCoapBlockwiseResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().RemoveBlockWiseResource(AsCoreType(aResource));
}
#endif

void otCoapSecureAddResource(otInstance *aInstance, otCoapResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().AddResource(AsCoreType(aResource));
}

void otCoapSecureRemoveResource(otInstance *aInstance, otCoapResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().RemoveResource(AsCoreType(aResource));
}

void otCoapSecureSetClientConnectEventCallback(otInstance                     *aInstance,
                                               otHandleCoapSecureClientConnect aHandler,
                                               void                           *aContext)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetConnectCallback(aHandler, aContext);
}

void otCoapSecureSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SetDefaultHandler(aHandler, aContext);
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
otError otCoapSecureSendResponseBlockWise(otInstance                 *aInstance,
                                          otMessage                  *aMessage,
                                          const otMessageInfo        *aMessageInfo,
                                          void                       *aContext,
                                          otCoapBlockwiseTransmitHook aTransmitHook)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SendMessage(AsCoapMessage(aMessage), nullptr,
                                                                                aContext, aTransmitHook);
}
#endif

otError otCoapSecureSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    return AsCoreType(aInstance).Get<Coap::ApplicationCoapSecure>().SendMessage(AsCoapMessage(aMessage));
}

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
