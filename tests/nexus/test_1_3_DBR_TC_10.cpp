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
static constexpr uint32_t kBrActionTime = 30 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 60 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingResponseTime = 5 * 1000;

/**
 * Time to advance for the Eth_1 configuration, in milliseconds.
 */
static constexpr uint32_t kEth1ConfigTime = 10 * 1000;

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

/**
 * Default Hop Limit for Echo Request.
 */
static constexpr uint8_t kDefaultHopLimit = 64;

/**
 * IPv6 GUA address for Eth_1.
 */
static const char kEth1Gua[] = "2001:db8:1::1";

/**
 * IPv6 GUA prefix GUA_1.
 */
static const char kGua1Prefix[] = "2001:db8:1::/64";

static void AddOnMeshPrefix(Node &aNode, const Ip6::Prefix &aPrefix, bool aDefaultRoute)
{
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    config.mPrefix       = aPrefix;
    config.mStable       = true;
    config.mSlaac        = true;
    config.mPreferred    = true;
    config.mOnMesh       = true;
    config.mDefaultRoute = aDefaultRoute;
    config.mPreference   = NetworkData::kRoutePreferenceMedium;
    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
}

void Test_1_3_DBR_TC_10(const char *aJsonFileName)
{
    /**
     * 1.10. [1.3] [CERT] Reachability - Single BR - Configure OMR address - OMR Routing - Default routes
     *
     * 1.10.1. Purpose
     *   To test the following:
     *   - Thread End Device DUT accepts OMR prefix and configures OMR address
     *   - Thread Router DUT accepts route to off-mesh prefix and routes packets to it
     *   - Reachability of Thread Device DUT from infrastructure devices
     *   - Thread Router DUT can process default route published by a BR and route packets based on it.
     *   - Note: current test is not so elaborate yet for route determination/default-route. TBD: In the future,
     *     another BR (BR_2) may be added and it can be checked the DUT Router picks the appropriate/best route to
     *     either BR_1 or BR_2. For this, a new topology with two different AILs for BR_1 and BR_2 would be most useful.
     *
     * 1.10.2. Topology
     *   - BR_1-Test bed border router device operating as a Thread Border Router and the Leader
     *   - ED_1 - Device operating as a Thread End Device DUT
     *   - Eth_1-Test bed border router device on an Adjacent Infrastructure Link
     *   - Router 1-Device operating as a Thread Router DUT
     *
     * Spec Reference   | V1.3.0 Section
     * -----------------|---------------
     * Reachability     | 1.3
     */

    Core nexus;

    Node &br1     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ed1     = nexus.CreateNode();
    Node &eth1    = nexus.CreateNode();

    br1.SetName("BR_1");
    router1.SetName("Router_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 0
     * - Device: Eth 1
     * - Description (DBR-1.10): Harness enables device and configures Ethernet link with an on-link IPv6 GUA prefix
     *   GUA_1. Eth_1 is configured to multicast ND RA. The ND RA contains a PIO option as follows: Prefix: GUA 1,
     *   L=1 (on-link prefix), A=1 (SLAAC is enabled). Device has a global address "Eth_1 GUA" automatically
     *   configured from prefix GUA_1.
     * - Pass Criteria: N/A
     */
    Log("Step 0: Eth_1 configures Ethernet link with GUA_1 prefix and multicasts ND RA.");

    eth1.mInfraIf.Init(eth1);

    {
        Ip6::Address eth1Gua;
        SuccessOrQuit(eth1Gua.FromString(kEth1Gua));
        eth1.mInfraIf.AddAddress(eth1Gua);
    }

    {
        Ip6::Prefix gua1;
        SuccessOrQuit(gua1.FromString(kGua1Prefix));
        eth1.mInfraIf.StartRouterAdvertisement(gua1);
    }

    nexus.AdvanceTime(kEth1ConfigTime);

    /**
     * Step 1
     * - Device: BR_1
     * - Description (DBR-1.10): Enable. Automatically, it configures an OMR prefix for the Thread Network and also
     *   automatically advertises the external (default) route to GUA 1 using the ::/0 prefix in the Thread Network
     *   and using a Has Route TLV.
     * - Pass Criteria: N/A
     */
    Log("Step 1: BR_1 enables and configures OMR prefix and external route to GUA_1.");

    br1.mInfraIf.Init(br1);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);

    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omr1;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr1));

    /**
     * Step 2
     * - Device: Router 1, ED 1 (DUT)
     * - Description (DBR-1.10): Enable. Automatically, topology is formed.
     * - Pass Criteria: N/A
     */
    Log("Step 2: Router_1 and ED_1 enable and form topology.");

    br1.AllowList(router1);
    router1.AllowList(br1);

    router1.AllowList(ed1);
    ed1.AllowList(router1);

    router1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    ed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3
     * - Device: ED 1 (DUT)
     * - Description (DBR-1.10): Automatically configures an OMR address based on OMR prefix. Note: the verification
     *   that the new address works is done in step 4.
     * - Pass Criteria: N/A
     */
    Log("Step 3: ED_1 automatically configures OMR address.");

    const Ip6::Address &ed1Omr = ed1.FindMatchingAddress(omr1.ToString().AsCString());
    VerifyOrQuit(!ed1Omr.IsUnspecified());

    /**
     * Step 4
     * - Device: Eth 1
     * - Description (DBR-1.10): Harness instructs device to send ICMPv6 Echo Request to ED 1. IPv6 Source: Eth 1
     *   GUA, IPv6 Destination: ED 1 OMR
     * - Pass Criteria:
     *   - Eth 1 receives an ICMPv6 Echo Reply from ED_1.
     *   - IPv6 Source: ED_1 OMR
     *   - IPv6 Destination: Eth_1 GUA
     */
    Log("Step 4: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.");

    Ip6::Address eth1Gua;
    SuccessOrQuit(eth1Gua.FromString(kEth1Gua));

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 5
     * - Device: Router 1 (DUT)
     * - Description (DBR-1.10): Automatically routes the packets of step 4.
     * - Pass Criteria: N/A (Already verified in step 4.)
     */
    Log("Step 5: Router_1 automatically routes the packets of step 4.");

    /**
     * Step 6
     * - Device: ED_1
     * - Description (DBR-1.10): Only in the topology where Router_1 is DUT: Harness instructs device to send ICMPv6
     *   Echo Request to Eth_1. IPv6 Source: ED 1 OMR, IPv6 Destination: Eth 1 GUA
     * - Pass Criteria:
     *   - Only in the topology where Router_1 is DUT: ED_1 receives an ICMPv6 Echo Reply from Eth_1.
     *   - IPv6 Source: Eth 1 GUA
     *   - IPv6 Destination: ED_1 OMR
     */
    Log("Step 6: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 7
     * - Device: Router_1 (DUT)
     * - Description (DBR-1.10): Automatically routes the packets of step 6.
     * - Pass Criteria: N/A (Already verified in step 6.)
     */
    Log("Step 7: Router_1 automatically routes the packets of step 6.");

    /**
     * Step 8
     * - Device: Eth 1
     * - Description (DBR-1.10): Only in the topology where Router _1 is DUT: Harness instructs device to send ICMPv6
     *   Echo Request to Router 1. IPv6 Source: Eth_1 GUA, IPv6 Destination: Router_1 OMR
     * - Pass Criteria:
     *   - Only in the topology where Router_1 is DUT: Eth_1 receives an ICMPv6 Echo Reply from Router_1.
     *   - IPv6 Source: Router 1 OMR
     *   - IPv6 Destination: Eth 1 GUA
     */
    Log("Step 8: Eth_1 sends ICMPv6 Echo Request to Router_1 OMR.");

    const Ip6::Address &router1Omr = router1.FindMatchingAddress(omr1.ToString().AsCString());
    VerifyOrQuit(!router1Omr.IsUnspecified());

    eth1.mInfraIf.SendEchoRequest(eth1Gua, router1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 9
     * - Device: BR_1
     * - Description (DBR-1.10): Harness configures device to: Stop advertising the external route to prefix GUA_1
     *   by removing the Prefix TLV with ::/0 completely. Keep publishing the flag P_default = true in the Border
     *   Router sub-TLV. This should normally happen automatically if the device is 1.4-compliant. Wait some time for
     *   the Network Data change to propagate throughout the network. Note: this emulates a legacy BR that doesn't
     *   advertise the default route using an external ::/0 route, but only uses the P_default flag (R flag) in the
     *   Border Router TLV for this.
     * - Pass Criteria:
     *   - BR 1 reference device MUST publish its OMR prefix with Border Router TLV with P_default = true (R flag =
     *     true).
     *   - Note: although this is not a validation on the DUT, it could be useful as a sanity check. Pre-1.4 BRs may
     *     behave differently; and it's important to use the correct value here.
     */
    Log("Step 9: BR_1 stops advertising ::/0 external route but keeps P_default=true in OMR prefix.");

    // To achieve this, we disable RoutingManager and manually manage Network Data.
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(false));

    // Wait for BR_1 to unpublish OMR prefix and ::/0.
    nexus.AdvanceTime(kBrActionTime);

    AddOnMeshPrefix(br1, omr1, true);
    br1.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 10
     * - Device: Eth 1
     * - Description (DBR-1.10): Harness instructs device to send ICMPv6 Echo Request to ED_1. IPv6 Source: Eth 1
     *   GUA, IPv6 Destination: ED 1 OMR
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *   - IPv6 Source: ED_1 OMR
     *   - IPv6 Destination: Eth 1 GUA
     */
    Log("Step 10: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.");

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 11
     * - Device: ED_1
     * - Description (DBR-1.10): Only in the topology where Router_1 is DUT: Harness instructs device to send ICMPv6
     *   Echo Request to Eth 1. IPv6 Source: ED_1 OMR, IPv6 Destination: Eth 1 GUA
     * - Pass Criteria:
     *   - Only in the topology where Router_1 is DUT: ED_1 receives an ICMPv6 Echo Reply from Eth_1.
     *   - IPv6 Source: Eth_1 GUA
     *   - IPv6 Destination: ED 1 OMR
     */
    Log("Step 11: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 12
     * - Device: BR 1
     * - Description (DBR-1.10): Harness configures device to publish for its OMR prefix: flag P_default = false in
     *   the Border Router sub-TLV; and By setting a Prefix TLV with the zero-length prefix ::/0, which includes Has
     *   Route TLV preference value (Prf) 11 (low). Wait some time for the Network Data change to propagate
     *   throughout the network.
     * - Pass Criteria:
     *   - BR_1 reference device MUST publish its OMR prefix with Border Router TLV with P_default = false (R flag =
     *     false).
     *   - Note: although this is not a validation on the DUT, it could be useful as a sanity check. Pre-1.4 BRs may
     *     behave differently; and it's important to use the correct value here.
     */
    Log("Step 12: BR_1 publishes OMR prefix with P_default=false and ::/0 external route with low preference.");

    {
        AddOnMeshPrefix(br1, omr1, false);

        NetworkData::ExternalRouteConfig routeConfig;
        routeConfig.Clear();
        routeConfig.mPrefix     = Ip6::Prefix(); // ::/0
        routeConfig.mStable     = true;
        routeConfig.mPreference = NetworkData::kRoutePreferenceLow; // Prf = 11
        SuccessOrQuit(br1.Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig));

        br1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 13
     * - Device: Eth 1
     * - Description (DBR-1.10): Repeat Step 10.
     * - Pass Criteria:
     *   - Repeat Step 10.
     */
    Log("Step 13: Eth_1 sends ICMPv6 Echo Request to ED_1 OMR.");

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    /**
     * Step 14
     * - Device: ED 1
     * - Description (DBR-1.10): Repeat Step 11.
     * - Pass Criteria:
     *   - Repeat Step 11.
     */
    Log("Step 14: ED_1 sends ICMPv6 Echo Request to Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    {
        nexus.AddTestVar("BR1", br1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
        nexus.AddTestVar("ROUTER1", router1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
        nexus.AddTestVar("ED1", ed1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
        nexus.AddTestVar("ETH1_GUA", kEth1Gua);
        nexus.AddTestVar("ED1_OMR", ed1Omr.ToString().AsCString());
        nexus.AddTestVar("ROUTER1_OMR", router1Omr.ToString().AsCString());
        nexus.AddTestVar("OMR_PREFIX", omr1.ToString().AsCString());
    }

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_DBR_TC_10((argc > 2) ? argv[2] : "test_1_3_DBR_TC_10.json");
    printf("All tests passed\n");
    return 0;
}
