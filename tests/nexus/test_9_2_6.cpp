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

#include "mac/data_poll_sender.hpp"
#include "meshcop/commissioner.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 200 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 5000;

/**
 * Time to wait for MLE Data propagation, in milliseconds.
 */
static constexpr uint32_t kDataPropagationTime = 10000;

/**
 * Time for Delay Timer, in milliseconds (1 minute).
 */
static constexpr uint32_t kDelayTimerTime = 60 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Primary and Secondary channels.
 */
static constexpr uint16_t kPrimaryChannel   = 11;
static constexpr uint16_t kSecondaryChannel = 12;

/**
 * Network Name and PSKc.
 */
static const char    kNetworkName[] = "Thread";
static const uint8_t kPskc[]        = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                       0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x02};

/**
 * Timestamps.
 */
static constexpr uint64_t kActiveTimestampInitial = 10;
static constexpr uint64_t kActiveTimestampNew     = 15;
static constexpr uint64_t kActiveTimestampFinal   = 75;
static constexpr uint64_t kPendingTimestamp       = 30;

void Test9_2_6(void)
{
    /**
     * 9.2.6 Commissioning - Dissemination of Operational Datasets
     *
     * 9.2.6.1 Topology
     * - DUT as Leader (Topology A)
     * - DUT as Router (Topology B)
     * - DUT as MED/SED (Topologies C and D)
     *
     * Note: Two sniffers are required to run this test case!
     *
     * 9.2.6.2 Purpose & Description
     * - DUT as Leader (Topology A): The purpose of this test case is to verify that the Leader device properly collects
     *   and disseminates Operational Datasets through a Thread network.
     * - DUT as Router (Topology B): The purpose of this test case is to show that the Router device correctly sets the
     *   Commissioning information propagated by the Leader device and sends it properly to devices already attached to
     *   it.
     * - DUT as MED/SED (Topologies C and D):
     *   - MED - requires full network data
     *   - SED - requires only stable network data
     * - Set on Leader: Active TimeStamp = 10s
     *
     * Spec Reference                                     | V1.1 Section  | V1.3.0 Section
     * ---------------------------------------------------|---------------|---------------
     * Updating the Active / Pending Operational Dataset  | 8.7.4 / 8.7.5 | 8.7.4 / 8.7.5
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &med1         = nexus.CreateNode();
    Node &sed1         = nexus.CreateNode();

    leader.SetName("LEADER");
    commissioner.SetName("COMMISSIONER");
    router1.SetName("ROUTER_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

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
    leader.AllowList(router1);
    commissioner.AllowList(leader);
    router1.AllowList(leader);
    router1.AllowList(med1);
    router1.AllowList(sed1);
    med1.AllowList(router1);
    sed1.AllowList(router1);

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;

        datasetInfo.Clear();
        SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        timestamp.SetSeconds(kActiveTimestampInitial);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    }
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    router1.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(500));
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to the Leader Anycast or Routing
     *   Locator:
     *   - CoAP URI-Path: coap (S)://[<Leader>]:MM/c/cs
     *   - CoAP Payload: Commissioner Session ID TLV (valid value), Steering Data TLV (allowed TLV)
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::SteeringData steeringData;
            steeringData.SetToPermitAllJoiners();
            SuccessOrQuit(
                Tlv::Append<MeshCoP::SteeringDataTlv>(*message, steeringData.GetData(), steeringData.GetLength()));
        }

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept (0x01)>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children (MED_1) via a
     *   multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response to the Link-Local All Nodes
     *   multicast address (FF02::1) with the new information, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <NOT incremented>
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV
     */

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children (MED_1) via a
     *   multicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response with the new information,
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <NOT incremented>
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     * - Description: No update is sent to SED_1 because Stable Data Version is unchanged.
     * - Pass Criteria: For DUT = Router: The DUT MUST NOT send a unicast MLE Data Response or MLE Child Update Request
     *   to SED_1.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Commissioner");

    /**
     * Step 7: Commissioner
     * - Description: Harness instructs the Commissioner to send MGMT_ACTIVE_SET.req to the Leader Anycast or Routing
     *   Locator:
     *   - CoAP Request: coap://[<L>]:MM/c/as
     *   - CoAP Payload: valid Commissioner Session ID TLV, Active Timestamp TLV : 15s, Network Name TLV : Thread,
     *     PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:02 (new value)
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampNew);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkName));
        SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, AsCoreType(reinterpret_cast<const otPskc *>(kPskc))));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader");

    /**
     * Step 8: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner with Status = Accept.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a MGMT_ACTIVE_SET.rsp frame with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <Accept>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 9: Leader
     * - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response to the Link-Local All Nodes
     *   multicast address (FF02::1) with the new information, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <incremented>
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV: 15s
     */

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_1");

    /**
     * Step 10: Router_1
     * - Description: Automatically requests the full network data from the Leader via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader, which includes the
     *   following TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");

    /**
     * Step 11: Leader
     * - Description: Automatically sends the requested full network data to Router_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1 including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV <new value>
     *   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
     *     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV <new value>, PAN
     *     ID TLV, PSKc TLV, Security Policy TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1");

    /**
     * Step 12: Router_1
     * - Description: Automatically sends the full network data to neighbors and rx-on-while-idle Children (MED_1) via a
     *   multicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response with the new information,
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV <15s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: MED_1");

    /**
     * Step 13: MED_1
     * - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
     *   TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    /**
     * Step 14: Router_1
     * - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, which includes the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9.
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Commissioner Session ID TLV, Border Agent
     *     Locator TLV, Steering Data TLV
     *   - Active Timestamp TLV (new value)
     *   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
     *     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV (New Value), PAN
     *     ID TLV, PSKc TLV, Security Policy TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15A: Router_1");

    /**
     * Step 15A: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Goto step 16
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15B: Router_1");

    /**
     * Step 15B: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: SED_1");

    /**
     * Step 16: SED_1
     * - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
     *   TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1");

    /**
     * Step 17: Router_1
     * - Description: Automatically sends the requested full network data to SED_1.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 9
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
     *     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV <new value>, PAN
     *     ID TLV, PSKc TLV, Security Policy TLV.
     */

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Commissioner");

    /**
     * Step 18: Commissioner
     * - Description: Harness instructs Commissioner to send MGMT_PENDING_SET.req to the Leader Anycast or Routing
     *   Locator:
     *   - CoAP Request: coap://[<L>]:MM/c/ps
     *   - CoAP Payload: Commissioner Session ID TLV <valid value>, Pending Timestamp TLV <30s>, Active Timestamp TLV
     *     <75s>, Delay Timer TLV <1 min>, Channel TLV <Secondary>
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampFinal);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kPendingTimestamp);
            SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, kDelayTimerTime));
        SuccessOrQuit(Tlv::Append<MeshCoP::ChannelTlv>(*message, Mle::ChannelTlvValue(kSecondaryChannel)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader");

    /**
     * Step 19: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner with Status = Accept.
     * - Pass Criteria: For DUT = Leader: The Leader MUST send MGMT_PENDING_SET.rsp frame to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <Accept>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Leader");

    /**
     * Step 20: Leader
     * - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response with the new information, including
     *   the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version field <incremented>, Stable Version field <incremented>
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Router_1");

    /**
     * Step 21: Router_1
     * - Description: Automatically requests full network data from the Leader via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = Router_1: The DUT MUST send a unicast MLE Data Request to the Leader, including the
     *   following TLVs:
     *   - Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: Leader");

    /**
     * Step 22: Leader
     * - Description: Automatically sends full network data to Router_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Pending Operational Dataset TLV
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 23: Router_1");

    /**
     * Step 23: Router_1
     * - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children via a multicast MLE
     *   Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST multicast a MLE Data Response with the new information, including
     *   the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 20
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 24: MED_1");

    /**
     * Step 24: MED_1
     * - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1 including the following
     *   TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 25: Router_1");

    /**
     * Step 25: Router_1
     * - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 20
     *   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV, Steering Data TLV
     *   - Pending Operational Dataset TLV: Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
     *     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security
     *     Policy TLV, Delay Timer TLV
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 26A: Router_1");

    /**
     * Step 26A: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 20
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     *   - Goto step 27
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 26B: Router_1");

    /**
     * Step 26B: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
     *     step 20
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 27: SED_1");

    /**
     * Step 27: SED_1
     * - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
     *   TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 28: Router_1");

    /**
     * Step 28: Router_1
     * - Description: Automatically sends the requested full network data to SED_1.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Network Data TLV
     *   - Pending Operational Dataset TLV: Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
     *     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security
     *     Policy TLV, Delay Timer TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 29: Harness");

    /**
     * Step 29: Harness
     * - Description: Wait for delay timer to expire.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDelayTimerTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 30: Harness");

    /**
     * Step 30: Harness
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address on
     *   the (new) Secondary channel.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(commissioner, med1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);
    nexus.SendAndVerifyEchoRequest(commissioner, sed1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_6();
    printf("All tests passed\n");
    return 0;
}
