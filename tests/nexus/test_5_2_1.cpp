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

#include <stdarg.h>
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
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachToChildTime = 10 * 1000;

void Test5_2_1(void)
{
    /**
     * 5.2.1 REED Attach
     *
     * 5.2.1.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - REED_1
     * - MED_1
     *
     * 5.2.1.2 Purpose & Description
     * The purpose of this test case is to show that the DUT is able to attach a REED and forward address solicits
     *   two hops away from the Leader.
     *
     * Spec Reference                               | V1.1 Section    | V1.3.0 Section
     * ---------------------------------------------|-----------------|---------------
     * Attaching to a Parent / Router ID Assignment | 4.7.1 / 5.9.10  | 4.5.1 / 5.9.10
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();
    Node &reed1  = nexus.CreateNode();
    Node &med1   = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");
    reed1.SetName("REED_1");
    med1.SetName("MED_1");

    // Establish topology using AllowList
    nexus.AllowLinkBetween(dut, leader);
    nexus.AllowLinkBetween(dut, reed1);
    nexus.AllowLinkBetween(reed1, med1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Router_1 (DUT)");

    /**
     * Step 1: Router_1 (DUT)
     * - Description: Attach to Leader and sends properly formatted MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in MLE Advertisements:
     *     - Source Address TLV
     *     - Leader Data TLV
     *     - Route64 TLV
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: REED_1");

    /**
     * Step 2: REED_1
     * - Description: Attach REED_1 to DUT; REED_1 automatically sends MLE Parent Request.
     * - Pass Criteria: N/A
     */
    reed1.Join(dut);
    nexus.AdvanceTime(kAttachToChildTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1 (DUT)");

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically sends an MLE Parent Response.
     * - Pass Criteria:
     *   - The DUT MUST respond with a MLE Parent Response.
     *   - The following TLVs MUST be present in the MLE Parent Response:
     *     - Challenge TLV
     *     - Connectivity TLV
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Link Margin TLV
     *     - Response TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1 (DUT)");

    /**
     * Step 4: Router_1 (DUT)
     * - Description: Automatically sends an MLE Child ID Response.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Response.
     *   - The following TLVs MUST be present in the Child ID Response:
     *     - Address16 TLV
     *     - Leader Data TLV
     *     - Network Data TLV
     *     - Source Address TLV
     *     - Route64 TLV (optional)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: MED_1");

    /**
     * Step 6: MED_1
     * - Description: The harness attaches MED_1 to REED_1.
     * - Pass Criteria: N/A
     */
    med1.Join(reed1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToChildTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: REED_1");

    /**
     * Step 7: REED_1
     * - Description: Automatically sends an Address Solicit Request to DUT.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_1 (DUT)");

    /**
     * Step 8: Router_1 (DUT)
     * - Description: Automatically forwards Address Solicit Request to Leader, and forwards Leader's Address Solicit
     *     Response to REED_1.
     * - Pass Criteria:
     *   - The DUT MUST forward the Address Solicit Request to the Leader.
     *   - The DUT MUST forward the Leader's Address Solicit Response to REED_1.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 9: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to REED_1.
     * - Pass Criteria:
     *   - REED_1 responds with ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, reed1.Get<Mle::Mle>().GetMeshLocalEid());

    nexus.SaveTestInfo("test_5_2_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_2_1();
    printf("All tests passed\n");
    return 0;
}
