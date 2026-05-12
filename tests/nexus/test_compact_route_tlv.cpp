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

void TestCompactRouteTlv(void)
{
    static constexpr uint16_t kNumRouters = 31;

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];

    Log("---------------------------------------------------------------------------------------");
    Log("TestCompactRouteTlv");

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form topology - leader + %u routers", kNumRouters);

    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    leader.Get<Mle::Mle>().SetRouterDowngradeThreshold(33);

    leader.Form();
    nexus.AdvanceTime(100 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        Node &router = nexus.CreateNode();

        routers[i] = &router;

        router.Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
        router.Get<Mle::Mle>().SetRouterDowngradeThreshold(33);
        router.Get<Mle::Mle>().SetRouterSelectionJitter(20);

        router.Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

        VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

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
    Log("Reset `routers[0]` and validate that it is restored quickly");

    routers[0]->Reset();

    routers[0]->Get<Mle::Mle>().SetRouterUpgradeThreshold(32);
    routers[0]->Get<Mle::Mle>().SetRouterDowngradeThreshold(33);
    routers[0]->Get<Mle::Mle>().SetRouterSelectionJitter(20);

    routers[0]->Get<ThreadNetif>().Up();
    SuccessOrQuit(routers[0]->Get<Mle::Mle>().Start());

    for (uint16_t i = 0; i < 5000; i++)
    {
        nexus.AdvanceTime(1);

        if (routers[0]->Get<Mle::Mle>().IsRouter())
        {
            Log("Node `routers[0]` restored it role after reset");
            Log("Validate that it got a compact Route TLV and therefore does not yet see all routers");
            VerifyOrQuit(routers[0]->Get<RouterTable>().GetActiveRouterCount() < kNumRouters + 1);
            break;
        }

        VerifyOrQuit(routers[0]->Get<Mle::Mle>().IsDetached());
    }

    VerifyOrQuit(routers[0]->Get<Mle::Mle>().IsRouter());

    Log("Validate that `routes[0]` discovered all routers");

    nexus.AdvanceTime(100 * Time::kOneSecondInMsec);

    VerifyOrQuit(routers[0]->Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(routers[0]->Get<RouterTable>().GetActiveRouterCount() == kNumRouters + 1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCompactRouteTlv();
    printf("All tests passed\n");

    return 0;
}
