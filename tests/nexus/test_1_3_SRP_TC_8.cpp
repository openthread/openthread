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

#include "common/clearable.hpp"

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
static const char         kSrpInstanceName2[]   = "service-test-2";
static const char         kSrpHostName[]        = "host-test-1";
static const char         kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort1      = 55555;
static constexpr uint16_t kSrpServicePort2      = 55556;
static constexpr uint16_t kSrpServicePriority2  = 8;
static constexpr uint32_t kSrpLease             = 10 * 60; // 10 minutes
static constexpr uint32_t kSrpKeyLease          = 20 * 60; // 20 minutes
static constexpr uint32_t kSrpKeyLeaseShorter   = 10 * 60; // 10 minutes

void Test_1_3_SRP_TC_8(const char *aJsonFileName)
{
    Srp::Client::Service service1;
    Srp::Client::Service service2;

    /**
     * 2.8. [1.3] [CERT] Removing Some Published Services
     *
     * 2.8.1. Purpose
     * To test the following:
     * - 1. Remove only selected service instances, not all.
     *
     * 2.8.2. Topology
     * - BR 1 (DUT) - Thread Border Router and the Leader
     * - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Server       | N/A          | 1.3
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
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Enable
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) Enable");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     *   Device: Eth 1, ED 1
     *   Description (SRP-2.8): Enable
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Step 3
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Automatically adds SRP Server information in the Thread Network Data. Automatically adds
     *     OMR prefix in Network Data.
     *   Pass Criteria:
     *   - [T1-T6] The DUT's SRP Server information MUST appear in the Thread Network Data.
     */
    Log("Step 3: BR 1 (DUT) SRP Server info in Network Data");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     *   Device: ED 1
     *   Description (SRP-2.8): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
     *     following options: Update Lease Option, Lease: 10 minutes, Key Lease: 20 minutes
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update for service 1");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        ClearAllBytes(service1);
        service1.mName         = kSrpServiceType;
        service1.mInstanceName = kSrpInstanceName1;
        service1.mPort         = kSrpServicePort1;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service1));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - [T7] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 6
     *   Device: ED 1
     *   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for
     *     "service-test-1._thread-test. udp.default.service.arpa." to the DUT.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 unicast DNS query SRV for service 1");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName1, kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Automatically sends DNS response.
     *   Pass Criteria:
     *   - [T8] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError) and
     *   - [T9] contains answer record: $ORIGIN default.service.arpa. service-test-1. thread-test._udp ( SRV 55555
     *     host-test-1 )
     *   - [T10] The DNS Response MAY contain in Additional records section: SORIGIN default.service.arpa. host-test-1
     *     AAAA <OMR address of ED 1>
     */
    Log("Step 7: BR 1 (DUT) sends DNS response");

    /**
     * Step 8
     *   Device: Eth_1
     *   Description (SRP-2.8): Harness instructs the device to send mDNS query QType SRV for "service-test-1.
     *     thread-test. udp.local.".
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth 1 sends mDNS query SRV for service 1");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName1;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 9
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Automatically sends mDNS Response.
     *   Pass Criteria:
     *   - [T11] The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError) and
     *   - [T12] contains answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 55555 host-test-1 )
     *   - [T13] DNS Response MAY contain in Additional records section: SORIGIN local. Host-test-1 AAAA <OMR address of
     *     ED 1>
     */
    Log("Step 9: BR 1 (DUT) sends mDNS Response");

    /**
     * Step 10
     *   Device: ED_1
     *   Description (SRP-2.8): Harness instructs the device to send SRP Update, adding one more service: $ORIGIN
     *     default.service.arpa. service-test-2._thread-test._udp ( SRV 8 55556 host-test-1 ) host-test-1 AAAA <OMR
     *     address of ED_1> with the following options: Update Lease Option, Lease: 10 minutes, Key Lease: 20 minutes
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED 1 sends SRP Update adding service 2");
    {
        ClearAllBytes(service2);
        service2.mName         = kSrpServiceType;
        service2.mInstanceName = kSrpInstanceName2;
        service2.mPort         = kSrpServicePort2;
        service2.mPriority     = kSrpServicePriority2;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service2));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 11
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - [T14] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 11: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 12
     *   Device: ED_1
     *   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType=SRV for service-test-2 to the
     *     DUT
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 12: ED 1 unicast DNS query SRV for service 2");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName2, kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 13
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Automatically sends DNS response.
     *   Pass Criteria:
     *   - [T15] The DUT MUST send a valid DNS Response.
     *   - [T16] containing service-test-2 answer record: $ORIGIN default.service.arpa. service-test-2._thread-test._udp
     *     ( SRV 55556 host-test-1 )
     *   - [T17] DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
     *     <OMR address of ED_1>
     */
    Log("Step 13: BR 1 (DUT) sends DNS response");

    /**
     * Step 14
     *   Device: Eth_1
     *   Description (SRP-2.8): Repeat Step 8. Note: this is repeated to verify that the first service is still there.
     *   Pass Criteria:
     *   - Repeat Step 8
     */
    Log("Step 14: Eth 1 sends mDNS query SRV for service 1 (repeat)");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName1;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 15
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Repeat Step 9
     *   Pass Criteria:
     *   - [T18-T20] Repeat Step 9
     */
    Log("Step 15: BR 1 (DUT) sends mDNS Response (repeat)");

    /**
     * Step 16
     *   Device: ED 1
     *   Description (SRP-2.8): Harness instructs device to send SRP Update, removing the first service only: 1. Service
     *     Discovery Instruction Delete an RR from an RRset semantics using data: $ORIGIN default.service.arpa. PTR
     *     service-test-1. thread-test._udp 2. Service Description Instruction Delete All RRsets From a Name semantics:
     *     $ORIGIN default.service.arpa. service-test-1._thread-test._udp ANY <tt1=0> with the following options: Update
     *     Lease Option, Lease: 10 minutes (unchanged), Key Lease: 10 minutes (shorter). Note: this method to remove a
     *     service follows 3.2.5.5.2 of [draft-srp-23]. Note: for OT reference device, the command 'srp client
     *     keyleaseinterval 600' is used to shorten the key lease interval as indicated.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 16: ED 1 sends SRP Update removing service 1");
    {
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLeaseShorter);
        SuccessOrQuit(ed1.Get<Srp::Client>().RemoveService(service1));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 17
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - [T21] The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 17: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 18
     *   Device: ED 1
     *   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for "service-test-1.
     *     thread-test. udp.default.service.arpa." to the DUT
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 18: ED 1 unicast DNS query SRV for service 1");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName1, kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 19
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.8): Automatically sends DNS response. Note: the service instance name with all associated
     *     records has been deleted in step 16.
     *   Pass Criteria:
     *   - [T22] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError); and
     *   - [T23] MUST contain 0 answer records.
     */
    Log("Step 19: BR 1 (DUT) sends DNS response (0 answers)");

    /**
     * Step 20
     *   Device: ED 1
     *   Description (SRP-2.8): Harness instructs the device to unicast DNS query QType SRV for "service-test-2.
     *     thread-test. udp.default.service.arpa." to the DUT.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 20: ED 1 unicast DNS query SRV for service 2");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName2, kSrpFullServiceType, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 21
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Automatically sends DNS response.
     *   Pass Criteria:
     *   - [T24] The DUT MUST send a valid DNS Response, with RCODE=0 (NoError) and
     *   - [T25] contains answer record: $ORIGIN default.service.arpa. service-test-2. thread-test._udp ( SRV 55556
     *     host-test-1 )
     *   - [T26] DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
     *     <OMR address of ED 1>
     */
    Log("Step 21: BR 1 (DUT) sends DNS response (service 2)");

    /**
     * Step 22
     *   Device: Eth_1
     *   Description (SRP-2.8): Harness instructs the device to send mDNS query QType SRV for "service-test-1.
     *     thread-test. udp.local".
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 22: Eth 1 sends mDNS query SRV for service 1");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName1;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 23
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Does not send a mDNS response.
     *   Pass Criteria:
     *   - [T27] The DUT MUST NOT send an mDNS response.
     */
    Log("Step 23: BR 1 (DUT) does NOT send mDNS response");

    /**
     * Step 24
     *   Device: Eth 1
     *   Description (SRP-2.8): Harness instructs the device to send mDNS query QType=PTR any services of type
     *     "_thread-test._udp.local".
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 24: Eth 1 sends mDNS query PTR for services");
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
     * Step 25
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.8): Automatically sends mDNS response with service-test-2.
     *   Pass Criteria:
     *   - [T28] The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError) and
     *   - [T29] contains service-test-2 answer record: $ORIGIN local. _thread-test._udp ( PTR
     *     service-test-2._thread-test._udp )
     *   - [T30] Response MUST NOT contain any further records in the Answers section.
     *   - [T31-32] DNS Response MAY contain in Additional records section: SORIGIN local.
     *     Service-test-2._thread-test._udp ( SRV 8 0 55556 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
     */
    Log("Step 25: BR 1 (DUT) sends mDNS response with service 2 only");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_8((argc > 2) ? argv[2] : "test_1_3_SRP_TC_8.json");
    printf("All tests passed\n");
    return 0;
}
