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
 * Time to advance for the BBR selection to complete, in milliseconds.
 */
static constexpr uint32_t kBbrSelectionTime = 10 * 1000;

/**
 * Reregistration Delay in seconds.
 */
static constexpr uint16_t kReregistrationDelay = 5;

/**
 * MLR Timeout in seconds.
 */
static constexpr uint32_t kMlrTimeout = 3600;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Multicast address MA1 (admin-local).
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * Global IPv6 address for Host.
 */
static const char kHostGua[] = "fd00:7d03:7d03:7d03::1";

void TestMatnTc10(void)
{
    /**
     * 5.10.8 MATN-TC-10: Failure of Primary BBR – Inbound Multicast
     *
     * 5.10.8.1 Topology
     * - BR_1: Test bed BR device operating as the Primary BBR.
     * - BR_2 (DUT): BR Device initially operating as a Secondary BBR. BR_1 is disconnected during this test; the DUT
     *   becomes the Primary BBR afterwards.
     * - ROUTER: Test bed device operating as a Thread Router.
     * - HOST: Test bed BR device operating as a non-Thread device able to send a multicast packet to BR_2.
     *
     * 5.10.8.2 Purpose & Description
     * Verify that a Secondary BBR can take over forwarding of inbound multicast transmissions to the Thread Network
     *   when the Primary BBR disconnects. The Secondary in that case becomes Primary.
     *
     * Spec Reference | V1.2 Section
     * ---------------|-------------
     * Multicast      | 5.10.1
     */

    Core         nexus;
    Node        &br1    = nexus.CreateNode();
    Node        &br2    = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &host   = nexus.CreateNode();
    Ip6::Address ma1;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("ROUTER");
    host.SetName("HOST");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation – BR_1, ROUTER, BR_2 (DUT)");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1, Router, BR_2 (DUT)
     * - Pass Criteria:
     *   - N/A
     */
    br1.AllowList(router);
    br1.AllowList(br2);
    router.AllowList(br1);
    router.AllowList(br2);
    br2.AllowList(br1);
    br2.AllowList(router);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    /**
     * P1: Router is configured such that it cannot become Leader when BR_1 disconnects during the test.
     */
    router.Get<Mle::Mle>().SetLeaderWeight(0);

    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    /**
     * Set BR_2 leader weight high to ensure it becomes leader.
     */
    br2.Get<Mle::Mle>().SetLeaderWeight(255);

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    /** Configure BR_2 reregistration delay. */
    {
        BackboneRouter::Config config;
        br2.Get<BackboneRouter::Local>().GetConfig(config);
        config.mReregistrationDelay = kReregistrationDelay;
        config.mMlrTimeout          = kMlrTimeout;
        SuccessOrQuit(br2.Get<BackboneRouter::Local>().SetConfig(config));
    }

    host.mInfraIf.Init(host);
    host.mInfraIf.AddAddress(host.mInfraIf.GetLinkLocalAddress());

    {
        Ip6::Address hostGua;
        SuccessOrQuit(hostGua.FromString(kHostGua));
        host.mInfraIf.AddAddress(hostGua);
    }

    nexus.AdvanceTime(kBbrSelectionTime);
    nexus.AdvanceTime(kStabilizationTime);

    Log("BR_1 State: %u, BR_2 State: %u", br1.Get<BackboneRouter::Local>().GetState(),
        br2.Get<BackboneRouter::Local>().GetState());

    nexus.AddTestVar("MA1", kMA1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Router registers multicast address, MA1, at BR_1.");

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register multicast address, MA1, at BR_1.
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(router.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: BR_1 automatically responds to Router’s multicast registration.");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Automatically responds to Router’s multicast registration.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br1.Get<BackboneRouter::MulticastListenersTable>().Has(ma1));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_1 automatically informs other BBRs on the network of the multicast registration.");

    /**
     * Step 3
     * - Device: BR_1
     * - Description: Automatically informs other BBRs on the network of the multicast registration. Informative:
     *   Multicasts a BMLR.ntf CoAP request to the Backbone Link including to BR_2, as follows:
     *   coap://[<All network BBR multicast>]:BB/b/bmr Where the payload contains: IPv6 Addresses TLV: MA1, Timeout TLV:
     *   3600
     * - Pass Criteria:
     *   - N/A
     *
     * Note: The BMLR.ntf (or BLMR.ntf) check is intentionally skipped in the Nexus test.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: BR_2 (DUT) receives BMLR.ntf and optionally updates its Backup Multicast Listener Table.");

    /**
     * Step 4
     * - Device: BR_2 (DUT)
     * - Description: Receives BMLR.ntf and optionally updates its Backup Multicast Listener Table.
     * - Pass Criteria:
     *   - None
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.");

    /**
     * Step 5
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    {
        Ip6::Address hostGua;
        SuccessOrQuit(hostGua.FromString(kHostGua));
        host.mInfraIf.SendEchoRequest(hostGua, ma1, kEchoIdentifier, kEchoPayloadSize);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: BR_1 automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 6
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to its Thread Network. Informative: Forwards the
     *   ping request packet with multicast address, MA1, to its Thread Network encapsulated in an MPL packet, where:
     *   MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: BR_2 (DUT) does not forward the ping request packet to its Thread Network.");

    /**
     * Step 7
     * - Device: BR_2 (DUT)
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the Thread
     *     Network, in whatever way, e.g. as a new MPL packet listing itself as the MPL Seed.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router receives the MPL packet and unicasts an ICMPv6 Echo (ping) Reply packet back to Host.");

    /**
     * Step 8
     * - Device: Router
     * - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
     *   unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8a: BR_1 powers down");

    /**
     * Step 8a
     * - Device: BR_1
     * - Description: Harness instructs the device to powerdown
     * - Pass Criteria:
     *   - N/A
     */
    br1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kAttachToRouterTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8b: BR_2 (DUT) becomes Primary BBR");

    /**
     * Step 8b
     * - Device: BR_2 (DUT)
     * - Description: Detects the missing Primary BBR and automatically becomes the Primary BBR of the Thread Network,
     *   distributing its BBR dataset.
     * - Pass Criteria:
     *   - The DUT MUST it distribute its BBR Dataset (with BR_2’s RLOC16) to Router.
     *   - Note the value of the Reregistration Delay of BR_2.
     */
    VerifyOrQuit(br2.Get<BackboneRouter::Local>().GetState() == BackboneRouter::Local::kStatePrimary);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router detects new BBR RLOC and automatically starts its re-registration timer.");

    /**
     * Step 9
     * - Device: Router
     * - Description: Detects new BBR RLOC and automatically starts its re-registration timer.
     * - Pass Criteria:
     *   - N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.");

    /**
     * Step 10
     * - Device: Host
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
     *   MA1.
     * - Pass Criteria:
     *   - N/A
     */
    {
        Ip6::Address hostGua;
        SuccessOrQuit(hostGua.FromString(kHostGua));
        host.mInfraIf.SendEchoRequest(hostGua, ma1, kEchoIdentifier + 1, kEchoPayloadSize);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: BR_2 (DUT) optionally forwards the ping request packet to its Thread Network.");

    /**
     * Step 11
     * - Device: BR_2 (DUT)
     * - Description: Optionally forwards the ping request packet to its Thread Network. Note: forwarding may occur if
     *   Router did already register (step 14), or if the DUT kept a Backup Multicast Listeners Table which is an
     *   optional feature.
     * - Pass Criteria:
     *   - The DUT MAY forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
     *     encapsulated in an MPL packet, where:
     *   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router optional: Receives the multicast ping request packet and sends a ICMPv6 Echo (ping) Reply.");

    /**
     * Step 12
     * - Device: Router
     * - Description: Optional: Receives the multicast ping request packet and sends a ICMPv6 Echo (ping) Reply packet
     *   back to Host. Informative: Only if BR_2 forwarded in step 11: Receives the MPL packet containing an
     *   encapsulated ping request packet to MA1, sent by Host, and unicasts a ping response packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router waits for the re-registration timer to expire.");

    /**
     * Step 13
     * - Device: Router_1
     * - Description: Wait for the re-registration timer to expire (according to the value of the Reregistration Delay
     *   distributed in the BBR Dataset of the DUT (BR_2) and note earlier).
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kReregistrationDelay * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router automatically re-registers for multicast address, MA1, at the DUT (BR_2).");

    /**
     * Step 14
     * - Device: Router
     * - Description: Automatically re-registers for multicast address, MA1, at the DUT (BR_2). Note: this step may have
     *   happened already any time after step 9.
     * - Pass Criteria:
     *   - N/A
     */
    // Explicitly trigger re-registration to ensure it happens within the test time frame.
    SuccessOrQuit(router.Get<ThreadNetif>().UnsubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(10 * 1000);
    SuccessOrQuit(router.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(30 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: BR_2 (DUT) automatically responds to the multicast registration.");

    /**
     * Step 15
     * - Device: BR_2 (DUT)
     * - Description: Automatically responds to the multicast registration.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to Router as follows: 2.04 changed
     *   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
     */
    VerifyOrQuit(br2.Get<BackboneRouter::MulticastListenersTable>().Has(ma1));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: BR_2 (DUT) automatically informs other BBRs on the network of the multicast registration.");

    /**
     * Step 16
     * - Device: BR_2 (DUT)
     * - Description: Automatically informs other BBRs on the network of the multicast registration.
     * - Pass Criteria:
     *   - The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link, as follows:
     *     coap://[<All network BBR multicast>]:BB/b/bmr
     *   - Where the payload contains: IPv6 Addresses TLV: MA1
     *   - Timeout TLV: MLR timeout of BR_2
     *
     * Note: The BMLR.ntf (or BLMR.ntf) check is intentionally skipped in the Nexus test.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: BR_1 does not receive the message nor respond, as it is turned off.");

    /**
     * Step 17
     * - Device: BR_1
     * - Description: Does not receive the message nor respond, as it is turned off.
     * - Pass Criteria:
     *   - N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: BR_2 (DUT) automatically multicasts an MLDv2 message.");

    /**
     * Step 18
     * - Device: BR_2 (DUT)
     * - Description: Automatically multicasts an MLDv2 message.
     * - Pass Criteria:
     *   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810] Section
     *     5.2). Where: Nr of Mcast Address Records (M): at least 1
     *   - Multicast Address Record [1]: See below
     *   - The Multicast Address Record contains the following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE)
     *   - Number of Sources (N): 0
     *   - Multicast Address: MA1
     *
     * Note: The MLDv2 check is intentionally skipped in the Nexus test.
     */
    nexus.AdvanceTime(60 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: BR_1 does not receive the message nor does it respond, as it is turned off.");

    /**
     * Step 19
     * - Device: BR_1
     * - Description: Does not receive the message nor does it respond, as it is turned off.
     * - Pass Criteria:
     *   - N/A
     */

    nexus.SaveTestInfo("test_1_2_MATN_TC_10.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc10();
    printf("All tests passed\n");
    return 0;
}
