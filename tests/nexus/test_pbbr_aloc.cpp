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

/** Delay in milliseconds for node startup/joining */
static constexpr uint32_t kNodeJoinDelay = 20 * 1000;
/** Delay in milliseconds for network to stabilize and PBBR to become primary */
static constexpr uint32_t kStabilizationDelay = 300 * 1000;

void TestPbbrAloc(void)
{
    // Topology:
    //   PBBR -- LEADER -- ROUTER

    Core nexus;

    Node &pbbr   = nexus.CreateNode();
    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    pbbr.SetName("PBBR");
    leader.SetName("LEADER");
    router.SetName("ROUTER");

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and enable PBBR");

    AllowLinkBetween(pbbr, leader);
    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kNodeJoinDelay);

    pbbr.Join(leader);
    pbbr.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kNodeJoinDelay);

    router.Join(leader);
    nexus.AdvanceTime(kNodeJoinDelay);

    // Wait for network to stabilize and PBBR to become primary
    nexus.AdvanceTime(kStabilizationDelay);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(pbbr.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(pbbr.Get<BackboneRouter::Local>().IsPrimary());

    Log("---------------------------------------------------------------------------------------");
    Log("Verify ALOC connectivity");

    Ip6::Address aloc;

    // 1. Leader ALOC
    leader.Get<Mle::Mle>().GetLeaderAloc(aloc);
    Log("Pinging Leader ALOC %s from ROUTER", aloc.ToString().AsCString());
    nexus.SendAndVerifyEchoRequest(router, aloc);

    // 2. PBBR ALOC
    aloc.SetPrefix(leader.Get<Mle::Mle>().GetMeshLocalPrefix());
    aloc.GetIid().SetToLocator(Mle::Aloc16::ForPrimaryBackboneRouter());
    Log("Pinging PBBR ALOC %s from ROUTER", aloc.ToString().AsCString());
    nexus.SendAndVerifyEchoRequest(router, aloc);

    Log("All tests passed");

    nexus.SaveTestInfo("test_pbbr_aloc.json", &leader);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestPbbrAloc();
    return 0;
}
