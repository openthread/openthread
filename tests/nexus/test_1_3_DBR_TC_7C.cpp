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
 * Time to advance for waiting, in milliseconds.
 */
static constexpr uint32_t kWaitTime = 20 * 1000;

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
 * IPv6 Hop Limit.
 */
static constexpr uint8_t kHopLimit = 64;

/**
 * Last byte of Eth 1 GUA address.
 */
static constexpr uint8_t kEth1GuaLastByte = 1;

/**
 * Ethernet 1 GUA prefix.
 */
static constexpr char kGua1Prefix[] = "2001:db8:1::/64";

/**
 * Border Router 2 Network Data prefix.
 */
static constexpr char kPre1Prefix[] = "fd12:3456:abcd:1234::/64";

void Test_1_3_DBR_TC_7C(void)
{
    /**
     * 1.7(c). [1.3] [CERT] Reachability - Multiple BRs - Single Thread / Single IPv6 Infrastructure - Presence of
     *   non-OMR prefixes
     *
     * 1.7c.1. Purpose
     * To test the following:
     * 1. Bi-directional reachability between single Thread Network and infrastructure devices
     * 2. DUT BR creates own OMR prefix when existing prefixes are not usable (e.g. no SLAAC, or deprecated)
     *
     * 1.7c.2. Topology
     * 1. Eth 1-Adjacent Infrastructure Link Reference Device
     * 2. BR 1 (DUT) - Border Router
     * 3. BR 2-Border Router Reference Device
     * 4. Rtr 1-Thread Router Reference Device and Leader
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Reachability     | N/A          | 1.3
     */

    Core nexus;

    Node &eth1 = nexus.CreateNode();
    Node &br1  = nexus.CreateNode();
    Node &br2  = nexus.CreateNode();
    Node &rtr1 = nexus.CreateNode();

    eth1.SetName("Eth_1");
    br1.SetName("BR_1");
    br2.SetName("BR_2");
    rtr1.SetName("RTR_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Device: Eth 1 Description (DBR-1.7c): Enable.");

    /**
     * Step 0
     * - Device: Eth 1
     * - Description (DBR-1.7c): Enable. Harness configures Ethernet link with an on-link IPv6 GUA prefix GUA_1. Eth_1
     *   is configured to multicast ND RAs.
     * - Pass Criteria:
     *   - N/A
     */

    eth1.mInfraIf.Init(eth1);
    Ip6::Prefix gua1Prefix;
    SuccessOrQuit(gua1Prefix.FromString(kGua1Prefix));

    Ip6::Address gua1Address;
    gua1Address                                      = AsCoreType(&gua1Prefix.mPrefix);
    gua1Address.mFields.m8[sizeof(Ip6::Address) - 1] = kEth1GuaLastByte;
    eth1.mInfraIf.AddAddress(gua1Address);
    eth1.mInfraIf.StartRouterAdvertisement(gua1Prefix);
    nexus.AddTestVar("GUA_1", kGua1Prefix);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0b: Device: Rtr 1 Description (DBR-1.7c): Enable. Becomes Leader.");

    /**
     * Step 0b
     * - Device: Rtr 1
     * - Description (DBR-1.7c): Enable. Becomes Leader.
     * - Pass Criteria:
     *   - N/A
     */

    rtr1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(rtr1.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: Eth 1, BR 2, Rtr 1 Description (DBR-1.7c): Enable; connects to Rtr_1.");

    /**
     * Step 1
     * - Device: Eth 1, BR 2, Rtr 1
     * - Description (DBR-1.7c): Enable; connects to Rtr_1. Harness configures BR_2 Network Data with prefix PRE_1 as
     *   below. fd12:3456:abcd:1234::/64 (ULA prefix) P_slaac = true (SLAAC is enabled for prefix) P_on_mesh = false
     *   (not an on-mesh prefix) P_stable = true P_preferred = true P_dhcp = false P_default = true P_preference = 00
     *   (medium) P_dp = false. Note: this looks almost like an OMR prefix, but due to having P_on_mesh = false it is
     *   not a valid OMR prefix and would not be usable by (new) Thread Devices for connectivity. Note 1: the
     *   automatic creation of an OMR prefix as a BR would normally do, is disabled by the Harness for BR_2. Instead,
     *   the administratively configured PRE_1 is used only. Note 2: to disable the automatic OMR prefix creation as
     *   stated above, one method is (if no better method is available) to disable the BR_2 border routing using "br
     *   disable" OT CLI command to stop the automatic prefix creation. Then using "netdata publish prefix" the
     *   device could configure different PRE_1 prefixes as needed per test run. Form topology (if not already
     *   formed). Wait for BR_2 to: Register as border router in Thread Network Data with prefix PRE_1. Send multicast
     *   ND RAs on AIL.
     * - Pass Criteria:
     *   - N/A
     */

    br2.Join(rtr1);
    nexus.AdvanceTime(kJoinNetworkTime);

    nexus.AddTestVar("PRE_1", kPre1Prefix);

    NetworkData::OnMeshPrefixConfig config;
    SuccessOrQuit(AsCoreType(&config.mPrefix).FromString(kPre1Prefix));
    config.mPreference   = NetworkData::kRoutePreferenceMedium;
    config.mPreferred    = true;
    config.mSlaac        = true;
    config.mDhcp         = false;
    config.mDefaultRoute = true;
    config.mOnMesh       = false;
    config.mStable       = true;

    SuccessOrQuit(br2.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    br2.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.mInfraIf.Init(br2);
    Ip6::Prefix pre1Prefix;
    SuccessOrQuit(pre1Prefix.FromString(kPre1Prefix));
    br2.mInfraIf.StartRouterAdvertisement(pre1Prefix);

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR 1 (DUT) Description (DBR-1.7c): Enable: switch on.");

    /**
     * Step 2
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.7c): Enable: switch on.
     * - Pass Criteria:
     *   - N/A
     */

    br1.Join(rtr1);
    nexus.AdvanceTime(kJoinNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR 1 (DUT) Description (DBR-1.7c): Automatically registers itself as a border router.");

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.7c): Automatically registers itself as a border router in the Thread Network Data and
     *   provides OMR prefix.
     * - Pass Criteria:
     *   - The DUT MUST register a new OMR Prefix (OMR_1) in the Thread Network Data, in a Prefix TLV.
     *   - Flags in the Border Router sub-TLV MUST be:
     *   - 17. P_preference = 11 (Low)
     *   - 18. P_default = true
     *   - 19. P_stable = true
     *   - 20. P_on_mesh = true
     *   - 21. P_preferred = true
     *   - 22. P_slaac = true
     *   - 23. P_dhcp = false
     *   - 24. P_dp = false
     *   - OMR_1 MUST be 64 bits long and start with 0xFD.
     */

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omr1;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr1));
    nexus.AddTestVar("OMR_1", omr1.ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: BR 1 (DUT) Description (DBR-1.7c): Automatically multicasts ND RAs on AIL.");

    /**
     * Step 4
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.7c): Automatically multicasts ND RAs on Adjacent Infrastructure Link.
     * - Pass Criteria:
     *   - The DUT MUST multicast ND RAs on the infrastructure link:
     *   - IPv6 destination MUST be ff02::1
     *   - MUST NOT contain a Prefix Information Option (PIO) with a ULA prefix.
     *   - MUST contain a Route Information Option (RIO) with OMR_1.
     *   - MUST NOT contain a Route Information Option (RIO) with PRE_1. (Note: reason is that it is not an on-mesh
     *     prefix.)
     */

    // Time already advanced.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: Eth 1 Description (DBR-1.7c): Harness instructs device to send Echo Request to Rtr 1.");

    /**
     * Step 5
     * - Device: Eth 1
     * - Description (DBR-1.7c): Harness instructs the device to send an ICMPv6 Echo Request to Rtr 1 via BR_1 or BR 2.
     *   IPv6 Source: Eth 1 GUA IPv6 Destination: Rtr 1 OMR address.
     * - Pass Criteria:
     *   - Eth 1 receives an ICMPv6 Echo Reply from Rtr 1.
     *   - IPv6 Source: Rtr_1 OMR
     *   - IPv6 Destination: Eth_1 GUA
     */

    const Ip6::Address &eth1Gua = eth1.mInfraIf.FindMatchingAddress(kGua1Prefix);
    const Ip6::Address &rtr1Omr = rtr1.FindMatchingAddress(omr1.ToString().AsCString());

    nexus.AddTestVar("ETH_1_GUA_ADDR", eth1Gua.ToString().AsCString());
    nexus.AddTestVar("RTR_1_OMR_ADDR", rtr1Omr.ToString().AsCString());

    eth1.mInfraIf.SendEchoRequest(eth1Gua, rtr1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: Rtr 1 Description (DBR-1.7c): Harness instructs device to send Echo Request to Eth 1.");

    /**
     * Step 6
     * - Device: Rtr 1
     * - Description (DBR-1.7c): Harness instructs the device to send an ICMPv6 Echo Request to Eth 1. IPv6 Source:
     * Rtr_1 OMR address IPv6 Destination: Eth 1 GUA
     * - Pass Criteria:
     *   - Rtr 1 receives an ICMPv6 Echo Reply from Eth 1.
     *   - IPv6 Source: Eth 1 GUA
     *   - IPv6 Destination: Rtr_1 OMR address
     */

    rtr1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kHopLimit, &rtr1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: BR 2 Description (DBR-1.7c): Harness disables the device.");

    /**
     * Step 7
     * - Device: BR 2
     * - Description (DBR-1.7c): Harness disables the device
     * - Pass Criteria:
     *   - N/A
     */

    br2.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7a: Device: N/A Description (DBR-1.7c): Harness waits -20 seconds.");

    /**
     * Step 7a
     * - Device: N/A
     * - Description (DBR-1.7c): Harness waits -20 seconds
     * - Pass Criteria:
     *   - N/A
     */

    nexus.AdvanceTime(kWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7b: Device: Eth 1 Description (DBR-1.7c): Harness instructs device to send an ND RS message.");

    /**
     * Step 7b
     * - Device: Eth 1
     * - Description (DBR-1.7c): Harness instructs the device to send an ND RS message to trigger the DUT to send a ND
     *   RA for the next step. Note: in Linux, this is done with the command rdisc6v eth0. The output of the command is
     *   captured in the test log.
     * - Pass Criteria:
     *   - N/A
     */

    Ip6::Nd::RouterSolicitHeader rsHeader;
    eth1.mInfraIf.SendIcmp6Nd(Ip6::Address::GetLinkLocalAllRoutersMulticast(),
                              reinterpret_cast<const uint8_t *>(&rsHeader), sizeof(rsHeader));
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: BR 1 (DUT) Description (DBR-1.7c): Repeat Step 4.");

    /**
     * Step 8
     * - Device: BR 1 (DUT)
     * - Description (DBR-1.7c): Repeat Step 4. Note: since the prefix PRE_1 is still present in the Leader's Network
     *   Data for some time, BR 1 will continue to advertise the route for PRE 1 as indicated in Step 4 criteria.
     * - Pass Criteria:
     *   - Repeat Step 4
     */

    // Time already advanced.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: Eth_1 Description (DBR-1.7c): Repeat Step 5.");

    /**
     * Step 9
     * - Device: Eth_1
     * - Description (DBR-1.7c): Repeat Step 5
     * - Pass Criteria:
     *   - Repeat Step 5
     */

    eth1.mInfraIf.SendEchoRequest(eth1Gua, rtr1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: Rtr_1 Description (DBR-1.7c): Repeat Step 6.");

    /**
     * Step 10
     * - Device: Rtr_1
     * - Description (DBR-1.7c): Repeat Step 6
     * - Pass Criteria:
     *   - Repeat Step 6
     */

    rtr1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, kHopLimit, &rtr1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    nexus.SaveTestInfo("test_1_3_DBR_TC_7C.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_7C();
    printf("All tests passed\n");
    return 0;
}
