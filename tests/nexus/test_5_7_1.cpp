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

void Test5_7_1(void)
{
    /**
     * 5.7.1 CoAP Diagnostic Get Request, Response and Reset Commands
     *
     * 5.7.1.1 Topology
     * - Topology A
     * - Topology B
     *
     * 5.7.1.2 Purpose & Description
     * These cases test the Diagnostic Get and Reset Commands as a part of the Network Management.
     *
     * Spec Reference      | V1.1 Section | V1.3.0 Section
     * --------------------|--------------|---------------
     * Diagnostic Commands | 10.11.2      | 10.11.2
     */

    Core nexus;

    Node &dut    = nexus.CreateNode();
    Node &leader = nexus.CreateNode();
    Node &fed1   = nexus.CreateNode();
    Node &med1   = nexus.CreateNode();
    Node &sed1   = nexus.CreateNode();
    Node &reed1  = nexus.CreateNode();

    dut.SetName("DUT");
    leader.SetName("LEADER");
    fed1.SetName("FED_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");
    reed1.SetName("REED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    /** Use AllowList to specify links between nodes. */

    nexus.AllowLinkBetween(dut, leader);
    nexus.AllowLinkBetween(dut, fed1);
    nexus.AllowLinkBetween(dut, med1);
    nexus.AllowLinkBetween(dut, sed1);
    nexus.AllowLinkBetween(dut, reed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    fed1.Join(dut, Node::kAsFed);
    med1.Join(dut, Node::kAsMed);
    sed1.Join(dut, Node::kAsSed);
    reed1.Join(dut, Node::kAsFtd);

    nexus.AdvanceTime(kAttachToRouterTime);
    nexus.AdvanceTime(kStabilizationTime);

    const Ip6::Address &dutRloc = dut.Get<Mle::Mle>().GetMeshLocalRloc();

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
     *     - TLV Type 5 – Route64 (required ONLY for Topology A)
     *     - TLV Type 6 – Leader Data
     *     - TLV Type 7 – Network Data
     *     - TLV Type 8 – IPv6 address list
     *     - TLV Type 17 – Channel Pages
     *   - The presence of each TLV MUST be validated. Where possible, the value of the TLV’s MUST be validated.
     *   - Route64 TLV MUST be omitted in Topology B.
     */
    Log("Step 2: Leader");

    uint8_t tlvTypes2[] = {
        NetworkDiagnostic::Tlv::kExtMacAddress, NetworkDiagnostic::Tlv::kAddress16,
        NetworkDiagnostic::Tlv::kMode,          NetworkDiagnostic::Tlv::kConnectivity,
        NetworkDiagnostic::Tlv::kRoute,         NetworkDiagnostic::Tlv::kLeaderData,
        NetworkDiagnostic::Tlv::kNetworkData,   NetworkDiagnostic::Tlv::kIp6AddressList,
        NetworkDiagnostic::Tlv::kChannelPages,
    };

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes2, sizeof(tlvTypes2),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

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
    Log("Step 3: Leader");

    uint8_t tlvTypes3[] = {NetworkDiagnostic::Tlv::kMacCounters};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes3, sizeof(tlvTypes3),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 3 – Timeout
     * - Pass Criteria:
     *   - The DUT MUST respond with a DIAG_GET.rsp response containing the required diagnostic TLV payload:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - TLV Value 3 - Timeout MUST be omitted from the response.
     */
    Log("Step 4: Leader");

    uint8_t tlvTypes4[] = {NetworkDiagnostic::Tlv::kTimeout};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes4, sizeof(tlvTypes4),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

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
    Log("Step 5: Leader");

    uint8_t tlvTypes5[] = {NetworkDiagnostic::Tlv::kBatteryLevel, NetworkDiagnostic::Tlv::kSupplyVoltage};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes5, sizeof(tlvTypes5),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 16 – Child Table
     * - Pass Criteria:
     *   - For Topology A:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: TLV Type 16 – Child Table. The content of the TLV MUST be correct.
     *   - For Topology B:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: Empty
     */
    Log("Step 6: Leader");

    uint8_t tlvTypes6[] = {NetworkDiagnostic::Tlv::kChildTable};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes6, sizeof(tlvTypes6),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    /**
     * Step 7: Leader
     * - Description: Harness instructs the device to send DIAG_RST.ntf to DUT’s Routing Locator (RLOC) for the
     *   following diagnostic TLV type:
     *   - TLV Type 9 - MAC Counters
     * - Pass Criteria:
     *   - The DUT MUST respond with a CoAP response:
     *   - CoAP Response Code: 2.04 Changed
     */
    Log("Step 7: Leader");

    uint8_t tlvTypes7[] = {NetworkDiagnostic::Tlv::kMacCounters};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticReset(dutRloc, tlvTypes7, sizeof(tlvTypes7)));
    nexus.AdvanceTime(kDiagResponseTime);

    /**
     * Step 8: Leader
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
    Log("Step 8: Leader");

    uint8_t tlvTypes8[] = {NetworkDiagnostic::Tlv::kMacCounters};

    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypes8, sizeof(tlvTypes8),
                                                                            nullptr, nullptr));
    nexus.AdvanceTime(kDiagResponseTime);

    nexus.SaveTestInfo("test_5_7_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_7_1();
    printf("All tests passed\n");
    return 0;
}
