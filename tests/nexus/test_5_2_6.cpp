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
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a large number of nodes to become routers.
 */
static constexpr uint32_t kRouterSelectionWaitTime = 400 * 1000;

/**
 * Time to wait for DUT to resign its Router ID.
 */
static constexpr uint32_t kDowngradeWaitTime = 300 * 1000;

/**
 * Time to wait for ICMPv6 Echo response.
 */
static constexpr uint32_t kEchoResponseWaitTime = 10000;

/**
 * Router thresholds.
 */
static constexpr uint8_t kRouterUpgradeThreshold   = 32;
static constexpr uint8_t kRouterDowngradeThreshold = 32;

/**
 * Number of routers for the test.
 */
static constexpr uint16_t kInitialRouterCount = 23;

void Test_5_2_6(void)
{
    /**
     * 5.2.6 Router Downgrade Threshold - REED
     *
     * 5.2.6.1 Topology
     * - Build a topology with 23 active routers, including the Leader, with no communication constraints and links of
     *   highest quality (quality=3)
     * - Set Router Downgrade Threshold and Router Upgrade Threshold on all test bed routers and Leader to 32
     *
     * 5.2.6.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT will downgrade to a REED when the network becomes too
     *   dense and the Router Downgrade Threshold conditions are met.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * Router ID Management  | 5.9.9        | 5.9.9
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode(); // DUT
    Node *routers[kInitialRouterCount - 2];

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

    for (uint16_t i = 0; i < kInitialRouterCount - 2; i++)
    {
        routers[i] = &nexus.CreateNode();
        char name[16];
        snprintf(name, sizeof(name), "ROUTER_%u", i + 2);
        routers[i]->SetName(name);
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(kRouterUpgradeThreshold);
    leader.Get<Mle::Mle>().SetRouterDowngradeThreshold(kRouterDowngradeThreshold);

    for (uint16_t i = 0; i < kInitialRouterCount - 2; i++)
    {
        routers[i]->Get<Mle::Mle>().SetRouterUpgradeThreshold(kRouterUpgradeThreshold);
        routers[i]->Get<Mle::Mle>().SetRouterDowngradeThreshold(kRouterDowngradeThreshold);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All - Ensure topology is formed correctly without Router_24");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly without Router_24,
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter(), "Router 1 did not become a router");

    for (uint16_t i = 0; i < kInitialRouterCount - 2; i++)
    {
        routers[i]->Join(leader);
    }
    nexus.AdvanceTime(kRouterSelectionWaitTime);

    VerifyOrQuit(leader.Get<RouterTable>().GetActiveRouterCount() == kInitialRouterCount);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_24 attaches to the network");

    /**
     * Step 2: Router_24
     * - Description: Harness causes Router_24 to attach to the network and ensures it has a link of quality 2 or
     *   better to Router_1 and Router_2
     * - Pass Criteria: N/A
     */
    Node &router24 = nexus.CreateNode();
    router24.SetName("ROUTER_24");
    router24.Get<Mle::Mle>().SetRouterUpgradeThreshold(kRouterUpgradeThreshold);
    router24.Get<Mle::Mle>().SetRouterDowngradeThreshold(kRouterDowngradeThreshold);

    router24.Join(leader);
    for (uint32_t i = 0; i < kAttachToRouterTime / 1000; i++)
    {
        nexus.AdvanceTime(1000);
        if (leader.Get<RouterTable>().GetActiveRouterCount() == kInitialRouterCount + 1)
        {
            break;
        }
    }

    VerifyOrQuit(leader.Get<RouterTable>().GetActiveRouterCount() == kInitialRouterCount + 1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1 (DUT) resigns its Router ID");

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Allow enough time for the DUT to get Network Data Updates and resign its Router ID.
     * - Pass Criteria:
     *   - The DUT MUST first reconnect to the network as a Child by sending properly formatted Parent Request and
     *     Child ID Request messages.
     *   - Once the DUT attaches as a Child, it MUST send an Address Release Message to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/ar
     *     - CoAP Payload:
     *       - MAC Extended Address TLV
     *       - RLOC16 TLV,
     */
    nexus.AdvanceTime(kDowngradeWaitTime);
    VerifyOrQuit(!router1.Get<Mle::Mle>().IsRouter(), "DUT did not downgrade to REED");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader receives Address Release message");

    /**
     * Step 4: Leader
     * - Description: Receives Address Release message and automatically sends a 2.04 Changed CoAP response.
     * - Pass Criteria: N/A
     */
    // Stack automatically handles this.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader verifies connectivity to DUT");

    /**
     * Step 5: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   DUT
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, router1.Get<Mle::Mle>().GetMeshLocalEid(), /* aPayloadSize */ 0,
                                   /* aHopLimit */ 64, kEchoResponseWaitTime);

    nexus.SaveTestInfo("test_5_2_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_5_2_6();
    printf("All tests passed\n");
    return 0;
}
