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
static const char         kSrpFullHostName[]    = "host-test-1.default.service.arpa.";
static constexpr uint16_t kSrpServicePort       = 55555;
static constexpr uint16_t kSrpUpdatedPort       = 55556;

void Test_1_3_SRP_TC_1(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.1. [1.3] [CERT] Register Single Service
     *
     * 2.1.1. Purpose
     * To test the following:
     * - 1. Handle SRP Update from Thread Device.
     * - 2. Respond to unicast DNS queries on Thread interface.
     * - 3. Respond to mDNS queries on infrastructure interface (by Advertising Proxy).
     *
     * 2.1.2. Topology
     * - BR 1 (DUT) - Thread Border Router and the Leader
     * - ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
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

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Enable: power on.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) power on.");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     *   Device: Eth 1, ED 1
     *   Description (SRP-2.1): Enable
     *   Pass Criteria:
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
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically adds its SRP Server information in the Thread Network Data.
     *   Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data. This can be verified as
     *     follows (Harness team can choose method which works best:)
     *   - DUT sends MLE Child ID Response message to ED_1 in step 2, where the message includes MLE
     *     Network Data TLV if the Child ED_1 requested this. This network data is then verified.
     *   - DUT sends MLE Data Response in step 1, 2 or 3 (per [ThreadSpec] Section 5.15.3)
     *   - Specifically, Thread Network Data MUST include a DNS-SD Unicast Dataset as per [ThreadSpec] 14.3.2.2:
     *   - S_service_data Length MUST be 1 or 19
     *   - (Note: Field "IPv6 address" does not need to be verified in this step. Reason: if this field
     *     would be wrong, step 4 would fail anyhow.)
     */
    Log("Step 3: BR 1 (DUT) adds its SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to send SRP Update to the IPv6 address of the
     *     node as indicated in the received DNS/SRP Unicast Dataset, to register a service: $ORIGIN
     *     default.service.arpa. _thread-test._udp PTR service-test-1._thread-test._udp
     *     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
     *     AAAA <OMR address of ED_1> Note: the above notation is defined at the top of this document. Note:
     *     normally an SRP client would only register the OMR address in this case. For testing purposes,
     *     both OMR/ML-EID are registered which deviates from usual client behavior.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update to register a service.");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));

        uint8_t addrsCount = 0;

        ed1.mSrpHostAddresses[addrsCount++] = ed1.Get<Mle::Mle>().GetMeshLocalEid();

        {
            Ip6::Prefix omrPrefix;
            SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
            ed1.mSrpHostAddresses[addrsCount++] = ed1.FindMatchingAddress(omrPrefix.ToString().AsCString());
        }

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostAddresses(ed1.mSrpHostAddresses, addrsCount));

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - The SRP Update Response MUST be valid; RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response.");

    /**
     * Step 6
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QType PTR to the DUT
     *     for services of type _thread-test_udp.default.service.arpa..
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 sends unicast DNS query QType PTR.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends DNS response.
     *   Pass Criteria:
     *   - The DNS Response MUST be valid and contain answer record: $ORIGIN default.service.arpa.
     *     _thread-test._udp PTR service-test-1._thread-test._udp
     *   - It MAY also contain Additional section records: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
     *     AAAA <OMR address of ED_1>
     */
    Log("Step 7: BR 1 (DUT) sends DNS response.");

    /**
     * Step 7b
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QTYPE SRV QNAME
     *     service-test-1._thread-test._udp .default.service.arpa.
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55555 host-test-1 )
     */
    Log("Step 7b: ED 1 sends unicast DNS query QTYPE SRV.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName, kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7c
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QTYPE AAAA QNAME
     *     host-test-1.default.service.arpa.
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record: $ORIGIN default.service.arpa.
     *     host-test-1 AAAA <ML-EID address of ED_1> AAAA <OMR address of ED_1>
     */
    Log("Step 7c: ED 1 sends unicast DNS query QTYPE AAAA.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kSrpFullHostName, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 8
     *   Device: Eth 1
     *   Description (SRP-2.1): Harness instructs the device to send a mDNS query QType=PTR to the DUT for
     *     services of type _thread-test_udp.local..
     *   Pass Criteria:
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
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends mDNS Response. Note: the first two pass conditions on the
     *     right are added to ensure the DUT does not propagate the mDNS onto its Thread Interface.
     *   Pass Criteria:
     *   - The DUT MUST NOT transmit an IPv6 multicast packet on the Thread Network to any of the addresses
     *     FF0X::FB, where X is any hex digit.
     *   - The DUT MUST NOT transmit any MPL multicast message on the Thread Network.
     *   - The mDNS Response MUST be valid and contain answer record: _thread-test._udp.local PTR
     *     ( service-test-1._thread-test._udp.local. )
     *   - It MAY also contain Additional section records: service-test-1._thread-test._udp.local.
     *     ( SRV 0 0 55555 host-test-1.local. ) host-test-1.local. AAAA <OMR address of ED_1>
     *   - The mDNS Response MAY contain Additional section record: host-test-1.local. AAAA <ML-EID address of ED_1>
     */
    Log("Step 9: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 9b
     *   Device: Eth_1
     *   Description (SRP-2.1): Harness instructs the device to send mDNS query: QTYPE SRV QNAME
     *     service-test-1._thread-test._udp.local
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record: service-test-1._thread-test._udp.local
     *     ( SRV 0 0 55555 host-test-1.local )
     */
    Log("Step 9b: Eth 1 sends mDNS query QTYPE SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;
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
     * Step 9c
     *   Device: Eth 1
     *   Description (SRP-2.1): Harness instructs the device to send mDNS query: QTYPE AAAA QNAME host-test-1.local
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record: host-test-1.local AAAA <OMR address of ED_1>
     *   - mDNS Response MAY contain an additional Answer section record: host-test-1.local. AAAA
     *     <ML-EID address of ED_1>
     */
    Log("Step 9c: Eth 1 sends mDNS query QTYPE AAAA.");
    {
        Dns::Multicast::Core::AddressResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback     = [](otInstance *, const otPlatDnssdAddressResult *) {};
        resolver.mHostName     = kSrpHostName;
        resolver.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartIp6AddressResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopIp6AddressResolver(resolver));
    }

    /**
     * Step 10
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to send SRP Update to the IPv6 address of the node
     *     as indicated in the received DNS/SRP Unicast Dataset to update the service parameters and address:
     *     Note: this invokes a "delete RRset" operation and an "add RRset" operation in the same SRP Update,
     *     per 2.2.5.5.2 of [draft-srp-13]. $ORIGIN default.service.arpa. _thread-test._udp PTR
     *     service-test-1._thread-test._udp service-test-1._thread-test._udp ( SRV 0 0 55556 host-test-1 )
     *     host-test-1 AAAA <ML-EID address of ED_1> Note: normally an SRP client would only register the OMR
     *     address in this case. For testing purposes, only the ML-EID is registered which deviates from
     *     usual client behavior.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED 1 sends SRP Update to update the service parameters and address.");
    {
        ed1.Get<Srp::Client>().ClearHostAndServices();
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));

        ed1.mSrpHostAddresses[0] = ed1.Get<Mle::Mle>().GetMeshLocalEid();
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostAddresses(ed1.mSrpHostAddresses, 1));

        // Update existing service
        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpUpdatedPort;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 11
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 11: BR 1 (DUT) sends SRP Update Response.");

    /**
     * Step 12
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QType PTR to the DUT
     *     for services of type '_thread-test._udp.default.service.arpa'.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 12: ED 1 sends unicast DNS query QType PTR.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 13
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends DNS response with the updated service information.
     *   Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and contain answer record: $ORIGIN default.service.arpa.
     *     _thread-test._udp PTR service-test-1._thread-test._udp
     *   - It MAY contain Additional section records: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55556 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
     *   - The DNS Response MUST NOT contain an AAAA record with OMR address of ED_1.
     */
    Log("Step 13: BR 1 (DUT) sends DNS response.");

    /**
     * Step 13b
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QTYPE SRV QNAME
     *     service-test-1._thread-test._udp default.service.arpa.
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 and contain in the Answer section: $ORIGIN default.service.arpa.
     *     service-test-1._thread-test._udp ( SRV 0 0 55556 host-test-1 )
     */
    Log("Step 13b: ED 1 sends unicast DNS query QTYPE SRV.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveService(kSrpInstanceName, kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 13c
     *   Device: ED 1
     *   Description (SRP-2.1): Harness instructs the device to unicast a UDP DNS query QTYPE=AAAA QNAME
     *     host-test-1.default.service.arpa
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 and in the Answer section it MAY contain the record: $ORIGIN
     *     default.service.arpa. host-test-1 AAAA <ML-EID address of ED_1>
     *   - DNS Response MUST NOT contain an AAAA record with OMR address of ED_1.
     */
    Log("Step 13c: ED 1 sends unicast DNS query QTYPE=AAAA.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kSrpFullHostName, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 14
     *   Device: Eth_1
     *   Description (SRP-2.1): Harness instructs the device to clear its mDNS cache and then send a mDNS query
     *     QType=PTR to the DUT for services of type _thread-test._udp.local..
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 14: Eth 1 sends mDNS query QType=PTR.");
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
     * Step 15
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.1): Automatically sends mDNS Response.
     *   Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response and contain answer record: _thread-test._udp.local PTR
     *     ( service-test-1._thread-test._udp.local. )
     *   - It MAY contain Additional section record: service-test-1._thread-test._udp.local.
     *     ( SRV 0 0 55556 host-test-1.local. )
     *   - It MAY contain in the Additional section records: host-test-1 AAAA <ML-EID address of ED_1>
     *   - The mDNS Response MUST NOT contain any answer records like; <any-string> AAAA <OMR address of ED_1>,
     *     <any-string> KEY <some-value>
     */
    Log("Step 15: BR 1 (DUT) sends mDNS Response.");

    /**
     * Step 15b
     *   Device: Eth_1
     *   Description (SRP-2.1): Harness instructs the device to send mDNS query QTYPE SRV QNAME
     *     service-test-1._thread-test._udp.local
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 with Answer section record: service-test-1._thread-test._udp.local
     *     ( SRV 0 0 55556 host-test-1.local )
     */
    Log("Step 15b: Eth 1 sends mDNS query QTYPE SRV.");
    {
        Dns::Multicast::Core::SrvResolver resolver;
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
     * Step 15c
     *   Device: Eth_1
     *   Description (SRP-2.1): Harness instructs the device to send mDNS query QTYPE AAAA QNAME host-test-1.local
     *   Pass Criteria:
     *   - The DUT MUST respond RCODE=0 and MAY contain Answer section record: host-test-1.local. AAAA
     *     <ML-EID address of ED_1>
     *   - The mDNS Response MUST NOT contain any answer records like; host-test-1.local. AAAA
     *     <OMR address of ED_1>
     */
    Log("Step 15c: Eth 1 sends mDNS query QTYPE AAAA.");
    {
        Dns::Multicast::Core::AddressResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback     = [](otInstance *, const otPlatDnssdAddressResult *) {};
        resolver.mHostName     = kSrpHostName;
        resolver.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartIp6AddressResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopIp6AddressResolver(resolver));
    }

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());

    {
        Ip6::Prefix omrPrefix;
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
        nexus.AddTestVar("ED_1_OMR_ADDR",
                         ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
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
    ot::Nexus::Test_1_3_SRP_TC_1((argc > 2) ? argv[2] : "test_1_3_SRP_TC_1.json");

    printf("All tests passed\n");
    return 0;
}
