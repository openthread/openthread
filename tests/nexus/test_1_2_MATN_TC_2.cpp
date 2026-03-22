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
static constexpr uint32_t kFormNetworkTime = 10 * 1000;

/**
 * Time to advance for a node to join as a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for MLR registration, in milliseconds.
 */
static constexpr uint32_t kMlrRegistrationTime = 5 * 1000;

/**
 * Time to advance for ICMPv6 Echo Reply, in milliseconds.
 */
static constexpr uint32_t kEchoReplyWaitTime = 5000;
/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * Multicast address MA2.
 */
static const char kMA2[] = "ff05::1234:777a:1";

/**
 * Multicast address MA1g.
 */
static const char kMA1g[] = "ff0e::1234:777a:1";

void TestMatnTc2(void)
{
    /**
     * 5.10.2 MATN-TC-02: Multicast listener registration and first use
     * Spec Reference          | V1.2 Section
     * ------------------------|-------------
     * Multicast Listener Reg. | 5.10.2
     */

    Core                                nexus;
    Node                               &br1  = nexus.CreateNode();
    Node                               &br2  = nexus.CreateNode();
    Node                               &td   = nexus.CreateNode();
    Node                               &host = nexus.CreateNode();
    Ip6::Address                        ma1;
    Ip6::Address                        ma2;
    Ip6::Address                        ma1g;
    const Ip6::Address                 *hostUla;
    const Ip6::Address                 *br2BackboneUla;
    const Ip6::Netif::MulticastAddress *netifAddr;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    td.SetName("TD");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));
    SuccessOrQuit(ma2.FromString(kMA2));
    SuccessOrQuit(ma1g.FromString(kMA1g));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - BR_1, BR_2, TD
     * - Pass Criteria:
     *   - N/A
     */
    br1.AllowList(br2);
    br1.AllowList(td);
    br2.AllowList(br1);
    br2.AllowList(td);
    td.AllowList(br1);
    td.AllowList(br2);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    td.Join(br1, Node::kAsFtd);
    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(td.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime * 2);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    hostUla        = &host.mInfraIf.FindMatchingAddress("fd00::/8");
    br2BackboneUla = &br2.mInfraIf.FindMatchingAddress("fd00::/8");

    br2.Get<BackboneRouter::BackboneTmfAgent>().SubscribeMulticast(
        br2.Get<BackboneRouter::Local>().GetAllNetworkBackboneRoutersAddress());

    br1.Get<BackboneRouter::BackboneTmfAgent>().SubscribeMulticast(
        br1.Get<BackboneRouter::Local>().GetAllNetworkBackboneRoutersAddress());

    // Add multicast variables to test info manually to ensure verify script sees them.
    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("MA2", kMA2);
    nexus.AddTestVar("MA1g", kMA1g);
    nexus.AddTestVar("HOST_BACKBONE_ULA", hostUla->ToString().AsCString());
    nexus.AddTestVar("BR_2_BACKBONE_ULA", br2BackboneUla->ToString().AsCString());

    // Verify TD sees Primary BBR
    VerifyOrQuit(td.Get<BackboneRouter::Leader>().HasPrimary());

    Log("Step 1: TD registers multicast address, MA1, at BR_1.");

    /**
     * Step 1
     * - Device: TD
     * - Description: User or Harness instructs the device to register multicast address, MA1, at BR_1.
     * - Pass Criteria:
     *   - For DUT = TD: The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR
     *     ALOC>]:MM/n/mr
     *   - Where the payload contains: IPv6 Addresses TLV: MA1
     */
    SuccessOrQuit(td.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    VerifyOrQuit(td.Get<Ip6::Netif>().IsMulticastSubscribed(ma1));

    {
        netifAddr = td.Get<Ip6::Netif>().GetMulticastAddresses().FindMatching(ma1);
        VerifyOrQuit(netifAddr != nullptr);
        VerifyOrQuit(netifAddr->IsMlrCandidate());
    }

    Log("Step 2: BR_1 automatically updates its multicast listener table.");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Automatically updates its multicast listener table.
     * - Pass Criteria:
     *   - None.
     */

    Log("Step 3: BR_1 automatically responds to the multicast registration.");

    /**
     * Step 3
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
     *   - Where the payload contains: Status TLV: 0 (ST_MLR_SUCCESS)
     */

    Log("Step 3a: BR_2 does not respond to the multicast registration.");

    /**
     * Step 3a
     * - Device: BR_2
     * - Description: Does not respond to the multicast registration.
     * - Pass Criteria:
     *   - For DUT = BR_2: The DUT MUST NOT respond to the multicast registration for address, MA1.
     *   - Note: in principle BR_2 should not even receive the request message. However this check is left in as an
     *     overall integrity check for the test setup.
     */

    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("Step 4: BR_1 automatically informs other BBRs on the network of the multicast registration.");

    /**
     * Step 4
     * - Device: BR_1
     * - Description: Automatically informs other BBRs on the network of the multicast registration.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link including to BR_2, as
     *     follows: coap://[<All network BBR multicast>]:BB/b/bmr
     *   - Where the payload contains: IPv6 Addresses TLV: MA1
     *   - Timeout TLV: The value 3600
     * - Note: This check is intentionally skipped in the verification script.
     */

    Log("Step 5: BR_1 automatically multicasts an MLDv2 message to start listening to the group MA1.");

    /**
     * Step 5
     * - Device: BR_1
     * - Description: Automatically multicasts an MLDv2 message to start listening to the group MA1.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST multicast an MLDv2 message of type "Version 2 Multicast Listener Report" (see
     *     [RFC 3810] Section 5.2).
     *   - Where: Nr of Mcast Address Records (M): at least 1
     *   - Multicast Address Record [1] contains the following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE)
     *   - Number of Sources (N): 0
     *   - Multicast Address: MA1
     * - Note: This check is intentionally skipped in the verification script.
     */

    Log("Step 6: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.");

    /**
     * Step 6
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *     address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 7: BR_2 does not forward the ping request packet to its Thread Network.");

    /**
     * Step 7
     * - Device: BR_2
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1,
     *     to the Thread Network - e.g. as a new MPL packet listing itself as the MPL Seed, or in other ways.
     */

    Log("Step 8: BR_1 automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to
     *     its Thread Network encapsulated in an MPL packet.
     *   - Where MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 9: TD receives the multicast ping request packet and automatically sends an ICMPv6 Echo (ping) Reply "
        "packet back to Host.");

    /**
     * Step 9
     * - Device: TD
     * - Description: Receives the multicast ping request packet and automatically sends an ICMPv6 Echo (ping) Reply
     *     packet back to Host.
     * - Pass Criteria:
     *   - For DUT = BR_1: Host MUST receive the ICMPv6 Echo (ping) Reply packet from TD.
     *   - For DUT = BR_2: Host MUST receive the ICMPv6 Echo (ping) Reply packet from TD.
     *   - For DUT = TD: Host SHOULD receive the ICMPv6 Echo (ping) Reply packet from TD. If it is not received, a
     *     warning is generated in the test report for this step, but the validation passes.
     *   - Note: if the TD DUT is a Posix device, it may not be able to process multicast ping requests.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 10: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA2.");

    /**
     * Step 10
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *     address, MA2.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma2, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 11: BR_2 does not forward the ping request packet to its Thread Network.");

    /**
     * Step 11
     * - Device: BR_2
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA2,
     *     to the Thread Network in whatever way.
     */

    Log("Step 12: BR_1 does not forward the ping request packet to its Thread Network.");

    /**
     * Step 12
     * - Device: BR_1
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA2,
     *     to its Thread Network in whatever way.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 13: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1g.");

    /**
     * Step 13
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *     address, MA1g.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1g, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 14: BR_2 does not forward the ping request packet to its Thread Network.");

    /**
     * Step 14
     * - Device: BR_2
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_2: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1g,
     *     to the Thread Network in whatever way.
     */

    Log("Step 15: BR_1 does not forward the ping request packet to its Thread Network.");

    /**
     * Step 15
     * - Device: BR_1
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - For DUT = BR_1: The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1g,
     *     to its Thread Network in whatever way.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 16: Host sends an ICMPv6 Echo (ping) Request packet to the global unicast address, Gg.");

    /**
     * Step 16
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the global unicast
     *     address, Gg.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, *br2BackboneUla, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 17: BR_2 receives and provides the ICMPv6 Echo (ping) Reply.");

    /**
     * Step 17
     * - Device: BR_2
     * - Description: Receives and provides the ICMPv6 Echo (ping) Reply. Note: this step is included as a very
     *     important test topology connectivity test. Because the previous steps only validate negative outcomes ("not
     *     forward"), it means the testing is only useful if the DUT BR_2 was actually up and running during these test
     *     steps. This ping aims to verify this.
     * - Pass Criteria:
     *   - For DUT = BR_2: The DUT MUST send ICMPv6 Echo (ping) Reply to the Host.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc2();
    printf("All tests passed\n");
    return 0;
}
