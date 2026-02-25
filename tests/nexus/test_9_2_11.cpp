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
 *     names of its contributors may be endorse or promote products
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

#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread.h>

#include "mac/data_poll_sender.hpp"
#include "meshcop/commissioner.hpp"
#include "meshcop/dataset_manager.hpp"
#include "net/ip6.hpp"
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
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 1000;

/**
 * Time to wait for network stabilization, in milliseconds.
 */
static constexpr uint32_t kStabilizeTime = 10 * 1000;

/**
 * Time to wait for data propagation, in milliseconds.
 */
static constexpr uint32_t kPropagationTime = 5 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Pending Timestamp value for Step 2.
 */
static constexpr uint64_t kPendingTimestampStep2 = 10;

/**
 * Active Timestamp value for Step 2.
 */
static constexpr uint64_t kActiveTimestampStep2 = 10;

/**
 * Delay Timer value for Step 2.
 */
static constexpr uint32_t kDelayTimerStep2 = 60;

/**
 * New Network Key for Step 2.
 */
static const uint8_t kNewNetworkKeyStep2[] = {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88,
                                              0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

/**
 * Pending Timestamp value for Step 10.
 */
static constexpr uint64_t kPendingTimestampStep10 = 20;

/**
 * Active Timestamp value for Step 10.
 */
static constexpr uint64_t kActiveTimestampStep10 = 70;

/**
 * Delay Timer value for Step 10.
 */
static constexpr uint32_t kDelayTimerStep10 = 500;

/**
 * New Network Key for Step 10.
 */
static const uint8_t kNewNetworkKeyStep10[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                               0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

/**
 * Wait time in seconds for Step 8.
 */
static constexpr uint32_t kWaitTimeStep8 = 310;

/**
 * Wait time in seconds for Step 16.
 */
static constexpr uint32_t kWaitTimeStep16 = 510;

void Test9_2_11(void)
{
    /**
     * 9.2.11 Commissioning - Leader Delay Timer Management
     *
     * 9.2.11.1 Topology
     * - Leader (DUT)
     * - Commissioner
     * - Router_1
     * - MED_1
     * - SED_1
     *
     * 9.2.11.2 Purpose & Description
     * The purpose of this test case is to confirm the DUT correctly applies DELAY_TIMER_DEFAULT when the master key is
     * changed. The Commissioner first tries to set a master key update to happen too soon (delay of 60s vs
     * DELAY_TIMER_DEFAULT of 300s); the DUT is expected to override the short value and communicate an appropriately
     * longer delay to the Router. The Commissioner then sets a delay time longer than default; the DUT is validated to
     * not artificially clamp the longer time back to the DELAY_TIMER_DEFAULT value.
     *
     * Spec Reference           | V1.1 Section | V1.3.0 Section
     * -------------------------|--------------|---------------
     * Parameters and Constants | 8.11         | 8.11
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
     * - Description: Topology Ensure topology is formed correctly.
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

        datasetInfo.Clear();
        SuccessOrQuit(leader.Get<MeshCoP::ActiveDatasetManager>().CreateNewNetwork(datasetInfo));

        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(*reinterpret_cast<const NetworkKey *>(kNewNetworkKeyStep10));

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
    }

    leader.Get<ThreadNetif>().Up();
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());
    SuccessOrQuit(commissioner.Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters));

    router1.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());
    SuccessOrQuit(router1.Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters));

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(2000)); // Keep SED alive

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SetId("commissioner"));
    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kStabilizeTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs Commissioner to send MGMT_PENDING_SET.req to the DUT Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - Pending Timestamp TLV <10s>
     *     - Active Timestamp TLV <70s>
     *     - Delay Timer TLV <60s>
     *     - Network Master Key TLV: New Master Key
     *   - The DUT Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
     * - Pass Criteria: N/A
     */

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;

        datasetInfo.Clear();

        timestamp.Clear();
        timestamp.SetSeconds(kActiveTimestampStep2);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kPendingTimestampStep2);
        datasetInfo.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        datasetInfo.Set<MeshCoP::Dataset::kDelay>(kDelayTimerStep2 * 1000);
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(*reinterpret_cast<const NetworkKey *>(kNewNetworkKeyStep2));

        SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendSetRequest(datasetInfo, nullptr, 0,
                                                                                        nullptr, nullptr));
    }

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to Commissioner and multicasts a MLE Data Response.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV <Accept>
     *   - The DUT MUST multicast MLE Data Response with the new network information, including the following TLVs:
     *     - Source Address TLV
     *     - Leader Data TLV:
     *       - Data Version field <incremented>
     *       - Stable Data Version field <incremented>
     *     - Network Data TLV:
     *       - Commissioning Data TLV:
     *         - Stable flag <set to 0>
     *         - Border Agent Locator TLV
     *         - Commissioner Session ID TLV
     *     - Active Timestamp TLV <10s>
     *     - Pending Timestamp TLV <10s>
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     * - Description: Automatically sends a unicast MLE Data Request to the DUT, including the following TLVs:
     *   - TLV Request TLV:
     *     - Active Timestamp TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader (DUT)");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to Router_1.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Router_1, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Commissioner Session ID TLV
     *       - Border Agent Locator TLV
     *   - Active Timestamp TLV <10s>
     *   - Pending Timestamp TLV <10s>
     *   - Pending Operational Dataset TLV
     *     - Delay Timer TLV <greater than 200s>
     *     - Network Master Key TLV: New Master Key
     *     - Active Timestamp TLV <70s>
     */

    nexus.AdvanceTime(kPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     * - Description: Automatically transmits the new network data to MED_1 by sending a multicast MLE Data Response to
     *   Link-Local All Nodes), including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV:
     *     - Data Version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV:
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <10s>
     *   - Pending Timestamp TLV <10s>
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     * - Description: Depending on the device implementation, automatically transmits the new network data to SED_1 by
     *   sending EITHER a MLE Data Response OR a MLE Child Update Request, each including the following TLVs:
     *   - Leader Data TLV:
     *     - Data Version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV
     *   - Active Timestamp TLV <10s>
     *   - Pending Timestamp TLV <10s>
     *   - Source Address TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: All");

    /**
     * Step 8: All
     * - Description: Wait for 300 seconds to expire.
     * - Pass Criteria: Verify all devices now use New Master key.
     */

    nexus.AdvanceTime(kWaitTimeStep8 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1");

    /**
     * Step 9: Router_1
     * - Description: Harness instructs Router_1 to send an ICMPv6 Echo Request on ML-RLOC from Router_1 to the DUT.
     * - Pass Criteria: Verify new MAC key is generated and used when sending ICMPv6 Echo Reply is received.
     */

    nexus.SendAndVerifyEchoRequest(router1, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Commissioner");

    /**
     * Step 10: Commissioner
     * - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the DUT Routing or Anycast
     *   Locator:
     *   - CoAP Request URI: CON POST coap://[Leader]:MM/c/ps
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV <valid>
     *     - Pending Timestamp TLV <20s>
     *     - Active Timestamp TLV <30s>
     *     - Delay Timer TLV <500s>
     *     - Network Master Key TLV: new master key
     *   - The DUT Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    {
        MeshCoP::Dataset::Info datasetInfo;
        MeshCoP::Timestamp     timestamp;

        datasetInfo.Clear();

        timestamp.Clear();
        timestamp.SetSeconds(kActiveTimestampStep10);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kPendingTimestampStep10);
        datasetInfo.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        datasetInfo.Set<MeshCoP::Dataset::kDelay>(kDelayTimerStep10 * 1000);
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(*reinterpret_cast<const NetworkKey *>(kNewNetworkKeyStep10));

        SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendSetRequest(datasetInfo, nullptr, 0,
                                                                                        nullptr, nullptr));
    }

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader (DUT)");

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically sends a MGMT_PENDING_SET.rsp to Commissioner and multicasts MLE Data Response.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV <Accept>
     *   - The DUT MUST multicast a MLE Data Response with the new information including the following TLVs:
     *     - Leader Data TLV
     *       - Data Version field <incremented>
     *       - Stable Data Version field <incremented>
     *     - Network Data TLV
     *       - Commissioning Data TLV:
     *         - Stable flag <set to 0>
     *         - Border Agent Locator TLV
     *         - Commissioner Session ID TLV
     *     - Active Timestamp TLV <70s>
     *     - Pending Timestamp TLV <20s>
     *     - Source Address TLV
     */

    nexus.AdvanceTime(10000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1");

    /**
     * Step 12: Router_1
     * - Description: Automatically sends a unicast MLE Data Request to the DUT, including the following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kStabilizeTime * 2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader (DUT)");

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically sends unicast MLE Data Response to Router_1.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Router_1, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <70s>
     *   - Pending Timestamp TLV <20s>
     *   - Pending Operational Dataset TLV
     *     - Active Timestamp TLV <30s>
     *     - Delay Timer TLV <greater than 300s>
     *     - Network Master Key TLV: new master key (set in step 10)
     */

    nexus.AdvanceTime(kPropagationTime * 2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    /**
     * Step 14: Router_1
     * - Description: Automatically transmits the new network data to MED_1 by sending a multicast MLE Data Response to
     *   the Link-Local All Nodes, including the following TLVs:
     *   - Leader Data TLV:
     *     - Data Version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV
     *     - Commissioning Data TLV:
     *       - Stable flag <set to 0>
     *       - Border Agent Locator TLV
     *       - Commissioner Session ID TLV
     *   - Active Timestamp TLV <70s>
     *   - Pending Timestamp TLV <20s>
     *   - Source Address TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kPropagationTime * 2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Router_1");

    /**
     * Step 15: Router_1
     * - Description: Depending on the device implementation, automatically transmits the new network data to SED_1 by
     *   sending EITHER a MLE Data Response OR a MLE Child Update Request, each including the following TLVs:
     *   - Leader Data TLV:
     *     - Data version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV
     *   - Active Timestamp TLV <70s>
     *   - Pending Timestamp TLV <20s>
     *   - Source Address TLV
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kPropagationTime * 2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Harness");

    /**
     * Step 16: Harness
     * - Description: Waits for 510 seconds to expire.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWaitTimeStep16 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1");

    /**
     * Step 17: Router_1
     * - Description: Harness instructs Router_1 to send ICMPv6 Echo Request on ML-RLOC from Router_1 to the Leader
     *   (DUT).
     * - Pass Criteria: The DUT MUST send an ICMPv6 Echo Reply using the new Master key.
     */

    nexus.SendAndVerifyEchoRequest(router1, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.AdvanceTime(kResponseTime * 5); // Added extra wait at the very end

    nexus.SaveTestInfo("test_9_2_11.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_11();
    printf("All tests passed\n");
    return 0;
}
