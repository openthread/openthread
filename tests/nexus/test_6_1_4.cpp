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
#include <string.h>

#include "mac/data_poll_sender.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachToChildTime = 10 * 1000;

/**
 * Time to advance for a node to upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the DUT to attach to its parent, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 20 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

/**
 * Threshold to keep nodes as REEDs.
 */
static constexpr uint8_t kReedThreshold = 1;

/**
 * Default threshold for router upgrade.
 */
static constexpr uint8_t kDefaultThreshold = 16;

void Test6_1_4(void)
{
    /**
     * 6.1.4 Attaching to a REED with Better Connectivity
     *
     * 6.1.4.1 Topology
     * - Ensure link quality between all nodes is set to 3.
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     * - Router_1
     * - Router_2
     * - REED_1
     * - REED_2
     *
     * 6.1.4.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT will pick REED_1 as its parent because of its better
     *   connectivity.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * Attaching to a Parent | 4.7.1        | 4.5.1
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
    dut.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE advertisements.
     * - Pass Criteria: N/A
     */

    /**
     * Use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - Leader and Router 1
     * - Leader and Router 2
     * - Leader and Router 3
     * - Router 1 and Router 3
     * - Router 2 and SED 1 (DUT)
     * - Router 3 and SED 1 (DUT)
     */
    leader.AllowList(router1);
    leader.AllowList(router2);
    leader.AllowList(router3);

    router1.AllowList(leader);
    router1.AllowList(router3);

    router2.AllowList(leader);
    router2.AllowList(dut);

    router3.AllowList(leader);
    router3.AllowList(router1);
    router3.AllowList(dut);

    dut.AllowList(router2);
    dut.AllowList(router3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    /**
     * Join Router 2 and Router 3 as REEDs (children) and prevent them from upgrading to routers yet.
     */
    router2.Get<Mle::Mle>().SetRouterUpgradeThreshold(kReedThreshold);
    router3.Get<Mle::Mle>().SetRouterUpgradeThreshold(kReedThreshold);

    router2.Join(leader);
    router3.Join(leader);
    nexus.AdvanceTime(kAttachToChildTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ED_1 / SED_1 (DUT)");

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV
     */
    dut.Join(leader, Node::kAsSed);
    SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: REED_1, REED_2");

    /**
     * Step 3: REED_1, REED_2
     * - Description: Do not respond to Parent Request.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED_1 / SED_1 (DUT)");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Parent Request with Scan Mask set to Routers AND REEDs.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (Value = 0xC0 [Routers and REEDs])
     *     - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: REED_1, REED_2");

    /**
     * Step 5: REED_1, REED_2
     * - Description: Automatically respond with MLE Parent Response. REED_1 has one more link quality connection than
     *   REED_2 in Connectivity TLV.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: ED_1 / SED_1 (DUT)");

    /**
     * Step 6: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Child ID Request to REED_1.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to REED_1.
     *   - The following TLVs MUST be present in the Child ID Request:
     *     - Address Registration TLV
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     */

    /**
     * Enable REED_1 (Router 3) to upgrade to router.
     */
    router3.Get<Mle::Mle>().SetRouterUpgradeThreshold(kDefaultThreshold);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: REED_1");

    /**
     * Step 7: REED_1
     * - Description: Automatically sends an Address Solicit Request to Leader with TOO_FEW_ROUTERS upgrade request.
     *   Leader automatically sends an Address Solicit Response and REED_1 becomes active router. REED_1 automatically
     *   sends MLE Child ID Response with DUT’s new 16-bit Address.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kAttachTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router3.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: REED_1");

    /**
     * Step 8: REED_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
     *   link local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(router3, dut.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_6_1_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test6_1_4();
    printf("All tests passed\n");
    return 0;
}
