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

// Regression test for the endless MLE role flap in
// `Mle::AnnounceHandler::HandleAnnounce`.
//
// Two FTDs share the same channel, PAN ID, and network credentials but
// hold different Active Dataset Timestamps, and the RF link between them
// is too weak to merge their partitions (Advertisements from a different
// partition are rejected with `kErrorLinkMarginLow` at `mle_ftd.cpp`,
// since their link margin is below `kPartitionMergeMinMargin`). MLE
// Announce frames are not link-margin gated, so they still reach the
// lower-timestamp peer.
//
// Without the fix, the lower-timestamp node loops
// Leader -> Detached -> Disabled -> Detached -> Leader: it runs the
// `kAnnounceAttachAfterDelay` action and pointlessly Stop()/Start()s on
// every Announce, even though the announced channel/PAN ID already match
// its current MAC parameters (so there is nothing to migrate to). Each
// re-attach allocates a fresh partition ID. With the fix, both nodes stay
// stable Leaders of their separate partitions and keep their partition
// IDs.

// RSS that yields a link margin below `kPartitionMergeMinMargin` (= 5 in
// the Nexus config) but still above the radio receive sensitivity
// (-100 dBm). With noise floor = sensitivity = -100 dBm, margin =
// rss + 100 = 3 dB: Advertisements are rejected for partition merge,
// while frames (including Announces) are still delivered and processed.
static constexpr int8_t kWeakLinkRss = -97;

// Active Timestamps (seconds). Identical dataset content otherwise; the
// higher timestamp drives the Announces that, without the fix, restart
// the lower-timestamp peer.
static constexpr uint64_t kTimestampOld = 1;
static constexpr uint64_t kTimestampNew = 1000;

static constexpr uint32_t kFormNetworkTime = 13 * 1000; // Time to form/attach as leader, in ms.
static constexpr uint32_t kSettleTime      = 10 * 1000; // Time for the network to settle, in ms.

// Per-round wait after each Announce. Far larger than the buggy
// `kAnnounceProcessTimeout` (250 ms) plus the subsequent re-attach, so a
// flap would be observed within a single round if present.
static constexpr uint32_t kAnnounceSettleTime = 5 * 1000;

// Number of Announce rounds to drive. Covers the original ~20-minute
// Python test window in (instant) simulated time.
static constexpr uint32_t kAnnounceRounds = 12;

void Test(void)
{
    Core nexus;

    Node &leaderOld = nexus.CreateNode();
    Node &leaderNew = nexus.CreateNode();

    leaderOld.SetName("LEADER_OLD");
    leaderNew.SetName("LEADER_NEW");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: LEADER_OLD forms its own partition with the older Active Timestamp");

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;

        // Build one shared dataset (random credentials/channel/PAN ID),
        // then give each node the same content but a different Active
        // Timestamp.
        SuccessOrQuit(datasetInfo.GenerateRandom(leaderOld.GetInstance()));

        timestamp.Clear();
        timestamp.SetSeconds(kTimestampOld);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leaderOld.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leaderOld.Get<ThreadNetif>().Up();
        SuccessOrQuit(leaderOld.Get<Mle::Mle>().Start());
        nexus.AdvanceTime(kFormNetworkTime);
        VerifyOrQuit(leaderOld.Get<Mle::Mle>().IsLeader());

        Log("---------------------------------------------------------------------------------------");
        Log("Step 2: LEADER_NEW forms its own partition with the newer Active Timestamp (isolated)");

        // Isolate both nodes so they cannot hear each other. This prevents
        // LEADER_OLD from attempting to attach to LEADER_NEW, and forces
        // LEADER_NEW to form its own partition.
        leaderNew.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
        leaderOld.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);

        timestamp.SetSeconds(kTimestampNew);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leaderNew.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leaderNew.Get<ThreadNetif>().Up();
        SuccessOrQuit(leaderNew.Get<Mle::Mle>().Start());
        nexus.AdvanceTime(kFormNetworkTime);
        VerifyOrQuit(leaderNew.Get<Mle::Mle>().IsLeader());
    }

    uint32_t partitionOld = leaderOld.Get<Mle::Mle>().GetLeaderData().GetPartitionId();
    uint32_t partitionNew = leaderNew.Get<Mle::Mle>().GetLeaderData().GetPartitionId();

    VerifyOrQuit(partitionOld != partitionNew);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Wire a weak link - Advertisements rejected (no merge), Announces still delivered");

    {
        const Mac::ExtAddress &extOld = leaderOld.Get<Mac::Mac>().GetExtAddress();
        const Mac::ExtAddress &extNew = leaderNew.Get<Mac::Mac>().GetExtAddress();

        leaderOld.AllowList(leaderNew);
        SuccessOrQuit(leaderOld.Get<Mac::Filter>().AddRssIn(extNew, kWeakLinkRss));

        leaderNew.AllowList(leaderOld);
        SuccessOrQuit(leaderNew.Get<Mac::Filter>().AddRssIn(extOld, kWeakLinkRss));
    }

    nexus.AdvanceTime(kSettleTime);

    // The link must be genuinely unmergeable: both nodes remain Leaders of
    // their own partitions.
    VerifyOrQuit(leaderOld.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leaderNew.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leaderOld.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionOld);
    VerifyOrQuit(leaderNew.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionNew);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Drive Announces from the newer peer; the older node must not flap");

    {
        uint8_t channel = leaderOld.Get<Mac::Mac>().GetPanChannel();

        for (uint32_t round = 0; round < kAnnounceRounds; round++)
        {
            // LEADER_NEW announces on the shared channel. LEADER_OLD sees a
            // newer Active Timestamp with a matching channel/PAN ID -- the
            // `kAnnounceAttachAfterDelay` path that used to Stop()/Start()
            // the attached node.
            leaderNew.Get<Mle::Mle>().SendAnnounce(channel);
            nexus.AdvanceTime(kAnnounceSettleTime);

            // A Stop()/Start() flap would drop the Leader role and
            // re-create the partition with a fresh ID on re-attach.
            VerifyOrQuit(leaderOld.Get<Mle::Mle>().IsLeader());
            VerifyOrQuit(leaderOld.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionOld);

            // The mirrored `kSendAnnouceBack` guard: LEADER_OLD's own
            // Announces reach LEADER_NEW (older timestamp) and must not
            // disturb it either.
            VerifyOrQuit(leaderNew.Get<Mle::Mle>().IsLeader());
            VerifyOrQuit(leaderNew.Get<Mle::Mle>().GetLeaderData().GetPartitionId() == partitionNew);
        }
    }

    nexus.SaveTestInfo("test_announce_no_flap_on_unmergeable_partitions.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test();
    printf("All tests passed\n");
    return 0;
}
