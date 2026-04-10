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
 * Time to advance for MLR registration, in milliseconds.
 */
static constexpr uint32_t kMlrRegistrationTime = 5 * 1000;

/**
 * Time to advance for ICMPv6 Echo Reply, in milliseconds.
 */
static constexpr uint32_t kEchoReplyWaitTime = 5000;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Number of batches of multicast registrations.
 */
static constexpr uint8_t kNumBatches = 5;

/**
 * Number of multicast addresses per registration request.
 */
static constexpr uint8_t kNumAddressesPerBatch = 15;

/**
 * Total number of multicast addresses to subscribe.
 */
static constexpr uint8_t kTotalNumAddresses = kNumBatches * kNumAddressesPerBatch;

/**
 * Multicast address MA2 which is not registered.
 */
static const char kMA2[] = "ff05::1234:777a:1";

void TestMatnTc16(void)
{
    /**
     * 5.10.12 MATN-TC-16: Large number of multicast group subscriptions to BBR
     *
     * 5.10.12.1 Topology
     * - BR_1 (DUT)
     * - Router
     * - Host
     *
     * 5.10.12.2 Purpose & Description
     * The purpose of this test case is to verify that the Primary BBR can handle a large number (75) of subscriptions
     *   to different multicast groups. Multicast registrations are performed each time with 15 multicast addresses
     *   per registration request.
     *
     * Spec Reference | V1.3.0 Section
     * ---------------|---------------
     * Multicast      | 5.10.12
     */

    Core                nexus;
    Node               &br1    = nexus.CreateNode();
    Node               &router = nexus.CreateNode();
    Node               &host   = nexus.CreateNode();
    Ip6::Address        mas[kTotalNumAddresses];
    Ip6::Address        ma2;
    const Ip6::Address *hostUla;

    br1.SetName("BR_1");
    router.SetName("ROUTER");
    host.SetName("HOST");

    for (uint8_t i = 0; i < kTotalNumAddresses; i++)
    {
        char buf[OT_IP6_ADDRESS_STRING_SIZE];
        snprintf(buf, sizeof(buf), "ff04::1234:777a:%x", i + 1);
        SuccessOrQuit(mas[i].FromString(buf));
    }

    SuccessOrQuit(ma2.FromString(kMA2));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1 (DUT), Router
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation – BR_1 (DUT), Router");

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

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    hostUla = &host.mInfraIf.FindMatchingAddress("fd00::/8");

    // Add multicast variables to test info manually to ensure verify script sees them.
    for (uint8_t i = 0; i < kTotalNumAddresses; i++)
    {
        String<16> key;
        key.Append("MAS%u", i);
        nexus.AddTestVar(key.AsCString(), mas[i].ToString().AsCString());
    }
    nexus.AddTestVar("MA2", kMA2);
    nexus.AddTestVar("HOST_BACKBONE_ULA", hostUla->ToString().AsCString());

    for (uint8_t i = 1; i <= kNumBatches; i++)
    {
        /**
         * Step 1
         * - Device: Router
         * - Description: Harness instructs the device to register for 15 multicast addresses, MASi. The device
         *   automatically unicasts an MLR.req CoAP request to the DUT (BR_1) as follows:
         *   coap://[<PBBR ALOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MASi (15 addresses)
         * - Pass Criteria:
         *   - N/A
         */
        Log("Step 1: Router registers 15 multicast addresses, MASi.");

        for (uint8_t j = 0; j < kNumAddressesPerBatch; j++)
        {
            uint8_t index = (i - 1) * kNumAddressesPerBatch + j;
            SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(mas[index]));
        }

        /**
         * Step 2
         * - Device: BR_1 (DUT)
         * - Description: Automatically responds to the multicast registration.
         * - Pass Criteria:
         *   - The DUT MUST unicast an MLR.rsp CoAP response to Router_1 as follows: 2.04 changed
         *   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
         */
        Log("Step 2: BR_1 (DUT) automatically responds to the multicast registration.");

        /**
         * Step 3
         * - Device: BR_1 (DUT)
         * - Description: Auotmatically informs any other BBRs on the network of the multicast registrations.
         * - Pass Criteria:
         *   - The DUT MUST multicast a BMLR.ntf CoAP request to the Backbone Link, as follows:
         *     coap://[<All network BBRs multicast>]:BB/b/bmr
         *   - Where the payload contains: IPv6 Addresses TLV: MASi (15 addresses)
         *   - Timeout TLV: default MLR timeout of BR_1
         */
        Log("Step 3: BR_1 (DUT) automatically informs any other BBRs on the network of the multicast registrations.");
        // Checks for BLMR.ntf (BMLR.ntf) are intentionally skipped.

        /**
         * Step 4
         * - Device: BR_1 (DUT)
         * - Description: Automatically multicasts an MLDv2 message. The MLDv2 message may also be sent as multiple
         *   MLDv2 messages with content distributed across these multiple messages.
         * - Pass Criteria:
         *   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810]
         *     Section 5.2).
         *   - Where: Nr of Mcast Address Records (M): >= 15
         *   - Multicast Address Record [j]: See below
         *   - Each of the j := 0 … 14 Multicast Address Record containing an address of the set MASi contains the
         *     following: Record Type: 4 (CHANGE_TO_EXCLUDE_MODE), Number of Sources (N): 0, Multicast Address: MASi[j]
         *   - Alternatively, the DUT MAY also send multiple of above messages each with a portion of the 15
         *     addresses MASi. In this case the Nr of Mcast Address Records can be < 15 but the sum over all messages
         *     MUST be >= 15.
         */
        Log("Step 4: BR_1 (DUT) automatically multicasts an MLDv2 message.");
        // Checks for MLDv2 are intentionally skipped.

        nexus.AdvanceTime(kMlrRegistrationTime);
    }

    for (uint8_t i = 1; i <= kNumBatches; i++)
    {
        /**
         * Step 5
         * - Device: Host
         * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
         *   address, MASi[ 3 * i - 1], on the backbone link.
         * - Pass Criteria:
         *   - N/A
         */
        Log("Step 5: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MASi[ 3 * i - 1].");

        uint8_t index = (i - 1) * kNumAddressesPerBatch + (3 * i - 1);
        host.mInfraIf.SendEchoRequest(*hostUla, mas[index], kEchoIdentifier, kEchoPayloadSize);
        nexus.AdvanceTime(100);

        /**
         * Step 6
         * - Device: BR_1 (DUT)
         * - Description: Automatically forwards the ping request packet to its Thread Network.
         * - Pass Criteria:
         *   - The DUT MUST forward the ICMPv6 Echo (ping) Rquest packet of the previous step to its Thread Network
         *     encapsulated in an MPL packet, where:
         *   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
         */
        Log("Step 6: BR_1 (DUT) automatically forwards the ping request packet to its Thread Network.");

        nexus.AdvanceTime(kEchoReplyWaitTime);
    }

    /**
     * Step 7
     * - Device: Host
     * - Description: Harness instructs the device to multicast a packet to the multicast addresses, MA2.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 7: Host multicasts a packet to the multicast addresses, MA2.");
    host.mInfraIf.SendEchoRequest(*hostUla, ma2, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(100);

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the packet with multicast address, MA2, to the Thread Network.
     */
    Log("Step 8: BR_1 (DUT) does not forward the packet to its Thread Network.");

    nexus.AdvanceTime(kEchoReplyWaitTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_16.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc16();
    printf("All tests passed\n");
    return 0;
}
