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
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Multicast address MA5 (realm-local).
 */
static const char kMA5[] = "ff03::1234:777a:1";

/**
 * Multicast address (admin-local).
 */
static const char kAdminLocalMcast[] = "ff04::1234:777a:1";

/**
 * Multicast address (site-local).
 */
static const char kSiteLocalMcast[] = "ff05::1234:777a:1";

/**
 * Multicast address (global).
 */
static const char kGlobalMcast[] = "ff0e::1234:777a:1";

/**
 * Multicast address (link-local).
 */
static const char kLinkLocalMcast[] = "ff02::1234:777a:1";

void TestMatnTc7(void)
{
    /**
     * 5.10.6 MATN-TC-07: Default BR multicast forwarding
     *
     * 5.10.6.1 Topology
     * - BR_1
     * - BR_2
     * - ROUTER
     *
     * 5.10.6.2 Purpose & Description
     * Verify if the default forwarding of multicast on a Primary BBR is correct. Note: this can be changed by
     *   forwarding flags of the BBR; however, this is application specific and change is not tested. This test case
     *   also verifies that a Secondary BBR does not forward multicast packets to the backbone link.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Multicast        | 5.10.6       | N/A
     */

    Core  nexus;
    Node &br1    = nexus.CreateNode();
    Node &br2    = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    Ip6::Address ma5;
    Ip6::Address adminLocalMcast;
    Ip6::Address siteLocalMcast;
    Ip6::Address globalMcast;
    Ip6::Address linkLocalMcast;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("ROUTER");

    SuccessOrQuit(ma5.FromString(kMA5));
    SuccessOrQuit(adminLocalMcast.FromString(kAdminLocalMcast));
    SuccessOrQuit(siteLocalMcast.FromString(kSiteLocalMcast));
    SuccessOrQuit(globalMcast.FromString(kGlobalMcast));
    SuccessOrQuit(linkLocalMcast.FromString(kLinkLocalMcast));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation - BR_1, BR_2, ROUTER");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - BR_1, BR_2, ROUTER
     * - Pass Criteria:
     *   - N/A
     */

    br1.AllowList(br2);
    br1.AllowList(router);
    br2.AllowList(br1);
    br2.AllowList(router);
    router.AllowList(br1);
    router.AllowList(br2);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    br2.Join(br1, Node::kAsFtd);
    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    const Ip6::Address &routerGlobalAddr = router.FindGlobalAddress();

    nexus.AddTestVar("MA5", kMA5);
    nexus.AddTestVar("ADMIN_LOCAL_MCAST", kAdminLocalMcast);
    nexus.AddTestVar("SITE_LOCAL_MCAST", kSiteLocalMcast);
    nexus.AddTestVar("GLOBAL_MCAST", kGlobalMcast);
    nexus.AddTestVar("LINK_LOCAL_MCAST", kLinkLocalMcast);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: ROUTER sends an ICMPv6 Echo (ping) Request packet with realm-local scope MA5.");

    /**
     * Step 1
     * - Device: ROUTER
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet with realm-local scope
     *   MA5.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(ma5, kEchoIdentifier, kEchoPayloadSize, 64, &routerGlobalAddr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: BR_1 does not forward the multicast ping request packet to the LAN.");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with realm-local scope address
     *     MA5 to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_2 does not forward the multicast ping request packet to the LAN.");

    /**
     * Step 3
     * - Device: BR_2
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with realm-local scope address
     *     MA5 to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ROUTER sends an ICMPv6 Echo Request with admin-local scope, encapsulated in an MPL packet.");

    /**
     * Step 4
     * - Device: ROUTER
     * - Description: Harness instructs the device to sends an ICMPv6 Echo (ping) Request packet with admin-local scope
     *   (address ff04::…), encapsulated in an MPL packet.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(adminLocalMcast, kEchoIdentifier, kEchoPayloadSize, 64, &routerGlobalAddr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: BR_1 automatically forwards the ping request packet to the LAN.");

    /**
     * Step 5
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet with admin-local scope (address
     *     ff04::…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: BR_2 does not forward the multicast ping request packet to the LAN.");

    /**
     * Step 6
     * - Device: BR_2
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with admin-local scope (address
     *     ff04::…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: ROUTER sends an ICMPv6 Echo Request with site-local scope, encapsulated in an MPL packet.");

    /**
     * Step 7
     * - Device: ROUTER
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with site-local scope
     *   (address ff05::…), encapsulated in an MPL packet.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(siteLocalMcast, kEchoIdentifier, kEchoPayloadSize, 64, &routerGlobalAddr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: BR_1 automatically forwards the ping request packet to the LAN.");

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet with site-local scope (address
     *     ff05::…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: BR_2 does not forward the multicast ping request packet to the LAN.");

    /**
     * Step 9
     * - Device: BR_2
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with site-local scope (address
     *     ff05::…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: ROUTER sends an ICMPv6 Echo Request with global scope, encapsulated in an MPL packet.");

    /**
     * Step 10
     * - Device: ROUTER
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with global scope
     *   (address ff0e::…) , encapsulated in an MPL packet.
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(globalMcast, kEchoIdentifier, kEchoPayloadSize, 64, &routerGlobalAddr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: BR_1 automatically forwards the ping request packet to the LAN.");

    /**
     * Step 11
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet with global scope (address ff0e::…)
     *     to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: BR_2 does not forward the multicast ping request packet to the LAN.");

    /**
     * Step 12
     * - Device: BR_2
     * - Description: Does not forward the multicast ping request packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with global scope (address
     *     ff0e::…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: ROUTER sends an ICMPv6 Echo (ping) Request packet with link-local scope.");

    /**
     * Step 13
     * - Device: ROUTER
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with link-local scope
     *   (address ff02::…).
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(linkLocalMcast, kEchoIdentifier, kEchoPayloadSize, 64, &routerGlobalAddr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: BR_1 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 14
     * - Device: BR_1
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST NOT forward the multicast ping packet with link-local scope (address ff02:…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: BR_2 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 15
     * - Device: BR_2
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ping packet with link-local scope (address ff02:…) to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: ROUTER sends an ICMPv6 Echo Request with global scope and ML-EID as source.");

    /**
     * Step 16
     * - Device: ROUTER
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request with global scope (address
     *   ff0e::…) , encapsulated in an MPL packet. The source address is chosen as the ML-EID. This implies that the
     *   packet has to stay on the mesh. Note: this can be implemented in OT CLI using ping -I <srcaddr>
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(globalMcast, kEchoIdentifier, kEchoPayloadSize, 64,
                           &router.Get<Mle::Mle>().GetMeshLocalEid());
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: BR_1 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 17
     * - Device: BR_1
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST NOT forward the multicast ping packet with global scope to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: BR_2 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 18
     * - Device: BR_2
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ping packet with global scope to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: ROUTER sends an ICMPv6 Echo Request with global scope and link-local address as source.");

    /**
     * Step 19
     * - Device: ROUTER
     * - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request with global scope (address
     *   ff0e::…) , encapsulated in an MPL packet. The source address is chosen as the link-local address (fe80::...).
     *   This implies that the encapsulated packet must never be forwarded. Note: this can be implemented in OT CLI
     *   using ping -I <srcaddr>
     * - Pass Criteria:
     *   - N/A
     */
    router.SendEchoRequest(globalMcast, kEchoIdentifier, kEchoPayloadSize, 64,
                           &router.Get<Mle::Mle>().GetLinkLocalAddress());
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: BR_1 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 20
     * - Device: BR_1
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_1:
     *   - The DUT MUST NOT forward the multicast ping packet with global scope to the LAN.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: BR_2 does not forward the multicast ping packet to the LAN.");

    /**
     * Step 21
     * - Device: BR_2
     * - Description: Does not forward the multicast ping packet to the LAN.
     * - Pass Criteria:
     *   - For DUT = BR_2:
     *   - The DUT MUST NOT forward the multicast ping packet with global scope to the LAN.
     */

    nexus.SaveTestInfo("test_1_2_MATN_TC_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc7();
    printf("All tests passed\n");
    return 0;
}
