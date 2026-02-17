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
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 32 * 1000;

/**
 * Initial key sequence counter value.
 */
static constexpr uint32_t kInitialKeySequence = 0;

void Test5_8_2(void)
{
    /**
     * 5.8.2 Key Increment Of 1
     *
     * 5.8.2.1 Topology
     * - Leader
     * - Router_1 (DUT)
     *
     * 5.8.2.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT properly decrypts MAC and MLE packets secured with a key
     *   index incremented by 1 and switches to the new key.
     *
     * Spec Reference                  | V1.1 Section | V1.3.0 Section
     * --------------------------------|--------------|---------------
     * MLE Message Security Processing | 7.3.1        | 7.3.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader forms the network");

    /**
     * Step 1: Leader
     * - Description: Forms the network. Starts the network using KeySequenceCounter = 0x00 (0).
     * - Pass Criteria: N/A
     */
    leader.Get<KeyManager>().SetCurrentKeySequence(kInitialKeySequence, KeyManager::kForceUpdate);
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1 (DUT) attaches to the network");

    /**
     * Step 2: Router_1 (DUT)
     * - Description: Automatically attaches to the network.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request with MLE Auxiliary Security Header containing:
     *     - Key ID Mode = 0x02 (2)
     *     - Key Source = 0x00 (0)
     *     - Key Index = 0x01 (1)
     *   - The DUT MUST send MLE Child ID Request with MLE Auxiliary Security Header containing:
     *     - Key ID Mode = 0x02 (2)
     *     - Key Source = 0x00 (0)
     *     - Key Index = 0x01 (1)
     */
    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader sends ICMPv6 Echo Request to the DUT");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply with MAC Auxiliary Security Header containing:
     *     - Key ID Mode = 0x01 (1)
     *     - Key Index = 0x01 (1)
     */
    nexus.SendAndVerifyEchoRequest(leader, router1.Get<Mle::Mle>().GetLinkLocalAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader increments KeySequenceCounter by 1 to force a key switch");

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to increment KeySequenceCounter by 1 to force a key switch. The DUT
     *   is expected to set incoming frame counters to 0 for all existing devices and send subsequent MAC and MLE frames
     *   with Key Index = 2.
     * - Pass Criteria: N/A
     */
    leader.Get<KeyManager>().SetCurrentKeySequence(leader.Get<KeyManager>().GetCurrentKeySequence() + 1,
                                                   KeyManager::kForceUpdate);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader sends ICMPv6 Echo Request to the DUT");

    /**
     * Step 5: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT.
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply with MAC Auxiliary security header containing:
     *     - Key ID Mode = 0x01 (1)
     *     - Key Index = 0x02 (2)
     */
    nexus.SendAndVerifyEchoRequest(leader, router1.Get<Mle::Mle>().GetLinkLocalAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1 (DUT) automatically reflects the Key Index update in its Advertisements");

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically reflects the Key Index update in its Advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Advertisements with MLE Auxiliary security header containing:
     *     - Key ID Mode = 0x02 (2)
     *     - Key Index = 0x02 (2)
     */
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_8_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_8_2();
    printf("All tests passed\n");
    return 0;
}
