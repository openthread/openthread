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
#include "thread/link_quality.hpp"

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
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Default noise floor used for RSSI calculations.
 */
static constexpr int8_t kDefaultNoiseFloor = -100;

/**
 * Typical RSSI for link quality 2.
 */
static const int8_t kRssiQuality2 = GetTypicalRssForLinkQuality(kDefaultNoiseFloor, kLinkQuality2);

/**
 * Typical RSSI for link quality 3.
 */
static const int8_t kRssiQuality3 = GetTypicalRssForLinkQuality(kDefaultNoiseFloor, kLinkQuality3);

void Test5_1_11(void)
{
    /**
     * 5.1.11 Attaching to a REED with better link quality
     *
     * 5.1.11.1 Topology
     * - Leader
     * - REED_1
     * - Router_2
     * - Router_1 (DUT)
     *
     * 5.1.11.2 Purpose & Description
     * The purpose of this test case is to validate that DUT will attach to a REED with the highest link quality,
     * when routers with the highest link quality are not available.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Parent Selection | 4.7.2        | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &reed1   = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    reed1.SetName("REED_1");
    router2.SetName("ROUTER_2");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    /**
     * Use AllowList feature to restrict the topology.
     */

    /**
     * Leader <-> REED_1
     */
    leader.AllowList(reed1);
    reed1.AllowList(leader);

    /**
     * Leader <-> Router_2
     */
    leader.AllowList(router2);
    router2.AllowList(leader);

    /**
     * REED_1 <-> DUT
     */
    reed1.AllowList(dut);
    dut.AllowList(reed1);

    /**
     * Router_2 <-> DUT
     */
    router2.AllowList(dut);
    dut.AllowList(router2);

    /**
     * REED_1 <-> Router_2 (to ensure they can see each other if needed, though not strictly required by topology)
     */
    reed1.AllowList(router2);
    router2.AllowList(reed1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader, REED_1, Router_2");

    /**
     * Step 1: Leader, REED_1, Router_2
     * - Description: Setup the topology without the DUT. Verify Leader and Router_2 are sending MLE
     *   Advertisements.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    reed1.Get<Mle::Mle>().SetRouterUpgradeThreshold(0);
    reed1.Join(leader);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());

    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Test Harness");

    /**
     * Step 2: Test Harness
     * - Description: Harness configures the RSSI between Router_2 & Router_1 (DUT) to enable a link quality of 2
     *   (medium).
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(router2.Get<Mac::Mac>().GetExtAddress(), kRssiQuality2));
    SuccessOrQuit(router2.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiQuality2));

    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(reed1.Get<Mac::Mac>().GetExtAddress(), kRssiQuality3));
    SuccessOrQuit(reed1.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiQuality3));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1 (DUT)");

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically sends a MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV
     */
    SuccessOrQuit(dut.Get<Mle::Mle>().SetRouterEligible(false));
    dut.Join(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_2");

    /**
     * Step 4: Router_2
     * - Description: Automatically responds to DUT with MLE Parent Response.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1 (DUT)");

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Automatically sends another MLE Parent Request - to Routers and REEDs - when it doesn’t see the
     *   highest link quality in Router_2’s response.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request with the Scan Mask set to All Routers and REEDs.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0xC0 (Routers and REEDs)
     *     - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1 (DUT)");

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically sends MLE Child ID Request to REED_1 due to its better link quality.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to REED_1, including the following TLVs:
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The following TLV MUST NOT be present in the Child ID Request:
     *     - Address Registration TLV
     */
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == reed1.Get<Mac::Mac>().GetExtAddress());

    nexus.SaveTestInfo("test_5_1_11.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_11();
    printf("All tests passed\n");
    return 0;
}
