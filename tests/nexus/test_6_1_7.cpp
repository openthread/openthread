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
 * Time to advance for the DUT to attach to the leader, in milliseconds.
 */
static constexpr uint32_t kAttachToLeaderTime = 20 * 1000;

/**
 * Time to advance for the DUT to send Link Requests to neighboring routers.
 */
static constexpr uint32_t kLinkRequestTime = 60 * 1000;

void RunTest6_1_7(const char *aJsonFile)
{
    /**
     * 6.1.7 End Device Synchronization
     *
     * 6.1.7.1 Topology
     * - DUT as Full End Device (FED)
     * - Leader
     * - Router_1
     * - Router_2
     * - Router_3
     *
     * 6.1.7.2 Purpose & Description
     * The purpose of this test case is to validate End Device Synchronization on the DUT.
     *
     * Spec Reference                | V1.1 Section | V1.3.0 Section
     * ------------------------------|--------------|---------------
     * REED and FED Synchronization  | 4.7.7.4      | 4.7.1.4
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
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Setup the topology without the DUT. Ensure all routers and the Leader are sending MLE
     *   advertisements.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to specify links between nodes. */
    leader.AllowList(router1);
    leader.AllowList(router2);

    router1.AllowList(leader);
    router1.AllowList(router3);

    router2.AllowList(leader);
    router2.AllowList(router3);

    router3.AllowList(router1);
    router3.AllowList(router2);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    router3.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: FED (DUT)");

    /**
     * Step 2: FED (DUT)
     * - Description: Automatically attaches to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to the Leader.
     */
    dut.AllowList(leader);
    leader.AllowList(dut);

    dut.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(kAttachToLeaderTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: FED (DUT)");

    /**
     * Step 3: FED (DUT)
     * - Description: Automatically sends Link Requests to Router_1, Router_2 & Router_3.
     * - Pass Criteria:
     *   - The DUT MUST unicast Link Requests to each Router which contains the following TLVs:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - Source Address TLV
     *     - Version TLV
     */
    dut.AllowList(router1);
    dut.AllowList(router2);
    dut.AllowList(router3);

    router1.AllowList(dut);
    router2.AllowList(dut);
    router3.AllowList(dut);

    nexus.AdvanceTime(kLinkRequestTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1, Router_2, Router_3");

    /**
     * Step 4: Router_1, Router_2, Router_3
     * - Description: Automatically send Link Accept to the DUT.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kLinkRequestTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::RunTest6_1_7((argc > 2) ? argv[2] : "test_6_1_7.json");
    printf("All tests passed\n");
    return 0;
}
