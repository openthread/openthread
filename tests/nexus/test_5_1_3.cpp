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
 * Maximum value for Partition ID.
 */
static constexpr uint32_t kMaxPartitionId = 0xffffffff;

/**
 * Network ID Timeout for Router_2, in seconds.
 */
static constexpr uint8_t kRouter2NetworkIdTimeout = 100;

/**
 * Network ID Timeout for Router_1 (DUT), in seconds.
 */
static constexpr uint8_t kRouter1NetworkIdTimeout = 120;

/**
 * Time to wait for Router_2 to time out and become leader, in milliseconds.
 */
static constexpr uint32_t kRouter2TimeoutWaitTime = (kRouter2NetworkIdTimeout + 20) * 1000;

/**
 * Total time to wait for Router_1 (DUT) to time out and reattach, in milliseconds.
 * This accounts for the 120s timeout plus a small buffer for state transitions.
 */
static constexpr uint32_t kRouter1TotalTimeoutWaitTime = (kRouter1NetworkIdTimeout + 20) * 1000;

/**
 * Incremental time to advance for Router_1 (DUT) reattachment, in milliseconds.
 */
static constexpr uint32_t kRouter1TimeoutWaitTime = kRouter1TotalTimeoutWaitTime - kRouter2TimeoutWaitTime;

void Test5_1_3(void)
{
    /**
     * 5.1.3 Router Address Reallocation – DUT attaches to new partition
     *
     * 5.1.3.1 Topology
     * - Set Partition ID on Leader to max value
     * - Set Router_2 NETWORK_ID_TIMEOUT to 110 seconds (10 seconds faster than default). If the DUT uses a timeout
     *   faster than default, timing may need to be adjusted.
     *
     * 5.1.3.2 Purpose & Description
     * The purpose of this test case is to verify that after the removal of the Leader from the network, the DUT will
     * first attempt to reattach to the original partition (P1), and then attach to a new partition (P2).
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
     * - Description: Harness configures the NETWORK ID TIMEOUT to be 110 seconds.
     * - Pass Criteria: N/A
     */
    router2.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter2NetworkIdTimeout);

    /**
     * Step 2: All
     * - Description: Verify topology is formed correctly
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, router2);
    nexus.AllowLinkBetween(router1, router2);

    leader.Get<Mle::Mle>().SetPreferredLeaderPartitionId(kMaxPartitionId);
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
     * Step 3: Leader
     * - Description: Harness silently powers-off the Leader
     * - Pass Criteria: N/A
     */
    leader.Get<Mle::Mle>().Stop();

    /**
     * Step 4: Router_2
     * - Description: Times out after 110 seconds and automatically creates a new partition (P2) with itself as the
     *   Leader of P2
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kRouter2TimeoutWaitTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Times out after 120 seconds and automatically attempts to reattach to original partition (P1)
     * - Pass Criteria:
     *   - The DUT MUST attempt to reattach to its original partition (P1) by sending a MLE Parent Request with an IP
     *     Hop Limit of 255 to the Link-Local All-Routers multicast address (FF02::2).
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (MUST have E and R flags set)
     *     - Version TLV
     *   - The DUT MUST make two separate attempts to reconnect to its original partition (P1) in this manner.
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
     * - Description: Automatically attaches to Router_2’s partition (P2)
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted Child ID Request to Router_2.
     *   - The following TLVs MUST be present in the Child ID Request:
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

    /**
     * Step 8: Router_1 (DUT)
     * - Description: Automatically sends Address Solicit Request
     * - Pass Criteria:
     *   - The DUT MUST send an Address Solicit Request.
     *   - Ensure the Address Solicit Request is properly formatted:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/as
     *     - CoAP Payload:
     *       - MAC Extended Address TLV
     *       - Status TLV
     *       - RLOC16 TLV (optional)
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    /**
     * Step 9: Router_2
     * - Description: Automatically responds with Address Solicit Response Message
     * - Pass Criteria: N/A
     */

    nexus.SaveTestInfo("test_5_1_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_3();
    printf("All tests passed\n");
    return 0;
}
