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

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a sleepy end device to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Long external poll period for controlled sleepy child testing, in milliseconds.
 */
static constexpr uint32_t kLongPollPeriod = 10 * 1000;

/**
 * Time spanning multiple 1-second `Mle::HandleTimeTick()` intervals while child is asleep, in milliseconds.
 */
static constexpr uint32_t kMultipleTicksWaitTime = 2500;

/**
 * Time spanning one `Mle::HandleTimeTick()` interval while staying within the 2-second 6LoWPAN reassembly timeout.
 */
static constexpr uint32_t kMidFragmentTickWaitTime = 1100;

static void AddStableOnMeshPrefix(Node &aNode, uint16_t aIndex)
{
    NetworkData::OnMeshPrefixConfig config;
    char                            prefixString[24];

    config.Clear();
    snprintf(prefixString, sizeof(prefixString), "fd00:%x::/64", aIndex);
    SuccessOrQuit(config.GetPrefix().FromString(prefixString));
    config.mOnMesh = true;
    config.mStable = true;
    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
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

static const Message *FindQueuedChildUpdateRequest(Node &aParent, const Child &aChild)
{
    return aParent.Get<IndirectSender>().FindQueuedMessageForSleepyChild(
        aChild, Message::AcceptMle<Mle::kCommandChildUpdateRequest>);
}

void TestMultiFragmentChildUpdateAcrossTimeTick(void)
{
    /**
     * Scenario 1: Multi-fragment `Child Update Request` spanning across 1-second `Mle::HandleTimeTick()` boundaries.
     *
     * Verifies that when Network Data updates once with 10 stable on-mesh prefixes (producing a 4-fragment MLE
     * `Child Update Request`), periodic 1-second `Mle::HandleTimeTick()` calls to `SynchronizeChildNetworkData()`
     * neither replace the queued message while the SED is asleep nor reset an in-progress fragmented delivery after
     * the first fragment has been sent.
     */

    static constexpr uint16_t kNumPrefixes = 10;

    Core           nexus;
    Node          &leader = nexus.CreateNode();
    Node          &sed    = nexus.CreateNode();
    Child         *child;
    const Message *queuedMsg;
    uint32_t       txUnicast;
    uint32_t       rxIpPackets;
    uint8_t        expectedVersion;

    leader.SetName("LEADER");
    sed.SetName("SED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 1: Multi-fragment Child Update Request across Mle::HandleTimeTick()");

    AllowLinkBetween(leader, sed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    child = leader.Get<ChildTable>().FindChild(sed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);

    // Pause automatic polling so poll timing is controlled explicitly by the test.
    sed.Get<DataPollSender>().StopPolling();
    nexus.AdvanceTime(100);

    Log("Register %u stable on-mesh prefixes on Leader (4-fragment Child Update Request)", kNumPrefixes);

    for (uint16_t i = 1; i <= kNumPrefixes; i++)
    {
        AddStableOnMeshPrefix(leader, i);
    }

    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10);

    expectedVersion = leader.Get<NetworkData::Leader>().GetVersion(child->GetNetworkDataType());
    VerifyOrQuit(child->GetNetworkDataVersion() != expectedVersion);

    queuedMsg = FindQueuedChildUpdateRequest(leader, *child);
    VerifyOrQuit(queuedMsg != nullptr);
    VerifyOrQuit(child->GetIndirectMessageCount() == 1);

    Log("Advance %u ms (> 2 time ticks) while SED is asleep and verify queued message is not replaced",
        kMultipleTicksWaitTime);

    txUnicast = leader.Get<Mac::Mac>().GetCounters().mTxUnicast;
    nexus.AdvanceTime(kMultipleTicksWaitTime);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxUnicast == txUnicast);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) == queuedMsg);
    VerifyOrQuit(child->GetIndirectMessageCount() == 1);

    Log("Send a single Data Poll to deliver fragment 1, then pause before fragment 2");

    rxIpPackets = sed.Get<MeshForwarder>().GetCounters().mRxSuccess;
    sed.Get<DataPollSender>().StartPolling();
    SuccessOrQuit(sed.Get<DataPollSender>().SendDataPoll());
    sed.Get<DataPollSender>().StopPolling();
    nexus.AdvanceTime(20);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxUnicast == txUnicast + 1);
    VerifyOrQuit(child->IsSendingIndirectMsg(*queuedMsg));
    VerifyOrQuit(sed.Get<MeshForwarder>().GetCounters().mRxSuccess == rxIpPackets);

    Log("Advance %u ms across Mle::HandleTimeTick() mid-fragment and verify fragment offset is preserved",
        kMidFragmentTickWaitTime);

    nexus.AdvanceTime(kMidFragmentTickWaitTime);

    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) == queuedMsg);
    VerifyOrQuit(child->IsSendingIndirectMsg(*queuedMsg));
    VerifyOrQuit(child->GetNetworkDataVersion() != expectedVersion);

    Log("Resume polling and verify remaining fragments complete delivery without re-sending fragment 1");

    txUnicast = leader.Get<Mac::Mac>().GetCounters().mTxUnicast;
    sed.Get<DataPollSender>().StartPolling();
    SuccessOrQuit(sed.Get<DataPollSender>().SendDataPoll());
    nexus.AdvanceTime(100);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxUnicast == txUnicast + 3);
    VerifyOrQuit(sed.Get<MeshForwarder>().GetCounters().mRxSuccess == rxIpPackets + 1);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) == nullptr);
    VerifyOrQuit(child->GetIndirectMessageCount() == 0);
    VerifyOrQuit(child->GetNetworkDataVersion() == expectedVersion);
    VerifyOrQuit(CountOnMeshPrefixes(sed) == kNumPrefixes);
}

void TestBackToBackNetworkDataUpdatesWhileSedAsleep(void)
{
    /**
     * Scenario 2: Back-to-back Network Data updates (`V1` then `V2`) while SED is asleep, plus retry after drop.
     *
     * Verifies that:
     * 1. With a SED using a 10 s poll period, registering prefix 1 (`V1`) queues one `Child Update Request` that
     *    remains untouched across two 1-second `Mle::HandleTimeTick()` intervals.
     * 2. Registering prefix 2 (`V2`) while the SED is still asleep removes the outdated `V1` request and queues a
     *    new `V2` request so the SED receives `V2` immediately in a single `Child Update Request` exchange when it
     *    next polls.
     * 3. If a queued `Child Update Request` is dropped before delivery, the next `Mle::HandleTimeTick()` re-queues
     *    a `Child Update Request` so the sleepy child still converges.
     */

    Core           nexus;
    Node          &leader = nexus.CreateNode();
    Node          &sed    = nexus.CreateNode();
    Child         *child;
    const Message *msgV1;
    const Message *msgV2;
    uint16_t       msgV1Length;
    uint32_t       txUnicast;
    uint32_t       rxIpPackets;
    uint8_t        versionV1;
    uint8_t        versionV2;
    uint8_t        versionV3;

    leader.SetName("LEADER");
    sed.SetName("SED");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 2: Back-to-back Network Data updates V1 -> V2 while SED is asleep");

    AllowLinkBetween(leader, sed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    sed.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(kLongPollPeriod));

    // Align right after a keep-alive poll so almost the full 10 s poll period remains.
    SuccessOrQuit(sed.Get<DataPollSender>().SendDataPoll());
    nexus.AdvanceTime(50);

    child = leader.Get<ChildTable>().FindChild(sed.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);

    Log("Register prefix 1 (V1) while SED is asleep");
    AddStableOnMeshPrefix(leader, 1);
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10);

    versionV1 = leader.Get<NetworkData::Leader>().GetVersion(child->GetNetworkDataType());
    msgV1     = FindQueuedChildUpdateRequest(leader, *child);
    VerifyOrQuit(msgV1 != nullptr);
    msgV1Length = msgV1->GetLength();
    VerifyOrQuit(child->GetIndirectMessageCount() == 1);

    Log("Advance %u ms (> 2 time ticks) while SED remains asleep and verify V1 request is kept",
        kMultipleTicksWaitTime);

    txUnicast = leader.Get<Mac::Mac>().GetCounters().mTxUnicast;
    nexus.AdvanceTime(kMultipleTicksWaitTime);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxUnicast == txUnicast);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) == msgV1);
    VerifyOrQuit(child->GetIndirectMessageCount() == 1);

    Log("Register prefix 2 (V2) while SED is still asleep and verify outdated V1 request is replaced");
    AddStableOnMeshPrefix(leader, 2);
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10);

    versionV2 = leader.Get<NetworkData::Leader>().GetVersion(child->GetNetworkDataType());
    VerifyOrQuit(versionV2 != versionV1);

    msgV2 = FindQueuedChildUpdateRequest(leader, *child);
    VerifyOrQuit(msgV2 != nullptr);
    VerifyOrQuit(msgV2->GetLength() > msgV1Length);
    VerifyOrQuit(child->GetIndirectMessageCount() == 1);

    Log("Advance until SED's 10 s poll fires and verify V2 arrives in a single Child Update Request");
    rxIpPackets = sed.Get<MeshForwarder>().GetCounters().mRxSuccess;
    nexus.AdvanceTime(kLongPollPeriod - kMultipleTicksWaitTime);

    VerifyOrQuit(sed.Get<MeshForwarder>().GetCounters().mRxSuccess == rxIpPackets + 1);
    VerifyOrQuit(child->GetIndirectMessageCount() == 0);
    VerifyOrQuit(child->GetNetworkDataVersion() == versionV2);
    VerifyOrQuit(CountOnMeshPrefixes(sed) == 2);

    // Let SED's own follow-up Child Update exchange finish before next step.
    nexus.AdvanceTime(500);

    Log("Register prefix 3 (V3), purge queued request to simulate drop, and verify time tick re-queues it");
    AddStableOnMeshPrefix(leader, 3);
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10);

    versionV3 = leader.Get<NetworkData::Leader>().GetVersion(child->GetNetworkDataType());
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) != nullptr);

    leader.Get<MeshForwarder>().RemoveMessagesForChild(*child, Message::AcceptMle<Mle::kCommandChildUpdateRequest>);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) == nullptr);
    VerifyOrQuit(child->GetNetworkDataVersion() != versionV3);

    nexus.AdvanceTime(kMidFragmentTickWaitTime);
    VerifyOrQuit(FindQueuedChildUpdateRequest(leader, *child) != nullptr);

    nexus.AdvanceTime(kLongPollPeriod);
    VerifyOrQuit(child->GetNetworkDataVersion() == versionV3);
    VerifyOrQuit(CountOnMeshPrefixes(sed) == 3);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMultiFragmentChildUpdateAcrossTimeTick();
    ot::Nexus::TestBackToBackNetworkDataUpdatesWhileSedAsleep();

    printf("All tests passed\n");
    return 0;
}
