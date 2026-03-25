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
static const char         kSrpInstance1[]       = "service-test-1";
static const char         kSrpHost1[]           = "host-test-1";
static const char         kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort       = 55555;
static constexpr uint32_t kSrpLease             = 30;
static constexpr uint32_t kSrpKeyLease          = 600;
static constexpr uint32_t kWaitTime15s          = 15 * 1000;
static constexpr uint32_t kWaitTime16s          = 16 * 1000;

void Test_1_3_SRP_TC_3(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.3. [1.3] [CERT] Service Instance Lease - Renewal and Automatic Service/Host Removal
     *
     * 2.3.1. Purpose
     * To test the following:
     * - Handle lease renewal before lease expiration.
     * - Handle service/host lease expiration and removal of the service/host.
     *
     * 2.3.2. Topology
     * - 1. BR_1 (DUT)-Thread Border Router and the Leader
     * - 2. ED_1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference           | V1.3.0 Section
     * -------------------------|---------------
     * DNS-SD Unicast Dataset   | 14.3.2.2
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

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Enable: switch on.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) switch on.");
    {
        Srp::Server::LeaseConfig leaseConfig;
        br1.Get<Srp::Server>().GetLeaseConfig(leaseConfig);
        leaseConfig.mMaxLease = kSrpLease;
        leaseConfig.mMinLease = kSrpLease;
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
     * - Device: Eth 1, ED 1
     * - Description (SRP-2.3): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable.");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and EDs to get OMR addresses
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically adds SRP Server information in the Thread Network Data. Automatically
     *   provides an OMR prefix to the Thread Network.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data. This can be verified as specified
     *     in test case 2.1 step 3.
     *   - Specifically, it MUST include a DNS-SD Unicast Dataset as per [ThreadSpec] 14.3.2.2.
     *   - S_service_data Length MUST be 1 or 19
     */
    Log("Step 3: BR 1 (DUT) adds SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     * - Device: ED 1
     * - Description (SRP-2.3): Harness instructs the device to send SRP Update to DUT: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1> with the
     *   following options: Update Lease Option, Lease: 30 seconds, Key Lease: 10 minutes. Harness
     *   instructs/configures device to avoid automatic renewal or resending of this SRP Update.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update to DUT.");
    {
        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHost1));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName          = kSrpServiceType;
        service.mInstanceName  = kSrpInstance1;
        service.mPort          = kSrpServicePort;
        service.mTxtEntries    = nullptr;
        service.mNumTxtEntries = 0;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));

        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    }
    nexus.AdvanceTime(kSrpRegistrationTime);
    ed1.Get<Srp::Client>().Stop();

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response.");

    /**
     * Step 6
     * - Device: ED 1
     * - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
     *   "service-test-1._thread-test._udp.default.service.arpa"
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 sends unicast DNS query QType SRV.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstance1, kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends DNS response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and contain in Answers section: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
     *   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
     *     <OMR address of ED_1>
     */
    Log("Step 7: BR 1 (DUT) sends DNS response.");

    /**
     * Step 8
     * - Device: Eth_1
     * - Description (SRP-2.3): Harness instructs the device to send mDNS query QType SRV for
     *   "service-test-1._thread-test._udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth 1 sends mDNS query QType SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstance1;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 9
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response and contain in Answers section: $ORIGIN local.
     *     Service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
     *   - The DNS Response MAY contain in Additional records section: $ORIGIN local. Host-test-1 AAAA <OMR address of
     *     ED_1>
     */
    Log("Step 9: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 10
     * - Device: N/A
     * - Description (SRP-2.3): Harness waits for 15 seconds.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: Harness waits for 15 seconds.");
    nexus.AdvanceTime(kWaitTime15s);

    /**
     * Step 11
     * - Device: ED 1
     * - Description (SRP-2.3): Harness instructs the device to send SRP Update to the DUT to renew the service lease:
     *   $ORIGIN default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA
     *   <OMR address of ED_1> with the following options: Update Lease Option, Lease: 30 seconds, Key Lease: 10
     *   minutes. Harness instructs/configures the device to avoid automatic renewal or resending of this SRP Update.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 11: ED 1 sends SRP Update to renew the service lease.");
    {
        ed1.Get<Srp::Client>().ClearHostAndServices();
        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHost1));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName          = kSrpServiceType;
        service.mInstanceName  = kSrpInstance1;
        service.mPort          = kSrpServicePort;
        service.mTxtEntries    = nullptr;
        service.mNumTxtEntries = 0;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));

        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    }
    nexus.AdvanceTime(kSrpRegistrationTime);
    ed1.Get<Srp::Client>().Stop();

    /**
     * Step 12
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 12: BR 1 (DUT) sends SRP Update Response.");

    /**
     * Step 13
     * - Device: N/A
     * - Description (SRP-2.3): Harness waits for 16 seconds.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 13: Harness waits for 16 seconds.");
    nexus.AdvanceTime(kWaitTime16s);

    /**
     * Step 14
     * - Device: ED 1
     * - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
     *   "service-test-1._thread-test._udp.default.service.arpa"
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 14: ED 1 sends unicast DNS query QType SRV.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstance1, kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 15
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends DNS response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and contain answer record: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
     *   - The DNS Response MAY contain in Additional records section: $ORIGIN default.service.arpa. host-test-1 AAAA
     *     <OMR address of ED_1>
     */
    Log("Step 15: BR 1 (DUT) sends DNS response.");

    /**
     * Step 16
     * - Device: Eth 1
     * - Description (SRP-2.3): Harness instructs the device to send mDNS query QType PTR for any services of type
     *   "_thread-test._udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Eth 1 sends mDNS query QType PTR.");
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
     * Step 17
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response and contain answer record: $ORIGIN local. _thread-test._udp ( PTR
     *     service-test-1._thread-test._udp )
     *   - THe DNS Response MAY contain in Additional records section: $ORIGIN local. Service-test-1_thread-test._udp (
     *     SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
     */
    Log("Step 17: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 18
     * - Device: N/A
     * - Description (SRP-2.3): Harness waits 15 seconds for the service lease to expire. Note: Both service and host
     *   entries should expire now, after >= 31 seconds after last registration update. But the KEY record registration
     *   remains active for both names.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 18: Harness waits 15 seconds for the service lease to expire.");
    nexus.AdvanceTime(kWaitTime15s);

    /**
     * Step 19
     * - Device: ED 1
     * - Description (SRP-2.3): Harness instructs the device to unicast DNS query QType SRV to the DUT for service
     *   instance name "service-test-1._thread-test._udp.default.service.arpa"
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 19: ED 1 sends unicast DNS query QType SRV.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstance1, kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 20
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Automatically sends DNS response but no matching SRV records. Note: only the key lease
     *   is still active on the service instance name. The DUT's response may vary as shown on the right, depending on
     *   implementation.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and MUST be one of the below:
     *   - 1. RCODE=0 (NoError) with 0 answer records.
     *   - 2. RCODE=3 (NxDomain) with 0 answer records.
     */
    Log("Step 20: BR 1 (DUT) sends DNS response (no SRV).");

    /**
     * Step 21
     * - Device: ED 1
     * - Description (SRP-2.3): (Reserved for TBD future unicast DNS query QTYPE=ANY to the DUT for records for name
     *   "host-test-1.default.service.arpa".)
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 21: ED 1 reserved step.");

    /**
     * Step 22
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): (Reserved for TBD future: Automatically sends DNS response.) Note: because the key
     *   lease is still active, the KEY record is optionally returned in response to the "ANY" query.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 22: BR 1 (DUT) reserved step.");

    /**
     * Step 23
     * - Device: Eth 1
     * - Description (SRP-2.3): Harness instructs device to send mDNS query QType=SRV for
     *   "service-test-1._thread-test_udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 23: Eth 1 sends mDNS query QType SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstance1;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 24
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
     * - Pass Criteria:
     *   - The DUT MUST perform one of below options:
     *   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
     *   - 2. MUST NOT send any mDNS response.
     *   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
     */
    Log("Step 24: BR 1 (DUT) mDNS verification for Step 23.");

    /**
     * Step 25
     * - Device: Eth_1
     * - Description (SRP-2.3): Harness instructs the device to send mDNS query QType=PTR for any services of type
     *   "_thread-test._udp.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 25: Eth 1 sends mDNS query QType PTR.");
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
     * Step 26
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
     * - Pass Criteria:
     *   - The DUT MUST perform one of below options:
     *   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
     *   - 2. MUST NOT send any mDNS response.
     *   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
     */
    Log("Step 26: BR 1 (DUT) mDNS verification for Step 25.");

    /**
     * Step 27
     * - Device: Eth 1
     * - Description (SRP-2.3): Harness instructs device to send mDNS query for Qtype AAAA for name "host-test-1.local."
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 27: Eth 1 sends mDNS query for QType AAAA.");
    {
        Dns::Multicast::Core::AddressResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback     = [](otInstance *, const otPlatDnssdAddressResult *) {};
        resolver.mHostName     = kSrpHost1;
        resolver.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartIp6AddressResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopIp6AddressResolver(resolver));
    }

    /**
     * Step 28
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.3): Does not respond to mDNS query, or automatically responds NSEC, or no-error-no-answer.
     *   Note: this indicates the DUT is authoritative for the host name, but has no AAAA records to answer for the
     *   name.
     * - Pass Criteria:
     *   - The DUT MUST perform one of below options:
     *   - 1. Sends RCODE=0 (NoError) and NSEC Answer record.
     *   - 2. MUST NOT send any mDNS response.
     *   - 3. Sends RCODE=0 (NoError) and 0 Answer records.
     */
    Log("Step 28: BR 1 (DUT) mDNS verification for Step 27.");

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());

    {
        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT", srpPortString.AsCString());
    }

    Ip6::Prefix omrPrefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
    nexus.AddTestVar("ED_1_OMR_ADDR", ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_3((argc > 2) ? argv[2] : "test_1_3_SRP_TC_3.json");

    printf("All tests passed\n");
    return 0;
}
