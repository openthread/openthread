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

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 20 * 1000;

/**
 * Time to advance for SRP server information to be updated in Network Data.
 */
static constexpr uint32_t kSrpServerInfoUpdateTime = 20 * 1000;

/**
 * Time to advance for SRP registration, in milliseconds.
 */
static constexpr uint32_t kSrpRegistrationTime = 10 * 1000;

/**
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP service registration parameters.
 */
static const char         kSrpServiceType[]     = "_thread-test._udp";
static const char         kSrpInstanceName1[]   = "service-test-1";
static const char         kSrpInstanceName3[]   = "service-test-3";
static const char         kSrpHostName1[]       = "host-test-1";
static const char         kSrpHostName2[]       = "host-test-2";
static const char         kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort       = 55555;

/**
 * Lease intervals in seconds.
 */
static constexpr uint32_t kLease30s    = 30;
static constexpr uint32_t kLease10s    = 10;
static constexpr uint32_t kKeyLease60s = 60;
static constexpr uint32_t kKeyLease30s = 30;

/**
 * Wait times in milliseconds.
 */
static constexpr uint32_t kWait30s = 30 * 1000;
static constexpr uint32_t kWait35s = 35 * 1000;

static void RegisterSrpService(Node                 &aNode,
                               Srp::Client::Service &aService,
                               const char           *aHostName,
                               const char           *aInstanceName,
                               uint32_t              aLease,
                               uint32_t              aKeyLease,
                               uint16_t              aWeight,
                               uint16_t              aPriority)
{
    aNode.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(aNode.Get<Srp::Client>().SetHostName(aHostName));
    SuccessOrQuit(aNode.Get<Srp::Client>().EnableAutoHostAddress());
    aNode.Get<Srp::Client>().SetLeaseInterval(aLease);
    aNode.Get<Srp::Client>().SetKeyLeaseInterval(aKeyLease);

    ClearAllBytes(aService);
    aService.mName         = kSrpServiceType;
    aService.mInstanceName = aInstanceName;
    aService.mPort         = kSrpServicePort;
    aService.mWeight       = aWeight;
    aService.mPriority     = aPriority;
    SuccessOrQuit(aNode.Get<Srp::Client>().AddService(aService));
}

void Test_1_3_SRP_TC_4(const char *aJsonFileName)
{
    Srp::Client::Service          service;
    Dns::Multicast::Core::Browser browser;

    /**
     * 2.4. [1.3] [CERT] Key Lease
     *
     * 2.4.1. Purpose
     * To test the following:
     * - 1. Key lease handling on SRP server on DUT.
     * - 2. Verify that a service instance name cannot be claimed even after the service instance lease expired, if the
     *   key lease is still active.
     *
     * 2.4.2. Topology
     * - BR 1 (DUT) - Thread Border Router and the Leader
     * - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - ED 2-Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Server       | N/A          | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &ed2  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    ed2.SetName("ED_2");
    eth1.SetName("Eth_1");

    br1.Form();
    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.4): Enable: switch on.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR_1 (DUT) enable.");
    {
        Srp::Server::LeaseConfig leaseConfig;
        br1.Get<Srp::Server>().GetLeaseConfig(leaseConfig);
        leaseConfig.mMinLease = kLease30s;
        SuccessOrQuit(br1.Get<Srp::Server>().SetLeaseConfig(leaseConfig));
    }
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     * - Device: Eth 1, ED 1, ED 2
     * - Description (SRP-2.4): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1, ED 2 enable.");
    ed1.Join(br1, Node::kAsFed);
    ed2.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and EDs to get an OMR address
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.4): Automatically adds its SRP Server information in the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data: it MUST contain a DNS/SRP Unicast
     *     Dataset, or a DNS/SRP Anycast Dataset, or both.
     */
    Log("Step 3: BR_1 (DUT) adds its SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     * - Device: ED 1
     * - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
     *   following options: Update Lease Option Lease: 30 seconds Key Lease: 60 seconds
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update.");
    RegisterSrpService(ed1, service, kSrpHostName1, kSrpInstanceName1, kLease30s, kKeyLease60s, 1, 1);
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends SRP Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Response with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Response.");

    /**
     * Step 6
     * - Device: ED_1
     * - Description (SRP-2.4): Harness instructs the device to unicast DNS query QType=SRV for
     *   "service-test-1._thread-test_udp.default.service.arpa".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 sends unicast DNS query QType=SRV.");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName1, kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends DNS response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response containing answer record: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 )
     *   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
     *     <OMR address of ED_1>
     */
    Log("Step 7: BR 1 (DUT) sends DNS response.");

    /**
     * Step 8
     * - Device: ED 1
     * - Description (SRP-2.4): Harness instructs the device to send unicast DNS query QType=PTR for any services of
     *   type _thread-test_udp.default.service.arpa.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: ED 1 sends unicast DNS query QType=PTR.");
    SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 9
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends DNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response containing answer record: $ORIGIN default.service.arpa.
     *     _thread-test._udp ( PTR service-test-1._thread-test._udp )
     *   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
     */
    Log("Step 9: BR 1 (DUT) sends DNS Response.");

    /**
     * Step 10
     * - Device: N/A
     * - Description (SRP-2.4): Harness waits for 35 seconds. Note: during this time there must be no automatic
     *   reregistration of the service done by ED 1. This can be implemented on (OpenThread) ED_1 by either disabling
     *   automatic service reregistration or by internally removing the service without doing the deregistration
     *   request.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: Wait 35 seconds.");
    ed1.Get<Srp::Client>().ClearHostAndServices();
    nexus.AdvanceTime(kWait35s);

    /**
     * Step 11
     * - Device: ED 2
     * - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-2 ) host-test-2 AAAA <OMR address of ED_2> with the
     *   following options: Update Lease Option Lease: 30 seconds Key Lease: 30 seconds
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 11: ED 2 sends SRP Update.");
    RegisterSrpService(ed2, service, kSrpHostName2, kSrpInstanceName1, kLease30s, kKeyLease30s, 0, 0);
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 12
     * - Device: BR 1. (DUT)
     * - Description (SRP-2.4): Automatically sends SRP Update Response, denying the registration because the key lease
     *   for the expired service 'service-test-1' is still active. Note: spec for this is 3.2.5.2, 3.3.3, 3.3.5 and 5.1
     *   of [draft-srp-23].
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=6 (YXDOMAIN).
     */
    Log("Step 12: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).");

    /**
     * Step 13
     * - Device: N/A
     * - Description (SRP-2.4): Harness waits for 30 seconds. Note: during this time there must be no automatic
     *   reregistration of the service done by ED 2. This can be implemented on (Open Thread) ED_2 by either disabling
     *   automatic service reregistration or by internally removing the service without doing the deregistration
     *   request.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 13: Wait 30 seconds.");
    ed2.Get<Srp::Client>().ClearHostAndServices();
    nexus.AdvanceTime(kWait30s);

    /**
     * Step 14
     * - Device: ED 2
     * - Description (SRP-2.4): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_2> Update
     *   Lease Option Lease: 10 seconds Key Lease: 60 seconds Note: ED 2 here uses the hostname of ED 1 to verify that
     *   the name host-test-1 has become available again, after key expiry thereaf. Note: the requested Lease time of
     *   10 seconds will be automatically adjusted by the SRP Server to 30 seconds (the minimum allowed).
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 14: ED 2 sends SRP Update with host-test-1.");
    RegisterSrpService(ed2, service, kSrpHostName1, kSrpInstanceName1, kLease10s, kKeyLease60s, 1, 1);
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 15
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Response with RCODE = 0 (NoError).
     */
    Log("Step 15: BR 1 (DUT) sends SRP Response.");

    /**
     * Step 16
     * - Device: Eth_1
     * - Description (SRP-2.4): Harness instructs the device to send mDNS query QType PTR for any services of type
     *   "_thread-test._udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Eth 1 sends mDNS query QType PTR.");
    {
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = "_thread-test._udp";
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 17
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response containing only answer record: $ORIGIN local. _thread-test._udp (
     *     PTR service-test-1._thread-test._udp )
     *   - The mDNS Response MAY contain in Additional records section: $ORIGIN local.
     *     Service-test-1._thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_2>
     */
    Log("Step 17: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 18
     * - Device: N/A
     * - Description (SRP-2.4): Harness waits for 35 seconds, for the service lease to expire.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 18: Wait 35 seconds.");
    ed2.Get<Srp::Client>().ClearHostAndServices();
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    nexus.AdvanceTime(kWait35s);

    /**
     * Step 19
     * - Device: Eth_1
     * - Description (SRP-2.4): Harness instructs the device to send mDNS query QType PTR for any services of type
     *   "_thread-test_udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 19: Eth 1 sends mDNS query QType PTR.");
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 20
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Does not send mDNS Response, since the service with matching type has expired. Note:
     *   although the key leases on service-test-1 and host-test-1 are still active now, the generic service pointer is
     *   expired and therefore no result to the PTR query is given. There is no record with the generic name stored,
     *   anymore.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an mDNS Response.
     */
    Log("Step 20: BR 1 (DUT) should not send mDNS Response.");

    /**
     * Step 21
     * - Device: ED 1
     * - Description (SRP-2.4): Harness instructs the device to send SRP Update with different service name but
     *   conflicting host name: $ORIGIN default.service.arpa. service-test-3._thread-test._udp ( SRV 0 0 55555
     *   host-test-1 ) host-test-1 AAAA <OMR address of ED_1> With the following options: Update Lease Option Lease:
     *   30 seconds Key Lease: 30 seconds
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 21: ED 1 sends SRP Update with conflicting host name.");
    RegisterSrpService(ed1, service, kSrpHostName1, kSrpInstanceName3, kLease30s, kKeyLease30s, 0, 0);
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 22
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.4): Automatically sends SRP Update Response, denying the registration because host
     *   'host-test-1' is already claimed by ED_2 for its key lease time.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE = 6 (YXDOMAIN).
     */
    Log("Step 22: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).");

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_2_MLEID_ADDR", ed2.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    {
        Ip6::Prefix omrPrefix;
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
        nexus.AddTestVar("ED_1_OMR_ADDR",
                         ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
        nexus.AddTestVar("ED_2_OMR_ADDR",
                         ed2.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
    }

    {
        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT", srpPortString.AsCString());
    }

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_4((argc > 2) ? argv[2] : "test_1_3_SRP_TC_4.json");

    printf("All tests passed\n");
    return 0;
}
