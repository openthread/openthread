/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/uri_paths.hpp"
#include "utils/verhoeff_checksum.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 50 * Time::kOneSecondInMsec;

/**
 * Time to advance for the DUT to connect to the network, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 10 * Time::kOneSecondInMsec;

/**
 * Time to advance for mDNS discovery, in milliseconds.
 */
static constexpr uint32_t kMdnsDiscoveryTime = 5 * Time::kOneSecondInMsec;

/**
 * Time to advance for DTLS connection, in milliseconds.
 */
static constexpr uint32_t kConnectTimeout = 3 * Time::kOneSecondInMsec;

/**
 * Time to advance for TMF message exchange, in milliseconds.
 */
static constexpr uint32_t kTmfExchangeTime = 1 * Time::kOneSecondInMsec;

/**
 * Max ePSKc connection attempts.
 */
static constexpr uint8_t kMaxEpskcConnAttempts = 10;

/**
 * The filename for the DTLS key log.
 */
static const char kKeyLogFilename[] = "/tmp/test_1_4_CS_TC_3.keys";

/**
 * The master secret length in bytes.
 */
static constexpr uint8_t kMasterSecretLength = 48;

/**
 * The random buffer length in bytes.
 */
static constexpr uint8_t kRandomBufferLength = MeshCoP::SecureTransport::kSecureTransportRandomBufferSize;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * The UDP port for ePSKc DTLS session.
 */
static constexpr uint16_t kEpskcPort = 1234;

/**
 * Time to wait for DTLS retransmissions to finish (in milliseconds).
 */
static constexpr uint32_t kDtlsRetransmissionWaitTime = 60 * Time::kOneSecondInMsec;

/**
 * Time to wait for DUT state to be cleaned up after DTLS session (in milliseconds).
 */
static constexpr uint32_t kDutStateCleanTime = 100 * Time::kOneSecondInMsec;

using EphemeralKeyManager = MeshCoP::BorderAgent::EphemeralKeyManager;
using Manager             = MeshCoP::BorderAgent::Manager;

static void HandleMbedtlsExportKeys(void                       *aContext,
                                    mbedtls_ssl_key_export_type aType,
                                    const unsigned char        *aMasterSecret,
                                    size_t                      aMasterSecretLen,
                                    const unsigned char         aClientRandom[kRandomBufferLength],
                                    const unsigned char         aServerRandom[kRandomBufferLength],
                                    mbedtls_tls_prf_types       aTlsPrfType)
{
    FILE *file = fopen(kKeyLogFilename, "a");

    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMasterSecretLen);
    OT_UNUSED_VARIABLE(aServerRandom);
    OT_UNUSED_VARIABLE(aTlsPrfType);

    if (file != nullptr)
    {
        if (aType == MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET)
        {
            fprintf(file, "CLIENT_RANDOM ");
            for (int i = 0; i < kRandomBufferLength; i++)
            {
                fprintf(file, "%02x", aClientRandom[i]);
            }
            fprintf(file, " ");
            for (int i = 0; i < kMasterSecretLength; i++)
            {
                fprintf(file, "%02x", aMasterSecret[i]);
            }
            fprintf(file, "\n");
        }
        fclose(file);
    }
}

typedef Dns::Name::Buffer DnsName;

struct BrowseOutcome
{
    DnsName      mServiceInstance;
    DnsName      mHostName;
    uint16_t     mPort;
    Ip6::Address mAddress;
};

static Array<BrowseOutcome, 5> sBrowseOutcomes;

static void HandleBrowseCallback(otInstance *aInstance, const Dns::Multicast::Core::BrowseResult *aResult)
{
    BrowseOutcome *outcome;

    OT_UNUSED_VARIABLE(aInstance);

    outcome = sBrowseOutcomes.PushBack();
    VerifyOrQuit(outcome != nullptr);

    SuccessOrQuit(StringCopy(outcome->mServiceInstance, aResult->mServiceInstance));
}

static void HandleSrvCallback(otInstance *aInstance, const Dns::Multicast::Core::SrvResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    for (BrowseOutcome &outcome : sBrowseOutcomes)
    {
        if (StringMatch(outcome.mServiceInstance, aResult->mServiceInstance))
        {
            outcome.mPort = aResult->mPort;
            SuccessOrQuit(StringCopy(outcome.mHostName, aResult->mHostName));
            break;
        }
    }
}

static void HandleAddressCallback(otInstance *aInstance, const Dns::Multicast::Core::AddressResult *aResult)
{
    OT_UNUSED_VARIABLE(aInstance);

    for (BrowseOutcome &outcome : sBrowseOutcomes)
    {
        if (StringMatch(outcome.mHostName, aResult->mHostName))
        {
            if (aResult->mAddressesLength > 0)
            {
                outcome.mAddress = AsCoreType(&aResult->mAddresses[0].mAddress);
            }
            break;
        }
    }
}

static void DiscoverMeshcopEService(Node &aCommNode, Core &aNexus, Ip6::Address &aAddress, uint16_t &aPort)
{
    sBrowseOutcomes.Clear();
    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop-e._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleBrowseCallback;
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StartBrowser(browser));
        aNexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 1);

    {
        Dns::Multicast::Core::SrvResolver srvResolver;
        ClearAllBytes(srvResolver);
        srvResolver.mServiceInstance = sBrowseOutcomes[0].mServiceInstance;
        srvResolver.mServiceType     = "_meshcop-e._udp";
        srvResolver.mInfraIfIndex    = kInfraIfIndex;
        srvResolver.mCallback        = HandleSrvCallback;
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StartSrvResolver(srvResolver));
        aNexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StopSrvResolver(srvResolver));
    }

    {
        Dns::Multicast::Core::AddressResolver addrResolver;
        ClearAllBytes(addrResolver);
        addrResolver.mHostName     = sBrowseOutcomes[0].mHostName;
        addrResolver.mInfraIfIndex = kInfraIfIndex;
        addrResolver.mCallback     = HandleAddressCallback;
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StartIp6AddressResolver(addrResolver));
        aNexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(aCommNode.Get<Dns::Multicast::Core>().StopIp6AddressResolver(addrResolver));
    }

    aPort    = sBrowseOutcomes[0].mPort;
    aAddress = sBrowseOutcomes[0].mAddress;
}

void Test1_4_CS_TC_3(void)
{
    /**
     * 12.3. [1.4] [CERT] [COMPONENT] BR component creates numeric passcode and Candidate connects
     *
     * 12.3.1. Purpose
     * This test is for automatically testing BR component DUTs. Instead of a test operator typing the passcode, the
     *   DUT generates a passcode and shares it via the Thread Harness API to the Harness so that it can execute the
     *   automated test. No UI, GUI or display is needed on the BR DUT in this case.
     * In detail, the purpose is to verify that the Border Router DUT component:
     * - correctly implements the Thread Credential Sharing feature by supporting the Thread Administration Sharing
     *   procedure
     * - is able to generate a numeric Thread Administration One-Time Passcode consisting of 9 digits
     * - allows an external Candidate to connect using the ePSKc that is based on the passcode
     * - is able to facilitate the Candidate’s operations
     * - enables access to a Candidate using the ePSKc, only once.
     *
     * 12.3.2. Topology
     * - BR_1 – Border Router Thread Component DUT
     * - Router_1 – Thread Router reference device and Leader
     * - Comm_1 – IPv6 host reference device on the AIL that implements a Commissioner Candidate role.
     *
     * Spec Reference            | V1.4 Section
     * --------------------------|-----------------
     * Thread Credential Sharing | 8.4.1.2.3, 8.4.9
     */

    Core nexus;

    Node &router1 = nexus.CreateNode();
    Node &br1     = nexus.CreateNode();
    Node &comm1   = nexus.CreateNode();

    router1.SetName("Router_1");
    br1.SetName("BR_1");
    comm1.SetName("Comm_1");

    router1.Get<Manager>().SetEnabled(false);
    comm1.Get<Manager>().SetEnabled(false);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    // Remove any existing key log file.
    remove(kKeyLogFilename);

#if OPENTHREAD_CONFIG_MBEDTLS_PROVIDES_SSL_KEY_EXPORT
    static_cast<MeshCoP::SecureTransport &>(comm1.Get<Tmf::SecureAgent>())
        .SetKeylogCallback(HandleMbedtlsExportKeys, &comm1.Get<Tmf::SecureAgent>());
#endif

    SuccessOrQuit(br1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    comm1.Get<ThreadNetif>().Up();

    // Wait for the interfaces to be ready and addresses to be configured.
    nexus.AdvanceTime(Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Enable and form network.");

    /**
     * Step 1
     * - Device: Router_1, BR_1 (DUT)
     * - Description (CS-12.3): Enable and form network. The DUT is configured with the Network Key of the test
     *   Thread Network, so that it can connect to it.
     * - Pass Criteria: The DUT MUST connect to Router_1’s network.
     */

    router1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Join(router1);
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(comm1.Get<Mac::Mac>().SetPanChannel(router1.Get<Mac::Mac>().GetPanChannel()));
    comm1.Get<Mac::Mac>().SetPanId(router1.Get<Mac::Mac>().GetPanId());
    comm1.Get<ThreadNetif>().Up();

    br1.Get<Manager>().SetEnabled(true);
    nexus.AdvanceTime(Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Comm_1 discovers BR DUT service.");

    /**
     * Step 2
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to do mDNS Qtype=PTR discovery query for DNS-SD service type
     *   “_meshcop._udp.local”, to find the BR DUT service. ... Harness then verifies data in the SRV record and TXT
     *   record as detailed to the right.
     * - Pass Criteria:
     *   - The DUT MUST respond with an mDNS message that contains one PTR record of type “_meshcop._udp.local” in the
     *     Answer section.
     *   - Comm_1 MUST be able to obtain the TXT record as detailed on the left.
     *   - In the TXT record:
     *     - ePSKc Support flag (Bit 11) in State Bitmap (sb) MUST be ‘1’ (ePSKc supported)
     *     - ‘vn’ field MUST be present and contain >= 1 characters.
     *     - ‘mn’ field MUST be present and contain >= 1 characters.
     *   - Comm_1 MUST be able to obtain the SRV record as detailed on the left. Harness logs and stores the port
     *     number in the SRV record for later use.
     */

    // Border Agent starts automatically when attached.
    VerifyOrQuit(br1.Get<Manager>().IsEnabled());

    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleBrowseCallback;
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    VerifyOrQuit(sBrowseOutcomes.GetLength() == 1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_1 (DUT) produces passcode.");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (CS-12.3): Harness instructs the DUT, via Test Harness API, to produce the Thread Administration
     *   One-Time Passcode.
     * - Pass Criteria: Passcode is successfully retrieved.
     */

    EphemeralKeyManager::Tap tap1;
    SuccessOrQuit(tap1.GenerateRandom());
    Log("Passcode 1: %s", tap1.mTap);
    nexus.AddTestVar("Passcode1", tap1.mTap);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Harness validates the passcode.");

    /**
     * Step 4
     * - Device: Harness
     * - Description (CS-12.3): Harness validates the passcode. Harness calculates the ePSKc as follows: each
     *   character from the passcode string is taken as an ePSKc octet.
     * - Pass Criteria:
     *   - Harness validates the following, using its passcode validation function:
     *   - The passcode MUST contain only digits 0-9.
     *   - The passcode MUST be 9 digits long.
     *   - The last digit of the passcode MUST be equal to the check digit calculated by the Verhoeff algorithm on
     *     the first 8 digits.
     */

    SuccessOrQuit(tap1.Validate());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Comm_1 discovers meshcop-e service.");

    /**
     * Step 5
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to discover a DNS-SD service type via mDNS, Qtype=PTR, name
     *   “_meshcop-e._udp.local”. ... Harness stores the discovered IPv6 address and port for later use; and logs it.
     * - Pass Criteria:
     *   - The DUT MUST respond (mDNS) with a DNS-SD message that contains one PTR record of type
     *     “_meshcop-e._udp.local” in the Answer section.
     *   - Comm_1 MUST be able to obtain an IPv6 address (from AAAA record) and port (from SRV record) of the
     *     “meshcop-e” service as detailed on the left.
     */

    SuccessOrQuit(br1.Get<EphemeralKeyManager>().Start(tap1.mTap, 0, kEpskcPort));

    uint16_t     epskcPort;
    Ip6::Address br1Addr;
    DiscoverMeshcopEService(comm1, nexus, br1Addr, epskcPort);
    Log("Discovered ePSKc address: %s, port: %u", br1Addr.ToString().AsCString(), epskcPort);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Comm_1 connects with incorrect ePSKc.");

    /**
     * Step 6
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect to the DUT on the address and port found in step 5.
     *   For the DTLS PSK, it uses as ePSKc an incorrect ePSKc which is calculated as follows: 1. Seven octets
     *   (b[0]…b[6]) copied from the correct ePSKc of step 4. 2. One octet (b[7]) as a random digit that differs from
     *   the correct ePSKc of step 4. 3. One octet (b[8]) calculated as the correct Verhoeff check digit over the
     *   previous 8 numeric digits. ... Harness then waits for 3 seconds.
     * - Pass Criteria:
     *   - The DTLS handshake/connection to the DUT MUST NOT succeed: the DUT MUST reject attempt to establish a DTLS
     *     session.
     */

    {
        char    incorrectTap[EphemeralKeyManager::Tap::kLength + 1];
        char    checksum;
        uint8_t i = 7;

        memcpy(incorrectTap, tap1.mTap, EphemeralKeyManager::Tap::kLength + 1);
        incorrectTap[i]++;
        if (incorrectTap[i] > '9')
        {
            incorrectTap[i] = '0';
        }
        incorrectTap[EphemeralKeyManager::Tap::kLength - 1] = '\0';
        SuccessOrQuit(Utils::VerhoeffChecksum::Calculate(incorrectTap, checksum));
        incorrectTap[EphemeralKeyManager::Tap::kLength - 1] = checksum;

        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(incorrectTap),
                                                           EphemeralKeyManager::Tap::kLength));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));

        nexus.AdvanceTime(kConnectTimeout);
        VerifyOrQuit(!comm1.Get<Tmf::SecureAgent>().IsConnected());
        comm1.Get<Tmf::SecureAgent>().Close();

        // Wait longer to ensure all retransmissions are finished.
        nexus.AdvanceTime(kDtlsRetransmissionWaitTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Comm_1 connects with correct ePSKc.");

    /**
     * Step 7
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect again to the DUT on the address and port found in
     *   step 5. For the DTLS PSK, it now uses the correct ePSKc as calculated in step 4.
     * - Pass Criteria: The DTLS handshake/connection to the DUT MUST succeed.
     */

    {
        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(tap1.mTap),
                                                           EphemeralKeyManager::Tap::kLength));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));

        nexus.AdvanceTime(kConnectTimeout);
        VerifyOrQuit(comm1.Get<Tmf::SecureAgent>().IsConnected());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Comm_1 sends MGMT_ACTIVE_GET.req.");

    /**
     * Step 8
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to send TMF message MGMT_ACTIVE_GET.req to the DUT over the
     *   secure DTLS session. The Get TLV is not included in the request.
     * - Pass Criteria:
     *   - Comm_1 MUST receive the response TMF message MGMT_ACTIVE_GET.rsp over the secure DTLS session.
     *   - Response payload MUST contain Network Key TLV, ...
     */

    {
        Coap::Message *message;
        message = comm1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriActiveGet);
        VerifyOrQuit(message != nullptr);
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SendMessage(*message));
    }
    nexus.AdvanceTime(kTmfExchangeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Comm_1 sends MGMT_PENDING_GET.req.");

    /**
     * Step 9
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to send MGMT_PENDING_GET.req to the DUT over the secure DTLS
     *   session. The Get TLV is not included in the request.
     * - Pass Criteria:
     *   - Comm_1 MUST receive the TMF response message MGMT_PENDING_GET.rsp.
     *   - Response CoAP payload MUST be empty.
     */

    {
        Coap::Message *message;
        message = comm1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriPendingGet);
        VerifyOrQuit(message != nullptr);
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SendMessage(*message));
    }
    nexus.AdvanceTime(kTmfExchangeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Comm_1 closes the DTLS connection.");

    /**
     * Step 10
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to close the Commissioner session, i.e. close the DTLS
     *   connection. Then, wait for 3 seconds.
     * - Pass Criteria: N/A
     */

    comm1.Get<Tmf::SecureAgent>().Close();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Comm_1 connects again (should fail).");

    /**
     * Step 11
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect again to the DUT on the address and port found in
     *   step 5. For the DTLS PSK, it uses the correct ePSKc as determined in step 4. ...
     * - Pass Criteria:
     *   - The DTLS handshake/connection to the DUT MUST NOT succeed: the DUT MUST reject the DTLS connection attempt.
     */

    {
        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));

        nexus.AdvanceTime(kConnectTimeout);
        VerifyOrQuit(!comm1.Get<Tmf::SecureAgent>().IsConnected());
        comm1.Get<Tmf::SecureAgent>().Close();

        // Wait much longer to ensure all retransmissions are finished and the DUT state is clean.
        // MbedTLS retransmissions can last for a while.
        nexus.AdvanceTime(kDutStateCleanTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Wait 3s and clear mDNS cache.");

    /**
     * Step 12
     * - Device: Harness
     * - Description (CS-12.3): Harness waits for 3 seconds. Harness clears its mDNS discovery cache.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
    // Nexus mDNS cache is handled by AdvanceTime usually or explicitly.
    // In this test, we just clear our browse outcomes.
    sBrowseOutcomes.Clear();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Comm_1 discovers meshcop-e service (should fail).");

    /**
     * Step 13
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to discover a service type via mDNS, Qtype=PTR, name
     *   “_meshcop-e._udp.local”.
     * - Pass Criteria: The DUT MUST NOT respond with an mDNS message that contains a PTR record of type
     *   “_meshcop-e._udp.local” in the Answer section.
     */

    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop-e._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleBrowseCallback;
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }
    VerifyOrQuit(sBrowseOutcomes.GetLength() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: BR_1 (DUT) produces new passcode.");

    /**
     * Step 14
     * - Device: BR_1 (DUT)
     * - Description (CS-12.3): Harness instructs the DUT, via Test Harness API, to produce the Thread Administration
     *   One-Time Passcode.
     * - Pass Criteria: Passcode is successfully retrieved.
     */

    EphemeralKeyManager::Tap tap2;
    SuccessOrQuit(tap2.GenerateRandom());
    Log("Passcode 2: %s", tap2.mTap);
    nexus.AddTestVar("Passcode2", tap2.mTap);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Harness validates the new passcode.");

    /**
     * Step 15
     * - Device: Harness
     * - Description (CS-12.3): Harness validates the passcode. From the passcode, the Harness calculates the ePSKc
     *   using the procedure of step 4.
     * - Pass Criteria:
     *   - Harness validates the following, using its passcode validation function:
     *   - The passcode MUST differ from the passcode previously obtained in step 4
     *   - The passcode MUST contain only digits 0-9
     *   - The passcode MUST be 9 digits long
     *   - The last digit of the passcode MUST be equal to the check digit calculated by the Verhoeff algorithm on
     *     the first 8 digits.
     */

    SuccessOrQuit(tap2.Validate());
    VerifyOrQuit(!StringMatch(tap2.mTap, tap1.mTap));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Comm_1 discovers meshcop-e service.");

    /**
     * Step 16
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to discover a service type via mDNS, Qtype=PTR, name
     *   “_meshcop-e._udp.local”. ... Harness stores the discovered IPv6 address and port for later use; and logs these.
     * - Pass Criteria:
     *   - The DUT MUST respond with an mDNS message that contains one PTR record of type “_meshcop-e._udp.local” in
     *     the Answer section.
     *   - Comm_1 MUST be able to obtain an IPv6 address (from AAAA record) and port (from SRV record) of the
     *     “meshcop-e” service as detailed on the left.
     */

    SuccessOrQuit(br1.Get<EphemeralKeyManager>().Start(tap2.mTap, 0, kEpskcPort + 1));

    DiscoverMeshcopEService(comm1, nexus, br1Addr, epskcPort);
    Log("Discovered ePSKc address: %s, port: %u", br1Addr.ToString().AsCString(), epskcPort);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Comm_1 connects with correct ePSKc.");

    /**
     * Step 17
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect to the DUT on the address and port found in step
     *   16. For the DTLS PSK, it uses the correct ePSKc as calculated in step 15.
     * - Pass Criteria: The DTLS handshake/connection to the DUT MUST succeed.
     */

    {
        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(tap2.mTap),
                                                           EphemeralKeyManager::Tap::kLength));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));

        nexus.AdvanceTime(kConnectTimeout);
        VerifyOrQuit(comm1.Get<Tmf::SecureAgent>().IsConnected());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Comm_1 sends MGMT_ACTIVE_GET.req.");

    /**
     * Step 18
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs the device to send TMF message MGMT_ACTIVE_GET.req to the DUT over
     *   the secure DTLS session. The Get TLV is not included in the request.
     * - Pass Criteria:
     *   - Comm_1 MUST receive the response TMF message MGMT_ACTIVE_GET.rsp over the secure DTLS session.
     *   - Response payload MUST contain Network Key TLV, ...
     */

    {
        Coap::Message *message;
        message = comm1.Get<Tmf::SecureAgent>().AllocateAndInitPriorityConfirmablePostMessage(kUriActiveGet);
        VerifyOrQuit(message != nullptr);
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SendMessage(*message));
    }
    nexus.AdvanceTime(kTmfExchangeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Comm_1 closes the DTLS connection.");

    /**
     * Step 19
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to close the DTLS connection.
     * - Pass Criteria: N/A
     */

    comm1.Get<Tmf::SecureAgent>().Close();
    nexus.AdvanceTime(3 * Time::kOneSecondInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: BR_1 (DUT) produces new passcode.");

    /**
     * Step 20
     * - Device: BR_1 (DUT)
     * - Description (CS-12.3): Harness instructs the DUT, via Test Harness API, to produce the Thread Administration
     *   One-Time Passcode.
     * - Pass Criteria: Passcode is successfully retrieved.
     */

    EphemeralKeyManager::Tap tap3;
    SuccessOrQuit(tap3.GenerateRandom());
    Log("Passcode 3: %s", tap3.mTap);
    nexus.AddTestVar("Passcode3", tap3.mTap);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Harness validates the new passcode.");

    /**
     * Step 21
     * - Device: Harness
     * - Description (CS-12.3): Harness validates the passcode. From the passcode, the Harness calculates the ePSKc
     *   again using the procedure of step 4.
     * - Pass Criteria:
     *   - Harness validates the following, equal to step 4, using its passcode validation function:
     *   - The passcode MUST contain only digits 0-9.
     *   - The passcode MUST be 9 digits long.
     *   - The last digit of the passcode MUST be equal to the check digit calculated by the Verhoeff algorithm on
     *     the first 8 digits.
     *   - Harness also validates that the passcode MUST be different from the passcodes obtained before in steps 4
     *     and 15.
     */

    SuccessOrQuit(tap3.Validate());
    VerifyOrQuit(!StringMatch(tap3.mTap, tap1.mTap));
    VerifyOrQuit(!StringMatch(tap3.mTap, tap2.mTap));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: Comm_1 discovers meshcop-e service.");

    /**
     * Step 22
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to discover a service type via mDNS, Qtype=PTR, name
     *   “_meshcop-e._udp.local”. ... Harness stores the discovered IPv6 address and port for later use; and logs these.
     * - Pass Criteria:
     *   - DUT MUST respond with an mDNS message that contains one PTR record of type “_meshcop-e._udp.local” in the
     *     Answer section.
     *   - Comm_1 MUST be able to obtain an IPv6 address (from AAAA record) and port (from SRV record) of the
     *     “meshcop-e” service as detailed on the left.
     */

    SuccessOrQuit(br1.Get<EphemeralKeyManager>().Start(tap3.mTap, 0, kEpskcPort + 2));

    DiscoverMeshcopEService(comm1, nexus, br1Addr, epskcPort);
    Log("Discovered ePSKc address: %s, port: %u", br1Addr.ToString().AsCString(), epskcPort);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 23: Comm_1 connects with incorrect ePSKc 10 times.");

    /**
     * Step 23
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect to the DUT on the address and port found in step
     *   22. For the DTLS PSK, it uses an incorrect ePSKc as calculated using the procedure of step 6 applied to the
     *   passcode found in step 21. This connection attempt is done in total MAX_EPSKC_CONN_ATTEMPTS (10) times. Each
     *   time, the pass criteria are checked. Between attempts, the Harness pauses for 2 seconds.
     * - Pass Criteria:
     *   - The DTLS handshake/connection to DUT MUST NOT succeed.
     *   - In all cases, the DUT MUST NOT send ICMPv6 Port Unreachable error - this way of failing is not allowed here.
     *     The DUT MUST respond with DTLS messages.
     */

    {
        char    incorrectTap[EphemeralKeyManager::Tap::kLength + 1];
        char    checksum;
        uint8_t i = 7;

        memcpy(incorrectTap, tap3.mTap, EphemeralKeyManager::Tap::kLength + 1);
        incorrectTap[i]++;
        if (incorrectTap[i] > '9')
        {
            incorrectTap[i] = '0';
        }
        incorrectTap[EphemeralKeyManager::Tap::kLength - 1] = '\0';
        SuccessOrQuit(Utils::VerhoeffChecksum::Calculate(incorrectTap, checksum));
        incorrectTap[EphemeralKeyManager::Tap::kLength - 1] = checksum;

        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(incorrectTap),
                                                           EphemeralKeyManager::Tap::kLength));

        for (uint8_t attempt = 1; attempt <= kMaxEpskcConnAttempts; attempt++)
        {
            Log("Attempt %u with incorrect ePSKc.", attempt);
            SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
            SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));
            nexus.AdvanceTime(kConnectTimeout);
            VerifyOrQuit(!comm1.Get<Tmf::SecureAgent>().IsConnected());
            comm1.Get<Tmf::SecureAgent>().Close();
            nexus.AdvanceTime(2 * Time::kOneSecondInMsec);
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 24: Comm_1 connects with correct ePSKc (should fail).");

    /**
     * Step 24
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to connect again to the DUT on the address and port found in
     *   step 22. For the DTLS PSK, it uses the correct ePSKc as calculated in step 21. But the connection still fails
     *   due to the maximum number of attempts passed.
     * - Pass Criteria:
     *   - The DTLS handshake/connection to DUT MUST NOT succeed.
     *   - Note: the reason that the correct ePSKc does not succeed here is that the maximum number of failed
     *     connection attempts has been reached. This is being tested here.
     */

    {
        Ip6::SockAddr br1SockAddr;
        br1SockAddr.SetAddress(br1Addr);
        br1SockAddr.SetPort(epskcPort);

        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().SetPsk(reinterpret_cast<const uint8_t *>(tap3.mTap),
                                                           EphemeralKeyManager::Tap::kLength));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Open(Ip6::kNetifBackbone));
        SuccessOrQuit(comm1.Get<Tmf::SecureAgent>().Connect(br1SockAddr));

        nexus.AdvanceTime(kConnectTimeout);
        VerifyOrQuit(!comm1.Get<Tmf::SecureAgent>().IsConnected());
        comm1.Get<Tmf::SecureAgent>().Close();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 25: Comm_1 clears its mDNS discovery cache.");

    /**
     * Step 25
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs device to clear its mDNS discovery cache.
     * - Pass Criteria: N/A
     */

    sBrowseOutcomes.Clear();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 26: Comm_1 discovers meshcop-e service (should fail).");

    /**
     * Step 26
     * - Device: Comm_1
     * - Description (CS-12.3): Harness instructs the device to discover a service type via mDNS, Qtype=PTR, name
     *   “_meshcop-e._udp.local”.
     * - Pass Criteria: The DUT MUST NOT respond with an mDNS message that contains a PTR record of type
     *   “_meshcop-e._udp.local” in the Answer section.
     */

    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mServiceType  = "_meshcop-e._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        browser.mCallback     = HandleBrowseCallback;
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kMdnsDiscoveryTime);
        SuccessOrQuit(comm1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }
    VerifyOrQuit(sBrowseOutcomes.GetLength() == 0);

    nexus.SaveTestInfo("test_1_4_CS_TC_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_4_CS_TC_3();
    printf("All tests passed\n");
    return 0;
}
