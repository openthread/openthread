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

static constexpr uint32_t kFormNetworkTime       = 13 * 1000;
static constexpr uint32_t kAttachTime            = 200 * 1000;
static constexpr uint32_t kPartitionCreationTime = 300 * 1000;
static constexpr uint32_t kStabilizationTime     = 10 * 1000;
static constexpr uint32_t kEchoTimeout           = 5000;
static constexpr uint32_t kPollPeriod            = 500;
static constexpr uint16_t kEchoIdentifier        = 0;
static constexpr uint16_t kEchoPayloadSize       = 64;

enum Topology : uint8_t
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_2_1(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.2.1 Connectivity when Parent Creates Partition
     *
     * 6.2.1.1 Topology
     * - Topology A: DUT as End Device (ED_1) attached to Router_1.
     * - Topology B: DUT as Sleepy End Device (SED_1) attached to Router_1.
     * - Leader: Connected to Router_1.
     *
     * 6.2.1.2 Purpose & Description
     * The purpose of this test case is to show that the DUT upholds connectivity, or reattaches with its parent, when
     *   the Leader is removed and the Router creates a new partition.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Children         | 5.16.6       | 5.16.6
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

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    leader.AllowList(router1);
    router1.AllowList(leader);
    router1.AllowList(dut);
    dut.AllowList(router1);

    leader.Form();

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    if (aTopology == kTopologyA)
    {
        dut.Join(router1, Node::kAsMed);
    }
    else
    {
        dut.Join(router1, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2: Leader
     * - Description: Harness silently powers-down the Leader.
     * - Pass Criteria: N/A
     */
    Log("Step 2: Leader");
    leader.Get<Mle::Mle>().Stop();
    leader.Get<ThreadNetif>().Down();

    /**
     * Step 3: Router_1
     * - Description: Automatically creates new partition and begins transmitting MLE Advertisements.
     * - Pass Criteria: N/A
     */
    Log("Step 3: Router_1");
    nexus.AdvanceTime(kPartitionCreationTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());

    /**
     * Step 4: MED_1 / SED_1 (DUT)
     * - Description: Automatically remains attached or reattaches to Router_1.
     * - Pass Criteria: N/A
     */
    Log("Step 4: MED_1 / SED_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());

    /**
     * Step 5: Router_1
     * - Description: To verify connectivity, Harness instructs the device to send an ICMPv6 Echo Request to the DUT
     *   link local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */
    Log("Step 5: Router_1");
    nexus.SendAndVerifyEchoRequest(router1, dut.Get<Mle::Mle>().GetLinkLocalAddress(), kEchoIdentifier,
                                   kEchoPayloadSize, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} /* namespace Nexus */
} /* namespace ot */

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_2_1(ot::Nexus::kTopologyA, "test_6_2_1_A.json");
        ot::Nexus::RunTest6_2_1(ot::Nexus::kTopologyB, "test_6_2_1_B.json");
    }
    else
    {
        ot::Nexus::Topology topology;
        const char         *defaultJsonFile;

        if (strcmp(argv[1], "A") == 0)
        {
            topology        = ot::Nexus::kTopologyA;
            defaultJsonFile = "test_6_2_1_A.json";
        }
        else if (strcmp(argv[1], "B") == 0)
        {
            topology        = ot::Nexus::kTopologyB;
            defaultJsonFile = "test_6_2_1_B.json";
        }
        else
        {
            fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
            return 1;
        }

        ot::Nexus::RunTest6_2_1(topology, (argc > 2) ? argv[2] : defaultJsonFile);
    }

    printf("All tests passed\n");
    return 0;
}
