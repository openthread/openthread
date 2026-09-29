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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/key_manager.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * Time::kOneSecondInMsec;

/**
 * Time to attach as a child and upgrade to router role (>120s).
 */
static constexpr uint32_t kAttachmentTime = 200 * Time::kOneSecondInMsec;

/**
 * Time testing that the leader maintains its state for at least 10s after a policy change.
 */
static constexpr uint32_t kLeaderHoldTime = 9 * Time::kOneSecondInMsec;

/**
 * The shortest interval that allows processing events.
 */
static constexpr uint32_t kShortestTime = 1 * Time::kOneSecondInMsec;

/**
 * Sufficient time for the security policy to be propagated and a router
 * downgrade (>120s) or child timeout (>240s) to occur.
 */
static constexpr uint32_t kPolicyStabilizationTime = 600 * Time::kOneSecondInMsec;

/**
 * Device mode to configure as a Full Thread Device (FTD).
 */
static const Mle::DeviceMode kFtdDeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle |
                                            Mle::DeviceMode::kModeFullThreadDevice |
                                            Mle::DeviceMode::kModeFullNetworkData);

/**
 * Device mode to configure as a Minimal End Device (MED).
 */
static const Mle::DeviceMode kMedDeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData);

void TestRouterDowngradeOnSecPolicyChange(void)
{
    // This test verifies leader and router downgrade delay on security policy version threshold change,
    // and that the Router role is correctly controlled when reconfiguring a FED or MED to FTD after it has attached.
    //
    // Topology:
    //
    // FED -- LEADER -- MED
    //          |
    //          |
    //        ROUTER
    //

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &fed    = nexus.CreateNode();
    Node &med    = nexus.CreateNode();

    // Default router selection jitter (120s) is used with the timing below

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNone));

    Log("---------------------------------------------------------------------------------------");
    Log("Form initial topology");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, fed);
    AllowLinkBetween(leader, med);

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    fed.SetName("FED");
    med.SetName("MED");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router.Join(leader);
    nexus.AdvanceTime(kAttachmentTime);

    fed.Join(leader);
    SuccessOrQuit(fed.Get<Mle::Mle>().SetRouterEligible(false));
    nexus.AdvanceTime(kAttachmentTime);

    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachmentTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouterRoleAllowed());
    // End device nodes initially have their router role disallowed
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("After initial attachment, convert FED to FTD and verify that the router role is now allowed");

    SuccessOrQuit(fed.Get<Mle::Mle>().SetRouterEligible(true));
    VerifyOrQuit(fed.Get<Mle::Mle>().IsRouterRoleAllowed());

    nexus.AdvanceTime(kAttachmentTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("After initial attachment, convert MED to FTD and verify that the router role is now allowed");

    SuccessOrQuit(med.Get<Mle::Mle>().SetDeviceMode(kFtdDeviceMode));
    VerifyOrQuit(med.Get<Mle::Mle>().IsRouterRoleAllowed());

    nexus.AdvanceTime(kAttachmentTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Revert FED and MED to their original configurations, with router role disallowed");

    SuccessOrQuit(fed.Get<Mle::Mle>().SetRouterEligible(false));
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());

    SuccessOrQuit(med.Get<Mle::Mle>().SetDeviceMode(kMedDeviceMode));
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());

    nexus.AdvanceTime(kAttachmentTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Change security policy, disable `R` bit and set version threshold to max value (7)");

    {
        MeshCoP::Dataset::Info datasetInfo;
        SecurityPolicy         policy;
        MeshCoP::Timestamp     timestamp;

        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        policy.SetToDefault();
        policy.mRoutersEnabled             = false;
        policy.mVersionThresholdForRouting = 7;
        datasetInfo.Set<MeshCoP::Dataset::kSecurityPolicy>(policy);

        timestamp.Clear();
        timestamp.SetSeconds(30);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Leader should remain in Leader role for at least 10 seconds");

    nexus.AdvanceTime(kLeaderHoldTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    // Don't verify the router's current role, as it may have downgraded already

    Log("---------------------------------------------------------------------------------------");
    Log("Verify propagated role changes");

    // Active routers now have Router role disallowed
    VerifyOrQuit(!leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!router.Get<Mle::Mle>().IsRouterRoleAllowed());
    // End devices continue to have Router role disallowed
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Change back security policy. This should cancel the ongoing downgrade delay");

    {
        MeshCoP::Dataset::Info datasetInfo;
        SecurityPolicy         policy;
        MeshCoP::Timestamp     timestamp;

        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        policy.SetToDefault();
        policy.mRoutersEnabled             = true;
        policy.mVersionThresholdForRouting = 0;
        datasetInfo.Set<MeshCoP::Dataset::kSecurityPolicy>(policy);

        timestamp.Clear();
        timestamp.SetSeconds(60);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Make sure the Leader device remains in its role");

    nexus.AdvanceTime(kShortestTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Make sure the Router device remains in or restores its role");

    // Advance a sufficiently long time
    nexus.AdvanceTime(kPolicyStabilizationTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Change security policy again, disable `R` bit and set version threshold to max value (7)");

    {
        MeshCoP::Dataset::Info datasetInfo;
        SecurityPolicy         policy;
        MeshCoP::Timestamp     timestamp;

        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        policy.SetToDefault();
        policy.mRoutersEnabled             = false;
        policy.mVersionThresholdForRouting = 7;
        datasetInfo.Set<MeshCoP::Dataset::kSecurityPolicy>(policy);

        timestamp.Clear();
        timestamp.SetSeconds(90);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Leader should remain in Leader role for at least 10 seconds");

    nexus.AdvanceTime(kLeaderHoldTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Make sure both leader and router are downgraded and are now `detached`.");

    nexus.AdvanceTime(kPolicyStabilizationTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(router.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(fed.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(med.Get<Mle::Mle>().IsDetached());

    VerifyOrQuit(!leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!router.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("---------------------------------------------------------------------------------------");
    Log("Verify that the Router role remains disallowed when FED and MED nodes are re-configured as FTDs.");

    SuccessOrQuit(fed.Get<Mle::Mle>().SetRouterEligible(true));
    VerifyOrQuit(!fed.Get<Mle::Mle>().IsRouterRoleAllowed());

    SuccessOrQuit(med.Get<Mle::Mle>().SetDeviceMode(kFtdDeviceMode));
    VerifyOrQuit(!med.Get<Mle::Mle>().IsRouterRoleAllowed());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRouterDowngradeOnSecPolicyChange();
    printf("All tests passed\n");
    return 0;
}
