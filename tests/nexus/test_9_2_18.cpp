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

#include "meshcop/commissioner.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/dataset_manager.hpp"
#include "meshcop/meshcop_leader.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "meshcop/network_name.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Nexus {

using MeshCoP::ExtendedPanId;
using MeshCoP::NetworkName;

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Delay Timer value in milliseconds (300 seconds).
 */
static constexpr uint32_t kDelayTimer300s = 300 * 1000;

/**
 * Active Timestamp value 20000.
 */
static constexpr uint64_t kActiveTimestamp20000 = 20000;

/**
 * Active Timestamp value 20.
 */
static constexpr uint64_t kActiveTimestamp20 = 20;

/**
 * Pending Timestamp value 20.
 */
static constexpr uint64_t kPendingTimestamp20 = 20;

/**
 * Network Master Key 1.
 */
static const uint8_t kNetworkKey1[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                       0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

/**
 * Network Master Key 2.
 */
static const uint8_t kNetworkKey2[] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
                                       0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

/**
 * PSKc 1.
 */
static const uint8_t kPskc1[] = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                 0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x04};

/**
 * PSKc 2.
 */
static const uint8_t kPskc2[] = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                 0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x05};

void Test9_2_18(void)
{
    /**
     * 9.2.18 Rolling Back the Active Timestamp with Pending Operational Dataset
     *
     * 9.2.18.1 Topology
     * - Commissioner
     * - Leader
     * - Router 1
     * - MED 1
     * - SED 1
     *
     * 9.2.18.2 Purpose & Description
     * The purpose of this test case is to ensure that the DUT can roll back the Active Timestamp value by scheduling
     *   an update through the Pending Operational Dataset only with the inclusion of a new Network Master Key.
     *
     * Spec Reference          | V1.1 Section | V1.3.0 Section
     * ------------------------|--------------|---------------
     * Delay Timer Management  | 8.4.3.4      | 8.4.3.4
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
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to specify links between nodes. */
    commissioner.AllowList(leader);
    leader.AllowList(commissioner);

    router1.AllowList(leader);
    leader.AllowList(router1);

    router1.AllowList(med1);
    med1.AllowList(router1);

    router1.AllowList(sed1);
    sed1.AllowList(router1);

    MeshCoP::Dataset::Info datasetInfo;
    SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(1);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    }
    memcpy(datasetInfo.Update<MeshCoP::Dataset::kNetworkKey>().m8, kNetworkKey1, sizeof(kNetworkKey1));
    datasetInfo.Set<MeshCoP::Dataset::kChannel>(11);
    datasetInfo.Set<MeshCoP::Dataset::kPanId>(0x1234);
    {
        uint8_t extPanIdBytes[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
        memcpy(datasetInfo.Update<MeshCoP::Dataset::kExtendedPanId>().m8, extPanIdBytes, sizeof(extPanIdBytes));
    }
    SuccessOrQuit(datasetInfo.Update<MeshCoP::Dataset::kNetworkName>().Set("Initial"));
    memcpy(datasetInfo.Update<MeshCoP::Dataset::kPskc>().m8, kPskc1, sizeof(kPskc1));

    leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsFullThreadDevice());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    {
        NetworkKey key;
        memcpy(key.m8, kNetworkKey1, sizeof(key.m8));
        nexus.AddNetworkKey(key);
        memcpy(key.m8, kNetworkKey2, sizeof(key.m8));
        nexus.AddNetworkKey(key);
    }

    med1.Join(router1, Node::kAsFed);
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    /** Start Commissioner and wait for it to become active. */
    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to the Leader Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV (valid)
     *     - Active Timestamp TLV <20000>
     *     - Network Name TLV: "GRL"
     *     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:04
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
     * - Pass Criteria: N/A
     */

    MeshCoP::Dataset::Info activeDatasetInfo;
    activeDatasetInfo.Clear();
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(kActiveTimestamp20000);
        activeDatasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    }
    SuccessOrQuit(activeDatasetInfo.Update<MeshCoP::Dataset::kNetworkName>().Set("GRL"));
    memcpy(activeDatasetInfo.Update<MeshCoP::Dataset::kPskc>().m8, kPskc1, sizeof(kPskc1));

    /**
     * Step 3: Leader
     * - Description: Automatically responds with a MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept (01)>
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(
            *message, commissioner.Get<MeshCoP::Commissioner>().GetSessionId()));
        {
            MeshCoP::Dataset dataset;
            dataset.SetFrom(activeDatasetInfo);
            SuccessOrQuit(message->AppendBytes(dataset.GetBytes(), dataset.GetLength()));
        }
        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().GetTimestamp().GetSeconds() == kActiveTimestamp20000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Commissioner");

    /**
     * Step 4: Commissioner
     * - Description: Harness instructs the Commissioner to send MGMT_PENDING_SET.req to the Leader Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV (valid)
     *     - Pending Timestamp TLV <20s>
     *     - Active Timestamp TLV <20s>
     *     - Delay Timer TLV <20s>
     *     - Network Name TLV : "Should not be"
     * - Pass Criteria: N/A
     */

    MeshCoP::Dataset::Info pendingDatasetInfo1;
    pendingDatasetInfo1.Clear();
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(kPendingTimestamp20);
        pendingDatasetInfo1.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);
        timestamp.SetSeconds(kActiveTimestamp20);
        pendingDatasetInfo1.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    }
    pendingDatasetInfo1.Set<MeshCoP::Dataset::kDelay>(20000); // 20s
    SuccessOrQuit(pendingDatasetInfo1.Update<MeshCoP::Dataset::kNetworkName>().Set("Should not be"));

    /**
     * Step 5: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Reject (-1)>
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(
            *message, commissioner.Get<MeshCoP::Commissioner>().GetSessionId()));
        {
            MeshCoP::Dataset dataset;
            dataset.SetFrom(pendingDatasetInfo1);
            SuccessOrQuit(message->AppendBytes(dataset.GetBytes(), dataset.GetLength()));
        }
        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(!leader.Get<MeshCoP::PendingDatasetManager>().GetTimestamp().IsValid());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Commissioner");

    /**
     * Step 6: Commissioner
     * - Description: Harness instructs Commissioner to send MGMT_PENDING_SET.req to the Leader Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV (valid)
     *     - Pending Timestamp TLV <20s>
     *     - Active Timestamp TLV <20s>
     *     - Delay Timer TLV <300s>
     *     - Network Name TLV : "My House"
     *     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:05
     *     - Network Master Key TLV: (00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ee)
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    MeshCoP::Dataset::Info pendingDatasetInfo2;
    pendingDatasetInfo2.Clear();
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(kPendingTimestamp20);
        pendingDatasetInfo2.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);
        timestamp.SetSeconds(kActiveTimestamp20);
        pendingDatasetInfo2.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    }
    pendingDatasetInfo2.Set<MeshCoP::Dataset::kDelay>(kDelayTimer300s);
    SuccessOrQuit(pendingDatasetInfo2.Update<MeshCoP::Dataset::kNetworkName>().Set("My House"));
    memcpy(pendingDatasetInfo2.Update<MeshCoP::Dataset::kPskc>().m8, kPskc2, sizeof(kPskc2));
    memcpy(pendingDatasetInfo2.Update<MeshCoP::Dataset::kNetworkKey>().m8, kNetworkKey2, sizeof(kNetworkKey2));

    /**
     * Step 7: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept (01)>
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(
            *message, commissioner.Get<MeshCoP::Commissioner>().GetSessionId()));
        {
            MeshCoP::Dataset dataset;
            dataset.SetFrom(pendingDatasetInfo2);
            SuccessOrQuit(message->AppendBytes(dataset.GetBytes(), dataset.GetLength()));
        }
        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(leader.Get<MeshCoP::PendingDatasetManager>().GetTimestamp().GetSeconds() == kPendingTimestamp20);

    /**
     * Step 8: Leader
     * - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response to the Link-Local All Nodes
     *   multicast address (FF02::1) with the new information, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>)
     *   - Network Data TLV: Commissioning Data TLV: (Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV)
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader");

    /**
     * Step 9: Router_1
     * - Description: Automatically sends a unicast MLE Data Request to the Leader.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader including the
     *   following TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1");

    /**
     * Step 10: Leader
     * - Description: Automatically sends a unicast MLE Data Response to Router_1.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, which includes the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV: Commissioning Data TLV: (Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
     *     Session ID TLV)
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     *   - Pending Operational Dataset TLV: (Active Timestamp TLV, Network Master Key TLV, Network Name TLV)
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader");

    /**
     * Step 11: Router_1
     * - Description: Automatically sends the new network data to neighbors and rx-on-while-idle Children (MED_1).
     * - Pass Criteria: For DUT = Router: The DUT MUST multicast a MLE Data Response with the new information, which the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV (Data Version field MUST have the same value that the Leader set in Step 8, Stable Data
     *     Version field MUST have the same value that the Leader set in Step 8)
     *   - Network Data TLV: (Stable flag <set to 0>, Commissioning Data TLV: Border Agent Locator TLV, Commissioner
     *     Session ID TLV)
     *   - Active Timestamp TLV
     *   - Pending Timestamp TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router_1");

    /**
     * Step 12A: Router_1
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV (Data version numbers MUST be the same as the ones sent in the multicast data response in
     *     step 8.)
     *   - Network Data TLV
     *   - Active Timestamp TLV <20000s>
     *   - Pending Timestamp TLV <20s>
     *   - Goto step 13
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 12A: Router_1");

    /**
     * Step 13: SED_1
     * - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
     * - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
     *   TLVs:
     *   - TLV Request TLV: Network Data TLV
     *   - Active Timestamp TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: SED_1");

    /**
     * Step 14: Router_1
     * - Description: Automatically sends the requested full network data to SED_1.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Network Data TLV
     *   - Pending Operational Dataset TLV: (Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
     *     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security
     *     Policy TLV, Delay Timer TLV)
     *   - Active Timestamp TLV <20000s>
     *   - Pending Timestamp TLV <20s>
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Harness");

    /**
     * Step 16: Harness
     * - Description: Wait for data to distribute and for Pending set Delay time to expire ~300s.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDelayTimer300s);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Commissioner");

    /**
     * Step 17: Commissioner
     * - Description: Harness verifies connectivity by instructing Commissioner to send an ICMPv6 Echo Request to the
     *   DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply. ** Note that Wireshark will not be able to
     *   decode the ICMPv6 Echo Request packet.
     */

    VerifyOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().GetTimestamp().GetSeconds() == kActiveTimestamp20);

    nexus.AdvanceTime(kStabilizationTime);

    // Verify connectivity by pinging from Commissioner to Leader
    commissioner.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), 0);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_9_2_18.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_18();
    printf("All tests passed\n");
    return 0;
}
