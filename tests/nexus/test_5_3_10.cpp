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
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 5 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoResponseTime = 5000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for Leader to expire Router ID, in seconds.
 */
static constexpr uint32_t kRouterIdExpirationTimeInSec = 580;

/**
 * Default hop limit for IPv6 packets.
 */
static constexpr uint8_t kDefaultHopLimit = 64;

/**
 * ICMPv6 Echo Request identifiers used in different steps.
 */
static constexpr uint16_t kIcmpIdentifierStep6  = 0x1234;
static constexpr uint16_t kIcmpIdentifierStep7a = 0xabcd;
static constexpr uint16_t kIcmpIdentifierStep7b = 0xabce;

/**
 * Prefix 1 for SLAAC.
 */
static constexpr char kPrefix1[] = "2003::/64";

/**
 * Prefix 2 for SLAAC.
 */
static constexpr char kPrefix2[] = "2004::/64";

void Test5_3_10(void)
{
    /**
     * 5.3.10 Address Query - SLAAC GUA
     *
     * 5.3.10.1 Topology
     * - Leader
     * - Border Router
     * - Router 1
     * - Router 2 (DUT)
     * - MED 1
     *
     * 5.3.10.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to generate Address Query and Address
     *   Notification messages.
     *
     * Spec Reference                                  | V1.1 Section | V1.3.0 Section
     * ------------------------------------------------|--------------|---------------
     * Address Query / Proactive Address Notifications | 5.4.2 / 5.4.3 | 5.4.2 / 5.4.3
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &br      = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();

    leader.SetName("LEADER");
    br.SetName("BR");
    router1.SetName("ROUTER_1");
    dut.SetName("DUT");
    med1.SetName("MED_1");

    nexus.AdvanceTime(0);
    Instance::SetLogLevel(kLogLevelNote);

    Log("Step 1: Border Router");

    /**
     * Step 1: Border Router
     * - Description: Harness configures the device with the two On-Mesh Prefixes below:
     *   - Prefix 1: P_Prefix=2003::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     *   - Prefix 2: P_Prefix=2004::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     * - Pass Criteria: N/A
     */

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    nexus.AllowLinkBetween(leader, br);

    br.Join(leader);

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

    const char *prefixes[] = {kPrefix1, kPrefix2};

    for (const char *prefixStr : prefixes)
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(prefixStr));
        config.mOnMesh       = true;
        config.mDefaultRoute = true;
        config.mStable       = true;
        config.mSlaac        = true;
        config.mPreferred    = true;
        SuccessOrQuit(br.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    br.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    Log("Step 2: All");

    /**
     * Step 2: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, dut);
    nexus.AllowLinkBetween(router1, dut);
    nexus.AllowLinkBetween(dut, med1);

    router1.Join(leader);
    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    med1.Join(dut, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: MED_1");

    /**
     * Step 3: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_1 GUA 2003:: address.
     * - Pass Criteria:
     *   - The DUT MUST generate an Address Query Request on MED_1’s behalf to find Router_1 address.
     *   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV
     *   - The DUT MUST receive and process the incoming Address Query Response, and forward the ICMPv6 Echo Request
     *     packet to Router_1.
     */

    nexus.SendAndVerifyEchoRequest(med1, router1.FindMatchingAddress(kPrefix1), 0, kDefaultHopLimit, kEchoResponseTime);

    Log("Step 4: Border Router");

    /**
     * Step 4: Border Router
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to MED_1 GUA 2003:: address.
     * - Pass Criteria:
     *   - The DUT MUST respond to the Address Query Request with a properly formatted Address Notification Message:
     *   - CoAP URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
     *   - CoAP Payload:
     *     - Target EID TLV
     *     - RLOC16 TLV
     *     - ML-EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator.
     *   - The IPv6 Destination address MUST be the RLOC of the destination.
     */

    nexus.SendAndVerifyEchoRequest(br, med1.FindMatchingAddress(kPrefix1), 0, kDefaultHopLimit, kEchoResponseTime);

    Log("Step 5: MED_1");

    /**
     * Step 5: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_1 GUA 2003:: address.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query, as the Router_1 address should be cached.
     *   - The DUT MUST forward the ICMPv6 Echo Reply to MED_1.
     */

    nexus.SendAndVerifyEchoRequest(med1, router1.FindMatchingAddress(kPrefix1), 0, kDefaultHopLimit, kEchoResponseTime);

    nexus.SaveTestInfo("test_5_3_10.json");

    Log("Step 6: Router_2 (DUT)");

    /**
     * Step 6: Router_2 (DUT)
     * - Description: Harness silently powers off Router_1 and waits 580 seconds to allow the Leader to expire its
     *   Router ID. Send an ICMPv6 Echo Request from MED_1 to Router_1 GUA 2003:: address.
     * - Pass Criteria:
     *   - The DUT MUST update its address cache and removes all entries based on Router_1’s Router ID.
     *   - The DUT MUST send an Address Query Request to discover Router_1’s RLOC address.
     */

    Ip6::Address router1Gua = router1.FindMatchingAddress(kPrefix1);

    router1.Reset();
    nexus.AdvanceTime(kRouterIdExpirationTimeInSec * 1000);

    med1.SendEchoRequest(router1Gua, kIcmpIdentifierStep6);
    nexus.AdvanceTime(kEchoResponseTime);

    Log("Step 7: MED_1");

    /**
     * Step 7: MED_1
     * - Description: Harness silently powers off MED_1 and waits to allow the DUT to timeout the child. Send two
     *   ICMPv6 Echo Requests from Border Router to MED_1 GUA 2003:: address (one to clear the EID-to-RLOC Map Cache of
     *   the sender and the other to produce Address Query).
     * - Pass Criteria:
     *   - The DUT MUST NOT respond with an Address Notification message.
     */

    Ip6::Address med1Gua = med1.FindMatchingAddress(kPrefix1);

    med1.Reset();
    nexus.AdvanceTime(dut.Get<Mle::Mle>().GetTimeout() * 1000);

    /**
     * First Echo Request to clear the EID-to-RLOC Map Cache of the sender (BR)
     */
    br.SendEchoRequest(med1Gua, kIcmpIdentifierStep7a);
    nexus.AdvanceTime(kEchoResponseTime);

    /**
     * Second Echo Request to produce Address Query
     */
    br.SendEchoRequest(med1Gua, kIcmpIdentifierStep7b);
    nexus.AdvanceTime(kEchoResponseTime);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_10();
    printf("All tests passed\n");
    return 0;
}
