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
 * Time to wait for MLE advertisements and link synchronization.
 */
static constexpr uint32_t kLinkSyncTime = 60 * 1000;

void Test5_1_12(void)
{
    /**
     * 5.1.12 New Router Neighbor Synchronization
     *
     * 5.1.12.1 Topology
     * Topology information is not explicitly detailed in the text, but the procedure involves Router_1 (DUT) and
     * Router_2.
     *
     * 5.1.12.2 Purpose & Description
     * The purpose of this test case is to validate that when the DUT sees a new router for the first time, it will
     * synchronize using the New Router Neighbor Synchronization procedure.
     *
     * Spec Reference                     | V1.1 Section | V1.3.0 Section
     * -----------------------------------|--------------|---------------
     * New Router Neighbor Synchronization| 4.7.7.2      | 4.7.1.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router2.SetName("ROUTER_2");
    dut.SetName("DUT");

    // Leader <-> Router_2
    nexus.AllowLinkBetween(leader, router2);

    // Leader <-> DUT
    nexus.AllowLinkBetween(leader, dut);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Ensure topology is formed correctly.");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Automatically transmits MLE advertisements.");

    /**
     * Step 2: Router_1 (DUT)
     * - Description: Automatically transmits MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All
     *     Nodes multicast address (FF02::1).
     *   - The following TLVs MUST be present in the Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */
    nexus.AdvanceTime(kLinkSyncTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Harness enables communication between Router_1 (DUT) and Router_2.");

    /**
     * Step 3: Test Harness
     * - Description: Harness enables communication between Router_1 (DUT) and Router_2.
     * - Pass Criteria: N/A
     */
    nexus.AllowLinkBetween(dut, router2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: The DUT and Router_2 automatically exchange unicast Link Request and unicast Link Accept messages.");

    /**
     * Step 4: Router_1 (DUT) OR Router_2
     * - Description: The DUT and Router_2 automatically exchange unicast Link Request and unicast Link Accept messages
     *   OR Link Accept and Request messages.
     * - Pass Criteria:
     *   - Link Request messages MUST be Unicast.
     *   - The following TLVs MUST be present in the Link Request messages:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - TLV Request TLV: Link Margin TLV
     *     - Source Address TLV
     *     - Version TLV
     *   - Link Accept or Link Accept and Request Messages MUST be Unicast.
     *   - The following TLVs MUST be present in the Link Accept or Link Accept And Request Messages:
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Link Margin TLV
     *     - Response TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - TLV Request TLV: Link Margin TLV (situational)*
     *     - Challenge TLV (situational)*
     *     - MLE Frame Counter TLV (optional)
     */
    nexus.AdvanceTime(kLinkSyncTime);

    nexus.SaveTestInfo("test_5_1_12.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_12();
    printf("All tests passed\n");
    return 0;
}
