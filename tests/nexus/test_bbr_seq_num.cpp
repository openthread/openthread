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

static constexpr uint32_t kFormNetworkTime        = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime     = 200 * 1000;
static constexpr uint32_t kStabilizationTime      = 15 * 1000;
static constexpr uint32_t kDelayedNetDataSyncTime = 5 * 1000;
static constexpr uint32_t kNetDataResyncTime      = 35 * 1000;
static constexpr uint32_t kDelayedEnableTime      = 60 * 1000;
static constexpr uint8_t  kLeaderWeight           = 72;

struct BbrRoleChangeContext
{
    Node           *mNode        = nullptr;
    Node           *mPeer        = nullptr;
    Mle::DeviceRole mTargetRole  = Mle::kRoleDisabled;
    bool            mUnallowLink = false;
    bool            mEnableBbr   = false;
    bool            mTriggered   = false;
};

static void HandleRoleChangedBeforeNetDataSync(otChangedFlags aFlags, void *aContext)
{
    BbrRoleChangeContext *context = static_cast<BbrRoleChangeContext *>(aContext);

    VerifyOrExit(context != nullptr);
    VerifyOrExit(!context->mTriggered);
    VerifyOrExit((aFlags & OT_CHANGED_THREAD_ROLE) != 0);
    VerifyOrQuit(context->mNode != nullptr);
    VerifyOrExit(context->mNode->Get<Mle::Mle>().GetRole() == context->mTargetRole);

    VerifyOrQuit(context->mNode->Get<Mle::Mle>().IsRetrievingNewNetworkData());
    VerifyOrQuit(!context->mNode->Get<BackboneRouter::Leader>().HasPrimary());

    context->mTriggered = true;

    if (context->mUnallowLink)
    {
        VerifyOrQuit(context->mPeer != nullptr);
        UnallowLinkBetween(*context->mNode, *context->mPeer);
    }

    if (context->mEnableBbr)
    {
        context->mNode->Get<BackboneRouter::Local>().SetEnabled(true);
    }

exit:
    return;
}

struct BbrLocalChangeContext
{
    Node    *mNode                     = nullptr;
    uint32_t mCount                    = 0;
    uint8_t  mExpectedStaleSeqNum      = 0;
    uint8_t  mExpectedRestoredSeqNum   = 0;
    bool     mEnableOnRestore          = false;
    bool     mEnabledWhileStalePresent = false;
};

static void HandleStateChanged(otChangedFlags aFlags, void *aContext)
{
    BbrLocalChangeContext *context = static_cast<BbrLocalChangeContext *>(aContext);
    BackboneRouter::Config localConfig;

    VerifyOrExit(context != nullptr);
    VerifyOrExit((aFlags & OT_CHANGED_THREAD_BACKBONE_ROUTER_LOCAL) != 0);

    context->mCount++;

    VerifyOrExit(context->mEnableOnRestore);
    VerifyOrQuit(context->mNode != nullptr);
    VerifyOrExit(!context->mNode->Get<BackboneRouter::Local>().IsEnabled());

    VerifyOrQuit(context->mNode->Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(context->mNode->Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(context->mNode->Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber ==
                 context->mExpectedStaleSeqNum);

    context->mNode->Get<BackboneRouter::Local>().GetConfig(localConfig);
    VerifyOrQuit(localConfig.mSequenceNumber == context->mExpectedRestoredSeqNum);

    context->mEnableOnRestore          = false;
    context->mEnabledWhileStalePresent = true;
    context->mNode->Get<BackboneRouter::Local>().SetEnabled(true);

exit:
    return;
}

void TestBbrRestoreSeqNumWhileDisabled(void)
{
    /**
     * Verifies that a rebooted Primary Backbone Router (PBBR) restores its
     * previous BBR sequence number (+1), reregistration delay, and MLR timeout
     * from Network Data into RAM immediately upon attaching, even when its
     * local BBR function is initially disabled (waiting for AIL readiness):
     * - Subtest A: Stale BBR Dataset is purged by Leader before BBR is enabled.
     * - Subtest B: BBR is enabled while the stale BBR Dataset is still present
     *   in Network Data (verifies single +1 increment, no double increment).
     * - Subtest C: Sequence number wrap-around (126 -> 0) when restoring while
     *   disabled.
     * - Subtest D: Reboot Router PBBR with SetEnabled(true) called before
     *   attaching (legacy mode) -> restores (old + 1) once without double
     *   incrementing.
     */

    Core                   nexus;
    Node                  &leader = nexus.CreateNode();
    Node                  &br     = nexus.CreateNode();
    BackboneRouter::Config config;
    BbrLocalChangeContext  changeContext;

    leader.SetName("LEADER");
    br.SetName("BR");

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network and establish BR as Primary BBR with custom config");

    AllowLinkBetween(leader, br);

    leader.Get<Mle::Mle>().SetLeaderWeight(kLeaderWeight);
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    br.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

    config.mSequenceNumber      = 40;
    config.mReregistrationDelay = 12;
    config.mMlrTimeout          = 600;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    br.Get<BackboneRouter::Local>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 40);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest A: Reboot BR with BBR disabled, let Leader purge stale entry, then enable later");

    br.Reset();
    AllowLinkBetween(leader, br);
    VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsEnabled());

    changeContext = BbrLocalChangeContext();
    SuccessOrQuit(br.Get<Notifier>().RegisterCallback(HandleStateChanged, &changeContext));

    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsEnabled());

    // Leader must have removed the stale BBR Dataset because BR has BBR disabled.
    VerifyOrQuit(!leader.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(!br.Get<BackboneRouter::Leader>().HasPrimary());

    // Despite BBR still being disabled and Network Data purged, BR must have restored (old + 1) into RAM.
    VerifyOrQuit(changeContext.mCount == 1);
    br.Get<BackboneRouter::Local>().GetConfig(config);
    VerifyOrQuit(config.mSequenceNumber == 41);
    VerifyOrQuit(config.mReregistrationDelay == 12);
    VerifyOrQuit(config.mMlrTimeout == 600);

    // Simulate long delay before infrastructure link becomes active.
    nexus.AdvanceTime(kDelayedEnableTime);
    VerifyOrQuit(!leader.Get<BackboneRouter::Leader>().HasPrimary());

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    br.Get<Notifier>().RemoveCallback(HandleStateChanged, &changeContext);

    VerifyOrQuit(changeContext.mCount == 1);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 41);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest B: Reboot BR with BBR disabled and enable while stale entry is still in Network Data");

    br.Reset();
    AllowLinkBetween(leader, br);
    VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsEnabled());

    // Enable BBR from the Notifier callback before SVR_DATA.ntf purges the stale PBBR entry (41) on Leader.
    changeContext                         = BbrLocalChangeContext();
    changeContext.mNode                   = &br;
    changeContext.mExpectedStaleSeqNum    = 41;
    changeContext.mExpectedRestoredSeqNum = 42;
    changeContext.mEnableOnRestore        = true;
    SuccessOrQuit(br.Get<Notifier>().RegisterCallback(HandleStateChanged, &changeContext));

    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);

    br.Get<Notifier>().RemoveCallback(HandleStateChanged, &changeContext);

    VerifyOrQuit(changeContext.mEnabledWhileStalePresent);
    VerifyOrQuit(changeContext.mCount == 1);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 42);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest C: Sequence number wrap-around (126 -> 0) when restoring while disabled");

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mSequenceNumber = 126;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 126);

    br.Reset();
    AllowLinkBetween(leader, br);
    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(!leader.Get<BackboneRouter::Leader>().HasPrimary());

    br.Get<BackboneRouter::Local>().GetConfig(config);
    VerifyOrQuit(config.mSequenceNumber == 0);

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 0);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest D: Reboot Router PBBR with SetEnabled(true) called before attach");

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mSequenceNumber = 55;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 55);

    br.Reset();
    AllowLinkBetween(leader, br);
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 56);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);
}

void TestBbrDeactivateReactivateWithoutReboot(void)
{
    /**
     * Verifies BBR sequence number management when BBR is deactivated and
     * reactivated at runtime without rebooting:
     * - Subtest A: SetEnabled(false), wait for Network Data removal, then
     *   SetEnabled(true) -> uses incremented RAM sequence number (S + 1).
     * - Subtest B: SetEnabled(false) followed immediately by SetEnabled(true)
     *   before Network Data updates -> uses RAM sequence number (S + 2) without
     *   re-reading stale Network Data or double-incrementing.
     */

    Core                   nexus;
    Node                  &leader = nexus.CreateNode();
    Node                  &br     = nexus.CreateNode();
    BackboneRouter::Config config;

    leader.SetName("LEADER");
    br.SetName("BR");

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("TestBbrDeactivateReactivateWithoutReboot");

    AllowLinkBetween(leader, br);

    leader.Get<Mle::Mle>().SetLeaderWeight(kLeaderWeight);
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mSequenceNumber = 20;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 20);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest A: Disable BBR, wait for Network Data purge, then re-enable");

    br.Get<BackboneRouter::Local>().SetEnabled(false);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().GetState() == BackboneRouter::Local::kStateDisabled);
    br.Get<BackboneRouter::Local>().GetConfig(config);
    VerifyOrQuit(config.mSequenceNumber == 21);

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(!leader.Get<BackboneRouter::Leader>().HasPrimary());

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 21);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Subtest B: Disable BBR and immediately re-enable before Network Data updates");

    br.Get<BackboneRouter::Local>().SetEnabled(false);
    VerifyOrQuit(br.Get<BackboneRouter::Leader>().HasPrimary());
    VerifyOrQuit(br.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 21);

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 22);
}

void TestBbrLeaderRebootAndExplicitSetConfig(void)
{
    /**
     * Verifies:
     * 1. A node acting as both Leader and PBBR restores its sequence number (+1)
     *    across reboots (both when BBR is enabled before attach and when BBR is
     *    enabled after attach).
     * 2. Explicit SetConfig() called before attach is not overwritten by stale
     *    Network Data upon attaching.
     */

    Core                   nexus;
    Node                  &br     = nexus.CreateNode();
    Node                  &router = nexus.CreateNode();
    BackboneRouter::Config config;

    br.SetName("BR_LEADER");
    router.SetName("ROUTER");

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("TestBbrLeaderRebootAndExplicitSetConfig");

    AllowLinkBetween(br, router);

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    router.Join(br);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    br.Get<BackboneRouter::Local>().GetConfig(config);
    config.mSequenceNumber = 75;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 75);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 1: Reboot Leader+PBBR with SetEnabled(true) called before Mle::Start()");

    br.Reset();
    AllowLinkBetween(br, router);
    br.Get<BackboneRouter::Local>().SetEnabled(true);
    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 76);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 2: Reboot Leader+PBBR with BBR disabled, let stale entry purge, then enable");

    br.Reset();
    AllowLinkBetween(br, router);
    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(!router.Get<BackboneRouter::Leader>().HasPrimary());

    br.Get<BackboneRouter::Local>().GetConfig(config);
    VerifyOrQuit(config.mSequenceNumber == 77);

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 77);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Log("Case 3: Explicit SetConfig() before attach is preserved over stale Network Data");

    br.Reset();
    AllowLinkBetween(br, router);

    config.mSequenceNumber      = 99;
    config.mReregistrationDelay = 8;
    config.mMlrTimeout          = 900;
    SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));

    br.Get<ThreadNetif>().Up();
    SuccessOrQuit(br.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    br.Get<BackboneRouter::Local>().GetConfig(config);
    VerifyOrQuit(config.mSequenceNumber == 99);
    VerifyOrQuit(config.mReregistrationDelay == 8);
    VerifyOrQuit(config.mMlrTimeout == 900);

    br.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 99);
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 8);
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 900);
}

void TestBbrNetDataSyncBeforeRegistration(void)
{
    /**
     * Verifies that BackboneRouter::Local does not register a new BBR service
     * before MLE finishes retrieving Network Data after role restoration:
     * - Subtest A: Reboot Router PBBR with SetEnabled(true) before attach,
     *   jitter=1s, and delayed MLE Data Response -> waits for Network Data
     *   sync and restores (old + 1), delay, and timeout.
     * - Subtest B: Reboot Router PBBR with BBR disabled, then call
     *   SetEnabled(true) immediately upon role change before Network Data
     *   arrives -> defers registration until Network Data sync and restores
     *   (old + 1), delay, and timeout.
     * - Subtest C: Reboot Router with SetEnabled(true) before attach when no
     *   PBBR exists in Network Data -> registers and becomes Primary after
     *   Network Data sync.
     * - Subtest D: Reboot Leader+PBBR with SetEnabled(true) before attach,
     *   jitter=1s, and delayed MLE Data Response -> waits for Network Data
     *   sync and restores (old + 1), delay, and timeout.
     * - Subtest E: Reboot Leader with SetEnabled(true) before attach when no
     *   PBBR exists in Network Data -> registers and becomes Primary after
     *   Network Data sync.
     */

    {
        Core                   nexus;
        Node                  &leader = nexus.CreateNode();
        Node                  &br     = nexus.CreateNode();
        BackboneRouter::Config config;
        BbrRoleChangeContext   roleContext;

        leader.SetName("LEADER");
        br.SetName("BR");

        nexus.AdvanceTime(0);
        SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

        Log("---------------------------------------------------------------------------------------");
        Log("TestBbrNetDataSyncBeforeRegistration (Router)");

        AllowLinkBetween(leader, br);

        leader.Get<Mle::Mle>().SetLeaderWeight(kLeaderWeight);
        leader.Form();
        nexus.AdvanceTime(kFormNetworkTime);
        VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

        br.Join(leader);
        nexus.AdvanceTime(kAttachToRouterTime);
        VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

        config.mSequenceNumber      = 55;
        config.mReregistrationDelay = 12;
        config.mMlrTimeout          = 600;
        SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
        br.Get<BackboneRouter::Local>().SetEnabled(true);
        nexus.AdvanceTime(kStabilizationTime);

        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 55);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Subtest A: Router PBBR reboot with delayed MLE Data Response and registration jitter 1s");

        br.Reset();
        AllowLinkBetween(leader, br);
        br.Get<BackboneRouter::Local>().SetRegistrationJitter(1);
        br.Get<BackboneRouter::Local>().SetEnabled(true);

        roleContext              = BbrRoleChangeContext();
        roleContext.mNode        = &br;
        roleContext.mPeer        = &leader;
        roleContext.mTargetRole  = Mle::kRoleRouter;
        roleContext.mUnallowLink = true;
        SuccessOrQuit(br.Get<Notifier>().RegisterCallback(HandleRoleChangedBeforeNetDataSync, &roleContext));

        br.Get<ThreadNetif>().Up();
        SuccessOrQuit(br.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(kDelayedNetDataSyncTime);
        br.Get<Notifier>().RemoveCallback(HandleRoleChangedBeforeNetDataSync, &roleContext);

        VerifyOrQuit(roleContext.mTriggered);
        VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
        VerifyOrQuit(!br.Get<BackboneRouter::Leader>().HasPrimary());
        VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 55);

        AllowLinkBetween(leader, br);
        nexus.AdvanceTime(kNetDataResyncTime);

        VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 56);
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Subtest B: Router PBBR reboot with SetEnabled(true) called on role change before netdata sync");

        br.Reset();
        AllowLinkBetween(leader, br);
        VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsEnabled());

        roleContext             = BbrRoleChangeContext();
        roleContext.mNode       = &br;
        roleContext.mTargetRole = Mle::kRoleRouter;
        roleContext.mEnableBbr  = true;
        SuccessOrQuit(br.Get<Notifier>().RegisterCallback(HandleRoleChangedBeforeNetDataSync, &roleContext));

        br.Get<ThreadNetif>().Up();
        SuccessOrQuit(br.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(kStabilizationTime);
        br.Get<Notifier>().RemoveCallback(HandleRoleChangedBeforeNetDataSync, &roleContext);

        VerifyOrQuit(roleContext.mTriggered);
        VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 57);
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Subtest C: Router reboot with SetEnabled(true) when no PBBR exists in Network Data");

        br.Get<BackboneRouter::Local>().SetEnabled(false);
        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(!leader.Get<BackboneRouter::Leader>().HasPrimary());

        br.Reset();
        AllowLinkBetween(leader, br);
        br.Get<BackboneRouter::Local>().SetEnabled(true);
        br.Get<ThreadNetif>().Up();
        SuccessOrQuit(br.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(leader.Get<BackboneRouter::Leader>().HasPrimary());
    }

    {
        Core                   nexus;
        Node                  &br     = nexus.CreateNode();
        Node                  &router = nexus.CreateNode();
        BackboneRouter::Config config;
        BbrRoleChangeContext   roleContext;

        br.SetName("BR_LEADER");
        router.SetName("ROUTER");

        nexus.AdvanceTime(0);
        SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

        Log("---------------------------------------------------------------------------------------");
        Log("TestBbrNetDataSyncBeforeRegistration (Leader)");

        AllowLinkBetween(br, router);

        br.Form();
        nexus.AdvanceTime(kFormNetworkTime);
        VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

        router.Join(br);
        nexus.AdvanceTime(kAttachToRouterTime);
        VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

        config.mSequenceNumber      = 75;
        config.mReregistrationDelay = 12;
        config.mMlrTimeout          = 600;
        SuccessOrQuit(br.Get<BackboneRouter::Local>().SetConfig(config));
        br.Get<BackboneRouter::Local>().SetEnabled(true);
        nexus.AdvanceTime(kStabilizationTime);

        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 75);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Subtest D: Leader+PBBR reboot with delayed MLE Data Response and registration jitter 1s");

        br.Reset();
        AllowLinkBetween(br, router);
        br.Get<BackboneRouter::Local>().SetRegistrationJitter(1);
        br.Get<BackboneRouter::Local>().SetEnabled(true);

        roleContext              = BbrRoleChangeContext();
        roleContext.mNode        = &br;
        roleContext.mPeer        = &router;
        roleContext.mTargetRole  = Mle::kRoleLeader;
        roleContext.mUnallowLink = true;
        SuccessOrQuit(br.Get<Notifier>().RegisterCallback(HandleRoleChangedBeforeNetDataSync, &roleContext));

        br.Get<ThreadNetif>().Up();
        SuccessOrQuit(br.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(kDelayedNetDataSyncTime);
        br.Get<Notifier>().RemoveCallback(HandleRoleChangedBeforeNetDataSync, &roleContext);

        VerifyOrQuit(roleContext.mTriggered);
        VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());
        VerifyOrQuit(!br.Get<BackboneRouter::Leader>().HasPrimary());
        VerifyOrQuit(!br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 75);

        AllowLinkBetween(br, router);
        nexus.AdvanceTime(kNetDataResyncTime);

        VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());
        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mSequenceNumber == 76);
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mReregistrationDelay == 12);
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().GetConfig().mMlrTimeout == 600);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        Log("Subtest E: Leader reboot with SetEnabled(true) when no PBBR exists in Network Data");

        br.Get<BackboneRouter::Local>().SetEnabled(false);
        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(!router.Get<BackboneRouter::Leader>().HasPrimary());

        br.Reset();
        AllowLinkBetween(br, router);
        br.Get<BackboneRouter::Local>().SetEnabled(true);
        br.Get<ThreadNetif>().Up();
        SuccessOrQuit(br.Get<Mle::Mle>().Start());

        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());
        VerifyOrQuit(br.Get<BackboneRouter::Local>().IsPrimary());
        VerifyOrQuit(router.Get<BackboneRouter::Leader>().HasPrimary());
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBbrRestoreSeqNumWhileDisabled();
    ot::Nexus::TestBbrDeactivateReactivateWithoutReboot();
    ot::Nexus::TestBbrLeaderRebootAndExplicitSetConfig();
    ot::Nexus::TestBbrNetDataSyncBeforeRegistration();
    printf("All tests passed\n");
    return 0;
}
