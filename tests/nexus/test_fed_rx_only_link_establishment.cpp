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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static uint8_t sRxOnlyNeighborCount = 0;

static void HandleNeighborTableChanged(otNeighborTableEvent aEvent, const otNeighborTableEntryInfo *aEntryInfo)
{
    VerifyOrQuit(static_cast<NeighborTable::Event>(aEvent) == NeighborTable::kRouterAdded);
    VerifyOrQuit(aEntryInfo != nullptr);

    Log("   Neighbor Table Event ROUTER_ADDED -> rloc16: 0x%04x", aEntryInfo->mInfo.mRouter.mRloc16);
    sRxOnlyNeighborCount++;
}

void TestFedRxOnlyLinkEstablishment(void)
{
    static constexpr uint16_t kNumRouters = 15;

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];
    Node *fed;

    Log("---------------------------------------------------------------------------------------");
    Log("TestFedRxOnlyLinkEstablishment");

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form topology - leader + %u routers", kNumRouters);

    leader.Form();
    nexus.AdvanceTime(100 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        Node &router = nexus.CreateNode();

        router.Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(300 * Time::kOneSecondInMsec);
        VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
        routers[i] = &router;

        Log("Router %-2u -> rloc16: 0x%04x", i, router.Get<Mle::Mle>().GetRloc16());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check Active Router Count on all routers");

    nexus.AdvanceTime(300 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<RouterTable>().GetActiveRouterCount() == kNumRouters + 1);

    for (const Node *router : routers)
    {
        VerifyOrQuit(router->Get<RouterTable>().GetActiveRouterCount() == kNumRouters + 1);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Add an FED child to the network");

    fed = &nexus.CreateNode();
    fed->Get<NeighborTable>().RegisterCallback(HandleNeighborTableChanged);

    fed->Join(leader, Node::kAsFed);
    nexus.AdvanceTime(2 * Time::kOneSecondInMsec);
    VerifyOrQuit(fed->Get<Mle::Mle>().IsChild());

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check that the FED successfully establishes link (rx-only) with all neighboring routers");

    VerifyOrQuit(fed->Get<RouterTable>().GetActiveRouterCount() == kNumRouters + 1);

    for (uint16_t i = 0; i < 45; i++)
    {
        nexus.AdvanceTime(2 * Time::kOneMinuteInMsec);

        if (sRxOnlyNeighborCount == kNumRouters)
        {
            break;
        }
    }

    VerifyOrQuit(sRxOnlyNeighborCount == kNumRouters);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFedRxOnlyLinkEstablishment();
    printf("All tests passed\n");

    return 0;
}
