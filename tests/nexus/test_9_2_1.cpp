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
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 10 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest9_2_1(Topology aTopology, const char *aJsonFile)
{
    /**
     * 9.2.1 Commissioner – MGMT_COMMISSIONER_GET.req & rsp
     *
     * 9.2.1.1 Topology
     * - Topology A: DUT as Leader, Commissioner (Non-DUT)
     * - Topology B: Leader (Non-DUT), DUT as Commissioner
     *
     * 9.2.1.2 Purpose & Description
     * - DUT as Leader (Topology A): The purpose of this test case is to verify Leader’s behavior when receiving
     *   MGMT_COMMISSIONER_GET.req direct from the active Commissioner.
     * - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the active Commissioner can
     *   read Commissioner Dataset parameters direct from the Leader using MGMT_COMMISSIONER_GET.req command.
     *
     * Spec Reference                    | V1.1 Section | V1.3.0 Section
     * ----------------------------------|--------------|---------------
     * Updating the Commissioner Dataset | 8.7.3        | 8.7.3
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();

    leader.SetName("LEADER");
    commissioner.SetName("COMMISSIONER");

    Node &dut = (aTopology == kTopologyA) ? leader : commissioner;

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    leader.AllowList(commissioner);
    commissioner.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Topology B Commissioner DUT / Topology A Leader DUT");

    /**
     * Step 2: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_COMMISSIONER_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
     *     Locator.
     * - Pass Criteria:
     *   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cg
     *     - CoAP Payload: <empty> - get all Commissioner Dataset parameters
     *     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
     *       RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID.
     *   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: <empty> - get all Commissioner Dataset
     *     parameters.
     */

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SendMgmtCommissionerGetRequest(nullptr, 0));
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: (entire Commissioner Dataset)
     *     - Border Agent Locator TLV
     *     - Commissioner Session ID TLV
     *     - Steering Data TLV.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Topology B Commissioner DUT / Topology A Leader DUT");

    /**
     * Step 4: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_COMMISSIONER_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
     *     Locator.
     * - Pass Criteria:
     *   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cg
     *     - CoAP Payload: Get TLV specifying: Commissioner Session ID TLV, Steering Data TLV
     *     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
     *       RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID.
     *   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Commissioner Session ID
     *     TLV, Steering Data TLV.
     */

    {
        uint8_t tlvs[] = {MeshCoP::Tlv::kCommissionerSessionId, MeshCoP::Tlv::kSteeringData};
        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SendMgmtCommissionerGetRequest(tlvs, sizeof(tlvs)));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Encoded values for the requested Commissioner Dataset parameters:
     *     - Commissioner Session ID TLV
     *     - Steering Data TLV.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Topology B Commissioner DUT / Topology A Leader DUT");

    /**
     * Step 6: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
     *     Locator.
     * - Pass Criteria:
     *   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cg
     *     - CoAP Payload: Get TLV specifying: Commissioner Session ID TLV (parameter in Commissioner Dataset), PAN ID
     *       TLV (parameter not in Commissioner Dataset)
     *     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
     *       RLOC).
     *   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Commissioner Session ID
     *     TLV (parameter in Commissioner Dataset), PAN ID TLV (parameter not in Commissioner Dataset).
     */

    {
        uint8_t tlvs[] = {MeshCoP::Tlv::kCommissionerSessionId, MeshCoP::Tlv::kPanId};
        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SendMgmtCommissionerGetRequest(tlvs, sizeof(tlvs)));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Encoded value for the requested Commissioner Dataset parameters:
     *     - Commissioner Session ID TLV
     *     - (PAN ID TLV in Get TLV is ignored).
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Topology B Commissioner DUT / Topology A Leader DUT");

    /**
     * Step 8: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
     *     Locator.
     * - Pass Criteria:
     *   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cg
     *     - CoAP Payload: Get TLV specifying: Border Agent Locator TLV, Network Name TLV
     *     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast OR Routing Locator (ALOC or
     *       RLOC).
     *   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Border Agent Locator
     *     TLV, Network Name TLV.
     */

    {
        uint8_t tlvs[] = {MeshCoP::Tlv::kBorderAgentLocator, MeshCoP::Tlv::kNetworkName};
        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().SendMgmtCommissionerGetRequest(tlvs, sizeof(tlvs)));
    }
    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 9: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Encoded value for the requested Commissioner Dataset parameters:
     *     - Border Agent Locator TLV
     *     - (Network Name TLV is ignored).
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: All");

    /**
     * Step 10: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(aTopology == kTopologyA ? commissioner : leader,
                                   dut.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest9_2_1(ot::Nexus::kTopologyA, "test_9_2_1_A.json");
        ot::Nexus::RunTest9_2_1(ot::Nexus::kTopologyB, "test_9_2_1_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest9_2_1(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_9_2_1_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest9_2_1(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_9_2_1_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
