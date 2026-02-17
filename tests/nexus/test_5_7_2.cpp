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
 * Time to wait for DIAG_GET to complete (20 seconds).
 */
static constexpr uint32_t kWait20Seconds = 20 * 1000;

/**
 * Time to wait before next step (2 seconds).
 */
static constexpr uint32_t kWait2Seconds = 2 * 1000;

/**
 * Number of routers in the topology.
 */
static constexpr uint8_t kNumRouters = 15;

/**
 * Base diagnostic TLV types used in Steps 2 and 8.
 */
static constexpr uint8_t kBaseDiagGetTlvs[] = {
    NetworkDiagnostic::Tlv::kExtMacAddress, NetworkDiagnostic::Tlv::kAddress16,
    NetworkDiagnostic::Tlv::kMode,          NetworkDiagnostic::Tlv::kConnectivity,
    NetworkDiagnostic::Tlv::kRoute,         NetworkDiagnostic::Tlv::kLeaderData,
    NetworkDiagnostic::Tlv::kNetworkData,   NetworkDiagnostic::Tlv::kIp6AddressList,
    NetworkDiagnostic::Tlv::kChannelPages,
};

/**
 * MAC Counters diagnostic TLV type used in Steps 3, 6, and 7.
 */
static constexpr uint8_t kMacCountersTlv[] = {NetworkDiagnostic::Tlv::kMacCounters};

void Test5_7_2(void)
{
    /**
     * 5.7.2 CoAP Diagnostic Get Query and Answer Commands – REED
     *
     * 5.7.2.1 Topology
     * - Leader
     * - Router_1
     * - REED_1 (DUT)
     * - (Additional routers as needed to satisfy REED conditions, typically a total of 16 active routers)
     *
     * 5.7.2.2 Purpose & Description
     * This test case exercises the Diagnostic Get Query and Answer commands as part of the Network Management. This
     *   test case topology is specific to REED DUTs.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Diag Commands    | 10.11.2      | 10.11.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &reed1  = nexus.CreateNode();
    Node *routers[kNumRouters];

    leader.SetName("LEADER");
    reed1.SetName("REED_1");

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
        routers[i]->SetName("ROUTER", i + 1);
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    /** Use AllowList to specify links between nodes. */
    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        leader.AllowList(*routers[i]);
        routers[i]->AllowList(leader);
    }

    reed1.AllowList(*routers[0]);
    routers[0]->AllowList(reed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        routers[i]->Join(leader);
    }

    nexus.AdvanceTime(kAttachToRouterTime);

    for (uint8_t i = 0; i < kNumRouters; i++)
    {
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouter());
    }

    reed1.Join(*routers[0], Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());

    const Ip6::Address &reedRloc = reed1.Get<Mle::Mle>().GetMeshLocalRloc();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV types:
     *   - TLV Type 0 – MAC Extended Address (64-bit)
     *   - TLV Type 1 - MAC Address (16-bit)
     *   - TLV Type 2 - Mode (Capability information)
     *   - TLV Type 4 – Connectivity
     *   - TLV Type 5 – Route64
     *   - TLV Type 6 – Leader Data
     *   - TLV Type 7 – Network Data
     *   - TLV Type 8 – IPv6 address list
     *   - TLV Type 17 – Channel Pages
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLVs:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 4 – Connectivity
     *     - TLV Type 5 – Route64 (optional)
     *     - TLV Type 6 – Leader Data
     *     - TLV Type 7 – Network Data
     *     - TLV Type 8 – IPv6 address list
     *     - TLV Type 17 – Channel Pages
     *   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(
        reedRloc, kBaseDiagGetTlvs, sizeof(kBaseDiagGetTlvs), nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 9 - MAC Counters
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLV:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - TLV Type 9 - MAC Counters
     *   - TLV Type 9 - MAC Counters MUST contain a list of MAC Counters.
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(reedRloc, kMacCountersTlv,
                                                                            sizeof(kMacCountersTlv), nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV types:
     *   - TLV Type 3 – Timeout
     *   - TLV Type 16 – Child Table TLV
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response containing the required diagnostic TLV payload:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - The Timeout TLV MUST NOT be present.
     */

    uint8_t tlvTypes4[] = {NetworkDiagnostic::Tlv::kTimeout, NetworkDiagnostic::Tlv::kChildTable};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(reedRloc, tlvTypes4, sizeof(tlvTypes4),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV types:
     *   - TLV Type 14 – Battery Level
     *   - TLV Type 15 – Supply Voltage
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response optionally containing the requested diagnostic TLVs:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - TLV Type 14 – Battery Level (optional)
     *     - TLV Type 15 – Supply Voltage (optional)
     */

    uint8_t tlvTypes5[] = {NetworkDiagnostic::Tlv::kBatteryLevel, NetworkDiagnostic::Tlv::kSupplyVoltage};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(reedRloc, tlvTypes5, sizeof(tlvTypes5),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5a: Test Harness");

    /**
     * Step 5a: Test Harness
     * - Description: Harness waits 20 seconds to allow DIAG_GET to complete.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWait20Seconds);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send DIAG_RST.ntf to DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 9 - MAC Counters
     * - Pass Criteria:
     *   - The DUT MUST respond with a CoAP response:
     *   - CoAP Response Code: 2.04 Changed
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticReset(reedRloc, kMacCountersTlv,
                                                                              sizeof(kMacCountersTlv)));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6a: Test Harness");

    /**
     * Step 6a: Test Harness
     * - Description: Harness waits ONLY 2 seconds before executing next step.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWait2Seconds);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader");

    /**
     * Step 7: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 9 - MAC Counters
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLV:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - TLV Type 9 - MAC Counters
     *   - TLV Type 9 - MAC Counters MUST contain a list of MAC Counters with 0 value or less than value returned in
     *     step 3.
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(reedRloc, kMacCountersTlv,
                                                                            sizeof(kMacCountersTlv), nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader");

    /**
     * Step 8: Leader
     * - Description: Harness instructs the device to send DIAG_GET.query to Realm-Local All-Nodes multicast address
     *   (FF03::1) for the following diagnostic TLV types:
     *   - TLV Type 0 – MAC Extended Address (64-bit)
     *   - TLV Type 1 - MAC Address (16-bit)
     *   - TLV Type 2 - Mode (Capability information)
     *   - TLV Type 4 – Connectivity
     *   - TLV Type 5 – Route64
     *   - TLV Type 6 – Leader Data
     *   - TLV Type 7 – Network Data
     *   - TLV Type 8 – IPv6 address list
     *   - TLV Type 17 – Channel Pages
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.ans response containing the requested diagnostic TLVs:
     *   - CoAP Payload:
     *     - TLV Type 0 - MAC Extended Address (64-bit)
     *     - TLV Type 1 - MAC Address (16-bit)
     *     - TLV Type 2 - Mode (Capability information)
     *     - TLV Type 4 – Connectivity
     *     - TLV Type 5 – Route64 (optional)
     *     - TLV Type 6 – Leader Data
     *     - TLV Type 7 – Network Data
     *     - TLV Type 8 – IPv6 address list
     *     - TLV Type 17 – Channel Pages
     *   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
     */

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(
        Ip6::Address::GetRealmLocalAllNodesMulticast(), kBaseDiagGetTlvs, sizeof(kBaseDiagGetTlvs), nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    nexus.SaveTestInfo("test_5_7_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_7_2();
    printf("All tests passed\n");
    return 0;
}
