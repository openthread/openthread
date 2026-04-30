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

#include "meshcop/dataset_manager.hpp"
#include "meshcop/dataset_updater.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/key_manager.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 200 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

void TestKeyRotationGuardTime(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    child.SetName("CHILD");
    reed.SetName("REED");
    router.SetName("ROUTER");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Form the network");

    AllowLinkBetween(leader, child);
    AllowLinkBetween(leader, reed);
    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    child.Join(leader, Node::kAsMed);
    reed.Join(leader, Node::kAsFed);
    router.Join(leader, Node::kAsFtd);

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Validate initial key sequence counter and key switch guard time");

    Node *nodes[] = {&leader, &child, &reed, &router};

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 0);
        VerifyOrQuit(node->Get<KeyManager>().GetKeySwitchGuardTime() == 624);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Change the key rotation time and verify key switch guard time");

    uint16_t rotationTimes[] = {100, 1, 10, 888, 2};

    for (uint16_t rotationTime : rotationTimes)
    {
        MeshCoP::Dataset::Info datasetInfo;

        datasetInfo.Clear();
        datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().SetToDefault();
        datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().mRotationTime = rotationTime;

        SuccessOrQuit(leader.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));

        nexus.AdvanceTime(301 * 1000); // Wait for propagation (default delay is 300s)

        uint16_t effectiveRotationTime =
            (rotationTime < SecurityPolicy::kMinKeyRotationTime) ? SecurityPolicy::kMinKeyRotationTime : rotationTime;
        static constexpr uint32_t kKeySwitchGuardTimePercentage = 93;
        uint16_t                  expectedGuardTime =
            static_cast<uint16_t>(static_cast<uint32_t>(effectiveRotationTime) * kKeySwitchGuardTimePercentage / 100);

        for (Node *node : nodes)
        {
            VerifyOrQuit(node->Get<KeyManager>().GetKeySwitchGuardTime() == expectedGuardTime);
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Wait for automatic rotation to 1");

    // All nodes should already have rotationTime = 2 and guardTime = 1 hour from the end of the previous loop.
    // Trigger rotation by waiting 2 hours.
    nexus.AdvanceTime(2 * 3600 * 1000);

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 1);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Manually increment key sequence counter on `router` and verify guard time prevents update on others");

    router.Get<KeyManager>().SetCurrentKeySequence(2, KeyManager::kForceUpdate | KeyManager::kResetGuardTimer);

    // Advance 20 minutes. Guard timer is still active on others.
    nexus.AdvanceTime(20 * 60 * 1000);

    VerifyOrQuit(router.Get<KeyManager>().GetCurrentKeySequence() == 2);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 1);
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 1);
    VerifyOrQuit(reed.Get<KeyManager>().GetCurrentKeySequence() == 1);

    Log("---------------------------------------------------------------------------------------");
    Log("Verify nodes can still communicate");

    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(router, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Wait for guard time to expire and verify all nodes update");

    // Total wait of 4 hours since manual update of router.
    // T+1h: others' guard timer expires, they update to 2.
    // T+2h: router rotates to 3.
    // T+3h: others' guard timer expires, they update to 3.
    nexus.AdvanceTime(4 * 3600 * 1000);

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() >= 3);
    }

    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(router, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_key_rotation_guard_time.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestKeyRotationGuardTime();
    printf("All tests passed\n");
    return 0;
}
