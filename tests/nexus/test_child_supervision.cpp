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
#include "thread/child_supervision.hpp"
#include "thread/child_table.hpp"

namespace ot {
namespace Nexus {

void TestChildSupervision(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));

    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Check initial supervision interval");

    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == sed.Get<SupervisionListener>().GetInterval());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Change supervision interval on child");

    sed.Get<SupervisionListener>().SetInterval(20);
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetInterval() == 20);
    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == 20);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Change supervision check timeout on the child");

    sed.Get<SupervisionListener>().SetTimeout(25);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetTimeout() == 25);

    // Wait for multiple supervision intervals and ensure that child stays attached
    nexus.AdvanceTime(110 * 1000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Disable supervision check on child and block traffic from child on parent");

    sed.Get<SupervisionListener>().SetTimeout(0);

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    // sed is not in allowlist, so it will be blocked.

    uint32_t childTimeout = leader.Get<ChildTable>().GetChildAtIndex(0)->GetTimeout();

    nexus.AdvanceTime((childTimeout + 1) * 1000);
    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == 0);

    // Since supervision check is disabled on the child, it should continue to stay attached (since data polls are acked
    // by radio driver).
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Re-enable supervision check on child");

    sed.Get<SupervisionListener>().SetTimeout(25);

    nexus.AdvanceTime(35 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Disable allowlist on parent and re-attach");

    leader.Get<Mac::Filter>().SetMode(Mac::Filter::kModeRssInOnly);
    nexus.AdvanceTime(30 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    sed.Get<SupervisionListener>().ResetCounter();
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Set the supervision interval to zero on child");

    sed.Get<SupervisionListener>().SetInterval(0);
    sed.Get<SupervisionListener>().SetTimeout(25);
    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(sed.Get<SupervisionListener>().GetInterval() == 0);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetTimeout() == 25);

    {
        Child *child = leader.Get<ChildTable>().GetChildAtIndex(0);
        VerifyOrQuit(child != nullptr);
        VerifyOrQuit(child->GetSupervisionInterval() == 0);
    }

    // Wait for multiple check timeouts. The child should still stay attached.
    nexus.AdvanceTime(100 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == 1);
    VerifyOrQuit(sed.Get<SupervisionListener>().GetCounter() > 0);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildSupervision();
    printf("All tests passed\n");
    return 0;
}
