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

#include "meshcop/commissioner.hpp"
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
static constexpr uint32_t kJoinTime = 200 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 5000;

/**
 * Time to wait for MLE Announce transmission, in milliseconds.
 */
static constexpr uint32_t kAnnounceTime = 40 * 1000;

/**
 * Time to wait for network stabilization, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 30 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 10 * 1000;

/**
 * Primary and Secondary channels.
 */
static constexpr uint8_t kPrimaryChannel   = 12;
static constexpr uint8_t kSecondaryChannel = 11;

/**
 * Primary and Secondary PAN IDs.
 */
static constexpr uint16_t kPrimaryPanId   = 0x1111;
static constexpr uint16_t kSecondaryPanId = 0x2222;

/**
 * Primary and Secondary Active Timestamps.
 */
static constexpr uint64_t kPrimaryActiveTimestamp   = 10;
static constexpr uint64_t kSecondaryActiveTimestamp = 20;

/**
 * MGMT_ANNOUNCE_BEGIN parameters.
 */
static constexpr uint8_t  kAnnounceCount  = 3;
static constexpr uint16_t kAnnouncePeriod = 3000;

/**
 * Commissioner Session ID.
 */
static constexpr uint16_t kCommissionerSessionId = 0x1234;

void Test9_2_12(void)
{
    /**
     * 9.2.12 Commissioning - Merging networks on different channels and different PANs using MLE Announce
     *
     * 9.2.12.1 Purpose & Description
     * The purpose of this test case is to verify that networks on different channels - and having different PAN IDs -
     *   can merge using the MLE Announce command. The primary channel is always used to host the DUT network.
     *
     * Spec Reference                          | V1.1 Section | V1.3.0 Section
     * ----------------------------------------|--------------|---------------
     * Merging Channel and PAN ID Partitions   | 8.7.8        | 8.7.8
     */

    Core nexus;

    Node &leader1 = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &leader2 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();

    leader1.SetName("LEADER_1");
    router1.SetName("ROUTER_1");
    leader2.SetName("LEADER_2");
    med1.SetName("MED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /** Use AllowList feature to specify links between nodes. */
    leader2.AllowList(med1);
    med1.AllowList(leader2);

    leader1.AllowList(router1);
    router1.AllowList(leader1);

    router1.AllowList(leader2);
    leader2.AllowList(router1);

    router1.AllowList(med1);
    med1.AllowList(router1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    // Setup Secondary Network (L1, R1)
    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;
        MeshCoP::NetworkName   networkName;
        Mac::ChannelMask       channelMask;

        datasetInfo.Clear();
        SuccessOrQuit(datasetInfo.GenerateRandom(leader1.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kSecondaryPanId);
        timestamp.SetSeconds(kSecondaryActiveTimestamp);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        SuccessOrQuit(networkName.Set("Secondary"));
        datasetInfo.Set<MeshCoP::Dataset::kNetworkName>(networkName);
        channelMask.Clear();
        channelMask.AddChannel(kPrimaryChannel);
        channelMask.AddChannel(kSecondaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kChannelMask>(channelMask.GetMask());

        leader1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader1.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader1.Get<Mle::Mle>().Start());
    }
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader1.Get<Mle::Mle>().IsLeader());

    router1.Join(leader1);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    // Setup Primary Network (L2, M1)
    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;
        MeshCoP::NetworkName   networkName;
        Mac::ChannelMask       channelMask;

        SuccessOrQuit(leader1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kPrimaryPanId);
        timestamp.SetSeconds(kPrimaryActiveTimestamp);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        SuccessOrQuit(networkName.Set("Primary"));
        datasetInfo.Set<MeshCoP::Dataset::kNetworkName>(networkName);
        channelMask.Clear();
        channelMask.AddChannel(kPrimaryChannel);
        channelMask.AddChannel(kSecondaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kChannelMask>(channelMask.GetMask());

        leader2.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader2.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader2.Get<Mle::Mle>().Start());
    }
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader2.Get<Mle::Mle>().IsLeader());

    med1.Join(leader2, Node::kAsMed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());

    // Start Commissioner on Leader 1
    SuccessOrQuit(leader1.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(leader1.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader_1 (Commissioner)");

    /**
     * Step 2: Leader_1 (Commissioner)
     * - Description: Harness instructs Leader_1 to unicast MGMT_ANNOUNCE_BEGIN.ntf to Router_1:
     *   - CoAP Request URI: coap://[R1]:MM/c/ab
     *   - CoAP Payload: Commissioner Session ID TLV, Channel Mask TLV: ‘Primary’, Count TLV: 3, Period TLV: 3000ms
     * - Pass Criteria: N/A.
     */

    {
        uint32_t channelMask = (1 << kPrimaryChannel);

        Tmf::Agent    &agent   = leader1.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriAnnounceBegin);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, kCommissionerSessionId));
        SuccessOrQuit(MeshCoP::ChannelMaskTlv::AppendTo(*message, channelMask));
        SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::CountTlv>(*message, kAnnounceCount));
        SuccessOrQuit(MeshCoP::Tlv::Append<MeshCoP::PeriodTlv>(*message, kAnnouncePeriod));

        Tmf::MessageInfo messageInfo(leader1.GetInstance());
        messageInfo.SetPeerAddr(router1.Get<Mle::Mle>().GetMeshLocalRloc());

        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     * - Description: Automatically multicasts 3 MLE Announce messages on channel ‘Primary’, including the following
     *   TLVs:
     *   - Channel TLV: ‘Secondary’
     *   - Active Timestamp TLV: 20s
     *   - PAN ID TLV: ‘Secondary’
     * - Pass Criteria:
     *   - The MLE Announce messages have the Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast
     *     PAN ID (0xFFFF) and are secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, the Key Index set
     *     to 0xff,.
     *   - The MLE Announce messages are secured at the MLE layer using MLE Key Identifier Mode 2.
     */

    nexus.AdvanceTime(kAnnounceTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader_2");

    /**
     * Step 4: Leader_2
     * - Description: Automatically attaches to the network on the Secondary channel.
     * - Pass Criteria:
     *   - For DUT = Leader: The DUT MUST send a MLE Child ID Request on its new channel – the Secondary channel,
     *     including the following TLV: Active Timestamp TLV: 10s.
     *   - After receiving the MLE Child ID Response from Router_1, the DUT MUST send an MLE Announce on its previous
     *     channel – the Primary channel, including the following TLVs: Active Timestamp TLV: 20s, Channel TLV:
     *     ‘Secondary’, PAN ID TLV: ‘Secondary’.
     *   - The MLE Announce MUST have Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast PAN ID
     *     (0xFFFF) and MUST be secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, the Key Index set to
     *     0xff.
     *   - The MLE Announce MUST be secured at the MLE layer. MLE Key Identifier Mode MUST be set to 2.
     */

    nexus.AdvanceTime(kJoinTime + kAnnounceTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: MED_1");

    /**
     * Step 5: MED_1
     * - Description: Automatically attaches to the network on the Secondary channel.
     * - Pass Criteria:
     *   - For DUT = MED: The DUT MUST send an MLE Child ID Request on its new channel - the Secondary channel,
     *     including the following TLV: Active Timestamp TLV: 10s.
     *   - After receiving a Child ID Response from Router_1 or Leader_2, the DUT MUST send an MLE Announce on its
     *     previous channel - the Primary channel, including the following TLVs: Active Timestamp TLV: 20s, Channel
     *     TLV: ‘Secondary’, PAN ID TLV: ‘Secondary’.
     *   - The MLE Announce MUST have Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast PAN ID
     *     (0xFFFF) and MUST be secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, Key Index set to 0xff.
     *   - The MLE Announce MUST be secured at the MLE layer. MLE Key Identifier Mode MUST be set to 2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: All");

    /**
     * Step 6: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    VerifyOrQuit(leader2.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());

    nexus.SendAndVerifyEchoRequest(leader1, leader2.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(leader1, med1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_12.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_12();
    printf("All tests passed\n");
    return 0;
}
