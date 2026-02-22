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

/**
 * Active Timestamp value for Step 2.
 */
static constexpr uint64_t kActiveTimestampStep2 = 101;

/**
 * Active Timestamp value for Step 6.
 */
static constexpr uint64_t kActiveTimestampStep6 = 102;

/**
 * Active Timestamp value for Step 8.
 */
static constexpr uint64_t kActiveTimestampStep8 = 103;

/**
 * Active Timestamp value for Step 10.
 */
static constexpr uint64_t kActiveTimestampStep10 = 104;

/**
 * Active Timestamp value for Step 12.
 */
static constexpr uint64_t kActiveTimestampStep12 = 105;

/**
 * Active Timestamp value for Step 14.
 */
static constexpr uint64_t kActiveTimestampStep14 = 106;

/**
 * Active Timestamp value for Step 18.
 */
static constexpr uint64_t kActiveTimestampStep18 = 107;

/**
 * Active Timestamp value for Step 20.
 */
static constexpr uint64_t kActiveTimestampStep20 = 108;

/**
 * Channel Mask value for most steps.
 */
static const uint8_t kChannelMask[] = {0x00, 0x04, 0x00, 0x1f, 0xff, 0xe0};

/**
 * Channel Mask value for Step 8.
 */
static const uint8_t kChannelMaskStep8[] = {0x00, 0x04, 0x00, 0x1f, 0xfe, 0xe0};

/**
 * Extended PAN ID value for most steps.
 */
static const uint8_t kExtPanId[] = {0x00, 0x0d, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00};

/**
 * Extended PAN ID value for Step 6.
 */
static const uint8_t kExtPanIdStep6[] = {0x00, 0x0d, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x01};

/**
 * Network Name "GRL".
 */
static const char kNetworkNameGrl[] = "GRL";

/**
 * Network Name "threadcert".
 */
static const char kNetworkNameThreadCert[] = "threadcert";

/**
 * Network Name "UL".
 */
static const char kNetworkNameUl[] = "UL";

/**
 * PSKc value.
 */
static const uint8_t kPskc[] = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x00};

/**
 * PSKc value for Step 8.
 */
static const uint8_t kPskcStep8[] = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                     0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x01};

/**
 * Security Policy value for most steps.
 */
static const uint8_t kSecurityPolicy[] = {0x0e, 0x10, 0xef};

/**
 * Security Policy value for Step 10.
 */
static const uint8_t kSecurityPolicyStep10[] = {0x0e, 0x10, 0xff};

/**
 * Secondary Channel value.
 */
static constexpr uint16_t kSecondaryChannel = 12;

/**
 * Mesh-Local Prefix for Step 8.
 */
static const uint8_t kMeshLocalPrefixStep8[] = {0xfd, 0x00, 0x0d, 0xb7, 0x00, 0x00, 0x00, 0x00};

/**
 * Different Master Key for Step 10.
 */
static const uint8_t kMasterKeyStep10[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

/**
 * PAN ID for Step 12.
 */
static constexpr uint16_t kPanIdStep12 = 0xafce;

/**
 * Invalid Session ID for Step 14.
 */
static constexpr uint16_t kInvalidSessionId = 0xffff;

/**
 * Steering Data for Step 18.
 */
static const uint8_t kSteeringDataStep18[] = {0x11, 0x33, 0x20, 0x44, 0x00, 0x00};

/**
 * Future TLV Type.
 */
static constexpr uint8_t kFutureTlvType = 130;

/**
 * Future TLV Value.
 */
static const uint8_t kFutureTlvValue[] = {0xaa, 0x55};

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest9_2_4(Topology aTopology, const char *aJsonFile)
{
    /**
     * 9.2.4 Updating the Active Operational Dataset via Commissioner
     *
     * 9.2.4.1 Topology
     * - Topology A: DUT as Leader, Commissioner (Non-DUT)
     * - Topology B: Leader (Non-DUT), DUT as Commissioner
     *
     * 9.2.4.2 Purpose & Description
     * - DUT as Leader (Topology A): The purpose of this test case is to verify the Leader’s behavior when receiving
     *   MGMT_ACTIVE_SET.req directly from the active Commissioner.
     * - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the active Commissioner can
     *   set Active Operational Dataset parameters using the MGMT_ACTIVE_SET.req command.
     *
     * Spec Reference                          | V1.1 Section | V1.3.0 Section
     * ----------------------------------------|--------------|---------------
     * Updating the Active Operational Dataset | 8.7.4        | 8.7.4
     */

    Core nexus;

    Node &leader       = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();

    leader.SetName("LEADER");
    commissioner.SetName("COMMISSIONER");

    Node &dut = (aTopology == kTopologyA) ? leader : commissioner;

    OT_UNUSED_VARIABLE(dut);

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

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 2: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV and new values for Active Operational Dataset TLVs.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator with the following
     *   format:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV (valid)
     *     - Active Timestamp TLV: 101, 0
     *     - Channel Mask TLV: 00:04:00:1f:ff:e0
     *     - Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00
     *     - Network Name TLV: "GRL"
     *     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00
     *     - Security Policy TLV: 0e:10:ef
     *   - Note: The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep2);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(
            Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicy, sizeof(kSecurityPolicy)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 4: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_GET.req to Leader.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_GET.req to Leader.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_GET.req to Leader:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload: <empty> (get all Active Operational Dataset parameters)
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveGet);
        VerifyOrQuit(message != nullptr);

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_GET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: (entire Active Operational Dataset): Active Timestamp TLV, Channel TLV, Channel Mask TLV,
     *     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV,
     *     PSKc TLV, Security Policy TLV.
     *   - The Active Operational Dataset values MUST be equivalent to the Active Operational Dataset values set in
     *     step 2.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 6: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Channel
     *     TLV.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 102, 0, Channel TLV: ‘Secondary’
     *     <Attempt to set this>, Channel Mask TLV: 00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:01 (new
     *     value), Network Name TLV: "threadcert" (new value), PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ef.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep6);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::ChannelTlv>(*message, MeshCoP::ChannelTlvValue(0, kSecondaryChannel)));
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanIdStep6, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameThreadCert));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(
            Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicy, sizeof(kSecurityPolicy)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 8: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Network
     *     Mesh-Local Prefix TLV.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Leader Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 103, 0, Channel Mask TLV:
     *     00:04:00:1f:fe:e0 (new value), Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00 (new value), Network Mesh-Local
     *     Prefix TLV: FD00:0DB7::" (Attempt to set this), Network Name TLV: "UL", PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:01 (new value), Security Policy TLV: 0e:10:ef.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep8);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(
            Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMaskStep8, sizeof(kChannelMaskStep8)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        {
            Ip6::NetworkPrefix prefix;
            memcpy(prefix.m8, kMeshLocalPrefixStep8, sizeof(prefix.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::MeshLocalPrefixTlv>(*message, prefix));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameUl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskcStep8, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(
            Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicy, sizeof(kSecurityPolicy)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");

    /**
     * Step 9: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 10: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Network
     *     Master Key TLV and other TLVs.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 104, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Master Key TLV: Set to different key
     *     value from the original, Network Name TLV: "GRL", PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00
     *     (new value), Security Policy TLV: 0e:10:ff.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep10);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        {
            NetworkKey networkKey;
            memcpy(networkKey.m8, kMasterKeyStep10, sizeof(networkKey.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::NetworkKeyTlv>(*message, networkKey));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");

    /**
     * Step 11: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 12: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, and attempt to set PAN ID TLV.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 105, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PAN ID TLV: AFCE,
     *     PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep12);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        SuccessOrQuit(Tlv::Append<MeshCoP::PanIdTlv>(*message, kPanIdStep12));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader");

    /**
     * Step 13: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 14: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: New valid
     *     Active Timestamp TLV, and Invalid Commissioner Session ID.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (invalid), Active Timestamp TLV: 106, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, kInvalidSessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep14);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Leader");

    /**
     * Step 15: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 16: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: old, valid
     *     Active Timestamp TLV.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV (old): 101, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep2);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Leader");

    /**
     * Step 17: Leader
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 18: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, and unexpected Steering Data TLV.
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 107, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff, Steering Data TLV:
     *     11:33:20:44:00:00.
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep18);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));
        SuccessOrQuit(
            Tlv::AppendTlv(*message, MeshCoP::Tlv::kSteeringData, kSteeringDataStep18, sizeof(kSteeringDataStep18)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader");

    /**
     * Step 19: Leader
     * - Description: Automatically responds to MGMT_ACTIVE_SET.req with a MGMT_ACTIVE_SET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Topology B Commissioner DUT / Topology A Commissioner non-DUT");

    /**
     * Step 20: Topology B Commissioner DUT / Topology A Commissioner non-DUT
     * - Description:
     *   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
     *   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
     *   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
     *     Active Timestamp TLV, and unspecified TLV (Future TLV).
     * - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 108, 0, Channel Mask TLV:
     *     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
     *     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff, Future TLV: Type 130, Length
     *     2, Value (aa 55).
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep20);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kChannelMask, kChannelMask, sizeof(kChannelMask)));
        {
            MeshCoP::ExtendedPanId extPanId;
            memcpy(extPanId.m8, kExtPanId, sizeof(extPanId.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, extPanId));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameGrl));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPskc, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }
        SuccessOrQuit(Tlv::AppendTlv(*message, MeshCoP::Tlv::kSecurityPolicy, kSecurityPolicyStep10,
                                     sizeof(kSecurityPolicyStep10)));
        SuccessOrQuit(Tlv::AppendTlv(*message, kFutureTlvType, kFutureTlvValue, sizeof(kFutureTlvValue)));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Leader");

    /**
     * Step 21: Leader
     * - Description: Automatically responds to MGMT_ACTIVE_SET.req with a MGMT_ACTIVE_SET.rsp to Commissioner.
     * - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
     *   format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: All");

    /**
     * Step 22: All
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
        ot::Nexus::RunTest9_2_4(ot::Nexus::kTopologyA, "test_9_2_4_A.json");
        ot::Nexus::RunTest9_2_4(ot::Nexus::kTopologyB, "test_9_2_4_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest9_2_4(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_9_2_4_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest9_2_4(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_9_2_4_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
