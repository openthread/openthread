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
static constexpr uint32_t kFormNetworkTime = 20 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 300 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 100 * 1000;

/**
 * Longer time to advance for network stabilization and address configuration.
 */
static constexpr uint32_t kLongActionTime = 500 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingResponseTime = 100 * 1000;

/**
 * Default Hop Limit for Echo Request.
 */
static constexpr uint8_t kDefaultHopLimit = 64;

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
 * Max number of seconds to wait for a condition.
 */
static constexpr uint32_t kMaxWaitTime = 120;

/**
 * OMR prefix 3 string (numerically very low to be "winning").
 */
static constexpr char kOmr3PrefixStr[] = "fd00:3::/64";

/**
 * OMR prefix 4 string (numerically very high to be "losing" if same preference).
 */
static constexpr char kOmr4PrefixStr[] = "fdff:4::/64";

/**
 * Infrastructure GUA address string for Eth_1.
 */
static constexpr char kEth1GuaAddrStr[] = "2001:db8:1::1";

/**
 * Infrastructure GUA prefix string.
 */
static constexpr char kGua1PrefixStr[] = "2001:db8:1::/64";

static void DumpNetworkData(Node &aNode)
{
    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig  prefixConfig;
    NetworkData::ExternalRouteConfig routeConfig;

    Log("Network Data for %s (Version: %d):", aNode.GetName(),
        aNode.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet));
    while (aNode.Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
    {
        Log("  Prefix: %s (pref:%d, preferred:%s)", prefixConfig.GetPrefix().ToString().AsCString(),
            prefixConfig.mPreference, ToYesNo(prefixConfig.mPreferred));
    }

    iterator = NetworkData::kIteratorInit;
    while (aNode.Get<NetworkData::Leader>().GetNext(iterator, routeConfig) == kErrorNone)
    {
        Log("  Route: %s (pref:%d)", routeConfig.GetPrefix().ToString().AsCString(), routeConfig.mPreference);
    }
}

static bool HasNetDataPrefix(Node &aNode, const Ip6::Prefix &aPrefix)
{
    return aNode.Get<NetworkData::Leader>().ContainsOmrPrefix(aPrefix);
}

static bool HasNetDataRoute(Node &aNode, const char *aPrefixStr)
{
    bool                             hasRoute = false;
    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::ExternalRouteConfig routeConfig;
    Ip6::Prefix                      target;

    SuccessOrQuit(target.FromString(aPrefixStr));

    while (aNode.Get<NetworkData::Leader>().GetNext(iterator, routeConfig) == kErrorNone)
    {
        if (AsCoreType(&routeConfig.mPrefix) == target)
        {
            hasRoute = true;
            break;
        }
    }

    return hasRoute;
}

static void WaitForPrefix(Core &aNexus, Node &aNode, const Ip6::Prefix &aPrefix, bool aPresent)
{
    for (uint32_t i = 0; i < kMaxWaitTime; i++)
    {
        if (HasNetDataPrefix(aNode, aPrefix) == aPresent)
        {
            break;
        }
        aNexus.AdvanceTime(1000);
    }
    VerifyOrQuit(HasNetDataPrefix(aNode, aPrefix) == aPresent);
}

static void WaitForRoute(Core &aNexus, Node &aNode, const char *aPrefixStr, bool aPresent)
{
    for (uint32_t i = 0; i < kMaxWaitTime; i++)
    {
        if (HasNetDataRoute(aNode, aPrefixStr) == aPresent)
        {
            break;
        }
        aNexus.AdvanceTime(1000);
    }
    VerifyOrQuit(HasNetDataRoute(aNode, aPrefixStr) == aPresent);
}

static void PublishOmrPrefix(Node                        &aNode,
                             const Ip6::Prefix           &aPrefix,
                             NetworkData::RoutePreference aPreference,
                             bool                         aIsPreferred)
{
    NetworkData::OnMeshPrefixConfig config;
    config.Clear();
    config.GetPrefix() = aPrefix;
    config.mPreference = aPreference;
    config.mSlaac      = true;
    config.mOnMesh     = true;
    config.mStable     = true;
    config.mPreferred  = aIsPreferred;
    SuccessOrQuit(aNode.Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
}

void Test_1_3_DBR_TC_8(void)
{
    /**
     * 1.8. [1.3] [CERT] Reachability - Multiple BRs - Single Thread / Single IPv6 Infrastructure - OMR prefix selection
     *
     * 1.8.1. Purpose
     * - To test the following:
     *   - Maintain bi-directional reachability between Thread devices and infrastructure devices during prefix changes
     *   - DUT BR should observe proper OMR prefix selection when testbed-BR advertises various other prefixes:
     *     - Testbed advertises a winning prefix with equal preference, DUT should withdraw or deprecate its OMR prefix
     *     - Testbed advertises a numerically higher, non-winning prefix with equal preference, DUT should not change
     *       behavior.
     *     - Testbed advertises a numerically higher prefix with higher preference, i.e. winning OMR prefix, DUT should
     *       withdraw its OMR prefix.
     *     - Testbed withdraws winning OMR prefix, DUT should advertise its prefix with 'Low' preference again
     *     - Testbed OMR prefix becomes deprecated, DUT should advertise its prefix with 'Low' preference again
     *
     * 1.8.2. Topology
     * - Eth_1 - Adjacent Infrastructure Link Reference Device
     * - BR_1 (DUT) - Border Router
     * - BR_2 - Border Router Reference Device and Leader
     * - ED_1 - Thread Reference Device (End Device) attached to BR_1
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Reachability     | N/A          | 1.3
     */

    Core nexus;

    Node &eth1 = nexus.CreateNode();
    Node &br1  = nexus.CreateNode();
    Node &br2  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();

    eth1.SetName("Eth_1");
    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 0
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA_1. Eth_1 is configured to multicast ND
     *     RAs.
     *   - Automatically configures a global address “Eth_1 GUA”.
     * - Pass Criteria
     *   - N/A
     */
    Log("Step 0: Eth_1 configured with GUA_1.");

    eth1.mInfraIf.Init(eth1);
    Ip6::Address eth1Gua;
    SuccessOrQuit(eth1Gua.FromString(kEth1GuaAddrStr));
    eth1.mInfraIf.AddAddress(eth1Gua);

    Ip6::Prefix gua1;
    SuccessOrQuit(gua1.FromString(kGua1PrefixStr));
    eth1.mInfraIf.StartRouterAdvertisement(gua1);

    nexus.AddTestVar("ETH_1_GUA_ADDR", kEth1GuaAddrStr);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1
     * - Device: Eth_1, BR_2, ED_1, ED_2
     * - Description (DBR-1.8):
     *   - Enable; BR_2 automatically configures an OMR prefix for the mesh, OMR_1. Harness records the value of OMR_1
     *     and saves it for verification use.
     *   - Note: the OT command ‘br omrprefix’ should provide the prefix from BR_2.
     *   - Form topology. Wait for BR_2 to:
     *     - Become Leader
     *     - Register as border router in Thread Network Data with its OMR prefix OMR_1
     *     - Send multicast ND RAs on AIL
     *     - Automatically advertise a route to GUA_1 in the Thread Network Data, using a Prefix TLV:
     *       - Prefix = ::/0
     *       - Domain ID = <self-selected value>
     *       - Has Route sub-TLV
     * - Pass Criteria
     *   - N/A
     */
    Log("Step 1: BR_2 becomes Leader and registers OMR_1.");

    br1.AllowList(br2);
    br1.AllowList(ed1);
    br2.AllowList(br1);
    ed1.AllowList(br1);

    br2.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    nexus.AdvanceTime(kBrActionTime);
    WaitForRoute(nexus, br2, "::/0", true);

    Ip6::Prefix omr1;
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr1));
    nexus.AddTestVar("OMR_1_PREFIX", omr1.ToString().AsCString());
    Log("OMR_1_PREFIX: %s", omr1.ToString().AsCString());
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8): Enable: switch on.
     * - Pass Criteria
     *   - N/A
     */
    Log("Step 2: Enable BR_1 (DUT).");

    br1.Join(br2);
    nexus.AdvanceTime(kJoinNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically registers itself as a border router in the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT MUST register a route to GUA_1 in the Thread Network Data as follows:
     *     - Prefix TLV: Prefix = ::/0
     *     - Domain ID = <same value as that of BR_2 in step 1>
     *     - Has Route sub-TLV
     *       - R_preference = 00 (medium) or 11 (low)
     *   - DUT MUST NOT register an OMR prefix (e.g. OMR_1, or other) in Thread Network Data.
     */
    Log("Step 3: BR_1 (DUT) registers route to GUA_1.");

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically announces the route to OMR_1 on the AIL.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST contain a Route Information Option (RIO) with OMR_1.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     */
    Log("Step 4: BR_1 (DUT) announces OMR_1 on AIL.");

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs device to remove the existing OMR prefix (OMR_1) being advertised and send out new network
     *     data without any prefix.
     *   - Note: for 1.3.x cert, this could be implemented by 'br disable' or 'netdata unpublish <OMR_1>' on BR_2.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 5: BR_2 removes OMR_1.");

    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    WaitForPrefix(nexus, br2, omr1, false);

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically chooses a new OMR prefix OMR_2 and includes it in the Network Data.
     * - Pass Criteria:
     *   - The DUT MUST register a new OMR prefix (OMR_2) in the Thread Network Data using Prefix TLV as follows:
     *     - Prefix = OMR_2
     *       - Border Router sub-TLV
     *         - P_preference = 11 (Low)
     *         - P_default = true
     *         - P_stable = true
     *         - P_on_mesh = true
     *         - P_preferred = true
     *         - P_slaac = true
     *         - P_dhcp = false
     *         - P_dp = false
     *   - OMR_2 MUST be 64 bits long and start with 0xFD.
     *   - OMR_2 MUST differ from OMR_1.
     *   - Also MUST contain a Prefix TLV as follows:
     *     - Prefix = ::/0
     *     - Domain ID <same value as BR_2 Domain ID>
     *     - Has Route TLV
     *       - R_preference = 00 (medium) or 11 (low)
     *   - Note: one possible way to check this is to verify the contents of the SVR_DATA.ntf message sent by DUT to
     *     Leader.
     */
    Log("Step 6: BR_1 (DUT) chooses OMR_2 and registers in Network Data.");

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omr2;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr2));
    VerifyOrQuit(omr2 != omr1);
    nexus.AddTestVar("OMR_2_PREFIX", omr2.ToString().AsCString());
    Log("OMR_2_PREFIX: %s", omr2.ToString().AsCString());
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 7
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST contain a Route Information Option (RIO) with OMR_2.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low).
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
     */
    Log("Step 7: BR_1 (DUT) multicasts ND RA with OMR_2, no OMR_1.");

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 8
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via either one of BR_1 or BR_2.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *   - IPv6 Source: ED_1 OMR (using prefix OMR_2)
     *   - IPv6 Destination: Eth_1 GUA
     */
    Log("Step 8: Eth_1 pings ED_1 OMR_2.");

    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);
    const Ip6::Address &ed1Omr2 = ed1.FindMatchingAddress(omr2.ToString().AsCString());
    nexus.AddTestVar("ED_1_OMR_2_ADDR", ed1Omr2.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr2, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 9
     * - Device: ED_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
     *     - IPv6 Source: ED_1 OMR address (using prefix OMR_2)
     *     - IPv6 Destination: Eth_1 GUA
     * - Pass Criteria:
     *   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
     */
    Log("Step 9: ED_1 OMR_2 pings Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr2);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 10
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs BR_2 to add an OMR prefix OMR_3 numerically lower than OMR_2, but with same preference
     *     "Low".
     *   - Note: can use OT command 'netdata publish prefix' for this.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: BR_2 adds OMR_3.");

    Ip6::Prefix omr3;
    SuccessOrQuit(omr3.FromString(kOmr3PrefixStr));

    PublishOmrPrefix(br2, omr3, NetworkData::kRoutePreferenceLow, true);

    nexus.AdvanceTime(kLongActionTime);

    WaitForPrefix(nexus, br2, omr3, true);
    DumpNetworkData(br2);

    nexus.AddTestVar("OMR_3_PREFIX", kOmr3PrefixStr);
    Log("OMR_3_PREFIX: %s", kOmr3PrefixStr);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 11
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically withdraws its own OMR prefix OMR_2 from the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT MUST register new Network Data:
     *   - MUST NOT contain any of the OMR prefixes OMR_1, OMR_2, or OMR_3 in a Prefix TLV.
     *   - MUST contain a Prefix TLV:
     *     - Prefix = ::/0
     *     - Domain ID <same value as BR_2 Domain ID>
     *     - Has Route TLV
     *       - R_preference = 00 (medium) or 11 (low)
     */
    Log("Step 11: BR_1 (DUT) withdraws OMR_2 from Network Data.");

    WaitForPrefix(nexus, br2, omr2, false);
    nexus.AdvanceTime(kLongActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 12
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically multicasts ND RAs on Adjacent Infrastructure Link including OMR_2/OMR_3.
     *   - Note: the reason that OMR_2 is still included initially, is that OMR_2 has been created by BR_1 originally
     *     and as seen by BR_1, it stays in a deprecated state for at most OMR_ADDR_DEPRECATION_TIME
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
     *     - MUST contain a Route Information Option (RIO) with OMR_2.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     *     - MUST contain a Route Information Option (RIO) with OMR_3.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     */
    Log("Step 12: BR_1 (DUT) multicasts ND RA with OMR_2 and OMR_3.");

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br1);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 13
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1 or BR_2.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_3)
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *     - IPv6 Source: ED_1 OMR (using prefix OMR_3)
     *     - IPv6 Destination: Eth_1 GUA
     */
    Log("Step 13: Eth_1 pings ED_1 OMR_3.");

    const Ip6::Address &ed1Omr3 = ed1.FindMatchingAddress(kOmr3PrefixStr);
    nexus.AddTestVar("ED_1_OMR_3_ADDR", ed1Omr3.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr3, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 14
     * - Device: ED_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
     *     - IPv6 Source: ED_1 OMR address (using prefix OMR_3)
     *     - IPv6 Destination: Eth_1 GUA
     * - Pass Criteria:
     *   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_3)
     */
    Log("Step 14: ED_1 OMR_3 pings Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr3);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 15
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs device to remove existing OMR prefix (OMR_3) that is being advertised and sends out new
     *     network data without the prefix OMR_3.
     *   - Note: can use OT command 'netdata unpublish <prefix>' for this.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 15: BR_2 removes OMR_3.");

    SuccessOrQuit(br2.Get<NetworkData::Publisher>().UnpublishPrefix(omr3));
    WaitForPrefix(nexus, br2, omr3, false);

    nexus.AdvanceTime(kLongActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 16
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically starts advertising its own OMR prefix OMR_2 again.
     * - Pass Criteria:
     *   - The DUT MUST register its OMR prefix OMR_2 in the Thread Network Data. Flags in the Border Router sub-TLV
     *     MUST be:
     *     - P_preference = 11 (Low)
     *     - P_default = true
     *     - P_stable = true
     *     - P_on_mesh = true
     *     - P_preferred = true
     *     - P_slaac = true
     *     - P_dhcp = false
     *     - P_dp = false
     *   - OMR_2 MUST be 64 bits long and start with 0xFD.
     *   - OMR_2 MUST be equal to the OMR_2 values as used in previous test steps.
     */
    Log("Step 16: BR_1 (DUT) re-registers OMR_2.");

    WaitForPrefix(nexus, br2, omr2, true);

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 17
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically multicasts ND RAs on Adjacent Infrastructure Link with OMR_2.
     *   - Note: the reason that OMR_3 is not advertised, is that OMR_3 was withdrawn and was not originally created by
     *     BR_1. So BR_1 has no responsibility to advertise this deprecated prefix.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
     *     - MUST contain a Route Information Option (RIO) with OMR_2.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_3.
     */
    Log("Step 17: BR_1 (DUT) multicasts ND RA with OMR_2, no OMR_3.");

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 18
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs the device to add new OMR prefix (OMR_4) numerically higher than OMR_2, but with same
     *     preference "Low".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 18: BR_2 adds OMR_4.");

    Ip6::Prefix omr4;
    SuccessOrQuit(omr4.FromString(kOmr4PrefixStr));

    PublishOmrPrefix(br2, omr4, NetworkData::kRoutePreferenceLow, true);

    WaitForPrefix(nexus, br2, omr4, true);

    nexus.AdvanceTime(kLongActionTime);
    nexus.AddTestVar("OMR_4_PREFIX", kOmr4PrefixStr);
    Log("OMR_4_PREFIX: %s", kOmr4PrefixStr);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 19
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - No change in behavior for advertising its OMR prefix, OMR_2.
     *   - Automatically starts advertising a route for the new OMR_4 prefix.
     * - Pass Criteria:
     *   - The DUT MUST NOT send a Network Data update with any one of the following prefixes in a Prefix TLV:
     *     - OMR_1, OMR_2, OMR_3, OMR_4
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST contain a Route Information Option (RIO) with OMR_2.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low).
     *     - MUST contain a Route Information Option (RIO) with OMR_4.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low).
     *     - MUST NOT contain RIO with any one of OMR_1, OMR_3.
     */
    Log("Step 19: BR_1 (DUT) continues OMR_2, adds OMR_4 on AIL.");

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 20
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs the device to update the numerically higher OMR_4 with a higher preference value 00
     *     ('Medium').
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 20: BR_2 updates OMR_4 to Medium pref.");

    PublishOmrPrefix(br2, omr4, NetworkData::kRoutePreferenceMedium, true);

    nexus.AdvanceTime(kLongActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 21
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8): Automatically withdraws its current OMR prefix OMR_2.
     * - Pass Criteria:
     *   - The DUT MUST send a Network Data update. In the new network data:
     *     - the following prefixes MUST NOT be present in a Prefix TLV: OMR_1, OMR_2, OMR_3
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST contain a Route Information Option (RIO) with OMR_2:
     *       - "Prf" bits MUST be 00 (medium) or 11 (low).
     *     - MUST contain a Route Information Option (RIO) with OMR_4:
     *       - "Prf" bits MUST be 00 (medium) or 11 (low).
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_1 or OMR_3.
     */
    Log("Step 21: BR_1 (DUT) withdraws OMR_2.");

    WaitForPrefix(nexus, br2, omr2, false);

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 22
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness instructs the device to update the prefix OMR_4 to a deprecated state by setting P_preferred =
     *     'false'.
     *   - Note: can use OT command 'netdata publish prefix' for this where the flags do not contain the 'p' (Preferred)
     *     flag.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 22: BR_2 deprecates OMR_4.");

    PublishOmrPrefix(br2, omr4, NetworkData::kRoutePreferenceMedium, false);

    nexus.AdvanceTime(kLongActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 23
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8): Automatically starts advertising its own OMR prefix OMR_2 again.
     * - Pass Criteria:
     *   - The DUT MUST register its OMR prefix OMR_2 in the Thread Network Data. Flags in the Border Router sub-TLV
     *     MUST be:
     *     - P_preference = 11 (Low)
     *     - P_default = true
     *     - P_stable = true
     *     - P_on_mesh = true
     *     - P_preferred = true
     *     - P_slaac = true
     *     - P_dhcp = false
     *     - P_dp = false
     *   - OMR_2 MUST be 64 bits long and start with 0xFD.
     *   - OMR_2 MUST be equal to the OMR_2 values as used in previous test steps.
     */
    Log("Step 23: BR_1 (DUT) re-registers OMR_2.");

    WaitForPrefix(nexus, br2, omr2, true);

    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 24
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     *   - Note: even though OMR_4 is deprecated, it is still valid so it is advertised on the AIL in a RIO.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *     - IPv6 destination MUST be ff02::1
     *     - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_1.
     *     - MUST contain a Route Information Option (RIO) with OMR_2.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     *     - MUST NOT contain a Route Information Option (RIO) with OMR_3.
     *     - MUST contain a Route Information Option (RIO) with OMR_4.
     *       - "Prf" bits MUST be 00 (medium) or 11 (low)
     */
    Log("Step 24: BR_1 (DUT) multicasts ND RA with OMR_2 and OMR_4.");

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 25
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *     - IPv6 Source: ED_1 OMR (using prefix OMR_2)
     *     - IPv6 Destination: Eth_1 GUA
     */
    Log("Step 25: Eth_1 pings ED_1 OMR_2.");

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr2, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 26
     * - Device: ED_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
     *     - IPv6 Source: ED_1 OMR address (using prefix OMR_2)
     *     - IPv6 Destination: Eth_1 GUA
     * - Pass Criteria:
     *   - ED_1 receives an ICMPv6 Echo Reply from Eth_1.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
     */
    Log("Step 26: ED_1 OMR_2 pings Eth_1 GUA.");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr2);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 27
     * - Device: BR_2
     * - Description (DBR-1.8):
     *   - Harness disables the BR function of the device. Or if that is not possible, switch it off.
     *   - Note: disabling BR may use 'br disable' CLI command.
     *   - Note: in case of switching off BR_2, the OMR prefix OMR_4 that was advertised by BR_2 remains active for some
     *     time. The Leader timeout of Leader BR_2 does not happen yet during the below steps of this test.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 27: BR_2 disabled.");

    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    nexus.AdvanceTime(kBrActionTime);
    DumpNetworkData(br2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 28
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to ED_1 via BR_1.
     *     - IPv6 Source: Eth_1 GUA
     *     - IPv6 Destination: ED_1 OMR address (using prefix OMR_2)
     * - Pass Criteria:
     *   - Eth_1 receives an ICMPv6 Echo Reply from ED_1.
     *     - IPv6 Source: ED_1 OMR (using prefix OMR_2)
     *     - IPv6 Destination: Eth_1 GUA
     */
    Log("Step 28: Eth_1 pings ED_1 OMR_2.");

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr2, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 29
     * - Device: ED_1
     * - Description (DBR-1.8):
     *   - Harness instructs the device to send an ICMPv6 Echo Request to Eth_1.
     *     - IPv6 Source: ED_1 OMR address (using prefix OMR_4)
     *     - IPv6 Destination: Eth_1 GUA.
     *   - Note: ED_1 is forced here to use a source address based on prefix OMR_4, even though that prefix is
     *     deprecated. In OT CLI this can most likely be achieved using the command:
     *     - ping -I <ED_1-OMR_4-address> <Eth_1-GUA>
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 29: ED_1 pings Eth_1 using OMR_4.");

    const Ip6::Address &ed1Omr4 = ed1.FindMatchingAddress(kOmr4PrefixStr);
    nexus.AddTestVar("ED_1_OMR_4_ADDR", ed1Omr4.ToString().AsCString());
    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kDefaultHopLimit, &ed1Omr4);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 30
     * - Device: Eth_1
     * - Description (DBR-1.8):
     *   - Automatically replies to the ICMPv6 Echo Request with Echo Reply, that is sent to BR_1.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 30: Eth_1 replies to OMR_4.");

    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 31
     * - Device: BR_1 (DUT)
     * - Description (DBR-1.8):
     *   - Automatically attempts to deliver the Echo Reply to a node on the mesh.
     * - Pass Criteria:
     *   - The DUT MUST attempt to deliver the packet to a node on the mesh, either:
     *     - 1. BR_1 sends a multicast Address Query for ED_1 OMR_4 based address into the mesh;
     *     - 2. or ED_1 receives the Echo Reply from Eth_1 as follows:
     *       - IPv6 Source: Eth_1 GUA
     *       - IPv6 Destination: ED_1 OMR address (using prefix OMR_4)
     */
    Log("Step 31: BR_1 delivers Echo Reply for OMR_4.");

    nexus.AdvanceTime(kPingResponseTime);

    nexus.SaveTestInfo("test_1_3_DBR_TC_8.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_8();
    printf("All tests passed\n");
    return 0;
}
