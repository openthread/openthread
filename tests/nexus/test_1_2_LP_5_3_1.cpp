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
 * Time to advance for a node to join as a SSED.
 */
static constexpr uint32_t kAttachAsSsedTime = 20 * 1000;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols.
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

/**
 * Time to advance for CSL synchronization to complete, in milliseconds.
 */
static constexpr uint32_t kCslSyncTime = 5 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void Test1_2_LP_5_3_1(void)
{
    /**
     * 5.3.1 SSED Attachment
     *
     * 5.3.1.1 Topology
     * - Leader (DUT)
     * - Router_1
     * - SSED_1
     *
     * 5.3.1.2 Purpose & Description
     * The purpose of this test is to validate that the DUT can successfully support a SSED that establishes CSL
     *   Synchronization after attach using Child Update Request.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|--------------
     * SSED Attachment  | 4.6.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ssed1   = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, Router_1, SSED_1.
     * - Pass Criteria: N/A.
     */

    /** Use AllowList feature to restrict the topology. */
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

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SSED_1");

    /**
     * Step 2: SSED_1
     * - Description: Automatically attaches to the DUT. Automatically initiates a CSL connection via a MLE Child
     *   Update Request that contains the CSL IEs and the CSL Synchronized Timeout TLV.
     * - Pass Criteria: N/A.
     */

    ssed1.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically completes the CSL connection with SSED_1 by unicasting MLE Child Update
     *   Response.
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SSED_1.
     *   - The Frame Version of the packet MUST be: IEEE Std 802.15.4-2015 (value = 0b10).
     */

    nexus.AdvanceTime(kCslSyncTime);
    VerifyOrQuit(ssed1.Get<Mac::Mac>().IsCslEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address (via the DUT).
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo Request to SSED_1.
     *   - The Frame Version of the packet MUST be: IEEE Std 802.15.4-2015 (value = 0b10).
     *   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
     *   - Router_1 MUST receive an ICMPv6 Echo Response from SSED_1.
     */

    nexus.SendAndVerifyEchoRequest(router1, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    nexus.SaveTestInfo("test_1_2_LP_5_3_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_1();
    printf("All tests passed\n");
    return 0;
}
