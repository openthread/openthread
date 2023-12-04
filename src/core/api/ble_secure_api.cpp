/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the OpenThread BLE Secure API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <openthread/ble_secure.h>
#include <openthread/platform/ble.h>

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "meshcop/tcat_agent.hpp"
#include "radio/ble_secure.hpp"

using namespace ot;

otError otBleSecureStart(otInstance              *aInstance,
                         otHandleBleSecureConnect aConnectHandler,
                         otHandleBleSecureReceive aReceiveHandler,
                         bool                     aTlvMode,
                         void                    *aContext)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().Start(aConnectHandler, aReceiveHandler, aTlvMode, aContext);
}

otError otBleSecureTcatStart(otInstance *aInstance, const otTcatVendorInfo *aVendorInfo, otHandleTcatJoin aHandler)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().TcatStart(AsCoreType(aVendorInfo), aHandler);
}

void otBleSecureStop(otInstance *aInstance) { AsCoreType(aInstance).Get<Ble::BleSecure>().Stop(); }

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
void otBleSecureSetPsk(otInstance    *aInstance,
                       const uint8_t *aPsk,
                       uint16_t       aPskLength,
                       const uint8_t *aPskIdentity,
                       uint16_t       aPskIdLength)
{
    AssertPointerIsNotNull(aPsk);
    AssertPointerIsNotNull(aPskIdentity);
    OT_ASSERT(aPskLength != 0 && aPskIdLength != 0);

    AsCoreType(aInstance).Get<Ble::BleSecure>().SetPreSharedKey(aPsk, aPskLength, aPskIdentity, aPskIdLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
otError otBleSecureGetPeerCertificateBase64(otInstance *aInstance, unsigned char *aPeerCert, size_t *aCertLength)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().GetPeerCertificateBase64(aPeerCert, aCertLength);
}
#endif // defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

#if defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
otError otBleSecureGetPeerSubjectAttributeByOid(otInstance *aInstance,
                                                const char *aOid,
                                                size_t      aOidLength,
                                                uint8_t    *aAttributeBuffer,
                                                size_t     *aAttributeLength,
                                                int        *aAsn1Type)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().GetPeerSubjectAttributeByOid(aOid, aOidLength, aAttributeBuffer,
                                                                                    aAttributeLength, aAsn1Type);
}

otError otBleSecureGetThreadAttributeFromPeerCertificate(otInstance *aInstance,
                                                         int         aThreadOidDescriptor,
                                                         uint8_t    *aAttributeBuffer,
                                                         size_t     *aAttributeLength)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().GetThreadAttributeFromPeerCertificate(
        aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}
#endif // defined(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)

otError otBleSecureGetThreadAttributeFromOwnCertificate(otInstance *aInstance,
                                                        int         aThreadOidDescriptor,
                                                        uint8_t    *aAttributeBuffer,
                                                        size_t     *aAttributeLength)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().GetThreadAttributeFromOwnCertificate(
        aThreadOidDescriptor, aAttributeBuffer, aAttributeLength);
}

void otBleSecureSetSslAuthMode(otInstance *aInstance, bool aVerifyPeerCertificate)
{
    AsCoreType(aInstance).Get<Ble::BleSecure>().SetSslAuthMode(aVerifyPeerCertificate);
}

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
void otBleSecureSetCertificate(otInstance    *aInstance,
                               const uint8_t *aX509Cert,
                               uint32_t       aX509Length,
                               const uint8_t *aPrivateKey,
                               uint32_t       aPrivateKeyLength)
{
    OT_ASSERT(aX509Cert != nullptr && aX509Length != 0 && aPrivateKey != nullptr && aPrivateKeyLength != 0);

    AsCoreType(aInstance).Get<Ble::BleSecure>().SetCertificate(aX509Cert, aX509Length, aPrivateKey, aPrivateKeyLength);
}

void otBleSecureSetCaCertificateChain(otInstance    *aInstance,
                                      const uint8_t *aX509CaCertificateChain,
                                      uint32_t       aX509CaCertChainLength)
{
    OT_ASSERT(aX509CaCertificateChain != nullptr && aX509CaCertChainLength != 0);

    AsCoreType(aInstance).Get<Ble::BleSecure>().SetCaCertificateChain(aX509CaCertificateChain, aX509CaCertChainLength);
}
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

otError otBleSecureConnect(otInstance *aInstance) { return AsCoreType(aInstance).Get<Ble::BleSecure>().Connect(); }

void otBleSecureDisconnect(otInstance *aInstance) { AsCoreType(aInstance).Get<Ble::BleSecure>().Disconnect(); }

bool otBleSecureIsConnectionActive(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().IsConnectionActive();
}

bool otBleSecureIsConnected(otInstance *aInstance) { return AsCoreType(aInstance).Get<Ble::BleSecure>().IsConnected(); }

bool otBleSecureIsTcatEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().IsTcatEnabled();
}

bool otBleSecureIsCommandClassAuthorized(otInstance *aInstance, otTcatCommandClass aCommandClass)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().IsCommandClassAuthorized(
        static_cast<Ble::BleSecure::CommandClass>(aCommandClass));
}

otError otBleSecureSendMessage(otInstance *aInstance, otMessage *aMessage)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().SendMessage(AsCoreType(aMessage));
}

otError otBleSecureSend(otInstance *aInstance, uint8_t *aBuf, uint16_t aLength)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().Send(aBuf, aLength);
}

otError otBleSecureSendApplicationTlv(otInstance *aInstance, uint8_t *aBuf, uint16_t aLength)
{
    return AsCoreType(aInstance).Get<Ble::BleSecure>().SendApplicationTlv(aBuf, aLength);
}

otError otBleSecureFlush(otInstance *aInstance) { return AsCoreType(aInstance).Get<Ble::BleSecure>().Flush(); }

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
