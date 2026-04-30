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

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

void TestLongRoutes(void)
{
    static constexpr uint16_t kNumRouters = 25;

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];

    Log("---------------------------------------------------------------------------------------");
    Log("TestLongRoutes");

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Form topology - long chain of routers");

    leader.Form();
    nexus.AdvanceTime(100 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    routers[0] = &nexus.CreateNode();
    AllowLinkBetween(leader, *routers[0]);
    routers[0]->Join(leader, Node::kAsFtd);

    for (uint16_t i = 1; i < kNumRouters; i++)
    {
        Log("Router %u", i);

        routers[i] = &nexus.CreateNode();
        AllowLinkBetween(*routers[i], *routers[i - 1]);
        routers[i]->Join(*routers[i - 1], Node::kAsFtd);
        nexus.AdvanceTime(20 * 1000);

        VerifyOrQuit(routers[i - 1]->Get<Mle::Mle>().IsRouter());
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Check the path cost from last router to leader");

    VerifyOrQuit(routers[kNumRouters - 1]->Get<RouterTable>().GetPathCostToLeader() == kNumRouters);
}

#endif // OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    ot::Nexus::TestLongRoutes();
    printf("All tests passed\n");
#else
    printf("OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE is not enabled, test is skipped\n");
#endif
    return 0;
}
