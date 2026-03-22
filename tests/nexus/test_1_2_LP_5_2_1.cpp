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

#include "mac/data_poll_sender.hpp"
#include "thread/child_table.hpp"
#include "thread/mesh_forwarder.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachToChildTime = 10 * 1000;

/**
 * Time to advance for a node to join as a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Data poll period in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 1000;

/**
 * Very long data poll period in milliseconds (effectively disabling periodic polling).
 */
static constexpr uint32_t kLongPollPeriod = 3600 * 1000;

/**
 * Short wait time to advance nexus time, in milliseconds.
 */
static constexpr uint32_t kShortWaitTime = 100;

/**
 * Wait time for ICMPv6 Echo Reply, in milliseconds.
 */
static constexpr uint32_t kEchoReplyWaitTime = 500;

/**
 * Wait time to ensure a data poll has occurred, in milliseconds.
 */
static constexpr uint32_t kPollWaitTime = 2000;

void Test1_2_LP_5_2_1(void)
{
    /**
     * 5.2.1 Enhanced Frame Pending with Thread V1.2 End Device & V1.1 End Device
     *
     * 5.2.1.1 Topology
     * - SED_1: is Thread 1.1 SED.
     * - SED_2: is Thread 1.2 SED with Enhanced Frame Pending (EFP) support; polling via 802.15.4 MAC Data Requests is
     *   disabled.
     * - SED_3: is Thread 1.2 SED with Enhanced Frame Pending (EFP) support; polling via 802.15.4 MAC Data Requests is
     *   enabled.
     * - Router_1: is a Thread Router.
     * - DUT: is Thread Leader.
     *
     * 5.2.1.2 Purpose & Description
     * This test case verifies that the DUT correctly manages the Frame Pending bit in acknowledgments to MAC Data and
     *   Data Request frames for Thread V1.2 and V1.1 sleepy end devices.
     *
     * Spec Reference          | V1.2 Section
     * ------------------------|--------------
     * Enhanced Frame Pending  | 3.2.6.2
     */

    Core nexus;

    Node &dut     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();
    Node &sed2    = nexus.CreateNode();
    Node &sed3    = nexus.CreateNode();

    dut.SetName("LEADER");
    router1.SetName("ROUTER_1");
    sed1.SetName("SED_1");
    sed2.SetName("SED_2");
    sed3.SetName("SED_3");

    /**
     * - Use AllowList to specify links between nodes. There is a link between the following node pairs:
     *   - Leader (DUT) and Router 1
     *   - Leader (DUT) and SED 1
     *   - Leader (DUT) and SED 2
     *   - Leader (DUT) and SED 3
     */
    dut.AllowList(router1);
    router1.AllowList(dut);

    dut.AllowList(sed1);
    sed1.AllowList(dut);

    dut.AllowList(sed2);
    sed2.AllowList(dut);

    dut.AllowList(sed3);
    sed3.AllowList(dut);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1: All Devices
     * - Topology formation.
     */
    Log("Step 1: Topology formation");
    dut.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(dut, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    sed1.Join(dut, Node::kAsSed);
    sed2.Join(dut, Node::kAsSed);
    sed3.Join(dut, Node::kAsSed);
    nexus.AdvanceTime(kAttachToChildTime);

    /**
     * - Manually set SED_1 version to 1.1 in Leader's child table to strictly follow the test spec.
     */
    {
        Child *child = dut.Get<ChildTable>().FindChild(sed1.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
        VerifyOrQuit(child != nullptr);
        child->SetVersion(kThreadVersion1p1);
    }

    /**
     * - SED_1 – enable polling via 802.15.4 MAC Data Requests.
     * - SED_2 – disable polling via 802.15.4 MAC Data Requests.
     * - SED_3 – enable polling via 802.15.4 MAC Data Requests.
     */
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    SuccessOrQuit(sed2.Get<DataPollSender>().SetExternalPollPeriod(kLongPollPeriod));
    SuccessOrQuit(sed3.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    /**
     * Step 2: SED_2
     * - Harness instructs the device to send an empty MAC Data frame to the DUT (Leader).
     * - Pass Criteria: DUT MUST send an acknowledgment with Frame Pending bit set to 0.
     */
    Log("Step 2: SED_2 sends empty MAC Data frame to Leader");
    SuccessOrQuit(sed2.Get<MeshForwarder>().SendEmptyMessage());
    nexus.AdvanceTime(kShortWaitTime);

    /**
     * Step 3: Router_1
     * - Harness instructs the device to send an ICMPv6 Echo Request to SED_2.
     * - Pass Criteria: N/A.
     */
    Log("Step 3: Router_1 sends Echo Request to SED_2");
    router1.SendEchoRequest(sed2.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kShortWaitTime);

    /**
     * Step 4: SED_2
     * - Harness instructs the device to send an empty MAC Data frame to the DUT (Leader).
     * - Pass Criteria: DUT MUST send an acknowledgment with Frame Pending bit set to 1.
     */
    Log("Step 4: SED_2 sends empty MAC Data frame to Leader");
    SuccessOrQuit(sed2.Get<MeshForwarder>().SendEmptyMessage());
    nexus.AdvanceTime(kShortWaitTime);

    /**
     * Step 5: SED_2
     * - Harness instructs the device to send an 802.15.4 Data Request message to the DUT (Leader).
     * - Pass Criteria: N/A.
     */
    Log("Step 5: SED_2 sends Data Request to Leader");
    SuccessOrQuit(sed2.Get<DataPollSender>().SendDataPoll());
    nexus.AdvanceTime(kShortWaitTime);

    /**
     * Step 6: DUT
     * - DUT sends an acknowledgment with Frame Pending bit set to 1.
     * - Pass Criteria: N/A.
     */
    Log("Step 6: DUT sends acknowledgment with Frame Pending bit set to 1");
    // Handled by Step 5 execution and verification.

    /**
     * Step 7: DUT
     * - Forward the ICMPv6 Echo Request to SED_2.
     * - Pass Criteria: N/A.
     */
    Log("Step 7: DUT forwards Echo Request to SED_2");
    // Handled by Step 5 execution and verification.

    /**
     * Step 8: SED_2
     * - Acknowledges the ICMPv6 Echo Request frame.
     * - Pass Criteria: N/A.
     */
    Log("Step 8: SED_2 acknowledges Echo Request");
    // Handled by Step 5 execution and verification.

    /**
     * Step 9: SED_2
     * - Automatically replies with an ICMPv6 Echo Reply to Router_1.
     * - Pass Criteria: N/A.
     */
    Log("Step 9: SED_2 sends Echo Reply to Router_1");
    nexus.AdvanceTime(kEchoReplyWaitTime);

    /**
     * Step 10: DUT
     * - DUT MUST send an acknowledgment with Frame Pending bit set to 0 to SED_2.
     * - Pass Criteria: N/A.
     */
    Log("Step 10: DUT sends acknowledgment with Frame Pending bit set to 0 to SED_2");
    // Handled by Step 9 execution and verification.

    /**
     * Step 11: Router_1
     * - Harness instructs the device to send an ICMPv6 Echo Request to SED_3.
     * - Pass Criteria: N/A.
     */
    Log("Step 11: Router_1 sends Echo Request to SED_3");
    router1.SendEchoRequest(sed3.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPollWaitTime); // Wait for poll

    /**
     * Step 12: SED_3
     * - Automatically replies with an ICMPv6 Echo Reply to Router_1.
     * - Pass Criteria: Router_1 MUST receive the Echo Reply.
     */
    Log("Step 12: SED_3 sends Echo Reply to Router_1");
    nexus.AdvanceTime(kPollWaitTime);

    /**
     * Step 13: Router_1
     * - Harness instructs the device to send an ICMPv6 Echo Request to SED_1.
     * - Pass Criteria: N/A.
     */
    Log("Step 13: Router_1 sends Echo Request to SED_1");
    router1.SendEchoRequest(sed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPollWaitTime); // Wait for poll

    /**
     * Step 14: SED_1
     * - Automatically replies with an ICMPv6 Echo Reply to Router_1.
     * - Pass Criteria: Router_1 MUST receive the Echo Reply.
     */
    Log("Step 14: SED_1 sends Echo Reply to Router_1");
    nexus.AdvanceTime(kPollWaitTime);

    nexus.SaveTestInfo("test_1_2_LP_5_2_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_2_1();
    printf("All tests passed\n");
    return 0;
}
