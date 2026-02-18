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
#include <string.h>

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
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 20 * 1000;

/**
 * Router 2 network ID timeout, in seconds.
 */
static constexpr uint32_t kRouter2NetworkIdTimeout = 110;

/**
 * Max Partition ID.
 */
static constexpr uint32_t kMaxPartitionId = 0xffffffff;

/**
 * End device timeout for Topology A, in seconds.
 */
static constexpr uint32_t kEndDeviceTimeout = 120;

/**
 * Poll period for SED in Topology B, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest_6_2_2(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.2.2 Connectivity when Parent Joins Partition
     *
     * 6.2.2.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader: Configured with NETWORK_ID_TIMEOUT = 120 seconds (default).
     * - Router_1: Parent of the DUT.
     * - Router_2: Set NETWORK_ID_TIMEOUT = 110 seconds. Set Partition ID to max value.
     *
     * 6.2.2.2 Purpose & Description
     * The purpose of this test case is to show that the DUT will uphold connectivity when the Leader is removed and
     *   Router_1 joins a new partition.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Children         | 5.16.6       | 5.16.6
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    if (aTopology == kTopologyA)
    {
        dut.SetName("ED_1");
    }
    else
    {
        dut.SetName("SED_1");
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly. Ensure that the DUT successfully attached to Router_1.
     * - Pass Criteria: N/A
     */
    leader.AllowList(router1);
    leader.AllowList(router2);
    router1.AllowList(leader);
    router1.AllowList(router2);
    router1.AllowList(dut);
    router2.AllowList(leader);
    router2.AllowList(router1);
    dut.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    if (aTopology == kTopologyA)
    {
        dut.Join(router1, Node::kAsMed);
        dut.Get<Mle::Mle>().SetTimeout(kEndDeviceTimeout);
    }
    else
    {
        dut.Join(router1, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_2");

    /**
     * Step 2: Router_2
     * - Description: Harness configures Router_2 with NETWORK_ID_TIMEOUT = 110 seconds.
     * - Pass Criteria: N/A
     */
    router2.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter2NetworkIdTimeout);
    router2.Get<Mle::Mle>().SetPreferredLeaderPartitionId(kMaxPartitionId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness silently removes the Leader from the network.
     * - Pass Criteria: N/A
     */
    leader.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_2");

    /**
     * Step 4: Router_2
     * - Description: Automatically creates new partition and begins transmitting MLE Advertisements.
     * - Pass Criteria: N/A
     */
    // Router 2 will timeout after 110 seconds and become leader.
    nexus.AdvanceTime(kRouter2NetworkIdTimeout * 1000 + kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically joins Router_2 partition.
     * - Pass Criteria: N/A
     */
    // Router 1 will timeout after 120 seconds and join Router 2's partition.
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router2.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == kMaxPartitionId);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router1.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == kMaxPartitionId);

    if (aTopology == kTopologyA)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Step 6: Test Harness (Topology A only)");

        /**
         * Step 6: Test Harness (Topology A only)
         * - Description: Wait for 2x (double) the End Device timeout period.
         * - Pass Criteria: N/A
         */
        nexus.AdvanceTime(2 * kEndDeviceTimeout * 1000);

        Log("---------------------------------------------------------------------------------------");
        Log("Step 7: MED_1 (DUT) [Topology A only]");

        /**
         * Step 7: MED_1 (DUT) [Topology A only]
         * - Description: Automatically sends MLE Child Update Request to Router_1. Router_1 automatically sends a MLE
         *   Child Update Response to MED_1.
         * - Pass Criteria:
         *   - The DUT MUST send a MLE Child Update Request to Router_1, including the following TLVs:
         *     - Source Address TLV
         *     - Leader Data TLV
         *       - Partition ID (value = max value)
         *       - Version (value matches its parent value)
         *       - Stable Version (value matches its parent value)
         *     - Mode TLV
         *   - The DUT MUST NOT transmit a MLE Announce message or an additional MLE Child ID Request.
         */
        nexus.AdvanceTime(kStabilizationTime);
    }
    else
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Step 8: SED_1 (DUT) [Topology B only]");

        /**
         * Step 8: SED_1 (DUT) [Topology B only]
         * - Description: Automatically sends periodic 802.15.4 Data Request messages as part of the keep-alive message.
         * - Pass Criteria:
         *   - The DUT MUST send a 802.15.4 Data Request command to the parent device and receive an ACK message in
         *     response.
         *   - The DUT MUST NOT transmit a MLE Announce message or an additional MLE Child ID Request. If it does, the
         *     test has failed.
         */
        nexus.AdvanceTime(kStabilizationTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1");

    /**
     * Step 9: Router_1
     * - Description: To verify connectivity, Harness instructs Router_1 to send an ICMPv6 Echo Request to the DUT link
     *   local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(router1, dut.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest_6_2_2(ot::Nexus::kTopologyA, "test_6_2_2_A.json");
        ot::Nexus::RunTest_6_2_2(ot::Nexus::kTopologyB, "test_6_2_2_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest_6_2_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_2_2_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest_6_2_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_2_2_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
