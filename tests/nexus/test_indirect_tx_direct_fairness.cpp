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

static constexpr uint16_t kNumSeds               = 20;
static constexpr uint32_t kPollPeriod            = 600;
static constexpr uint16_t kMinQueuedPerSed       = 2;
static constexpr uint16_t kSedEchoPayloadSize    = 250;
static constexpr uint16_t kSedEchoIdentifier     = 0x1111;
static constexpr uint16_t kDirectEchoPayloadSize = 32;
static constexpr uint16_t kDirectEchoIdentifier  = 0x2222;
static constexpr uint32_t kFormNetworkTime       = 15 * 1000;
static constexpr uint32_t kAttachToRouterTime    = 150 * 1000;
static constexpr uint32_t kSedAttachTime         = 10 * 1000;
static constexpr uint32_t kWarmupDuration        = 1000;
static constexpr uint32_t kReplenishInterval     = 10;
static constexpr uint32_t kTestDuration          = 150 * 1000;
static constexpr uint32_t kDirectEchoInterval    = 10 * 1000;

struct DirectEchoContext
{
    uint16_t mExpectedIdentifier;
    uint32_t mReplyCount;
};

static void HandleDirectEchoResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     const otIcmp6Header *aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    DirectEchoContext      *context = static_cast<DirectEchoContext *>(aContext);
    const Ip6::Icmp6Header *header  = AsCoreTypePtr(aIcmpHeader);

    VerifyOrQuit(context != nullptr);
    VerifyOrQuit(header != nullptr);

    if ((header->GetType() == Ip6::Icmp6Header::kTypeEchoReply) && (header->GetId() == context->mExpectedIdentifier))
    {
        context->mReplyCount++;
    }
}

static void ReplenishSedQueues(Node &aLeader, Node *const aSeds[], Child *const aChildren[])
{
    for (uint16_t i = 0; i < kNumSeds; i++)
    {
        uint16_t queued = aChildren[i]->GetIndirectMessageCount();

        while (queued < kMinQueuedPerSed)
        {
            aLeader.SendEchoRequest(aSeds[i]->Get<Mle::Mle>().GetMeshLocalEid(), kSedEchoIdentifier,
                                    kSedEchoPayloadSize);
            queued++;
        }
    }
}

void TestIndirectTxDirectFairness(void)
{
    Core               nexus;
    Node              &leader = nexus.CreateNode();
    Node              &router = nexus.CreateNode();
    Node              *seds[kNumSeds];
    Child             *children[kNumSeds];
    DirectEchoContext  echoContext{kDirectEchoIdentifier, 0};
    Ip6::Icmp::Handler echoHandler(HandleDirectEchoResponse, &echoContext);
    uint32_t           initialTxBroadcast;
    uint32_t           broadcastsSent;
    uint32_t           directEchosSent = 0;

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    for (uint16_t i = 0; i < kNumSeds; i++)
    {
        seds[i] = &nexus.CreateNode();
        seds[i]->SetName("SED", i);
    }

    nexus.AdvanceTime(0);

    // Use Warn log level to avoid excessive logging during sustained high-rate indirect traffic.
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelWarn));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network with Leader, Router, and %u SEDs attached to Leader", kNumSeds);

    AllowLinkBetween(leader, router);

    for (Node *sed : seds)
    {
        AllowLinkBetween(leader, *sed);
    }

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<NeighborTable>().FindNeighbor(leader.Get<Mac::Mac>().GetExtAddress(),
                                                          Neighbor::kInStateValid) != nullptr);

    for (Node *sed : seds)
    {
        sed->Get<Ip6::Icmp>().SetEchoMode(OT_ICMP6_ECHO_HANDLER_DISABLED);
        sed->Join(leader, Node::kAsSed);
        SuccessOrQuit(sed->Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    nexus.AdvanceTime(kSedAttachTime);

    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == kNumSeds);

    for (uint16_t i = 0; i < kNumSeds; i++)
    {
        VerifyOrQuit(seds[i]->Get<Mle::Mle>().IsChild());
        children[i] =
            leader.Get<ChildTable>().FindChild(seds[i]->Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
        VerifyOrQuit(children[i] != nullptr);
    }

    // Prime EID-to-RLOC cache between Leader and Router before starting sustained load.
    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid(), kDirectEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Sustain back-to-back indirect multi-fragment traffic for %lu s", ToUlong(kTestDuration / 1000));

    // Warm up indirect traffic across one full poll period so all SEDs are actively polling back-to-back.
    for (uint32_t elapsed = 0; elapsed < kWarmupDuration; elapsed += kReplenishInterval)
    {
        ReplenishSedQueues(leader, seds, children);
        nexus.AdvanceTime(kReplenishInterval);
    }

    initialTxBroadcast = leader.Get<Mac::Mac>().GetCounters().mTxBroadcast;
    SuccessOrQuit(leader.Get<Ip6::Icmp>().RegisterHandler(echoHandler));

    for (uint32_t elapsed = 0; elapsed < kTestDuration; elapsed += kReplenishInterval)
    {
        ReplenishSedQueues(leader, seds, children);

        if ((elapsed % kDirectEchoInterval) == 0)
        {
            leader.SendEchoRequest(router.Get<Mle::Mle>().GetMeshLocalEid(), kDirectEchoIdentifier,
                                   kDirectEchoPayloadSize);
            directEchosSent++;
        }

        nexus.AdvanceTime(kReplenishInterval);
    }

    SuccessOrQuit(leader.Get<Ip6::Icmp>().UnregisterHandler(echoHandler));

    broadcastsSent = leader.Get<Mac::Mac>().GetCounters().mTxBroadcast - initialTxBroadcast;

    Log("Broadcast frames sent by Leader: %lu", ToUlong(broadcastsSent));
    Log("Direct Echo Replies received on Leader: %lu / %lu", ToUlong(echoContext.mReplyCount),
        ToUlong(directEchosSent));
    Log("Router role: %s, same partition as Leader: %s", router.GetExtendedRoleString(),
        ToYesNo(router.Get<Mle::Mle>().GetLeaderData().GetPartitionId() ==
                leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId()));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Verify direct traffic, MLE advertisements, and neighbor/child states");

    VerifyOrQuit(broadcastsSent > 0);
    VerifyOrQuit(echoContext.mReplyCount == directEchosSent);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().GetLeaderData().GetPartitionId() ==
                 leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId());
    VerifyOrQuit(router.Get<NeighborTable>().FindNeighbor(leader.Get<Mac::Mac>().GetExtAddress(),
                                                          Neighbor::kInStateValid) != nullptr);
    VerifyOrQuit(leader.Get<NeighborTable>().FindNeighbor(router.Get<Mac::Mac>().GetExtAddress(),
                                                          Neighbor::kInStateValid) != nullptr);
    VerifyOrQuit(leader.Get<ChildTable>().GetNumChildren(Child::kInStateValid) == kNumSeds);

    for (Node *sed : seds)
    {
        VerifyOrQuit(sed->Get<Mle::Mle>().IsChild());
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIndirectTxDirectFairness();
    printf("All tests passed\n");
    return 0;
}
