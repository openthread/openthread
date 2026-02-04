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
static constexpr uint32_t kFormNetworkTime = 30 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Partition ID for Leader.
 */
static constexpr uint32_t kLeaderPartitionId = 1;

/**
 * Network ID Timeout for Router_2, in seconds.
 */
static constexpr uint8_t kRouter2NetworkIdTimeout = 200;

/**
 * Network ID Timeout for Router_1 (DUT), in seconds.
 */
static constexpr uint8_t kRouter1NetworkIdTimeout = 120;

/**
 * Time to wait for Router_1 (DUT) to time out and become leader, in milliseconds.
 * This accounts for the 120s timeout plus a small buffer for state transitions.
 */
static constexpr uint32_t kRouter1TimeoutWaitTime = (kRouter1NetworkIdTimeout + 20) * 1000;

void Test5_1_4(void)
{
    /**
     * 5.1.4 Router Address Reallocation – DUT creates new partition
     *
     * 5.1.4.1 Topology
     * - Set Router_2 NETWORK_ID_TIMEOUT to 200 seconds
     * - Set Partition ID on Leader to 1.
     *
     * 5.1.4.2 Purpose & Description
     * The purpose of this test case is to verify that when the original Leader is removed from the network, the DUT
     * will create a new partition as Leader and will assign a router ID if a specific ID is requested.
     *
     * Spec Reference                             | V1.1 Section    | V1.3.0 Section
     * -------------------------------------------|-----------------|-----------------
     * Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10  | 5.9.9 / 5.9.10
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    /**
     * Step 1: Router_2
     * - Description: Harness configures NETWORK ID TIMEOUT to be 200 seconds
     * - Pass Criteria: N/A
     */
    router2.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter2NetworkIdTimeout);

    /**
     * Step 2: Leader
     * - Description: Harness configures Partition ID to 1
     * - Pass Criteria: N/A
     */
    leader.Get<Mle::Mle>().SetPreferredLeaderPartitionId(kLeaderPartitionId);

    /**
     * Step 3: All
     * - Description: Verify topology is formed correctly
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    leader.AllowList(router2);
    router2.AllowList(leader);

    router1.AllowList(router2);
    router2.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    /**
     * Step 4: Leader
     * - Description: Harness silently powers-off the Leader
     * - Pass Criteria: N/A
     */
    leader.Get<Mle::Mle>().Stop();

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Times out after 120 seconds and automatically attempts to reattach to partition
     * - Pass Criteria:
     *   - The DUT MUST attempt to reattach to its original partition by sending a MLE Parent Request to the Link-Local
     *     All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (MUST have E and R flags set)
     *     - Version TLV
     *   - The DUT MUST make two separate attempts to reconnect to its current Partition in this manner.
     */
    nexus.AdvanceTime(kRouter1TimeoutWaitTime);

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically attempts to attach to any other partition
     * - Pass Criteria:
     *   - The DUT MUST attempt to attach to any other partition within range by sending a MLE Parent Request.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV
     *     - Version TLV
     */

    /**
     * Step 7: Router_1 (DUT)
     * - Description: Automatically creates a new partition with different Partition ID, initial VN_version, initial
     *   VN_stable_version, and initial ID sequence number
     * - Pass Criteria: N/A
     */
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());

    /**
     * Step 8: Router_2
     * - Description: Automatically starts attaching to the DUT-led partition by sending MLE Parent Request
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted MLE Parent Response to Router_2, including the following:
     *   - Leader Data TLV:
     *     - Partition ID different from original
     *     - Initial VN_version & VN_stable_version different from the original
     *     - Initial ID sequence number different from the original
     */

    /**
     * Step 9: Router_2
     * - Description: Automatically sends MLE Child ID Request
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted Child ID Response to Router_2 (See 5.1.1 Attaching for pass criteria)
     */

    /**
     * Step 10: Router_1 (DUT)
     * - Description: Automatically sends Address Solicit Response Message
     * - Pass Criteria:
     *   - The DUT MUST send a properly-formatted Address Solicit Response Message to Router_2.
     *   - If a specific router ID is requested, the DUT MUST provide this router ID:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Status TLV (value = 0 [Success])
     *     - RLOC16 TLV
     *     - Router Mask TLV
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    nexus.SaveTestInfo("test_5_1_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_4();
    printf("All tests passed\n");
    return 0;
}
