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
 * Time to advance for the DUT to send Parent Request and receive Parent Responses.
 */
static constexpr uint32_t kParentSelectionTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * RSSI value to enable a link quality of 2 (medium).
 * Link margin > 10 dB gives link quality 2.
 * Noise floor is -100 dBm.
 */
static constexpr int8_t kRssiLinkQuality2 = -85;

void Test5_1_10(void)
{
    /**
     * 5.1.10 Attaching to a Router with better link quality
     *
     * 5.1.10.1 Topology
     * - Leader
     * - Router_1
     * - Router_2
     * - Router_3 (DUT)
     *
     * 5.1.10.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT will choose a router with better link quality as its
     *   parent.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Parent Selection | 4.7.2        | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader, Router_1, Router_2");

    /**
     * Step 1: Leader, Router_1, Router_2
     * - Description: Setup the topology without the DUT. Verify all are sending MLE Advertisements.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    leader.AllowList(router2);

    router1.AllowList(leader);
    router1.AllowList(router2);

    router2.AllowList(leader);
    router2.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Test Harness");

    /**
     * Step 2: Test Harness
     * - Description: Harness configures the RSSI between Router_2 & Router_3 (DUT) to enable a link quality of 2
     *   (medium).
     * - Pass Criteria: N/A
     */

    /** Restricted topology for DUT. */
    dut.AllowList(router1);
    dut.AllowList(router2);

    router1.AllowList(dut);
    router2.AllowList(dut);

    IgnoreError(dut.Get<Mac::Filter>().AddRssIn(router2.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));
    IgnoreError(router2.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_3 (DUT)");

    /**
     * Step 3: Router_3 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Mode TLV
     *     - Challenge TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV
     */

    dut.Get<Mle::Mle>().SetRouterEligible(false);
    dut.Join(leader);

    /**
     * Step 4: Router_1, Router_2
     * - Description: Each devices automatically responds to DUT with MLE Parent Response.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kParentSelectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_3 (DUT)");

    /**
     * Step 5: Router_3 (DUT)
     * - Description: Automatically sends MLE Child ID Request to Router_1 due to better link quality.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to Router_1, including the following TLVs:
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

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    nexus.SaveTestInfo("test_5_1_10.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_10();
    printf("All tests passed\n");
    return 0;
}
