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
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for commissioner keep-alive, in milliseconds.
 */
static constexpr uint32_t kKeepAliveTime = 50 * 1000;

void Test_1_1_8_3_1(void)
{
    /**
     * 8.3.1 On Mesh Commissioner - Commissioner Petitioning, Commissioner Keep-alive messaging, Steering Data Updating
     *   and Commissioner Resigning
     *
     * 8.3.1.1 Topology
     * - Topology A: DUT is the Leader, communicating with a Commissioner.
     * - Topology B: DUT is the Commissioner, communicating with a Leader.
     *
     * 8.3.1.2 Purpose & Description
     * - Commissioner as a DUT: The purpose of this test case is to verify that a Commissioner Candidate is able to
     *   register itself to the network as a Commissioner, send periodic Commissioner keep-alive messages, update
     *   steering data and unregister itself as a Commissioner.
     * - Leader as a DUT: The purpose of this test case is to verify that the Leader accepts the Commissioner Candidate
     *   as a Commissioner in the network, responds to periodic Commissioner keep-alive messages, propagates Thread
     *   Network Data correctly in the network and unregisters the Commissioner on its request.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Commissioning    | 8.7 / 8.8    | 8.7 / 8.8
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();

    leader.SetName("LEADER");
    commissioner.SetName("COMMISSIONER");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Begin wireless sniffer and ensure topology is created and connectivity between nodes.
     * - Pass Criteria: N/A.
     */

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     * - Description: Commissioner sends a petition request to Leader.
     * - Pass Criteria:
     *   Verify that:
     *   - Commissioner sends a TMF Leader Petition Request (LEAD_PET.req) to Leader.
     *     - CoAP Request URI: CON POST coap://<L>:MM/c/lp
     *     - CoAP Payload: Commissioner ID TLV
     *
     *   Fail conditions:
     *   - Commissioner does not send a TMF Leader Petition Request to Leader with the correct TLV.
     */

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Leader accepts Commissioner to the network.
     * - Pass Criteria:
     *   Verify that:
     *   - Leader sends a TMF Leader Petition Response (LEAD_PET.rsp) to Commissioner.
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV (value = Accept), Commissioner ID TLV (optional), Commissioner Session ID TLV.
     *   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
     *     TLV, Network Data TLV, Source Address TLV.
     *     - Leader Data TLV shows increased Data Version number.
     *     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set.
     *     - Commissioning Data TLV contains Commissioner Session ID and Border Agent Locator sub-TLVs.
     *
     *   Fail conditions:
     *   - Leader does not send a TMF Leader Petition response to Commissioner with correct TLVs.
     *   - Leader does not send a MLE Data Response message with correct TLVs.
     */

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Commissioner");

    /**
     * Step 4: Commissioner
     * - Description: Within 50 seconds Commissioner sends Commissioner keep-alive message.
     * - Pass Criteria:
     *   Verify that:
     *   - Commissioner sends a TMF Leader Petition Keep Alive Request (LEAD_KA.req) to Leader.
     *     - CoAP Request URI: CON POST coap://<L>:MM/c/la
     *     - CoAP Payload: State TLV (value = Accept), Commissioner Session ID TLV.
     *
     *   Fail conditions:
     *   - Commissioner does not send a TMF Leader Petition Keep Alive Request to Leader with the correct TLVs.
     */

    nexus.AdvanceTime(kKeepAliveTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Leader responds with ‘Accept’.
     * - Pass Criteria:
     *   Verify that:
     *   - Leader sends a TMF Leader Petition Keep Alive Response (LEAD_KA.rsp) to Commissioner.
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV (value = Accept).
     *
     *   Fail conditions:
     *   - Leader does not send a TMF Leader Petition Keep Alive Response to Commissioner with correct TLV.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Commissioner");

    /**
     * Step 6: Commissioner
     * - Description: Commissioner adds a new device to be commissioned. If Commissioner is a test bed device, it uses
     *   Leader Anycast address as destination address.
     * - Pass Criteria:
     *   Verify that:
     *   - Commissioner sends a TMF Set Commissioner Dataset Request (MGMT_COMMISSIONER_SET.req) to Leader ALOC 0xFC00
     *     or Leader RLOC.
     *     - CoAP Request URI: CON POST coap://<L>:MM/c/cs
     *     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV.
     *
     *   Fail conditions:
     *   - Commissioner does not send a TMF Set Commissioner Dataset Request to Leader with the correct TLVs.
     */

    {
        MeshCoP::CommissioningDataset dataset;
        dataset.Clear();
        dataset.mIsSteeringDataSet = true;
        AsCoreType(&dataset.mSteeringData).SetToPermitAllJoiners();

        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SendMgmtCommissionerSetRequest(dataset, nullptr, 0));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Leader Responds with ‘Accept’.
     * - Pass Criteria:
     *   Verify that:
     *   - Leader sends a TMF Set Commissioner Dataset Response (MGMT_COMMISSIONER_SET.rsp) to the source address of
     *     the previous TMF Set Commissioner Dataset Request:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV (value = Accept).
     *   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
     *     TLV, Network Data TLV, Source Address TLV.
     *     - Leader Data TLV shows increased Data Version number.
     *     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set.
     *     - Commissioning Data TLV contains Commissioner Session ID, Border Agent Locator and Steering Data sub-TLVs.
     *
     *   Fail conditions:
     *   - Leader does not send a TMF Set Commissioner Dataset Response to Commissioner with correct TLV.
     *   - Leader does not send a MLE Data Response message with correct TLVs.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Commissioner");

    /**
     * Step 8: Commissioner
     * - Description: Commissioner sends a resign request to Leader.
     * - Pass Criteria:
     *   Verify that:
     *   - Commissioner sends a TMF Leader Petition Keep Alive Request (LEAD_KA.req) to Leader.
     *     - CoAP Request URI: CON POST coap://<L>:MM/c/la
     *     - CoAP Payload: State TLV (value = Reject), Commissioner Session ID TLV.
     *
     *   Fail conditions:
     *   - Commissioner does not send a TMF Leader Petition Keep Alive Request to Leader with the correct TLV.
     */

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Stop());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 9: Leader
     * - Description: Leader accepts the resignation by responding with ‘Reject’.
     * - Pass Criteria:
     *   Verify that:
     *   - Leader sends a TMF Leader Petition Keep Alive Response (LEAD_KA.rsp) to Commissioner.
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV (value = Reject).
     *   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
     *     TLV, Network Data TLV, Source Address TLV.
     *     - Leader Data TLV shows increased Data Version number.
     *     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set and incremented value for
     *       Commissioner Session ID.
     *
     *   Fail conditions:
     *   - Leader does not send a TMF Leader Petition Keep Alive Response to Commissioner with correct TLV.
     *   - Leader does not send a MLE Data Response message with correct TLVs.
     */

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsDisabled());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Commissioner");

    /**
     * Step 10: Commissioner
     * - Description: Commissioner sends a petition request to Leader.
     * - Pass Criteria: Should there be something listed here for the Commissioner DUT?
     */

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");

    /**
     * Step 11: Leader
     * - Description: Leader accepts Commissioner to the network.
     * - Pass Criteria:
     *   Verify that:
     *   - Leader sends a TMF Leader Petition Response (LEAD_PET.rsp) to Commissioner.
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: State TLV (value = Accept), Commissioner ID TLV (optional), Commissioner Session ID TLV.
     *     - Commissioner Session ID TLV contains higher Session ID number than in Step 9.
     */

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    nexus.SaveTestInfo("test_1_1_8_3_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_1_8_3_1();
    printf("All tests passed\n");
    return 0;
}
