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
 * The purpose of this test is to verify that when a FED switches to a better parent while its previous parent is
 * still in range, the previous parent removes the FED from its child table well before the child timeout.
 *
 * A FED does not register its addresses with its parent, so the previous parent cannot use the empty IP message
 * from `InformPreviousParent()` to find the child entry. If the stale entry stays, the previous parent keeps treating
 * the FED's old RLOC16 as reachable and silently drops traffic routed to it by stale EID-to-RLOC cache entries.
 *
 * A Link Request from the FED would also make the previous parent drop the stale entry, so whether that happens would
 * depend on advertisement timing. To make the outcome deterministic, the FED's child router links are set to zero, so
 * that it only uses gradual router link establishment, which needs LQ2 or better and waits 5 minutes after attaching.
 * LEADER is at LQ1 at the FED.
 *
 * Topology (link quality at FED is set by an RSS filter on FED):
 *
 *  LEADER, ROUTER_1 : linked to each other
 *  FED              : linked to LEADER (LQ1 from Step 3); linked to ROUTER_1 only from Step 3;
 *                     stops hearing LEADER from Step 5 (LEADER still hears FED);
 *                     after its reset in Step 7, hears only LEADER (ROUTER_1 still hears FED)
 */

static constexpr int8_t kWeakRss = -92; // Link margin 8 dB with the nexus radio's -100 dBm sensitivity (LQ1).

void TestFedInformPreviousParentOnReattach(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &fed     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    fed.SetName("FED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network with LEADER and ROUTER_1");

    AllowLinkBetween(leader, router1);
    AllowLinkBetween(leader, fed);

    leader.Form();
    nexus.AdvanceTime(13000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(200000);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: FED joins LEADER");

    SuccessOrQuit(fed.Get<Mle::Mle>().SetChildRouterLinks(0));
    fed.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(120000);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(leader.Get<ChildTable>().FindChild(fed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) !=
                 nullptr);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: LEADER becomes weak and ROUTER_1 strong at FED; FED searches for a better parent");

    AllowLinkBetween(router1, fed);
    SuccessOrQuit(fed.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kWeakRss));

    SuccessOrQuit(fed.Get<Mle::Mle>().SearchForBetterParent());

    for (uint32_t elapsed = 0; elapsed < 10000; elapsed += 100)
    {
        nexus.AdvanceTime(100);

        if (fed.Get<Mle::Mle>().IsChild() &&
            fed.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress())
        {
            break;
        }
    }

    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Verify LEADER removed FED from its child table before the child timeout");

    // The FED uses the default child timeout (240 s), so a removal within a few seconds
    // can only come from the FED informing its previous parent. The FED still hears LEADER, so it also receives
    // and rejects the Child Update Response from LEADER, which is no longer its parent.
    VerifyOrQuit(fed.Get<Mle::Mle>().GetTimeout() > 60);

    nexus.AdvanceTime(5000);

    VerifyOrQuit(leader.Get<ChildTable>().FindChild(fed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) ==
                 nullptr);

    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(router1.Get<ChildTable>().FindChild(fed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) !=
                 nullptr);

    // Steps 5 and 6 verify that a child can stay with its parent when the parent's RLOC16 changes, as happens when
    // the parent gets a different Router ID, for example after a partition merge. LEADER releasing the Router ID of
    // ROUTER_1 stands in for that here. The FED re-attaches to the same parent under a new RLOC16 and must not tell
    // that parent, which is its current parent, to remove it.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: LEADER releases the Router ID of ROUTER_1; FED re-attaches to ROUTER_1 under its new RLOC16");

    // From here on the FED hears only ROUTER_1, so it re-attaches to the same parent.
    fed.UnallowList(leader);

    {
        uint16_t oldRloc16 = router1.Get<Mle::Mle>().GetRloc16();

        // Make ROUTER_1 request a Router ID again well within the leader's Router ID reuse delay, so that it gets a
        // different one.
        router1.Get<Mle::Mle>().SetRouterSelectionJitter(1);

        SuccessOrQuit(leader.Get<RouterTable>().Release(Mle::RouterIdFromRloc16(oldRloc16)));

        for (uint32_t elapsed = 0; elapsed < 600; elapsed++)
        {
            nexus.AdvanceTime(1000);

            if (router1.Get<Mle::Mle>().IsRouter() && (router1.Get<Mle::Mle>().GetRloc16() != oldRloc16) &&
                fed.Get<Mle::Mle>().IsChild() &&
                (fed.Get<Mle::Mle>().GetParent().GetRloc16() == router1.Get<Mle::Mle>().GetRloc16()))
            {
                break;
            }
        }

        VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
        VerifyOrQuit(router1.Get<Mle::Mle>().GetRloc16() != oldRloc16);
        VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
        VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetRloc16() == router1.Get<Mle::Mle>().GetRloc16());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Verify ROUTER_1 keeps FED as its child");

    nexus.AdvanceTime(5000);

    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(router1.Get<ChildTable>().FindChild(fed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) !=
                 nullptr);
    VerifyOrQuit(fed.Get<Mle::Mle>().GetTimeout() > 60);

    // Step 7 verifies that the FED still informs its previous parent after a reset, using the parent it restores from
    // non-volatile settings.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: FED resets and, no longer hearing ROUTER_1, attaches to LEADER");

    // The reset clears the FED's MAC filter and runtime configuration, so set them up again. The FED hears only
    // LEADER from now on, while ROUTER_1 still hears the FED.
    fed.Reset();
    fed.AllowList(leader);
    SuccessOrQuit(fed.Get<Mle::Mle>().SetRouterEligible(false));
    SuccessOrQuit(fed.Get<Mle::Mle>().SetChildRouterLinks(0));
    fed.Get<ThreadNetif>().Up();
    SuccessOrQuit(fed.Get<Mle::Mle>().Start());

    for (uint32_t elapsed = 0; elapsed < 60000; elapsed += 100)
    {
        nexus.AdvanceTime(100);

        if (fed.Get<Mle::Mle>().IsChild() &&
            fed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress())
        {
            break;
        }
    }

    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Verify ROUTER_1 removed FED from its child table before the child timeout");

    nexus.AdvanceTime(5000);

    VerifyOrQuit(router1.Get<ChildTable>().FindChild(fed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid) ==
                 nullptr);
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFedInformPreviousParentOnReattach();
    printf("All tests passed\n");
    return 0;
}
