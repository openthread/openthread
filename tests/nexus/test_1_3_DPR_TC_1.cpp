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

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "net/mdns.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 20 * 1000;

/**
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 5 * 1000;

/**
 * Time to wait for DNS response.
 */
static constexpr uint32_t kDnsResponseTime = 20 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Service and Host names.
 */
static const char kInfraServiceType[]   = "_infra-test._udp";
static const char kInfraHostName[]      = "host-test-eth";
static const char kDiscoveryProxyType[] = "_infra-test._udp.default.service.arpa.";

struct ServiceInfo
{
    const char *mInstanceName;
    uint16_t    mPort;
    const char *mTxtData;
};

static const ServiceInfo kInfraServices[] = {
    {"service-test-1", 55551, "\x0Adummy=abcd\x0Fthread-test=a49"},
    {"service-test-2", 55552, "\x04a=2\x12thread-test=b50_test"},
    {"service-test-3", 55553, "\x0Ba_dummy=42\x0Cthread-test="},
    {"service-test-4", 55554, "\x0Dthread-test=1\x15ignorethis=thread-test="},
    {"service-test-5", 55555,
     "\x0B"
     "a88=thread-\x10THREAD-TEST=C51\x14ignorethis=thread-test"},
};

static void RegisterMdnsServices(Node &aEth1)
{
    Dns::Multicast::Core &mdns = aEth1.Get<Dns::Multicast::Core>();

    SuccessOrQuit(mdns.SetEnabled(true, kInfraIfIndex));

    {
        Dns::Multicast::Core::Host host;

        ClearAllBytes(host);
        host.mHostName        = kInfraHostName;
        host.mAddresses       = &aEth1.mInfraIf.GetAddresses()[1]; // ULA address
        host.mAddressesLength = 1;
        SuccessOrQuit(mdns.RegisterHost(host, 0, nullptr));
    }

    for (const ServiceInfo &info : kInfraServices)
    {
        Dns::Multicast::Core::Service service;

        ClearAllBytes(service);
        service.mServiceInstance = info.mInstanceName;
        service.mServiceType     = kInfraServiceType;
        service.mHostName        = kInfraHostName;
        service.mPort            = info.mPort;
        service.mTxtData         = reinterpret_cast<const uint8_t *>(info.mTxtData);
        service.mTxtDataLength   = static_cast<uint16_t>(strlen(info.mTxtData));
        SuccessOrQuit(mdns.RegisterService(service, 0, nullptr));
    }
}

void Test_1_3_DPR_TC_1(const char *aJsonFileName)
{
    /**
     * 5.1. [1.3] [CERT] Service discovery of services on Thread and Infrastructure - Single BR
     *
     * 5.1.1 Purpose
     * To test the following:
     * 1. Discovery proxy on the BR to perform mDNS query on the infrastructure link in the case where it doesn’t have
     *   any answer.
     * 2. Respond to unicast DNS queries made by a Thread Device on the Thread interface.
     * 3. Thread Device can discover services advertised by infrastructure device (mDNS) via BR DUT.
     * 4. Single BR case
     *
     * 5.1.2 Topology
     * - BR_1 (DUT) - Thread Border Router, and the Leader
     * - ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference      | V1.1 Section | V1.3.0 Section
     * --------------------|--------------|---------------
     * Discovery Proxy     | N/A          | 5.1
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    Srp::Client::Service service;
    Dns::TxtEntry        txtEntry;

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: BR_1 (DUT) Description (DPR-5.1): Enable");

    /**
     * Step 1
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.1): Enable
     * - Pass Criteria: N/A
     */

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: Eth_1, ED_1 Description (DPR-5.1): Enable");

    /**
     * Step 2
     * - Device: Eth_1, ED_1
     * - Description (DPR-5.1): Enable
     * - Pass Criteria: N/A
     */

    eth1.mInfraIf.Init(eth1);

    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR_1 (DUT) Description (DPR-5.1): Automatically adds its SRP Server information...");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.1): Automatically adds its SRP Server information in the Thread Network Data.
     *   Automatically configures an OMR prefix on the Thread Network. Automatically configures a ULA prefix on the AIL.
     * - Pass Criteria:
     *   - The DUT’s SRP Server information MUST appear in the Thread Network Data.
     *   - It MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
     */

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omrPrefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
    nexus.AddTestVar("OMR_PREFIX", omrPrefix.ToString().AsCString());

    Ip6::Prefix pre1Prefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(pre1Prefix));
    nexus.AddTestVar("PRE_1_PREFIX", pre1Prefix.ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: Eth_1 Description (DPR-5.1): Harness instructs device to advertise N=5 services...");

    /**
     * Step 4
     * - Device: Eth_1
     * - Description (DPR-5.1): Harness instructs device to advertise N=5 services using mDNS.
     * - Pass Criteria: N/A
     */

    RegisterMdnsServices(eth1);
    nexus.AdvanceTime(kDnsResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: ED_1 Description (DPR-5.1): Harness instructs the device to send an SRP Update...");

    /**
     * Step 5
     * - Device: ED_1
     * - Description (DPR-5.1): Harness instructs the device to send an SRP Update to DUT as follows:
     *   $ORIGIN default.service.arpa. service-test-6._thread-test._udp ( SRV 0 0 55551 host-test-1 TXT
     *   thread-te=trick=value ) host-test-1 AAAA <OMR address of ED_1>
     * - Pass Criteria: N/A
     */

    ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName("host-test-1"));
    SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

    ClearAllBytes(service);
    service.mName         = "_thread-test._udp";
    service.mInstanceName = "service-test-6";
    service.mPort         = 55551;

    txtEntry.mKey         = "thread-te";
    txtEntry.mValue       = reinterpret_cast<const uint8_t *>("trick=value");
    txtEntry.mValueLength = 11;

    service.mTxtEntries    = &txtEntry;
    service.mNumTxtEntries = 1;

    SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: BR_1 (DUT) Description (DPR-5.1): Automatically sends SRP Update Response with SUCCESS");

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.1): Automatically sends SRP Update Response with “SUCCESS”.
     * - Pass Criteria:
     *   - The DUT MUST send SRP Update Response with status 0 ( “NoError” ).
     */

    VerifyOrQuit(ed1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: ED_1 Description (DPR-5.1): Harness instructs the device to unicast DNS query QType=PTR...");

    /**
     * Step 7
     * - Device: ED_1
     * - Description (DPR-5.1): Harness instructs the device to unicast DNS query QType=PTR to the DUT for services
     *   of type “_infra-test._udp.default.service.arpa.”
     * - Pass Criteria: N/A
     */

    const Ip6::Address      &br1Mleid = br1.Get<Mle::Mle>().GetMeshLocalEid();
    Dns::Client::QueryConfig queryConfig;

    queryConfig.Clear();
    AsCoreType(&queryConfig.mServerSockAddr.mAddress) = br1Mleid;
    queryConfig.mServerSockAddr.mPort                 = Dns::ServiceDiscovery::Server::kPort;

    SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kDiscoveryProxyType, nullptr, nullptr, &queryConfig));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: BR_1 (DUT) Description (DPR-5.1): Automatically MAY send mDNS query...");

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.1): Automatically MAY send mDNS query on the infrastructure link.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: Eth_1 Description (DPR-5.1): In case DUT queried in step 8, it automatically sends mDNS...");

    /**
     * Step 9
     * - Device: Eth_1
     * - Description (DPR-5.1): In case DUT queried in step 8, it automatically sends mDNS response with its active
     *   services.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDnsResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: BR_1 (DUT) Description (DPR-5.1): Automatically sends a DNS response to ED_1");

    /**
     * Step 10
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.1): Automatically sends a DNS response to ED_1.
     * - Pass Criteria: N/A (validation of response in the next step)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Device: ED_1 Description (DPR-5.1): Receives the DNS response with at least one of the N services");

    /**
     * Step 11
     * - Device: ED_1
     * - Description (DPR-5.1): Receives the DNS response with at least one of the N services.
     * - Pass Criteria:
     *   - The DUT must send a valid DNS Response, containing in the Answer records section at least one of the services
     *     advertised by Eth_1, specifically the below where N is a number out of 1, 2, 3, 4, 5:
     *     $ORIGIN default.service.arpa. _infra-test._udp ( PTR service-test-N._infra-test._udp )
     *   - It MAY contain in the Additional records section one or more of the below services identified by number N
     *     plus the host (AAAA) record:
     *     $ORIGIN default.service.arpa. service-test-1._infra-test._udp ( SRV 0 0 55551 host-test-eth TXT
     *     dummy=abcd;thread-test=a49 ) ... host-test-eth AAAA <ULA address of Eth_1>
     *   - Response MUST NOT contain any AAAA record with a link-local address.
     *   - Response MUST NOT contain “service-test-6” records.
     *   - Response MUST NOT contain any “host-test-1” records.
     */

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFile = (argc > 1) ? argv[argc - 1] : "test_1_3_DPR_TC_1.json";

    ot::Nexus::Test_1_3_DPR_TC_1(jsonFile);
    printf("All tests passed\n");
    return 0;
}
