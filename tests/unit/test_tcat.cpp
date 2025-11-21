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

#include "cli/cli_dataset.hpp"

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
#define COMM_XPAN_ID {0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0xca, 0xfe}
#define COMM_XPAN_ID_ALT {0xef, 0x13, 0x98, 0xc2, 0xfd, 0x50, 0x4b, 0x67}

namespace ot {
namespace MeshCoP {

static constexpr char     kPskdVendor[]                  = "J01NM3";
static constexpr char     kUrl[]                         = "dummy_url";
static constexpr char     kDomainName[]                  = "DefaultDomain";
static constexpr char     kNetworkName[]                 = COMM_NETWORK_NAME;
static constexpr char     kWrongName[]                   = "WrongName";
static const uint8_t      kExtPanId[8]                   = COMM_XPAN_ID;
static const uint8_t      kExtPanIdAlt[8]                = COMM_XPAN_ID_ALT;
static constexpr uint16_t kConnectionId                  = 0;
static constexpr int      kCertificateThreadVersion      = 2;
static constexpr int      kCertificateAuthorizationField = 3;

static constexpr otTcatVendorInfo vendorInfo = {.mProvisioningUrl = kUrl, .mPskdString = kPskdVendor};

// TCAT command class bits for expressing any combination of classes in tests
static constexpr uint16_t kClassNone            = 0;
static constexpr uint16_t kClassGeneral         = 1 << TcatAgent::kGeneral;
static constexpr uint16_t kClassCommissioning   = 1 << TcatAgent::kCommissioning;
static constexpr uint16_t kClassExtraction      = 1 << TcatAgent::kExtraction;
static constexpr uint16_t kClassDecommissioning = 1 << TcatAgent::kDecommissioning;
static constexpr uint16_t kClassApplication     = 1 << TcatAgent::kApplication;

// TCAT authorization fields
static const uint8_t kDeviceCert1AuthField[5] = {0x20, 0x01, 0x01, 0x01, 0x01};
static const uint8_t kDeviceCert2AuthField[5] = {0x20, 0x02, 0x03, 0x04, 0x24};
static const uint8_t kCommCert1AuthField[5]   = {0x21, 0x01, 0x01, 0x01, 0x01};
static const uint8_t kCommCert2AuthField[5]   = {0x21, 0x1F, 0x3F, 0x3F, 0x3F};
static const uint8_t kCommCert4AuthField[5]   = {0x21, 0x21, 0x05, 0x09, 0x11};
static const uint8_t kCommCert5AuthField[5]   = {0x21, 0x03, 0x02, 0x83, 0x41};

static const otOperationalDataset kFullDataset = {
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
            .m8 = COMM_XPAN_ID,
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

static const otOperationalDataset kPartialDataset = {
    .mNetworkKey =
        {
            .m8 = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
        },
    .mComponents =
        {
            .mIsActiveTimestampPresent = false,
            .mIsNetworkKeyPresent      = true,
            .mIsNetworkNamePresent     = false,
            .mIsExtendedPanIdPresent   = false,
            .mIsMeshLocalPrefixPresent = false,
            .mIsPanIdPresent           = false,
            .mIsChannelPresent         = false,
            .mIsPskcPresent            = false,
            .mIsSecurityPolicyPresent  = false,
            .mIsChannelMaskPresent     = false,
        },
};

static Dataset::Info                            fullDataset, partialDataset;
static NetworkName                              commNetworkName, commDomainName;
static ExtendedPanId                            commExtPanId;
static TcatAgent::CertificateAuthorizationField commAuth, deviceAuth;

// Helper class to test BLE connection state.
class TestBleSecure
{
public:
    TestBleSecure(void)
        : mIsConnected(false)
        , mIsBleConnectionOpen(false)
    {
    }

    void HandleBleSecureConnect(const bool aConnected, const bool aBleConnectionOpen)
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

static void handleBleSecureConnect(otInstance *aInstance, bool aConnected, bool aBleConnectionOpen, void *aContext)
{
    OT_UNUSED_VARIABLE(aInstance);

    static_cast<TestBleSecure *>(aContext)->HandleBleSecureConnect(aConnected, aBleConnectionOpen);
}

// test helper to validate that only classes set '1' in aCommandClassesBitmap are authorized and others not.
static bool commandClassesAuthorized(const TcatAgent *aAgent, const uint16_t aCommandClassesBitmap)
{
    bool validationResult = true;

    static_assert(TcatAgent::kInvalid < 16, "kInvalid must be less than 16 to fit in uint16_t");
    for (uint16_t i = TcatAgent::kGeneral; i <= TcatAgent::kInvalid; i++)
    {
        const bool isAuthorizedByAgent     = aAgent->IsCommandClassAuthorized(static_cast<TcatAgent::CommandClass>(i));
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

// test helper to validate if Set Active Dataset commands are authorized or not, given the dataset to write.
static bool setActiveDatasetAuthorized(const TcatAgent *aAgent, const Dataset::Info &aDatasetInfo)
{
    Dataset dataset;

    // Convert high-level Dataset::Info into TLVs representation required by IsSetActiveDatasetAuthorized()
    VerifyOrQuit(dataset.WriteTlvsFrom(aDatasetInfo) == kErrorNone);
    return aAgent->IsSetActiveDatasetAuthorized(&dataset);
}

static Instance *testInitInstanceTcat(void)
{
    Instance *instance = testInitInstance();

    otBleSecureSetCertificate(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_X509_CERT), sizeof(OT_TCAT_X509_CERT),
                              reinterpret_cast<const uint8_t *>(OT_TCAT_PRIV_KEY), sizeof(OT_TCAT_PRIV_KEY));
    otBleSecureSetCaCertificateChain(instance, reinterpret_cast<const uint8_t *>(OT_TCAT_TRUSTED_ROOT_CERTIFICATE),
                                     sizeof(OT_TCAT_TRUSTED_ROOT_CERTIFICATE));
    otBleSecureSetSslAuthMode(instance, true);

    SuccessOrQuit(otBleSecureSetTcatVendorInfo(instance, &vendorInfo));

    // reset default data items used across tests
    fullDataset    = AsCoreType(&kFullDataset);
    partialDataset = AsCoreType(&kPartialDataset);
    commNetworkName.Set(kNetworkName);
    commDomainName.Set(kDomainName);
    memcpy(&commExtPanId, &kExtPanId, sizeof(commExtPanId));
    memcpy(&commAuth, &kCommCert1AuthField, sizeof(commAuth));
    memcpy(&deviceAuth, &kDeviceCert1AuthField, sizeof(deviceAuth));

    return instance;
}

void TestTcatConnectionAndCertAttributes(void)
{
    uint8_t       attributeBuffer[8];
    size_t        attributeLen;
    TestBleSecure ble;
    Instance     *instance = testInitInstanceTcat();

    // Validate BLE secure and Tcat start APIs
    VerifyOrQuit(otBleSecureTcatStart(instance, nullptr) == kErrorInvalidState);
    SuccessOrQuit(otBleSecureStart(instance, handleBleSecureConnect, nullptr, true, &ble));
    VerifyOrQuit(otBleSecureStart(instance, handleBleSecureConnect, nullptr, true, nullptr) == kErrorAlready);
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

    static_assert(5 == sizeof(kDeviceCert1AuthField), "expectedTcatAuthField size incorrect for test");
    attributeLen = 5;
    SuccessOrQuit(otBleSecureGetThreadAttributeFromOwnCertificate(instance, kCertificateAuthorizationField,
                                                                  &attributeBuffer[0], &attributeLen));
    VerifyOrQuit(attributeLen == 5 && memcmp(&kDeviceCert1AuthField, &attributeBuffer, attributeLen) == 0);

    // Validate TLS client connection can be started only when peer is BLE-connected
    otBleSecureDisconnect(instance);
    VerifyOrQuit(otBleSecureConnect(instance) == kErrorInvalidState);

    // Validate Tcat agent state changes after stopping BLE secure
    VerifyOrQuit(otBleSecureIsTcatAgentStarted(instance));
    otBleSecureStop(instance);
    VerifyOrQuit(!otBleSecureIsTcatAgentStarted(instance));

    testFreeInstance(instance);
}

class UnitTester
{
private:
    // Mock action: TCAT Commissioner connects with authorization aCommAuth while device has aDeviceAuth.
    static void mockCommissionerConnected(TcatAgent                                     *aAgent,
                                          const TcatAgent::CertificateAuthorizationField aCommAuth,
                                          const TcatAgent::CertificateAuthorizationField aDeviceAuth,
                                          const bool                                     aIsCommissionedAtStart)
    {
        aAgent->mState                          = TcatAgent::kStateConnected;
        aAgent->mCommissionerAuthorizationField = aCommAuth;
        aAgent->mDeviceAuthorizationField       = aDeviceAuth;
        aAgent->mPskcVerified                   = false;
        aAgent->mPskdVerified                   = false;
        aAgent->mCommissionerHasExtendedPanId   = false;
        aAgent->mCommissionerHasNetworkName     = false;
        aAgent->mCommissionerHasDomainName      = false;
        aAgent->mIsCommissioned                 = aIsCommissionedAtStart;
    }

    // Mock condition: commissioner has or has not the given Extended Pan ID in its certificate.
    static void mockExtPanId(TcatAgent *aAgent, const bool aCommHasExtPanId, const ExtendedPanId *aExtPanId)
    {
        aAgent->mCommissionerHasExtendedPanId = aCommHasExtPanId;
        aAgent->mCommissionerExtendedPanId    = *aExtPanId;
    }

    // Mock condition: commissioner has or has not the given Network Name in its certificate.
    static void mockNetworkName(TcatAgent *aAgent, const bool aCommHasNetworkName, const NetworkName *aNetworkName)
    {
        aAgent->mCommissionerHasNetworkName = aCommHasNetworkName;
        aAgent->mCommissionerNetworkName    = *aNetworkName;
    }

    // Mock condition: commissioner has or has not the given Domain Name in its certificate.
    static void mockDomainName(TcatAgent *aAgent, const bool aCommHasDomainName, const NetworkName *aDomainName)
    {
        aAgent->mCommissionerHasDomainName = aCommHasDomainName;
        aAgent->mCommissionerDomainName    = *aDomainName;
    }

public:
    static void TestTcatCommissioner1Auth(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());

        // validate no Commissioner authorizations if not connected
        VerifyOrQuit(commandClassesAuthorized(agent, kClassNone));

        // Mock TCAT Commissioner 1 connects to the agent - verify it has access to all classes
        // ====================================================================================
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));

        // Write a partial Active Dataset and verify that Commissioner can still overwrite this with another dataset
        // if needed.
        instance->Get<ActiveDatasetManager>().SaveLocal(partialDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsPartiallyComplete());
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        // Write a full Active Dataset and verify that Commissioner can still overwrite this.
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsPartiallyComplete());
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        // And back to partial dataset.
        instance->Get<ActiveDatasetManager>().SaveLocal(partialDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsPartiallyComplete());
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        // provide PSKc proof-of-possession - verify access is same as before
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsPartiallyComplete());
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner2Auth(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // Mock TCAT Commissioner 2 connects to the agent - verify it only has access to class General by default.
        // CommCert2 contains Network Name and Extended PAN ID in this initial test, but not the (also-required)
        // Thread Domain Name.
        // =======================================================================================================
        memcpy(&commAuth, &kCommCert2AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        mockNetworkName(agent, true, &commNetworkName);
        mockExtPanId(agent, true, &commExtPanId);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));

        // Verify that Set Active Dataset can't be used yet, despite a matching XPAN ID and Network Name for the
        // dataset that the Commissioner wants to write.
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // provide PSKd proof-of-possession - this is required for all 4 command classes, but not sufficient yet.
        // So verify there's no change.
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // Commissioner cert now has a matching Domain Name - does not unlock any new classes, because
        // Network Name and XPAN ID can't match, due to Device being uncommissioned. Writing Active Dataset works now.
        mockDomainName(agent, true, &commDomainName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        // Writing a partial dataset does not work: misses the required Network Name and XPAN ID
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Commissioner now has no XPAN ID anymore in cert - verify this prevents Set Active Dataset.
        // It's a misconfig in the Commissioner's cert.
        mockExtPanId(agent, false, &commExtPanId);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Commissioner now has correct XPAN ID in cert, but not matching the dataset it wants to write.
        mockExtPanId(agent, true, &commExtPanId);
        fullDataset.mExtendedPanId.m8[2]++; // modify bits in dataset to be written.
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Commissioner now attempts to write a dataset with XPAN ID matching to that in cert.
        fullDataset.mExtendedPanId = commExtPanId;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: Active Dataset is now configured (device is commissioned), but XPAN ID in Dataset
        // doesn't match the XPAN ID in the Commissioner's cert; and PSKc proof is not given yet,
        // so most classes remain unavailable.
        fullDataset.mExtendedPanId.m8[2]++; // modify bits in dataset to be written.
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(instance->Get<ActiveDatasetManager>().IsCommissioned());
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // XPAN ID now matches again; class Commissioning authorization (0x1F) is restored. It doesn't require
        // PSKc proof.
        fullDataset.mExtendedPanId = commExtPanId;
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        // Active Dataset can be overwritten because Device was uncommissioned at session start.
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Now PSKc proof is given, unlocking more command classes
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Commissioner connects again - this time, the Device is already commissioned at the start of the session.
        mockCommissionerConnected(agent, commAuth, deviceAuth, true);
        mockNetworkName(agent, true, &commNetworkName);
        mockExtPanId(agent, true, &commExtPanId);
        mockDomainName(agent, true, &commDomainName);
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // PSKd proof does not authorize Set Active Dataset - because device is already commissioned.
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner4Auth(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // Mock TCAT Commissioner 4 connects to the Device - verify it only has access to class General by default.
        // The Device is commissioned already at start of the TCAT Link.
        // =======================================================================================================
        memcpy(&commAuth, &kCommCert4AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, true);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // PSKc proof - satisfies 0x21. Set Active Dataset is not allowed: Device already commissioned.
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // Matching network name now in Commissioner cert - satisfies 0x05
        mockNetworkName(agent, true, &commNetworkName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: matching XPAN ID present in Comm cert - satisfies 0x09
        mockExtPanId(agent, true, &commExtPanId);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: Thread Domain name in cert matches - Application class added.
        mockDomainName(agent, true, &commDomainName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: PSKc proof not given - Commissioning class is revoked
        agent->mPskcVerified = false;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassExtraction | kClassDecommissioning |
                                                         kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: network name present, but mismatch - Extraction revoked
        NetworkName wrongNetworkName;
        wrongNetworkName.Set("WrongName");
        mockNetworkName(agent, true, &wrongNetworkName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassDecommissioning | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: XPAN ID present, but mismatch - Decommissioning revoked
        fullDataset.mExtendedPanId.m8[4]++; // change bits to force a mismatch
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        mockExtPanId(agent, true, &commExtPanId);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: XPAN ID not present in dataset - same as before
        fullDataset.mExtendedPanId                      = commExtPanId; // restore changes bits of above
        fullDataset.mComponents.mIsExtendedPanIdPresent = false;
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: Network Name not present in dataset - same as before
        fullDataset.mComponents.mIsNetworkNamePresent = false;
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // New situation: Device is decommissioned (by some other Commissioner). Then, this Commissioner
        // connects again and does PSKc proof. Set Active Dataset access should now be allowed.
        instance->Get<ActiveDatasetManager>().Clear();
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner5Auth(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // Mock TCAT Commissioner 5 connects to the agent - it requires checks that are unknown to the Device
        // ==================================================================================================
        memcpy(&commAuth, &kCommCert5AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, true);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // PSKd proof is given: it won't enable Extraction, because Extraction access flag bit 0 = 0.
        // Also it won't enable Decommissioning, because this class has an unknown flag bit 7 set i.e. the Commissioner
        // is configured to require a method that the TCAT Device doesn't know about. Application class is also not
        // enabled, since it requires a check with unknown flag bit 6. Device will enable Commissioning class.
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Connect again and test the situation that Device was uncommissioned at connection start.
        // Commissioning is now enabled with PSKd proof.
        instance->Get<ActiveDatasetManager>().Clear();
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner1AuthWithDeviceRequirements(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // test different auth info: for TCAT Device 2 which has specific authorization requirements per class.
        memcpy(&deviceAuth, &kDeviceCert2AuthField, sizeof(deviceAuth));

        // Mock TCAT Commissioner 1 connects to the agent - verify
        // ====================================================================================
        memcpy(&commAuth, &kCommCert1AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassExtraction));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // PSKd proof
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, partialDataset));

        // New situation: Device is commissioned and Commissioner has matching Network Name; and connects.
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        mockCommissionerConnected(agent, commAuth, deviceAuth, true);
        mockNetworkName(agent, true, &commNetworkName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassExtraction | kClassDecommissioning |
                                                         kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // PSKc proof
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassExtraction | kClassDecommissioning |
                                                         kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // If Network Name does not match
        NetworkName wrongNetworkName;
        wrongNetworkName.Set(kWrongName);
        mockNetworkName(agent, true, &wrongNetworkName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassExtraction | kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        agent->mPskdVerified = true;
        agent->mPskcVerified = false;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassApplication));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner2AuthWithDeviceRequirements(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // test different auth info: for TCAT Device 2 which has specific authorization requirements per class.
        memcpy(&deviceAuth, &kDeviceCert2AuthField, sizeof(deviceAuth));

        // Mock TCAT Commissioner 2 connects to the agent
        // ==============================================
        memcpy(&commAuth, &kCommCert2AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, false);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // PSKd proof
        agent->mPskdVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Network Name match
        mockNetworkName(agent, true, &commNetworkName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // XPAN ID match
        mockExtPanId(agent, true, &commExtPanId);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Domain Name match
        mockDomainName(agent, true, &commDomainName);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // PSKc proof
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning | kClassApplication));
        VerifyOrQuit(setActiveDatasetAuthorized(agent, fullDataset));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, partialDataset));

        // Try write a full dataset with differing XPAN ID
        fullDataset.mExtendedPanId.m8[2]++;
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        testFreeInstance(instance);
    }

    static void TestTcatCommissioner4AuthWithExistingPartialDataset(void)
    {
        Instance  *instance = testInitInstanceTcat();
        TcatAgent *agent    = &instance->Get<TcatAgent>();

        // TCAT device was commissioned earlier on with a partial dataset.
        instance->Get<ActiveDatasetManager>().SaveLocal(partialDataset);

        // Mock TCAT Commissioner 4 connects to the Device.
        // static const uint8_t kCommCert4AuthField[5]   = {0x21, 0x21, 0x05, 0x09, 0x11};
        memcpy(&commAuth, &kCommCert4AuthField, sizeof(commAuth));
        mockCommissionerConnected(agent, commAuth, deviceAuth, true);
        mockNetworkName(agent, true, &commNetworkName);
        mockExtPanId(agent, true, &commExtPanId);

        // It wants access to Extraction class (0x05) based on matching Network Name, but it's denied.
        // Decommissioning (0x09) based on matching XPAN ID is also denied.
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // PSKc proof - satisfies 0x21. Set Active Dataset is not allowed: Device already commissioned.
        agent->mPskcVerified = true;
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        // As a sanity check, redo the test assuming that a (matching) full dataset was initially in the Device.
        // Now, Extraction and Commissioning classes do work because of matching elements in the Commcert.
        instance->Get<ActiveDatasetManager>().SaveLocal(fullDataset);
        VerifyOrQuit(commandClassesAuthorized(agent, kClassGeneral | kClassCommissioning | kClassExtraction |
                                                         kClassDecommissioning));
        VerifyOrQuit(!setActiveDatasetAuthorized(agent, fullDataset));

        testFreeInstance(instance);
    }
}; // class UnitTester

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BLE_TCAT_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    ot::MeshCoP::TestTcatConnectionAndCertAttributes();
    ot::MeshCoP::UnitTester::TestTcatCommissioner1Auth();
    ot::MeshCoP::UnitTester::TestTcatCommissioner2Auth();
    ot::MeshCoP::UnitTester::TestTcatCommissioner4Auth();
    ot::MeshCoP::UnitTester::TestTcatCommissioner5Auth();
    ot::MeshCoP::UnitTester::TestTcatCommissioner1AuthWithDeviceRequirements();
    ot::MeshCoP::UnitTester::TestTcatCommissioner2AuthWithDeviceRequirements();
    ot::MeshCoP::UnitTester::TestTcatCommissioner4AuthWithExistingPartialDataset();
    printf("All tests passed\n");
#else
    printf("TCAT feature is not enabled\n");
#endif
    return 0;
}
