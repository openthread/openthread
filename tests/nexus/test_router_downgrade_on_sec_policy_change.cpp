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

void TestRouterDowngradeOnSecPolicyChange(void)
{
    // This test verifies leader and router downgrade delay on security policy version threshold change.
    //
    // Topology:
    //
    //     LEADER
    //       |
    //       |
    //     ROUTER
    //

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    VerifyOrQuit(leader.Get<Mle::Mle>().SetRouterSelectionJitter(10) == kErrorNone);
    VerifyOrQuit(router.Get<Mle::Mle>().SetRouterSelectionJitter(10) == kErrorNone);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNone));

    Log("---------------------------------------------------------------------------------------");
    Log("Form initial topology");

    AllowLinkBetween(leader, router);

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    leader.Form();
    nexus.AdvanceTime(13 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(300 * Time::kOneSecondInMsec);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouterRoleAllowed());

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

    // Wait for the dataset to propagate and be processed.
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // We check that security policy is propagated to and applied on
    // leader and router. This means `IsRouterRoleAllowed()` should be
    // false on both.
    VerifyOrQuit(!leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!router.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("Leader should stay as leader for at least 10 seconds");
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

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

    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("Make sure both devices stay as leader and router");

    nexus.AdvanceTime(600 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouterRoleAllowed());

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

    // Wait for the dataset to propagate and be processed.
    nexus.AdvanceTime(5 * Time::kOneSecondInMsec);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // We check that security policy is propagated to and applied on
    // leader and router. This means `IsRouterRoleAllowed()` should be
    // false on both.
    VerifyOrQuit(!leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!router.Get<Mle::Mle>().IsRouterRoleAllowed());

    Log("Leader should stay as leader for at least 10 seconds");
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("Make sure both leader and router are downgraded and are now `detached`.");

    nexus.AdvanceTime(150 * Time::kOneSecondInMsec);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsDetached());
    VerifyOrQuit(router.Get<Mle::Mle>().IsDetached());

    VerifyOrQuit(!leader.Get<Mle::Mle>().IsRouterRoleAllowed());
    VerifyOrQuit(!router.Get<Mle::Mle>().IsRouterRoleAllowed());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestRouterDowngradeOnSecPolicyChange();
    printf("All tests passed\n");
    return 0;
}
