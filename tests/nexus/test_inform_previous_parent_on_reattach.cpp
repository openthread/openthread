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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * The purpose of this test is to verify a SED will inform its previous parent when re-attaches to another parent.
 *
 * Initial Topology:
 *
 *  LEADER ----- ROUTER
 *    |
 *   SED
 *
 */

void RunTest(const char *aJsonFileName)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network with LEADER and ROUTER");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, sed);

    leader.Form();
    nexus.AdvanceTime(13000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(200000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SED joins LEADER");

    sed.Get<Mle::Mle>().SetTimeout(60); // 60 seconds
    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(20000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Switch links (Block LEADER <-> SED, Allow ROUTER <-> SED)");

    UnallowLinkBetween(leader, sed);
    AllowLinkBetween(router, sed);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: SED re-attaches to ROUTER");

    // We wait for SED to realize LEADER is gone and re-attach to ROUTER.
    // Child timeout is 60s. We wait 100s.
    nexus.AdvanceTime(100000);

    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().GetParent().GetExtAddress() == router.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify LEADER's child table");

    // SED will re-attach to ROUTER, which will inform LEADER to remove the child.
    VerifyOrQuit(leader.Get<ChildTable>().FindChild(sed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) ==
                 nullptr);

    nexus.SaveTestInfo(aJsonFileName, &leader);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::RunTest(argc > 2 ? argv[2] : "test_inform_previous_parent_on_reattach.json");
    printf("All tests passed\n");
    return 0;
}
