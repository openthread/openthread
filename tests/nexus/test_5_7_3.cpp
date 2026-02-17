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
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/network_diagnostic.hpp"

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
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for the diagnostic response to be received.
 */
static constexpr uint32_t kDiagResponseTime = 5 * 1000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

/**
 * Diagnostic TLV types used in the query.
 */
static constexpr uint8_t kDiagGetTlvs[] = {
    NetworkDiagnostic::Tlv::kExtMacAddress, NetworkDiagnostic::Tlv::kAddress16,
    NetworkDiagnostic::Tlv::kMode,          NetworkDiagnostic::Tlv::kConnectivity,
    NetworkDiagnostic::Tlv::kRoute,         NetworkDiagnostic::Tlv::kLeaderData,
    NetworkDiagnostic::Tlv::kNetworkData,   NetworkDiagnostic::Tlv::kIp6AddressList,
    NetworkDiagnostic::Tlv::kChildTable,    NetworkDiagnostic::Tlv::kChannelPages,
};

struct DiagGetContext
{
    uint8_t mResponseCount;
};

static void HandleDiagnosticGetAnswer(otError              aError,
                                      otMessage           *aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      void                *aContext)
{
    DiagGetContext *context = static_cast<DiagGetContext *>(aContext);
    Coap::Message  *message = AsCoapMessagePtr(aMessage);
    Mac::ExtAddress extAddress;
    uint16_t        shortAddress;
    uint8_t         mode;

    OT_UNUSED_VARIABLE(extAddress);
    OT_UNUSED_VARIABLE(shortAddress);
    OT_UNUSED_VARIABLE(mode);

    VerifyOrQuit(aError == OT_ERROR_NONE);
    VerifyOrQuit(message != nullptr);
    VerifyOrQuit(aMessageInfo != nullptr);

    context->mResponseCount++;

    Log("Diagnostic Answer from %s", AsCoreType(&aMessageInfo->mPeerAddr).ToString().AsCString());

    SuccessOrQuit(Tlv::Find<NetworkDiagnostic::ExtMacAddressTlv>(*message, extAddress));
    SuccessOrQuit(Tlv::Find<NetworkDiagnostic::Address16Tlv>(*message, shortAddress));
    SuccessOrQuit(Tlv::Find<NetworkDiagnostic::ModeTlv>(*message, mode));
}

void Test5_7_3(void)
{
    /**
     * 5.7.3 CoAP Diagnostic Query and Answer Commands - Router, FED
     *
     * 5.7.3.1 Topology
     * - Topology A
     * - Topology B
     *
     * 5.7.3.2 Purpose & Description
     * The purpose of this test case is to verify functionality of commands Diagnostic_Get.query and Diagnostic_Get.ans.
     *   Thread Diagnostic commands MUST be supported by FTDs.
     *
     * Spec Reference                               | V1.1 Section          | V1.3.0 Section
     * ---------------------------------------------|-----------------------|-----------------------
     * Get Diagnostic Query / Get Diagnostic Answer | 10.11.2.3 / 10.11.2.4 | 10.11.2.3 / 10.11.2.4
     */

    Core           nexus;
    DiagGetContext context = {0};

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &fed1    = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    fed1.SetName("FED_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    /** Use AllowList to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(fed1);
    fed1.AllowList(router1);

    router1.AllowList(med1);
    med1.AllowList(router1);

    router1.AllowList(sed1);
    sed1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    fed1.Join(router1, Node::kAsFed);
    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    nexus.AdvanceTime(kAttachToRouterTime);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(fed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the device to send DIAG_GET.query to the Realm-Local All-Thread-Nodes multicast
     *   address for the following diagnostic TLV types:
     *   - Topology A (Router DUT):
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 4 - Connectivity
     *     - TLV Type 5 - Route64
     *     - TLV Type 6 - Leader Data
     *     - TLV Type 7 - Network Data
     *     - TLV Type 8 - IPv6 address list
     *     - TLV Type 16 - Child Table
     *     - TLV Type 17 - Channel Pages
     *   - Topology B (FED DUT):
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 6 - Leader Data
     *     - TLV Type 7 - Network Data
     *     - TLV Type 8 - IPv6 address list
     *     - TLV Type 17 - Channel Pages
     * - Pass Criteria: N/A.
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(
        leader.Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress(), kDiagGetTlvs, sizeof(kDiagGetTlvs),
        HandleDiagnosticGetAnswer, &context));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Topology A (Router DUT)");

    /**
     * Step 3: Topology A (Router DUT)
     * - Description: Automatically responds with a DIAG_GET.ans response.
     * - Pass Criteria:
     *   - The DIAG_GET.ans response MUST contain the requested diagnostic TLVs:
     *   - CoAP Payload:
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 4 - Connectivity
     *     - TLV Type 5 - Route64
     *     - TLV Type 6 - Leader Data
     *     - TLV Type 7 - Network Data
     *     - TLV Type 8 - IPv6 address list
     *     - TLV Type 16 - Child Table
     *     - TLV Type 17 - Channel Pages
     *   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
     */

    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Topology A (Router DUT)");

    /**
     * Step 4: Topology A (Router DUT)
     * - Description: The DUT automatically multicasts the DIAG_GET.query frame.
     * - Pass Criteria:
     *   - The DUT MUST use IEEE 802.15.4 indirect transmissions to forward the DIAG_GET.query to SED_1.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Topology B (FED DUT)");

    /**
     * Step 5: Topology B (FED DUT)
     * - Description: The DUT automatically responds with DIAG_GET.ans.
     * - Pass Criteria:
     *   - The DIAG_GET.ans response MUST contain the requested diagnostic TLVs:
     *   - CoAP Payload:
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 6 - Leader Data
     *     - TLV Type 7 - Network Data
     *     - TLV Type 8 - IPv6 address list
     *     - TLV Type 17 - Channel Pages
     *   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
     */

    // Verify that we received at least 4 responses (Leader, Router, FED, MED, SED)
    // Note: Leader doesn't respond to its own multicast query, so 4 responses.
    VerifyOrQuit(context.mResponseCount >= 4);

    nexus.SaveTestInfo("test_5_7_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_7_3();
    printf("All tests passed\n");
    return 0;
}
