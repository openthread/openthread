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
 * Time to advance for an isolated router to detect isolation and re-attach, in milliseconds.
 */
static constexpr uint32_t kReattachTime = 120 * 1000;

/**
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 60 * 1000;

/**
 * Timeout for ICMP Echo response, in milliseconds.
 */
static constexpr uint32_t kPingTimeout = 5000;

void Test5_5_5(void)
{
    /**
     * 5.5.5 Split and Merge with REED
     *
     * 5.5.5.1 Topology
     * - Test topology has a total of 16 active routers, including the Leader. Router_1 is restricted only to
     *   communicate with Router_3 and the DUT.
     *
     * 5.5.5.2 Purpose & Description
     * The purpose of this test case is to show that the DUT will upgrade to a Router when Router_3 is eliminated.
     *
     * Spec Reference             | V1.1 Section | V1.3.0 Section
     * ---------------------------|--------------|---------------
     * Thread Network Partitions  | 5.16         | 5.16
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node *r[16];

    for (uint16_t i = 1; i <= 15; i++)
    {
        r[i] = &nexus.CreateNode();
        r[i]->SetName("ROUTER", i);
    }

    Node &dut = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("REED", 1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly without the DUT.
     * - Pass Criteria: N/A
     */

    /** Configure AllowList for specific links. */
    for (uint16_t i = 2; i <= 15; i++)
    {
        leader.AllowList(*r[i]);
        r[i]->AllowList(leader);
    }

    r[1]->AllowList(*r[3]);
    r[3]->AllowList(*r[1]);

    /** DUT links. */
    dut.AllowList(*r[2]);
    r[2]->AllowList(dut);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (uint16_t i = 2; i <= 15; i++)
    {
        r[i]->Join(leader);
    }
    nexus.AdvanceTime(kAttachToRouterTime);

    r[1]->Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(r[1]->Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(r[2]->Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(r[3]->Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: REED_1 (DUT)");

    /**
     * Step 2: REED_1 (DUT)
     * - Description: The DUT is added to the topology. Harness filters are set to limit the DUT to attach to Router_2.
     * - Pass Criteria: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
     */
    SuccessOrQuit(dut.Get<Mle::Mle>().SetRouterEligible(false));
    dut.Join(leader);
    nexus.AdvanceTime(kAttachAsChildTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == r[2]->Get<Mac::Mac>().GetExtAddress());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_3");

    /**
     * Step 3: Router_3
     * - Description: Harness instructs the device to powerdown – removing it from the network.
     * - Pass Criteria: N/A
     */
    r[3]->Reset();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Automatically attempt to re-attach to the partition by sending multicast Parent Requests to the
     *   Routers and REEDs address.
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(dut.Get<Mle::Mle>().SetRouterEligible(true));
    dut.AllowList(*r[1]);
    r[1]->AllowList(dut);
    nexus.AdvanceTime(kReattachTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: REED_1 (DUT)");

    /**
     * Step 5: REED_1 (DUT)
     * - Description: Automatically sends MLE Parent Response to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent response to Router_1.
     *   - The MLE Parent Response to Router_1 MUST be properly formatted.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     * - Description: Automatically sends MLE Child ID Request to the DUT.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: REED_1 (DUT)");

    /**
     * Step 7: REED_1 (DUT)
     * - Description: Automatically sends an Address Solicit Request to Leader, receives a short address and becomes a
     *   router.
     * - Pass Criteria:
     *   - The Address Solicit Request MUST be properly formatted:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/as
     *     - CoAP Payload:
     *       - MAC Extended Address TLV
     *       - Status TLV
     *       - RLOC16 TLV (optional)
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: REED_1 (DUT)");

    /**
     * Step 8: REED_1 (DUT)
     * - Description: Automatically (optionally) sends multicast Link Request.
     * - Pass Criteria:
     *   - The DUT MAY send a multicast Link Request Message.
     *   - If sent, the following TLVs MUST be present in the Multicast Link Request Message:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - TLV Request TLV: Link Margin
     *     - Source Address TLV
     *     - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: REED_1 (DUT)");

    /**
     * Step 9: REED_1 (DUT)
     * - Description: Automatically sends Child ID Response to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Child ID Response to Router_1.
     *   - The Child ID Response MUST be properly formatted.
     */
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(r[1]->Get<Mle::Mle>().IsChild());
    VerifyOrQuit(r[1]->Get<Mle::Mle>().GetParent().GetExtAddress() == dut.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_1");

    /**
     * Step 10: Router_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST route the ICMPv6 Echo request to the Leader.
     *   - The DUT MUST route the ICMPv6 Echo reply back to Router_1.
     */
    nexus.SendAndVerifyEchoRequest(*r[1], leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kPingTimeout);

    nexus.SaveTestInfo("test_5_5_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_5();
    printf("All tests passed\n");
    return 0;
}
