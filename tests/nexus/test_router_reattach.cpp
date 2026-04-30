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
static constexpr uint32_t kAttachToRouterTime = 30 * 1000;

void TestRouterReattach(void)
{
    Core          nexus;
    const uint8_t kNumRouters = 32;
    Node         *nodes[kNumRouters];

    Log("Creating %u nodes", kNumRouters);
    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        nodes[i] = &nexus.CreateNode();
        nodes[i]->SetName("Node", i + 1);
    }

    nexus.AdvanceTime(0);

    // Step 1: Start Leader
    Log("Step 1: Starting Leader");
    nodes[0]->Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    nodes[0]->Get<Mle::Mle>().SetRouterDowngradeThreshold(32);
    nodes[0]->Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(nodes[0]->Get<Mle::Mle>().IsLeader());

    // Step 2: Start other 31 routers
    Log("Step 2: Starting other 31 routers");
    for (uint8_t i = 1; i < kNumRouters; i++)
    {
        nodes[i]->Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
        nodes[i]->Get<Mle::Mle>().SetRouterDowngradeThreshold(32);
        VerifyOrQuit(nodes[i]->Get<Mle::Mle>().SetRouterSelectionJitter(1) == kErrorNone);
        nodes[i]->Join(*nodes[0]);
        nexus.AdvanceTime(kAttachToRouterTime);
        Log("Node %u role: %s", i + 1, nodes[i]->GetExtendedRoleString());
        VerifyOrQuit(nodes[i]->Get<Mle::Mle>().IsRouter());
    }

    // Step 3: Reset Node 2 (index 1)
    Log("Step 3: Resetting Node 2");
    nodes[1]->Reset();
    nodes[1]->Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    nodes[1]->Get<Mle::Mle>().SetRouterDowngradeThreshold(32);
    VerifyOrQuit(nodes[1]->Get<Mle::Mle>().SetRouterSelectionJitter(3) == kErrorNone);

    // Re-enable and start
    Log("Step 4: Restarting Node 2");
    nodes[1]->Get<ThreadNetif>().Up();
    SuccessOrQuit(nodes[1]->Get<Mle::Mle>().Start());
    VerifyOrQuit(nodes[1]->Get<Mle::Mle>().GetRouterDowngradeThreshold() == 32);

    // Verify it restores as Router
    Log("Step 5: Verifying Node 2 restores as Router");
    nexus.AdvanceTime(1000);
    VerifyOrQuit(nodes[1]->Get<Mle::Mle>().IsRouter());

    // Verify it doesn't downgrade after Router Selection Jitter
    Log("Step 6: Verifying Node 2 does not downgrade");
    nexus.AdvanceTime(5000);
    VerifyOrQuit(nodes[1]->Get<Mle::Mle>().IsRouter());

    nexus.SaveTestInfo("test_router_reattach.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRouterReattach();
    printf("All tests passed\n");
    return 0;
}
