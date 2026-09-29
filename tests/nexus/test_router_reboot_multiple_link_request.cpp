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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance after Router reset to allow it to send all link requests.
 *
 * Router transitions must account for a potential jitter of up to 2 minutes (120 seconds).
 */
static constexpr uint32_t kResetRestoringTime = 120 * 1000;

void TestRouterRebootMultipleLinkRequest(void)
{
    /**
     * Test Router Reboot Multiple Link Request
     *
     * Topology
     * - Leader
     * - Router
     * - MED (attached to Router)
     *
     * Purpose & Description
     * The purpose of this test case is to show that when a router is rebooted, it sends
     * MLE_MAX_RESTORING_TRANSMISSION_COUNT MLE link request packets if no response is received.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    med.SetName("MED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("Step 1: Form topology");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(router, med);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    med.Join(router, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().GetParent().GetExtAddress() == router.Get<Mac::Mac>().GetExtAddress());

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 2: Reset Router and isolate it from Leader");

    router.Reset();

    // Isolate Router from Leader so it doesn't receive Link Accept
    UnallowLinkBetween(leader, router);

    Log("Step 3: Start Router and wait for it to send multiple Link Requests");

    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kResetRestoringTime);

    nexus.SaveTestInfo("test_router_reboot_multiple_link_request.json", &leader);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRouterRebootMultipleLinkRequest();
    printf("All tests passed\n");
    return 0;
}
