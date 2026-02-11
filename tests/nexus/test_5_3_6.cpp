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
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

void Test5_3_6(void)
{
    /**
     * 5.3.6 Router ID Mask
     *
     * 5.3.6.1 Topology
     * - Router_1
     * - Router_2
     * - Leader (DUT)
     *
     * 5.3.6.2 Purpose & Description
     * The purpose of this test case is to verify that the router ID mask is managed correctly, as the connectivity to a
     *   router or group of routers is lost and / or a new router is added to the network.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * Router ID Management  | 5.9.9        | 5.9.9
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     *
     *   - Description: Ensure topology is formed correctly.
     *   - Pass Criteria: N/A
     */

    /**
     * Use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - Leader (DUT) and Router 1
     * - Router 1 and Router 2
     */
    leader.AllowList(router1);
    router1.AllowList(leader);
    router1.AllowList(router2);
    router2.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    uint16_t router1Rloc16 = router1.Get<Mle::Mle>().GetRloc16();
    uint16_t router2Rloc16 = router2.Get<Mle::Mle>().GetRloc16();
    uint8_t  router1Id     = Mle::RouterIdFromRloc16(router1Rloc16);
    uint8_t  router2Id     = Mle::RouterIdFromRloc16(router2Rloc16);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_2");

    /**
     * Step 2: Router_2
     *
     *   - Description: Harness silently disables the device.
     *   - Pass Criteria: N/A
     */
    router2.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Delay");

    /**
     * Step 3: Delay
     *
     *   - Description: Pause for 12 minutes.
     *   - Pass Criteria: N/A
     */
    nexus.AdvanceTime(12 * Time::kOneMinuteInMsec);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     *
     *   - Description: The DUT updates its routing cost and ID set.
     *   - Pass Criteria:
     *     - The DUT’s routing cost to Router_2 MUST count to infinity.
     *     - The DUT MUST remove Router_2 ID from its ID set.
     *     - Verify route data has settled.
     */
    VerifyOrQuit(leader.Get<RouterTable>().GetPathCost(router2Rloc16) >= Mle::kMaxRouteCost);
    VerifyOrQuit(!leader.Get<RouterTable>().IsAllocated(router2Id));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_2");

    /**
     * Step 5: Router_2
     *
     *   - Description: Harness re-enables the device and waits for it to reattach and upgrade to a
     *     router.
     *   - Pass Criteria:
     *     - The DUT MUST reset the MLE Advertisement trickle timer and send an Advertisement.
     */
    SuccessOrQuit(router2.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kAttachToRouterTime + kStabilizationTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    router2Rloc16 = router2.Get<Mle::Mle>().GetRloc16();
    router2Id     = Mle::RouterIdFromRloc16(router2Rloc16);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1, Router_2");

    /**
     * Step 6: Router_1, Router_2
     *
     *   - Description: Harness silently disables both devices.
     *   - Pass Criteria:
     *     - The DUT’s routing cost to Router_1 MUST go directly to infinity as there is no
     *       multi-hop cost for Router_1.
     *     - The DUT MUST remove Router_1 & Router_2 IDs from its ID set.
     */
    router1.Get<Mle::Mle>().Stop();
    router2.Get<Mle::Mle>().Stop();

    nexus.AdvanceTime(12 * Time::kOneMinuteInMsec);

    VerifyOrQuit(leader.Get<RouterTable>().GetPathCost(router1Rloc16) >= Mle::kMaxRouteCost);
    VerifyOrQuit(!leader.Get<RouterTable>().IsAllocated(router1Id));
    VerifyOrQuit(!leader.Get<RouterTable>().IsAllocated(router2Id));

    nexus.SaveTestInfo("test_5_3_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_6();
    printf("All tests passed\n");
    return 0;
}
