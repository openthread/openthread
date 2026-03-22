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
static const char         kSrpInstanceName[]    = "service-test-1";
static const char         kSrpHostName[]        = "host-test-1";
static const char         kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort       = 55555;
static constexpr uint32_t kLease                = 20 * 60; // 20 minutes
static constexpr uint32_t kKeyLease             = 20 * 60; // 20 minutes

void Test_1_3_SRP_TC_6(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.6. [1.3] [CERT] Name Compression
     *
     * 2.6.1. Purpose
     * To test the following:
     * - 1. Handle RRs without name compression.
     * - 2. Handle RRs with name compression
     *
     * 2.6.2. Topology
     * - 1. BR 1 (DUT) - Thread Border Router and the Leader
     * - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference | V1.1 Section | V1.3.0 Section
     * ---------------|--------------|---------------
     * SRP Server     | N/A          | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    br1.Form();
    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.6): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) Enable.");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     * - Device: Eth 1. ED 1
     * - Description (SRP-2.6): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable.");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and ED to get an OMR address
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.6): Automatically adds SRP Server information in the Thread Network Data. Automatically
     *   adds OMR prefix to the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data.
     */
    Log("Step 3: BR 1 (DUT) adds SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     * - Device: ED 1
     * - Description (SRP-2.6): Harness instructs the device to send SRP Update without name compression: $ORIGIN
     *   default.service.arpa. _thread-test. udp PTR service-test-1._thread-test._udp
     *   service-test-1._thread-test._udp ( SRV 55555 host-test-1) host-test-1 AAAA OMR address of ED 1> with the
     *   following options: Update Lease Option Lease: 20 minutes Key Lease: 20 minutes
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update without name compression.");
    {
        SuccessOrQuit(ed1.SetDnsNameCompressionEnabled(false));
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

        ed1.Get<Srp::Client>().SetLeaseInterval(kLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kKeyLease);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.6): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with Rcode=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response.");

    /**
     * Step 6
     * - Device: ED 1
     * - Description (SRP-2.6): Harness instructs the device to unicast DNS QType PTR query to the DUT for all
     *   services of type \"_thread-test._udp.local.\".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 sends unicast DNS QType PTR query.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.6): Automatically sends DNS response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response with RCODE=0 (NoError), containing in the Answers section: $ORIGIN
     *     default.service.arpa. _thread-test. udp PTR service-test-1. thread-test._udp
     *   - It MAY contain in the Additional records section: $ORIGIN local. Service-test-1_thread-test._udp ( SRV 0
     *     55555 host-test-1) testAAAA <OMR address of ED_1> host--1
     */
    Log("Step 7: BR 1 (DUT) sends DNS response.");

    /**
     * Step 8
     * - Device: Eth_1
     * - Description (SRP-2.6): Hamess instructs the device to send mDNS query QType=PTR query for all services of
     *   type \"_thread-test._udp.local\"..
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth 1 sends mDNS query QType=PTR.");
    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = kSrpServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 9
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.6): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response with Rcode=0 (NoError), containing in the Answers section: $ORIGIN
     *     local. _thread-test. udp PTR service-test-1._thread-test._udp
     *   - It MAY contain in the Additional records section: $ORIGIN local. Service-test-1. thread-test._udp SRV 55555
     *     host-test-1) host-test-1 AAAA OMR address of ED_1>
     */
    Log("Step 9: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 9b
     * - Device: Eth 1
     * - Description (SRP-2.6): Harness instructs the device to send mDNS query QTYPE=SRV
     *   QNAME-service-test-1._thread-test._udp.local
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response with RCODE=0, containing in the Answers section record:
     *     service-test-1. thread-test. udp.local ( SRV 55555 host-test-1.local)
     */
    Log("Step 9b: Eth 1 sends mDNS query QTYPE=SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;

        // Clear cache to force a new query
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 10
     * - Device: N/A
     * - Description (SRP-2.6): Repeat Steps 4 through 9b, where in step 4 name compression is enabled for all
     *   repeated names in the SRP Update.
     * - Pass Criteria:
     *   - Repeat Steps 4 through 9b.
     */
    Log("Step 10: Repeat Steps 4 through 9b with name compression.");

    // Repeat Step 4 (with compression)
    Log("Step 4 (Repeat): ED 1 sends SRP Update with name compression.");
    {
        SuccessOrQuit(ed1.SetDnsNameCompressionEnabled(true));
        ed1.Get<Srp::Client>().ClearHostAndServices();

        ed1.Get<Srp::Client>().SetLeaseInterval(kLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kKeyLease);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    // Step 5 (Repeat)
    Log("Step 5 (Repeat): BR 1 (DUT) sends SRP Update Response.");

    // Step 6 (Repeat)
    Log("Step 6 (Repeat): ED 1 sends unicast DNS QType PTR query.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    // Step 7 (Repeat)
    Log("Step 7 (Repeat): BR 1 (DUT) sends DNS response.");

    // Step 8 (Repeat)
    Log("Step 8 (Repeat): Eth 1 sends mDNS query QType=PTR.");
    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = kSrpServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    // Step 9 (Repeat)
    Log("Step 9 (Repeat): BR 1 (DUT) sends mDNS Response.");

    // Step 9b (Repeat)
    Log("Step 9b (Repeat): Eth 1 sends mDNS query QTYPE=SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;

        // Clear cache to force a new query
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    {
        Ip6::Prefix omrPrefix;
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
        nexus.AddTestVar("ED_1_OMR_ADDR",
                         ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
    }
    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    {
        String<10> portStr;
        portStr.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT", portStr.AsCString());
    }

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_6((argc > 2) ? argv[2] : "test_1_3_SRP_TC_6.json");

    printf("All tests passed\n");
    return 0;
}
