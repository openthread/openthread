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
 * IPv6 GUA address for Eth_1.
 */
static const char kEth1Gua[] = "2001:db8:1::1";

/**
 * IPv6 GUA prefix GUA_1.
 */
static const char kGua1Prefix[] = "2001:db8:1::/64";

void Test_1_3_DBR_TC_6(void)
{
    /**
     * 1.6. [1.3] [CERT] Reachability - Multiple BRs - Single Thread - Single IPv6 Infrastructure
     *
     * 1.6.1. Purpose
     *   To test the following:
     *   - 1. Bi-directional reachability between single Thread Network and infrastructure devices
     *   - 2. Multiple BRs
     *   - 3. IPv6 infrastructure is already existing
     *   - 4. DUT BR adopts existing OMR prefixes and doesn't advertise PIO
     *
     * 1.6.2. Topology
     *   - 1. BR 1 (DUT) - Thread Border Router
     *   - 2. BR 2-Test Bed border router device operating as a Thread Border Router and the Leader
     *   - 3. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     *   - 4. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference | V1.3.0 Section
     * ---------------|---------------
     * Reachability   | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &br2  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Device: Eth 1 Description (DBR-1.6): Harness configures Ethernet link with an on-link IPv6 GUA prefix "
        "GUA 1. Eth 1 is configured to multicast ND RAS.");

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

    nexus.AdvanceTime(10 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: Eth 1, BR 2 Description (DBR-1.6): Form topology. Wait for BR_2 to: 1. Register as border "
        "router in Thread Network Data with an OMR prefix OMR_1 2. Send multicast ND RAS");

    br2.mInfraIf.Init(br2);
    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);

    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    br2.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix omr1;
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omr1));

    br2.mInfraIf.SendRouterAdvertisement(Ip6::Address::GetLinkLocalAllNodesMulticast(), nullptr, &omr1);
    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR 1 (DUT) Description (DBR-1.6): Enable: switch on.");

    br1.mInfraIf.Init(br1);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);

    br1.AllowList(br2);
    br2.AllowList(br1);

    br1.Join(br2);
    nexus.AdvanceTime(kJoinNetworkTime);

    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2b: Device: ED 1 Description (DBR-1.6): Harness enables device.");

    ed1.AllowList(br1);
    br1.AllowList(ed1);

    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR 1 (DUT) Description (DBR-1.6): Automatically registers itself as a border router in the "
        "Thread Network Data.");

    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: BR 1 (DUT) Description (DBR-1.6): Automatically multicasts ND RAs on Adjacent Infrastructure "
        "Link.");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: Eth_1 Description (DBR-1.6): Harness instructs the device to send an ICMPv6 Echo Request to "
        "ED 1 via BR 1 or BR 2. 1. IPv6 Source: Eth 1 GUA 2. IPv6 Destination: ED_1 OMR");

    const Ip6::Address &ed1Omr = ed1.FindMatchingAddress(omr1.ToString().AsCString());
    Ip6::Address        eth1Gua;

    SuccessOrQuit(eth1Gua.FromString(kEth1Gua));

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: ED_1 Description (DBR-1.6): Harness instructs the device to send an ICMPv6 Echo Request to "
        "Eth_1. 1. IPv6 Source: ED 1 OMR 2. IPv6 Destination: Eth_1 GUA");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: BR 2 Description (DBR-1.6): Harness disables the device.");

    br2.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(10 * 1000);
    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: BR 1 (DUT) Description (DBR-1.6): Repeat Step 4");

    {
        Ip6::Nd::RouterSolicitHeader rs;
        Ip6::Nd::Icmp6Packet         packet;

        packet.Init(reinterpret_cast<const uint8_t *>(&rs), sizeof(rs));
        eth1.mInfraIf.SendIcmp6Nd(Ip6::Address::GetLinkLocalAllRoutersMulticast(), packet.GetBytes(),
                                  packet.GetLength());
    }
    nexus.AdvanceTime(kBrActionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: Eth 1 Description (DBR-1.6): Repeat Step 5");

    eth1.mInfraIf.SendEchoRequest(eth1Gua, ed1Omr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: ED 1 Description (DBR-1.6): Repeat Step 6");

    ed1.SendEchoRequest(eth1Gua, kEchoIdentifier, kEchoPayloadSize, 64, &ed1Omr);
    nexus.AdvanceTime(kPingResponseTime);

    {
        char                      macStr[18];
        InfraIf::LinkLayerAddress addr;

        br1.mInfraIf.GetLinkLayerAddress(addr);
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", addr.mAddress[0], addr.mAddress[1],
                 addr.mAddress[2], addr.mAddress[3], addr.mAddress[4], addr.mAddress[5]);
        nexus.AddTestVar("BR1_ETH", macStr);

        eth1.mInfraIf.GetLinkLayerAddress(addr);
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", addr.mAddress[0], addr.mAddress[1],
                 addr.mAddress[2], addr.mAddress[3], addr.mAddress[4], addr.mAddress[5]);
        nexus.AddTestVar("ETH1_ETH", macStr);
    }

    nexus.AddTestVar("BR1", br1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
    nexus.AddTestVar("BR2", br2.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
    nexus.AddTestVar("ED1", ed1.Get<Mac::Mac>().GetExtAddress().ToString().AsCString());
    nexus.AddTestVar("ETH1_GUA", kEth1Gua);
    nexus.AddTestVar("ED1_OMR", ed1Omr.ToString().AsCString());
    nexus.AddTestVar("OMR_PREFIX", omr1.ToString().AsCString());

    nexus.SaveTestInfo("test_1_3_DBR_TC_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_3_DBR_TC_6();
    printf("All tests passed\n");
    return 0;
}
