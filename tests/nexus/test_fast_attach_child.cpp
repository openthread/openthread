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
#include "platform/nexus_observer.hpp"

#if OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE

namespace ot {
namespace Nexus {

/**
 * Fast Attach: verify the Child-side early-select behavior.
 *
 * Two FED nodes attach (sequentially, to avoid radio interference) to the same
 * single-router partition. The first has Fast Attach enabled explicitly via
 * SetFastAttachEnabled(true) before Join; the second keeps the default
 * (disabled). The fast-attach child must complete attach noticeably sooner than
 * the legacy child because it sets the F flag, the Leader uses reduced jitter,
 * and the child early-selects on the first acceptable LQ3 Parent Response. The
 * legacy child sets no F flag and waits the full kParentRequestRouterTimeout
 * (750 ms) before selecting.
 *
 * Per-DUT attach time is measured from the DUT's first transmitted frame (its
 * first Parent Request) until IsChild(), polling in 1 ms steps. Measuring from
 * the first Parent Request excludes the random first-attach start delay
 * (uniform [0, 750] ms), so the bounds are deterministic: the slow DUT cannot
 * select a parent before kParentRequestRouterTimeout (750 ms), while the fast
 * DUT only waits for the Leader's reduced response jitter (at most 32 ms with
 * one router) plus the Child ID Request/Response exchange.
 *
 * The test also verifies that after a successful attach the fast-attach flag is
 * auto-cleared, so a subsequent attach attempt would not carry F unless
 * explicitly re-armed.
 *
 * Topology:
 *   LEADER -- FAST_DUT  (Fast Attach explicitly enabled, joins first)
 *   LEADER -- SLOW_DUT  (Fast Attach default-disabled, joins after settle)
 */

static constexpr uint32_t kPollStepMs      = 50;
static constexpr uint32_t kSettleTimeMs    = 2 * Time::kOneSecondInMsec;
static constexpr uint32_t kMaxAttachTimeMs = 3 * Time::kOneSecondInMsec;
// The legacy path cannot select a parent before kParentRequestRouterTimeout
// (750 ms) from queuing the Parent Request. The measurement starts at the
// frame transmission (after CSMA backoff), so allow a small margin.
static constexpr uint32_t kMinLegacyAttachMs = 700;
static constexpr uint32_t kMaxFastAttachMs   = 200; // Well below kMinLegacyAttachMs
// Fast Attach skips the random first-attach start delay (uniform [1, 750] ms),
// so the first Parent Request is sent right after Join() (plus CSMA backoff).
static constexpr uint32_t kMaxFastStartDelayMs = 50;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3
// Two Parent Requests to Routers precede the first one to Routers and REEDs.
static constexpr uint32_t kReedRequestStartMs = 2 * 750;
#else
static constexpr uint32_t kReedRequestStartMs = 750;
#endif

// The DUT hears nothing until just before its first Parent Request to Routers and REEDs.
static constexpr uint32_t kOpenLinksAtMs    = kReedRequestStartMs - 100;
static constexpr uint32_t kMaxFirstTxWaitMs = 1000;
static constexpr uint32_t kMaxTotalWaitMs   = 6 * Time::kOneSecondInMsec;

// Enough time for a full attach cycle (Parent Requests to Routers and REEDs,
// plus Announce on all channels for an MTD) to end without finding a parent.
static constexpr uint32_t kAttachCycleFailTimeMs = 20 * Time::kOneSecondInMsec;

// Puts the node in allowlist mode with an empty list, so it cannot hear any
// other node.
static void Isolate(Node &aNode) { aNode.Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist); }

// Records the time of the first frame transmitted by a given node.
class FirstTxObserver : public Observer
{
public:
    explicit FirstTxObserver(Core &aNexus)
        : mNexus(aNexus)
        , mNodeId(0)
        , mSeen(false)
        , mFirstTxTime(0)
    {
    }

    void     Arm(Node &aNode) { mNodeId = aNode.GetInstance().GetId(), mSeen = false; }
    bool     HasSeenTx(void) const { return mSeen; }
    uint64_t GetFirstTxTime(void) const { return mFirstTxTime; }

    void OnPacketEvent(uint32_t aSenderId, uint32_t, const uint8_t *, uint16_t) override
    {
        if (!mSeen && (aSenderId == mNodeId))
        {
            mSeen        = true;
            mFirstTxTime = mNexus.GetNowMicro64();
        }
    }

    void OnNodeStateChanged(Node *) override {}
    void OnLinkUpdate(uint32_t, uint32_t, bool) override {}
    void OnClearEvents(void) override {}
    void OnHeartbeat(uint64_t) override {}
    void DumpState(void) override {}
    bool IsConnected(void) const override { return false; }

private:
    Core    &mNexus;
    uint32_t mNodeId;
    bool     mSeen;
    uint64_t mFirstTxTime;
};

// Returns the time in ms from the DUT's first transmitted frame (its first
// Parent Request) until it becomes a child. Fails if the DUT does not attach in time. Also returns
// the time in ms from the call until that first frame in `aStartDelayMs`.
static uint32_t MeasureAttachTime(Core &aNexus, FirstTxObserver &aObserver, Node &aDut, uint32_t &aStartDelayMs)
{
    uint32_t attachTime = 0;
    uint64_t startTime  = aNexus.GetNowMicro64();

    aObserver.Arm(aDut);

    for (uint32_t step = 0; step < kMaxAttachTimeMs; step++)
    {
        aNexus.AdvanceTime(1);

        if (aDut.Get<Mle::Mle>().IsChild())
        {
            VerifyOrQuit(aObserver.HasSeenTx());
            aStartDelayMs = static_cast<uint32_t>((aObserver.GetFirstTxTime() - startTime) / 1000);
            attachTime    = static_cast<uint32_t>((aNexus.GetNowMicro64() - aObserver.GetFirstTxTime()) / 1000);
            break;
        }
    }

    VerifyOrQuit(aDut.Get<Mle::Mle>().IsChild());

    return attachTime;
}

// Returns the time in ms that the DUT keeps its radio on while idle during the
// `aDurationMs` that follow its first Parent Request.
static uint32_t MeasureIdleRxOnTime(Core &aNexus, FirstTxObserver &aObserver, Node &aDut, uint32_t aDurationMs)
{
    uint32_t rxOnMs = 0;

    aObserver.Arm(aDut);

    for (uint32_t waited = 0; !aObserver.HasSeenTx(); waited++)
    {
        VerifyOrQuit(waited < kMaxFirstTxWaitMs);
        aNexus.AdvanceTime(1);
    }

    for (uint32_t ms = 0; ms < aDurationMs; ms++)
    {
        aNexus.AdvanceTime(1);

        if (aDut.Get<MeshForwarder>().GetRxOnWhenIdle())
        {
            rxOnMs++;
        }
    }

    return rxOnMs;
}

void TestFastAttachChild(void)
{
    Core            nexus;
    Node           &leader  = nexus.CreateNode();
    Node           &fastDut = nexus.CreateNode();
    Node           &slowDut = nexus.CreateNode();
    FirstTxObserver observer(nexus);

    leader.SetName("LEADER");
    fastDut.SetName("FAST_DUT");
    slowDut.SetName("SLOW_DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, fastDut);
    AllowLinkBetween(leader, slowDut);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network with LEADER");

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    nexus.AddObserver(observer);

    Log("---------------------------------------------------------------------------------------");
    Log("Run 1: FAST_DUT attaches with Fast Attach explicitly enabled");

    // Default is disabled; verify and then explicitly enable.
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    SuccessOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    VerifyOrQuit(fastDut.Get<Mle::Mle>().IsFastAttachEnabled());

    fastDut.Join(leader, Node::kAsFed);

    uint32_t fastStartDelayMs = 0;
    uint32_t fastAttachMs     = MeasureAttachTime(nexus, observer, fastDut, fastStartDelayMs);
    VerifyOrQuit(fastStartDelayMs <= kMaxFastStartDelayMs);
    VerifyOrQuit(fastDut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!fastDut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());

    // Flag must be auto-cleared after successful attach.
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("Auto-clear verified: IsFastAttachEnabled() is FALSE post-attach");

    // Re-enabling while attached must be rejected.
    VerifyOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(true) == kErrorInvalidState);
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("Setter rejection verified: SetFastAttachEnabled(true) returns kErrorInvalidState when attached");

    // Disabling is always allowed regardless of role.
    SuccessOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(false));
    VerifyOrQuit(!fastDut.Get<Mle::Mle>().IsFastAttachEnabled());

    // Settle so the slow DUT's measurement starts cleanly.
    nexus.AdvanceTime(kSettleTimeMs);

    Log("---------------------------------------------------------------------------------------");
    Log("Run 2: SLOW_DUT attaches with Fast Attach default-disabled");

    VerifyOrQuit(!slowDut.Get<Mle::Mle>().IsFastAttachEnabled());

    slowDut.Join(leader, Node::kAsFed);

    uint32_t slowStartDelayMs = 0;
    uint32_t slowAttachMs     = MeasureAttachTime(nexus, observer, slowDut, slowStartDelayMs);
    VerifyOrQuit(slowDut.Get<Mle::Mle>().IsChild());

    nexus.RemoveObserver(observer);

    // Measured from the first Parent Request, the slow DUT must wait the full
    // kParentRequestRouterTimeout before selecting a parent, while the fast
    // DUT early-selects on the first LQ3 Parent Response.
    Log("From first Parent Request: FAST_DUT attached in %u ms, SLOW_DUT attached in %u ms", fastAttachMs,
        slowAttachMs);
    Log("Join() to first Parent Request: FAST_DUT %u ms, SLOW_DUT %u ms", fastStartDelayMs, slowStartDelayMs);

    VerifyOrQuit(fastAttachMs <= kMaxFastAttachMs);
    VerifyOrQuit(slowAttachMs >= kMinLegacyAttachMs);
}

void TestFastAttachPersistsAcrossFailedCycles(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("DUT (MED, isolated) fails several attach cycles with Fast Attach enabled");

    Isolate(dut);
    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    dut.Join(leader, Node::kAsMed);

    for (uint8_t cycle = 0; cycle < 3; cycle++)
    {
        nexus.AdvanceTime(kAttachCycleFailTimeMs);
        VerifyOrQuit(dut.Get<Mle::Mle>().IsDetached());
        VerifyOrQuit(dut.Get<Mle::Mle>().IsFastAttachEnabled());
    }

    Log("Fast Attach still enabled after failed attach cycles");

    Log("---------------------------------------------------------------------------------------");
    Log("Restore the link; DUT attaches and Fast Attach is cleared");

    AllowLinkBetween(leader, dut);

    for (uint32_t step = 0; step < 20 * 60 && !dut.Get<Mle::Mle>().IsChild(); step++)
    {
        nexus.AdvanceTime(Time::kOneSecondInMsec);
    }

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());
}

void TestFastAttachFailureBecomesLeader(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("DUT (FTD, isolated) attempts attach with Fast Attach enabled");

    Isolate(dut);
    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    dut.Join(leader, Node::kAsFtd);

    nexus.AdvanceTime(kAttachCycleFailTimeMs);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());
    VerifyOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true) == kErrorInvalidState);

    Log("DUT became Leader, Fast Attach not left enabled");
}

void TestFastAttachStopAndRestore(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, dut);

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Stop Thread on DUT while attaching with Fast Attach enabled");

    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    dut.Join(leader, Node::kAsFed);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDetached());

    dut.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kPollStepMs);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsDisabled());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("Fast Attach stays enabled after stop");

    Log("---------------------------------------------------------------------------------------");
    Log("Restart Thread; attach uses Fast Attach and clears it");

    SuccessOrQuit(dut.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Reset DUT and restore previous Child role with Fast Attach enabled");

    dut.Reset();
    AllowLinkBetween(leader, dut);

    // Router eligibility is not persisted, so restore the FED setting to
    // keep the DUT from being promoted to Router after it is restored.
    SuccessOrQuit(dut.Get<Mle::Mle>().SetRouterEligible(false));

    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    dut.Get<ThreadNetif>().Up();
    SuccessOrQuit(dut.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());
    Log("Role restore cleared Fast Attach");
}

void TestFastAttachAnnounceAttach(void)
{
    static constexpr uint8_t kStaleChannel = 11;
    static constexpr uint8_t kNewChannel   = 12;

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(leader, dut);

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;
        Mac::ChannelMask       channelMask;

        SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kNewChannel);
        timestamp.SetSeconds(20);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        channelMask.Clear();
        channelMask.AddChannel(kStaleChannel);
        channelMask.AddChannel(kNewChannel);
        datasetInfo.Set<MeshCoP::Dataset::kChannelMask>(channelMask.GetMask());

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
        VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kStaleChannel);
        timestamp.SetSeconds(10);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        dut.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DUT (MED) with stale dataset on channel %u attaches with Fast Attach enabled", kStaleChannel);

    SuccessOrQuit(dut.Get<Mle::Mle>().SetDeviceMode(
        Mle::DeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData)));
    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
    dut.Get<ThreadNetif>().Up();
    SuccessOrQuit(dut.Get<Mle::Mle>().Start());

    for (uint32_t step = 0; step < 60 * Time::kOneSecondInMsec && !dut.Get<Mle::Mle>().IsChild(); step++)
    {
        // Fast Attach must remain enabled throughout while detached,
        // including across the internal stop/start for the Announce attach.
        VerifyOrQuit(dut.Get<Mle::Mle>().IsFastAttachEnabled());
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mac::Mac>().GetPanChannel() == kNewChannel);
    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());

    Log("DUT attached on channel %u", kNewChannel);
}

void TestFastAttachBelowLinkQuality3(void)
{
    static constexpr int8_t kRssiLinkQuality2 = -85; // Link margin 15 over the -100 dBm noise floor

    Core            nexus;
    Node           &leader = nexus.CreateNode();
    Node           &dut    = nexus.CreateNode();
    FirstTxObserver observer(nexus);

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("DUT hears the Leader over a Link Quality 2 link only for Parent Requests to Routers and REEDs");

    // Parent Requests to Routers only accept a Link Quality 3 candidate, so
    // only the Routers and REEDs phase can distinguish the early-select gate.
    Isolate(dut);
    SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));

    nexus.AddObserver(observer);
    dut.Join(leader, Node::kAsFed);
    observer.Arm(dut);

    for (uint32_t waited = 0; !observer.HasSeenTx(); waited++)
    {
        VerifyOrQuit(waited < kMaxFirstTxWaitMs);
        nexus.AdvanceTime(1);
    }

    nexus.AdvanceTime(kOpenLinksAtMs);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsDetached());

    AllowLinkBetween(dut, leader);
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));

    for (uint32_t waited = 0; !dut.Get<Mle::Mle>().IsChild(); waited++)
    {
        VerifyOrQuit(waited < kMaxTotalWaitMs);
        nexus.AdvanceTime(1);
    }

    nexus.RemoveObserver(observer);

    {
        uint32_t attachMs = static_cast<uint32_t>((nexus.GetNowMicro64() - observer.GetFirstTxTime()) / 1000);

        Log("DUT attached %u ms after its first Parent Request", attachMs);

        VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetTwoWayLinkQuality() < kLinkQuality3);

        // No early selection on the Link Quality 2 response: the DUT waits for
        // the full kParentRequestReedTimeout (1250 ms).
        VerifyOrQuit(attachMs >= kReedRequestStartMs + 1200);
    }

    VerifyOrQuit(!dut.Get<Mle::Mle>().IsFastAttachEnabled());
}

void TestFastAttachSedRxOnHold(void)
{
    static constexpr uint8_t  kNumRouters       = 3;
    static constexpr uint32_t kRequestWaitMs    = 750; // First Parent Request is to Routers only.
    static constexpr uint32_t kAfterWaitMs      = kRequestWaitMs + 150;
    static constexpr uint32_t kMaxFastSedAttach = kRequestWaitMs - 50;

    Core            nexus;
    Node           &leader   = nexus.CreateNode();
    Node           &fastDut  = nexus.CreateNode();
    Node           &slowDut  = nexus.CreateNode();
    Node           &slowRxOn = nexus.CreateNode();
    Node           &fastRxOn = nexus.CreateNode();
    Node           *routers[kNumRouters];
    Node           *responders[kNumRouters + 1];
    FirstTxObserver observer(nexus);

    leader.SetName("LEADER");
    fastDut.SetName("FAST_SED");
    slowDut.SetName("SLOW_SED");

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    responders[0] = &leader;

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i]->Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(5 * Time::kOneSecondInMsec);
        responders[i + 1] = routers[i];
    }

    nexus.AdvanceTime(200 * Time::kOneSecondInMsec);

    for (Node *router : routers)
    {
        VerifyOrQuit(router->Get<Mle::Mle>().IsRouter());
    }

    nexus.AddObserver(observer);

    Log("---------------------------------------------------------------------------------------");
    Log("SLOW_SED (legacy): radio goes to sleep right after the Child ID Request");

    {
        uint32_t startDelayMs = 0;
        uint32_t attachMs;

        slowDut.Join(leader, Node::kAsSed);
        attachMs = MeasureAttachTime(nexus, observer, slowDut, startDelayMs);

        VerifyOrQuit(attachMs >= kMinLegacyAttachMs);
        VerifyOrQuit(!slowDut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
        VerifyOrQuit(!slowDut.Get<MeshForwarder>().GetRxOnWhenIdle());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("FAST_SED: radio stays on until the Parent Request wait expires while it polls, so that the late Parent");
    Log("Responses from the other routers are ACKed and not retransmitted");

    {
        uint32_t startDelayMs  = 0;
        uint32_t retriesBefore = 0;
        uint32_t retriesAfter  = 0;
        uint32_t attachMs;

        for (Node *node : responders)
        {
            retriesBefore += node->Get<Mac::Mac>().GetCounters().mTxRetry;
        }

        SuccessOrQuit(fastDut.Get<Mle::Mle>().SetFastAttachEnabled(true));
        fastDut.Join(leader, Node::kAsSed);
        attachMs = MeasureAttachTime(nexus, observer, fastDut, startDelayMs);

        Log("FAST_SED attached %u ms after its first Parent Request", attachMs);

        // The Child ID Response was retrieved by polling, well before the wait expired.
        VerifyOrQuit(attachMs <= kMaxFastSedAttach);
        VerifyOrQuit(fastDut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
        VerifyOrQuit(fastDut.Get<MeshForwarder>().GetRxOnWhenIdle());

        nexus.AdvanceTime(kAfterWaitMs - attachMs);

        VerifyOrQuit(fastDut.Get<Mle::Mle>().IsChild());
        VerifyOrQuit(!fastDut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
        VerifyOrQuit(!fastDut.Get<MeshForwarder>().GetRxOnWhenIdle());

        for (Node *node : responders)
        {
            retriesAfter += node->Get<Mac::Mac>().GetCounters().mTxRetry;
        }

        Log("Router MAC tx retries during the attach: %u", retriesAfter - retriesBefore);
        VerifyOrQuit(retriesAfter == retriesBefore);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Idle rx-on time from the first Parent Request: Fast Attach must not exceed a legacy attach");

    {
        static constexpr uint32_t kMeasureMs = 2000;
        // The two times are equal up to CSMA backoff and timer granularity.
        static constexpr uint32_t kToleranceMs = 20;

        uint32_t legacyRxOnMs;
        uint32_t fastRxOnMs;

        slowRxOn.Join(leader, Node::kAsSed);
        legacyRxOnMs = MeasureIdleRxOnTime(nexus, observer, slowRxOn, kMeasureMs);

        SuccessOrQuit(fastRxOn.Get<Mle::Mle>().SetFastAttachEnabled(true));
        fastRxOn.Join(leader, Node::kAsSed);
        fastRxOnMs = MeasureIdleRxOnTime(nexus, observer, fastRxOn, kMeasureMs);

        Log("Idle rx-on time: legacy SED %u ms, FAST_SED %u ms", legacyRxOnMs, fastRxOnMs);

        VerifyOrQuit(slowRxOn.Get<Mle::Mle>().IsChild());
        VerifyOrQuit(fastRxOn.Get<Mle::Mle>().IsChild());
        VerifyOrQuit(fastRxOnMs <= legacyRxOnMs + kToleranceMs);
    }

    nexus.RemoveObserver(observer);
}

void TestFastAttachSedRepeatedAttach(void)
{
    static constexpr uint8_t  kNumRouters    = 3;
    static constexpr uint8_t  kNumTrials     = 40;
    static constexpr uint32_t kRequestWaitMs = 750;
    static constexpr uint32_t kAfterWaitMs   = kRequestWaitMs + 150;
    static constexpr uint32_t kStopDuringMs  = 300;

    Core            nexus;
    Node           &leader = nexus.CreateNode();
    Node           &dut    = nexus.CreateNode();
    Node           *routers[kNumRouters];
    Node           *responders[kNumRouters + 1];
    FirstTxObserver observer(nexus);
    uint32_t        heldTrials    = 0;
    uint32_t        stoppedTrials = 0;

    leader.SetName("LEADER");
    dut.SetName("SED");

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);

    responders[0] = &leader;

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i]->Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(5 * Time::kOneSecondInMsec);
        responders[i + 1] = routers[i];
    }

    nexus.AdvanceTime(200 * Time::kOneSecondInMsec);

    nexus.AddObserver(observer);

    Log("---------------------------------------------------------------------------------------");
    Log("Repeated SED Fast Attach: hold is armed and released on every attach, and Stop() clears it");

    for (uint8_t trial = 0; trial < kNumTrials; trial++)
    {
        uint32_t retriesBefore = 0;
        uint32_t retriesAfter  = 0;
        bool     stopDuring    = (trial % 4 == 3);

        for (Node *node : responders)
        {
            retriesBefore += node->Get<Mac::Mac>().GetCounters().mTxRetry;
        }

        SuccessOrQuit(dut.Get<Mle::Mle>().SetFastAttachEnabled(true));
        observer.Arm(dut);

        if (trial == 0)
        {
            dut.Join(leader, Node::kAsSed);
        }
        else
        {
            SuccessOrQuit(dut.Get<Mle::Mle>().Start());
        }

        for (uint32_t waited = 0; !observer.HasSeenTx(); waited++)
        {
            VerifyOrQuit(waited < kMaxFirstTxWaitMs);
            nexus.AdvanceTime(1);
        }

        for (uint32_t waited = 0; !dut.Get<Mle::Mle>().IsChild(); waited++)
        {
            VerifyOrQuit(waited < kMaxAttachTimeMs);
            nexus.AdvanceTime(1);
        }

        {
            uint32_t attachMs = static_cast<uint32_t>((nexus.GetNowMicro64() - observer.GetFirstTxTime()) / 1000);

            VerifyOrQuit(attachMs < kRequestWaitMs);
        }

        // The hold is armed after an early select by a sleepy device.
        VerifyOrQuit(dut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
        VerifyOrQuit(dut.Get<MeshForwarder>().GetRxOnWhenIdle());
        heldTrials++;

        if (stopDuring)
        {
            uint32_t elapsed = static_cast<uint32_t>((nexus.GetNowMicro64() - observer.GetFirstTxTime()) / 1000);

            if (elapsed < kStopDuringMs)
            {
                nexus.AdvanceTime(kStopDuringMs - elapsed);
            }

            // Stopping Thread while the hold is active must clear it.
            dut.Get<Mle::Mle>().Stop();
            VerifyOrQuit(!dut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
            stoppedTrials++;

            // The hold timer must not fire or affect a disabled device.
            nexus.AdvanceTime(kAfterWaitMs);
            VerifyOrQuit(dut.Get<Mle::Mle>().IsDisabled());
            VerifyOrQuit(!dut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
        }
        else
        {
            uint32_t elapsed = static_cast<uint32_t>((nexus.GetNowMicro64() - observer.GetFirstTxTime()) / 1000);

            nexus.AdvanceTime(kAfterWaitMs - elapsed);

            VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
            VerifyOrQuit(!dut.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
            VerifyOrQuit(!dut.Get<MeshForwarder>().GetRxOnWhenIdle());

            dut.Get<Mle::Mle>().Stop();
            nexus.AdvanceTime(3 * Time::kOneSecondInMsec);
        }

        for (Node *node : responders)
        {
            retriesAfter += node->Get<Mac::Mac>().GetCounters().mTxRetry;
        }

        // Every late Parent Response was ACKed on the first attempt.
        VerifyOrQuit(retriesAfter == retriesBefore);
    }

    Log("%u attaches: hold armed in %u, Stop() during the hold in %u", kNumTrials, heldTrials, stoppedTrials);

    nexus.RemoveObserver(observer);
}

struct UdpCounter
{
    Node       *mNode;
    otUdpSocket mSocket;
    uint32_t    mRxCount;

    void Init(Node &aNode, uint16_t aPort)
    {
        otSockAddr bindAddr;

        ClearAllBytes(*this);
        ClearAllBytes(bindAddr);

        mNode          = &aNode;
        bindAddr.mPort = aPort;

        SuccessOrQuit(otUdpOpen(mNode, &mSocket, HandleReceive, this));
        SuccessOrQuit(otUdpBind(mNode, &mSocket, &bindAddr, OT_NETIF_THREAD_INTERNAL));
    }

    void Close(void) { SuccessOrQuit(otUdpClose(mNode, &mSocket)); }

    static void HandleReceive(void *aContext, otMessage *, const otMessageInfo *)
    {
        static_cast<UdpCounter *>(aContext)->mRxCount++;
    }
};

// Sends a link-local multicast UDP datagram, which is a broadcast frame at the MAC layer.
static void SendLinkLocalMulticast(Node &aSender, uint16_t aPort)
{
    Ip6::Udp::Socket socket(aSender, nullptr, nullptr);
    Ip6::MessageInfo messageInfo;
    Message         *message;

    SuccessOrQuit(socket.Open(Ip6::kNetifThreadInternal));
    message = socket.NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Append<uint8_t>(0xa5));

    messageInfo.SetPeerAddr(Ip6::Address::GetLinkLocalAllNodesMulticast());
    messageInfo.SetPeerPort(aPort);

    SuccessOrQuit(socket.SendTo(*message, messageInfo));
    SuccessOrQuit(socket.Close());
}

void TestFastAttachBroadcastFilteredWhileHeld(void)
{
    static constexpr uint16_t kUdpPort = 5000;

    Core       nexus;
    Node      &leader = nexus.CreateNode();
    Node      &med    = nexus.CreateNode();
    Node      &sed    = nexus.CreateNode();
    UdpCounter medRx;
    UdpCounter sedRx;
    uint32_t   filteredBefore;

    leader.SetName("LEADER");
    med.SetName("MED");
    sed.SetName("SED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    medRx.Init(med, kUdpPort);
    sedRx.Init(sed, kUdpPort);

    Log("---------------------------------------------------------------------------------------");
    Log("Control: an rx-on device receives the link-local multicast datagram");

    SendLinkLocalMulticast(leader, kUdpPort);
    nexus.AdvanceTime(100);
    VerifyOrQuit(medRx.mRxCount == 1);

    Log("---------------------------------------------------------------------------------------");
    Log("SED after a Fast Attach early select: broadcast frames are filtered while the radio is only held on");

    SuccessOrQuit(sed.Get<Mle::Mle>().SetFastAttachEnabled(true));
    sed.Join(leader, Node::kAsSed);

    for (uint32_t waited = 0; !sed.Get<Mle::Mle>().IsChild(); waited++)
    {
        VerifyOrQuit(waited < kMaxAttachTimeMs);
        nexus.AdvanceTime(1);
    }

    VerifyOrQuit(sed.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
    VerifyOrQuit(sed.Get<MeshForwarder>().GetRxOnWhenIdle());

    filteredBefore = sed.Get<Mac::Mac>().GetCounters().mRxDestAddrFiltered;

    SendLinkLocalMulticast(leader, kUdpPort);
    nexus.AdvanceTime(50);

    VerifyOrQuit(sedRx.mRxCount == 0);
    VerifyOrQuit(sed.Get<Mac::Mac>().GetCounters().mRxDestAddrFiltered > filteredBefore);
    VerifyOrQuit(sed.Get<MeshForwarder>().IsRxOnWhenIdleHeld());

    Log("---------------------------------------------------------------------------------------");
    Log("Suspending sleepy operation takes precedence over the hold: the radio is then simply rx-on");

    sed.Get<MeshForwarder>().SuspendSleepyOperation(MeshForwarder::kRequesterDiscoverScanner);
    VerifyOrQuit(!sed.Get<MeshForwarder>().IsRxOnWhenIdleHeld());
    VerifyOrQuit(sed.Get<MeshForwarder>().GetRxOnWhenIdle());

    SendLinkLocalMulticast(leader, kUdpPort);
    nexus.AdvanceTime(50);
    VerifyOrQuit(sedRx.mRxCount == 1);

    sed.Get<MeshForwarder>().ResumeSleepyOperation(MeshForwarder::kRequesterDiscoverScanner);
    VerifyOrQuit(sed.Get<MeshForwarder>().IsRxOnWhenIdleHeld());

    medRx.Close();
    sedRx.Close();
}

/**
 * Fast Attach: verify router-side jitter scaling with multiple Routers when the
 * Parent Request has both the E and F flags set.
 *
 * In the first attach cycle the DUT sends two Parent Requests to Routers only
 * (750 ms apart), followed by Parent Requests to Routers and REEDs (E flag
 * set). The DUT hears nothing during the Routers-only requests, and the links
 * are opened just before the first Routers-and-REEDs request. With F set, every
 * Router answers within `N_AR * 32 ms`, so the DUT selects a parent well before
 * the legacy kParentRequestReedTimeout (1250 ms). A control DUT without Fast
 * Attach waits for the full timeout.
 */
void TestFastAttachRoutersAndReeds(void)
{
    // Few responders keep the test sensitive to the router-side jitter: with
    // many, the minimum of the random delays is small even without Fast Attach.
    static constexpr uint8_t kNumRouters       = 1;
    static constexpr uint8_t kNumActiveRouters = kNumRouters + 1; // Including the Leader

    // Fast: N_AR * 32 ms response jitter plus the Child ID exchange and CSMA margin.
    static constexpr uint32_t kMaxFastAttachWithReedsMs = kReedRequestStartMs + kNumActiveRouters * 32 + 100;
    // Legacy: waits kParentRequestReedTimeout (1250 ms), allow a small margin.
    static constexpr uint32_t kMinLegacyAttachWithReedsMs = kReedRequestStartMs + 1200;

    Core            nexus;
    Node           &leader  = nexus.CreateNode();
    Node           &fastDut = nexus.CreateNode();
    Node           &slowDut = nexus.CreateNode();
    Node           *routers[kNumRouters];
    FirstTxObserver observer(nexus);

    leader.SetName("LEADER");
    fastDut.SetName("FAST_DUT");
    slowDut.SetName("SLOW_DUT");

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
    }

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    for (Node *router : routers)
    {
        router->Join(leader, Node::kAsFtd);
        nexus.AdvanceTime(5 * Time::kOneSecondInMsec);
    }

    nexus.AdvanceTime(200 * Time::kOneSecondInMsec);

    for (Node *router : routers)
    {
        VerifyOrQuit(router->Get<Mle::Mle>().IsRouter());
    }

    nexus.AddObserver(observer);

    for (uint8_t run = 0; run < 2; run++)
    {
        Node    *dut      = (run == 0) ? &fastDut : &slowDut;
        bool     isFast   = (run == 0);
        uint32_t attachMs = 0;

        Log("---------------------------------------------------------------------------------------");
        Log("%s attaches with the first Parent Request with E flag heard by %u active Routers",
            isFast ? "FAST_DUT (Fast Attach)" : "SLOW_DUT (legacy)", kNumActiveRouters);

        // The DUT hears no one for its first (Routers only) Parent Request.
        dut->Get<Mac::Filter>().SetMode(Mac::Filter::kModeAllowlist);
        SuccessOrQuit(dut->Get<Mle::Mle>().SetFastAttachEnabled(isFast));

        dut->Join(leader, Node::kAsFed);
        observer.Arm(*dut);

        for (uint32_t waited = 0; !observer.HasSeenTx(); waited++)
        {
            VerifyOrQuit(waited < kMaxFirstTxWaitMs);
            nexus.AdvanceTime(1);
        }

        nexus.AdvanceTime(kOpenLinksAtMs);
        VerifyOrQuit(dut->Get<Mle::Mle>().IsDetached());

        AllowLinkBetween(*dut, leader);

        for (Node *router : routers)
        {
            AllowLinkBetween(*dut, *router);
        }

        for (uint32_t waited = 0; !dut->Get<Mle::Mle>().IsChild(); waited++)
        {
            VerifyOrQuit(waited < kMaxTotalWaitMs);
            nexus.AdvanceTime(1);
        }

        attachMs = static_cast<uint32_t>((nexus.GetNowMicro64() - observer.GetFirstTxTime()) / 1000);
        Log("%s attached %u ms after its first Parent Request", isFast ? "FAST_DUT" : "SLOW_DUT", attachMs);

        if (isFast)
        {
            VerifyOrQuit(attachMs > kOpenLinksAtMs);
            VerifyOrQuit(attachMs <= kMaxFastAttachWithReedsMs);
        }
        else
        {
            VerifyOrQuit(attachMs >= kMinLegacyAttachWithReedsMs);
        }
    }

    nexus.RemoveObserver(observer);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestFastAttachChild();
    ot::Nexus::TestFastAttachPersistsAcrossFailedCycles();
    ot::Nexus::TestFastAttachFailureBecomesLeader();
    ot::Nexus::TestFastAttachStopAndRestore();
    ot::Nexus::TestFastAttachAnnounceAttach();
    ot::Nexus::TestFastAttachBelowLinkQuality3();
    ot::Nexus::TestFastAttachRoutersAndReeds();
    ot::Nexus::TestFastAttachSedRxOnHold();
    ot::Nexus::TestFastAttachSedRepeatedAttach();
    ot::Nexus::TestFastAttachBroadcastFilteredWhileHeld();
    printf("All tests passed\n");
    return 0;
}

#else // OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE

int main(void)
{
    printf("Fast Attach is disabled at compile time; test skipped.\n");
    return 0;
}

#endif // OPENTHREAD_CONFIG_MLE_FAST_ATTACH_ENABLE
