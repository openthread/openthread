/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "openthread-core-config.h"

#include "test_platform.h"
#include "test_util.h"

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

#include <openthread/ble_secure.h>

#define OT_TCAT_X509_CERT                                                \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIIB6TCCAZCgAwIBAgICNekwCgYIKoZIzj0EAwIwcTEmMCQGA1UEAwwdVGhyZWFk\n" \
    "IENlcnRpZmljYXRpb24gRGV2aWNlQ0ExGTAXBgNVBAoMEFRocmVhZCBHcm91cCBJ\n" \
    "bmMxEjAQBgNVBAcMCVNhbiBSYW1vbjELMAkGA1UECAwCQ0ExCzAJBgNVBAYTAlVT\n" \
    "MCAXDTI0MDUwNzA5Mzk0NVoYDzI5OTkxMjMxMDkzOTQ1WjA8MSEwHwYDVQQDDBhU\n" \
    "Q0FUIEV4YW1wbGUgRGV2aWNlQ2VydDExFzAVBgNVBAUTDjQ3MjMtOTgzMy0wMDAx\n" \
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE11h/4vKZXVXv+1GDZo066spItloT\n" \
    "dpCi0bux0jvpQSHLdQBIc+40zVCxMDRUvbX//vJKGsSJKOVUlCojQ2wIdqNLMEkw\n" \
    "HwYDVR0jBBgwFoAUX6sbKWiIodS0MaiGYefnZlnt+BkwEAYJKwYBBAGC3yoCBAMC\n" \
    "AQUwFAYJKwYBBAGC3yoDBAcEBSABAQEBMAoGCCqGSM49BAMCA0cAMEQCIHWu+Rd1\n" \
    "VRlzrD8KbuyJcJFTXh2sQ9UIrFIA7+4e/GVcAiAVBdGqTxbt3TGkBBllpafAUB2/\n" \
    "s0GJj7E33oblqy5eHQ==\n"                                             \
    "-----END CERTIFICATE-----\n"

#define OT_TCAT_PRIV_KEY                                                 \
    "-----BEGIN EC PRIVATE KEY-----\n"                                   \
    "MHcCAQEEIIqKM1QTlNaquV74W6Viz/ggXoLqlPOP6LagSyaFO3oUoAoGCCqGSM49\n" \
    "AwEHoUQDQgAE11h/4vKZXVXv+1GDZo066spItloTdpCi0bux0jvpQSHLdQBIc+40\n" \
    "zVCxMDRUvbX//vJKGsSJKOVUlCojQ2wIdg==\n"                             \
    "-----END EC PRIVATE KEY-----\n"

#define OT_TCAT_TRUSTED_ROOT_CERTIFICATE                                 \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIICOzCCAeGgAwIBAgIJAKOc2hehOGoBMAoGCCqGSM49BAMCMHExJjAkBgNVBAMM\n" \
    "HVRocmVhZCBDZXJ0aWZpY2F0aW9uIERldmljZUNBMRkwFwYDVQQKDBBUaHJlYWQg\n" \
    "R3JvdXAgSW5jMRIwEAYDVQQHDAlTYW4gUmFtb24xCzAJBgNVBAgMAkNBMQswCQYD\n" \
    "VQQGEwJVUzAeFw0yNDA1MDMyMDAyMThaFw00NDA0MjgyMDAyMThaMHExJjAkBgNV\n" \
    "BAMMHVRocmVhZCBDZXJ0aWZpY2F0aW9uIERldmljZUNBMRkwFwYDVQQKDBBUaHJl\n" \
    "YWQgR3JvdXAgSW5jMRIwEAYDVQQHDAlTYW4gUmFtb24xCzAJBgNVBAgMAkNBMQsw\n" \
    "CQYDVQQGEwJVUzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGy850VBIPTkN3oL\n" \
    "x++zIUsZk2k26w4fuieFz9oNvjdb5W14+Yf3mvGWsl4NHyLxqhmamVAR4h7zWRlZ\n" \
    "0XyMVpKjYjBgMB4GA1UdEQQXMBWBE3RvbUB0aHJlYWRncm91cC5vcmcwDgYDVR0P\n" \
    "AQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFF+rGyloiKHUtDGo\n" \
    "hmHn52ZZ7fgZMAoGCCqGSM49BAMCA0gAMEUCIQCTq1qjPZs9fAJB6ppTXs588Pnu\n" \
    "eVFOwC8bd//D99KiHAIgU84kwFHIyDvFqu6y+u1hFqBGsiuTmKwZ2PHhVe/xK1k=\n" \
    "-----END CERTIFICATE-----\n"

namespace ot {

class TestBleSecure
{
public:
    TestBleSecure(void)
        : mIsConnected(false)
        , mIsBleConnectionOpen(false)
    {
    }

    void HandleBleSecureConnect(bool aConnected, bool aBleConnectionOpen)
    {
        mIsConnected         = aConnected;
        mIsBleConnectionOpen = aBleConnectionOpen;
    }

    bool IsConnected(void) const { return mIsConnected; }
    bool IsBleConnectionOpen(void) const { return mIsBleConnectionOpen; }

private:
    bool mIsConnected;
    bool mIsBleConnectionOpen;
};

static void HandleBleSecureConnect(otInstance *aInstance, bool aConnected, bool aBleConnectionOpen, void *aContext)
{
    OT_UNUSED_VARIABLE(aInstance);

    static_cast<TestBleSecure *>(aContext)->HandleBleSecureConnect(aConnected, aBleConnectionOpen);
}

void TestTcat(void)
{
    const char         kPskdVendor[]                  = "J01NM3";
    const char         kUrl[]                         = "dummy_url";
    constexpr uint16_t kConnectionId                  = 0;
    const int          kCertificateThreadVersion      = 2;
    const int          kCertificateAuthorizationField = 3;
    const uint8_t      expectedTcatAuthField[5]       = {0x20, 0x01, 0x01, 0x01, 0x01};
    uint8_t            attributeBuffer[8];
    size_t             attributeLen;

    TestBleSecure ble;
    Instance     *instance = testInitInstance();

    otTcatVendorInfo vendorInfo = {.mProvisioningUrl = kUrl, .mPskdString = kPskdVendor};

    otBleSecureSetCertificate(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_X509_CERT), sizeof(OT_TCAT_X509_CERT),
                              reinterpret_cast<const uint8_t *>(OT_TCAT_PRIV_KEY), sizeof(OT_TCAT_PRIV_KEY));

    otBleSecureSetCaCertificateChain(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_TRUSTED_ROOT_CERTIFICATE),
                                     sizeof(OT_TCAT_TRUSTED_ROOT_CERTIFICATE));

    otBleSecureSetSslAuthMode(instance, true);

    // Validate BLE secure and Tcat start APIs
    SuccessOrQuit(otBleSecureSetTcatVendorInfo(instance, &vendorInfo));
    VerifyOrQuit(otBleSecureTcatStart(instance, nullptr) == kErrorInvalidState);
    SuccessOrQuit(otBleSecureStart(instance, HandleBleSecureConnect, nullptr, true, &ble));
    VerifyOrQuit(otBleSecureStart(instance, HandleBleSecureConnect, nullptr, true, nullptr) == kErrorAlready);
    SuccessOrQuit(otBleSecureTcatStart(instance, nullptr));

    // Validate connection callbacks when platform informs that peer has connected/disconnected
    VerifyOrQuit(!otBleSecureIsConnected(instance));
    otPlatBleGapOnConnected(instance, kConnectionId);
    VerifyOrQuit(!ble.IsConnected() && ble.IsBleConnectionOpen());
    otPlatBleGapOnDisconnected(instance, kConnectionId);
    VerifyOrQuit(!ble.IsConnected() && !ble.IsBleConnectionOpen());

    // Verify that Thread-attribute parsing isn't available yet when not connected as client or server.
    attributeLen = sizeof(attributeBuffer);
    VerifyOrQuit(otBleSecureGetThreadAttributeFromPeerCertificate(instance, kCertificateAuthorizationField,
                                                                  &attributeBuffer[0],
                                                                  &attributeLen) == kErrorInvalidState);
    attributeLen = sizeof(attributeBuffer);
    VerifyOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(
                     instance, kCertificateThreadVersion, &attributeBuffer[0], &attributeLen) == kErrorInvalidState);

    // Validate connection callbacks when calling `otBleSecureDisconnect()`
    otPlatBleGapOnConnected(instance, kConnectionId);
    VerifyOrQuit(!ble.IsConnected() && ble.IsBleConnectionOpen());
    otBleSecureDisconnect(instance);
    VerifyOrQuit(!ble.IsConnected() && !ble.IsBleConnectionOpen());

    // Validate TLS connection can be started only when peer is connected
    otPlatBleGapOnConnected(instance, kConnectionId);
    SuccessOrQuit(otBleSecureConnect(instance));
    VerifyOrQuit(otBleSecureIsConnectionActive(instance));

    // Once in TLS client connecting state, the below cert eval functions are available.
    // Test that the Thread-specific attributes can be decoded properly.
    attributeLen = 1;
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateThreadVersion,
                                                                  &attributeBuffer[0], &attributeLen));
    VerifyOrQuit(attributeLen == 1 && attributeBuffer[0] >= kThreadVersion1p4);

    static_assert(5 == sizeof(expectedTcatAuthField), "expectedTcatAuthField size incorrect for test");
    attributeLen = 5;
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateAuthorizationField,
                                                                  &attributeBuffer[0], &attributeLen));
    VerifyOrQuit(attributeLen == 5 && memcmp(&expectedTcatAuthField, &attributeBuffer, attributeLen) == 0);

    // Validate TLS connection can be started only when peer is connected
    otBleSecureDisconnect(instance);
    VerifyOrQuit(otBleSecureConnect(instance) == kErrorInvalidState);

    // Validate Tcat state changes after stopping BLE secure
    VerifyOrQuit(otBleSecureIsTcatEnabled(instance));
    otBleSecureStop(instance);
    VerifyOrQuit(!otBleSecureIsTcatEnabled(instance));

    testFreeInstance(instance);
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    ot::TestTcat();
    printf("All tests passed\n");
#else
    printf("Tcat is not enabled\n");
    return -1;
#endif
    return 0;
}
