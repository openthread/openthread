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

void Test5_1_6(void)
{
    /**
     * 5.1.6 Leader removes Router ID
     *
     * 5.1.6.1 Topology
     * - Leader
     * - Router_1 (DUT)
     *
     * 5.1.6.2 Purpose & Description
     * The purpose of this test case is to verify that when the Leader de-allocates a Router ID, the DUT, as a
     * router, re-attaches.
     *
     * Spec Reference                               | V1.1 Section | V1.3.0 Section
     * ---------------------------------------------|--------------|---------------
     * Router ID Management / Router ID Assignment  | 5.16.1       | 5.16.1
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    // Use AllowList feature to restrict the topology
    leader.AllowList(router);
    router.AllowList(leader);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Verify topology is formed correctly");

    /**
     * Step 0: All
     * - Description: Verify topology is formed correctly
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Harness instructs the Leader to send a ' helper ping' (ICMPv6 Echo Request) to the DUT");

    /**
     * Step 1: Leader
     * - Description: Harness instructs the Leader to send a ' helper ping' (ICMPv6 Echo Request) to the DUT
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetLinkLocalAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Harness instructs the Leader to remove the Router ID of Router_1 (the DUT)");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the Leader to remove the Router ID of Router_1 (the DUT)
     * - Pass Criteria: N/A
     */
    uint8_t routerId = Mle::RouterIdFromRloc16(router.Get<Mle::Mle>().GetRloc16());
    SuccessOrQuit(leader.Get<RouterTable>().Release(routerId));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Automatically re-attaches once it recognizes its Router ID has been removed.");

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically re-attaches once it recognizes its Router ID has been removed.
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted MLE Parent Request, MLE Child ID Request, and Address Solicit
     *     Request messages to the Leader. (See 5.1.1 for formatting)
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Harness verifies connectivity by instructing the Leader to send an ICMPv6 Echo Request to the DUT");

    /**
     * Step 4: Leader
     * - Description: Harness verifies connectivity by instructing the Leader to send an ICMPv6 Echo Request to
     *   the DUT
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetLinkLocalAddress());

    nexus.SaveTestInfo("test_5_1_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_6();
    printf("All tests passed\n");
    return 0;
}
