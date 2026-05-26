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
 * Time to advance for a node to join as a SSED.
 */
static constexpr uint32_t kAttachAsSsedTime = 20 * 1000;

/**
 * CSL Period in milliseconds.
 */
static constexpr uint32_t kCslPeriodMs = 100;

/**
 * CSL Period in units of 10 symbols.
 */
static constexpr uint32_t kCslPeriod = kCslPeriodMs * 1000 / OT_US_PER_TEN_SYMBOLS;

/**
 * Time to advance for CSL synchronization to complete, in milliseconds.
 */
static constexpr uint32_t kCslSyncTime = 5 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void TestRetransmissionSecurity(void)
{
    /**
     * Retransmission Security Test
     *
     * Topology:
     * - Leader (DUT)
     * - SSED_1
     *
     * Purpose:
     * Validate that retransmitted frames with CSL IE use an incremented frame counter
     * and are correctly encrypted for each attempt.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &ssed1  = nexus.CreateNode();

    leader.SetName("LEADER");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Network Formation");

    AllowLinkBetween(leader, ssed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    ssed1.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());

    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);
    nexus.AdvanceTime(kCslSyncTime);
    VerifyOrQuit(ssed1.Get<Mac::Mac>().IsCslEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Trigger Retransmissions (SSED to Leader)");

    // Turn off leader radio so it misses SSED frames and SSED retries
    SuccessOrQuit(otPlatRadioSleep(&leader.GetInstance()));

    ssed1.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), 0x1234, kEchoPayloadSize);

    // Advance time enough for multiple retries.
    nexus.AdvanceTime(2000);

    // Turn leader radio back on
    SuccessOrQuit(otPlatRadioReceive(&leader.GetInstance(), leader.Get<Mac::Mac>().GetPanChannel()));

    nexus.SaveTestInfo("test_retransmission_security.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRetransmissionSecurity();
    printf("All tests passed\n");
    return 0;
}
