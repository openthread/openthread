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
#include <string.h>

#include "meshcop/dataset.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 10 * 1000;

/**
 * Time to advance for the DUT to recognize that its parent is gone, in milliseconds.
 */
static constexpr uint32_t kParentLossTime = 60 * 1000;

/**
 * Time to advance for the DUT to send Parent Request and receive no response, in milliseconds.
 */
static constexpr uint32_t kParentSelectionTime = 10 * 1000;

/**
 * Time to advance for the DUT to send MLE Announce and receive MLE Announce response, in milliseconds.
 */
static constexpr uint32_t kAnnounceTime = 10 * 1000;

/**
 * Time to advance for the DUT to attach to a new parent, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 20 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Primary Channel.
 */
static constexpr uint16_t kPrimaryChannel = 11;

/**
 * Secondary Channel.
 */
static constexpr uint16_t kSecondaryChannel = 12;

/**
 * Leader_1 PAN ID.
 */
static constexpr uint16_t kLeader1PanId = 0x1111;

/**
 * Leader_2 PAN ID.
 */
static constexpr uint16_t kLeader2PanId = 0x2222;

/**
 * Leader_1 Active Timestamp.
 */
static constexpr uint64_t kLeader1Timestamp = 10;

/**
 * Leader_2 Active Timestamp.
 */
static constexpr uint64_t kLeader2Timestamp = 20;

/**
 * Network Key.
 */
static const uint8_t kNetworkKey[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

void Test9_2_17(void)
{
    /**
     * 9.2.17 Orphaned End Devices
     *
     * 9.2.17.1 Topology
     * - Leader_1
     * - Leader_2
     * - ED_1 (DUT)
     *
     * 9.2.17.2 Purpose & Description
     * The purpose of this test case to validate end device functionality when its Parent is no longer available and
     *   searches for a new Parent using MLE Announce messages.
     *
     * Spec Reference       | V1.1 Section | V1.3.0 Section
     * ---------------------|--------------|---------------
     * Orphaned End Devices | 8.7.7        | 8.7.7
     */

    Core nexus;

    Node &leader1 = nexus.CreateNode();
    Node &leader2 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader1.SetName("LEADER_1");
    leader2.SetName("LEADER_2");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Form the two topologies and ensure the DUT is attached to Leader_1
     * - Pass Criteria: Ensure topology is formed correctly. Verify that Leader_1 & Leader_2 are sending MLE
     *   Advertisements on separate channels.
     */

    leader1.AllowList(dut);
    dut.AllowList(leader1);

    leader2.AllowList(dut);
    dut.AllowList(leader2);

    // Leader 1 <-> DUT is allowed.
    // Leader 2 <-> DUT is initially blocked by radio channel and different PAN ID, but here we also use AllowList.
    // We start with Leader 2 being reachable but on different channel.
    // Actually, to fully control connectivity as per spec, we'll unallow Leader 2 for now.
    leader2.UnallowList(dut);
    dut.UnallowList(leader2);

    dut.Get<Mle::Mle>().SetTimeout(20);

    {
        MeshCoP::Dataset::Info datasetInfo;

        SuccessOrQuit(datasetInfo.GenerateRandom(leader1.GetInstance()));

        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kLeader1PanId);
        {
            NetworkKey &networkKey = datasetInfo.Update<MeshCoP::Dataset::kNetworkKey>();
            memcpy(networkKey.m8, kNetworkKey, sizeof(networkKey));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kLeader1Timestamp);
            timestamp.SetTicks(0);
            datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        }
        datasetInfo.Set<MeshCoP::Dataset::kChannelMask>((1 << kPrimaryChannel) | (1 << kSecondaryChannel));

        leader1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader1.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader1.Get<Mle::Mle>().Start());
    }

    {
        MeshCoP::Dataset::Info datasetInfo;

        SuccessOrQuit(datasetInfo.GenerateRandom(leader2.GetInstance()));

        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kLeader2PanId);
        {
            NetworkKey &networkKey = datasetInfo.Update<MeshCoP::Dataset::kNetworkKey>();
            memcpy(networkKey.m8, kNetworkKey, sizeof(networkKey));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kLeader2Timestamp);
            timestamp.SetTicks(0);
            datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        }

        datasetInfo.Set<MeshCoP::Dataset::kChannelMask>((1 << kPrimaryChannel) | (1 << kSecondaryChannel));

        leader2.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader2.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader2.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(leader2.Get<Mle::Mle>().IsLeader());

    dut.Join(leader1, Node::kAsMed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == leader1.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader_1");

    /**
     * Step 2: Leader_1
     * - Description: Harness silently powers-down Leader_1 and enables connectivity between the DUT and Leader_2
     * - Pass Criteria: N/A
     */

    leader1.Get<Mle::Mle>().Stop();
    leader1.Get<ThreadNetif>().Down();

    leader2.AllowList(dut);
    dut.AllowList(leader2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: DUT");

    /**
     * Step 3: DUT
     * - Description: Automatically recognizes that its Parent is gone when it doesn’t receive responses to MLE Child
     *   Update Requests
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kParentLossTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: DUT");

    /**
     * Step 4: DUT
     * - Description: Automatically attempts to reattach to its current Thread Partition using the standard attaching
     *   process
     * - Pass Criteria: The DUT MUST send a MLE Parent Request
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: DUT");

    /**
     * Step 5: DUT
     * - Description: The DUT does not receive a MLE Parent Response to its request
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kParentSelectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: DUT");

    /**
     * Step 6: DUT
     * - Description: After failing to receive a MLE Parent Response to its request, the DUT automatically sends a MLE
     *   Announce Message on the Secondary channel and waits on the Primary channel to hear any announcements.
     * - Pass Criteria: The DUT MUST send a MLE Announce Message, including the following TLVs:
     *   - Channel TLV: ‘Primary’
     *   - Active Timestamp TLV
     *   - PAN ID TLV
     *   - The Destination PAN ID in the IEEE 802.15.4 MAC header MUST be set to the Broadcast PAN ID (0xFFFF) and MUST
     *     be secured using Key ID Mode 2.
     */

    nexus.AdvanceTime(kAnnounceTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader_2");

    /**
     * Step 7: Leader_2
     * - Description: Receives the MLE Announce from the DUT and automatically sends a MLE Announce on the Primary
     *   channel because Leader_2 has a new Active Timestamp
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kAnnounceTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: DUT");

    /**
     * Step 8: DUT
     * - Description: Receives the MLE Announce from Leader_2 and automatically attempts to attach on the Secondary
     *   channel
     * - Pass Criteria: The DUT MUST attempt to attach on the Secondary channel, with the new PAN ID it received in the
     *   MLE Announce message from Leader_2. The DUT MUST send a Parent Request on the Secondary channel
     */

    nexus.AdvanceTime(kAttachTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == leader2.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader_2");

    /**
     * Step 9: Leader_2
     * - Description: Harness verifies connectivity by instructing Leader_2 to send an ICMP Echo Request to the DUT
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */

    nexus.SendAndVerifyEchoRequest(leader2, dut.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_17.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_17();
    printf("All tests passed\n");
    return 0;
}
