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

void TestRadioFilter(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("Leader");
    router.SetName("Router");
    sed.SetName("SED");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, sed);

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    // Set poll period for SED to receive replies faster
    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(40));

    Ip6::Address leaderMleid = leader.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address routerMleid = router.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address sedMleid    = sed.Get<Mle::Mle>().GetMeshLocalEid();

    // Validate initial state of `radiofilter` upon start
    VerifyOrQuit(!leader.Get<Mac::Mac>().IsRadioFilterEnabled());
    VerifyOrQuit(!router.Get<Mac::Mac>().IsRadioFilterEnabled());
    VerifyOrQuit(!sed.Get<Mac::Mac>().IsRadioFilterEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Validate connections by pinging");
    nexus.SendAndVerifyEchoRequest(router, leaderMleid);
    nexus.SendAndVerifyEchoRequest(sed, leaderMleid);
    nexus.SendAndVerifyEchoRequest(leader, routerMleid);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Validate behavior when `radiofilter` is enabled on router");
    router.Get<Mac::Mac>().SetRadioFilterEnabled(true);
    VerifyOrQuit(router.Get<Mac::Mac>().IsRadioFilterEnabled());

    nexus.SendAndVerifyNoEchoResponse(leader, routerMleid);
    nexus.SendAndVerifyNoEchoResponse(router, leaderMleid);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Validate behavior when `radiofilter` is disabled on router");
    router.Get<Mac::Mac>().SetRadioFilterEnabled(false);
    VerifyOrQuit(!router.Get<Mac::Mac>().IsRadioFilterEnabled());

    nexus.SendAndVerifyEchoRequest(leader, routerMleid);
    nexus.SendAndVerifyEchoRequest(router, leaderMleid);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Validate `radiofilter` behavior on sed");
    nexus.SendAndVerifyEchoRequest(sed, leaderMleid);

    sed.Get<Mac::Mac>().SetRadioFilterEnabled(true);
    VerifyOrQuit(sed.Get<Mac::Mac>().IsRadioFilterEnabled());

    nexus.SendAndVerifyNoEchoResponse(sed, leaderMleid);

    // Advance time to trigger detach
    nexus.AdvanceTime(300 * 1000);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsDetached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Validate behavior when `radiofilter` is disabled on sed");
    sed.Get<Mac::Mac>().SetRadioFilterEnabled(false);
    VerifyOrQuit(!sed.Get<Mac::Mac>().IsRadioFilterEnabled());

    sed.Join(leader, Node::kAsSed);

    // Advance time to allow reattach
    for (int i = 0; i < 200; i++)
    {
        nexus.AdvanceTime(1000);
        if (sed.Get<Mle::Mle>().IsChild())
        {
            break;
        }
    }
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    nexus.SendAndVerifyEchoRequest(leader, sedMleid);
    nexus.SendAndVerifyEchoRequest(sed, leaderMleid);

    nexus.SaveTestInfo("test_radio_filter.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRadioFilter();
    printf("All tests passed\n");
    return 0;
}
