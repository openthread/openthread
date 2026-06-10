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
#include "thread/mle_types.hpp"
#include "thread/router_table.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Link establishment delay threshold (in milliseconds).
 * test_router_multicast_link_request.py uses 3 seconds + 3 seconds buffer.
 */
static constexpr uint32_t kLinkEstablishDelay = 6 * 1000;

static void CheckLink(Node &aNodeA, Node &aNodeB)
{
    Router::Info info;
    uint8_t      bRouterId = Mle::RouterIdFromRloc16(aNodeB.Get<Mle::Mle>().GetRloc16());
    uint8_t      aRouterId = Mle::RouterIdFromRloc16(aNodeA.Get<Mle::Mle>().GetRloc16());

    Log("Checking link between %s (ID %d) and %s (ID %d)", aNodeA.GetName(), aRouterId, aNodeB.GetName(), bRouterId);

    SuccessOrQuit(aNodeA.Get<RouterTable>().GetRouterInfo(bRouterId, info));
    VerifyOrQuit(info.mLinkEstablished);

    SuccessOrQuit(aNodeB.Get<RouterTable>().GetRouterInfo(aRouterId, info));
    VerifyOrQuit(info.mLinkEstablished);
}

void TestRouterMulticastLinkRequest(const char *aJsonFileName)
{
    /**
     * This test verifies if a device can quickly and efficiently establish links with
     * neighboring routers by sending a multicast Link Request message after becoming
     * a router.
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &reed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");
    reed.SetName("REED");

    // Topology:
    // Leader is connected to Router1, Router2, Router3.
    // REED is connected to Router1, Router2, Router3.
    AllowLinkBetween(leader, router1);
    AllowLinkBetween(leader, router2);
    AllowLinkBetween(leader, router3);
    AllowLinkBetween(router1, reed);
    AllowLinkBetween(router2, reed);
    AllowLinkBetween(router3, reed);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader forms network");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Routers join the leader");
    router1.Join(leader);
    router2.Join(leader);
    router3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Wait for network to stabilize");
    nexus.AdvanceTime(60 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: REED joins and upgrades to router");
    // REED joins through router1
    reed.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify REED establishes links with all neighboring routers");
    nexus.AdvanceTime(kLinkEstablishDelay);

    CheckLink(reed, router1);
    CheckLink(reed, router2);
    CheckLink(reed, router3);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFileName = (argc > 2) ? argv[2] : "test_router_multicast_link_request.json";

    ot::Nexus::TestRouterMulticastLinkRequest(jsonFileName);
    printf("All tests passed\n");
    return 0;
}
