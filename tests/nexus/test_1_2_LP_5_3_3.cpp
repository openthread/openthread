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

#include "mac/data_poll_sender.hpp"
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
 * Time to wait for SSED CSL timeout at DUT, in milliseconds.
 */
static constexpr uint32_t kSsedUnsyncWaitTime = 25 * 1000;

/**
 * Time to wait for CSL synchronization to resume, in milliseconds.
 */
static constexpr uint32_t kWaitTime18s = 18 * 1000;

/**
 * CSL period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 500;

/**
 * CSL timeout in seconds.
 */
static constexpr uint32_t kCslTimeout = 20;

/**
 * Time to advance for ICMPv6 Echo Request/Response, in milliseconds.
 */
static constexpr uint32_t kEchoWaitTime = kCslPeriodMs * 2;

/**
 * Time to advance for CSL synchronization to complete, in milliseconds.
 */
static constexpr uint32_t kCslSyncTime = 5 * 1000;

/**
 * ICMPv6 Echo Request Identifiers.
 */
static constexpr uint16_t kEchoId1 = 0x01;
static constexpr uint16_t kEchoId2 = 0x02;
static constexpr uint16_t kEchoId3 = 0x03;

/**
 * Time to advance for the final ICMPv6 Echo Request, in milliseconds.
 */
static constexpr uint32_t kFinalEchoWaitTime = kCslPeriodMs * 4;

/**
 * Payload size for ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void Test1_2_LP_5_3_3(void)
{
    /**
     * 5.3.3 SSED Becomes Unsynchronized
     *
     * 5.3.3.1 Topology
     * - Leader (DUT)
     * - Router 1
     * - SSED 1
     *
     * 5.3.3.2 Purpose & Description
     * The purpose of this test is to validate that when the DUT considers the SSED unsynchronized, it does not use CSL
     *   transmission to transmit any messages, but rather falls back to indirect transmission until the SSED is CSL
     *   synchronized again.
     *
     * Spec Reference                | V1.2 Section
     * ------------------------------|--------------
     * SSED Becomes Unsynchronized   | 4.6.5.1.1
     */

    Core nexus;

    Node &dut     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ssed1   = nexus.CreateNode();

    dut.SetName("DUT");
    router1.SetName("ROUTER_1");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0: SSED_1
     * - Description: Preconditions: Set CSL Synchronized Timeout = 20s. Set CSL Period = 500ms.
     * - Pass Criteria: N/A.
     */
    Log("Step 0: SSED_1");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, Router_1, SSED_1.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: All");

    /**
     * Use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - Leader (DUT) and Router 1
     * - Leader (DUT) and SSED 1
     */
    dut.AllowList(router1);
    router1.AllowList(dut);

    dut.AllowList(ssed1);
    ssed1.AllowList(dut);

    dut.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());

    router1.Join(dut);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    /**
     * Step 2: SSED_1
     * - Description: Automatically attaches to the DUT and establishes CSL synchronization.
     * - Pass Criteria:
     *   - The DUT MUST sent MLE Child ID Response to SSED_1.
     *   - The DUT MUST unicast MLE Child Update Response to SSED_1.
     */
    Log("Step 2: SSED_1");

    ssed1.Join(dut, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);

    nexus.AdvanceTime(kCslSyncTime);
    VerifyOrQuit(ssed1.Get<Mac::Mac>().GetCslPeriod() > 0);

    /**
     * Step 3: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to SSED_1
     *   mesh-local address.
     * - Pass Criteria: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL
     *   slot.
     */
    Log("Step 3: Router_1");

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId1, kEchoPayloadSize);
    nexus.AdvanceTime(kEchoWaitTime);

    /**
     * Step 4: SSED_1
     * - Description: Automatically replies with ICMPv6 Echo Reply.
     * - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Reply from SSED_1.
     */
    Log("Step 4: SSED_1");

    nexus.AdvanceTime(kEchoWaitTime);

    /**
     * Step 5: SSED_1
     * - Description: Harness instructs the device to stop sending periodic synchronization data poll frames.
     * - Pass Criteria: N/A.
     */
    Log("Step 5: SSED_1");

    ssed1.Get<DataPollSender>().StopPolling();

    /**
     * Step 6: Harness
     * - Description: Harness waits 25 seconds to time out the SSED / CSL connection.
     * - Pass Criteria: N/A (Implicit).
     */
    Log("Step 6: Harness");

    nexus.AdvanceTime(kSsedUnsyncWaitTime);

    /**
     * Step 7: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address.
     * - Pass Criteria: N/A.
     */
    Log("Step 7: Router_1");

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId2, kEchoPayloadSize);

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically buffers the frame for SSED_1 in the indirect (SED) queue.
     * - Pass Criteria: The DUT MUST NOT relay the ICMPv6 Echo Request frame to SSED_1.
     */
    Log("Step 8: Leader (DUT)");

    nexus.AdvanceTime(kEchoWaitTime);

    /**
     * Step 9: SSED_1
     * - Description: Harness instructs the device to resume sending synchronization messages containing CSL IEs.
     * - Pass Criteria: N/A.
     */
    Log("Step 9: SSED_1");

    ssed1.Get<DataPollSender>().StartPolling();

    /**
     * Step 10: Harness
     * - Description: Harness waits 18 seconds for the SSED / CSL connection to resume.
     * - Pass Criteria: N/A (Implicit).
     */
    Log("Step 10: Harness");

    nexus.AdvanceTime(kWaitTime18s);

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically sends MAC Acknowledgement then relays the message.
     * - Pass Criteria: The DUT MUST verify that the Frame Pending bit is set and successfully transmits the ICMPv6 Echo
     *   Request.
     */
    Log("Step 11: Leader (DUT)");

    nexus.AdvanceTime(kEchoWaitTime);

    /**
     * Step 12: SSED_1
     * - Description: Automatically responds with ICMPv6 Echo Reply.
     * - Pass Criteria:
     *   - SSED_1 MUST respond with ICMPv6 Echo Reply.
     *   - The DUT MUST relay the ICMPv6 Echo Reply to Router_1.
     */
    Log("Step 12: SSED_1");

    nexus.AdvanceTime(kEchoWaitTime);

    /**
     * Step 12b: SSED_1
     * - Description: Harness instructs the device to stop sending periodic synchronization data poll frames.
     * - Pass Criteria: N/A.
     */
    Log("Step 12b: SSED_1");

    ssed1.Get<DataPollSender>().StopPolling();

    /**
     * Step 13: Router_1
     * - Description: Harness verifies CSL synchronization by instructing the device to send an ICMPv6 Echo Request to
     *   the SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
     *   - SSED_1 MUST respond with ICMPv6 Echo Reply.
     *   - The DUT MUST relay the ICMPv6 Echo Reply to Router_1.
     */
    Log("Step 13: Router_1");

    router1.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId3, kEchoPayloadSize);
    nexus.AdvanceTime(kFinalEchoWaitTime);

    nexus.SaveTestInfo("test_1_2_LP_5_3_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_3();
    printf("All tests passed\n");
    return 0;
}
