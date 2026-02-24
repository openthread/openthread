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
 * Time to advance for a node to upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kRouterUpgradeTime = 200 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 5000;

/**
 * Time to wait for the Delay Timer to expire, in milliseconds.
 */
static constexpr uint32_t kDelayTimerTime = 250 * 1000;

/**
 * Isolation time, in milliseconds.
 */
static constexpr uint32_t kIsolationTime = 300 * 1000;

/**
 * Network ID Timeout, in milliseconds.
 */
static constexpr uint32_t kNetworkIdTimeout = 180 * 1000;

/**
 * Time to wait for reattachment, in milliseconds.
 */
static constexpr uint32_t kReattachTime = 30 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Primary Channel.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Secondary Channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

/**
 * Primary PAN ID.
 */
static constexpr uint16_t kPrimaryPanId = 0xFACE;

/**
 * Secondary PAN ID.
 */
static constexpr uint16_t kSecondaryPanId = 0xAFCE;

/**
 * Partition Weight.
 */
static constexpr uint8_t kPartitionWeight = 72;

/**
 * Active Timestamp for Leader.
 */
static constexpr uint64_t kLeaderActiveTimestamp = 15;

/**
 * Active Timestamp for Step 2.
 */
static constexpr uint64_t kActiveTimestampStep2 = 165;

/**
 * Pending Timestamp for Step 2.
 */
static constexpr uint64_t kPendingTimestampStep2 = 30;

/**
 * Delay Timer for Step 2.
 */
static constexpr uint32_t kDelayTimerStep2 = 250 * 1000;

void Test9_2_10(void)
{
    /**
     * 9.2.10 Commissioning – Delay timer persistent at partitioning
     *
     * 9.2.10.1 Topology
     * - Commissioner
     * - Leader
     * - Router_1 (DUT)
     * - MED_1
     * - SED_1
     *
     * 9.2.10.2 Purpose & Description
     * The purpose of this test case is to verify that the Thread device maintains a delay timer after partitioning.
     *
     * Spec Reference                             | V1.1 Section | V1.3.0 Section
     * -------------------------------------------|--------------|---------------
     * Migrating Across Thread Network Partitions | 8.4.3.5      | 8.4.3.5
     */

    Core nexus;

    Node &commissioner = nexus.CreateNode();
    Node &leader       = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &med1         = nexus.CreateNode();
    Node &sed1         = nexus.CreateNode();

    commissioner.SetName("COMMISSIONER");
    leader.SetName("LEADER");
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
     * - Pass Criteria: N/A.
     */

    commissioner.AllowList(leader);
    leader.AllowList(commissioner);

    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(med1);
    med1.AllowList(router1);

    router1.AllowList(sed1);
    sed1.AllowList(router1);

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kPrimaryPanId);
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kLeaderActiveTimestamp);
            timestamp.SetTicks(0);
            datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        }
        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader.Get<Mle::Mle>().SetLeaderWeight(kPartitionWeight);
        leader.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    router1.Join(leader);
    nexus.AdvanceTime(kRouterUpgradeTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader’s Anycast or Routing
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV <valid>
     *     - Active Timestamp TLV <165s>
     *     - Pending Timestamp TLV <30s>
     *     - Delay Timer TLV <250s>
     *     - Channel TLV <’Secondary’>
     *     - PAN ID TLV <0xAFCE>
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
     * - Pass Criteria: N/A.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep2);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kPendingTimestampStep2);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, kDelayTimerStep2));
        SuccessOrQuit(Tlv::Append<MeshCoP::ChannelTlv>(*message, MeshCoP::ChannelTlvValue(0, kSecondaryChannel)));
        SuccessOrQuit(Tlv::Append<MeshCoP::PanIdTlv>(*message, kSecondaryPanId));

        SuccessOrQuit(agent.SendMessageToLeaderAloc(*message));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends a MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept>
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response, which
     *   includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV:
     *     - Data Version value <incremented>
     *     - Stable Version value <incremented>
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically requests the full network data from Leader via a unicast MLE Data Request
     * - Pass Criteria: For DUT=Router: The DUT MUST send a unicast MLE Data Request to the Leader, which includes the
     *   following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Automatically sends the requested full network data to Router_1 via a unicast MLE Data Response:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV <165s>
     *     - Delay Timer TLV: <250s>
     *     - Channel TLV <’Secondary’>
     *     - PAN ID TLV <0xAFCE>
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     * - Description: Automatically sends the new network data to neighbors and rx-on-when-idle Children (MED_1) via a
     *   multicast MLE Data Response
     * - Pass Criteria: For DUT=Router: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast
     *   address (FF02::1), including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV:
     *     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: MED_1");

    /**
     * Step 8: MED_1
     * - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request
     * - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
     *   TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1");

    /**
     * Step 9: Router_1
     * - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, including the
     * following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Steering Data TLV
     *   - Active Timestamp TLV (new value)
     *   - Active Operational Dataset TLV**
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Extended PAN ID TLV
     *     - Network Mesh-Local Prefix TLV
     *     - Network Master Key TLV
     *     - Network Name TLV (New Value)
     *     - PAN ID TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     *   - ** the Active Operational Dataset TLV MUST NOT contain the Active Timestamp TLV
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10B: Router_1");

    /**
     * Step 10B: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
     *   - Network Data TLV
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: SED_1");

    /**
     * Step 11: SED_1
     * - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request
     * - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, which includes the
     *   following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1");

    /**
     * Step 12: Router_1
     * - Description: Automatically sends the requested full network data to SED_1
     * - Pass Criteria: For DUT=Router: The DUT MUST send a unicast MLE Data Response to SED_1, which includes the
     *   following TLVs:
     *   - Source Address TLV
     *   - Network Data TLV:
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
     *   - Active Timestamp TLV <15s>
     *   - Pending Timestamp TLV <30s>
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: User");

    /**
     * Step 13: User
     * - Description: Harness instructs the user to isolate Router_1, MED_1, and SED_1 from both the Leader and the
     *   Commissioner. RF isolation will last for 300 seconds; steps 14-17 occur during isolation.
     * - Pass Criteria: N/A.
     */

    router1.Get<Mac::Filter>().ClearAddresses();
    router1.AllowList(med1);
    router1.AllowList(sed1);

    med1.Get<Mac::Filter>().ClearAddresses();
    med1.AllowList(router1);

    sed1.Get<Mac::Filter>().ClearAddresses();
    sed1.AllowList(router1);

    leader.Get<Mac::Filter>().ClearAddresses();
    commissioner.Get<Mac::Filter>().ClearAddresses();
    commissioner.AllowList(leader);
    leader.AllowList(commissioner);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    /**
     * Step 14: Router_1
     * - Description: Automatically starts a new partition
     * - Pass Criteria: For DUT=Router: After NETWORK_ID_TIMEOUT, the DUT MUST start a new partition with parameters set
     *   in Active Operational Dataset (Channel = ‘Primary’, PAN ID = 0xFACE).
     */

    nexus.AdvanceTime(kNetworkIdTimeout);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mac::Mac>().GetPanId() == kPrimaryPanId);
    VerifyOrQuit(router1.Get<Mac::Mac>().GetPanChannel() == kPrimaryChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Leader, Commissioner");

    /**
     * Step 15: Leader, Commissioner
     * - Description: After the Delay Timer expires, the network automatically moves to the Secondary channel, PAN ID =
     *   0xAFCE
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Router_1");

    /**
     * Step 16: Router_1
     * - Description: Automatically moves to the secondary channel
     * - Pass Criteria: For DUT=Router: After the Delay Timer expires, the DUT MUST move to the Secondary channel, PAN
     * ID = 0xAFCE.
     */

    nexus.AdvanceTime(kDelayTimerTime - kNetworkIdTimeout);

    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanId() == kSecondaryPanId);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    VerifyOrQuit(router1.Get<Mac::Mac>().GetPanId() == kSecondaryPanId);
    VerifyOrQuit(router1.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: MED_1/SED_1");

    /**
     * Step 17: MED_1/SED_1
     * - Description: Automatically moves to the secondary channel
     * - Pass Criteria: For DUT = MED/SED: After the Delay Timer expires, the DUT MUST move to the Secondary channel,
     * PAN ID = 0xAFCE.
     */

    VerifyOrQuit(med1.Get<Mac::Mac>().GetPanId() == kSecondaryPanId);
    VerifyOrQuit(med1.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    VerifyOrQuit(sed1.Get<Mac::Mac>().GetPanId() == kSecondaryPanId);
    VerifyOrQuit(sed1.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: User");

    /**
     * Step 18: User
     * - Description: Harness instructs the user to remove the RF isolation that began in step 13
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kIsolationTime - kDelayTimerTime);

    router1.AllowList(leader);
    leader.AllowList(router1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Router_1");

    /**
     * Step 19: Router_1
     * - Description: Automatically reattaches to the Leader
     * - Pass Criteria: For DUT=Router: The DUT MUST reattach to the Leader and the partitions MUST merge.
     */

    nexus.AdvanceTime(kReattachTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Leader");

    /**
     * Step 20: Leader
     * - Description: The harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   DUT mesh local address
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(leader, router1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_10.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_10();
    printf("All tests passed\n");
    return 0;
}
