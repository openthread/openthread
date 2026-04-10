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

void Test_1_3_DBR_TC_3(void)
{
    /**
     * 1.3. [1.3] [CERT] Reachability - Multiple BRs - Multiple Thread / Single Infrastructure Link
     *
     * 1.3.2. Topology
     * - BR_1 (DUT) - Thread Border Router and the Leader
     * - BR_2 - Test bed device operating as a Thread Border Router and the Leader on adjacent Thread network
     * - ED_1 - Test bed device operating as a Thread End Device, attached to BR_1
     * - ED_2 - Test bed device operating as a Thread End Device, attached to BR_2
     *
     * 1.3.1. Purpose
     * To test the following:
     * - 1. Bi-directional reachability between multiple Thread Networks attached via an adjacent infrastructure link
     * - 2. No existing IPv6 infrastructure
     * - 3. Single BR per Thread Network
     *
     * Spec Reference   | V1.3.0 Section
     * -----------------|---------------
     * DBR              | 1.3
     */

    Core nexus;

    Node &br1 = nexus.CreateNode();
    Node &br2 = nexus.CreateNode();
    Node &ed1 = nexus.CreateNode();
    Node &ed2 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");
    ed2.SetName("ED_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.3): Enable: power on.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR_1 (DUT) Enable: power on.");

    br1.Form();
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        nexus.AddNetworkKey(datasetInfo.Get<MeshCoP::Dataset::kNetworkKey>());
    }

    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.3): Automatically registers itself as a border router in the Thread Network Data.
     *   Automatically creates a ULA prefix PRE_1 for the adjacent infrastructure link. Automatically
     *   multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - Note: pass criteria are identical to DBR-1.1 step 3.
     *   - The DUT internally registers an OMR Prefix (OMR_1) in the Thread Network Data.
     *   - The DUT MUST send a multicast MLE Data Response with Thread Network Data containing at least two
     *     Prefix TLVs.
     *   - Prefix TLV 1: OMR prefix OMR_1. MUST include a Border Router sub-TLV.
     *   - Flags in the Border Router TLV MUST be: P_preference = 11 (Low), P_default = true, P_stable = true,
     *     P_on_mesh = true, P_preferred = true, P_slaac = true, P_dhcp = false, P_dp = false.
     *   - Prefix TLV 2: ULA prefix fc00::/7. (This is used as the shortened version of PRE_1). Includes Has
     *     Route sub-TLV.
     *   - OMR_1 MUST be 64 bits long and start with 0xFD.
     *   - The DUT MUST multicast ND RAs including the following:
     *   - IPv6 destination MUST be ff02::1.
     *   - M bit and O bit MUST be '0'.
     *   - "Router Lifetime" = 0 (indicating it's not a default router).
     *   - Prefix Information Option (PIO) ULA ULA_1 A bit MUST be '1'.
     *   - Route Information Option (RIO) OMR = OMR_1.
     *   - ULA_1 MUST contain the Extended PAN ID as follows:
     *   - Global ID equals the 40 most significant bits of the Extended PAN ID.
     *   - Subnet ID equals the 16 least significant bits of the Extended PAN ID.
     */
    Log("Step 3: BR_1 (DUT) Automatically registers as border router and multicasts ND RAs.");

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omrPrefix1;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix1));
    nexus.AddTestVar("OMR_PREFIX_1", omrPrefix1.ToString().AsCString());

    Ip6::Prefix pre1Prefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOnLinkPrefix(pre1Prefix));
    nexus.AddTestVar("PRE_1_PREFIX_1", pre1Prefix.ToString().AsCString());

    MeshCoP::Dataset::Info datasetInfo1;
    SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo1));
    nexus.AddTestVar("EXT_PAN_ID_1", datasetInfo1.Get<MeshCoP::Dataset::kExtendedPanId>().ToString().AsCString());

    /**
     * Step 4
     * - Device: BR_2, ED_1, ED_2
     * - Description (DBR-1.3): Form topology. Wait for BR_2 to: 1. Register as border router in Thread
     *   Network Data 2. Send multicast ND RAs on the adjacent infrastructure link with: RIO with OMR
     *   prefix (OMR_2).
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: BR_2, ED_1, ED_2 Form topology.");

    br2.Form();
    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(br2.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));
        nexus.AddNetworkKey(datasetInfo.Get<MeshCoP::Dataset::kNetworkKey>());
    }

    nexus.AdvanceTime(kFormNetworkTime);
    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omrPrefix2;
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix2));
    nexus.AddTestVar("OMR_PREFIX_2", omrPrefix2.ToString().AsCString());

    br1.AllowList(ed1);
    ed1.AllowList(br1);
    ed1.Join(br1, Node::kAsFed);

    br2.AllowList(ed2);
    ed2.AllowList(br2);
    ed2.Join(br2, Node::kAsFed);

    nexus.AdvanceTime(kJoinNetworkTime);
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 5
     * - Device: ED_1
     * - Description (DBR-1.3): Harness instructs device to send ICMPv6 Echo Request to ED_2. IPv6 Source:
     *   ED_1 OMR, IPv6 Destination: ED_2 OMR.
     * - Pass Criteria:
     *   - ED_1 receives an ICMPv6 Echo Reply from ED_2.
     *   - IPv6 Source: ED_2 OMR.
     *   - IPv6 Destination: ED_1 OMR.
     */
    Log("Step 5: ED_1 sends ICMPv6 Echo Request to ED_2.");

    const Ip6::Address &ed1Omr = ed1.FindMatchingAddress(omrPrefix1.ToString().AsCString());
    const Ip6::Address &ed2Omr = ed2.FindMatchingAddress(omrPrefix2.ToString().AsCString());

    nexus.AddTestVar("ED_1_OMR_ADDR", ed1Omr.ToString().AsCString());
    nexus.AddTestVar("ED_2_OMR_ADDR", ed2Omr.ToString().AsCString());

    ed1.SendEchoRequest(ed2Omr, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 6
     * - Device: ED_2
     * - Description (DBR-1.3): Harness instructs device to send ICMPv6 Echo Request to ED_1. IPv6 Source:
     *   ED_2 OMR, IPv6 Destination: ED_1 OMR.
     * - Pass Criteria:
     *   - ED_2 receives an ICMPv6 Echo Reply from ED_1.
     *   - IPv6 Source: ED_1 OMR.
     *   - IPv6 Destination: ED_2 OMR.
     */
    Log("Step 6: ED_2 sends ICMPv6 Echo Request to ED_1.");

    ed2.SendEchoRequest(ed1Omr, kEchoIdentifier, kEchoPayloadSize, 64, &ed2Omr);
    nexus.AdvanceTime(kPingResponseTime);

    nexus.SaveTestInfo("test_1_3_DBR_TC_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_3();
    printf("All tests passed\n");
    return 0;
}
