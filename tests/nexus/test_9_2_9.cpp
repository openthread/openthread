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
#include "meshcop/dataset_manager.hpp"
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
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 1000;

/**
 * Time for the delay timer, in milliseconds.
 */
static constexpr uint32_t kDelayTimer1000 = 1000 * 1000;

/**
 * Time for the delay timer, in milliseconds.
 */
static constexpr uint32_t kDelayTimer200 = 200 * 1000;

/**
 * Time to wait for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizeTime = 10 * 1000;

/**
 * Duration for RF isolation, in milliseconds.
 */
static constexpr uint32_t kRfIsolationTime = 250 * 1000;

/**
 * Time to wait for petitioning, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Initial active timestamp.
 */
static constexpr uint64_t kInitialActiveTimestamp = 10;

/**
 * Active timestamp in Step 2.
 */
static constexpr uint64_t kActiveTimestampStep2 = 210;

/**
 * Pending timestamp in Step 2.
 */
static constexpr uint64_t kPendingTimestampStep2 = 30;

/**
 * Active timestamp in Step 11.
 */
static constexpr uint64_t kActiveTimestampStep11 = 15;

/**
 * Active timestamp in Step 15.
 */
static constexpr uint64_t kActiveTimestampStep15 = 410;

/**
 * Pending timestamp in Step 15.
 */
static constexpr uint64_t kPendingTimestampStep15 = 50;

/**
 * PAN ID 0xFACE.
 */
static constexpr uint16_t kPanIdFace = 0xFACE;

/**
 * PAN ID 0xAFCE.
 */
static constexpr uint16_t kPanIdAfce = 0xAFCE;

/**
 * PAN ID 0xABCD.
 */
static constexpr uint16_t kPanIdAbcd = 0xABCD;

/**
 * Primary channel.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Secondary channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

void Test9_2_9(void)
{
    /**
     * 9.2.9 Commissioning – Synchronizing Pending Operational Datasets When 2 Partitions Merge
     *
     * 9.2.9.1 Topology
     * - NOTE: Two sniffers are required to run this test case! The second sniffer is used for debug traces.
     * - NOTE: RF isolation is required for this test case.
     *
     * 9.2.9.2 Purpose & Description
     * The purpose of this test case is to verify how Pending Operational Datasets are synchronized when two
     *   partitions merge.
     *
     * Spec Reference                              | V1.1 Section | V1.3.0 Section
     * --------------------------------------------|--------------|---------------
     * Migrating Across Thread Network Partitions  | 8.4.3.5      | 8.4.3.5
     *
     * Set on Leader:
     * - Active Timestamp = 10s
     * - Channel = ‘Primary’
     * - PAN ID = 0xFACE
     * - Network Name = ‘GRL’
     * - When DUT=Router, set Leader Partition ID to max
     *
     * Set on Router_2:
     * - Set NETWORK_ID_TIMEOUT = 70s
     * - When DUT=Leader, set Router_2 Partition ID to be the lowest possible value
     *
     * NOTE For Pass Criteria:
     * - The following sequence of events do not need to follow the exact order given in the test procedure. Based on
     *   device implementation, the below validation could be different:
     *   - When the Leader is the DUT, it may send either multiple MLE Data Response packets (one after the Active
     *     Update and one after the Pending Dataset update) or may choose to wait 1-2 seconds and then send only a
     *     single MLE Data Response with both Active and Pending Updates.
     *   - Router_1 (when used as a testbed device) may send the MGMT_ACTIVE_SET and MGMT_PENDING_SET specified in
     *     steps 23 & 27 in either reverse order or simultaneously.
     */

    Core nexus;

    Node &commissioner = nexus.CreateNode();
    Node &leader       = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &router2      = nexus.CreateNode();

    {
        static const uint8_t kExtAddr0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
        static const uint8_t kExtAddr1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
        static const uint8_t kExtAddr2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
        static const uint8_t kExtAddr3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
        Mac::ExtAddress      extAddr;

        extAddr.Set(kExtAddr0);
        commissioner.Get<Mac::Mac>().SetExtAddress(extAddr);
        extAddr.Set(kExtAddr1);
        leader.Get<Mac::Mac>().SetExtAddress(extAddr);
        extAddr.Set(kExtAddr2);
        router1.Get<Mac::Mac>().SetExtAddress(extAddr);
        extAddr.Set(kExtAddr3);
        router2.Get<Mac::Mac>().SetExtAddress(extAddr);
    }

    commissioner.SetName("COMMISSIONER");
    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    leader.Get<Mle::Mle>().SetPreferredLeaderPartitionId(3);
    router2.Get<Mle::Mle>().SetPreferredLeaderPartitionId(2);
    router1.Get<Mle::Mle>().SetPreferredLeaderPartitionId(1);

    leader.Get<Mle::Mle>().SetLeaderWeight(200);
    router2.Get<Mle::Mle>().SetLeaderWeight(128);
    router1.Get<Mle::Mle>().SetLeaderWeight(64);

    router2.Get<Mle::Mle>().SetNetworkIdTimeout(70);
    router1.Get<Mle::Mle>().SetNetworkIdTimeout(120);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    commissioner.AllowList(leader);
    leader.AllowList(commissioner);

    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(router2);
    router2.AllowList(router1);

    leader.Get<Mac::Mac>().SetPanId(kPanIdFace);
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;
        static const uint8_t   kNetworkKey[]      = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                     0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        static const uint8_t   kMeshLocalPrefix[] = {0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00};

        dataset.Clear();
        timestamp.Clear();
        timestamp.SetSeconds(kInitialActiveTimestamp);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        dataset.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        dataset.Set<MeshCoP::Dataset::kPanId>(kPanIdFace);
        memcpy(dataset.Update<MeshCoP::Dataset::kNetworkKey>().m8, kNetworkKey, sizeof(kNetworkKey));
        memcpy(dataset.Update<MeshCoP::Dataset::kMeshLocalPrefix>().m8, kMeshLocalPrefix, sizeof(kMeshLocalPrefix));
        SuccessOrQuit(dataset.Update<MeshCoP::Dataset::kNetworkName>().Set("GRL"));
        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(dataset);
    }

    commissioner.Join(leader, Node::kAsFtd);
    router1.Join(leader, Node::kAsFtd);
    router2.Join(router1, Node::kAsFtd);

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - Delay Timer TLV: 1000s
     *     - Channel TLV: ‘Secondary’
     *     - PAN ID TLV: 0xAFCE
     *     - Active Timestamp TLV: 210s
     *     - Pending Timestamp TLV: 30s
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    {
        MeshCoP::Dataset::Info pendingDataset;
        MeshCoP::Timestamp     timestamp;

        pendingDataset.Clear();

        timestamp.SetSeconds(kPendingTimestampStep2);
        pendingDataset.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kActiveTimestampStep2);
        pendingDataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        pendingDataset.Set<MeshCoP::Dataset::kDelay>(kDelayTimer1000);
        pendingDataset.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);
        pendingDataset.Set<MeshCoP::Dataset::kPanId>(kPanIdAfce);

        SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendSetRequest(pendingDataset, nullptr, 0,
                                                                                        nullptr, nullptr));
    }

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept (01)>
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    router1.UnallowList(router2);
    router2.UnallowList(router1);

    /**
     * Step 4: Leader
     * - Description: Automatically sends a multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response with the new information, including
     *   the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 10s
     *   - Pending Timestamp TLV: 30s
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Automatically sends unicast MLE Data Request to the Leader.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader, including the
     *   following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV (10s)
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Automatically sends unicast MLE Data Response to Router_1.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 10s
     *   - Pending Timestamp TLV: 30s
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV: 210s
     *     - Delay Timer TLV: ~1000s
     *     - Channel TLV: ‘Secondary’
     *     - PAN ID TLV: 0xAFCE
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    router1.AllowList(router2);
    router2.AllowList(router1);

    /**
     * Step 6: Router_1
     * - Description: Automatically sends multicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 10s
     *   - Pending Timestamp TLV: 30s
     */

    nexus.AdvanceTime(40000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_2");

    /**
     * Step 7: Router_2
     * - Description: Automatically sends a unicast MLE Data Request to Router_1, including the following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_1");

    /**
     * Step 8: Router_1
     * - Description: Automatically sends a unicast MLE Data Response to Router_2.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to Router_2, including the
     *   following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 10s
     *   - Pending Timestamp TLV: 30s
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV: 210s
     *     - Delay Timer TLV: ~1000s
     *     - Channel TLV: ‘Secondary’
     *     - PAN ID TLV: 0xAFCE
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: User");

    /**
     * Step 9: User
     * - Description: Places (Router_1 and Router_2) OR (Leader and Commissioner) in RF isolation for 250 seconds.
     * - Pass Criteria: N/A
     */

    leader.UnallowList(router1);
    leader.UnallowList(router2);
    router1.UnallowList(leader);
    router2.UnallowList(leader);

    nexus.AdvanceTime(kRfIsolationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_1, Router_2");

    /**
     * Step 10: Router_1, Router_2
     * - Description: Router_2 automatically creates a new partition and sets the Partition ID to its lowest possible
     *   value.
     * - Pass Criteria: For DUT = Router: The DUT MUST attach to the new partition formed by Router_2.
     */

    SuccessOrQuit(router2.Get<Mle::Mle>().BecomeLeader(Mle::Mle::kIgnoreLeaderWeight));
    nexus.AdvanceTime(kStabilizeTime * 6);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router_2");

    /**
     * Step 11: Router_2
     * - Description: Harness starts the on-mesh commissioner on Router_2 and configures the following new Active
     *   Operational Dataset values:
     *   - valid Commissioner Session ID TLV
     *   - Active Timestamp TLV: 15s
     *   - Network Name TLV: ‘TEST’
     * - Pass Criteria: N/A
     */

    SuccessOrQuit(router2.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;

        dataset.Clear();
        timestamp.SetSeconds(kActiveTimestampStep11);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        SuccessOrQuit(dataset.Update<MeshCoP::Dataset::kNetworkName>().Set("TEST"));
        router2.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(dataset);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1");

    /**
     * Step 12: Router_1
     * - Description: Automatically unicasts MLE Data Request to Router_2.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to Router_2, including the
     *   following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV (10s)
     *   - Pending Timestamp TLV (30s)
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router_2");

    /**
     * Step 13: Router_2
     * - Description: Automatically unicasts MLE Data Response to Router_1.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    /**
     * Step 14: Router_1
     * - Description: Automatically sends multicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 15s
     *   - Pending Timestamp TLV: 30s
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Router_2");

    /**
     * Step 15: Router_2
     * - Description: Harness configures the device with a new Pending Operational Dataset with the following values:
     *   - valid Commissioner Session ID TLV
     *   - Delay Timer TLV: 200s
     *   - Channel TLV: ‘Primary’
     *   - PAN ID TLV: 0xABCD
     *   - Active Timestamp TLV: 410s
     *   - Pending Timestamp TLV: 50s
     * - Pass Criteria: N/A
     */

    {
        MeshCoP::Dataset::Info pendingDataset;
        MeshCoP::Timestamp     timestamp;

        pendingDataset.Clear();

        timestamp.SetSeconds(kPendingTimestampStep15);
        pendingDataset.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kActiveTimestampStep15);
        pendingDataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        pendingDataset.Set<MeshCoP::Dataset::kDelay>(kDelayTimer200);
        pendingDataset.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        pendingDataset.Set<MeshCoP::Dataset::kPanId>(kPanIdAbcd);

        router2.Get<MeshCoP::PendingDatasetManager>().SaveLocal(pendingDataset);
    }

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Router_2");

    /**
     * Step 16: Router_2
     * - Description: Automatically sends multicast MLE Data Response with the new information including the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV including:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 15s
     *   - Pending Timestamp TLV: 50s
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1");

    /**
     * Step 17: Router_1
     * - Description: Automatically sends unicast MLE Data Request to Router_2.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to Router_2, including the
     *   following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV (15s)
     *   - Pending Timestamp TLV (30s)
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Router_2");

    /**
     * Step 18: Router_2
     * - Description: Automatically sends unicast MLE Data Response to Router_1 ….
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Router_1");

    /**
     * Step 19: Router_1
     * - Description: Automatically sends multicast MLE Data Response.
     * - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, which includes the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 15s
     *   - Pending Timestamp TLV: 50s
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: User");

    /**
     * Step 20: User
     * - Description: Removes RF isolation.
     * - Pass Criteria: N/A
     */

    leader.AllowList(router1);
    router1.AllowList(leader);

    nexus.AdvanceTime(300000); // 300s to ensure merge and dataset sync.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Router_1");

    /**
     * Step 21: Router_1
     * - Description: Automatically attaches to the Leader.
     * - Pass Criteria: For DUT = Router: The DUT MUST go through the attachment process and send MLE Child ID Request
     *   to the Leader, including the following TLV:
     *   - Active Timestamp TLV: 15s
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: Leader");

    /**
     * Step 22: Leader
     * - Description: Automatically replies to Router_1 with MLE Child ID Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MLE Child ID Response to Router_1, including its current
     *   active timestamp and active configuration set:
     *   - Active Timestamp TLV: 10s
     *   - Active Operational Dataset TLV:
     *     - Network Name TLV: “GRL”
     *   - Pending Timestamp TLV: 30s
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV: 210s
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 23: Router_1");

    /**
     * Step 23: Router_1
     * - Description: Automatically sends a MGMT_ACTIVE_SET.req to the Leader RLOC or Anycast Locator.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_ACTIVE_SET.req to the Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[Leader]:MM/c/as
     *   - CoAP Payload:
     *     - Entire Active Operational Dataset
     *     - Active Timestamp TLV: 15s
     *     - Network Name TLV: “TEST”
     *     - PAN ID TLV
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Extended PAN ID TLV
     *     - Mesh Local Prefix TLV
     *     - Network Master Key
     *     - Security Policy TLV
     *     - PSKc TLV
     *     - NO Commissioner Session ID TLV
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 24: Leader");

    /**
     * Step 24: Leader
     * - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept>
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 25: Leader");

    /**
     * Step 25: Leader
     * - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner:
     *   - CoAP Request: coap://[ Commissioner]:MM/c/dc
     *   - CoAP Payload: <empty>
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 26: Leader");

    /**
     * Step 26: Leader
     * - Description: Automatically sends multicast MLE Data Response with the new information.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 27: Router_1");

    /**
     * Step 27: Router_1
     * - Description: Automatically sends MGMT_PENDING_SET.req to the Leader Router or Anycast Locator (RLOC or ALOC).
     * - Pass Criteria: For DUT = Router: The DUT MUST send a MGMT_PENDING_SET.req to the Leader RLOC or ALOC:
     *   - CoAP Request URI: coap://[Leader]:MM/c/ps
     *   - CoAP Payload:
     *     - Delay Timer TLV: ~200s
     *     - Channel TLV: ‘Primary’
     *     - PAN ID TLV: 0xABCD
     *     - Network Name TLV: ‘TEST’
     *     - Active Timestamp TLV: 410s
     *     - Pending Timestamp TLV: 50s
     *     - Entire Pending Operational Dataset
     *     - NO Commissioner Session ID TLV
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 28: Leader");

    /**
     * Step 28: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to Router_1.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to Router_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV <value = Accept>
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 29: Leader");

    /**
     * Step 29: Leader
     * - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner:
     *   - CoAP Request: coap://[ Commissioner]:MM/c/dc
     *   - CoAP Payload: <empty>
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 30: Leader");

    /**
     * Step 30: Leader
     * - Description: Automatically sends multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response with the new information, including
     *   the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field incremented
     *     - Stable Data Version field incremented
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *       - Stable flag set to 0
     *   - Active Timestamp TLV: 15s
     *   - Pending Timestamp TLV: 50s
     */

    nexus.AdvanceTime(kStabilizeTime * 5);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 31: Commissioner");

    /**
     * Step 31: Commissioner
     * - Description: Automatically sends a MLE Data Request to the Leader, including the following TLVs:
     *   - TLV Request TLV
     *     - Network Data TLV
     *   - Active Timestamp TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 32: Leader");

    /**
     * Step 32: Leader
     * - Description: Automatically sends unicast MLE Data Response to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to the Commissioner, including
     *   the new Pending Timestamp and Pending Operational Dataset:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - PAN ID TLV: 0xABCD
     *   - Active Timestamp TLV: 15s
     *   - Active Operational Dataset TLV:
     *     - Network Name TLV: ‘TEST’
     *   - Pending Timestamp TLV: 50s
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV: 410s
     *     - Delay Timer TLV: ~200s
     *     - Channel TLV: ‘Primary’
     *     - PAN ID TLV: 0xABCD
     *     - Network Name TLV: ‘TEST’
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 33: Router_1");

    /**
     * Step 33: Router_1
     * - Description: Automatically sends MLE Data Request to the Leader.
     * - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Request to the Leader, including the following
     *   TLVs:
     *   - TLV Request TLV
     *     - Network Data TLV
     *   - Active Timestamp TLV (10s)
     *   - Pending Timestamp TLV (30s)
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 34: Leader");

    /**
     * Step 34: Leader
     * - Description: Automatically sends a unicast MLE Data Response to Router_1.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, including the new
     *   Pending Timestamp and Pending Operational Dataset:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Active Timestamp TLV: 15s
     *   - Active Operational Dataset TLV:
     *     - Network Name TLV: ‘TEST’
     *   - Pending Timestamp TLV: 50s
     *   - Pending Operational Dataset TLV:
     *     - Active Timestamp TLV: 410s
     *     - Delay Timer TLV: ~200s
     *     - Channel TLV: ‘Primary’
     *     - PAN ID TLV: 0xABCD
     */

    nexus.AdvanceTime(kStabilizeTime * 5);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 35: Router_2");

    /**
     * Step 35: Router_2
     * - Description: Automatically re-attaches to its old partition.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 36: Commissioner, Router_2");

    /**
     * Step 36: Commissioner, Router_2
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address:
     *   - For DUT = Router, the ping is sent from the Commissioner.
     *   - For DUT = Leader, the ping is sent from Router_2.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(router2, leader.Get<Mle::Mle>().GetMeshLocalEid());

    nexus.SaveTestInfo("test_9_2_9.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_9();
    printf("All tests passed\n");
    return 0;
}
