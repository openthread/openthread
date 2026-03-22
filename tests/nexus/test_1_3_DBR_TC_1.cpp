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
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 20 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingResponseTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

void Test_1_3_DBR_TC_1(void)
{
    /**
     * 1.1. [1.3] [CERT] Reachability - Single BR / Single Infrastructure Link
     *
     * 1.1.1. Purpose
     * To test the following situation
     * 1. Bi-directional reachability between Thread devices and infrastructure devices
     * 2. No existing IPv6 infrastructure
     * 3. Single BR
     * 4. Verify that the BR DUT does not forward IPv6 packets that must not be forwarded (e.g. link-local)
     * 5. Verify that the BR DUT sends the right ND RA messages on AIL, and includes the right prefixes in Network Data
     *
     * 1.1.2. Topology
     * - BR 1 (DUT) - Thread Border Router and Leader
     * - ED 1 - Test bed device operating as a Thread End Device
     * - Eth 1 - Test bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Reachability     | N/A          | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: Eth 1, ED 1 Description (DBR-1.1): Enable.");

    /**
     * Step 1
     * - Device: Eth 1, ED 1
     * - Description (DBR-1.1): Enable.
     * - Pass Criteria: N/A
     */

    eth1.mInfraIf.Init(eth1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR 1 (DUT) Description (DBR-1.1): Enable.");

    /**
     * Step 2
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.1): Enable.
     * - Pass Criteria: N/A
     */

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR 1 (DUT) Description (DBR-1.1): Automatically registers itself as a Border Router.");

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.1): Automatically registers itself as a Border Router in the Thread Network Data. The DUT:
     *   adds an OMR prefix OMR_1 to its Thread Network Data; adds an on-link ULA prefix PRE_1 on the adjacent
     *   infrastructure link (AIL); adds an external route in the Thread Network Data based on PRE_1.
     * - Pass Criteria:
     *   - Note: pass criteria are identical to DBR-1.3 step 2.
     *   - The DUT internally registers an OMR Prefix OMR_1 in the Thread Network Data.
     *   - The DUT MUST send a multicast MLE Data Response with Thread Network Data containing at least two Prefix TLVs:
     *   - 1. Prefix TLV: OMR prefix OMR_1.
     *   - MUST include a Border Router sub-TLV. Flags in the Border Router TLV MUST be:
     *   - P_preference = 11 (Low)
     *   - P_default=true
     *   - P_stable=true
     *   - P_on_mesh=true
     *   - P_preferred=true
     *   - P_slaac = true
     *   - P_dhcp=false
     *   - P_dp=false
     *   - 2. Prefix TLV: ULA prefix fc00::/7. (This is used as the shortened version of PRE_1)
     *   - Includes Has Route sub-TLV
     *   - OMR 1 MUST be 64 bits long and start with 0xFD.
     */

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omrPrefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
    nexus.AddTestVar("OMR_PREFIX", omrPrefix.ToString().AsCString());

    Ip6::Prefix pre1Prefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(pre1Prefix));
    nexus.AddTestVar("PRE_1_PREFIX", pre1Prefix.ToString().AsCString());

    MeshCoP::Dataset::Info datasetInfo;
    SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
    const MeshCoP::ExtendedPanId &extPanId = datasetInfo.Get<MeshCoP::Dataset::kExtendedPanId>();
    nexus.AddTestVar("EXT_PAN_ID_VAR", extPanId.ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: BR 1 (DUT) Description (DBR-1.1): Automatically multicasts ND RAs on AIL.");

    /**
     * Step 4
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.1): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs.
     *   - IPv6 destination MUST be ff02::1
     *   - M bit and O bit MUST be '0'
     *   - MUST contain "Router Lifetime" = 0. (indicating it's not a default router)
     *   - MUST contain a Prefix Information Option (PIO) with a ULA prefix PRE_1.
     *   - A bit MUST be '1'
     *   - MUST contain a Route Information Option (RIO) with the OMR prefix OMR 1.
     *   - PRE_1 MUST be 64 bits long and start with 0xFD and MUST differ from OMR_1.
     *   - PRE_1 MUST contain the Extended PAN ID as follows:
     *   - Global ID equals the 40 most significant bits of the Extended PAN ID
     *   - Subnet ID equals the 16 least significant bits of the Extended PAN ID
     */

    // Time already advanced in Step 3 is enough for RAs.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: Eth 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED 1.");

    /**
     * Step 5
     * - Device: Eth 1
     * - Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED 1. 1. IPv6 Source: Eth_1
     * ULA 2. IPv6 Destination: ED_1 OMR address
     * - Pass Criteria:
     *   - Eth 1 receives an ICMPv6 Echo Reply from ED_1.
     *   - 1. IPv6 Source: ED_1 OMR address
     *   - 2. IPv6 Destination: Eth_1 ULA
     */

    const Ip6::Address &eth1Ula = eth1.mInfraIf.FindMatchingAddress(pre1Prefix.ToString().AsCString());
    const Ip6::Address &ed1Omr  = ed1.FindMatchingAddress(omrPrefix.ToString().AsCString());

    nexus.AddTestVar("ETH_1_ULA_ADDR", eth1Ula.ToString().AsCString());
    nexus.AddTestVar("ED_1_OMR_ADDR", ed1Omr.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Ula, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: ED 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.");

    /**
     * Step 6
     * - Device: ED 1
     * - Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1. 1. IPv6 Source: ED_1 OMR
     *   address 2. IPv6 Destination: Eth_1 ULA
     * - Pass Criteria:
     *   - ED 1 receives an ICMPv6 Echo Reply from Eth 1.
     *   - 1. IPv6 Source: Eth_1 ULA
     *   - 2. IPv6 Destination: ED_1 OMR address
     */

    ed1.SendEchoRequest(eth1Ula, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: Eth 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED 1.");

    /**
     * Step 7
     * - Device: Eth 1
     * - Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED_1. 1. IPv6 Source: Eth_1
     *   link-local 2. IPv6 Destination: ED 1 OMR address
     * - Pass Criteria:
     *   - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Thread network.
     */

    const Ip6::Address &eth1Ll = eth1.mInfraIf.FindMatchingAddress("fe80::/10");
    nexus.AddTestVar("ETH_1_LL_ADDR", eth1Ll.ToString().AsCString());
    eth1.mInfraIf.SendEchoRequest(eth1Ll, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: ED 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.");

    /**
     * Step 8
     * - Device: ED 1
     * - Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1. 1. IPv6 Source: ED 1
     *   link-local 2. IPv6 Destination: Eth_1 ULA
     * - Pass Criteria:
     *   - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Adjacent Infrastructure Network.
     */

    const Ip6::Address &ed1Ll = ed1.FindMatchingAddress("fe80::/10");
    nexus.AddTestVar("ED_1_LL_ADDR", ed1Ll.ToString().AsCString());
    ed1.SendEchoRequest(eth1Ula, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Ll);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: Eth 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to ED 1.");

    /**
     * Step 9
     * - Device: Eth 1
     * - Description (DBR-1.1): Harness instructs device to add a route to the Thread Network's mesh-local prefix via
     * the infrastructure link. Harness then instructs device to send ICMPv6 Echo Request to ED 1. 1. IPv6 Source: Eth 1
     *   ULA 2. IPv6 Destination: ED_1-ML-EID
     * - Pass Criteria:
     *   - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Thread network.
     */

    const Ip6::Address &ed1Mleid = ed1.Get<Mle::Mle>().GetMeshLocalEid();
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1Mleid.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Ula, ed1Mleid, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: ED 1 Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1.");

    /**
     * Step 10
     * - Device: ED 1
     * - Description (DBR-1.1): Harness instructs device to send ICMPv6 Echo Request to Eth 1. 1. IPv6 Source: ED 1
     * ML-EID
     *   2. IPv6 Destination: Eth 1 ULA
     * - Pass Criteria:
     *   - BR_1 (DUT) MUST NOT forward the ICMPv6 Echo Request to the Adjacent Infrastructure Network.
     */

    ed1.SendEchoRequest(eth1Ula, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Mleid);
    nexus.AdvanceTime(kPingResponseTime);

    nexus.SaveTestInfo("test_1_3_DBR_TC_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_1();
    printf("All tests passed\n");
    return 0;
}
