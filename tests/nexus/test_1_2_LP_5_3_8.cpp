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

#include "meshcop/dataset.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/mle.hpp"

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
 * Time to advance for a node to join as a SSED.
 */
static constexpr uint32_t kAttachAsSsedTime = 10 * 1000;

/**
 * Time to wait for the CSL change to complete, in milliseconds.
 */
static constexpr uint32_t kWaitCslChangeTime = 18 * 1000;

/**
 * Time to wait for the network activities to complete, in milliseconds.
 */
static constexpr uint32_t kWaitNetworkActivitiesTime = 18 * 1000;

/**
 * Time to wait for the Pending Dataset to take effect, in milliseconds.
 */
static constexpr uint32_t kDelayTimerTime = 10 * 1000;

/**
 * Time to wait for the MLE Child Update Response, in milliseconds.
 */
static constexpr uint32_t kChildUpdateResponseWaitTime = 1000;

/**
 * Time to wait for the Pending Dataset to settle, in milliseconds.
 */
static constexpr uint32_t kWaitPendingDatasetTime = 30 * 1000;

/**
 * CSL Period in units of 160 microseconds. 500ms = 3125 units.
 */
static constexpr uint16_t kCslPeriod = 3125;

/**
 * CSL Synchronized Timeout in seconds.
 */
static constexpr uint32_t kCslTimeout = 20;

/**
 * Primary Channel.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Secondary Channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

/**
 * Ternary Channel.
 */
static constexpr uint8_t kTernaryChannel = 13;

/**
 * PAN ID.
 */
static constexpr uint16_t kPanId = 0x1234;

/**
 * Active Timestamp for the Pending Dataset.
 */
static constexpr uint64_t kActiveTimestamp = 200;

/**
 * Pending Timestamp for the Pending Dataset.
 */
static constexpr uint64_t kPendingTimestamp = 100;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void Test5_3_8(void)
{
    /**
     * 5.3.8 Router supports SSED modifying CSL Channel TLV
     *
     * 5.3.8.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - SSED_1
     *
     * 5.3.8.2 Purpose & Description
     * The purpose of this test case is to validate that a Router can support a SSED that modifies the CSL Channel TLV.
     *
     * Spec Reference                             | V1.2 Section
     * -------------------------------------------|--------------
     * Router supports SSED modifying CSL Channel | 4.6.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ssed1   = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    ssed1.SetName("SSED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: SSED_1");

    /**
     * Step 0: SSED_1
     * - Description: Preconditions: Set CSL Synchronized Timeout = 20s. Set CSL Period = 500ms.
     * - Pass Criteria: N/A.
     */

    ssed1.Get<Mle::Mle>().SetCslTimeout(kCslTimeout);
    ssed1.Get<Mac::Mac>().SetCslPeriod(kCslPeriod);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology formation: Leader, DUT, SSED_1.
     * - Pass Criteria: N/A.
     */

    /** Use AllowList feature to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);
    router1.AllowList(ssed1);
    ssed1.AllowList(router1);

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kPanId);
        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }
    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SSED_1");

    /**
     * Step 2: SSED_1
     * - Description: Automatically attaches to the DUT on the Primary Channel and establishes CSL synchronization.
     * - Pass Criteria:
     *   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
     *   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
     */

    ssed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsSsedTime);
    ssed1.Get<DataPollSender>().StopPolling();
    VerifyOrQuit(ssed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(ssed1.Get<Mac::Mac>().IsCslEnabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - 3.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     *   - 3.2: The Leader MUST receive the ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: SSED_1");

    /**
     * Step 4: SSED_1
     * - Description: Harness instructs the device to modify its CSL Channel TLV to the Secondary Channel and send a
     *   MLE Child Update Request.
     * - Pass Criteria: N/A.
     */

    ssed1.Get<Mac::Mac>().SetCslChannel(kSecondaryChannel);
    ssed1.Get<Mle::Mle>().ScheduleChildUpdateRequest();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1 (DUT)");

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Automatically responds with MLE Child Update Response.
     * - Pass Criteria:
     *   - 5.1: The DUT MUST send MLE Child Update Response on the Secondary Channel.
     */

    nexus.AdvanceTime(kChildUpdateResponseWaitTime);
    ssed1.Get<DataPollSender>().StopPolling();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Harness");

    /**
     * Step 6: Harness
     * - Description: Harness waits 18 seconds for the CSL change to complete.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kWaitCslChangeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local
     *   address.
     * - Pass Criteria:
     *   - 7.1: The DUT MUST transmit the ICMPv6 Echo Request on the Secondary Channel.
     *   - 7.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     *   - 7.3: The Leader MUST receive the ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: SSED_1");

    /**
     * Step 8: SSED_1
     * - Description: Harness instructs the device to modify its CSL Channel TLV back to the original (Primary)
     *   Channel and send a MLE Child Update Request. To do this, the Harness sends the channel value ‘0’ which
     *   denotes ‘same channel as used for Thread Network’. Note: in OT CLI, this is set with `csl channel 0`.
     * - Pass Criteria: N/A.
     */

    ssed1.Get<Mac::Mac>().SetCslChannel(0);
    ssed1.Get<Mle::Mle>().ScheduleChildUpdateRequest();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1 (DUT)");

    /**
     * Step 9: Router_1 (DUT)
     * - Description: Automatically responds with a MLE Child Update Response.
     * - Pass Criteria:
     *   - 9.1: The DUT MUST send a MLE Child Update Response.
     */

    nexus.AdvanceTime(kChildUpdateResponseWaitTime);
    ssed1.Get<DataPollSender>().StopPolling();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Harness");

    /**
     * Step 10: Harness
     * - Description: Harness waits 18 seconds for the network activities to complete.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kWaitNetworkActivitiesTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");

    /**
     * Step 11: Leader
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   SSED_1 mesh-local address.
     * - Pass Criteria:
     *   - 11.1: The DUT MUST transmit the ICMPv6 Echo Request on the Primary Channel.
     *   - 11.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     *   - 11.3: The Leader MUST receive the ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader");

    /**
     * Step 12: Leader
     * - Description: Harness configures a new Terniary channel for the Thread Network, using the Pending Dataset.
     *   The new dataset automatically propagates to all devices in the network.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kTernaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kDelay>(kDelayTimerTime);
        {
            MeshCoP::Timestamp timestamp;

            timestamp.Clear();
            timestamp.SetSeconds(kActiveTimestamp);
            datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

            timestamp.Clear();
            timestamp.SetSeconds(kPendingTimestamp);
            datasetInfo.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);
        }

        leader.Get<MeshCoP::PendingDatasetManager>().SaveLocal(datasetInfo);
    }

    nexus.AdvanceTime(2 * kChildUpdateResponseWaitTime);
    ssed1.Get<DataPollSender>().StopPolling();
    nexus.AdvanceTime(kWaitPendingDatasetTime - 2 * kChildUpdateResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader");

    /**
     * Step 13: Leader
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local
     *   address.
     * - Pass Criteria:
     *   - 13.1: The DUT MUST transmit the ICMPv6 Echo Request on the Terniary Channel.
     *   - 13.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
     */

    nexus.SendAndVerifyEchoRequest(leader, ssed1.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize);

    nexus.SaveTestInfo("test_1_2_LP_5_3_8.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_8();
    printf("All tests passed\n");
    return 0;
}
