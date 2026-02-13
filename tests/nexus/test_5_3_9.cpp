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
 * Time to advance for the network to stabilize after routers have attached, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for DHCPv6 GUA assignment, in milliseconds.
 */
static constexpr uint32_t kDhcpAssignmentTime = 100 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoResponseTime = 5 * 1000;

/**
 * Time to wait for Router ID to expire, in milliseconds.
 */
static constexpr uint32_t kRouterIdTimeout = 580 * 1000;

/**
 * Time to wait for Child ID to expire, in milliseconds.
 */
static constexpr uint32_t kChildTimeout = 600 * 1000;

/**
 * Payload size for ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Hop limit for ICMPv6 Echo Request.
 */
static constexpr uint8_t kHopLimit = 64;

/**
 * Poll period for SED 1, in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 100;

void Test5_3_9(void)
{
    /**
     * 5.3.9 Address Query - DHCP GUA
     *
     * 5.3.9.1 Topology
     * - The Leader is configured as a Border Router with DHCPv6 server for prefixes 2001:: & 2002::
     *
     * 5.3.9.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to generate Address Query and Address
     *   Notification messages properly.
     *
     * Spec Reference                                  | V1.1 Section  | V1.3.0 Section
     * ------------------------------------------------|---------------|---------------
     * Address Query / Proactive Address Notifications | 5.4.2 / 5.4.3 | 5.4.2 / 5.4.3
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    dut.SetName("DUT");
    router3.SetName("ROUTER_3");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: Leader
     * - Description: Harness configures the device to be a DHCPv6 Border Router for prefixes 2001:: & 2002::
     * - Pass Criteria: N/A
     */
    Log("Step 1: Leader");
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2001::/64"));
        config.mDhcp      = true;
        config.mOnMesh    = true;
        config.mPreferred = true;
        config.mStable    = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2002::/64"));
        config.mDhcp      = true;
        config.mOnMesh    = true;
        config.mPreferred = true;
        config.mStable    = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */
    Log("Step 2: All");
    leader.AllowList(router1);
    router1.AllowList(leader);
    leader.AllowList(dut);
    dut.AllowList(leader);
    leader.AllowList(router3);
    router3.AllowList(leader);
    dut.AllowList(sed1);
    sed1.AllowList(dut);

    router1.Join(leader);
    dut.Join(leader);
    router3.Join(leader);

    nexus.AdvanceTime(kAttachToRouterTime);

    sed1.Join(dut, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);

    // Wait for DHCPv6 GUAs to be assigned.
    nexus.AdvanceTime(kDhcpAssignmentTime);

    nexus.AdvanceTime(kStabilizationTime);

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));

    // Save test info while all nodes have their addresses.
    nexus.SaveTestInfo("test_5_3_9.json");

    /**
     * Step 3: SED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_3 using GUA 2001:: address.
     * - Pass Criteria:
     *   - The DUT MUST generate an Address Query Request on SED_1’s behalf to find Router_3 address.
     *   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV
     *   - The DUT MUST receive and process the incoming Address Query Response and forward the ICMPv6 Echo Request
     *     packet to SED_1.
     */
    Log("Step 3: SED_1");
    nexus.SendAndVerifyEchoRequest(sed1, router3.FindMatchingAddress("2001::/64"), kEchoPayloadSize, kHopLimit,
                                   kEchoResponseTime);

    /**
     * Step 4: Router_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to SED_1 using GUA 2001:: address.
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
    Log("Step 4: Router_1");
    nexus.SendAndVerifyEchoRequest(router1, sed1.FindMatchingAddress("2001::/64"), kEchoPayloadSize, kHopLimit,
                                   kEchoResponseTime);

    /**
     * Step 5: SED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_3 using GUA 2001:: address.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query as Router_3 address should be cached.
     *   - The DUT MUST forward the ICMPv6 Echo Reply to SED_1.
     */
    Log("Step 5: SED_1");
    nexus.SendAndVerifyEchoRequest(sed1, router3.FindMatchingAddress("2001::/64"), kEchoPayloadSize, kHopLimit,
                                   kEchoResponseTime);

    /**
     * Step 6: Router_2 (DUT)
     * - Description: Harness silently powers off Router_3 and waits 580 seconds to allow Leader to expire its Router
     *   ID. Send an ICMPv6 Echo Request from MED_1 to Router_3 GUA 2001:: address.
     * - Pass Criteria:
     *   - The DUT MUST update its address cache and remove all entries based on Router_3’s Router ID.
     *   - The DUT MUST send an Address Query to discover Router_3’s RLOC address.
     */
    Log("Step 6: Router_2 (DUT)");
    Ip6::Address router3Gua = router3.FindMatchingAddress("2001::/64");
    router3.Reset();
    nexus.AdvanceTime(kRouterIdTimeout);

    sed1.SendEchoRequest(router3Gua, 0, kEchoPayloadSize);
    nexus.AdvanceTime(kEchoResponseTime);

    /**
     * Step 7: SED_1
     * - Description: Harness silently powers off SED_1 and waits to allow the DUT to timeout the child. Send two
     *   ICMPv6 Echo Requests from Router_1 to SED_1 GUA 2001:: address (one to clear the EID-to-RLOC Map Cache of the
     *   sender and the other to produce Address Query).
     * - Pass Criteria:
     *   - The DUT MUST NOT respond with an Address Notification message.
     */
    Log("Step 7: SED_1");
    Ip6::Address sed1Gua = sed1.FindMatchingAddress("2001::/64");
    sed1.Reset();
    nexus.AdvanceTime(kChildTimeout);

    router1.SendEchoRequest(sed1Gua, 1, kEchoPayloadSize);
    nexus.AdvanceTime(kEchoResponseTime);
    router1.SendEchoRequest(sed1Gua, 2, kEchoPayloadSize);
    nexus.AdvanceTime(kEchoResponseTime);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_9();
    return 0;
}
