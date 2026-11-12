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

static constexpr uint32_t kFormNetworkTime   = 13 * 1000;
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;
static constexpr uint16_t kEchoIdentifier    = 0x1234;
static constexpr uint16_t kEchoPayloadSize   = 32;
static constexpr uint16_t kScanDuration      = 200; // > OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT (100 ms)

static void SendSingleDataPoll(Node &aSed)
{
    aSed.Get<DataPollSender>().StartPolling();
    SuccessOrQuit(aSed.Get<DataPollSender>().SendDataPoll());
    aSed.Get<DataPollSender>().StopPolling();
}

void TestSkipExpiredPendingPollsAndDeliverFreshPolls(void)
{
    /**
     * Scenario 1: Skip expired pending Data Polls while serving fresh pending Data Polls, and preserve queued
     * indirect messages across `DataPollHandler::kMaxPollTriggeredTxAttempts` cycles.
     *
     * On each cycle while the Leader's MAC is occupied on the PAN channel for 200 ms (`> kDataPollTimeout` of 100 ms):
     * - `sed1` polls at `t = 10 ms`, claiming `DataPollHandler::mIndirectTxChild`.
     * - `sed2` polls at `t = 20 ms` and enters the pending poll set (`Child::IsDataPollPending()`). Its 100 ms
     *   post-poll receive window expires at `t = 120 ms` (`80 ms` before the scan completes).
     * - `sed3` polls at `t = 150 ms` and enters the pending poll set. Its 100 ms post-poll receive window remains
     *   open (`[150 ms, 250 ms]`) when the scan completes at `t = 200 ms`.
     *
     * Verifies across `DataPollHandler::kMaxPollTriggeredTxAttempts` (4) cycles that:
     * 1. `DataPollHandler::ProcessPendingPolls()` skips `sed2`'s expired pending poll without wasting an indirect
     *    transmission attempt into dead air (`mTxIndirectMaxRetryExpiry` increments only once per cycle for `sed1`,
     *    never for `sed2` or `sed3`).
     * 2. `sed3`'s fresh pending poll is immediately served and delivered in every cycle.
     * 3. `sed2`'s queued indirect message is not dropped after 4 expired pending polls, and is cleanly delivered on
     *    `sed2`'s next Data Poll when the Leader is idle.
     */

    Core        nexus;
    Node       &leader = nexus.CreateNode();
    Node       &sed1   = nexus.CreateNode();
    Node       &sed2   = nexus.CreateNode();
    Node       &sed3   = nexus.CreateNode();
    Node *const seds[] = {&sed1, &sed2, &sed3};
    Child      *child1;
    Child      *child2;
    Child      *child3;
    uint32_t    panChannelMask;
    uint32_t    sed2InitialRx;
    uint32_t    txIndirectExpiry;

    leader.SetName("LEADER");
    sed1.SetName("SED_1");
    sed2.SetName("SED_2");
    sed3.SetName("SED_3");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 1: Skip expired pending Data Polls and preserve queued indirect messages");

    AllowLinkBetween(leader, sed1);
    AllowLinkBetween(leader, sed2);
    AllowLinkBetween(leader, sed3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (Node *sed : seds)
    {
        sed->Get<Ip6::Icmp>().SetEchoMode(OT_ICMP6_ECHO_HANDLER_DISABLED);
        sed->Join(leader, Node::kAsSed);
    }

    nexus.AdvanceTime(kAttachAsChildTime);

    for (Node *sed : seds)
    {
        VerifyOrQuit(sed->Get<Mle::Mle>().IsChild());
        sed->Get<DataPollSender>().StopPolling();
    }

    nexus.AdvanceTime(200);

    child1 = leader.Get<ChildTable>().FindChild(sed1.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    child2 = leader.Get<ChildTable>().FindChild(sed2.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    child3 = leader.Get<ChildTable>().FindChild(sed3.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child1 != nullptr);
    VerifyOrQuit(child2 != nullptr);
    VerifyOrQuit(child3 != nullptr);

    panChannelMask = 1UL << leader.Get<Mac::Mac>().GetPanChannel();

    // Queue one message for `sed2` that must survive across all `kMaxPollTriggeredTxAttempts` expired-poll cycles.
    sed2InitialRx = sed2.Get<MeshForwarder>().GetCounters().mRxSuccess;
    leader.SendEchoRequest(sed2.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(1);
    VerifyOrQuit(child2->GetIndirectMessageCount() == 1);

    for (uint8_t cycle = 0; cycle < DataPollHandler::kMaxPollTriggeredTxAttempts; cycle++)
    {
        uint32_t sed1RxBefore = sed1.Get<MeshForwarder>().GetCounters().mRxSuccess;
        uint32_t sed3RxBefore = sed3.Get<MeshForwarder>().GetCounters().mRxSuccess;
        uint32_t expiryBefore;

        leader.SendEchoRequest(sed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
        leader.SendEchoRequest(sed3.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
        nexus.AdvanceTime(1);

        VerifyOrQuit(child1->GetIndirectMessageCount() == 1);
        VerifyOrQuit(child2->GetIndirectMessageCount() == 1);
        VerifyOrQuit(child3->GetIndirectMessageCount() == 1);

        expiryBefore = leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry;

        // Occupy Leader's MAC on the PAN channel for 200 ms so incoming Data Polls are received and ACKed with FP=1.
        SuccessOrQuit(leader.Get<Mac::Mac>().EnergyScan(panChannelMask, kScanDuration, nullptr, nullptr));
        nexus.AdvanceTime(10);

        // t = 10 ms: `sed1` claims `mIndirectTxChild` (its 100 ms receive window closes at t = 110 ms).
        SendSingleDataPoll(sed1);
        nexus.AdvanceTime(10);

        // t = 20 ms: `sed2` queues a pending poll whose 100 ms receive window closes at t = 120 ms (expired by t=200).
        SendSingleDataPoll(sed2);
        nexus.AdvanceTime(130);

        // t = 150 ms: `sed3` queues a fresh pending poll whose 100 ms receive window stays open until t = 250 ms.
        SendSingleDataPoll(sed3);

        // Advance past scan completion (t = 200 ms) to t = 230 ms.
        nexus.AdvanceTime(80);

        // `sed3` must have received its frame, and `sed2`'s expired pending poll must NOT have caused a NoAck attempt.
        VerifyOrQuit(sed3.Get<MeshForwarder>().GetCounters().mRxSuccess == sed3RxBefore + 1);
        VerifyOrQuit(child3->GetIndirectMessageCount() == 0);
        VerifyOrQuit(child2->GetIndirectMessageCount() == 1);
        VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry == expiryBefore + 1);

        // Drain `sed1`'s queued message on an idle channel before the next cycle.
        SendSingleDataPoll(sed1);
        nexus.AdvanceTime(20);
        VerifyOrQuit(sed1.Get<MeshForwarder>().GetCounters().mRxSuccess == sed1RxBefore + 1);
        VerifyOrQuit(child1->GetIndirectMessageCount() == 0);
    }

    Log("Verify `sed2`'s queued message survived %u expired pending polls and is delivered on next poll",
        DataPollHandler::kMaxPollTriggeredTxAttempts);

    VerifyOrQuit(child2->GetIndirectMessageCount() == 1);
    txIndirectExpiry = leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry;

    SendSingleDataPoll(sed2);
    nexus.AdvanceTime(20);

    VerifyOrQuit(sed2.Get<MeshForwarder>().GetCounters().mRxSuccess == sed2InitialRx + 1);
    VerifyOrQuit(child2->GetIndirectMessageCount() == 0);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry == txIndirectExpiry);
}

void TestAllPendingPollsExpiredAndFifoAmongFreshPolls(void)
{
    /**
     * Scenario 2:
     * - Part A: When all pending Data Polls have expired by the time `ProcessPendingPolls()` runs, no indirect
     *   transmission is requested for the expired polls and `DataPollHandler` returns cleanly to idle.
     * - Part B: When multiple fresh (`< kMaxPendingPollTime`) pending Data Polls exist, `ProcessPendingPolls()`
     *   continues to serve both in FIFO order of `GetLastHeard()`.
     */

    Core        nexus;
    Node       &leader = nexus.CreateNode();
    Node       &sed1   = nexus.CreateNode();
    Node       &sed2   = nexus.CreateNode();
    Node       &sed3   = nexus.CreateNode();
    Node *const seds[] = {&sed1, &sed2, &sed3};
    Child      *child1;
    Child      *child2;
    Child      *child3;
    uint32_t    panChannelMask;
    uint32_t    expiryBefore;
    uint32_t    sed2RxBefore;
    uint32_t    sed3RxBefore;

    leader.SetName("LEADER");
    sed1.SetName("SED_1");
    sed2.SetName("SED_2");
    sed3.SetName("SED_3");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 2: All pending polls expired, and FIFO ordering among multiple fresh polls");

    AllowLinkBetween(leader, sed1);
    AllowLinkBetween(leader, sed2);
    AllowLinkBetween(leader, sed3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (Node *sed : seds)
    {
        sed->Get<Ip6::Icmp>().SetEchoMode(OT_ICMP6_ECHO_HANDLER_DISABLED);
        sed->Join(leader, Node::kAsSed);
    }

    nexus.AdvanceTime(kAttachAsChildTime);

    for (Node *sed : seds)
    {
        VerifyOrQuit(sed->Get<Mle::Mle>().IsChild());
        sed->Get<DataPollSender>().StopPolling();
    }

    nexus.AdvanceTime(200);

    child1 = leader.Get<ChildTable>().FindChild(sed1.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    child2 = leader.Get<ChildTable>().FindChild(sed2.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    child3 = leader.Get<ChildTable>().FindChild(sed3.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child1 != nullptr);
    VerifyOrQuit(child2 != nullptr);
    VerifyOrQuit(child3 != nullptr);

    panChannelMask = 1UL << leader.Get<Mac::Mac>().GetPanChannel();

    Log("Part A: All pending polls (`sed2` and `sed3`) expire before `ProcessPendingPolls()` runs");

    leader.SendEchoRequest(sed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    leader.SendEchoRequest(sed2.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    leader.SendEchoRequest(sed3.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(1);

    expiryBefore = leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry;

    SuccessOrQuit(leader.Get<Mac::Mac>().EnergyScan(panChannelMask, kScanDuration, nullptr, nullptr));
    nexus.AdvanceTime(10);
    SendSingleDataPoll(sed1);
    nexus.AdvanceTime(10);
    SendSingleDataPoll(sed2);
    nexus.AdvanceTime(10);
    SendSingleDataPoll(sed3);

    // Advance past scan completion (t = 200 ms) to t = 230 ms; both `sed2` (t=20) and `sed3` (t=30) have expired.
    nexus.AdvanceTime(200);

    // Only `sed1` (`mIndirectTxChild`) attempted indirect TX; both expired pending polls were skipped.
    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry == expiryBefore + 1);
    VerifyOrQuit(child2->GetIndirectMessageCount() == 1);
    VerifyOrQuit(child3->GetIndirectMessageCount() == 1);

    // Drain `sed1`'s message, then verify Part B with both `sed2` and `sed3` sending fresh pending polls.
    SendSingleDataPoll(sed1);
    nexus.AdvanceTime(20);
    VerifyOrQuit(child1->GetIndirectMessageCount() == 0);

    Log("Part B: Multiple fresh pending polls (`sed3` then `sed2`) are both served without expiry");

    leader.SendEchoRequest(sed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(1);

    sed2RxBefore = sed2.Get<MeshForwarder>().GetCounters().mRxSuccess;
    sed3RxBefore = sed3.Get<MeshForwarder>().GetCounters().mRxSuccess;
    expiryBefore = leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry;

    SuccessOrQuit(leader.Get<Mac::Mac>().EnergyScan(panChannelMask, kScanDuration, nullptr, nullptr));
    nexus.AdvanceTime(10);
    SendSingleDataPoll(sed1);
    nexus.AdvanceTime(130);

    // Both `sed3` (t = 140 ms) and `sed2` (t = 150 ms) remain fresh (< 100 ms old) when scan ends at t = 200 ms.
    SendSingleDataPoll(sed3);
    nexus.AdvanceTime(10);
    SendSingleDataPoll(sed2);
    nexus.AdvanceTime(80);

    VerifyOrQuit(sed3.Get<MeshForwarder>().GetCounters().mRxSuccess == sed3RxBefore + 1);
    VerifyOrQuit(sed2.Get<MeshForwarder>().GetCounters().mRxSuccess == sed2RxBefore + 1);
    VerifyOrQuit(child2->GetIndirectMessageCount() == 0);
    VerifyOrQuit(child3->GetIndirectMessageCount() == 0);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxIndirectMaxRetryExpiry == expiryBefore + 1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSkipExpiredPendingPollsAndDeliverFreshPolls();
    ot::Nexus::TestAllPendingPollsExpiredAndFifoAmongFreshPolls();

    printf("All tests passed\n");
    return 0;
}
