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
 * Time to advance for a node to join as a SSED.
 */
static constexpr uint32_t kAttachAsSsedTime = 20 * 1000;

/**
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * CSL synchronized timeout in seconds.
 */
static constexpr uint32_t kCslTimeout10s = 10;
static constexpr uint32_t kCslTimeout20s = 20;
static constexpr uint32_t kCslTimeout30s = 30;

/**
 * Wait times as specified in the test procedure.
 */
static constexpr uint32_t kWaitTime35s = 35 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Radio channels.
 */
static constexpr uint8_t kPrimaryChannel   = 11;
static constexpr uint8_t kSecondaryChannel = 26;
static constexpr uint8_t kRandomChannel2   = 12;
static constexpr uint8_t kRandomChannel3   = 13;
static constexpr uint8_t kRandomChannel4   = 14;
static constexpr uint8_t kRandomChannel5   = 15;

/**
 * Echo request identifiers.
 */
static constexpr uint16_t kEchoIdentifierStep3 = 0x1234;
static constexpr uint16_t kEchoIdentifierStep6 = 0x5678;

void Test1_2_LP_5_3_5(void)
{
    /**
     * 5.3.5 Minimum number of SSED Support
     *
     * 5.3.5.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - SSED_1
     * - SSED_2
     * - SSED_3
     * - SSED_4
     * - SSED_5
     * - SSED_6
     *
     * 5.3.5.2 Purpose and Description
     * The purpose of this test is to verify that a Router can reliably support
     *   a minimum of 6 SSED children simultaneously that are each using a
     *   different CSL channel.
     *
     * - SSED_1 and SSED_2 are each configured with a CSL Synchronized
     *   Timeout of 10 seconds.
     * - SSED_3 and SSED_4 are each configured with a CSL Synchronized
     *   Timeout of 20 seconds.
     * - SSED_5 and SSED_6 are each configured with a CSL Synchronized
     *   Timeout of 30 seconds.
     *
     * SSED 1 and 6 are configured to use the Primary and Secondary harness
     *   channels, respectively. The other four SSEDs are configured to run
     *   on another four available random channels. No over-the-air
     *   captures are generated for these four SSEDs.
     *
     * SPEC Section: 3.2.6.3.2
     */

    Core nexus;

    static constexpr uint8_t kNumSseds = 6;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node *sseds[kNumSseds];

    leader.SetName("LEADER");
    router1.SetName("DUT");

    for (int i = 0; i < kNumSseds; i++)
    {
        sseds[i] = &nexus.CreateNode();
        sseds[i]->SetName("SSED", i + 1);
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: SSED_1-6");

    /**
     * Step 0: SSED_1-6
     * - Description: Preconditions:
     *   - Set CSL Period = 500ms
     *   - SSED_1, _2: Set CSL Synchronized Timeout = 10s
     *   - SSED_3, _4: Set CSL Synchronized Timeout = 20s
     *   - SSED_5, _6: Set CSL Synchronized Timeout = 30s
     * - Pass Criteria: N/A
     */

    // Target parameters as specified in Step 0
    static constexpr uint32_t kCslTimeouts[] = {kCslTimeout10s, kCslTimeout10s, kCslTimeout20s,
                                                kCslTimeout20s, kCslTimeout30s, kCslTimeout30s};
    static constexpr uint8_t  kCslChannels[] = {kPrimaryChannel, kRandomChannel2, kRandomChannel3,
                                                kRandomChannel4, kRandomChannel5, kSecondaryChannel};

    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology formation: DUT, SSED_1, SSED_2, SSED_3,
     *   SSED_4, SSED_5, SSED_6.
     * - Pass Criteria: N/A
     */

    leader.AllowList(router1);
    router1.AllowList(leader);

    for (Node *ssed : sseds)
    {
        router1.AllowList(*ssed);
        ssed->AllowList(router1);
    }

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    // Join SSEDs.
    // To satisfy criterion 2.2 (Child Update Response for SSED_1), SSED_1 joins with defaults first,
    // and then we update its parameters after it attaches.
    // Others join with their target parameters directly to establish sync during Child ID exchange.

    for (int i = 0; i < kNumSseds; i++)
    {
        if (i > 0)
        {
            // For SSED_2-6, apply target parameters before joining so they are used in Child ID Request
            sseds[i]->Get<Mac::Mac>().SetCslPeriod(kCslPeriod500ms);
            sseds[i]->Get<Mle::Mle>().SetCslTimeout(kCslTimeouts[i]);
            sseds[i]->Get<Mac::Mac>().SetCslChannel(kCslChannels[i]);
        }

        sseds[i]->Join(router1, Node::kAsSed);
    }

    Log("Step 2: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6");

    /**
     * Step 2: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
     * - Description: Each device automatically attaches to the DUT and
     *   establishes CSL synchronization.
     * - Pass Criteria:
     *   - 2.1: The DUT MUST unicast MLE Child ID Response to SSED_1.
     *   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
     *   - 2.3: The DUT MUST unicast MLE Child ID Response to SSED_2.
     *   - 2.4: The DUT MUST unicast MLE Child ID Response to SSED_3.
     *   - 2.5: The DUT MUST unicast MLE Child ID Response to SSED_4.
     *   - 2.6: The DUT MUST unicast MLE Child ID Response to SSED_5.
     *   - 2.7: The DUT MUST unicast MLE Child ID Response to SSED_6.
     */

    // Initial attach for all SSEDs
    nexus.AdvanceTime(kAttachAsSsedTime);

    for (Node *ssed : sseds)
    {
        VerifyOrQuit(ssed->Get<Mle::Mle>().IsAttached());
    }

    // Now update SSED_1 to trigger MLE Child Update Request/Response (Criterion 2.2)
    sseds[0]->Get<Mac::Mac>().SetCslPeriod(kCslPeriod500ms);
    sseds[0]->Get<Mle::Mle>().SetCslTimeout(kCslTimeouts[0]);
    sseds[0]->Get<Mac::Mac>().SetCslChannel(kCslChannels[0]);

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness verifies connectivity by instructing the
     *   device to send an ICMPv6 Echo Request to each SSED mesh-local
     *   address.
     * - Pass Criteria:
     *   - 3.1: The DUT MUST forward the ICMPv6 Echo Requests to SSED_1
     *     and SSED_6 on the correct channel.
     *   - 3.2: SSED_1 MUST NOT send a MAC Data Request prior to
     *     receiving the ICMPv6 Echo Request from the Leader.
     */

    auto sendEchoRequests = [&](uint16_t aIdentifier) {
        for (Node *ssed : sseds)
        {
            leader.SendEchoRequest(ssed->Get<Mle::Mle>().GetMeshLocalEid(), aIdentifier, kEchoPayloadSize);
            nexus.AdvanceTime(1000);
        }
    };

    sendEchoRequests(kEchoIdentifierStep3);

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 4: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6");

    /**
     * Step 4: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
     * - Description: Each device automatically replies with ICMPv6 Echo
     *   Reply. The CSL unsynchronized timer on the DUT should be reset
     *   to 0.
     * - Pass Criteria:
     *   - 4.1: The 802.15.4 Frame Headers for the SSED_1 and SSED_6
     *     ICMPv6 Echo Replies MUST include the CSL Period IE and CSL
     *     Phase IE.
     *   - 4.2: The DUT MUST forward an ICMPv6 Echo Reply from SSED_1.
     *   - 4.3: The DUT MUST forward an ICMPv6 Echo Reply from SSED_2.
     *   - 4.4: The DUT MUST forward an ICMPv6 Echo Reply from SSED_3.
     *   - 4.5: The DUT MUST forward an ICMPv6 Echo Reply from SSED_4.
     *   - 4.6: The DUT MUST forward an ICMPv6 Echo Reply from SSED_5.
     *   - 4.7: The DUT MUST forward an ICMPv6 Echo Reply from SSED_6.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 5: Harness");

    /**
     * Step 5: Harness
     * - Description: Harness waits for 35 seconds.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWaitTime35s);

    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness verifies connectivity by instructing the
     *   device to send an ICMPv6 Echo Request to each SSED mesh-local
     *   address.
     * - Pass Criteria:
     *   - 6.1: The DUT MUST forward the ICMPv6 Echo Requests to SSED_1
     *     and SSED_6 on the correct channel.
     *   - 6.2: SSED_1 MUST NOT send a MAC Data Request prior to
     *     receiving the ICMPv6 Echo Request from the Leader.
     */

    sendEchoRequests(kEchoIdentifierStep6);

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 7: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6");

    /**
     * Step 7: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
     * - Description: Each device automatically replies with ICMPv6 Echo
     *   Reply. The CSL unsynchronized timer on the DUT should be reset
     *   to 0.
     * - Pass Criteria:
     *   - 7.1: The 802.15.4 Frame Headers for the SSED_1 and SSED_6
     *     ICMPv6 Echo Replies MUST include the CSL Period IE and CSL
     *     Phase IE.
     *   - 7.2: The DUT MUST forward an ICMPv6 Echo Reply from SSED_1.
     *   - 7.3: The DUT MUST forward an ICMPv6 Echo Reply from SSED_2.
     *   - 7.4: The DUT MUST forward an ICMPv6 Echo Reply from SSED_3.
     *   - 7.5: The DUT MUST forward an ICMPv6 Echo Reply from SSED_4.
     *   - 7.6: The DUT MUST forward an ICMPv6 Echo Reply from SSED_5.
     *   - 7.7: The DUT MUST forward an ICMPv6 Echo Reply from SSED_6.
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_1_2_LP_5_3_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_LP_5_3_5();
    printf("All tests passed\n");
    return 0;
}
