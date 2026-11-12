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

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kJoinTime           = 20 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kMaxDetectTime      = 30 * 1000;
static constexpr uint32_t kMaxDetectAdvTime   = 60 * 1000;
static constexpr uint32_t kDetectStep         = 10;
static constexpr uint32_t kReattachTime       = 30 * 1000;
static constexpr uint32_t kIsolationTime      = 25 * 1000;
static constexpr uint32_t kFallbackTime       = 120 * 1000;

static constexpr uint8_t kLargeJitter = 255;
static constexpr uint8_t kSmallJitter = 1;

static void ReleaseRouterId(Node &aLeader, Node &aDut)
{
    SuccessOrQuit(aLeader.Get<RouterTable>().Release(Mle::RouterIdFromRloc16(aDut.Get<Mle::Mle>().GetRloc16())));
}

static void WaitForDetach(Core &aNexus, Node &aDut, uint32_t aMaxTime)
{
    uint32_t elapsed = 0;

    while (!aDut.Get<Mle::Mle>().IsDetached())
    {
        VerifyOrQuit(elapsed < aMaxTime);
        aNexus.AdvanceTime(kDetectStep);
        elapsed += kDetectStep;
    }
}

void TestRouterIdDeallocReattach(void)
{
    /**
     * Router whose Router ID is deallocated by the Leader (Thread 1.4.1 5.9.9 and 5.16.1).
     *
     * Topology:
     *
     *   LEADER ---- REED ---- DUT ---- OTHER
     *      \__________________/
     *
     * The LEADER-DUT and DUT-OTHER links are only enabled in some phases. OTHER forms its own partition with a
     * higher leader weight and is never linked to LEADER or REED.
     *
     * Phase 1: The DUT learns that its Router ID was removed and can then only reach the REED. It MUST send its
     * Parent Requests with both R and E flags set, attach through the REED, and immediately request its previous
     * Router ID (the DUT uses a large router selection jitter, so a normal REED upgrade would be much later).
     *
     * Phase 2: The DUT is isolated while the Leader releases its Router ID and the ID sequence advances several
     * times. When the link to the REED is restored, the DUT MUST accept the REED (whose ID sequence equals the
     * newly learned one) and request its previous Router ID again.
     *
     * Phase 3: The DUT's Router ID is released and the DUT is isolated. After the single reconnect attempt fails,
     * the DUT MUST fall back to a full regular attach and eventually forms its own partition.
     *
     * Phase 4: The DUT merges back into the LEADER's partition and becomes a router. Its Router ID is released and
     * it can then only reach OTHER. The DUT MUST attach to OTHER's higher-priority partition during the reconnect
     * attempt.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();
    Node &other  = nexus.CreateNode();

    uint32_t partitionId;
    uint32_t otherPartitionId;
    uint8_t  dutIdSequence;
    uint8_t  leaderIdSequence;

    leader.SetName("LEADER");
    reed.SetName("REED");
    dut.SetName("DUT");
    other.SetName("OTHER");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Form topology: DUT as router, REED as child of LEADER, OTHER as leader of another partition");

    AllowLinkBetween(leader, dut);
    AllowLinkBetween(leader, reed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    reed.Join(leader);
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(0);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    AllowLinkBetween(reed, dut);
    dut.Get<Mle::Mle>().SetRouterSelectionJitter(kLargeJitter);

    // OTHER hears no other node until Step 8 and forms a partition with a higher leader weight.
    other.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
    other.Get<Mle::Mle>().SetLeaderWeight(leader.Get<Mle::Mle>().GetLeaderData().GetWeighting() + 1);
    other.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(other.Get<Mle::Mle>().IsLeader());

    partitionId      = leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId();
    otherPartitionId = other.Get<Mle::Mle>().GetLeaderData().GetPartitionId();
    VerifyOrQuit(otherPartitionId != partitionId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader releases the DUT's Router ID, then the DUT can only reach the REED");

    ReleaseRouterId(leader, dut);
    WaitForDetach(nexus, dut, kMaxDetectTime);
    UnallowLinkBetween(leader, dut);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: DUT reattaches through the REED and immediately becomes router again");

    nexus.AdvanceTime(kReattachTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionId);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: DUT is isolated while the Leader releases its Router ID and the ID sequence advances");

    UnallowLinkBetween(reed, dut);
    ReleaseRouterId(leader, dut);
    nexus.AdvanceTime(kIsolationTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    dutIdSequence    = dut.Get<RouterTable>().GetRouterIdSequence();
    leaderIdSequence = leader.Get<RouterTable>().GetRouterIdSequence();
    VerifyOrQuit(static_cast<uint8_t>(leaderIdSequence - dutIdSequence) >= 2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Link to the REED is restored, DUT reattaches and immediately becomes router again");

    AllowLinkBetween(reed, dut);
    WaitForDetach(nexus, dut, kMaxDetectAdvTime);
    nexus.AdvanceTime(kReattachTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader releases the DUT's Router ID again, then the DUT is isolated");

    ReleaseRouterId(leader, dut);
    WaitForDetach(nexus, dut, kMaxDetectTime);
    UnallowLinkBetween(reed, dut);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Reconnect to the same partition fails, DUT falls back to any partition and forms its own");

    nexus.AdvanceTime(kFallbackTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetLeaderData().GetPartitionId() != partitionId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: DUT merges back into the Leader's partition and becomes router");

    dut.Get<Mle::Mle>().SetRouterSelectionJitter(kSmallJitter);
    AllowLinkBetween(reed, dut);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader releases the DUT's Router ID, then the DUT can only reach OTHER");

    ReleaseRouterId(leader, dut);
    WaitForDetach(nexus, dut, kMaxDetectTime);
    UnallowLinkBetween(reed, dut);
    AllowLinkBetween(dut, other);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: DUT attaches to OTHER's higher-priority partition");

    nexus.AdvanceTime(kReattachTime);

    VerifyOrQuit(!dut.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == otherPartitionId);

    nexus.SaveTestInfo("test_router_id_dealloc_reattach.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRouterIdDeallocReattach();
    printf("All tests passed\n");
    return 0;
}
