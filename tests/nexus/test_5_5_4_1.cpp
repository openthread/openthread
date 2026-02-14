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
 * Time to advance for a node to join and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time for DUT reset.
 */
static constexpr uint32_t kResetTime = 300 * 1000;

/**
 * Time to wait for network merge.
 */
static constexpr uint32_t kMergeWaitTime = 300 * 1000;

/**
 * The identifier used for Echo Request.
 */
static constexpr uint16_t kEchoIdentifier = 0xabcd;

/**
 * The identifiers used for marking steps in the trace.
 */
static constexpr uint16_t kMarkStep3StartIdentifier = 0x5303;
static constexpr uint16_t kMarkStep3EndIdentifier   = 0x5304;

void Test5_5_4_1(void)
{
    /**
     * 5.5.4 Split and Merge with Routers
     *
     * 5.5.4.1 Topology A (DUT as Leader)
     * - The topology consists of a Leader (DUT) connected to Router_1 and Router_2. Router_1 is connected to Router_3.
     *   Router_2 is connected to Router_4.
     *
     * Purpose & Description
     * The purpose of this test case is to show that the Leader will merge two separate network partitions and allow
     *   communication across a single unified network.
     *
     * Spec Reference            | V1.1 Section | V1.3.0 Section
     * --------------------------|--------------|---------------
     * Thread Network Partitions | 5.16         | 5.16
     */

    Core nexus;

    Node &dut     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &router4 = nexus.CreateNode();

    dut.SetName("DUT");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");
    router4.SetName("ROUTER_4");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    dut.AllowList(router1);
    dut.AllowList(router2);

    router1.AllowList(dut);
    router1.AllowList(router3);

    router2.AllowList(dut);
    router2.AllowList(router4);

    router3.AllowList(router1);

    router4.AllowList(router2);

    dut.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());

    router1.Join(dut);
    router2.Join(dut);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    router3.Join(router1);
    router4.Join(router2);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router4.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader (DUT)");

    /**
     * Step 2: Leader (DUT)
     * - Description: Automatically transmits MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All Nodes
     *     multicast address (FF02::1).
     *   - The following TLVs MUST be present in the Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Reset the DUT for 300 seconds (longer than NETWORK_ID_TIMEOUT default value of 120 seconds).
     * - Pass Criteria: The DUT MUST stop sending MLE advertisements,.
     */

    dut.Get<Mle::Mle>().Stop();

    // Mark the start of the reset period.
    router1.SendEchoRequest(router3.Get<Mle::Mle>().GetMeshLocalEid(), kMarkStep3StartIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Routers");

    /**
     * Step 4: Routers
     * - Description: Automatically create two partitions after DUT is removed and NETWORK_ID_TIMEOUT expires.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kResetTime);

    // Mark the end of the reset period.
    router1.SendEchoRequest(router3.Get<Mle::Mle>().GetMeshLocalEid(), kMarkStep3EndIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Harness");

    /**
     * Step 5: Harness
     * - Description: Wait for 200 seconds (After 200 seconds the DUT will be done resetting, and the network will
     *   have merged into a single partition).
     * - Pass Criteria: N/A.
     */

    SuccessOrQuit(dut.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kMergeWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_3");

    /**
     * Step 6: Router_3
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_4.
     * - Pass Criteria: Router_4 MUST send an ICMPv6 Echo Reply to Router_3.
     */

    router3.SendEchoRequest(router4.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_5_4_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_4_1();
    printf("All tests passed\n");
    return 0;
}
