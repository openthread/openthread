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

void TestMleBlockingDowngrade(void)
{
    // This test validates the MLE router role downgrade logic when a REED is
    // promoted to a router due to a Child ID Request. It verifies that the
    // newly promoted router blocks its own downgrade to ensure the child remains
    // connected, and that this block is correctly lifted when the child detaches
    // or when a new router is introduced to the network (providing an alternative
    // parent for the child).

    static constexpr uint8_t kNumRouters = 10;

    Core  nexus;
    Node &leader    = nexus.CreateNode();
    Node &dut       = nexus.CreateNode();
    Node &child     = nexus.CreateNode();
    Node &newRouter = nexus.CreateNode();
    Node *routers[kNumRouters];

    for (Node *&router : routers)
    {
        router = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNone));

    Log("---------------------------------------------------------------------------------------");
    Log("Form initial topology");

    leader.AllowList(dut);
    dut.AllowList(leader);

    leader.AllowList(newRouter);
    newRouter.AllowList(leader);

    child.AllowList(dut);
    dut.AllowList(child);

    child.AllowList(newRouter);
    newRouter.AllowList(child);

    for (Node *router : routers)
    {
        leader.AllowList(*router);
        router->AllowList(leader);

        dut.AllowList(*router);
        router->AllowList(dut);

        newRouter.AllowList(*router);
        router->AllowList(newRouter);
    }

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (Node *router : routers)
    {
        router->Join(leader);
    }

    nexus.AdvanceTime(300 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (Node *router : routers)
    {
        VerifyOrQuit(router->Get<Mle::Mle>().IsRouter());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Setup DUT - Validate it stays as a REED");

    dut.Get<Mle::Mle>().SetRouterUpgradeThreshold(3);
    dut.Get<Mle::Mle>().SetRouterDowngradeThreshold(4);

    VerifyOrQuit(!dut.Get<Mle::Mle>().IsDowngradeBlocked());

    dut.Join(leader);

    nexus.AdvanceTime(300 * Time::kOneSecondInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsDowngradeBlocked());

    Log("---------------------------------------------------------------------------------------");
    Log("Attach `child` only connecting through the DUT");

    child.Join(dut, Node::kAsFed);

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(child.Get<Mle::Mle>().GetParent().GetExtAddress() == dut.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Validate that DUT is promoted to router and blocking downgrade");

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDowngradeBlocked());

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDowngradeBlocked());

    Log("---------------------------------------------------------------------------------------");
    Log("Disconnect one of the routers - validate that DUT still blocks downgrade");

    routers[0]->Get<Mle::Mle>().Stop();
    routers[0]->Get<ThreadNetif>().Down();

    nexus.AdvanceTime(10);

    VerifyOrQuit(routers[0]->Get<Mle::Mle>().IsDisabled());

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDowngradeBlocked());
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Disconnect the child - validate that DUT can now downgrade");

    child.Get<Mle::Mle>().Stop();
    child.Get<ThreadNetif>().Down();

    nexus.AdvanceTime(10);

    VerifyOrQuit(child.Get<Mle::Mle>().IsDisabled());

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(!dut.Get<Mle::Mle>().IsDowngradeBlocked());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Reconnect the child - validate that DUT once again disallow downgrade");

    child.Get<ThreadNetif>().Up();
    SuccessOrQuit(child.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(30 * Time::kOneSecondInMsec);

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDowngradeBlocked());

    nexus.AdvanceTime(10 * Time::kOneMinuteInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDowngradeBlocked());

    Log("---------------------------------------------------------------------------------------");
    Log("Introduce `newRouter` as a new router - validate DUT now allows downgrade");

    newRouter.Join(leader);

    nexus.AdvanceTime(15 * Time::kOneMinuteInMsec);

    VerifyOrQuit(newRouter.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsDowngradeBlocked());

    Log("---------------------------------------------------------------------------------------");
    Log("Validate that DUT is downgraded to REED and child connected to the new router");

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(child.Get<Mle::Mle>().GetParent().GetExtAddress() == newRouter.Get<Mac::Mac>().GetExtAddress());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMleBlockingDowngrade();
    printf("All tests passed\n");
    return 0;
}
