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
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 10 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP service registration parameters.
 */
static const char         kSrpServiceType[]      = "_thread-test._udp";
static const char         kSrpInstanceName[]     = "service test 1";
static const char         kSrpHostName[]         = "host-test-1";
static const char         kDnsQueryServiceType[] = "_thread-test._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort        = 1500;

void Test_1_3_SRP_TC_5(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.5. [1.3] [CERT] KEY record inclusion/omission
     *
     * 2.5.1. Purpose
     * To test the following:
     * - 1. Handle Service Description Instructions that include the KEY RR.
     * - 2. Handle Service Description Instructions that omit the KEY RR.
     * - 3. Report error on Host Description Instructions that omit the KEY RR.
     *
     * 2.5.2. Topology
     * - BR_1 (DUT) - Thread Border Router and the Leader
     * - ED_1 - Test Bed device operating as a Thread End Device, attached to BR_1
     * - Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
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
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Enable: switch on.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR_1 (DUT) Enable: switch on.");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2
     * - Device: Eth_1, ED_1
     * - Description (SRP-2.5): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth_1, ED_1 Enable.");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Wait for BR to advertise prefixes and ED to get an OMR address
     */
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically adds SRP Server information in the Thread Network Data; and configures
     *   OMR prefix.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data, as DNS/SRP Unicast Dataset, or
     *     DNS/SRP Anycast Dataset, or both.
     *   - ULA OMR prefix MUST appear in the Thread Network Data.
     */
    Log("Step 3: BR_1 (DUT) Automatically adds SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    /**
     * Step 4
     * - Device: ED_1
     * - Description (SRP-2.5): Harness instructs the device to send SRP Update to the DUT with service KEY records
     *   enabled:
     *   $ORIGIN default.service.arpa.
     *   _thread-test._udp PTR service test 1._thread-test._udp
     *   service test 1._thread-test._udp ( SRV 0 0 1500 host-test-1 ) KEY <public key of ED_1>
     *   host-test-1 AAAA <ML-EID address of ED_1> AAAA <OMR address of ED_1> KEY <public key of ED_1>
     *   with the following options: Update Lease Option Lease: 20 minutes Key Lease: 20 minutes
     *   Note: normally a client will not register the ML-EID address in this situation. For testing purposes, the
     *   harness will configure/force this.
     *   Note: including service KEY records is not typically done by default. Reference device is configured for
     *   this.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED_1 sends SRP Update to the DUT with service KEY records enabled.");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));

        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;

        ed1.Get<Srp::Client>().SetServiceKeyRecordEnabled(true);
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 5
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically sends success SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response with RCODE=0 (NoError).
     */
    Log("Step 5: BR_1 (DUT) Automatically sends success SRP Update Response.");
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 6
     * - Device: ED_1
     * - Description (SRP-2.5): Harness instructs the device to unicast DNS QType=PTR query to the DUT for all
     *   services of type _thread-test._udp.local..
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED_1 unicast DNS QType=PTR query to the DUT.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kDnsQueryServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically sends DNS response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response containing Answer record: $ORIGIN default.service.arpa.
     *     _thread-test._udp PTR service test 1._thread-test._udp
     *   - It MAY contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service test 1._thread-test._udp ( SRV 0 0 1500 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1>
     *     AAAA <OMR address of ED_1>
     *   - The DNS Response MUST NOT contain any RRType=KEY records.
     */
    Log("Step 7: BR_1 (DUT) Automatically sends DNS response.");
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 8
     * - Device: Eth_1
     * - Description (SRP-2.5): Harness instructs the device to send mDNS QType=PTR query for services of type
     *   _thread-test._udp.local.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: Eth_1 send mDNS QType=PTR query.");
    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = kSrpServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime);
        IgnoreError(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    /**
     * Step 9
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response containing Answer record: $ORIGIN local.
     *     _thread-test._udp PTR service test 1._thread-test._udp
     *   - It MAY contain in the Additional records section: Service test 1._thread-test._udp ( SRV 0 0 1500
     *     host-test-1 ) host-test-1 AAAA <OMR address of ED_1>
     *   - The DNS Response MUST NOT contain any RRType=KEY records.
     *   - The DNS Response MAY also contain in the Additional records section: host-test-1.local AAAA
     *     <ML-EID address of ED_1>
     */
    Log("Step 9: BR_1 (DUT) Automatically sends mDNS Response.");
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 10
     * - Device: N/A
     * - Description (SRP-2.5): Repeat Steps 4 through 9 with KEY record omitted from Service Description
     *   Instruction in step 4, i.e. the following record is omitted in step 4: service test 1._thread-test._udp KEY
     *   <pubkey of ED_1>. The host KEY records are kept.
     * - Pass Criteria:
     *   - Same as Steps 4 through 9.
     */
    Log("Step 10: Repeat Steps 4 through 9 with KEY record omitted from Service Description Instruction.");

    /**
     * Implementation of Step 10 (Repeat 4-9)
     */
    Log("Step 10.4: ED_1 sends SRP Update with service KEY record disabled.");
    {
        ed1.Get<Srp::Client>().SetServiceKeyRecordEnabled(false);
        IgnoreError(ed1.Get<Srp::Client>().ClearService(service));
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 10.5: BR_1 (DUT) Automatically sends success SRP Update Response.");
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 10.6: ED_1 unicast DNS QType=PTR query.");
    {
        SuccessOrQuit(ed1.Get<Dns::Client>().Browse(kDnsQueryServiceType, nullptr, nullptr));
    }
    nexus.AdvanceTime(kDnsQueryTime);

    Log("Step 10.7: BR_1 (DUT) Automatically sends DNS response.");
    nexus.AdvanceTime(kDnsQueryTime);

    Log("Step 10.8: Eth_1 send mDNS QType=PTR query.");
    {
        Dns::Multicast::Core::Browser browser;
        ClearAllBytes(browser);
        browser.mCallback     = [](otInstance *, const otPlatDnssdBrowseResult *) {};
        browser.mServiceType  = kSrpServiceType;
        browser.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartBrowser(browser));
        nexus.AdvanceTime(kDnsQueryTime * 2);
        IgnoreError(eth1.Get<Dns::Multicast::Core>().StopBrowser(browser));
    }

    Log("Step 10.9: BR_1 (DUT) Automatically sends mDNS Response.");
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 11
     * - Device: ED_1
     * - Description (SRP-2.5): Repeat Step 4 with service KEY records omitted, while keeping host KEY records.
     * - Pass Criteria:
     *   - Repeat Step 4
     */
    Log("Step 11: Repeat Step 4 with service KEY records omitted.");
    /**
     * This seems redundant with Step 10's repeat of Step 4, but I will follow the spec.
     */
    {
        ed1.Get<Srp::Client>().SetServiceKeyRecordEnabled(false);
        IgnoreError(ed1.Get<Srp::Client>().ClearService(service));
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 12
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically sends success SRP Update Response.
     * - Pass Criteria:
     *   - Repeat Step 5
     */
    Log("Step 12: BR_1 (DUT) Automatically sends success SRP Update Response.");
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 13
     * - Device: ED_1
     * - Description (SRP-2.5): Repeat Step 4 with all KEY records omitted. Note: currently (2022-07) this is not yet
     *   supported in reference devices. It could be implemented using a "UDP send" API with a manually constructed
     *   sequence of bytes that forms the invalid SRP Update message. To be considered for the future.
     * - Pass Criteria:
     *   - N/A (TBD to be added in the future)
     */
    Log("Step 13: ED_1 Repeat Step 4 with all KEY records omitted.");
    {
        ed1.Get<Srp::Client>().SetServiceKeyRecordEnabled(false);
        ed1.Get<Srp::Client>().SetHostKeyRecordEnabled(false);
        IgnoreError(ed1.Get<Srp::Client>().ClearService(service));
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 14
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.5): Automatically responds with error SRP Update Response.
     * - Pass Criteria:
     *   - N/A (TBD-add in the future verify that RCODE=5, Refused)
     */
    Log("Step 14: BR_1 (DUT) Automatically responds with error SRP Update Response.");
    nexus.AdvanceTime(kBrActionTime);

    Log("All steps completed.");

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());

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
    ot::Nexus::Test_1_3_SRP_TC_5((argc > 2) ? argv[2] : "test_1_3_SRP_TC_5.json");

    printf("All tests passed\n");
    return 0;
}
