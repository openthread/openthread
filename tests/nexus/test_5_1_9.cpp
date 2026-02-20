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
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kJoinTime = 20 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 * This duration accounts for MLE attach process and ROUTER_SELECTION_JITTER.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance to wait for the first Parent Request (Routers only) and lack of response.
 */
static constexpr uint32_t kWaitFirstParentRequestTime = 1000;

/**
 * Time to advance to wait for the second Parent Request (Routers and REEDs).
 */
static constexpr uint32_t kWaitSecondParentRequestTime = 2000;

/**
 * RSSI to enable a link quality of 3.
 */
static constexpr int8_t kLq3Rssi = -50;

void Test5_1_9(void)
{
    /**
     * 5.1.9 Attaching to a REED with better connectivity
     *
     * 5.1.9.1 Topology
     * - Leader
     * - Router_1
     * - REED_1
     * - REED_2
     * - Router_2 (DUT)
     *
     * 5.1.9.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT will pick REED_1 as its parent because of its better
     *   connectivity.
     * - In order for this test case to be valid, the link quality between all nodes must be of the highest quality
     *   (3). If this condition cannot be met the test case is invalid.
     *
     * Spec Reference                             | V1.1 Section    | V1.3.0 Section
     * -------------------------------------------|-----------------|-----------------
     * Parent Selection                           | 4.7.2           | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &reed1   = nexus.CreateNode();
    Node &reed2   = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    reed1.SetName("REED_1");
    reed2.SetName("REED_2");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    /**
     * Step 1: Leader, Router_1, REED_2, REED_1
     * - Description: Setup the topology without the DUT. Verify Router_1 and the Leader are sending MLE
     *   advertisements.
     * - Pass Criteria: N/A
     */
    Log("Step 1: Setup the topology without the DUT");

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, reed1);
    nexus.AllowLinkBetween(leader, reed2);
    nexus.AllowLinkBetween(router1, reed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    reed1.Join(leader);
    reed1.Get<Mle::Mle>().SetRouterUpgradeThreshold(0);
    reed2.Join(leader);
    reed2.Get<Mle::Mle>().SetRouterUpgradeThreshold(0);
    nexus.AdvanceTime(kJoinTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(reed2.Get<Mle::Mle>().IsChild());

    /**
     * Step 2: Test Harness
     * - Description: Harness configures the RSSI between Leader, Router_1, Router_2 (DUT), REED_1, and REED_2 to enable
     *   a link quality of 3 (highest).
     * - Pass Criteria: N/A
     */
    Log("Step 2: Harness configures the RSSI");

    IgnoreError(leader.Get<Mac::Filter>().AddRssIn(router1.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(leader.Get<Mac::Filter>().AddRssIn(reed1.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(leader.Get<Mac::Filter>().AddRssIn(reed2.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));

    IgnoreError(router1.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(router1.Get<Mac::Filter>().AddRssIn(reed1.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));

    IgnoreError(reed1.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(reed1.Get<Mac::Filter>().AddRssIn(router1.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));

    IgnoreError(reed2.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));

    // Setup DUT connectivity (only to REEDs)
    nexus.AllowLinkBetween(dut, reed1);
    nexus.AllowLinkBetween(dut, reed2);

    IgnoreError(dut.Get<Mac::Filter>().AddRssIn(reed1.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(dut.Get<Mac::Filter>().AddRssIn(reed2.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(reed1.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));
    IgnoreError(reed2.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kLq3Rssi));

    /**
     * Step 3: Router_2 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop
     *     Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Mode TLV
     *     - Challenge TLV
     *     - Scan Mask TLV = 0x80 (Active Routers)
     *     - Version TLV
     */
    Log("Step 3: Router_2 (DUT) begins attach process");
    dut.Join(reed1, Node::kAsFed); // reed1 is just a placeholder here, it will scan.

    /**
     * Step 4: REED_2, REED_1
     * - Description: Devices do not respond to the All-Routers Parent Request.
     * - Pass Criteria: N/A
     */
    Log("Step 4: REEDs do not respond to the All-Routers Parent Request");
    nexus.AdvanceTime(kWaitFirstParentRequestTime); // Wait for the first Parent Request and lack of response.

    /**
     * Step 5: Router_2 (DUT)
     * - Description: Automatically sends MLE Parent Request with Scan Mask set to Routers and REEDs.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop
     *     Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0xC0 (Active Routers and REEDs)
     *     - Version TLV
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to '0x02'
     */
    Log("Step 5: Router_2 (DUT) sends MLE Parent Request with Scan Mask set to Routers and REEDs");
    // This happens automatically in Join process when no router responds.
    nexus.AdvanceTime(kWaitSecondParentRequestTime);

    /**
     * Step 6: REED_1, REED_2
     * - Description: Each device automatically responds to DUT with MLE Parent Response. REED_1 reports more high
     *   quality connection than REED_2 in the Connectivity TLV.
     * - Pass Criteria: N/A
     */
    Log("Step 6: REEDs respond with MLE Parent Response");
    // REEDs respond automatically.

    /**
     * Step 7: Router_2 (DUT)
     * - Description: Automatically sends a MLE Child ID Request to REED_1.
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
    Log("Step 7: Router_2 (DUT) sends a MLE Child ID Request to REED_1");
    // Wait for Child ID exchange.
    nexus.AdvanceTime(kJoinTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == reed1.Get<Mac::Mac>().GetExtAddress());

    nexus.SaveTestInfo("test_5_1_9.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_9();
    printf("All tests passed\n");
    return 0;
}
