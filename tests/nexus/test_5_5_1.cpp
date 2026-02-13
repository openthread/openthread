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
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Leader reboot time in milliseconds.
 * Must be less than Leader Timeout value (default 120 seconds).
 */
static constexpr uint32_t kLeaderRebootTime = 80 * 1000;

/**
 * Time to advance after Leader reset to allow it to synchronize.
 */
static constexpr uint32_t kSynchronizationTime = 10 * 1000;

void Test5_5_1(void)
{
    /**
     * 5.5.1 Leader Reboot < timeout
     *
     * 5.5.1.1 Topology
     * - Leader
     * - Router_1
     *
     * 5.5.1.2 Purpose & Description
     * The purpose of this test case is to show that when the Leader is rebooted for a time period shorter than the
     *   leader timeout, it does not trigger network partitioning and remains the leader when it reattaches to the
     *   network.
     *
     * Spec Reference      | V1.1 Section | V1.3.0 Section
     * --------------------|--------------|---------------
     * Losing Connectivity | 5.16.1       | 5.16.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader, Router_1");

    /**
     * Step 2: Leader, Router_1
     * - Description: Transmit MLE advertisements.
     * - Pass Criteria:
     *   - The devices MUST send properly formatted MLE Advertisements.
     *   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in MLE Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     *   - Non-DUT device: Harness instructs device to send a ICMPv6 Echo Request to the DUT to help differentiate
     *     between Link Requests sent before and after reset.
     */
    nexus.AdvanceTime(kStabilizationTime);
    nexus.SendAndVerifyEchoRequest(router1, leader.Get<Mle::Mle>().GetLinkLocalAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Reset Leader.
     *   - If DUT=Leader and testing is manual, this is a UI pop-up box interaction.
     *   - Allowed Leader reboot time is 125 seconds (must be greater than Leader Timeout value [default 120 seconds]).
     * - Pass Criteria:
     *   - For DUT = Leader: The Leader MUST stop sending MLE advertisements.
     *   - The Leader reboot time MUST be less than Leader Timeout value (default 120 seconds).
     */
    leader.Reset();
    nexus.AdvanceTime(kLeaderRebootTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically performs Synchronization after Reset, sends Link Request.
     * - Pass Criteria:
     *   - For DUT = Leader: The Leader MUST send a multicast Link Request.
     *   - The following TLVs MUST be present in the Link Request:
     *     - Challenge TLV
     *     - TLV Request TLV: Address16 TLV, Route64 TLV
     *     - Version TLV
     */
    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kSynchronizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically responds with a Link Accept.
     * - Pass Criteria:
     *   - For DUT = Router: Router_1 MUST reply with a Link Accept.
     *   - The following TLVs MUST be present in the Link Accept:
     *     - Address16 TLV
     *     - Leader Data TLV
     *     - Link-Layer Frame Counter TLV
     *     - Response TLV
     *     - Route64 TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *     - Challenge TLV (situational - MUST be included if the response is an Accept and Request message)
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Does NOT send a Parent Request.
     * - Pass Criteria:
     *   - For DUT = Leader: The Leader MUST NOT send a Parent Request after it is re-enabled.
     */
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: All");

    /**
     * Step 7: All
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the Router_1 link local
     *   address.
     * - Pass Criteria:
     *   - For DUT = Router: Router_1 MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, router1.Get<Mle::Mle>().GetLinkLocalAddress());

    nexus.SaveTestInfo("test_5_5_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_1();
    printf("All tests passed\n");
    return 0;
}
