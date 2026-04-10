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
 * Time to advance for a node to join as a SSED, in milliseconds.
 */
static constexpr uint32_t kAttachAsSsedTime = 20 * 1000;

/**
 * Time to advance for a node to transition between modes, in milliseconds.
 */
static constexpr uint32_t kTransitionTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMPv6 Echo Request identifier for initial connectivity check.
 */
static constexpr uint16_t kEchoIdInitial = 1;

/**
 * ICMPv6 Echo Request identifier for connectivity check in MED mode.
 */
static constexpr uint16_t kEchoIdAsMed = 2;

/**
 * ICMPv6 Echo Request identifier for connectivity check after returning to SSED mode.
 */
static constexpr uint16_t kEchoIdAsSsed = 3;

/**
 * CSL period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 500;

/**
 * CSL timeout in seconds.
 */
static constexpr uint32_t kCslTimeoutSec = 20;

void Test1_2_LP_5_3_6(void)
{
    /**
     * 5.3.6 Router supports SSED transition to MED
     *
     * 5.3.6.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - SSED_1
     *
     * 5.3.6.2 Purpose & Description
     * The purpose of this test case is to validate that a Router can support a child transitioning from a SSED to a
     *   MED and back.
     *
     * Spec Reference                          | V1.2 Section
     * ----------------------------------------|-------------
     * Router supports SSED transition to MED  | 4.6.5.2
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

    /**
     * Step 0: SSED_1
     * - Description: Preconditions: Set CSL Synchronized Timeout = 20s, Set CSL Period = 500ms.
     * - Pass Criteria: N/A
     */
    Log("Step 0: SSED_1");

    /**
     * Step 1: All
     * - Description: Topology formation: Leader, DUT, SSED_1.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    /** Use AllowList feature to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);
    router1.AllowList(ssed1);
    ssed1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    /**
     * Step 2: SSED_1
     * - Description: Automatically attaches to the DUT and establishes CSL synchronization.
     * - Pass Criteria:
     *   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
     *   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */
    Log("Step 2: SSED_1");

    ssed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(ssed1.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeoutSec);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED mesh-local address.
     * - Pass Criteria:
     *   - 3.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */
    Log("Step 3: Leader");

    /**
     * Step 4: SSED_1
     * - Description: Automatically replies with ICMPv6 Echo Reply.
     * - Pass Criteria: Verify that the SSED replies with an ICMPv6 Echo Reply and is received by the Router.
     */
    Log("Step 4: SSED_1");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdInitial, kEchoPayloadSize,
                           Ip6::kDefaultHopLimit);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5: SSED_1 (MED_1)
     * - Description: Harness instructs the device to transition to a MED and send an MLE Child Update Request and
     *   properly sets the Mode TLV.
     * - Pass Criteria: N/A
     */
    Log("Step 5: SSED_1 (MED_1)");

    {
        Mle::DeviceMode mode;
        mode.Set(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);
        SuccessOrQuit(ssed1.Get<Mle::Mle>().SetDeviceMode(mode));
    }

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically responds with a MLE Child Update Response.
     * - Pass Criteria:
     *   - 6.1: The DUT MUST unicast MLE Child Update Response to SSED_1, including the following:
     *     - Mode TLV
     *     - R bit = 1
     *     - D bit = 0
     */
    Log("Step 6: Router_1 (DUT)");

    nexus.AdvanceTime(kTransitionTime);

    /**
     * Step 7: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to
     *   SSED_1’s mesh-local address.
     * - Pass Criteria:
     *   - 7.1: The DUT MUST forward the ICMPv6 Echo Reply from the SSED to the Leader.
     */
    Log("Step 7: Leader");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdAsMed, kEchoPayloadSize,
                           Ip6::kDefaultHopLimit);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 8: MED_1 (SSED_1)
     * - Description: Harness instructs the device to transition back to a SSED and send an MLE Child Update
     *   Request.
     * - Pass Criteria: N/A
     */
    Log("Step 8: MED_1 (SSED_1)");

    {
        Mle::DeviceMode mode;
        mode.Set(Mle::DeviceMode::kModeFullNetworkData);
        SuccessOrQuit(ssed1.Get<Mle::Mle>().SetDeviceMode(mode));
    }

    /**
     * Step 9: Router_1 (DUT)
     * - Description: Automatically responds with a MLE Child Update Response.
     * - Pass Criteria:
     *   - 9.1: The DUT MUST unicast MLE Child Update Response to SSED_1, including the following:
     *     - Mode TLV
     *     - R bit set = 0
     *     - D bit set = 0
     */
    Log("Step 9: Router_1 (DUT)");

    nexus.AdvanceTime(kTransitionTime);

    /**
     * Step 10: SSED_1
     * - Description: Automatically begins sending synchronization messages containing CSL IEs.
     * - Pass Criteria: N/A
     */
    Log("Step 10: SSED_1");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 11: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - 11.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */
    Log("Step 11: Leader");

    /**
     * Step 12: SSED_1
     * - Description: Automatically replies with ICMPv6 Echo Reply.
     * - Pass Criteria:
     *   - 12.1: SSED_1 MUST send an ICMPv6 Echo Reply.
     */
    Log("Step 12: SSED_1");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdAsSsed, kEchoPayloadSize,
                           Ip6::kDefaultHopLimit);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_1_2_LP_5_3_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_6();
    printf("All tests passed\n");
    return 0;
}
