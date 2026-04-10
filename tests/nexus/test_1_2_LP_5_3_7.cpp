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
 * Time to advance to wait for 2 seconds.
 */
static constexpr uint32_t kWait2s = 2 * 1000;

/**
 * Time to advance to wait for 10 seconds.
 */
static constexpr uint32_t kWait10s = 10 * 1000;

/**
 * Time to advance to wait for 15 seconds.
 */
static constexpr uint32_t kWait15s = 15 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Echo identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * CSL period in units of 160us. 500ms = 3125 * 160us.
 */
static constexpr uint16_t kCslPeriod = 3125;

/**
 * CSL synchronized timeout in seconds.
 */
static constexpr uint32_t kCslTimeout10s = 10;
static constexpr uint32_t kCslTimeout20s = 20;

void Test1_2_LP_5_3_7(void)
{
    /**
     * 5.3.7 Router supports SSED modifying CSL Synchronized Timeout TLV
     *
     * 5.3.7.1 Topology
     *   - Leader
     *   - Router_1 (DUT)
     *   - SSED_1
     *
     * 5.3.7.2 Purpose & Description
     *   The purpose of this test case is to validate that a Router can support a SSED that modifies the CSL
     *   Synchronized Timeout TLV.
     *
     * Spec Reference  | V1.2 Section
     * ----------------|-------------
     * SSED Attachment | 4.6.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ssed1   = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: SSED_1: Preconditions: Set CSL Synchronized Timeout = 10s. Set CSL Period = 500ms.");

    /**
     * Step 0: SSED_1
     *   Description: Preconditions: Set CSL Synchronized Timeout = 10s. Set CSL Period = 500ms.
     *   Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All: Topology formation: Leader, DUT, SSED_1.");

    /**
     * Step 1: All
     *   Description: Topology formation: Leader, DUT, SSED_1.
     *   Pass Criteria: N/A.
     */
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

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SSED_1: Automatically attaches to the DUT and establishes CSL synchronization.");

    /**
     * Step 2: SSED_1
     *   Description: Automatically attaches to the DUT and establishes CSL synchronization.
     *   Pass Criteria:
     *   - 2.1: The DUT MUST unicast MLE Child ID Response to SSED_1.
     *   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */
    ssed1.Join(router1, Node::kAsSed);
    // Set CSL parameters again to override defaults set by Join()
    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout10s);
    ssed1.Get<DataPollSender>().StopPolling();
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Harness: Harness waits for 2 seconds.");

    /**
     * Step 3: Harness
     *   Description: Harness waits for 2 seconds.
     *   Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kWait2s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.");

    /**
     * Step 4: Leader
     *   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *     SSED_1 mesh-local address.
     *   Pass Criteria:
     *   - 4.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     *   - Verify that the Leader receives the ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: SSED_1: Modify its CSL Synchronized Timeout TLV to 20s and send an MLE Child Update Request.");

    /**
     * Step 5: SSED_1
     *   Description: Harness instructs the device to modify its CSL Synchronized Timeout TLV to 20s and send an MLE
     *     Child Update Request.
     *   Pass Criteria: N/A.
     */
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout20s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1 (DUT): Automatically responds with MLE Child Update Response.");

    /**
     * Step 6: Router_1 (DUT)
     *   Description: Automatically responds with MLE Child Update Response.
     *   Pass Criteria:
     *   - 6.1: The DUT MUST unicast MLE Child Update Response to SSED_1.
     *   - Note: Following this response, Harness instructs SSED_1 to stop sending periodic synchronization data poll
     *     frames.
     */
    nexus.AdvanceTime(kWait2s);
    ssed1.Get<DataPollSender>().StopPolling();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Harness: Harness waits for 15 seconds.");

    /**
     * Step 7: Harness
     *   Description: Harness waits for 15 seconds.
     *   Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kWait15s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.");

    /**
     * Step 8: Leader
     *   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *     SSED_1 mesh-local address.
     *   Pass Criteria:
     *   - 8.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     *   - Verify that the Leader receives the ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: SSED_1: Modify its CSL Synchronized Timeout TLV to 10s and send a MLE Child Update Request.");

    /**
     * Step 9: SSED_1
     *   Description: Harness instructs the device to modify its CSL Synchronized Timeout TLV to 10s and send a MLE
     *     Child Update Request.
     *   Pass Criteria: N/A.
     */
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout10s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_1 (DUT): Automatically responds with MLE Child Update Response.");

    /**
     * Step 10: Router_1 (DUT)
     *   Description: Automatically responds with MLE Child Update Response.
     *   Pass Criteria:
     *   - 10.1: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */
    nexus.AdvanceTime(kWait2s);
    ssed1.Get<DataPollSender>().StopPolling();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Harness: Harness waits for 15 seconds.");

    /**
     * Step 11: Harness
     *   Description: Harness waits 15 seconds.
     *   Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kWait15s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.");

    /**
     * Step 12: Leader
     *   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *     SSED_1 mesh-local address. The frame should be buffered by the DUT in the indirect (SED) queue.
     *   Pass Criteria:
     *   - 12.1: The DUT MUST NOT relay the frame to SSED_1.
     */
    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kWait2s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: SSED_1: Resume sending synchronization messages containing CSL IEs.");

    /**
     * Step 13: SSED_1
     *   Description: Harness instructs the device to resume sending synchronization messages containing CSL IEs.
     *   Pass Criteria: N/A.
     */
    ssed1.Get<Mle::Mle>().ScheduleChildUpdateRequest();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1 (DUT): Automatically sends MAC Acknowledgement and relays the message.");

    /**
     * Step 14: Router_1 (DUT)
     *   Description: Automatically sends MAC Acknowledgement and relays the message.
     *   Pass Criteria:
     *   - 14.1: The DUT MUST send MAC Acknowledgement with the Frame Pending bit = 1.
     *   - 14.2: The DUT MUST forward the ICMPv6 Echo Request to SSED_1.
     */
    nexus.AdvanceTime(kWait2s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: SSED_1: Automatically responds with ICMPv6 Echo Reply.");

    /**
     * Step 15: SSED_1
     *   Description: Automatically responds with ICMPv6 Echo Reply.
     *   Pass Criteria:
     *   - 15.1: The DUT MUST forward the ICMPv6 Echo Reply.
     *   - Note: Harness waits 10 seconds for CSL synchronization to resume.
     */
    nexus.AdvanceTime(kWait10s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader: Harness verifies CSL synchronization by instructing the device to send an Echo Request.");

    /**
     * Step 16: Leader
     *   Description: Harness verifies CSL synchronization by instructing the device to send an ICMPv6 Echo Request to
     *     the SSED_1 mesh-local address.
     *   Pass Criteria:
     *   - 16.1: The ICMPv6 Echo Request frame MUST be buffered by the DUT and sent to SSED_1 within a subsequent CSL
     *     slot.
     *   - 16.2: The DUT must forward the ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    nexus.SaveTestInfo("test_1_2_LP_5_3_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_7();
    printf("All tests passed\n");
    return 0;
}
