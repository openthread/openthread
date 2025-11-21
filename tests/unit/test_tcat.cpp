/*
 *  Copyright (c) 2024-2025, The OpenThread Authors.
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

#include "cli/cli_dataset.hpp"

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

#define COMM_NETWORK_NAME "OpenThread-c64e"

namespace ot {
namespace MeshCoP {

static constexpr char     kPskdVendor[]                  = "J01NM3";
static constexpr char     kUrl[]                         = "dummy_url";
static constexpr char     kDomainName[]                  = "DefaultDomain";
static constexpr char     kNetworkName[]                 = COMM_NETWORK_NAME;
static constexpr uint16_t kConnectionId                  = 0;
static constexpr int      kCertificateThreadVersion      = 2;
static constexpr int      kCertificateAuthorizationField = 3;

// TCAT command class bits for expressing any combination of classes in tests
static constexpr uint16_t kClassNone            = 0;
static constexpr uint16_t kClassGeneral         = 1 << TcatAgent::kGeneral;
static constexpr uint16_t kClassCommissioning   = 1 << TcatAgent::kCommissioning;
static constexpr uint16_t kClassExtraction      = 1 << TcatAgent::kExtraction;
static constexpr uint16_t kClassDecommissioning = 1 << TcatAgent::kDecommissioning;
static constexpr uint16_t kClassApplication     = 1 << TcatAgent::kApplication;

// TCAT authorization fields
static const uint8_t kExpectedDevAuthField[5] = {0x20, 0x01, 0x01, 0x01, 0x01};
static const uint8_t kCommCert1AuthField[5]   = {0x21, 0x01, 0x01, 0x01, 0x01};
static const uint8_t kCommCert2AuthField[5]   = {0x21, 0x1F, 0x3F, 0x3F, 0x3F};
static const uint8_t kCommCert4AuthField[5]   = {0x21, 0x21, 0x05, 0x09, 0x11};
static const uint8_t kCommCert5AuthField[5]   = {0x21, 0x03, 0x02, 0x83, 0x41};

// TCAT certificate encoded info
static const uint8_t kCommCert2ExtendedPanId[8] = {0xef, 0x13, 0x98, 0xc2, 0xfd, 0x50, 0x4b, 0x67};

static const otOperationalDataset kDataset = {
    .mActiveTimestamp =
        {
            .mSeconds       = 1,
            .mTicks         = 0,
            .mAuthoritative = false,
        },
    .mNetworkKey =
        {
            .m8 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        },
    .mNetworkName = {COMM_NETWORK_NAME},
    .mExtendedPanId =
        {
            .m8 = {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe},
        },
    .mMeshLocalPrefix =
        {
            .m8 = {0xfd, 0x00, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        },
    .mPanId   = 0x1234,
    .mChannel = 11,
    .mPskc =
        {
            .m8 = {0xc2, 0x3a, 0x76, 0xe9, 0x8f, 0x1a, 0x64, 0x83, 0x63, 0x9b, 0x1a, 0xc1, 0x27, 0x1e, 0x2e, 0x27},
        },
    .mSecurityPolicy =
        {
            .mRotationTime                 = 672,
            .mObtainNetworkKeyEnabled      = true,
            .mNativeCommissioningEnabled   = true,
            .mRoutersEnabled               = true,
            .mExternalCommissioningEnabled = true,
        },
    .mChannelMask = 0x07fff800,
    .mComponents =
        {
            .mIsActiveTimestampPresent = true,
            .mIsNetworkKeyPresent      = true,
            .mIsNetworkNamePresent     = true,
            .mIsExtendedPanIdPresent   = true,
            .mIsMeshLocalPrefixPresent = true,
            .mIsPanIdPresent           = true,
            .mIsChannelPresent         = true,
            .mIsPskcPresent            = true,
            .mIsSecurityPolicyPresent  = true,
            .mIsChannelMaskPresent     = true,
        },
};

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

// test helper to validate that only classes set '1' in aCommandClassesBitmap are authorized and others not.
static bool CommandClassesAuthorized(const TcatAgent *agent, const uint16_t aCommandClassesBitmap)
{
    bool validationResult = true;

    static_assert(TcatAgent::kInvalid < 16, "kInvalid must be less than 16 to fit in uint16_t");
    for (uint16_t i = TcatAgent::kGeneral; i <= TcatAgent::kInvalid; i++)
    {
        const bool isAuthorizedByAgent     = agent->IsCommandClassAuthorized(static_cast<TcatAgent::CommandClass>(i));
        const bool isAuthorizationExpected = (aCommandClassesBitmap & (1 << i)) != 0;
        if (isAuthorizedByAgent != isAuthorizationExpected)
        {
            printf("Expected command class %d authorization '%d', but TCAT Agent reports '%d'\n", i,
                   isAuthorizationExpected, isAuthorizedByAgent);
            validationResult = false;
        }
    }
    return validationResult;
}

void TestTcat(void)
{
    uint8_t       attributeBuffer[8];
    size_t        attributeLen;
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

    // Validate TLS connection can be started (as client) only when peer is BLE-connected
    otPlatBleGapOnConnected(instance, kConnectionId);
    SuccessOrQuit(otBleSecureConnect(instance));
    VerifyOrQuit(otBleSecureIsConnectionActive(instance));

    // Once in TLS client connecting state, the below cert eval functions are available.
    // Test that the Thread-specific attributes from own certificate can be decoded properly.
    attributeLen = 1;
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateThreadVersion,
                                                                  &attributeBuffer[0], &attributeLen));
    VerifyOrQuit(attributeLen == 1 && attributeBuffer[0] >= kThreadVersion1p4);

    static_assert(5 == sizeof(kExpectedDevAuthField), "expectedTcatAuthField size incorrect for test");
    attributeLen = 5;
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateAuthorizationField,
                                                                  &attributeBuffer[0], &attributeLen));
    VerifyOrQuit(attributeLen == 5 && memcmp(&kExpectedDevAuthField, &attributeBuffer, attributeLen) == 0);

    // Validate TLS client connection can be started only when peer is BLE-connected
    otBleSecureDisconnect(instance);
    VerifyOrQuit(otBleSecureConnect(instance) == kErrorInvalidState);

    // Validate Tcat agent state changes after stopping BLE secure
    VerifyOrQuit(otBleSecureIsTcatAgentStarted(instance));
    otBleSecureStop(instance);
    VerifyOrQuit(!otBleSecureIsTcatAgentStarted(instance));

    testFreeInstance(instance);
}

void TestTcatCommissionerAuth(void)
{
    constexpr otTcatVendorInfo               vendorInfo = {.mProvisioningUrl = kUrl, .mPskdString = kPskdVendor};
    TcatAgent::CertificateAuthorizationField commAuth, deviceAuth;
    size_t                                   lenAuth  = sizeof(deviceAuth);
    Instance                                *instance = testInitInstance();
    TcatAgent                               *agent    = &instance->Get<TcatAgent>();
    TcatAgentTestProbe                       tester(agent);
    NetworkName                              commNetworkName;
    NetworkName                              commDomainName;
    ExtendedPanId                            commExtPanId;
    Dataset::Info                            dataset;

    commNetworkName.Set(kNetworkName);
    commDomainName.Set(kDomainName);
    dataset = AsCoreType(&kDataset);

    // TCAT Device is initially uncommissioned
    VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());

    // Configure TCAT Device certs and vendorInfo
    SuccessOrQuit(otBleSecureSetTcatVendorInfo(instance, &vendorInfo));
    otBleSecureSetCertificate(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_X509_CERT), sizeof(OT_TCAT_X509_CERT),
                              reinterpret_cast<const uint8_t *>(OT_TCAT_PRIV_KEY), sizeof(OT_TCAT_PRIV_KEY));
    otBleSecureSetCaCertificateChain(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_TRUSTED_ROOT_CERTIFICATE),
                                     sizeof(OT_TCAT_TRUSTED_ROOT_CERTIFICATE));
    otBleSecureSetSslAuthMode(instance, true);

    // validate no Commissioner authorizations if not connected
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassNone));

    // setup TCAT and connect as client, such that own cert info becomes available.
    SuccessOrQuit(otBleSecureStart(instance, nullptr, nullptr, true, nullptr));
    otPlatBleGapOnConnected(instance, kConnectionId);
    SuccessOrQuit(otBleSecureTcatStart(instance, nullptr));
    SuccessOrQuit(otBleSecureConnect(instance));
    VerifyOrQuit(otBleSecureIsConnectionActive(instance));

    // get auth info from own cert of the TCAT Device
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateAuthorizationField,
                                                                  reinterpret_cast<uint8_t *>(&deviceAuth), &lenAuth));

    // Mock TCAT Commissioner 1 connects to the agent - verify it has access to all classes
    // ====================================================================================
    memcpy(&commAuth, &kCommCert1AuthField, sizeof(commAuth));
    tester.MockCommissionerConnected(commAuth, deviceAuth);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // provide PSKc proof-of-possession - verify access is same as before
    tester.MockCommissionerPskcProof(true);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // Mock TCAT Commissioner 2 connects to the agent - verify it only has access to class General by default.
    // CommCert2 contains Network Name, Extended PAN ID in this initial test.
    // =======================================================================================================
    memcpy(&commAuth, &kCommCert2AuthField, sizeof(commAuth));
    memcpy(&commExtPanId, &kCommCert2ExtendedPanId, sizeof(commExtPanId));

    tester.MockCommissionerConnected(commAuth, deviceAuth);
    tester.MockNetworkName(true, &commNetworkName);
    tester.MockXpan(true, &commExtPanId);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral));

    // provide PSKd proof-of-possession - verify there's no change
    tester.MockCommissionerPskdProof(true);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral));

    // Commissioner now has matching Domain Name - unlocks Commissioning class; and also all other classes
    // because the required PSKc proof is automatically satisfied: the Device doesn't have a PSKc stored, yet.
    tester.MockDomainName(true, &commDomainName);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // Commissioner now has matching XPAN ID - same as before
    tester.MockXpan(true, &commExtPanId);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // Commissioner now has matching Network Name also - same as before
    tester.MockNetworkName(true, &commNetworkName);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // New situation: Active Dataset is configured, but XPAN ID doesn't match - and PSKc proof is not given - so
    // most classes are revoked.
    instance->Get<ActiveDatasetManager>().SaveLocal(dataset);
    VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsCommissioned());
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral));

    // XPAN ID now matches again; class Commissioning authorization is restored because that doesn't require
    // PSKc proof.
    dataset.mExtendedPanId = commExtPanId;
    instance->Get<ActiveDatasetManager>().SaveLocal(dataset);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));

    // Now PSKc proof is given, unlocking more classes
    tester.MockCommissionerPskcProof(true);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // Mock TCAT Commissioner 4 connects to the agent - verify it only has access to class General by default.
    // =======================================================================================================
    memcpy(&commAuth, &kCommCert4AuthField, sizeof(commAuth));
    tester.MockCommissionerConnected(commAuth, deviceAuth);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral));

    // PSKc proof
    tester.MockCommissionerPskcProof(true);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));

    // Network name
    tester.MockNetworkName(true, &commNetworkName);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction));

    // New situation: XPAN ID present in Comm cert
    tester.MockXpan(true, &commExtPanId);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning));

    // New situation: Thread Domain name in cert matches - Application class added.
    tester.MockDomainName(true, &commDomainName);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                     kClassDecommissioning | kClassApplication));

    // New situation: PSKc proof not given - Commissioning revoked
    tester.MockCommissionerPskcProof(false);
    VerifyOrQuit(
        CommandClassesAuthorized(agent, kClassGeneral | kClassExtraction | kClassDecommissioning | kClassApplication));

    // New situation: network name present, but mismatch - Extraction revoked
    commNetworkName.Set("WrongName");
    tester.MockNetworkName(true, &commNetworkName);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassDecommissioning | kClassApplication));

    // New situation: XPAN ID present, but mismatch - Decommissioning revoked
    dataset.mExtendedPanId.m8[4]++;
    instance->Get<ActiveDatasetManager>().SaveLocal(dataset);
    tester.MockXpan(true, &commExtPanId);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassApplication));

    // Mock TCAT Commissioner 5 connects to the agent - it requires checks that are unknown to the Device
    // ==================================================================================================
    memcpy(&commAuth, &kCommCert5AuthField, sizeof(commAuth));
    tester.MockCommissionerConnected(commAuth, deviceAuth);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral));

    // PSKd proof is given: it won't enable Extraction, because Extraction access flag = 0.
    // Also it won't enable Decommissioning, because it has an unknown flag bit 7 set.
    // It will enable Commissioning.
    tester.MockCommissionerPskdProof(true);
    VerifyOrQuit(CommandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));

    testFreeInstance(instance);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    ot::MeshCoP::TestTcat();
    ot::MeshCoP::TestTcatCommissionerAuth();
    printf("All tests passed\n");
#else
    printf("Tcat is not enabled\n");
    return -1;
#endif
    return 0;
}
