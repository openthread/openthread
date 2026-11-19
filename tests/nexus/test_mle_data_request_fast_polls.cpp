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
#include "thread/link_metrics.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime         = 13 * 1000;
static constexpr uint32_t kAttachAsChildTime       = 10 * 1000;
static constexpr uint32_t kSettleTime              = 5 * 1000;
static constexpr uint32_t kIdleCheckWindow         = 3 * 1000;
static constexpr uint32_t kFirstFastPollWait       = 100; // Between first poll (10 ms) and second poll (198 ms)
static constexpr uint32_t kFastPollPeriod          = 188;
static constexpr uint32_t kChildUpdateResponseWait = 150; // Past attach-mode poll (100 ms) and before next fast poll
static constexpr uint32_t kChildUpdateSettle       = 500; // Past fast poll (188 ms) + 100 ms delay + 100 ms attach poll
static constexpr uint32_t kMaxWaitSteps            = 1500;
static constexpr uint32_t kShortWaitSteps          = 200;

static bool sReportReceived;

static void HandleLinkMetricsReport(const otIp6Address        *aSource,
                                    const otLinkMetricsValues *aMetricsValues,
                                    otLinkMetricsStatus        aStatus,
                                    void                      *aContext)
{
    OT_UNUSED_VARIABLE(aSource);
    OT_UNUSED_VARIABLE(aMetricsValues);
    OT_UNUSED_VARIABLE(aContext);

    VerifyOrQuit(aStatus == OT_LINK_METRICS_STATUS_SUCCESS);
    sReportReceived = true;
}

static uint32_t GetTxDataPollCount(Node &aNode) { return aNode.Get<Mac::Mac>().GetCounters().mTxDataPoll; }

static void AddOnMeshPrefix(Node &aNode, uint16_t aIndex, bool aStable)
{
    NetworkData::OnMeshPrefixConfig config;
    char                            prefixString[24];

    config.Clear();
    snprintf(prefixString, sizeof(prefixString), "fd00:%x::/64", aIndex);
    SuccessOrQuit(config.GetPrefix().FromString(prefixString));
    config.mOnMesh = true;
    config.mStable = aStable;
    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

static uint16_t CountOnMeshPrefixes(Node &aNode)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig config;
    uint16_t                        count = 0;

    while (aNode.Get<NetworkData::Leader>().GetNext(iterator, config) == kErrorNone)
    {
        count++;
    }

    return count;
}

static const Message *FindQueuedDataResponse(Node &aParent, const Child &aChild)
{
    return aParent.Get<IndirectSender>().FindQueuedMessageForSleepyChild(aChild,
                                                                         Message::AcceptMle<Mle::kCommandDataResponse>);
}

static const Message *FindQueuedChildUpdateRequest(Node &aParent, const Child &aChild)
{
    return aParent.Get<IndirectSender>().FindQueuedMessageForSleepyChild(
        aChild, Message::AcceptMle<Mle::kCommandChildUpdateRequest>);
}

static void SendLinkMetricsSingleProbe(Node &aSed, Node &aLeader)
{
    LinkMetrics::Metrics metrics;

    metrics.Clear();
    metrics.mLqi    = true;
    sReportReceived = false;

    SuccessOrQuit(aSed.Get<LinkMetrics::Initiator>().Query(aLeader.Get<Mle::Mle>().GetLinkLocalAddress(),
                                                           /* aSeriesId */ 0, &metrics));
}

static void TriggerTrackedDataRequestAndDropResponse(Core    &aNexus,
                                                     Node    &aLeader,
                                                     Node    &aSed,
                                                     Child   &aChild,
                                                     uint16_t aPrefixIndex,
                                                     bool     aStable)
{
    uint16_t initialPrefixes = CountOnMeshPrefixes(aSed);

    AddOnMeshPrefix(aLeader, aPrefixIndex, aStable);
    aNexus.AdvanceTime(10);

    // Purge the immediate Child Update Request and align right after a 1-second
    // TimeTicker re-queue so the Link Metrics probe and MLE Data Request exchange
    // completes well before the next TimeTicker tick.
    VerifyOrQuit(FindQueuedChildUpdateRequest(aLeader, aChild) != nullptr);
    aLeader.Get<MeshForwarder>().RemoveMessagesForChild(aChild, Message::AcceptMle<Mle::kCommandChildUpdateRequest>);

    for (uint32_t i = 0; (i < kMaxWaitSteps) && (FindQueuedChildUpdateRequest(aLeader, aChild) == nullptr); i++)
    {
        aNexus.AdvanceTime(1);
    }

    VerifyOrQuit(FindQueuedChildUpdateRequest(aLeader, aChild) != nullptr);
    aLeader.Get<MeshForwarder>().RemoveMessagesForChild(aChild, Message::AcceptMle<Mle::kCommandChildUpdateRequest>);

    // A Link Metrics Single Probe Data Response carries the newer Leader Data version
    // without a Network Data TLV, prompting the SED to send a tracked MLE Data Request.
    SendLinkMetricsSingleProbe(aSed, aLeader);

    for (uint32_t i = 0; (i < kShortWaitSteps) && !sReportReceived; i++)
    {
        aNexus.AdvanceTime(1);
    }

    VerifyOrQuit(sReportReceived);
    VerifyOrQuit(CountOnMeshPrefixes(aSed) == initialPrefixes);

    // Wait until the SED transmits its MLE Data Request and Leader queues Data Response,
    // then drop the Data Response before the SED polls for it.
    for (uint32_t i = 0; (i < kShortWaitSteps) && (FindQueuedDataResponse(aLeader, aChild) == nullptr); i++)
    {
        aNexus.AdvanceTime(1);
    }

    VerifyOrQuit(FindQueuedDataResponse(aLeader, aChild) != nullptr);
    aLeader.Get<MeshForwarder>().RemoveMessagesForChild(aChild, Message::AcceptMle<Mle::kCommandDataResponse>);
    VerifyOrQuit(FindQueuedDataResponse(aLeader, aChild) == nullptr);
    VerifyOrQuit(CountOnMeshPrefixes(aSed) == initialPrefixes);
}

static Child &SetupLeaderAndSed(Core &aNexus, Node &aLeader, Node &aSed, Node::JoinMode aJoinMode)
{
    Child   *child;
    uint32_t pollsBefore;

    aLeader.SetName("LEADER");
    aSed.SetName("SED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    AllowLinkBetween(aLeader, aSed);

    aLeader.Form();
    aNexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(aLeader.Get<Mle::Mle>().IsLeader());

    aSed.Join(aLeader, aJoinMode);
    aNexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(aSed.Get<Mle::Mle>().IsChild());

    aSed.Get<LinkMetrics::Initiator>().SetReportCallback(HandleLinkMetricsReport, nullptr);

    child = aLeader.Get<ChildTable>().FindChild(aSed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);

    aNexus.AdvanceTime(kSettleTime);
    pollsBefore = GetTxDataPollCount(aSed);
    aNexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(aSed) == pollsBefore);

    return *child;
}

void TestChildUpdateRequestCancelsMleFastPolls(void)
{
    Core     nexus;
    Node    &leader = nexus.CreateNode();
    Node    &sed    = nexus.CreateNode();
    Child   &child  = SetupLeaderAndSed(nexus, leader, sed, Node::kAsSed);
    uint32_t pollsBefore;
    uint8_t  expectedVersion;

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 1: Unicast Child Update Request satisfying Data Request cancels MLE fast polls");

    Log("Case 1A: Child Update Request stops standalone MLE fast polling");

    TriggerTrackedDataRequestAndDropResponse(nexus, leader, sed, child, /* aPrefixIndex */ 1, /* aStable */ true);

    // Trigger an immediate unicast Child Update Request carrying the updated Network Data.
    AddOnMeshPrefix(leader, /* aPrefixIndex */ 2, /* aStable */ true);
    nexus.AdvanceTime(1);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, child) != nullptr);

    expectedVersion = leader.Get<NetworkData::Leader>().GetVersion(child.GetNetworkDataType());

    // Wait for SED to poll and receive Child Update Request, plus its follow-up Child Update exchange.
    nexus.AdvanceTime(kChildUpdateSettle);
    VerifyOrQuit(CountOnMeshPrefixes(sed) == 2);
    VerifyOrQuit(child.GetNetworkDataVersion() == expectedVersion);
    VerifyOrQuit(child.GetIndirectMessageCount() == 0);

    // Fast polling must already be stopped so no polls occur during the idle window.
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    Log("Case 1B: Child Update Request clears kRequesterMle without blocking other requesters");

    TriggerTrackedDataRequestAndDropResponse(nexus, leader, sed, child, /* aPrefixIndex */ 3, /* aStable */ true);

    sed.Get<DataPollSender>().RequestFastPolls(DataPollSender::kRequesterUser, 12);

    AddOnMeshPrefix(leader, /* aPrefixIndex */ 4, /* aStable */ true);
    nexus.AdvanceTime(1);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, child) != nullptr);

    expectedVersion = leader.Get<NetworkData::Leader>().GetVersion(child.GetNetworkDataType());

    // Advance past Child Update Request delivery and SED's follow-up Child Update exchange.
    nexus.AdvanceTime(kChildUpdateSettle);
    VerifyOrQuit(CountOnMeshPrefixes(sed) == 4);
    VerifyOrQuit(child.GetNetworkDataVersion() == expectedVersion);
    VerifyOrQuit(child.GetIndirectMessageCount() == 0);

    // `kRequesterUser` must still keep fast polling active.
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 1);

    // Cancelling `kRequesterUser` alone must stop fast polling immediately because
    // `kRequesterMle` was already cancelled by `RetxTracker`.
    sed.Get<DataPollSender>().CancelFastPolls(DataPollSender::kRequesterUser);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);
}

void TestRoleChangeToChildCancelsMleFastPolls(void)
{
    Core     nexus;
    Node    &leader = nexus.CreateNode();
    Node    &sed    = nexus.CreateNode();
    Child   &child  = SetupLeaderAndSed(nexus, leader, sed, Node::kAsSedWithFullNetData);
    uint32_t pollsBefore;

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 2: RetxTracker reset on UpdateOnRoleChangeToChild() cancels MLE fast polls");

    // Use a non-stable prefix so switching from full to stable-only Network Data leaves
    // the SED's stable version already up to date with the Leader.
    TriggerTrackedDataRequestAndDropResponse(nexus, leader, sed, child, /* aPrefixIndex */ 1, /* aStable */ false);

    // Switching device mode while attached as a sleepy child invokes `SetStateChild()`
    // -> `RetxTracker::UpdateOnRoleChangeToChild()`, which resets `mDataRequest` and
    // must cancel `kRequesterMle`.
    SuccessOrQuit(sed.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(0)));

    nexus.AdvanceTime(kChildUpdateSettle);
    VerifyOrQuit(child.GetIndirectMessageCount() == 0);
    VerifyOrQuit(sed.Get<Mle::Mle>().GetNetworkDataType() == NetworkData::kStableSubset);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);
}

void TestUnrelatedChildUpdatePreservesLinkMetricsFastPolls(void)
{
    Core     nexus;
    Node    &leader = nexus.CreateNode();
    Node    &sed    = nexus.CreateNode();
    Child   &child  = SetupLeaderAndSed(nexus, leader, sed, Node::kAsSed);
    uint32_t pollsBefore;

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 3: Unrelated Child Update does not cancel Link Metrics MLE fast polls");

    // Initiate a Link Metrics Single Probe (uses `kRequesterMle` fast polls while
    // `RetxTracker::mDataRequest` remains `kIdle`) and drop Leader's Data Response.
    pollsBefore = GetTxDataPollCount(sed);
    SendLinkMetricsSingleProbe(sed, leader);

    for (uint32_t i = 0; (i < kShortWaitSteps) && (FindQueuedDataResponse(leader, child) == nullptr); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(FindQueuedDataResponse(leader, child) != nullptr);
    leader.Get<MeshForwarder>().RemoveMessagesForChild(child, Message::AcceptMle<Mle::kCommandDataResponse>);

    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 1);
    VerifyOrQuit(!sReportReceived);

    // Trigger an unrelated Child Update Request/Response exchange whose `HandleLeaderData()`
    // calls `RetxTracker::UpdateOnDataResponseRx()` while `mDataRequest` is idle.
    SuccessOrQuit(sed.Get<Mle::Mle>().SendChildUpdateRequestToParent());
    nexus.AdvanceTime(kChildUpdateResponseWait);
    VerifyOrQuit(child.GetIndirectMessageCount() == 0);

    // `kRequesterMle` must remain active so fast polls continue for the Link Metrics probe.
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(2 * kFastPollPeriod);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore + 2);

    nexus.AdvanceTime(kIdleCheckWindow);
    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);

    // Verify a normal Link Metrics Single Probe Data Response still cancels `kRequesterMle`
    // in `HandleDataResponse()`.
    SendLinkMetricsSingleProbe(sed, leader);
    nexus.AdvanceTime(kFirstFastPollWait);
    VerifyOrQuit(sReportReceived);

    pollsBefore = GetTxDataPollCount(sed);
    nexus.AdvanceTime(kIdleCheckWindow);
    VerifyOrQuit(GetTxDataPollCount(sed) == pollsBefore);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildUpdateRequestCancelsMleFastPolls();
    ot::Nexus::TestRoleChangeToChildCancelsMleFastPolls();
    ot::Nexus::TestUnrelatedChildUpdatePreservesLinkMetricsFastPolls();

    printf("All tests passed\n");
    return 0;
}
