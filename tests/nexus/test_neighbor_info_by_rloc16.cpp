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

static void VerifyMatchesIteration(Node &aNode, const otNeighborInfo &aInfo)
{
    // The direct lookup must report the same entry as iterating the table.

    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    otNeighborInfo         iterInfo;
    bool                   found = false;

    while (otThreadGetNextNeighborInfo(&aNode.GetInstance(), &iterator, &iterInfo) == OT_ERROR_NONE)
    {
        if (iterInfo.mRloc16 == aInfo.mRloc16)
        {
            VerifyOrQuit(AsCoreType(&iterInfo.mExtAddress) == AsCoreType(&aInfo.mExtAddress));
            VerifyOrQuit(iterInfo.mIsChild == aInfo.mIsChild);
            VerifyOrQuit(iterInfo.mRxOnWhenIdle == aInfo.mRxOnWhenIdle);
            VerifyOrQuit(iterInfo.mFullThreadDevice == aInfo.mFullThreadDevice);
            VerifyOrQuit(iterInfo.mFullNetworkData == aInfo.mFullNetworkData);
            VerifyOrQuit(iterInfo.mVersion == aInfo.mVersion);
            found = true;
        }
    }

    VerifyOrQuit(found);
}

static void VerifyCliRole(Node &aNode, uint16_t aRloc16, const char *aExpectedRoleLine)
{
    aNode.InputCli("neighbor 0x%04x", aRloc16);
    VerifyOrQuit(aNode.IsCliOutputSuccess());
    VerifyOrQuit(aNode.GetCliOutputLines()[0].Matches(aExpectedRoleLine));
}

void TestNeighborInfoByRloc16(void)
{
    Core           nexus;
    otNeighborInfo info;
    uint16_t       router1Rloc16;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med     = nexus.CreateNode();
    Node &fed     = nexus.CreateNode();

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network with a router, an MED, and an FED");

    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // The children join before `router1`, so the leader is the only
    // parent candidate and `router1` is a non-parent router to them.
    med.Join(leader, Node::kAsMed);
    fed.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(20 * 1000);

    router1.Join(leader);
    nexus.AdvanceTime(300 * 1000);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().GetParent().GetRloc16() == leader.Get<Mle::Mle>().GetRloc16());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(fed.Get<Mle::Mle>().GetParent().GetRloc16() == leader.Get<Mle::Mle>().GetRloc16());

    Log("---------------------------------------------------------------------------------------");
    Log("Leader: look up a neighboring router by RLOC16");

    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), router1.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(info.mRloc16 == router1.Get<Mle::Mle>().GetRloc16());
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == router1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(!info.mIsChild);
    VerifyOrQuit(info.mFullThreadDevice);
    VerifyMatchesIteration(leader, info);
    VerifyCliRole(leader, router1.Get<Mle::Mle>().GetRloc16(), "Role: R");

    Log("---------------------------------------------------------------------------------------");
    Log("Leader: look up a child by RLOC16");

    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), med.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(info.mRloc16 == med.Get<Mle::Mle>().GetRloc16());
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == med.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(info.mIsChild);
    VerifyOrQuit(!info.mFullThreadDevice);
    VerifyMatchesIteration(leader, info);
    VerifyCliRole(leader, med.Get<Mle::Mle>().GetRloc16(), "Role: C");

    Log("---------------------------------------------------------------------------------------");
    Log("Router: look up the leader by RLOC16");

    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&router1.GetInstance(), leader.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(!info.mIsChild);
    VerifyCliRole(router1, leader.Get<Mle::Mle>().GetRloc16(), "Role: R");

    Log("---------------------------------------------------------------------------------------");
    Log("Leader: unknown, unused-bit, broadcast, invalid, and own RLOC16 are not found");

    // Child IDs are allocated from 1 upwards, so the highest one is unused.
    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(),
                                                 leader.Get<Mle::Mle>().GetRloc16() | Mle::kMaxChildId,
                                                 &info) == OT_ERROR_NOT_FOUND);

    // Bit 9 is not part of the Router ID or Child ID, so this must not
    // match `router1` even though the router table is indexed by Router ID.
    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), router1.Get<Mle::Mle>().GetRloc16() | 0x0200,
                                                 &info) == OT_ERROR_NOT_FOUND);

    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), Mac::kShortAddrBroadcast, &info) ==
                 OT_ERROR_NOT_FOUND);
    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), Mac::kShortAddrInvalid, &info) ==
                 OT_ERROR_NOT_FOUND);
    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), leader.Get<Mle::Mle>().GetRloc16(), &info) ==
                 OT_ERROR_NOT_FOUND);

    Log("---------------------------------------------------------------------------------------");
    Log("MED: parent is found, non-parent router is not");

    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&med.GetInstance(), leader.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(!info.mIsChild);
    VerifyCliRole(med, leader.Get<Mle::Mle>().GetRloc16(), "Role: R");

    // Not cross-checked against iteration: the FTD build of `otThreadGetNextNeighborInfo()`
    // does not report the parent while in child role (the MTD build does).

    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&med.GetInstance(), router1.Get<Mle::Mle>().GetRloc16(), &info) ==
                 OT_ERROR_NOT_FOUND);

    Log("---------------------------------------------------------------------------------------");
    Log("FED: parent is found, and so is a non-parent router it has a link with");

    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&fed.GetInstance(), leader.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == leader.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(!info.mIsChild);

    // An FTD child establishes links with neighboring routers other than
    // its parent, and these must be reported just as iteration does.
    SuccessOrQuit(otThreadGetNeighborInfoByRloc16(&fed.GetInstance(), router1.Get<Mle::Mle>().GetRloc16(), &info));
    VerifyOrQuit(AsCoreType(&info.mExtAddress) == router1.Get<Mac::Mac>().GetExtAddress());
    VerifyOrQuit(!info.mIsChild);
    VerifyMatchesIteration(fed, info);

    Log("---------------------------------------------------------------------------------------");
    Log("Leader: router that has left is no longer found");

    router1Rloc16 = router1.Get<Mle::Mle>().GetRloc16();
    router1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(300 * 1000);

    VerifyOrQuit(otThreadGetNeighborInfoByRloc16(&leader.GetInstance(), router1Rloc16, &info) == OT_ERROR_NOT_FOUND);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestNeighborInfoByRloc16();
    printf("All tests passed\n");
    return 0;
}
