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
 * Time to advance for the DUT to attach to the leader, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

/**
 * Payload size for fragmented ICMPv6 Echo Request, in bytes.
 * A size of 1200 bytes plus headers will exceed the 802.15.4 frame size and trigger fragmentation.
 */
static constexpr uint16_t kFragmentedPayloadSize = 1200;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_4_2(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.4.2 Realm-Local Addressing
     *
     * 6.4.2.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     *
     * 6.4.2.2 Purpose & Description
     * The purpose of this test case is to validate the Realm-Local addresses that the DUT configures.
     *
     * Spec Reference    | V1.1 Section | V1.3.0 Section
     * ------------------|--------------|---------------
     * Realm-Local Scope | 5.2.3.2      | 5.2.1.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

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
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    router1.AllowList(leader);
    router1.AllowList(dut);
    dut.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    if (aTopology == kTopologyA)
    {
        dut.Join(leader, Node::kAsMed);
    }
    else
    {
        dut.Join(leader, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT ML-EID.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the DUT ML-EID.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetMeshLocalEid(), kFragmentedPayloadSize, 64,
                                   kEchoTimeout);

    if (aTopology == kTopologyA)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Step 4: Leader [Topology A only]");

        /**
         * Step 4: Leader [Topology A only]
         * - Description: Harness instructs the device to send a ICMPv6 Echo Request to the Realm-Local All-Nodes
         *   multicast address (FF03::1).
         * - Pass Criteria:
         *   - The DUT (ED_1) MUST respond with an ICMPv6 Echo Reply.
         */
        nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllNodesMulticast(), 0, 64, kEchoTimeout);

        Log("---------------------------------------------------------------------------------------");
        Log("Step 5: Leader [Topology A only]");

        /**
         * Step 5: Leader [Topology A only]
         * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
         *   All-Nodes multicast address (FF03::1).
         * - Pass Criteria:
         *   - The DUT (ED_1) MUST respond with an ICMPv6 Echo Reply.
         */
        nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllNodesMulticast(), kFragmentedPayloadSize,
                                       64, kEchoTimeout);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All Thread Nodes
     *   multicast address.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, leader.Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress(), 0, 64,
                                   kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local All
     *   Thread Nodes multicast address.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, leader.Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress(),
                                   kFragmentedPayloadSize, 64, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        // If no argument, run both topologies.
        ot::Nexus::RunTest6_4_2(ot::Nexus::kTopologyA, "test_6_4_2_A.json");
        ot::Nexus::RunTest6_4_2(ot::Nexus::kTopologyB, "test_6_4_2_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest6_4_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_4_2_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest6_4_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_4_2_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
