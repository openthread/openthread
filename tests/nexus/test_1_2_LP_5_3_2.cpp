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
#include "thread/child_table.hpp"

namespace ot {
namespace Nexus {

/**
 * Time conversion constants.
 */
static constexpr uint32_t kMsPerSecond = 1000;
static constexpr uint32_t kUsPerMs     = 1000;

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * kMsPerSecond;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * kMsPerSecond;

/**
 * Time to advance for a node to join as a SSED.
 */
static constexpr uint32_t kAttachAsSsedTime = 5 * kMsPerSecond;

/**
 * Time to wait in the test, in milliseconds.
 */
static constexpr uint32_t kWaitTime = 18 * kMsPerSecond;

/**
 * CSL period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 500;

/**
 * CSL period in units of 10 symbols.
 */
static constexpr uint32_t kCslPeriodInTenSymbols = kCslPeriodMs * kUsPerMs / kUsPerTenSymbols;

/**
 * CSL timeout in seconds.
 */
static constexpr uint32_t kCslTimeout = 20;

/**
 * CSL timeout in seconds to deactivate autosynchronization.
 */
static constexpr uint32_t kDeactivatedCslTimeout = 100;

/**
 * UDP port for synchronization message.
 */
static constexpr uint16_t kUdpPort = 1234;

/**
 * Payload size for ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Identifiers for ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoId1 = 0x01;
static constexpr uint16_t kEchoId2 = 0x02;
static constexpr uint16_t kEchoId3 = 0x03;

/**
 * Multiplier for CSL period to allow for message delivery and response.
 */
static constexpr uint32_t kCslWaitMultiplier = 2;

/**
 * Time to wait for CSL message delivery and response, in milliseconds.
 */
static constexpr uint32_t kCslWaitTime = kCslPeriodMs * kCslWaitMultiplier;

/**
 * Helper to send a UDP message.
 */
static void SendUdpMessage(Node &aNode, const Ip6::Address &aDestination, uint16_t aPort)
{
    Message             *message;
    Ip6::MessageInfo     messageInfo;
    static const uint8_t kPayload[] = {0x01, 0x02, 0x03, 0x04};

    message = aNode.Get<Ip6::Udp>().NewMessage();
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(message->AppendBytes(kPayload, sizeof(kPayload)));

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetPeerPort(aPort);

    Log("Sending UDP message from Node %lu (%s) to %s", ToUlong(aNode.GetId()), aNode.GetName(),
        aDestination.ToString().AsCString());

    SuccessOrQuit(aNode.Get<Ip6::Udp>().SendDatagram(*message, messageInfo));
}

/**
 * Helper to verify CSL synchronization and timer reset on the router.
 */
static void VerifyCslTimerReset(Node &aRouter, Node &aSsed)
{
    const Child *child = aRouter.Get<ChildTable>().FindChild(aSsed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateAny);

    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(child->IsCslSynchronized());

    // Resetting to 0 means CslLastHeard is updated to current time.
    Log("Checking CSL timer reset: Now=%u, LastHeard=%u", TimerMilli::GetNow().GetValue(),
        child->GetCslLastHeard().GetValue());
    VerifyOrQuit(TimerMilli::GetNow() - child->GetCslLastHeard() <= kCslWaitTime);
}

void Test1_2_LP_5_3_2(void)
{
    /**
     * 5.3.2 CSL Synchronized Communication
     *
     * 5.3.2.1 Topology
     * - Leader
     * - Router 1 (DUT)
     * - SSED 1
     *
     * 5.3.2.2 Purpose & Description
     * The purpose of this test is to validate that the DUT is successfully able to sustain a CSL connection with a
     *   SSED that uses different types of messages to maintain its CSL Synchronization.
     *
     * 5.3.2.3 Configuration
     * - Leader: Thread 1.2 Leader.
     * - Router 1 (DUT): Thread 1.2 Router.
     * - SSED_1: Thread 1.2 SSED.
     * - SSED_1 is configured to have a CSL Synchronized Timeout value of 20s.
     * - Set CSL Period = 500ms.
     *
     * Spec Reference                 | V1.2 Section
     * -------------------------------|--------------
     * CSL Synchronized Communication | 4.6.5.2.3
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
     * - Pass Criteria: N/A.
     */
    Log("Step 0: SSED_1");

    // Pre-set CSL parameters on SSED_1 before it joins.
    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriodInTenSymbols);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);

    /**
     * Step 1: All
     * - Description: Topology formation: Leader, DUT, SSED_1.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: All");

    /** Use AllowList feature to restrict the topology. */
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
     * - Description: Automatically attaches to the DUT.
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */
    Log("Step 2: SSED_1");

    ssed1.Join(router1, Node::kAsSed);

    // Override again after Join just in case Join() resets them to defaults.
    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriodInTenSymbols);
    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);

    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    /**
     * Step 2a: SSED_1
     * - Description: Harness instructs the device to deactivate autosynchronization.
     * - Pass Criteria: N/A.
     */
    Log("Step 2a: SSED_1");

    // Deactivate autosynchronization by setting a very long CSL timeout on SSED_1.
    ssed1.Get<Mle::Mle>().SetCslTimeout(kDeactivatedCslTimeout);

    /**
     * Step 3: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 3: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to send a UDP message to SSED_1 mesh-local address to
     *   synchronize it.
     * - Pass Criteria: SSED_1 MUST NOT send a MAC Data Request prior to receiving the UDP message from the
     *   Leader.
     */
    Log("Step 4: Leader");

    SendUdpMessage(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kUdpPort);

    /**
     * Step 5: SSED_1
     * - Description: Automatically responds to the DUT with an enhanced acknowledgement that contains CSL IEs.
     * - Pass Criteria: The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
     */
    Log("Step 5: SSED_1");

    nexus.AdvanceTime(kCslWaitTime); // Allow time for message delivery and ACK.
    VerifyCslTimerReset(router1, ssed1);

    /**
     * Step 6: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 6: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 7: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to
     *   the SSED_1 mesh-local address via the DUT.
     * - Pass Criteria:
     *   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
     *   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
     */
    Log("Step 7: Leader");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId1, kEchoPayloadSize);
    nexus.AdvanceTime(kCslWaitTime); // Allow time for Echo Request and Reply.

    /**
     * Step 8: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 8: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 9: SSED_1
     * - Description: Harness instructs the device to send a MAC Data Request message containing CSL IEs to the
     *   DUT.
     * - Pass Criteria: The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
     */
    Log("Step 9: SSED_1");

    SuccessOrQuit(ssed1.Get<DataPollSender>().SendDataPoll());
    nexus.AdvanceTime(kCslWaitTime);
    VerifyCslTimerReset(router1, ssed1);

    /**
     * Step 10: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 10: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 11: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to
     *   the SSED_1 mesh-local address via the DUT.
     * - Pass Criteria:
     *   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
     *   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
     */
    Log("Step 11: Leader");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId2, kEchoPayloadSize);
    nexus.AdvanceTime(kCslWaitTime);

    /**
     * Step 12: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 12: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 13: SSED_1
     * - Description: Harness instructs the device to send a UDP Message to the DUT.
     * - Pass Criteria:
     *   - The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
     *   - Check that the 802.15.4 Frame Header includes following Information Elements:
     *     - CSL Period IE (Check value of this TLV)
     *     - CSL Phase IE
     *   - Compare the value of this TLV with the expected value.
     */
    Log("Step 13: SSED_1");

    SendUdpMessage(ssed1, router1.Get<Mle::Mle>().GetMeshLocalEid(), kUdpPort);
    nexus.AdvanceTime(kCslWaitTime);
    VerifyCslTimerReset(router1, ssed1);

    /**
     * Step 14: Router (DUT)
     * - Description: Automatically sends an Enhanced ACK to SSED_1.
     * - Pass Criteria:
     *   - The Information Elements MUST NOT be included.
     *   - The Enhanced ACK must occur within the ACK timeout.
     */
    Log("Step 14: Router (DUT)");

    /**
     * Step 15: Harness
     * - Description: Harness waits for 18 seconds.
     * - Pass Criteria: N/A.
     */
    Log("Step 15: Harness");

    nexus.AdvanceTime(kWaitTime);

    /**
     * Step 16: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to
     *   the SSED_1 mesh-local address via the DUT.
     * - Pass Criteria:
     *   - SSED1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
     *   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
     */
    Log("Step 16: Leader");

    leader.SendEchoRequest(ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoId3, kEchoPayloadSize);
    nexus.AdvanceTime(kCslWaitTime);

    nexus.SaveTestInfo("test_1_2_LP_5_3_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_2();
    printf("All tests passed\n");
    return 0;
}
