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
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 5 * 1000;

/**
 * The number of pings to send to advance the frame counter beyond the storage threshold.
 */
static constexpr uint16_t kNumPings = 1010;

void TestReset(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &ed     = nexus.CreateNode();

    leader.SetName("Leader");
    router.SetName("Router");
    ed.SetName("ED");

    nexus.AdvanceTime(0);

    // Set up allowlists to enforce Leader <-> Router <-> ED topology
    AllowLinkBetween(leader, router);
    AllowLinkBetween(router, ed);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Forming network");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    ed.Join(router, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(ed.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(10 * 1000); // Extra time to stabilize

    const Ip6::Address &leaderRloc = leader.Get<Mle::Mle>().GetMeshLocalRloc();

    Log("---------------------------------------------------------------------------------------");
    Log("Sending %u pings from ED to Leader", kNumPings);

    for (uint16_t i = 0; i < kNumPings; i++)
    {
        // Use a 500ms timeout for each ping to ensure it has enough time in a multi-hop setup
        nexus.SendAndVerifyEchoRequest(ed, leaderRloc, 0, Ip6::kDefaultHopLimit, 500);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Resetting nodes sequentially");

    Log("Resetting Leader");
    leader.Reset();
    AllowLinkBetween(leader, router);
    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Resetting Router");
    router.Reset();
    AllowLinkBetween(router, leader);
    AllowLinkBetween(router, ed);
    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("Resetting ED");
    ed.Reset();
    AllowLinkBetween(ed, router);
    ed.Get<ThreadNetif>().Up();
    SuccessOrQuit(ed.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(ed.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(10 * 1000); // Extra time to stabilize

    Log("---------------------------------------------------------------------------------------");
    Log("Verifying connectivity after resets");

    // The success of this ping after resets is critical.
    // Since we sent 1010 pings before resets, the frame counters on all nodes
    // have advanced significantly. If a node fails to recover its frame counter
    // from settings after reset, it would start at 0, and its packets would be
    // rejected by its neighbors.
    nexus.SendAndVerifyEchoRequest(ed, leaderRloc);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestReset();
    printf("All tests passed\n");
    return 0;
}
