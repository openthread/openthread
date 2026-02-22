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
static constexpr uint32_t kFormNetworkTime = 10 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * Time to advance for petition process, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to advance for response timeout, in milliseconds.
 */
static constexpr uint32_t kResponseTimeout = 5 * 1000;

/**
 * Invalid Commissioner Session ID.
 */
static constexpr uint16_t kInvalidSessionId = 0xffff;

/**
 * Border Agent RLOC value.
 */
static constexpr uint16_t kBorderAgentRloc = 0x0400;

static void AppendSteeringDataTlv(Coap::Message &aMessage)
{
    MeshCoP::SteeringData steeringData;

    steeringData.SetToPermitAllJoiners();
    SuccessOrQuit(Tlv::Append<MeshCoP::SteeringDataTlv>(aMessage, steeringData.GetData(), steeringData.GetLength()));
}

void Test9_2_2(void)
{
    /**
     * 9.2.2 On Mesh Commissioner – MGMT_COMMISSIONER_SET.req & rsp
     *
     * 9.2.2.1 Topology
     * - DUT as Leader, Commissioner (Non-DUT)
     *
     * 9.2.2.2 Purpose & Description
     * - DUT as Leader (Topology A): The purpose of this test case is to verify Leader’s behavior when receiving
     *   MGMT_COMMISSIONER_SET.req directly from the active Commissioner.
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

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: All");

    leader.AllowList(commissioner);
    commissioner.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    /**
     * Step 2: Topology A Leader DUT
     * - This step should only be run when the DUT is the Leader. Skip this step if the DUT is the Commissioner.
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or
     *   Routing Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/cs
     *   - CoAP Payload: (missing Commissioner Session ID TLV), Steering Data TLV (0xFF)
     * - Pass Criteria: N/A.
     */
    Log("Step 2: Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        AppendSteeringDataTlv(*message);

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 3: Leader
     * - Please note that step is only valid if step 2 is run.
     * - Description: DUT automatically responds to MGMT_COMMISSIONER_SET.req with a MGMT_COMMISSIONER_SET.rsp to
     *   Commissioner without user or harness intervention.
     * - Pass Criteria: Verify MGMT_COMMISSIONER_SET.rsp frame has the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (0xFF))
     */
    Log("Step 3: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 4: Topology B Commissioner (DUT) / Topology A Leader DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_SET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or
     *     Routing Locator.
     * - Pass Criteria:
     *   - Topology B: Verify MGMT_COMMISSIONER_SET.req frame has the following format:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cs
     *     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF)
     *   - Topology A:
     *     - CoAP Request URI: coap://[<L>]:MM/c/cs
     *     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF)
     *   - Topology A & B: Verify Destination Address of MGMT_COMMISSIONER_SET.req frame is DUT’s Anycast or Routing
     *     Locator (ALOC or RLOC):
     *     - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
     *     - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
     *       Router ID
     */
    Log("Step 4: Topology B Commissioner (DUT) / Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));

        AppendSteeringDataTlv(*message);

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 5: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     */
    Log("Step 5: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 6: Leader
     * - Description: Automatically sends a multicast MLE Data Response.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response with the new information,
     *   including a Network Data TLV including:
     *   - Commissioning Data TLV
     *     - Stable flag set to 0;
     *     - Commissioner Session ID TLV, Border Agent Locator TLV, Steering Data TLV
     */
    Log("Step 6: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 7: Topology A Leader DUT
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or
     *   Routing Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/cs
     *   - CoAP Payload: Commissioner Session ID TLV, Border Agent Locator TLV (0x0400) (not allowed TLV)
     * - Pass Criteria: N/A.
     */
    Log("Step 7: Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        SuccessOrQuit(Tlv::Append<MeshCoP::BorderAgentLocatorTlv>(*message, kBorderAgentRloc));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 8: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (0xFF))
     */
    Log("Step 8: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 9: Topology A Leader DUT
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or
     *   Routing Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/cs
     *   - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF), Border Agent Locator TLV (0x0400)
     *     (not allowed TLV)
     * - Pass Criteria: N/A.
     */
    Log("Step 9: Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));

        AppendSteeringDataTlv(*message);

        SuccessOrQuit(Tlv::Append<MeshCoP::BorderAgentLocatorTlv>(*message, kBorderAgentRloc));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 10: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (0xFF))
     */
    Log("Step 10: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 11: Topology A Leader DUT
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT’s Anycast or Routing
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/cs
     *   - CoAP Payload: Commissioner Session ID TLV (0xFFFF) (invalid value), Steering Data TLV (0xFF)
     * - Pass Criteria: N/A.
     */
    Log("Step 11: Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, kInvalidSessionId));

        AppendSteeringDataTlv(*message);

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 12: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (0xFF))
     */
    Log("Step 12: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 13: Topology A Leader DUT
     * - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT’s Anycast or Routing
     *   Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/cs
     *   - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF), Channel TLV (not allowed TLV)
     * - Pass Criteria: N/A.
     */
    Log("Step 13: Topology A Leader DUT");

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriCommissionerSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));

        AppendSteeringDataTlv(*message);

        SuccessOrQuit(Tlv::Append<MeshCoP::ChannelTlv>(*message, Mle::ChannelTlvValue(11)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    /**
     * Step 14: Leader
     * - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
     *   following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     */
    Log("Step 14: Leader");
    nexus.AdvanceTime(kResponseTimeout);

    /**
     * Step 15: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    Log("Step 15: All");

    nexus.SendAndVerifyEchoRequest(commissioner, leader.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.AdvanceTime(kResponseTimeout);

    nexus.SaveTestInfo("test_9_2_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_2();
    printf("All tests passed\n");
    return 0;
}
