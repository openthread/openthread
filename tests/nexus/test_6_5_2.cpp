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
 * Time to advance for the DUT to attach to a parent, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 20 * 1000;

/**
 * Time to wait for child synchronization, in milliseconds.
 */
static constexpr uint32_t kChildSyncTimeout = 20 * 1000;

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

void RunTest_6_5_2(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.5.2 Child Synchronization after Reset - No Parent Response
     *
     * 6.5.2.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     * - Router_1
     *
     * 6.5.2.2 Purpose & Description
     * The purpose of this test case is to validate that after the DUT resets and receives no response from its parent,
     *   it will reattach to the network through a different parent.
     *
     * Spec Reference                   | V1.1 Section | V1.3.0 Section
     * ---------------------------------|--------------|---------------
     * Child Synchronization after Reset | 4.7.6        | 4.6.4
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
    if (aTopology == kTopologyA)
    {
        Log("Topology A: ED_1 (DUT)");
    }
    else
    {
        Log("Topology B: SED_1 (DUT)");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly
     * - Pass Criteria: N/A
     */

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
        dut.Join(router1, Node::kAsMed);
    }
    else
    {
        dut.Join(router1, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness silently removes Router_1 from the network
     * - Pass Criteria: N/A
     */
    router1.Reset();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: ED_1 / SED_1 (DUT)");

    /**
     * Step 3: ED_1 / SED_1 (DUT)
     * - Description: User is prompted to reset the DUT
     * - Pass Criteria: N/A
     */
    dut.Reset();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED_1 / SED_1 (DUT)");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends an MLE Child Update Request to Router_1
     * - Pass Criteria:
     *   - The following TLVs MUST be included in the Child Update Request:
     *     - Mode TLV
     *     - Challenge TLV (required for Thread version >= 4)
     *     - Address Registration TLV (optional)
     *   - If the DUT is a SED, it MUST resume polling after sending MLE Child Update Request.
     */

    /**
     * After reset, we need to restart the stack
     */
    dut.Get<ThreadNetif>().Up();
    if (aTopology == kTopologyB)
    {
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }
    SuccessOrQuit(dut.Get<Mle::Mle>().Start());

    /**
     * Step 4 happens automatically after start as the DUT tries to sync with its known parent.
     *   We allow some time for the Child Update Request to be sent.
     */
    nexus.AdvanceTime(kChildSyncTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: No response
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: ED_1 / SED_1 (DUT)");

    /**
     * Step 6: ED_1 / SED_1 (DUT)
     * - Description: Automatically attaches to the Leader
     * - Pass Criteria:
     *   - The DUT MUST attach to the Leader by following the procedure in 6.1.1 Attaching to a Router
     */

    /**
     * Enable link between Leader and DUT so it can re-attach to Leader.
     */
    leader.AllowList(dut);
    dut.AllowList(leader);

    /**
     * Wait for the DUT to realize synchronization failed and start a new attach process.
     *   This may take some time depending on MLE timeout.
     */
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
     *   link local address
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply
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
        ot::Nexus::RunTest_6_5_2(ot::Nexus::kTopologyA, "test_6_5_2_A.json");
        ot::Nexus::RunTest_6_5_2(ot::Nexus::kTopologyB, "test_6_5_2_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest_6_5_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_5_2_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest_6_5_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_5_2_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
