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
 * Time to advance for the DUT to attach to the leader, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 10 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_1_1(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.1.1 Attaching to a Router
     *
     * 6.1.1.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     *
     * 6.1.1.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to successfully attach to a network.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * Attaching to a Parent | 4.7.1        | 4.5.1
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");

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

    if (aTopology == kTopologyA)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Topology A: ED_1 (DUT)");
    }
    else
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Topology B: SED_1 (DUT)");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader");

    /**
     * Step 1: Leader
     * - Description: Begin wireless sniffer and ensure the Leader is sending MLE Advertisements.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: DUT");

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to '0x02'.
     */
    if (aTopology == kTopologyA)
    {
        dut.Join(leader, Node::kAsMed);
    }
    else
    {
        dut.Join(leader, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically responds with a MLE Parent Response.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: DUT");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     * - Description: Receives the MLE Parent Response and automatically sends a MLE Child ID Request.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Request.
     *   - The following TLVs MUST be present in the Child ID Request:
     *     - Address Registration TLV
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to '0x02'.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Automatically responds with MLE Child ID Response.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
     *   link local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        // If no argument, run both topologies.
        ot::Nexus::RunTest6_1_1(ot::Nexus::kTopologyA, "test_6_1_1_A.json");
        ot::Nexus::RunTest6_1_1(ot::Nexus::kTopologyB, "test_6_1_1_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest6_1_1(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_1_1_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest6_1_1(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_1_1_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
