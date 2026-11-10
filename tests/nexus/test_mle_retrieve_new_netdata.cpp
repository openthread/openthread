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
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Maximum 1 ms steps to wait for role restoration after reset.
 */
static constexpr uint32_t kRestoreWaitSteps = 5 * 1000;

/**
 * Time to advance past Network Data sync timeout (`kMaxNetDataSyncWait` = 60s) and singleton leader formation.
 */
static constexpr uint32_t kNetDataSyncTimeoutAndFormTime = 85 * 1000;

/**
 * Time to advance while isolated so Leader's Advertisement trickle timer reaches `Imax` (12s).
 */
static constexpr uint32_t kTrickleMaxIntervalSettleTime = 35 * 1000;

/**
 * Step time and maximum step count when waiting for an `Imax` Advertisement from Leader.
 */
static constexpr uint32_t kAdvWaitStepTime = 50;
static constexpr uint32_t kMaxAdvWaitSteps = 500;

void TestLeaderRestoreTimeoutThenAttachChild(void)
{
    /**
     * Scenario 1: Leader role restoration timeout -> singleton Leader -> Child attach.
     *
     * Verifies that when a Leader restores its role after reset via `Link Accept` (setting
     * `mRetrieveNewNetworkData = true`), loses connectivity before receiving `Data Response`,
     * times out after `kMaxNetDataSyncWait`, detaches, and forms a fresh singleton partition as
     * Leader, `mRetrieveNewNetworkData` is cleared so a new child can attach.
     */

    Core     nexus;
    Node    &leader = nexus.CreateNode();
    Node    &router = nexus.CreateNode();
    Node    &ed1    = nexus.CreateNode();
    uint32_t oldPartitionId;
    uint32_t rxBroadcast;

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    ed1.SetName("ED1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 1: Leader role restoration timeout -> singleton Leader -> Child attach");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    oldPartitionId = leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId();

    Log("Reset Leader and start role restoration");
    leader.Reset();
    AllowLinkBetween(leader, router);
    rxBroadcast = router.Get<Mac::Mac>().GetCounters().mRxBroadcast;
    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && (router.Get<Mac::Mac>().GetCounters().mRxBroadcast == rxBroadcast);
         i++)
    {
        nexus.AdvanceTime(1);
    }

    // Block Leader -> Router frames once Router receives Link Request so Router still sends
    // Link Accept to Leader, while Leader's subsequent Data Request is dropped.
    router.UnallowList(leader);

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && !leader.Get<Mle::Mle>().IsLeader(); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Isolate Leader immediately before Data Response arrives");
    UnallowLinkBetween(leader, router);

    Log("Advance past kMaxNetDataSyncWait so Leader detaches and forms a singleton partition");
    nexus.AdvanceTime(kNetDataSyncTimeoutAndFormTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leader.Get<Mle::Mle>().GetLeaderData().GetPartitionId() != oldPartitionId);

    Log("Join ED1 as MED child to singleton Leader");
    AllowLinkBetween(leader, ed1);
    ed1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsChild());
}

void TestRouterRestoreInterruptedThenReattachAsReedAndAcceptChild(void)
{
    /**
     * Scenario 2: Router role restoration interrupted -> reattach as REED via Child ID Response -> accept Child.
     *
     * Verifies that when a Router restores its role after reset via `Link Accept` (setting
     * `mRetrieveNewNetworkData = true`), detaches before receiving `Data Response`, reattaches
     * as a REED via `Parent Request` / `Child ID Response`, and then upgrades to Router to
     * accept an isolated MED child, `mRetrieveNewNetworkData` is cleared so `Child ID Response`
     * with `NetworkDataTlv` succeeds.
     */

    Core     nexus;
    Node    &leader = nexus.CreateNode();
    Node    &router = nexus.CreateNode();
    Node    &ed2    = nexus.CreateNode();
    uint32_t rxBroadcast;
    uint32_t txBroadcast;

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    ed2.SetName("ED2");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 2: Router role restoration interrupted -> reattach as REED -> accept Child");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("Reset Router and start role restoration");
    router.Reset();
    AllowLinkBetween(leader, router);
    router.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
    rxBroadcast = leader.Get<Mac::Mac>().GetCounters().mRxBroadcast;
    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && (leader.Get<Mac::Mac>().GetCounters().mRxBroadcast == rxBroadcast);
         i++)
    {
        nexus.AdvanceTime(1);
    }

    // Block Router -> Leader frames once Leader receives Link Request so Leader still sends
    // Link Accept to Router, while Router's subsequent Data Request is dropped.
    leader.UnallowList(router);

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && !router.Get<Mle::Mle>().IsRouter(); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("Isolate Router before Data Response arrives and detach");
    UnallowLinkBetween(leader, router);
    SuccessOrQuit(router.Get<Mle::Mle>().BecomeDetached());
    VerifyOrQuit(router.Get<Mle::Mle>().IsDetached());

    Log("Re-allow link to Leader so Router reattaches as REED via Child ID Response");
    AllowLinkBetween(leader, router);

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && !router.Get<Mle::Mle>().IsChild(); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(router.Get<Mle::Mle>().IsChild());

    // Isolate REED while Leader's Advertisement trickle timer settles to Imax (12s) and wait right
    // past an Imax Advertisement so at least 6s remain before the next Leader Advertisement.
    UnallowLinkBetween(leader, router);
    nexus.AdvanceTime(kTrickleMaxIntervalSettleTime);

    txBroadcast = leader.Get<Mac::Mac>().GetCounters().mTxBroadcast;

    for (uint32_t i = 0; (i < kMaxAdvWaitSteps) && (leader.Get<Mac::Mac>().GetCounters().mTxBroadcast == txBroadcast);
         i++)
    {
        nexus.AdvanceTime(kAdvWaitStepTime);
    }

    VerifyOrQuit(leader.Get<Mac::Mac>().GetCounters().mTxBroadcast > txBroadcast);

    Log("Join isolated ED2 to REED so REED upgrades to Router and sends Child ID Response");
    AllowLinkBetween(router, ed2);
    ed2.Join(router, Node::kAsMed);

    for (uint32_t i = 0;
         (i < kRestoreWaitSteps) && !router.Get<ChildTable>().HasChildren(Child::kInStateAnyExceptInvalid); i++)
    {
        nexus.AdvanceTime(1);
    }

    AllowLinkBetween(leader, router);

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && !router.Get<Mle::Mle>().IsRouter(); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    // Isolate Router from Leader after upgrade so subsequent Advertisements from Leader cannot
    // trigger a Data Request/Response exchange that masks stale `mRetrieveNewNetworkData` state.
    UnallowLinkBetween(leader, router);

    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(ed2.Get<Mle::Mle>().IsChild());
}

void TestBecomeLeaderFromChildWhileRetrievingNetData(void)
{
    /**
     * Scenario 3: Direct `BecomeLeader` while attached as Child with `mRetrieveNewNetworkData == true`.
     *
     * Verifies that when a REED child sets `mRetrieveNewNetworkData = true` (after reset and
     * receiving `Child Update Response` without `NetworkDataTlv`), is isolated before receiving
     * `Data Response`, and calls `BecomeLeader(kIgnoreLeaderWeight)` directly without going
     * through `SetStateDetached()`, `mRetrieveNewNetworkData` is cleared so a MED child can attach.
     */

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();
    Node &ed3    = nexus.CreateNode();

    leader.SetName("LEADER");
    reed.SetName("REED");
    ed3.SetName("ED3");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Scenario 3: Direct BecomeLeader while attached as Child with mRetrieveNewNetworkData");

    AllowLinkBetween(leader, reed);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
    reed.Join(leader);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("Reset REED and restore child role via Child Update Response without NetworkDataTlv");
    reed.Reset();
    AllowLinkBetween(leader, reed);
    reed.Get<Mle::Mle>().SetRouterUpgradeThreshold(1);
    reed.Get<ThreadNetif>().Up();
    SuccessOrQuit(reed.Get<Mle::Mle>().Start());

    for (uint32_t i = 0; (i < kRestoreWaitSteps) && !reed.Get<Mle::Mle>().IsChild(); i++)
    {
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    Log("Isolate REED before Data Request/Response and call BecomeLeader directly");
    UnallowLinkBetween(leader, reed);
    SuccessOrQuit(reed.Get<Mle::Mle>().BecomeLeader(Mle::Mle::kIgnoreLeaderWeight));
    VerifyOrQuit(reed.Get<Mle::Mle>().IsLeader());

    Log("Join ED3 as MED child to new Leader");
    AllowLinkBetween(reed, ed3);
    ed3.Join(reed, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(ed3.Get<Mle::Mle>().IsChild());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestLeaderRestoreTimeoutThenAttachChild();
    ot::Nexus::TestRouterRestoreInterruptedThenReattachAsReedAndAcceptChild();
    ot::Nexus::TestBecomeLeaderFromChildWhileRetrievingNetData();

    printf("All tests passed\n");
    return 0;
}
