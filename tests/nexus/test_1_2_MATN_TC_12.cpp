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
static constexpr uint32_t kFormNetworkTime = 10 * 1000;

/**
 * Time to advance for a node to join as a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * ICMPv6 Echo Request identifiers.
 */
static constexpr uint16_t kEchoIdentifier1 = 0x1231;
static constexpr uint16_t kEchoIdentifier2 = 0x1232;
static constexpr uint16_t kEchoIdentifier3 = 0x1233;
static constexpr uint16_t kEchoIdentifier4 = 0x1234;
static constexpr uint16_t kEchoIdentifier5 = 0x1235;
static constexpr uint16_t kEchoIdentifier6 = 0x1236;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 130;

/**
 * Multicast address (admin-local).
 */
static const char kAdminLocalMcast[] = "ff04::1234:777a:1";

/**
 * Multicast address (site-local).
 */
static const char kSiteLocalMcast[] = "ff05::1234:777a:1";

/**
 * IPv6 Hop Limit values.
 */
static constexpr uint8_t kHopLimit59  = 59;
static constexpr uint8_t kHopLimit1   = 1;
static constexpr uint8_t kHopLimit159 = 159;
static constexpr uint8_t kHopLimit2   = 2;
static constexpr uint8_t kHopLimit0   = 0;

void TestMatnTc12(void)
{
    /**
     * 5.10.9 MATN-TC-12: Hop limit processing
     *
     * 5.10.9.1 Topology
     * - BR_1 (DUT)
     * - Router
     * - Host
     *
     * 5.10.9.2 Purpose & Description
     * The purpose of this test case is to verify that a Primary BBR correctly decrements the Hop Limit field by 1 if
     *   packet is forwarded with MPL, and if forwarded from Thread Network (using MPL) to LAN. This test also verifies
     *   that the BR drops a packet with Hop Limit 0. It also checks the use of IPv6 packets that span multiple 6LoWPAN
     *   fragments.
     *
     * Spec Reference | V1.1 Section | V1.2 Section | V1.3.0 Section
     * ---------------|--------------|--------------|---------------
     * Multicast      | N/A          | 5.10.9       | 5.10.9
     */

    Core                nexus;
    Node               &br1    = nexus.CreateNode();
    Node               &router = nexus.CreateNode();
    Node               &host   = nexus.CreateNode();
    Ip6::Address        ma1;
    Ip6::Address        ma2;
    const Ip6::Address *hostUla;

    br1.SetName("BR_1");
    router.SetName("Router");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kAdminLocalMcast));
    SuccessOrQuit(ma2.FromString(kSiteLocalMcast));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Topology formation – BR_1 (DUT), Router");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1 (DUT), Router
     * - Pass Criteria:
     *   - N/A
     */
    br1.AllowList(router);
    router.AllowList(br1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);
    hostUla = &host.mInfraIf.FindMatchingAddress("fd00::/8");

    nexus.AddTestVar("MA1", kAdminLocalMcast);
    nexus.AddTestVar("MA2", kSiteLocalMcast);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    Log("BR_1 is Primary BBR");

    Log("Step 1: Harness instructs the device to register a multicast address, MA1");

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register a multicast address, MA1
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(router.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 2: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast "
        "address, MA1, with the IPv6 Hop Limit field set to 59. The size of the payload is 130 bytes.");

    /**
     * Step 2
     * - Device: Host
     * - Description: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1, with the IPv6 Hop Limit field set to 59. The size of the payload is 130 bytes.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier1, kEchoPayloadSize, kHopLimit59);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: Automatically forwards the ping request packet onto the Thread Network");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet onto the Thread Network
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet to Router_1 as an MPL packet encapsulating the
     *     IPv6 packet with the Hop Limit field of the inner packet set to 58.
     */

    Log("Step 4: Receives the multicast ping request packet.");

    /**
     * Step 4
     * - Device: Router
     * - Description: Receives the multicast ping request packet.
     * - Pass Criteria:
     *   - N/A
     */

    Log("Step 5: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast "
        "address, MA1, with the IPv6 Hop Limit field set to 1. The size of the payload is 130 bytes.");

    /**
     * Step 5
     * - Device: Host
     * - Description: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1, with the IPv6 Hop Limit field set to 1. The size of the payload is 130 bytes.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier2, kEchoPayloadSize, kHopLimit1);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 6: Does not forward the ping request packet.");

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the ping request packet.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the Thread Network.
     */

    Log("Step 7: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL "
        "packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set to "
        "159. The size of the payload is 130 bytes.");

    /**
     * Step 7
     * - Device: Router
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
     *   packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set to 159.
     *   The size of the payload is 130 bytes.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma2, kEchoIdentifier3, kEchoPayloadSize, kHopLimit159);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 8: Automatically forwards the ping request packet to the LAN.");

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet to the LAN with the Hop Limit field set
     *     to 158.
     */

    Log("Step 9: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL "
        "multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet "
        "set to 2. The size of the payload is 130 bytes.");

    /**
     * Step 9
     * - Device: Router
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
     *   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
     *   to 2. The size of the payload is 130 bytes.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma2, kEchoIdentifier4, kEchoPayloadSize, kHopLimit2);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 10: Automatically forwards the ping request packet to the LAN.");

    /**
     * Step 10
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet to the LAN with the Hop Limit field set
     *     to 1.
     */

    Log("Step 11: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet encapsulated in an MPL "
        "multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet "
        "set to 1. The size of the payload is 130 bytes.");

    /**
     * Step 11
     * - Device: Router
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet encapsulated in an MPL
     *   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
     *   to 1. The size of the payload is 130 bytes.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma2, kEchoIdentifier5, kEchoPayloadSize, kHopLimit1);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 12: Does not forward the ping request packet to the LAN.");

    /**
     * Step 12
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the LAN.
     */

    Log("Step 13: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL "
        "multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet "
        "set to 0.");

    /**
     * Step 13
     * - Device: Router
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
     *   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
     *   to 0.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma2, kEchoIdentifier6, kEchoPayloadSize, kHopLimit0);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 14: Does not forward the ping request packet to the LAN.");

    /**
     * Step 14
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the ping request packet to the LAN.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the LAN.
     */

    nexus.SaveTestInfo("test_1_2_MATN_TC_12.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc12();
    printf("All tests passed\n");
    return 0;
}
