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
static const char         kSrpServiceType[]  = "_thread-test._udp";
static const char         kSrpInstance1[]    = "service-test-1";
static const char         kSrpInstance2[]    = "service-test-2";
static const char         kSrpHost1[]        = "host-test-1";
static const char         kSrpHost2[]        = "host-test-2";
static constexpr uint16_t kSrpServicePort333 = 33333;
static constexpr uint16_t kSrpServicePort444 = 44444;

void Test_1_3_SRP_TC_2(const char *aJsonFileName)
{
    Srp::Client::Service service1;
    Srp::Client::Service service2;

    /**
     * 2.2. [1.3] [CERT] Name Conflicts - Single Thread Network
     *
     * 2.2.1. Purpose
     * To test the following:
     * - 1. Handle name conflict in Host Description record.
     * - 2. Handle name conflict in Service Description record.
     * - 3. Verify that original service is seen when discovering, and conflicting
     *   service is not seen on the AIL.
     *
     * 2.2.2. Topology
     * - 1. BR_1 (DUT) - Border Router and the Leader.
     * - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 3. ED 2-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 4. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * 2.2.3. Initial Conditions
     * - BR_1 (DUT) should automatically start SRP server capabilities at the
     *   beginning of the test.
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
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Enable: switch on.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) switch on.");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     * - Device: ED 1, ED 2, Eth 1
     * - Description (SRP-2.2): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: ED 1, ED 2, Eth 1 Enable.");
    ed1.Join(br1, Node::kAsFed);
    ed2.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and EDs to get OMR addresses
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Automatically adds its SRP Server information in the
     *   Thread Network Data.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data.
     *     This can be verified as specified in test case 2.1 step 3.
     *   - Specifically, it MUST include a DNS-SD Unicast Dataset.
     */
    Log("Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     * - Device: ED 1
     * - Description (SRP-2.2): Harness instructs the device to send SRP Update with
     *   initial service registration: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 0 0 33333 host-test-1 ) host-test-1
     *   AAAA <OMR address of ED_1>
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update with initial service registration.");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHost1));

        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service1);
        service1.mName          = kSrpServiceType;
        service1.mInstanceName  = kSrpInstance1;
        service1.mPort          = kSrpServicePort333;
        service1.mTxtEntries    = nullptr;
        service1.mNumTxtEntries = 0;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service1));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response (NoError).");

    /**
     * Step 6
     * - Device: ED 2
     * - Description (SRP-2.2): Harness instructs the device to send SRP Update with
     *   same-name (conflicting) service registration: $ORIGIN
     *   default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0 33333
     *   host-test-2 ) host-test-2 AAAA <OMR address of ED_2>
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 2 sends SRP Update with same-name (conflicting) service registration.");
    {
        ed2.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(ed2.Get<Srp::Client>().SetHostName(kSrpHost2));

        SuccessOrQuit(ed2.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service2);
        service2.mName         = kSrpServiceType;
        service2.mInstanceName = kSrpInstance1; // Conflict
        service2.mPort         = kSrpServicePort333;
        SuccessOrQuit(ed2.Get<Srp::Client>().AddService(service2));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 7
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=6 (YXDOMAIN).
     */
    Log("Step 7: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).");

    /**
     * Step 8
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs device to perform mDNS query QType
     *   PTR for any services of type _thread-test._udp.local..
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth 1 performs mDNS query QType PTR.");
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
     * - Description (SRP-2.2): Automatically sends mDNS response with initial
     *   registered service.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response and contain answer record:
     *     _thread-test._udp.local PTR ( service-test-1._thread-test._udp.local. )
     *   - It MAY also contain in the Additional records section:
     *     service-test-1._thread-test._udp.local. ( SRV 0 0 33333 host-test-1.local. )
     *   - It MAY also contain in the Additional records section: host-test-1.local.
     *     AAAA <OMR address of ED_1>
     *   - The mDNS Response MUST NOT contain any answer record or additional
     *     record like: <any-string> AAAA <OMR address of ED_2>
     *   - The mDNS Response MUST NOT contain any answer record or additional
     *     record like: host-test-1.local. AAAA <any non-OMR address>
     *   - The mDNS Response MUST NOT contain any answer record or additional
     *     record like: service-test-2._thread-test._udp.local. SRV <any data>
     *   - The mDNS Response MUST NOT contain any answer record or additional
     *     record like: <any-name> PTR service-test-2._thread-test_udp.local.
     */
    Log("Step 9: BR 1 (DUT) sends mDNS response with initial registered service.");

    /**
     * Step 9b
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE SRV QNAME service-test-1._thread-test._udp.local
     * - Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record:
     *     service-test-1._thread-test._udp.local ( SRV 0 0 33333 host-test-1.local )
     */
    Log("Step 9b: Eth 1 sends mDNS query QTYPE SRV.");
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
     * Step 9c
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE AAAA QNAME host-test-1.local
     * - Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record:
     *     host-test-1.local AAAA <OMR address of ED_1>
     */
    Log("Step 9c: Eth 1 sends mDNS query QTYPE AAAA.");
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
     * Step 10
     * - Device: ED 2
     * - Description (SRP-2.2): Harness instructs the device to send SRP Update with
     *   same-hostname (conflicting) service registration: $ORIGIN
     *   default.service.arpa. service-test-2._thread-test._udp ( SRV 0 0 33333
     *   host-test-1 ) host-test-1 AAAA <OMR address of ED_2>
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED 2 sends SRP Update with same-hostname (conflicting) service registration.");
    {
        ed2.Get<Srp::Client>().ClearHostAndServices();
        SuccessOrQuit(ed2.Get<Srp::Client>().SetHostName(kSrpHost1)); // Conflict

        SuccessOrQuit(ed2.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service2);
        service2.mName          = kSrpServiceType;
        service2.mInstanceName  = kSrpInstance2;
        service2.mPort          = kSrpServicePort333;
        service2.mTxtEntries    = nullptr;
        service2.mNumTxtEntries = 0;
        SuccessOrQuit(ed2.Get<Srp::Client>().AddService(service2));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 11
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=6 (YXDOMAIN).
     */
    Log("Step 11: BR 1 (DUT) sends SRP Update Response (YXDOMAIN).");

    /**
     * Step 12
     * - Device: Eth 1
     * - Description (SRP-2.2): Repeat Step 8
     * - Pass Criteria:
     *   - Repeat Step 8
     */
    Log("Step 12: Eth 1 repeats mDNS query QType PTR.");
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
     * Step 13
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.2): Repeat Step 9 (including 9b, 9c, etc)
     * - Pass Criteria:
     *   - Repeat Step 9 (including 9b, 9c, etc)
     */
    Log("Step 13: BR 1 (DUT) repeats mDNS response verification.");
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
     * Step 14
     * - Device: ED 2
     * - Description (SRP-2.2): Harness instructs the device to send SRP Update with
     *   non-conflicting service registration: $ORIGIN default.service.arpa.
     *   service-test-2._thread-test._udp ( SRV 1 0 44444 host-test-2 ) host-test-2
     *   AAAA <OMR address of ED_2>
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 14: ED 2 sends SRP Update with non-conflicting service registration.");
    {
        ed2.Get<Srp::Client>().ClearHostAndServices();
        SuccessOrQuit(ed2.Get<Srp::Client>().SetHostName(kSrpHost2));

        SuccessOrQuit(ed2.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service2);
        service2.mName          = kSrpServiceType;
        service2.mInstanceName  = kSrpInstance2;
        service2.mPort          = kSrpServicePort444;
        service2.mPriority      = 1;
        service2.mTxtEntries    = nullptr;
        service2.mNumTxtEntries = 0;
        SuccessOrQuit(ed2.Get<Srp::Client>().AddService(service2));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 15
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.2): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 15: BR 1 (DUT) sends SRP Update Response (NoError).");

    /**
     * Step 16
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs the device to perform mDNS query
     *   QType PTR for any services of type _thread-test_udp.local..
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Eth 1 performs mDNS query QType PTR.");
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
     * - Description (SRP-2.2): Automatically sends mDNS response with both
     *   registered services.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response and MUST contain 2 answer
     *     records: _thread-test._udp.local ( PTR service-test-1._thread-test._udp.local )
     *     AND _thread-test._udp.local ( PTR service-test-2._thread-test._udp.local )
     *   - It MAY also contain in the Additional records section:
     *     service-test-1._thread-test._udp.local. ( SRV 0 0 33333 host-test-1.local. )
     *   - It MAY also contain in the Additional records section:
     *     service-test-2._thread-test._udp.local. ( SRV 1 0 44444 host-test-2.local. )
     *   - It MAY also contain in the Additional records section: host-test-1.local.
     *     AAAA <OMR address of ED_1>
     *   - It MAY also contain in the Additional records section: host-test-2.local.
     *     AAAA <OMR address of ED_2>
     *   - The response MAY be sent as two separate mDNS response messages,
     *     splitting over the 2 services.
     */
    Log("Step 17: BR 1 (DUT) sends mDNS response with both registered services.");

    /**
     * Step 17b
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE=SRV QNAME service-test-1._thread-test._udp.local
     * - Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record:
     *     service-test-1._thread-test._udp.local ( SRV 0 0 33333 host-test-1.local )
     */
    Log("Step 17b: Eth 1 sends mDNS query QTYPE=SRV QNAME service-test-1.");
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
     * Step 17c
     * - Device: Eth_1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE SRV QNAME service-test-2._thread-test_udp.local
     * - Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record:
     *     service-test-2._thread-test._udp.local ( SRV 1 0 44444 host-test-2.local )
     */
    Log("Step 17c: Eth 1 sends mDNS query QTYPE=SRV QNAME service-test-2.");
    {
        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstance2;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 17d
     * - Device: Eth 1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE=AAAA QNAME host-test-1.local
     * - Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record:
     *     host-test-1.local AAAA <OMR address of ED_1>
     */
    Log("Step 17d: Eth 1 sends mDNS query QTYPE=AAAA QNAME host-test-1.");
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
     * Step 17e
     * - Device: Eth_1
     * - Description (SRP-2.2): Harness instructs the device to send mDNS query
     *   QTYPE AAAA QNAME host-test-2.local
     * - Pass Criteria:
     *   - THE DUT MUST respond RCODE=0 with Answer section record:
     *     host-test-2.local AAAA <OMR address of ED_2>
     */
    Log("Step 17e: Eth 1 sends mDNS query QTYPE=AAAA QNAME host-test-2.");
    {
        Dns::Multicast::Core::AddressResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback     = [](otInstance *, const otPlatDnssdAddressResult *) {};
        resolver.mHostName     = kSrpHost2;
        resolver.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartIp6AddressResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopIp6AddressResolver(resolver));
    }

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_2_MLEID_ADDR", ed2.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());

    {
        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT", srpPortString.AsCString());
    }

    Ip6::Prefix omrPrefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
    nexus.AddTestVar("ED_1_OMR_ADDR", ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
    nexus.AddTestVar("ED_2_OMR_ADDR", ed2.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_2((argc > 2) ? argv[2] : "test_1_3_SRP_TC_2.json");

    printf("All tests passed\n");
    return 0;
}
