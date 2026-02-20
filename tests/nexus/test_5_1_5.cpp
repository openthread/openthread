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
 * Wait time in Step 2, in milliseconds.
 * (200 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + extra time)
 */
static constexpr uint32_t kStep2WaitTime = 200 * 1000;

/**
 * Wait time in Step 5, in milliseconds.
 * (300 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + extra time)
 */
static constexpr uint32_t kStep5WaitTime = 300 * 1000;

void Test5_1_5(void)
{
    /**
     * 5.1.5 Router Address Timeout
     *
     * 5.1.5.1 Topology
     * - Leader (DUT)
     * - Router_1
     *
     * 5.1.5.2 Purpose & Description
     * The purpose of this test case is to verify that after deallocating a Router ID, the Leader (DUT) does not
     * reassign the Router ID for at least ID_REUSE_DELAY seconds.
     *
     * Spec Reference                              | V1.1 Section   | V1.3.0 Section
     * --------------------------------------------|----------------|---------------
     * Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10 | 5.9.9 / 5.9.10
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Verify topology is formed correctly
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    nexus.AllowLinkBetween(leader, router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Get<Mle::Mle>().SetRouterSelectionJitter(1);
    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    uint8_t firstRouterId = Mle::RouterIdFromRloc16(router1.Get<Mle::Mle>().GetRloc16());
    Log("Router_1 joined with Router ID: %u", firstRouterId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness silently powers-off Router_1 for 200 seconds.
     *   - (200 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + extra time)
     *   - Extra time is added so Router_1 is brought back within ID_REUSE_DELAY interval
     * - Pass Criteria: N/A
     */
    router1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kStep2WaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     * - Description: Harness silently powers-on Router_1 after 200 seconds.
     *   - Router_1 automatically sends a link request, re-attaches and requests its original Router ID.
     * - Pass Criteria: N/A
     */
    IgnoreError(router1.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically attaches Router_1 (Parent Response, Child ID Response, Address Solicit Response)
     * - Pass Criteria:
     *   - The RLOC16 TLV in the Address Solicit Response message MUST contain a different Router ID than the one
     *     allocated in the original attach because ID_REUSE_DELAY interval has not timed out.
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Status TLV (value = Success)
     *     - RLOC16 TLV
     *     - Router Mask TLV
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    uint8_t secondRouterId = Mle::RouterIdFromRloc16(router1.Get<Mle::Mle>().GetRloc16());
    Log("Router_1 re-joined with Router ID: %u", secondRouterId);
    VerifyOrQuit(secondRouterId != firstRouterId, "Router ID was reused too early");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Harness silently powers-off Router_1 for 300 seconds.
     *   - (300 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + extra time)
     *   - Extra time is added to bring Router_1 back after ID_REUSE_DELAY interval
     * - Pass Criteria: N/A
     */
    router1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kStep5WaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     * - Description: Harness silently powers-on Router_1 after 300 seconds.
     *   - Router_1 reattaches and requests its most recent Router ID.
     * - Pass Criteria: N/A
     */
    IgnoreError(router1.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically attaches Router_1 (Parent Response, Child ID Response, Address Solicit Response)
     * - Pass Criteria:
     *   - The RLOC16 TLV in the Address Solicit Response message MUST contain the requested Router ID
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Status TLV (value = Success)
     *     - RLOC16 TLV
     *     - Router Mask TLV
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    uint8_t thirdRouterId = Mle::RouterIdFromRloc16(router1.Get<Mle::Mle>().GetRloc16());
    Log("Router_1 re-joined after ID_REUSE_DELAY with Router ID: %u", thirdRouterId);
    VerifyOrQuit(thirdRouterId == secondRouterId, "Router ID was not reused after ID_REUSE_DELAY");

    nexus.SaveTestInfo("test_5_1_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_5();
    printf("All tests passed\n");
    return 0;
}
