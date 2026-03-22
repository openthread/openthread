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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for MLR registration, in milliseconds.
 */
static constexpr uint32_t kMlrRegistrationTime = 5 * 1000;

/**
 * Time to advance for ICMPv6 Echo Reply, in milliseconds.
 */
static constexpr uint32_t kEchoReplyWaitTime = 5000;

/**
 * Poll period for SED in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 1000;

/**
 * Time to allow for a Child Update Request, in milliseconds.
 */
static constexpr uint32_t kChildUpdateRequestTimeMs = 1000;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Multicast address MA2.
 */
static const char kMA2[] = "ff05::2";

/**
 * Multicast address MA3.
 */
static const char kMA3[] = "ff05::3";

/**
 * Multicast address MA5.
 */
static const char kMA5[] = "ff05::5";

void TestMatnTc19(void)
{
    /**
     * 5.10.15 MATN-TC-19: Multicast registration by MTD
     *
     * Purpose:
     * The purpose of this test case is to verify that an MTD can register a multicast address through a Parent Thread
     *   Router.
     *
     * Spec Reference | V1.2 Section
     * ---------------|-------------
     * MATN-TC-19     | 5.10.15
     */

    Core                nexus;
    Node               &br1    = nexus.CreateNode();
    Node               &br2    = nexus.CreateNode();
    Node               &router = nexus.CreateNode();
    Node               &mtd    = nexus.CreateNode();
    Node               &host   = nexus.CreateNode();
    Ip6::Address        ma2;
    Ip6::Address        ma3;
    Ip6::Address        ma5;
    const Ip6::Address *hostUla;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("Router");
    mtd.SetName("MTD");
    host.SetName("Host");

    SuccessOrQuit(ma2.FromString(kMA2));
    SuccessOrQuit(ma3.FromString(kMA3));
    SuccessOrQuit(ma5.FromString(kMA5));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Topology formation – BR_1, BR_2, Router, MTD");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1, BR_2, Router, MTD
     * - Pass Criteria:
     *   - N/A
     */

    br1.AllowList(br2);
    br1.AllowList(router);
    br2.AllowList(br1);
    br2.AllowList(router);
    router.AllowList(br1);
    router.AllowList(br2);
    router.AllowList(mtd);
    mtd.AllowList(router);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    br2.Join(br1, Node::kAsFtd);
    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    SuccessOrQuit(mtd.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
    mtd.Join(router, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(mtd.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(router.Get<BackboneRouter::Leader>().HasPrimary());

    hostUla = &host.mInfraIf.FindMatchingAddress("fd00::/8");

    nexus.AddTestVar("TEST_MA2", kMA2);
    nexus.AddTestVar("TEST_MA3", kMA3);
    nexus.AddTestVar("TEST_MA5", kMA5);
    nexus.AddTestVar("HOST_BACKBONE_ULA", hostUla->ToString().AsCString());

    Log("Step 1a: MTD Registers addresses MA2 & MA5 with its parent.");

    /**
     * Step 1a
     * - Device: MTD
     * - Description: Registers addresses MA2 & MA5 with its parent. (MA5 is optional for MEDs)
     * - Pass Criteria:
     *   - For DUT = MTD:
     *   - The DUT MUST unicast an MLE Child Update Request command to Router, where the payload contains (amongst other
     *     fields):
     *   - Address Registration TLV: MA2
     *   - MA5 (Optional for MED)
     */
    SuccessOrQuit(mtd.Get<Ip6::Netif>().SubscribeExternalMulticast(ma2));
    SuccessOrQuit(mtd.Get<Ip6::Netif>().SubscribeExternalMulticast(ma5));
    nexus.AdvanceTime(kChildUpdateRequestTimeMs);

    Log("Step 1b: Router Automatically responds to the registration request.");

    /**
     * Step 1b
     * - Device: Router
     * - Description: Automatically responds to the registration request.
     * - Pass Criteria:
     *   - For DUT = Router:
     *   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
     *     fields):
     *   - Address Registration TLV: MA2
     *   - MA5
     */

    Log("Step 1c: Router Automatically registers multicast address, MA2 at BR_1");

    /**
     * Step 1c
     * - Device: Router
     * - Description: Automatically registers multicast address, MA2 at BR_1
     * - Pass Criteria:
     *   - For DUT = Router:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains:
     *   - IPv6 Addresses TLV: MA2
     */
    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("Step 2: MTD Registers addresses MA2 and MA3 with its parent.");

    /**
     * Step 2
     * - Device: MTD
     * - Description: Registers addresses MA2 and MA3 with its parent.
     * - Pass Criteria:
     *   - For DUT = MTD:
     *   - The DUT MUST unicast an MLE Child Update Request command to Router_1, where the payload contains (amongst
     *     other fields):
     *   - Address Registration TLV: MA2 and MA3.
     */
    SuccessOrQuit(mtd.Get<Ip6::Netif>().UnsubscribeExternalMulticast(ma5));
    SuccessOrQuit(mtd.Get<Ip6::Netif>().SubscribeExternalMulticast(ma3));
    nexus.AdvanceTime(kChildUpdateRequestTimeMs);

    Log("Step 3: Router Automatically responds to the registration request.");

    /**
     * Step 3
     * - Device: Router
     * - Description: Automatically responds to the registration request.
     * - Pass Criteria:
     *   - For DUT = Router:
     *   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
     *     fields):
     *   - Address Registration TLV: MA2 and MA3.
     */

    Log("Step 4: Router Wait at most PARENT_AGGREGATE_DELAY.");

    /**
     * Step 4
     * - Device: Router
     * - Description: Wait at most PARENT_AGGREGATE_DELAY.
     * - Pass Criteria:
     *   - The next message (below) is sent not later than PARENT_AGGREGATE_DELAY seconds since step 3.
     */

    Log("Step 5: Router Automatically registers multicast addresses, MA2 and MA3 at BR_1.");

    /**
     * Step 5
     * - Device: Router
     * - Description: Automatically registers multicast addresses, MA2 and MA3 at BR_1. Unicasts an MLR.req CoAP request
     *   to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV:
     *   MA3, MA2 (optional).
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("Step 6: BR_1 Automatically responds to the registration request.");

    /**
     * Step 6
     * - Device: BR_1
     * - Description: Automatically responds to the registration request. Unicasts an MLR.rsp CoAP response to Router_1
     *   as follows: 2.04 changed Where the payload contains: Status TLV: ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */

    Log("Step 7: Host ICMPv6 Echo (ping) Request packet to multicast address, MA3, on the backbone link.");

    /**
     * Step 7
     * - Device: Host
     * - Description: Harness instructs the device to send ICMPv6 Echo (ping) Request packet to multicast address, MA3,
     *   on the backbone link.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma3, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 8: BR_1 Automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Automatically forwards the ping request packet to its Thread Network encapsulated in an MPL
     *   packet.
     * - Pass Criteria:
     *   - N/A
     */

    Log("Step 8a: Router Receives the multicast ping request packet.");

    /**
     * Step 8a
     * - Device: Router
     * - Description: Receives the multicast ping request packet and if MTD is a SED then it automatically pends it for
     *   delivery to MTD. If MTD is MED, it automatically forwards the packet as defined in step 10.
     * - Pass Criteria:
     *   - None.
     */

    Log("Step 9: MTD polls for the pending ping request packet.");

    /**
     * Step 9
     * - Device: MTD
     * - Description: If MTD is a SED, it wakes up and/or polls for the pending ping request packet. If MTD is a MED,
     *   this test step is skipped.
     * - Pass Criteria:
     *   - For DUT = MTD (SED):
     *   - The DUT MUST send a MAC poll request to Router_1.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 10: Router Automatically forwards the ping request packet to MTD.");

    /**
     * Step 10
     * - Device: Router
     * - Description: Automatically forwards the ping request packet to MTD.
     * - Pass Criteria:
     *   - For DUT = Router:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA3, to MTD using
     *     802.15.4 unicast.
     *   - The ping request packet MUST NOT be encapsulated in an MPL packet.
     */

    Log("Step 11: MTD sends a unicast ICMPv6 Echo Response packet back to Host.");

    /**
     * Step 11
     * - Device: MTD
     * - Description: Receives the multicast ping request packet and sends a unicast ICMPv6 Echo (ping) Response packet
     *   back to Host.
     * - Pass Criteria:
     *   - For DUT = MTD:
     *   - The DUT MUST unicast a ICMPv6 Echo (ping) Response packet back to Host.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_19.json");
}

} /* namespace Nexus */
} /* namespace ot */

int main(void)
{
    ot::Nexus::TestMatnTc19();
    printf("All tests passed\n");
    return 0;
}
