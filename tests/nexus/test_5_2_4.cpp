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
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 * This duration accounts for MLE attach process and ROUTER_SELECTION_JITTER.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * REED_ADVERTISEMENT_INTERVAL in milliseconds.
 */
static constexpr uint32_t kReedAdvertisementInterval = 570 * 1000;

/**
 * REED_ADVERTISEMENT_MAX_JITTER in milliseconds.
 */
static constexpr uint32_t kReedAdvertisementMaxJitter = 60 * 1000;

/**
 * Time to wait for MLE advertisement.
 */
static constexpr uint32_t kWaitTime = kReedAdvertisementInterval + kReedAdvertisementMaxJitter;

/**
 * Number of routers in the topology besides the leader.
 */
static constexpr uint16_t kNumRouters = 15;

void Test5_2_4(void)
{
    /**
     * 5.2.4 Router Upgrade Threshold - REED
     *
     * 5.2.4.1 Topology
     * - Each router numbered 1 through 15 have a link to leader
     * - Router 15 and REED_1 (DUT) have a link
     * - REED_1 (DUT) and MED_1 have a link.
     *
     * 5.2.4.2 Purpose & Description
     * The purpose of this test case is to:
     * 1. Verify that the DUT does not attempt to become a router if there are already 16 active routers on the Thread
     *   network AND it is not bringing children.
     * 2. Verify that the DUT transmits MLE Advertisement messages every REED_ADVERTISEMENT_INTERVAL (+
     *   REED_ADVERTISEMENT_MAX_JITTER) seconds.
     * 3. Verify that the DUT upgrades to a router by sending an Address Solicit Request when a child attempts to
     *   attach to it.
     *
     * Spec Reference                              | V1.1 Section   | V1.3.0 Section
     * --------------------------------------------|----------------|----------------
     * Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10 | 5.9.9 / 5.9.10
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];
    Node &reed1 = nexus.CreateNode();
    Node &med1  = nexus.CreateNode();

    leader.SetName("Leader");
    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
        routers[i]->SetName("Router", i + 1);
    }
    reed1.SetName("REED", 1);
    med1.SetName("MED", 1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /** Use AllowList feature to restrict the topology. */
    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        nexus.AllowLinkBetween(leader, *routers[i]);
    }

    /** Router 15 and REED_1 (DUT) have a link */
    nexus.AllowLinkBetween(*routers[kNumRouters - 1], reed1);

    /** REED_1 (DUT) and MED_1 have a link. */
    nexus.AllowLinkBetween(reed1, med1);

    Log("Step 1: Ensure topology is formed correctly without the DUT.");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly without the DUT.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        routers[i]->Join(leader);
    }
    nexus.AdvanceTime(kAttachToRouterTime);
    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouter());
    }

    Log("Step 2: The harness causes the DUT to attach to any node, 2-hops from the Leader.");

    /**
     * Step 2: REED_1 (DUT)
     * - Description: The harness causes the DUT to attach to any node, 2-hops from the Leader.
     * - Pass Criteria: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
     */
    reed1.Join(*routers[kNumRouters - 1]);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());

    Log("Step 3: Automatically sends MLE Advertisements.");

    /**
     * Step 3: REED_1 (DUT)
     * - Description: Automatically sends MLE Advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255, to the Link-Local All Nodes multicast
     *     address (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements:
     *     - Leader Data TLV
     *     - Source Address TLV
     *   - The following TLV MUST NOT be present in the MLE Advertisement:
     *     - Route64 TLV
     */

    Log("Step 4: Wait for REED_ADVERTISEMENT_INTERVAL+ REED_ADVERTISEMENT_MAX_JITTER seconds.");

    /**
     * Step 4: Wait
     * - Description: Wait for REED_ADVERTISEMENT_INTERVAL+ REED_ADVERTISEMENT_MAX_JITTER seconds (default time = 630
     *   seconds).
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kWaitTime);

    Log("Step 5: Automatically sends a MLE Advertisement.");

    /**
     * Step 5: REED_1 (DUT)
     * - Description: Automatically sends a MLE Advertisement.
     * - Pass Criteria: The DUT MUST send a second MLE Advertisement after REED_ADVERTISEMENT_INTERVAL+JITTER where
     *   JITTER <= REED_ADVERTISEMENT_MAX_JITTER.
     */

    Log("Step 6: Automatically sends multicast MLE Parent Request.");

    /**
     * Step 6: MED_1
     * - Description: Automatically sends multicast MLE Parent Request.
     * - Pass Criteria: N/A
     */
    med1.Join(reed1, Node::kAsMed);

    Log("Step 7: Automatically sends MLE Parent Response.");

    /**
     * Step 7: REED_1 (DUT)
     * - Description: Automatically sends MLE Parent Response.
     * - Pass Criteria: The DUT MUST reply with a properly formatted MLE Parent Response.
     */

    Log("Step 8: Automatically sends MLE Child ID Request to the DUT.");

    /**
     * Step 8: MED_1
     * - Description: Automatically sends MLE Child ID Request to the DUT.
     * - Pass Criteria: N/A
     */

    Log("Step 9: Automatically sends an Address Solicit Request to the Leader.");

    /**
     * Step 9: REED_1 (DUT)
     * - Description: Automatically sends an Address Solicit Request to the Leader.
     * - Pass Criteria:
     *   - Verify that the DUT’s Address Solicit Request is properly formatted:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/as
     *     - CoAP Payload:
     *       - MAC Extended Address TLV
     *       - Status TLV
     *       - RLOC16 TLV (optional)
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsRouter());

    Log("Step 10: Optionally, automatically sends a Multicast Link Request after receiving an Address Solicit "
        "Response.");

    /**
     * Step 10: REED_1 (DUT)
     * - Description: Optionally, automatically sends a Multicast Link Request after receiving an Address Solicit
     *   Response from Leader with its new Router ID.
     * - Pass Criteria:
     *   - The DUT MAY send a Multicast Link Request to the Link-Local All-Routers multicast address (FF02::2).
     *   - The following TLVs MUST be present in the Link Request:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - Source Address TLV
     *     - TLV Request TLV: Link Margin
     *     - Version TLV
     */

    Log("Step 11: Automatically sends MLE Child ID Response to MED_1.");

    /**
     * Step 11: REED_1 (DUT)
     * - Description: Automatically sends MLE Child ID Response to MED_1.
     * - Pass Criteria: The DUTs MLE Child ID Response MUST be properly formatted with MED_1’s new 16-bit address.
     */
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Log("Step 12: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.");

    /**
     * Step 12: MED_1
     * - Description: The harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   Leader.
     * - Pass Criteria: The Leader MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(med1, leader.Get<Mle::Mle>().GetMeshLocalEid());

    nexus.SaveTestInfo("test_5_2_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_2_4();
    printf("All tests passed\n");
    return 0;
}
