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
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * RSSI values for different link qualities.
 * All these values result in Link Quality 3 (highest).
 */
static constexpr int8_t kRssiHigh = -20;
static constexpr int8_t kRssiLow  = -60;

void Test5_1_8(void)
{
    /**
     * 5.1.8 Attaching to a Router with better connectivity
     *
     * 5.1.8.1 Topology
     * - Leader
     * - Router_1
     * - Router_2
     * - Router_3
     * - Router_4 (DUT)
     *
     * 5.1.8.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT chooses to attach to a router with better connectivity.
     * - In order for this test case to be valid, the link quality between all nodes must be of the highest quality
     * (3). If this condition cannot be met, the test case is invalid.
     *
     * Spec Reference                             | V1.1 Section    | V1.3.0 Section
     * -------------------------------------------|-----------------|-----------------
     * Parent Selection                           | 4.7.2           | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");
    dut.SetName("ROUTER_4");

    Instance::SetLogLevel(kLogLevelInfo);

    nexus.AdvanceTime(0);

    // Use AllowList feature to restrict the topology.
    // L <-> R3
    // R3 <-> R1
    // R1 <-> R2
    // DUT hears R1, R2, R3
    nexus.AllowLinkBetween(leader, router3);
    nexus.AllowLinkBetween(router3, router1);
    nexus.AllowLinkBetween(router1, router2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader, Router_1, Router_2, Router_3");

    /**
     * Step 1: Leader, Router_1, Router_2, Router_3
     * - Description: Setup the topology without the DUT. Verify all routers and Leader are sending MLE advertisements.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    router1.Join(router3);
    nexus.AdvanceTime(kAttachToRouterTime);

    router2.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Test Harness");

    /**
     * Step 2: Test Harness
     * - Description: Harness configures the RSSI between Router_1, Router_2, and Router_3 and Router_4 (DUT) to enable
     *   a link quality of 3 (highest).
     * - Pass Criteria: N/A
     */
    nexus.AllowLinkBetween(dut, router2);
    nexus.AllowLinkBetween(dut, router3);

    // Harness configures the RSSI to prefer Router 3.
    // All values below enable Link Quality 3 (highest).
    auto setRssi = [&](Node &aNodeA, Node &aNodeB, int8_t aRssi) {
        SuccessOrQuit(aNodeA.Get<Mac::Filter>().AddRssIn(aNodeB.Get<Mac::Mac>().GetExtAddress(), aRssi));
        SuccessOrQuit(aNodeB.Get<Mac::Filter>().AddRssIn(aNodeA.Get<Mac::Mac>().GetExtAddress(), aRssi));
    };

    setRssi(dut, router3, kRssiHigh);
    setRssi(dut, router2, kRssiLow);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_4 (DUT)");

    /**
     * Step 3: Router_4 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop
     *     Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (Active Routers)
     *     - Version TLV
     */
    SuccessOrQuit(dut.Get<Mle::Mle>().SetRouterEligible(false));
    dut.Join(router3, Node::kAsFed); // We use router3 as the dataset source, but it will scan and find all

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_2, Router_3");

    /**
     * Step 4: Router_2, Router_3
     * - Description: Each device automatically responds to the DUT with MLE Parent Response.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kAttachAsChildTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_4 (DUT)");

    /**
     * Step 5: Router_4 (DUT)
     * - Description: Automatically sends a MLE Child ID Request to Router_3 due to better connectivity.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to Router_3, including the following TLVs:
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The following TLV MUST NOT be present in the Child ID Request:
     *     - Address Registration TLV
     */
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router3.Get<Mac::Mac>().GetExtAddress());

    nexus.SaveTestInfo("test_5_1_8.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_8();
    printf("All tests passed\n");
    return 0;
}
