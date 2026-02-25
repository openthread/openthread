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

#include "meshcop/commissioner.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 20 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 20 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 15 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 15000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Delay timer value in seconds (60 minutes).
 */
static constexpr uint32_t kDelayTimerStep11 = 60 * 60;

/**
 * Delay timer value in seconds (1 minute).
 */
static constexpr uint32_t kDelayTimerStep17 = 60;

/**
 * Active Timestamp for Leader.
 */
static constexpr uint64_t kActiveTimestampLeader = 10;

/**
 * Active Timestamp for Router.
 */
static constexpr uint64_t kActiveTimestampStep5  = 15;
static constexpr uint64_t kActiveTimestampRouter = 20;

/**
 * Pending Timestamp for Router.
 */
static constexpr uint64_t kPendingTimestampRouter = 30;

/**
 * Pending Timestamp for Commissioner.
 */
static constexpr uint64_t kPendingTimestampCommissioner = 40;

/**
 * Active Timestamp for Commissioner.
 */
static constexpr uint64_t kActiveTimestampCommissioner = 80;

/**
 * Router Partition Weight.
 */
static constexpr uint8_t kRouterWeight = 47;

/**
 * PAN ID for Step 17.
 */
static constexpr uint16_t kPanIdStep17 = 0xafce;

/**
 * Secondary Channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

/**
 * Fixed Network Key for stable decryption.
 */
static constexpr uint8_t kNetworkKey[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                          0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

void Test9_2_7(void)
{
    /**
     * 9.2.7 Commissioning – Delay Timer Management
     *
     * 9.2.7.1 Topology
     * - NOTE: Two sniffers are required to run this test case!
     * - Set on Leader: Active Timestamp = 10s
     * - Set on Router: Active Timestamp = 20s, Pending Timestamp = 30s
     * - At the start of the test, the Router has a current Pending Operational Dataset with a delay timer set to 60
     *   minutes.
     * - Router Partition Weight is configured to a value of 47, to make it always lower than the Leader's weight.
     * - Initially, there is no link between the Leader and the Router.
     *
     * 9.2.7.2 Purpose & Description
     * The purpose of this test case is to verify that if the Leader receives a Pending Operational Dataset with a
     *   newer Pending Timestamp, it resets the running delay timer, installs the new Pending Operational Dataset, and
     *   disseminates the new Commissioning information in the network.
     *
     * Spec Reference                          | V1.1 Section | V1.3.0 Section
     * ----------------------------------------|--------------|---------------
     * Updating the Active Operational Dataset | 8.7.4        | 8.7.4
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &router       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    commissioner.SetName("COMMISSIONER");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    leader.AllowList(commissioner);
    commissioner.AllowList(leader);

    {
        MeshCoP::Dataset::Info datasetInfo;

        datasetInfo.Clear();
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(reinterpret_cast<const NetworkKey &>(kNetworkKey));
        datasetInfo.mActiveTimestamp.mSeconds             = kActiveTimestampLeader;
        datasetInfo.mActiveTimestamp.mTicks               = 0;
        datasetInfo.mComponents.mIsActiveTimestampPresent = true;
        datasetInfo.mComponents.mIsNetworkKeyPresent      = true;

        MeshCoP::NetworkName networkName;
        SuccessOrQuit(networkName.Set("Nexus-9-2-7"));
        datasetInfo.Set<MeshCoP::Dataset::kNetworkName>(networkName);

        datasetInfo.Set<MeshCoP::Dataset::kPanId>(0x1234);
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(11);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    nexus.AdvanceTime(20000);

    // Start commissioner session
    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("Step 2: Harness");

    /**
     * Step 2: Harness
     * - Description: Enable link between the DUT and Router.
     * - Pass Criteria: N/A
     */

    leader.AllowList(router);
    router.AllowList(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router");

    /**
     * Step 3: Router
     * - Description: Automatically attaches to the Leader (DUT). Within the MLE Child ID Request of the attach
     *   process, it includes the new active and pending timestamps.
     * - Pass Criteria: N/A
     */

    router.Get<Mle::Mle>().SetLeaderWeight(kRouterWeight);

    router.Join(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends MLE Child ID Response to the Router.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child ID Response to the Router, including the following TLVs:
     *   - Active Operational Dataset TLV:
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Extended PAN ID TLV
     *     - Network Master Key TLV
     *     - Network Mesh-Local Prefix TLV
     *     - Network Name TLV
     *     - PAN ID TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     *   - Active Timestamp TLV: 10s
     */

    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router");

    /**
     * Step 5: Router
     * - Description: Harness instructs the Router to send a MGMT_ACTIVE_SET.req to the DUT’s Anycast or Router
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: < Commissioner Session ID TLV not present>, Active Timestamp TLV : 20s, Active Operational
     *     Dataset TLV: all parameters in Active Dataset
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent            &agent = router.Get<Tmf::Agent>();
        Coap::Message         *message;
        MeshCoP::Dataset       dataset;
        MeshCoP::Dataset::Info datasetInfo;

        message = agent.NewPriorityConfirmablePostMessage(ot::kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(router.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
        dataset.ConvertTo(datasetInfo);

        {
            MeshCoP::Timestamp timestamp;
            // Deviation from spec: Step 5 uses 15 instead of 20 to ensure it is older than the timestamp in Step 11.
            timestamp.SetSeconds(kActiveTimestampStep5);
            timestamp.SetTicks(0);
            datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        }

        // We use a fresh dataset object to encode the modified datasetInfo
        MeshCoP::Dataset updatedDataset;
        SuccessOrQuit(updatedDataset.WriteTlvsFrom(datasetInfo));
        SuccessOrQuit(message->AppendBytes(updatedDataset.GetBytes(), updatedDataset.GetLength()));

        Tmf::MessageInfo messageInfo(router.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to the Router.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Router with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept)
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically multicasts a MLE Data Response with the new information.
     * - Pass Criteria: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast address (FF02::1),
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Version field incremented
     *   - Active Timestamp TLV: 20s
     *   - Network Data TLV:
     *     - Commissioner Data TLV:
     *       - Stable flag set to 0
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
     * - Pass Criteria: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner with the following format:
     *   - CoAP Request URI: coap://[Commissioner]:MM/c/dc
     *   - CoAP Payload: <empty>
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router");

    /**
     * Step 9: Router
     * - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT) with its current Active
     *   Timestamp.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to the Router with the new active timestamp and
     *   active operational dataset.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to the Router, including the following TLVs:
     *   - Active Operational Dataset TLV
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Extended PAN ID TLV
     *     - Network Master Key TLV
     *     - Network Mesh-Local Prefix TLV
     *     - Network Name TLV
     *     - PAN ID TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     *   - Active Timestamp TLV: 20s
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router");

    /**
     * Step 11: Router
     * - Description: Harness instructs the Router to send a MGMT_PENDING_SET.req to the DUT’s Anycast or Routing
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload: < Commissioner Session ID TLV not present>, Pending Timestamp TLV: 30s, Active Timestamp TLV:
     *     20s, Delay Timer TLV
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent        &agent = router.Get<Tmf::Agent>();
        Coap::Message     *message;
        MeshCoP::Dataset   dataset;
        MeshCoP::Timestamp timestamp;

        message = agent.NewPriorityConfirmablePostMessage(ot::kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(router.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));

        timestamp.SetSeconds(kActiveTimestampRouter);
        timestamp.SetTicks(0);
        SuccessOrQuit(dataset.Write<MeshCoP::ActiveTimestampTlv>(timestamp));

        timestamp.SetSeconds(kPendingTimestampRouter);
        timestamp.SetTicks(0);
        SuccessOrQuit(dataset.Write<MeshCoP::PendingTimestampTlv>(timestamp));

        SuccessOrQuit(dataset.Write<MeshCoP::DelayTimerTlv>(kDelayTimerStep11 * 1000));

        SuccessOrQuit(message->AppendBytes(dataset.GetBytes(), dataset.GetLength()));

        Tmf::MessageInfo messageInfo(router.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    // Wait for acceptance and retransmissions if needed
    nexus.AdvanceTime(2 * kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner and incorporates the new pending
     *   dataset values.
     * - Pass Criteria: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept)
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader (DUT)");

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically multicasts a MLE Data Response with the new information.
     * - Pass Criteria: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast address (FF02::1),
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Version field incremented
     *   - Network Data TLV:
     *     - Commissioner Data TLV:
     *       - Stable flag set to 0
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader (DUT)");

    /**
     * Step 14: Leader (DUT)
     * - Description: Automatically sends a MGMT_DATASET_CHANGED.ntf to the Commissioner.
     * - Pass Criteria: THE DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner with the following format:
     *   - CoAP Request URI: coap://[Commissioner]:MM/c/dc
     *   - CoAP Payload: <empty>
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Router");

    /**
     * Step 15: Router
     * - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT) with a new active timestamp.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    /**
     * Step 16: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to the Router with a new active timestamp, new
     *   pending timestamp, and a new pending operational dataset.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to the Router, which includes the following
     *   TLVs:
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Delay Timer TLV
     *     - Extended PAN ID TLV
     *     - Network Master Key TLV
     *     - Network Mesh-Local Prefix TLV
     *     - Network Name TLV
     *     - PAN ID TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     *   - Active Timestamp TLV: 20s
     *   - Pending Timestamp TLV: 30s
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Commissioner");

    /**
     * Step 17: Commissioner
     * - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader’s Anycast or
     *   Routing Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload: Valid Commissioner Session ID TLV, Pending Timestamp TLV: 40s, Active Timestamp TLV: 80s,
     *     Delay Timer TLV: 1min, Channel TLV: ‘Secondary’, PAN ID TLV: 0xAFCE
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */
    {
        Tmf::Agent    &agent     = commissioner.Get<Tmf::Agent>();
        Coap::Message *message   = agent.NewPriorityConfirmablePostMessage(ot::kUriPendingSet);
        uint16_t       sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kPendingTimestampCommissioner);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampCommissioner);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, kDelayTimerStep17 * 1000));
        SuccessOrQuit(Tlv::Append<MeshCoP::ChannelTlv>(*message, MeshCoP::ChannelTlvValue(0, kSecondaryChannel)));
        SuccessOrQuit(Tlv::Append<MeshCoP::PanIdTlv>(*message, kPanIdStep17));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Leader (DUT)");

    /**
     * Step 18: Leader (DUT)
     * - Description: Automatically sends a MGMT_PENDING_SET.rsp to the Commissioner with Status = Accept.
     * - Pass Criteria: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader (DUT)");

    /**
     * Step 19: Leader (DUT)
     * - Description: Automatically sends a multicast MLE Data Response.
     * - Pass Criteria: The DUT MUST send a MLE Data Response to the Link-Local All Nodes multicast address (FF02::1),
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag set to 0
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *   - Active Timestamp TLV: 20s
     *   - Pending Timestamp TLV: 40s
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Router");

    /**
     * Step 20: Router
     * - Description: Automatically sends a unicast MLE Data Request to the DUT with the new active timestamp and
     *   pending timestamp:
     *   - Active Timestamp TLV: 20s
     *   - Pending Timestamp TLV: 40s
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Leader (DUT)");

    /**
     * Step 21: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to the Router with the active Timestamp, the new
     *   pending timestamp and the current pending operational dataset.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to the Router, which includes the following
     *   TLVs:
     *   - Pending Operational Dataset TLV:
     *     - Channel TLV
     *     - Active Timestamp TLV
     *     - Channel Mask TLV
     *     - Extended PAN ID TLV
     *     - Network Mesh-Local Prefix TLV
     *     - Network Master Key TLV
     *     - Network Name TLV
     *     - PAN ID TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     *     - Delay Timer TLV
     *   - Active Timestamp TLV: 20s
     *   - Pending Timestamp TLV: 40s
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: All");

    /**
     * Step 22: All
     * - Description: Verify that after 60 seconds, the Thread network moves to the Secondary channel, with PAN ID:
     *   0xAFCE.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(100000);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanId() == kPanIdStep17);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    VerifyOrQuit(router.Get<Mac::Mac>().GetPanId() == kPanIdStep17);
    VerifyOrQuit(router.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 23: All");

    /**
     * Step 23: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 0, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_7();
    printf("All tests passed\n");
    return 0;
}
