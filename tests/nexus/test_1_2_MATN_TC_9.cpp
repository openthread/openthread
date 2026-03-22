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

void TestMatnTc9(void)
{
    /**
     * 5.10.7 MATN-TC-09: Failure of Primary BBR – Outbound Multicast
     *
     * 5.10.7.1 Topology
     * - BR_1: Test bed BR device initially operating as the Primary BBR.
     * - BR_2 (DUT): BR Device initially operating as a Secondary BBR. BR_1 is disconnected during this test; the DUT
     *   becomes the Primary BBR afterwards.
     * - Router: Test bed device operating as a Thread Router.
     *
     * 5.10.7.2 Purpose & Description
     * Verify that a Secondary BBR can take over forwarding of outbound multicast transmissions from the Thread Network
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
    Ip6::Address ma1;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("Router");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation – BR_1, Router, BR_2 (DUT)");

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

    nexus.AdvanceTime(kBbrSelectionTime);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.AddTestVar("MA1", kMA1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Router sends ICMPv6 Echo Request to MA1");

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
     *   MA1, encapsulated in an MPL packet
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: BR_1 forwards to LAN");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Automatically forwards the multicast ping request packet with multicast address, MA1, to the LAN.
     * - Pass Criteria:
     *   - N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_2 (DUT) does not forward to LAN");

    /**
     * Step 3
     * - Device: BR_2 (DUT)
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the
     *     LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4a: BR_1 powers down");

    /**
     * Step 4a
     * - Device: BR_1
     * - Description: Harness instructs the device to power-down
     * - Pass Criteria:
     *   - N/A
     */
    br1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kAttachToRouterTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4b: BR_2 (DUT) becomes Leader/PBBR");

    /**
     * Step 4b
     * - Device: BR_2 (DUT)
     * - Description: Detects the missing Primary BBR and automatically becomes the Leader/PBBR of the Thread Network,
     *   distributing its BBR dataset.
     * - Pass Criteria:
     *   - The DUT MUST become the Leader of the Thread Network by checking the Leader data.
     *   - Router MUST receive the new BBR Dataset from BR_2, where:
     *   - • RLOC16 in Server TLV == The RLOC16 of BR_2
     *   - All fields in the BBR Dataset Service TLV MUST contain valid values.
     */
    VerifyOrQuit(br2.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router sends ICMPv6 Echo Request to MA1");

    /**
     * Step 5
     * - Device: Router
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast address,
     *   MA1, encapsulated in an MPL packet.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma1, kEchoIdentifier + 1, kEchoPayloadSize);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: BR_2 (DUT) forwards to LAN");

    /**
     * Step 6
     * - Device: BR_2 (DUT)
     * - Description: Automatically forwards the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet with multicast address, MA1, to the LAN.
     */

    nexus.SaveTestInfo("test_1_2_MATN_TC_9.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc9();
    printf("All tests passed\n");
    return 0;
}
