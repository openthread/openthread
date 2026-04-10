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
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * CSL synchronized timeout in seconds.
 */
static constexpr uint32_t kCslTimeout20s = 20;

/**
 * Wait times as specified in the test procedure.
 */
static constexpr uint32_t kWaitTime15s = 15 * 1000;
static constexpr uint32_t kWaitTime18s = 18 * 1000;

/**
 * Identifiers for ICMPv6 Echo Requests.
 */
static constexpr uint16_t kEchoRequestIdentifier1 = 0x1234;
static constexpr uint16_t kEchoRequestIdentifier2 = 0x5678;
static constexpr uint16_t kEchoRequestIdentifier3 = 0x90ab;

void Test1_2_LP_5_3_4(void)
{
    /**
     * 5.3.4 SSED modifies CSL IE
     *
     * 5.3.4.1 Topology
     * - Leader (DUT)
     * - Router_1
     * - SSED_1
     *
     * 5.3.4.2 Purpose & Description
     * The purpose of this test is to validate that a Router is able to successfully maintain a robust CSL connection
     *   with a SSED even when the SSED modifies the CSL IEs during its synchronization.
     *
     * Spec Reference       | V1.2 Section
     * ---------------------|-------------
     * SSED modifies CSL IE | 4.10
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ssed1   = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    ssed1.SetName("SSED_1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: SSED_1");

    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, Router_1, SSED_1.
     * - Pass Criteria: N/A.
     */

    leader.AllowList(router1);
    leader.AllowList(ssed1);

    router1.AllowList(leader);

    ssed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("Step 2: SSED_1");

    /**
     * Step 2: SSED_1
     * - Description: Automatically attaches to the DUT and establishes CSL synchronization.
     * - Pass Criteria:
     *   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
     *   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */

    ssed1.Join(leader, Node::kAsSed);
    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod500ms);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout20s);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - 3.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
     *   - 3.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoRequestIdentifier1);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 4: SSED_1");

    /**
     * Step 4: SSED_1
     * - Description: Automatically sends ICMPv6 Echo Reply to the Router.
     * - Pass Criteria:
     *   - 4.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 5: SSED_1");

    /**
     * Step 5: SSED_1
     * - Description: Harness instructs the device to change its CSL Period IE to 3300ms.
     * - Pass Criteria: N/A.
     */

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod3300ms);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 6: Harness");

    /**
     * Step 6: Harness
     * - Description: Harness waits for 15 seconds.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kWaitTime15s);

    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - 7.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
     *   - 7.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoRequestIdentifier2);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 8: SSED_1");

    /**
     * Step 8: SSED_1
     * - Description: Automatically sends ICMPv6 Echo Reply to the Router.
     * - Pass Criteria:
     *   - 8.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 9: SSED_1");

    /**
     * Step 9: SSED_1
     * - Description: Harness instructs the device to change its CSL Period IE to 400ms.
     * - Pass Criteria: N/A.
     */

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod400ms);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 10: Harness");

    /**
     * Step 10: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kWaitTime18s);

    Log("Step 11: Router_1");

    /**
     * Step 11: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to SSED_1
     *   mesh-local address.
     * - Pass Criteria:
     *   - 11.1: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
     *   - 11.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoRequestIdentifier3);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 12: SSED_1");

    /**
     * Step 12: SSED_1
     * - Description: Automatically sends ICMPv6 Echo Reply to the Router.
     * - Pass Criteria:
     *   - 12.1: SSED_1 MUST send an ICMPv6 Echo Reply to Router_1.
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_1_2_LP_5_3_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_4();
    printf("All tests passed\n");
    return 0;
}
