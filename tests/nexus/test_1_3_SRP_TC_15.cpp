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
static const char         kSrpServiceType[]     = "_test-type._tcp";
static const char         kSrpInstanceName[]    = "service-test-1";
static const char         kSrpHostName[]        = "host-test-1";
static const char         kSrpFullServiceType[] = "_test-type._tcp.default.service.arpa.";
static const char         kSrpSubtype1[]        = "_test-subtype";
static const char         kSrpSubtype2[]        = "_other-subtype";
static const char         kSrpSubtypeInvalid[]  = "_test-subtypee";
static constexpr uint16_t kSrpServicePort       = 55555;

static const char *const kSubTypes1[] = {kSrpSubtype1, nullptr};
static const char *const kSubTypes2[] = {kSrpSubtype1, kSrpSubtype2, nullptr};
static const char *const kSubTypes3[] = {kSrpSubtype2, nullptr};

void Test_1_3_SRP_TC_15(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.15. [1.3] [CERT] Validation of SRP subtypes
     *
     * 2.15.1 Purpose
     * To test the following:
     * - 1. Handle service that includes additional subtypes of the basic service type.
     * - 2. Respond to the DNS queries for the basic service type.
     * - 3. Respond to the DNS queries for the service subtype.
     * - 4. Handle addition of new subtype to the service.
     * - 5. Handle removal of a subtype from the service.
     * - 6. Test service types using 'top' in <Service> part of name.
     *
     * 2.15.2 Topology
     * - 3. BR 1 (DUT) - Thread Border Router, and the Leader
     * - 4. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 5. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
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
     *   Description (SRP-2.15): Enable
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
     *   Description (SRP-2.15): Enable
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and ED to get an OMR address
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically adds SRP Server information in the Thread Network Data.
     *   Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data.
     */
    Log("Step 3: BR 1 (DUT) adds SRP Server information in Network Data");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT that includes a subtype:
     *     $ORIGIN default.service.arpa. _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
     *     _test-type._tcp ( PTR service-test-1._test-type._tcp) service-test-1._test-type._tcp ( SRV 8 55555
     *     host-test-1) host-test-1 AAAA <OMR address of ED 1>
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED 1 sends SRP Update with a subtype");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName          = kSrpServiceType;
        service.mInstanceName  = kSrpInstanceName;
        service.mPort          = kSrpServicePort;
        service.mSubTypeLabels = kSubTypes1;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 6
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to unicast DNS query QType = PTR for
     *     "_test-type._tcp.default.service.arpa" service type to the DUT.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 sends unicast DNS query QType PTR for service type");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.15): Automatically sends DNS response with service-test-1.
     *   Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and contain service-test-1 as follows in the Answer section:
     *     $ORIGIN default.service.arpa. _test-type._tcp ( PTR service-test-1. test-type._tcp)
     *   - It MAY also contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service-test-1._test-type._tcp ( SRV 8 8 55555 host-test-1)
     *   - host-test-1 AAAA <OMR address of ED 1>
     */
    Log("Step 7: BR 1 (DUT) sends DNS response with service-test-1");

    /**
     * Step 8
     *   Device: Eth 1
     *   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for _test-type._tcp.local."
     *     service type.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth 1 sends mDNS query QType PTR for service type");
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
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
     *     Answer section: $ORIGIN local. _test-type._tcp ( PTR service-test-1. test-type._tcp)
     *   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1._test-type. tcp ( SRV 8
     *     55555 host-test-1)
     *   - host-test-1 AAAA <OMR address of ED_1>
     */
    Log("Step 9: BR 1 (DUT) sends mDNS Response with service-test-1");

    /**
     * Step 10
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to unicast DNS query QType PTR for
     *     _test-subtype._sub._test-type._tcp.default.service.arpa" service type to the DUT.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED 1 sends unicast DNS query QType PTR for service subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype1, kSrpFullServiceType);
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(subServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 11
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.15): Automatically sends DNS response with service-test-1.
     *   Pass Criteria:
     *   - The DUT MUST send a valid DNS Response and contain service-test-1 as follows in the Answer records section:
     *     $ORIGIN default.service.arpa. _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
     *   - It MAY also contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service-test-1._test-type. tcp ( SRV 55555 host-test-1)
     *   - host-test-1 AAAA <OMR address of ED_1>
     */
    Log("Step 11: BR 1 (DUT) sends DNS response with service-test-1");

    /**
     * Step 12
     *   Device: Eth 1
     *   Description (SRP-2.15): Harness instructs the device to mDNS query QType PTR for
     *     "_test-subtype._sub._test-type._tcp.local" service type.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 12: Eth 1 sends mDNS query QType PTR for service subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype1, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 13
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
     *     Answer records: $ORIGIN local. _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
     *   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1. test-type._tcp ( SRV 8
     *     55555 host-test-1)
     *   - host-test-1 AAAA OMR address of ED_1>
     */
    Log("Step 13: BR 1 (DUT) sends mDNS Response with service-test-1");

    /**
     * Step 14
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT to add a new subtype to the
     *     existing service: $ORIGIN default.service.arpa. _other-subtype._sub._test-type._tcp ( PTR service-test-1.
     *     test-type._tcp) _test-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp) _test-type._tcp (
     * PTR service-test-1. test-type._tcp) service-test-1. test-type._tcp ( SRV 8 8 55555 host-test-1) host-test-1 AAAA
     *     OMR address of ED_1>
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 14: ED 1 adds a new subtype to the existing service");
    {
        SuccessOrQuit(ed1.Get<Srp::Client>().ClearService(service));
        service.mName          = kSrpServiceType;
        service.mInstanceName  = kSrpInstanceName;
        service.mPort          = kSrpServicePort;
        service.mSubTypeLabels = kSubTypes2;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 15
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
     */
    Log("Step 15: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 16
     *   Device: Eth_1
     *   Description (SRP-2.15): Harness instructs the device to mDNS query QType PTR for
     *     "_other-subtype._sub._test-type._tcp.local" service type, to check that the new subtype is present.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Eth 1 sends mDNS query QType PTR for the new subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype2, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 17
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response with RCODE=0 (NoError) and contain service-test-1 as follows in the
     *     Answer records section: $ORIGIN local. _other-subtype._sub._test-type._tcp ( PTR
     *     service-test-1._test-type._tcp)
     *   - It MAY also contain in the Additional records section: $ORIGIN local. Service-test-1. test-type._tcp ( SRV
     *     55555 host-test-1)
     *   - host-test-1 AAAA OMR address of ED_1>
     */
    Log("Step 17: BR 1 (DUT) sends mDNS Response with service-test-1");

    /**
     * Step 18
     *   Device: Eth_1
     *   Description (SRP-2.15): Harness instructs the device to mDNS query QType=PTR for
     *     "_test-subtype._sub_test-type._tcp.local" service type, to check that the originally registered subtype is
     *     still present.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 18: Eth 1 sends mDNS query QType PTR for the original subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype1, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 19
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - Repeat Step 13
     */
    Log("Step 19: BR 1 (DUT) sends mDNS Response (Repeat Step 13)");

    /**
     * Step 20
     *   Device: Eth_1
     *   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for _test-type._tcp.local."
     *     service type, to check that the query for the parent type also works after new subtype registration.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 20: Eth 1 sends mDNS query QType PTR for the parent service type");
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
     * Step 21
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - Repeat Step 9
     */
    Log("Step 21: BR 1 (DUT) sends mDNS Response (Repeat Step 9)");

    /**
     * Step 22
     *   Device: Eth 1
     *   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for non-existent subtype
     *     (name with extra character added): "_test-subtypee._sub._test-type_tcp.local." service type, to check that no
     *     service answer is given by the DUT.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 22: Eth 1 sends mDNS query QType PTR for a non-existent subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtypeInvalid, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 23
     *   Device: BR_1 (DUT)
     *   Description (SRP-2.15): Does not send an mDNS Response.
     *   Pass Criteria:
     *   - The DUT MUST NOT send an mDNS response.
     */
    Log("Step 23: BR 1 (DUT) does not send an mDNS Response");

    /**
     * Step 24
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT to re-register (i.e.,
     *     update), its service with the subtype "_test-subtype" removed from the service: $ORIGIN
     *     default.service.arpa. _other-subtype._sub._test-type._tcp ( PTR service-test-1._test-type._tcp)
     *     _test-type._tcp ( PTR service-test-1._test-type._tcp) service-test-1._test-type._tcp ( SRV 55555 host-test-1)
     *     host-test-1 AAAA OMR address of ED_1> Note: such an update would automatically delete any other,
     *     pre-existing subtypes on the SRP server for this service.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 24: ED 1 removes a subtype from the service");
    {
        SuccessOrQuit(ed1.Get<Srp::Client>().ClearService(service));
        service.mName          = kSrpServiceType;
        service.mInstanceName  = kSrpInstanceName;
        service.mPort          = kSrpServicePort;
        service.mSubTypeLabels = kSubTypes3;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 25
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends SRP Update Response.
     *   Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
     */
    Log("Step 25: BR 1 (DUT) sends SRP Update Response");

    /**
     * Step 26
     *   Device: Eth 1
     *   Description (SRP-2.15): Harness instructs the device to send mDNS query QType PTR for
     *     _other-subtype._sub._test-type._tcp.local" service type, to check that this subtype is still present.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 26: Eth 1 sends mDNS query QType PTR for the remaining subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype2, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 27
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Automatically sends mDNS Response with service-test-1.
     *   Pass Criteria:
     *   - Repeat Step 17
     */
    Log("Step 27: BR 1 (DUT) sends mDNS Response (Repeat Step 17)");

    /**
     * Step 28
     *   Device: Eth 1
     *   Description (SRP-2.15): Harness instructs the device to send mDNS QType=PTR query for
     *     _test-subtype._sub._test-type._tcp.local." service type to check that the removed subtype is no longer
     * present. Pass Criteria:
     *   - N/A
     */
    Log("Step 28: Eth 1 sends mDNS query QType PTR for the removed subtype");
    {
        char subServiceType[Dns::Name::kMaxNameSize];
        snprintf(subServiceType, sizeof(subServiceType), "%s._sub.%s", kSrpSubtype1, kSrpServiceType);

        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = subServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 29
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Does not send an mDNS Response.
     *   Pass Criteria:
     *   - The DUT MUST NOT send an mDNS response.
     */
    Log("Step 29: BR 1 (DUT) does not send an mDNS Response");

    /**
     * Step 30
     *   Device: ED 1
     *   Description (SRP-2.15): Harness instructs the device to send SRP Update to the DUT which isn't a valid SRP
     *     Update. This invalid update attempts to add a subtype to the service but is missing the Service Description
     *     Instruction (SRV record): $ORIGIN default.service.arpa. _test-subtype-2._sub._test-type._tcp ( PTR
     *     service-test-1._test-type._tcp) _test-type._tcp ( PTR service-test-1._test-type._tcp) host-test-1 AAAA OMR
     *     address of ED_1> Note (TBD): sending an invalid SRP Update of this form is not supported by current reference
     *     devices. As a future extension, the Harness should construct the required DNS Update message and send it
     *     using the UDP-send API.
     *   Pass Criteria:
     *   - N/A
     */
    Log("Step 30: ED 1 sends an invalid SRP Update (skipped)");

    /**
     * Step 31
     *   Device: BR 1 (DUT)
     *   Description (SRP-2.15): Rejects the invalid SRP Update.
     *   Pass Criteria:
     *   - N/A (skipping for 1.3.0 cert due to API issue - TBD)
     *   - (Future requirement TBD: MUST send SRP Update Response with RCODE=5, REFUSED.)
     */
    Log("Step 31: BR 1 (DUT) rejects the invalid SRP Update (skipped)");

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());

    {
        Ip6::Prefix omrPrefix;
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
        nexus.AddTestVar("ED_1_OMR_ADDR",
                         ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
    }

    nexus.AddTestVar("BR_1_SRP_PORT", br1.Get<Srp::Server>().GetPort());

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_15((argc > 2) ? argv[2] : "test_1_3_SRP_TC_15.json");

    printf("All tests passed\n");
    return 0;
}
