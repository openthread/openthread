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

static constexpr uint32_t kMaxAdvertisementInterval = 32 * 1000;

void TestMleMsgKeySeqJump(void)
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

    Node *nodes[] = {&leader, &child, &reed, &router};

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    for (Node *node : nodes)
    {
        node->Get<KeyManager>().SetCurrentKeySequence(0, KeyManager::kForceUpdate);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Form the network");

    leader.Form();
    nexus.AdvanceTime(15000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Nodes join");

    child.Join(leader, Node::kAsMed);
    reed.Join(leader, Node::kAsFed);
    router.Join(leader, Node::kAsFtd);

    nexus.AdvanceTime(200000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 0);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Manually increase the key seq on child and trigger Child Update Request");

    child.Get<KeyManager>().SetCurrentKeySequence(5, KeyManager::kForceUpdate);
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 5);

    // Trigger Child Update Request by changing mode
    {
        Mle::DeviceMode mode = child.Get<Mle::Mle>().GetDeviceMode();
        mode.Set(mode.Get() ^ Mle::DeviceMode::kModeFullNetworkData);
        SuccessOrQuit(child.Get<Mle::Mle>().SetDeviceMode(mode));
    }
    nexus.AdvanceTime(1000);

    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 5);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 5);

    Log("---------------------------------------------------------------------------------------");
    Log("Wait for MLE Advertisements for other nodes to adopt the new key seq");

    nexus.AdvanceTime(2 * kMaxAdvertisementInterval);
    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 5);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Manually increase the key seq on leader");

    leader.Get<KeyManager>().SetCurrentKeySequence(10, KeyManager::kForceUpdate);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 10);

    // Trigger Child Update Request for child (MED) to adopt the new key sequence.
    {
        Mle::DeviceMode mode = child.Get<Mle::Mle>().GetDeviceMode();
        mode.Set(mode.Get() ^ Mle::DeviceMode::kModeFullNetworkData);
        SuccessOrQuit(child.Get<Mle::Mle>().SetDeviceMode(mode));
    }

    // All nodes should adopt the new key sequence from authoritative advertisements or Child Update.
    // Wait long enough for them to do so.
    nexus.AdvanceTime(3 * kMaxAdvertisementInterval);

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 10);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Stop other nodes, jump leader key seq, and restart them");

    router.Reset();
    reed.Reset();
    child.Reset();

    leader.Get<KeyManager>().SetCurrentKeySequence(15, KeyManager::kForceUpdate);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 15);

    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());
    child.Get<ThreadNetif>().Up();
    SuccessOrQuit(child.Get<Mle::Mle>().Start());
    reed.Get<ThreadNetif>().Up();
    SuccessOrQuit(reed.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(10000);

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<Mle::Mle>().IsAttached());
        VerifyOrQuit(node->Get<KeyManager>().GetCurrentKeySequence() == 15);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Stop nodes other than leader, jump child key seq, and restart child");

    router.Reset();
    reed.Reset();
    child.Reset();

    child.Get<KeyManager>().SetCurrentKeySequence(20, KeyManager::kForceUpdate);
    child.Get<ThreadNetif>().Up();
    SuccessOrQuit(child.Get<Mle::Mle>().Start());
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 20);
    nexus.AdvanceTime(5000);

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 20);

    Log("---------------------------------------------------------------------------------------");
    Log("Restart router and reed");

    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());
    reed.Get<ThreadNetif>().Up();
    SuccessOrQuit(reed.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(10000);

    VerifyOrQuit(router.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsAttached());

    VerifyOrQuit(router.Get<KeyManager>().GetCurrentKeySequence() == 20);
    VerifyOrQuit(reed.Get<KeyManager>().GetCurrentKeySequence() == 20);

    Log("---------------------------------------------------------------------------------------");
    Log("Jump key seq on router and wait for advertisement");

    router.Get<KeyManager>().SetCurrentKeySequence(22, KeyManager::kForceUpdate);
    VerifyOrQuit(router.Get<KeyManager>().GetCurrentKeySequence() == 22);

    nexus.AdvanceTime(2 * kMaxAdvertisementInterval);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 22);
    VerifyOrQuit(reed.Get<KeyManager>().GetCurrentKeySequence() == 22);

    {
        Mle::DeviceMode mode = child.Get<Mle::Mle>().GetDeviceMode();
        mode.Set(mode.Get() ^ Mle::DeviceMode::kModeFullNetworkData);
        SuccessOrQuit(child.Get<Mle::Mle>().SetDeviceMode(mode));
    }
    nexus.AdvanceTime(10000);
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 22);

    Log("---------------------------------------------------------------------------------------");
    Log("Child factory reset and join with higher key seq");

    router.Reset();
    reed.Reset();

    child.Reset();
    child.mSettings.Wipe();
    VerifyOrQuit(child.Get<Mle::Mle>().GetRole() == Mle::kRoleDisabled);

    child.Get<KeyManager>().SetCurrentKeySequence(25, KeyManager::kForceUpdate);
    child.Join(leader, Node::kAsMed);
    VerifyOrQuit(child.Get<KeyManager>().GetCurrentKeySequence() == 25);
    nexus.AdvanceTime(5000);

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 25);

    nexus.SaveTestInfo("test_mle_msg_key_seq_jump.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMleMsgKeySeqJump();
    printf("All tests passed\n");
    return 0;
}
