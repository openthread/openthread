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
static constexpr uint32_t kBrActionTime = 30 * 1000;

/**
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 10 * 1000;

/**
 * Time to wait for DNS response.
 */
static constexpr uint32_t kDnsResponseTime = 20 * 1000;

/**
 * Time to wait for mDNS query.
 */
static constexpr uint32_t kMdnsQueryTime = 1 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Service Port.
 */
static constexpr uint16_t kServicePort = 55555;

/**
 * SRP Service Weight.
 */
static constexpr uint16_t kSrpWeight = 4;

void Test_1_3_DPR_TC_2(void)
{
    /**
     * 5.2. 1.3 CERT Service discovery of services on Thread and Infrastructure - Multiple BRs - Multiple Thread -
     *   Single infrastructure
     *
     * 5.2.1. Purpose
     *   To test the following:
     *   - 1. Discovery of on-mesh services between multiple Thread networks attached via an adjacent
     *     infrastructure link.
     *   - 2. BR Discovery Proxy function can discover services advertised by another BR’s Advertising Proxy
     *     function, and vice-versa.
     *   - 3. Test both PTR and SRV queries from Thread Network, and finding service in other Network.
     *
     * 5.2.2. Topology
     *   - BR_1 (DUT) - Thread Border Router, and Leader
     *   - BR_2 - Test Bed border router device operating as a Thread Border Router and Leader of an adjacent
     *     Thread network
     *   - ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
     *   - ED_2 - Test Bed device operating as a Thread End Device, attached to BR_2
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Discovery Proxy  | N/A          | 5.2
     */

    Core nexus;

    Node &br1 = nexus.CreateNode();
    Node &br2 = nexus.CreateNode();
    Node &ed1 = nexus.CreateNode();
    Node &ed2 = nexus.CreateNode();

    Srp::Client::Service service1;
    Srp::Client::Service service2;
    Ip6::Address         ed1Addresses[2];

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");
    ed2.SetName("ED_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    {
        Ip6::Prefix        prefix;
        Ip6::NetworkPrefix networkPrefix;

        SuccessOrQuit(prefix.FromString("fd00:db8::/64"));
        SuccessOrQuit(networkPrefix.SetFrom(prefix));
        br1.Get<Mle::Mle>().SetMeshLocalPrefix(networkPrefix);
        br2.Get<Mle::Mle>().SetMeshLocalPrefix(networkPrefix);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: BR_1 (DUT), ED_1 Description (DPR-5.2): Enable");

    /**
     * Step 1
     * - Device: BR_1 (DUT), ED_1
     * - Description (DPR-5.2): Enable
     * - Pass Criteria:
     *   - N/A
     */

    AllowLinkBetween(br1, ed1);

    br1.Form();

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        nexus.AddNetworkKey(datasetInfo.Get<MeshCoP::Dataset::kNetworkKey>());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR_1 (DUT) Description (DPR-5.2): Automatically adds its SRP Server information...");

    /**
     * Step 2
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.2): Automatically adds its SRP Server information in the Thread Network Data of first
     *     Thread network.
     * - Pass Criteria:
     *   - The DUT’s SRP Server information MUST appear in the Thread Network Data of the first Thread network.
     */

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: ED_1 Description (DPR-5.2): Harness instructs the device to send SRP Update...");

    /**
     * Step 3
     * - Device: ED_1
     * - Description (DPR-5.2): Harness instructs the device to send SRP Update $ORIGIN default.service.arpa.
     *     service-test-1._thread-type-1._udp ( SRV 0 4 55555 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
     *     AAAA <OMR address of ED_1> Note: normally an SRP client would only register the OMR address in this case.
     *     For testing purposes, both OMR/ML-EID are registered which deviates from usual client behavior.
     * - Pass Criteria:
     *   - N/A
     */

    ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName("host-test-1"));

    ed1Addresses[0] = ed1.Get<Mle::Mle>().GetMeshLocalEid();
    ed1Addresses[1] = ed1.FindGlobalAddress();
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostAddresses(ed1Addresses, 2));

    ClearAllBytes(service1);
    service1.mName         = "_thread-type-1._udp";
    service1.mInstanceName = "service-test-1";
    service1.mPort         = kServicePort;
    service1.mPriority     = 0;
    service1.mWeight       = kSrpWeight;
    SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service1));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends SRP Update Response");

    /**
     * Step 4
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
     */

    VerifyOrQuit(ed1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: BR_2, ED_2 Description (DPR-5.2): Enable");

    /**
     * Step 5
     * - Device: BR_2, ED_2
     * - Description (DPR-5.2): Enable
     * - Pass Criteria:
     *   - N/A
     */

    AllowLinkBetween(br2, ed2);

    UnallowLinkBetween(br1, br2);
    br1.UnallowList(ed2);
    ed1.UnallowList(br2);
    ed1.UnallowList(ed2);

    br2.Form();

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(br2.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        nexus.AddNetworkKey(datasetInfo.Get<MeshCoP::Dataset::kNetworkKey>());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsLeader());

    ed2.Join(br2, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(ed2.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: BR_2 Description (DPR-5.2): Automatically adds its SRP Server information...");

    /**
     * Step 6
     * - Device: BR_2
     * - Description (DPR-5.2): Automatically adds its SRP Server information in the Thread Network Data of the
     *     second Thread network.
     * - Pass Criteria:
     *   - BR_2’s SRP Server information MUST appear in the Thread Network Data of the second Thread network.
     */

    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br2.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: ED_2 Description (DPR-5.2): Harness instructs the device to send SRP Update...");

    /**
     * Step 7
     * - Device: ED_2
     * - Description (DPR-5.2): Harness instructs the device to send SRP Update $ORIGIN default.service.arpa.
     *     service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
     * - Pass Criteria:
     *   - N/A
     */

    ed2.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(ed2.Get<Srp::Client>().SetHostName("host-test-2"));
    SuccessOrQuit(ed2.Get<Srp::Client>().EnableAutoHostAddress());

    ClearAllBytes(service2);
    service2.mName         = "_thread-type-2._udp";
    service2.mInstanceName = "service-test-2";
    service2.mPort         = kServicePort;
    service2.mPriority     = 0;
    service2.mWeight       = 0;
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(service2));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: BR_2 Description (DPR-5.2): Automatically sends SRP Update Response");

    /**
     * Step 8
     * - Device: BR_2
     * - Description (DPR-5.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - SRP Update Response is valid and reports NoError (RCODE=0).
     */

    VerifyOrQuit(ed2.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: ED_1 Description (DPR-5.2): Harness instructs the device to unicast DNS query...");

    /**
     * Step 9
     * - Device: ED_1
     * - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=PTR to the DUT for all
     *     services of type “_thread-type-2._udp.default.service.arpa.”.
     * - Pass Criteria:
     *   - N/A
     */

    {
        Dns::Client::QueryConfig queryConfig;
        queryConfig.Clear();
        AsCoreType(&queryConfig.mServerSockAddr.mAddress) = br1.Get<Mle::Mle>().GetMeshLocalEid();
        queryConfig.mServerSockAddr.mPort                 = Dns::ServiceDiscovery::Server::kPort;

        SuccessOrQuit(
            ed1.Get<Dns::Client>().Browse("_thread-type-2._udp.default.service.arpa.", nullptr, nullptr, &queryConfig));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends DNS Response");

    /**
     * Step 10
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.2): Automatically sends DNS Response. Note: during this step, DUT performs mDNS query to
     *     BR_2 which responds with service-test-2. This isn’t shown in detail.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response, containing service-test-2 as Answer record: $ORIGIN
     *     default.service.arpa. _thread-type-2._udp ( PTR service-test-2._thread-type-2._udp )
     *   - It MAY contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
     */

    nexus.AdvanceTime(kDnsResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Device: ED_2 Description (DPR-5.2): Harness instructs the device to unicast DNS query...");

    /**
     * Step 11
     * - Device: ED_2
     * - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=PTR to BR_2 for all services
     *     of type “_thread-type-1._udp.default.service.arpa.”.
     * - Pass Criteria:
     *   - N/A
     */

    {
        Dns::Client::QueryConfig queryConfig;
        queryConfig.Clear();
        AsCoreType(&queryConfig.mServerSockAddr.mAddress) = br2.Get<Mle::Mle>().GetMeshLocalEid();
        queryConfig.mServerSockAddr.mPort                 = Dns::ServiceDiscovery::Server::kPort;

        SuccessOrQuit(
            ed2.Get<Dns::Client>().Browse("_thread-type-1._udp.default.service.arpa.", nullptr, nullptr, &queryConfig));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Device: BR_2 Description (DPR-5.2): Automatically sends mDNS query");

    /**
     * Step 12
     * - Device: BR_2
     * - Description (DPR-5.2): Automatically sends mDNS query.
     * - Pass Criteria:
     *   - N/A
     */

    nexus.AdvanceTime(kMdnsQueryTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Device: BR_1 (DUT) Description (DPR-5.2): Automatically responds to mDNS query...");

    /**
     * Step 13
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.2): Automatically responds to mDNS query with “service-test-1”.
     * - Pass Criteria:
     *   - N/A (verified in step 14)
     */

    nexus.AdvanceTime(kDnsResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Device: BR_2 Description (DPR-5.2): Automatically sends a DNS response to ED_2");

    /**
     * Step 14
     * - Device: BR_2
     * - Description (DPR-5.2): Automatically sends a DNS response to ED_2.
     * - Pass Criteria:
     *   - The DNS Response is received at ED_2, is valid and contains only service-test-1 in the Answers section:
     *     $ORIGIN default.service.arpa. _thread-type-1._udp ( PTR service-test-1._thread-type-1._udp )
     *   - It MAY contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service-test-1._thread-type-1._udp ( SRV 0 4 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
     *   - Also it MAY contain in the Additional records section, but only if the above Additional records are also
     *     included: $ORIGIN default.service.arpa. host-test-1 AAAA <ML-EID address of ED_1>
     */

    // Note: verification of DNS response from BR_2 to ED_2 is handled in Step 14.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Device: ED_1 Description (DPR-5.2): Harness instructs the device to unicast DNS query...");

    /**
     * Step 15
     * - Device: ED_1
     * - Description (DPR-5.2): Harness instructs the device to unicast DNS query QType=SRV to the DUT for service
     *     “service-test-2._thread-type-2._udp.default.service.arpa.”.
     * - Pass Criteria:
     *   - N/A
     */

    {
        Dns::Client::QueryConfig queryConfig;
        queryConfig.Clear();
        AsCoreType(&queryConfig.mServerSockAddr.mAddress) = br1.Get<Mle::Mle>().GetMeshLocalEid();
        queryConfig.mServerSockAddr.mPort                 = Dns::ServiceDiscovery::Server::kPort;

        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(
            "service-test-2", "_thread-type-2._udp.default.service.arpa.", nullptr, &queryConfig));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Device: BR_1 (DUT) Description (DPR-5.2): Automatically sends DNS Response");

    /**
     * Step 16
     * - Device: BR_1 (DUT)
     * - Description (DPR-5.2): Automatically sends DNS Response. Note: during this step, DUT MAY perform mDNS query
     *     to BR_2 which responds with service-test-2. This isn’t shown in detail.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response is valid, containing service-test-2 answer record: $ORIGIN
     *     default.service.arpa. service-test-2._thread-type-2._udp ( SRV 0 0 55555 host-test-2 )
     *   - It MAY contain in the Additional records section: host-test-2 AAAA <OMR address of ED_2>
     */

    nexus.AdvanceTime(kDnsResponseTime);

    nexus.AddTestVar("ED_1_OMR", ed1.FindGlobalAddress().ToString().AsCString());
    nexus.AddTestVar("ED_2_OMR", ed2.FindGlobalAddress().ToString().AsCString());

    nexus.SaveTestInfo("test_1_3_DPR_TC_2.json", &br1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DPR_TC_2();
    printf("All tests passed\n");
    return 0;
}
