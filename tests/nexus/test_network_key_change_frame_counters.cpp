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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 20 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Outgoing frame counter of a long-running device, e.g., as restored from non-volatile settings after many reboots.
 */
static constexpr uint32_t kLargeFrameCounter = 50000;

/**
 * Time to wait for a Pending Dataset with the Dataset Updater's default delay to propagate and be applied, in
 * milliseconds.
 */
static constexpr uint32_t kPendingDatasetApplyTime = MeshCoP::DatasetUpdater::kDefaultDelay + 10 * 1000;

void TestNetworkKeyChangeFrameCounters(void)
{
    // A device's outgoing MAC frame counter is tracked both by `KeyManager`
    // (which advertises it in the MLE Link-layer Frame Counter TLV) and, on
    // radios with `OT_RADIO_CAPS_TRANSMIT_SEC`, by the radio (which stamps it
    // on transmitted frames). The radio platform API recommends that
    // `otPlatRadioSetMacKey()` resets the radio's counter, and the Nexus radio
    // does so.
    //
    // This test validates that when the network key is changed on a running
    // device, `KeyManager` resets its own outgoing frame counters as well, so
    // that it agrees with the radio. If it does not, a single-radio device
    // advertises its old (large) counter to new neighbors while the radio
    // sends frames starting from zero. The neighbors then reject all
    // MAC-secured frames from the device as replays, until it reboots.
    //
    // Note: Nexus builds enable multi-radio (15.4 and TREL). With multi-radio,
    // appending the Link-layer Frame Counter TLV raises the radio counter to
    // the `KeyManager` value, which hides the mismatch on the air. So this
    // test checks the counters directly after the key change, before any MLE
    // message is sent.

    Core       nexus;
    NetworkKey newKey;

    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();

    leader.SetName("LEADER");
    child.SetName("CHILD");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Form the network on leader and give it a large outgoing frame counter");

    AllowLinkBetween(leader, child);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    leader.Get<KeyManager>().SetAllMacFrameCounters(kLargeFrameCounter, /* aSetIfLarger */ false);
    leader.Get<KeyManager>().SetMleFrameCounter(kLargeFrameCounter);

    VerifyOrQuit(leader.Get<KeyManager>().GetMaximumMacFrameCounter() == kLargeFrameCounter);
    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Apply a new Active Dataset with a new network key on the running leader");

    {
        MeshCoP::Dataset::Info datasetInfo;
        NetworkKey             appliedKey;
        MeshCoP::Timestamp     timestamp;

        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

        SuccessOrQuit(newKey.GenerateRandom());
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(newKey);

        timestamp = leader.Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();
        timestamp.SetSeconds(timestamp.GetSeconds() + 1);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

        leader.Get<KeyManager>().GetNetworkKey(appliedKey);
        VerifyOrQuit(appliedKey == newKey);

        nexus.AddNetworkKey(newKey);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Verify the leader's own outgoing frame counters restart from zero and match the radio");

    // The key sequence was already zero, so this is checking the network
    // key change itself, not a key sequence change.

    VerifyOrQuit(leader.Get<KeyManager>().GetCurrentKeySequence() == 0);

    // The Nexus radio resets its counter in `otPlatRadioSetMacKey()`. The
    // counter `KeyManager` advertises must match the one the radio uses.

    VerifyOrQuit(leader.mRadio.mRadioContext.mMacFrameCounter == 0);
    VerifyOrQuit(leader.Get<KeyManager>().GetMaximumMacFrameCounter() == 0);
    VerifyOrQuit(leader.Get<KeyManager>().GetMleFrameCounter() == 0);

    Log("---------------------------------------------------------------------------------------");
    Log("Attach a new child using the new key and verify MAC-secured traffic in both directions");

    child.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(child, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Apply a newer Active Dataset with the same network key and verify the counters are kept");

    // `ApplyConfiguration()` calls `SetNetworkKey()` for every Active
    // Dataset that contains a network key. The counters must only be reset
    // when the key actually changes, since outgoing frame counters must not
    // be decreased under the same key.

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;
        uint32_t               macFrameCounter;
        uint32_t               mleFrameCounter;

        macFrameCounter = leader.Get<KeyManager>().GetMaximumMacFrameCounter();
        mleFrameCounter = leader.Get<KeyManager>().GetMleFrameCounter();

        // Traffic since the key change has moved the counters off zero,
        // so a reset would be detected.

        VerifyOrQuit(macFrameCounter > 0);
        VerifyOrQuit(mleFrameCounter > 0);

        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        VerifyOrQuit(datasetInfo.Get<MeshCoP::Dataset::kNetworkKey>() == newKey);

        timestamp = leader.Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();
        timestamp.SetSeconds(timestamp.GetSeconds() + 1);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

        VerifyOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().GetTimestamp() == timestamp);
        VerifyOrQuit(leader.Get<KeyManager>().GetMaximumMacFrameCounter() == macFrameCounter);
        VerifyOrQuit(leader.Get<KeyManager>().GetMleFrameCounter() == mleFrameCounter);
        VerifyOrQuit(leader.mRadio.mRadioContext.mMacFrameCounter >= macFrameCounter);
    }

    nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
}

void TestPendingDatasetKeyChangeFrameCounters(void)
{
    // Changing the network key of a running network through a Pending
    // Dataset applies the new key on every device when the delay timer
    // expires, using the same `KeyManager::SetNetworkKey()` path as above.
    // This also changes the key sequence back to zero, in which case the
    // Thread specification requires the outgoing MAC and MLE frame counters
    // to be set to zero.
    //
    // This test validates that, on both the leader and the child, the
    // outgoing frame counters are reset when the Pending Dataset with the new
    // network key is applied, and match the radio's counter.

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &child  = nexus.CreateNode();

    leader.SetName("LEADER");
    child.SetName("CHILD");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Form the network and attach the child");

    AllowLinkBetween(leader, child);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    child.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Move to key sequence 1 and give both nodes large outgoing frame counters");

    leader.Get<KeyManager>().SetCurrentKeySequence(1, KeyManager::kForceUpdate | KeyManager::kResetGuardTimer);
    nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(node.Get<KeyManager>().GetCurrentKeySequence() == 1);

        node.Get<KeyManager>().SetAllMacFrameCounters(kLargeFrameCounter, /* aSetIfLarger */ false);
        node.Get<KeyManager>().SetMleFrameCounter(kLargeFrameCounter);
    }

    nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Change the network key through a Pending Dataset and wait for it to be applied");

    {
        MeshCoP::Dataset::Info datasetInfo;
        NetworkKey             newKey;

        SuccessOrQuit(newKey.GenerateRandom());

        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(newKey);

        SuccessOrQuit(leader.Get<MeshCoP::DatasetUpdater>().RequestUpdate(datasetInfo, nullptr, nullptr));

        nexus.AddNetworkKey(newKey);
        nexus.AdvanceTime(kPendingDatasetApplyTime);

        for (Node &node : nexus.GetNodes())
        {
            NetworkKey appliedKey;

            node.Get<KeyManager>().GetNetworkKey(appliedKey);
            VerifyOrQuit(appliedKey == newKey);
        }
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Verify outgoing frame counters were reset and match the radio on both nodes");

    // Frames may have been sent since the key change, so the counters
    // are not necessarily zero. Without the reset they would still be at
    // or above `kLargeFrameCounter`, since `KeyManager` only increases them.

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(node.Get<KeyManager>().GetCurrentKeySequence() == 0);
        VerifyOrQuit(node.Get<KeyManager>().GetMaximumMacFrameCounter() < kLargeFrameCounter);
        VerifyOrQuit(node.Get<KeyManager>().GetMleFrameCounter() < kLargeFrameCounter);
        VerifyOrQuit(node.Get<KeyManager>().GetMaximumMacFrameCounter() == node.mRadio.mRadioContext.mMacFrameCounter);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Verify MAC-secured traffic in both directions with the new key");

    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());

    nexus.SendAndVerifyEchoRequest(leader, child.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(child, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestNetworkKeyChangeFrameCounters();
    ot::Nexus::TestPendingDatasetKeyChangeFrameCounters();
    printf("All tests passed\n");
    return 0;
}
