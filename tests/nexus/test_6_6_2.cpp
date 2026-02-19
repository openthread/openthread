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
#include "thread/key_manager.hpp"

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
 * Initial key sequence counter.
 */
static constexpr uint32_t kInitialKeySequence = 127;

/**
 * Next key sequence counter.
 */
static constexpr uint32_t kNextKeySequence = 128;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_6_2(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.6.2 Key Increment of 1 with Roll-over
     *
     * 6.6.2.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     *
     * 6.6.2.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT properly decrypts MAC and MLE packets secured with a Key
     *   Index incremented by 1 (which causes a rollover) and switches to the new key.
     *
     * Spec Reference                  | V1.1 Section | V1.3.0 Section
     * --------------------------------|--------------|---------------
     * MLE Message Security Processing | 7.3.1        | 7.3.1
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
     * - Description: Harness instructs the device to form the network using thrKeySequenceCounter = 0x7F (127).
     * - Pass Criteria: N/A
     */
    leader.AllowList(dut);
    dut.AllowList(leader);

    leader.Form();
    leader.Get<KeyManager>().SetCurrentKeySequence(kInitialKeySequence,
                                                   KeyManager::kForceUpdate | KeyManager::kGuardTimerUnchanged);

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ED_1 / SED_1 (DUT)");

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     * - Description: Attach the DUT to the network.
     * - Pass Criteria:
     *   - The MLE Auxiliary Security Header of the MLE Child ID Request MUST contain:
     *     - Key Source = 0x7F (127)
     *     - Key Index = 0x80 (128)
     *     - Key ID Mode = 2
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

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT. The MAC Auxiliary
     *   security header contains:
     *   - Key Index = 0x80 (128)
     *   - Key ID Mode = 1
     * - Pass Criteria: N/A
     */
    leader.SendEchoRequest(dut.Get<Mle::Mle>().GetLinkLocalAddress(), 0, 0, 64);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED_1 / SED_1 (DUT)");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     * - Description: Automatically replies with ICMPv6 Echo Reply.
     * - Pass Criteria:
     *   - The DUT MUST reply with ICMPv6 Echo Reply.
     *   - The MAC Auxiliary Security Header MUST contain:
     *     - Key Index = 0x80 (128)
     *     - Key ID Mode = 1
     */
    nexus.AdvanceTime(kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Harness instructs the device to increment thrKeySequenceCounter by 1 to force a key switch.
     *   Incoming frame counters shall be set to 0 for all existing devices. All subsequent MLE and MAC frames are sent
     *   with Key Index = 1.
     * - Pass Criteria: N/A
     */
    leader.Get<KeyManager>().SetCurrentKeySequence(kNextKeySequence,
                                                   KeyManager::kForceUpdate | KeyManager::kGuardTimerUnchanged);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT. The MAC Auxiliary
     *   Security Header contains:
     *   - Key Index = 1
     *   - Key ID Mode = 1
     * - Pass Criteria: N/A
     */
    leader.SendEchoRequest(dut.Get<Mle::Mle>().GetLinkLocalAddress(), 1, 0, 64);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: ED_1 / SED_1 (DUT)");

    /**
     * Step 7: ED_1 / SED_1 (DUT)
     * - Description: Automatically replies with ICMPv6 Echo Reply.
     * - Pass Criteria:
     *   - The DUT MUST reply with ICMPv6 Echo Reply.
     *   - The MAC Auxiliary Security Header MUST contain:
     *     - Key Index = 1
     *     - Key ID Mode = 1
     */
    nexus.AdvanceTime(kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_6_2(ot::Nexus::kTopologyA, "test_6_6_2_A.json");
        ot::Nexus::RunTest6_6_2(ot::Nexus::kTopologyB, "test_6_6_2_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest6_6_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_6_2_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest6_6_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_6_2_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
