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

#include "mac/data_poll_sender.hpp"
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
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 20 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 1000;

/**
 * Time to wait for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizeTime = 10 * 1000;

/**
 * Time for the delay timer, in milliseconds.
 */
static constexpr uint32_t kDelayTimer = 60 * 1000;

/**
 * Time to power down the DUT, in milliseconds.
 */
static constexpr uint32_t kPowerDownTime = 60 * 1000;

/**
 * Time to wait for reattachment after restart, in milliseconds.
 */
static constexpr uint32_t kReattachTime = 150 * 1000;

/**
 * PAN ID for the active dataset.
 */
static constexpr uint16_t kActivePanId = 0xFACE;

/**
 * PAN ID for the pending dataset.
 */
static constexpr uint16_t kPendingPanId = 0xAFCE;

/**
 * Primary channel.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Secondary channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

/**
 * Active Timestamp for the initial dataset.
 */
static constexpr uint64_t kInitialActiveTimestamp = 10;

/**
 * Active Timestamp for the pending set.
 */
static constexpr uint64_t kPendingActiveTimestamp = 70;

/**
 * Pending Timestamp for the pending set.
 */
static constexpr uint64_t kPendingTimestamp = 20;

void Test9_2_8(void)
{
    /**
     * 9.2.8 Commissioning - Persistent Active/Pending Operational Datasets
     *
     * 9.2.8.1 Topology
     * - Commissioner
     * - Leader
     * - Router 1 (DUT)
     * - MED 1 (DUT)
     * - SED 1 (DUT)
     *
     * 9.2.8.2 Purpose & Description
     * The purpose of this test case is to verify that after a reset, the DUT reattaches to the test network using
     *   parameters set in Active/Pending Operational Datasets.
     *
     * Spec Reference                          | V1.1 Section | V1.3.0 Section
     * ----------------------------------------|--------------|---------------
     * Updating the Active Operational Dataset | 8.7.4        | 8.7.4
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

    leader.AllowList(med1);
    med1.AllowList(leader);

    leader.AllowList(sed1);
    sed1.AllowList(leader);

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;

        SuccessOrQuit(dataset.GenerateRandom(leader.GetInstance()));

        timestamp.Clear();
        timestamp.SetSeconds(kInitialActiveTimestamp);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        dataset.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        dataset.Set<MeshCoP::Dataset::kPanId>(kActivePanId);
        SuccessOrQuit(dataset.Update<MeshCoP::Dataset::kNetworkName>().Set("OpenThread"));
        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(dataset);
    }

    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader, Node::kAsMed);
    router1.Join(leader, Node::kAsFtd);
    med1.Join(leader, Node::kAsMed);
    sed1.Join(leader, Node::kAsSedWithFullNetData);

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(100));

    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs device to send MGMT_PENDING_SET.req to the Leader Anycast or Routing Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - Pending Timestamp TLV: 20s
     *     - Active Timestamp TLV: 70s
     *     - Delay Timer TLV: 60s
     *     - Channel TLV: ‘Secondary’
     *     - PAN ID TLV: 0xAFCE
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info pendingDataset;
        MeshCoP::Timestamp     timestamp;

        pendingDataset.Clear();

        timestamp.Clear();
        timestamp.SetSeconds(kPendingTimestamp);
        pendingDataset.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kPendingActiveTimestamp);
        pendingDataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        pendingDataset.Set<MeshCoP::Dataset::kDelay>(kDelayTimer);
        pendingDataset.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);
        pendingDataset.Set<MeshCoP::Dataset::kPanId>(kPendingPanId);

        SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendSetRequest(pendingDataset, nullptr, 0,
                                                                                        nullptr, nullptr));
    }

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsq to the Commissioner.
     * - Pass Criteria:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept).
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends a multicast MLE Data Response to the DUT with the new network data, including
     *   the following TLVs:
     *   - Leader Data TLV: Data Version field incremented, Stable Version field incremented
     *   - Network Data TLV: Commissioner Data TLV (Stable flag set to 0, Border Agent Locator TLV, Commissioner
     *     Session ID TLV)
     *   - Active Timestamp TLV: 70s
     *   - Pending Timestamp TLV: 20s
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: DUT");

    /**
     * Step 5: DUT
     * - Description: Automatically sends a MLE Data Request to request the full new network data.
     * - Pass Criteria: The DUT MUST send a MLE Data Request to the Leader and include its current Active Timestamp.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Automatically sends a MLE Data Response including the following TLVs: Active Timestamp TLV,
     *   Pending Timestamp TLV, Active Operational Dataset TLV, Pending Operational Dataset TLV. Ensure enough time is
     *   allowed for network data to propagate to all devices.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: User");

    /**
     * Step 7: User
     * - Description: Powers down the DUT for 60 seconds.
     * - Pass Criteria: N/A.
     */

    router1.Reset();
    med1.Reset();
    sed1.Reset();

    nexus.AdvanceTime(kPowerDownTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader, Commissioner");

    /**
     * Step 8: Leader, Commissioner
     * - Description: After Delay Timer expires, the network moves to Channel = ‘Secondary’, PAN ID: 0xAFCE.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kDelayTimer);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanId() == kPendingPanId);
    VerifyOrQuit(leader.Get<Mac::Mac>().GetPanChannel() == kSecondaryChannel);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: User");

    /**
     * Step 9: User
     * - Description: Restarts the DUT.
     * - Pass Criteria:
     *   - The DUT MUST attempt to reattach by sending Parent Request using the parameters from Active Operational
     *     Dataset (Channel = ‘Primary’, PANID: 0xFACE).
     *   - The DUT MUST then attach using the parameters from the Pending Operational Dataset (Channel = ‘Secondary’,
     *     PANID: 0xAFCE).
     */

    router1.AllowList(leader);
    med1.AllowList(leader);
    sed1.AllowList(leader);

    SuccessOrQuit(router1.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle |
                                                                        Mle::DeviceMode::kModeFullThreadDevice |
                                                                        Mle::DeviceMode::kModeFullNetworkData)));
    SuccessOrQuit(med1.Get<Mle::Mle>().SetDeviceMode(
        Mle::DeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle | Mle::DeviceMode::kModeFullNetworkData)));
    SuccessOrQuit(sed1.Get<Mle::Mle>().SetDeviceMode(Mle::DeviceMode(Mle::DeviceMode::kModeFullNetworkData)));

    router1.Get<ThreadNetif>().Up();
    med1.Get<ThreadNetif>().Up();
    sed1.Get<ThreadNetif>().Up();

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(100));

    SuccessOrQuit(router1.Get<Mle::Mle>().Start());
    SuccessOrQuit(med1.Get<Mle::Mle>().Start());
    SuccessOrQuit(sed1.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kReattachTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    VerifyOrQuit(router1.Get<Mac::Mac>().GetPanId() == kPendingPanId);
    VerifyOrQuit(med1.Get<Mac::Mac>().GetPanId() == kPendingPanId);
    VerifyOrQuit(sed1.Get<Mac::Mac>().GetPanId() == kPendingPanId);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Commissioner");

    /**
     * Step 10: Commissioner
     * - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
     *   the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(commissioner, med1.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.SendAndVerifyEchoRequest(commissioner, sed1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, 5000);

    nexus.SaveTestInfo("test_9_2_8.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_8();
    printf("All tests passed\n");
    return 0;
}
