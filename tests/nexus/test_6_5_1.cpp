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

/**
 * Child timeout duration for SED in seconds.
 */
static constexpr uint32_t kChildTimeout = 4;

/**
 * Time to reset the DUT, in milliseconds. Must be greater than Child Timeout.
 */
static constexpr uint32_t kResetTime = (kChildTimeout + 5) * 1000;

/**
 * Time to advance for re-attachment and synchronization, in milliseconds.
 */
static constexpr uint32_t kReattachTime = 40 * 1000;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_5_1(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.5.1 Child Synchronization after Reset - Reattach
     *
     * 6.5.1.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     *
     * 6.5.1.2 Purpose & Description
     * The purpose of this test case is to validate that after the DUT resets for a time period longer than the Child
     *   Timeout value, it sends an MLE Child Update Request and reattaches to its parent.
     *
     * Spec Reference                    | V1.1 Section | V1.3.0 Section
     * ----------------------------------|--------------|---------------
     * Child Synchronization after Reset | 4.7.6        | 4.6.4
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

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    leader.AllowList(dut);
    dut.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    if (aTopology == kTopologyA)
    {
        dut.Join(leader, Node::kAsMed);
    }
    else
    {
        dut.Join(leader, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    dut.Get<Mle::Mle>().SetTimeout(kChildTimeout);

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     * - Description: Test Harness Prompt: Reset the DUT for a time greater than the Child Timeout Duration.
     * - Pass Criteria: N/A
     */
    Log("Step 2: ED_1 / SED_1 (DUT)");

    dut.Reset();
    nexus.AdvanceTime(kResetTime);

    /**
     * Step 3: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Child Update Request to the Leader.
     * - Pass Criteria:
     *   - The following TLVs MUST be included in the Child Update Request:
     *     - Mode TLV
     *     - Challenge TLV (required for Thread version >= 4)
     *     - Address Registration TLV (optional)
     *   - If the DUT is a SED, it MUST resume polling after sending MLE Child Update.
     */
    Log("Step 3: ED_1 / SED_1 (DUT)");

    dut.Get<ThreadNetif>().Up();
    if (aTopology == kTopologyB)
    {
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }
    SuccessOrQuit(dut.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(5000);

    /**
     * Step 4: Leader
     * - Description: Automatically sends an MLE Child Update Response with a status of “Error”.
     * - Pass Criteria: N/A
     */
    Log("Step 4: Leader");

    nexus.AdvanceTime(5000);

    /**
     * Step 5: ED_1 / SED_1 (DUT)
     * - Description: Automatically reattaches to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST reattach to the Leader following the procedure in 6.1.1 Attaching to a Router.
     */
    Log("Step 5: ED_1 / SED_1 (DUT)");

    nexus.AdvanceTime(kReattachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    /**
     * Step 6: Leader
     * - Description: Harness verifies connectivity by instructing device to send an ICMPv6 Echo Request to the DUT
     *   link local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */
    Log("Step 6: Leader");

    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_5_1(ot::Nexus::kTopologyA, "test_6_5_1_A.json");
        ot::Nexus::RunTest6_5_1(ot::Nexus::kTopologyB, "test_6_5_1_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest6_5_1(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_5_1_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest6_5_1(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_5_1_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
