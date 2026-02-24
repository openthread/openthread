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

#include <cstdio>
#include <cstring>

#include "meshcop/commissioner.hpp"
#include "meshcop/meshcop_tlvs.hpp"
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
 * Time to advance for a node to upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kRouterUpgradeTime = 200 * 1000;

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
 * Primary Channel.
 */
static constexpr uint8_t kPrimaryChannel = 11;

/**
 * Secondary Channel.
 */
static constexpr uint8_t kSecondaryChannel = 12;

/**
 * Scan Count.
 */
static constexpr uint8_t kScanCount = 2;

/**
 * Scan Period.
 */
static constexpr uint16_t kScanPeriod = 200;

/**
 * Scan Duration.
 */
static constexpr uint16_t kScanDuration = 32;

/**
 * SED Data Poll Rate.
 */
static constexpr uint32_t kSedPollRate = 500;

static void SendMgmtEnergyScanQuery(Node &aCommissioner, const Ip6::Address &aDestAddr, uint16_t aSessionId)
{
    Tmf::Agent    &agent   = aCommissioner.Get<Tmf::Agent>();
    Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriEnergyScan);

    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, aSessionId));
    SuccessOrQuit(MeshCoP::ChannelMaskTlv::AppendTo(*message, (1 << kPrimaryChannel) | (1 << kSecondaryChannel)));
    SuccessOrQuit(Tlv::Append<MeshCoP::CountTlv>(*message, kScanCount));
    SuccessOrQuit(Tlv::Append<MeshCoP::PeriodTlv>(*message, kScanPeriod));
    SuccessOrQuit(Tlv::Append<MeshCoP::ScanDurationTlv>(*message, kScanDuration));

    SuccessOrQuit(agent.SendMessageTo(*message, aDestAddr));
}

void Test9_2_13(void)
{
    /**
     * 9.2.13 Energy Scan Requests
     *
     * 9.2.13.1 Topology
     * - NOTE: Two sniffers are required to run this test case!
     * - Leader_2 & SED_2 formed a separate network on another channel (Secondary channel).
     * - SED_2 is configured with a data poll rate set to 500ms.
     *
     * 9.2.13.2 Purpose & Description
     * The purpose of this test case is to ensure that the DUT is able to properly accept and process Energy Scan
     *   Requests with a MGMT_ED_REPORT.ans.
     *
     * Spec Reference                     | V1.1 Section | V1.3.0 Section
     * -----------------------------------|--------------|---------------
     * Collecting Energy Scan Information | 8.7.10       | 8.7.10
     */

    Core nexus;

    Node &leader1      = nexus.CreateNode();
    Node &commissioner = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &fed1         = nexus.CreateNode();
    Node &leader2      = nexus.CreateNode();
    Node &sed2         = nexus.CreateNode();

    leader1.SetName("Leader_1");
    commissioner.SetName("Commissioner");
    router1.SetName("Router_1");
    fed1.SetName("FED_1");
    leader2.SetName("Leader_2");
    sed2.SetName("SED_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("Step 1: Form topology and start a constant ICMPv6 Echo Request from Leader_2 to SED_2.");

    /**
     * Step 1: All
     * - Description: Form topology and start a constant ICMPv6 Echo Request from Leader_2 to SED_2.
     * - Pass Criteria: N/A.
     */

    /** Set up AllowList for links. */
    leader1.AllowList(commissioner);
    commissioner.AllowList(leader1);

    leader1.AllowList(router1);
    router1.AllowList(leader1);

    router1.AllowList(fed1);
    fed1.AllowList(router1);

    leader2.AllowList(sed2);
    sed2.AllowList(leader2);

    Ip6::Prefix        prefix;
    Ip6::NetworkPrefix networkPrefix;

    SuccessOrQuit(prefix.FromString("fd00:7d1:a11:1::/64"));
    SuccessOrQuit(networkPrefix.SetFrom(prefix));

    /** Network 1 on Primary Channel. */
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(datasetInfo.GenerateRandom(leader1.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kPrimaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kMeshLocalPrefix>(networkPrefix);
        leader1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader1.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader1.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader1.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader1);
    router1.Join(leader1);
    nexus.AdvanceTime(kRouterUpgradeTime);

    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    fed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(fed1.Get<Mle::Mle>().IsAttached());

    /** Network 2 on Secondary Channel. */
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(datasetInfo.GenerateRandom(leader2.GetInstance()));
        datasetInfo.Set<MeshCoP::Dataset::kChannel>(kSecondaryChannel);
        datasetInfo.Set<MeshCoP::Dataset::kMeshLocalPrefix>(networkPrefix);
        leader2.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        leader2.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader2.Get<Mle::Mle>().Start());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader2.Get<Mle::Mle>().IsLeader());

    sed2.Join(leader2, Node::kAsSed);
    SuccessOrQuit(sed2.Get<DataPollSender>().SetExternalPollPeriod(kSedPollRate));
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(sed2.Get<Mle::Mle>().IsAttached());

    /** Start ICMPv6 Echo Request from Leader_2 to SED_2. */
    leader2.SendEchoRequest(sed2.Get<Mle::Mle>().GetMeshLocalEid(), 0);

    /** Start Commissioner. */
    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    Log("Step 2: Commissioner sends a unicast MGMT_ED_SCAN.qry to the DUT.");

    /**
     * Step 2: Commissioner
     * - Description: Harness instructs the Commissioner to send a unicast MGMT_ED_SCAN.qry to the DUT for the Primary
     *   and Secondary channels:
     *   - CoAP Request URI: coap://[DUT]:MM/c/es
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Channel Mask TLV (Primary and Secondary)
     *     - Count TLV <0x02>
     *     - Period TLV <0x00C8>
     *     - Scan Duration TLV <0x20>
     * - Pass Criteria: N/A.
     */

    SendMgmtEnergyScanQuery(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid(), sessionId);

    Log("Step 3: DUT sends a MGMT_ED_REPORT.ans response to the Commissioner.");

    /**
     * Step 3: DUT
     * - Description: Automatically sends a MGMT_ED_REPORT.ans response to the Commissioner.
     * - Pass Criteria: The DUT MUST send MGMT_ED_REPORT.ans to the Commissioner and report energy measurements for the
     *   Primary and Secondary channels:
     *   - CoAP Request URI: coap://[Commissioner]:MM/c/er
     *   - CoAP Payload:
     *     - Channel Mask TLV (Primary and Secondary)
     *     - Energy List TLV (4 bytes)
     */

    nexus.AdvanceTime(kResponseTime);

    Log("Step 4: Commissioner sends multicast MGMT_ED_SCAN.qry.");

    /**
     * Step 4: Commissioner
     * - Description: Harness instructs the Commissioner to send MGMT_ED_SCAN.qry to the Realm Local All Thread Nodes
     *   multicast address: FF33:0040:<mesh local prefix>::1 for the Primary and Secondary channels:
     *   - CoAP Request URI: coap://[Destination]:MM/c/es
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Channel Mask TLV (Primary and Secondary)
     *     - Count TLV <0x02>
     *     - Period TLV <0x00C8>
     *     - Scan Duration TLV <0x20>
     * - Pass Criteria: N/A.
     */

    SendMgmtEnergyScanQuery(commissioner, commissioner.Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress(), sessionId);

    Log("Step 5: DUT sends a MGMT_ED_REPORT.ans response to the Commissioner.");

    /**
     * Step 5: DUT
     * - Description: Automatically sends a MGMT_ED_REPORT.ans response to the Commissioner.
     * - Pass Criteria: The DUT MUST send MGMT_ED_REPORT.ans to the Commissioner and report energy measurements for the
     *   Primary and Secondary channels:
     *   - CoAP Request URI: coap://[Commissioner]:MM/c/er
     *   - CoAP Payload:
     *     - Channel Mask TLV (Primary and Secondary)
     *     - Energy List TLV (length of 4 bytes)
     */

    nexus.AdvanceTime(kResponseTime);

    Log("Step 6: Commissioner sends an ICMPv6 Echo Request to the DUT.");

    /**
     * Step 6: Commissioner
     * - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
     *   the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, router1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_13.json", &leader1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_13();
    printf("All tests passed\n");
    return 0;
}
