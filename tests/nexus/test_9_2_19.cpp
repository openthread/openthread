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
static constexpr uint32_t kJoinTime = 10 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 2000;

/**
 * Delay timer value in milliseconds (1 minute).
 */
static constexpr uint32_t kDelayTimer = 60 * 1000;

/**
 * Time to wait for pending data to become operational, in milliseconds.
 */
static constexpr uint32_t kWaitDelayTime = 120 * 1000;

/**
 * New PAN ID value.
 */
static constexpr uint16_t kNewPanId = 0xAFCE;

/**
 * Active Timestamp value.
 */
static constexpr uint64_t kActiveTimestamp = 60;

/**
 * Pending Timestamp value.
 */
static constexpr uint64_t kPendingTimestamp = 30;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest9_2_19(Topology aTopology, const char *aJsonFile)
{
    /**
     * 9.2.19 Getting the Pending Operational Dataset
     *
     * 9.2.19.1 Topology
     * - Topology A: DUT as Leader, Commissioner (Non-DUT)
     * - Topology B: Leader (Non-DUT), DUT as Commissioner
     *
     * 9.2.19.2 Purpose & Description
     * - DUT as Leader (Topology A): The purpose of this test case is to verify the DUT’s behavior when receiving
     *   MGMT_PENDING_GET.req directly from the active Commissioner.
     * - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the DUT can read Pending
     *   Operational Dataset parameters direct from the Leader using the MGMT_PENDING_GET.req command.
     *
     * Spec Reference                            | V1.1 Section | V1.3.0 Section
     * ------------------------------------------|--------------|---------------
     * Updating the Pending Operational Dataset | 8.7.5        | 8.7.5
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();

    leader.SetName("LEADER");
    commissioner.SetName("COMMISSIONER");

    OT_UNUSED_VARIABLE(aTopology);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

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

    // Wait for nodes to stabilize and potentially become routers
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT Anycast or Routing Locator:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty>
     * - Pass Criteria:
     *   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
     *     - The Destination Address of MGMT_PENDING_GET.req frame MUST be Leader’s Anycast or Routing Locator (ALOC or
     *       RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID
     *   - Topology A: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Topology B Commissioner DUT / Topology A Leader DUT");

    SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendGetRequest(MeshCoP::Dataset::Components(),
                                                                                    nullptr, 0, nullptr));
    nexus.AdvanceTime(kResponseTime);

    /**
     * Step 3: Leader
     * - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: <empty> (no Pending Operational Dataset)
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 4: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_PENDING_SET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_SET.req to DUT’s Anycast or Routing Locator:
     *     - CoAP Request URI: coap://[<L>]:MM/c/ps
     *     - CoAP Payload: Active Timestamp TLV: 60s, Commissioner Session ID TLV (valid), Delay Timer TLV: 1 minute,
     *       PAN ID TLV: 0xAFCE (new value), Pending Timestamp TLV: 30s.
     * - Pass Criteria:
     *   - Topology B: The MGMT_PENDING_SET.req frame MUST have the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/ps
     *     - CoAP Payload: Active Timestamp TLV: 60s, Commissioner Session ID TLV (valid), Delay Timer TLV: 1 minute,
     *       Pending Timestamp TLV: 30s, PAN ID TLV: 0xAFCE (new value).
     *     - The Destination Address of MGMT_PENDING_SET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
     *       or RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID.
     *   - Topology A: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Topology B Commissioner DUT / Topology A Leader DUT");

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;

        dataset.Clear();

        timestamp.Clear();
        timestamp.SetSeconds(kActiveTimestamp);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        timestamp.Clear();
        timestamp.SetSeconds(kPendingTimestamp);
        dataset.Set<MeshCoP::Dataset::kPendingTimestamp>(timestamp);

        dataset.Set<MeshCoP::Dataset::kDelay>(kDelayTimer);
        dataset.Set<MeshCoP::Dataset::kPanId>(kNewPanId);

        SuccessOrQuit(
            commissioner.Get<MeshCoP::PendingDatasetManager>().SendSetRequest(dataset, nullptr, 0, nullptr, nullptr));
    }
    nexus.AdvanceTime(kResponseTime);

    /**
     * Step 5: Leader
     * - Description: Automatically responds to MGMT_PENDING_SET.req with a MGMT_PENDING_SET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The MGMT_PENDING_SET.rsp frame MUST have the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 6: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty>
     * - Pass Criteria:
     *   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
     *     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
     *       or RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID
     *   - Topology A: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Topology B Commissioner DUT / Topology A Leader DUT");

    SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendGetRequest(MeshCoP::Dataset::Components(),
                                                                                    nullptr, 0, nullptr));
    nexus.AdvanceTime(kResponseTime);

    /**
     * Step 7: Leader
     * - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Active Timestamp TLV
     *     - Channel TLV
     *     - Channel Mask TLV
     *     - Delay Timer TLV
     *     - Extended PAN ID TLV
     *     - Mesh-Local Prefix TLV
     *     - Network Master Key TLV
     *     - Network Name TLV
     *     - PAN ID TLV
     *     - Pending Timestamp TLV
     *     - PSKc TLV
     *     - Security Policy TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 8: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: Get TLV specifying: PAN ID TLV
     * - Pass Criteria:
     *   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: Get TLV specifying: PAN ID TLV
     *     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
     *       or RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID
     *   - Topology A: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Topology B Commissioner DUT / Topology A Leader DUT");

    {
        uint8_t tlvTypes[] = {MeshCoP::Tlv::kPanId};
        SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendGetRequest(
            MeshCoP::Dataset::Components(), tlvTypes, sizeof(tlvTypes), nullptr));
    }
    nexus.AdvanceTime(kResponseTime);

    /**
     * Step 9: Leader
     * - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Delay Timer TLV, PAN ID TLV
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 10: Harness
     * - Description: Wait for 92 seconds to allow pending data to become operational.
     * - Pass Criteria: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Harness");

    nexus.AdvanceTime(kWaitDelayTime);

    /**
     * Step 11: Topology B Commissioner DUT / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty>
     * - Pass Criteria:
     *   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/pg
     *     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
     *     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
     *       or RLOC):
     *       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *         Router ID
     *   - Topology A: N/A.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Topology B Commissioner DUT / Topology A Leader DUT");

    SuccessOrQuit(commissioner.Get<MeshCoP::PendingDatasetManager>().SendGetRequest(MeshCoP::Dataset::Components(),
                                                                                    nullptr, 0, nullptr));
    nexus.AdvanceTime(kResponseTime);

    /**
     * Step 12: Leader
     * - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: <empty> (no Pending Operational Dataset)
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader");
    nexus.AdvanceTime(kResponseTime);
    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest9_2_19(ot::Nexus::kTopologyA, "test_9_2_19_A.json");
        ot::Nexus::RunTest9_2_19(ot::Nexus::kTopologyB, "test_9_2_19_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest9_2_19(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_9_2_19_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest9_2_19(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_9_2_19_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
