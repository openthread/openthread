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
static constexpr uint32_t kResponseTime = 5000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

/**
 * Channel used for the main network.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Channel used for the separate network with same PAN ID.
 */
static constexpr uint8_t kSecondaryChannel = 20;

/**
 * PAN ID to use for both networks.
 */
static constexpr uint16_t kPanId = 0x1234;

void Test9_2_14(void)
{
    /**
     * 9.2.14 PAN ID Query Requests
     *
     * 9.2.14.1 Topology
     *   - Leader_2 forms a separate network on the Secondary channel, with the same PAN ID.
     *
     * 9.2.14.2 Purpose & Description
     *   The purpose of this test case is to ensure that the DUT is able to properly accept and process PAN ID
     *   Query requests and properly respond when a conflict is found.
     *
     * Spec Reference            | V1.1 Section | V1.3.0 Section
     * --------------------------|--------------|---------------
     * Avoiding PAN ID Conflicts | 8.7.9        | 8.7.9
     */

    Core nexus;

    Node &leader1      = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();
    Node &leader2      = nexus.CreateNode();

    leader1.SetName("LEADER_1");
    router1.SetName("ROUTER_1");
    commissioner.SetName("COMMISSIONER");
    leader2.SetName("LEADER_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     *   - Description: Topology Ensure topology is formed correctly.
     *   - Pass Criteria: N/A
     */

    leader1.AllowList(router1);
    leader1.AllowList(commissioner);

    router1.AllowList(leader1);
    router1.AllowList(leader2);

    commissioner.AllowList(leader1);

    leader2.AllowList(router1);

    {
        MeshCoP::Dataset::Info datasetInfo;

        SuccessOrQuit(datasetInfo.GenerateRandom(leader1.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kPanId>(kPanId);
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        leader1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

        leader1.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader1.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader1.Get<Mle::Mle>().IsLeader());

    router1.Join(leader1);
    commissioner.Join(leader1);

    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    while (!router1.Get<Mle::Mle>().IsRouter() || !commissioner.Get<Mle::Mle>().IsRouter())
    {
        nexus.AdvanceTime(1000);
    }

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    {
        MeshCoP::Dataset::Info datasetInfo;

        SuccessOrQuit(leader1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);

        leader2.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader2.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader2.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader2.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner");

    /**
     * Step 2: Commissioner
     *   - Description: Harness instructs the Commissioner to send a unicast MGMT_PANID_QUERY.qry to Router_1. For
     *     DUT = Commissioner: Through implementation-specific means, the user instructs the DUT to send a
     *     MGMT_PANID_QUERY.qry to Router_1.
     *   - Pass Criteria: For DUT = Commissioner: The DUT MUST send a unicast MGMT_PANID_QUERY.qry unicast to
     *     Router_1:
     *     - CoAP Request URI: coap://[R]:MM/c/pq
     *     - CoAP Payload:
     *       - Commissioner Session ID TLV
     *       - Channel Mask TLV
     *       - PAN ID TLV
     */

    {
        uint32_t channelMask = (1 << kSecondaryChannel);
        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().GetPanIdQueryClient().SendQuery(
            kPanId, channelMask, router1.Get<Mle::Mle>().GetMeshLocalRloc(), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     *   - Description: Automatically sends a MGMT_PANID_CONFLICT.ans reponse to the Commissioner.
     *   - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_PANID_CONFLICT.ans to the Commissioner:
     *     - CoAP Request URI: coap://[Commissioner]:MM/c/pc
     *     - CoAP Payload:
     *       - Channel Mask TLV
     *       - PAN ID TLV
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Commissioner");

    /**
     * Step 4: Commissioner
     *   - Description: Harness instructs Commissioner to send MGMT_PANID_QUERY.qry to All Thread Node Multicast
     *     Address: FF33:0040:<mesh local prefix>::1. For DUT = Commissioner: Through implementation-specific
     *     means, the user instructs the DUT to send a MGMT_PANID_QUERY.qry.
     *   - Pass Criteria: For DUT = Commissioner: The DUT MUST send a multicast MGMT_PANID_QUERY.qry
     *     - CoAP Request URI: coap://[Destination]:MM/c/pq
     *     - CoAP Payload:
     *       - Commissioner Session ID TLV
     *       - Channel Mask TLV
     *       - PAN ID TLV
     */

    {
        uint32_t channelMask = (1 << kSecondaryChannel);
        SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().GetPanIdQueryClient().SendQuery(
            kPanId, channelMask, commissioner.Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress(), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     *   - Description: Automatically sends a MGMT_PANID_CONFLICT.ans reponse to the Commissioner.
     *   - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_PANID_CONFLICT.ans to the Commissioner:
     *     - CoAP Request URI: coap://[Commissioner]:MM/c/pc
     *     - CoAP Payload:
     *       - Channel Mask TLV
     *       - PAN ID TLV
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Commissioner");

    /**
     * Step 6: Commissioner
     *   - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     *   - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_14.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_14();
    printf("All tests passed\n");
    return 0;
}
