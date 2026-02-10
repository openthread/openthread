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
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time for nodes to send MLE Advertisements.
 */
static constexpr uint32_t kAdvTime = 32 * 1000;

/**
 * Time to wait for router synchronization.
 */
static constexpr uint32_t kResetSyncTime = 10 * 1000;

void Test5_1_13(void)
{
    /**
     * 5.1.13 Router Synchronization after Reset
     *
     * 5.1.13.1 Topology
     * - Topology A
     * - Topology B
     *
     * 5.1.13.2 Purpose & Description
     * The purpose of this test case is to validate that when a router resets, it will synchronize upon returning by
     *   using the Router Synchronization after Reset procedure.
     *
     * Spec Reference                     | V1.1 Section | V1.3.0 Section
     * -----------------------------------|--------------|---------------
     * Router Synchronization after Reset | 4.7.7.3      | 4.7.1.3
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER_1");

    nexus.AdvanceTime(0);

    // Use AllowList feature to restrict the topology.
    leader.AllowList(router);
    router.AllowList(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Verify topology is formed correctly.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1 / Leader");

    /**
     * Step 2: Router_1 / Leader
     * - Description: Automatically transmit MLE advertisements.
     * - Pass Criteria:
     *   - Devices MUST send properly formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All
     *     Nodes multicast address (FF02::1).
     *   - The following TLVs MUST be present in the Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */
    nexus.AdvanceTime(kAdvTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     * - Description: Harness silently resets the device.
     * - Pass Criteria: N/A
     */
    router.Reset();
    router.AllowList(leader);
    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Automatically sends multicast Link Request message.
     * - Pass Criteria:
     *   - For DUT = Router: The Link Request message MUST be sent to the Link-Local All Routers multicast address
     *     (FF02::2).
     *   - The following TLVs MUST be present in the Link Request message:
     *     - Challenge TLV
     *     - TLV Request TLV
     *       - Address16 TLV
     *       - Route64 TLV
     *     - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Automatically replies to Router_1 with Link Accept message.
     * - Pass Criteria:
     *   - For DUT = Leader: The following TLVs MUST be present in the Link Accept Message:
     *     - Address16 TLV
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Response TLV
     *     - Route64 TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - Challenge TLV (situational - MUST be included if the response is an Accept and Request message)
     *     - MLE Frame Counter TLV (optional)
     *   - Responses to multicast Link Requests MUST be delayed by a random time of up to MLE_MAX_RESPONSE_DELAY (1
     *     second).
     */
    nexus.AdvanceTime(kResetSyncTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    nexus.SaveTestInfo("test_5_1_13.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_13();
    printf("All tests passed\n");
    return 0;
}
