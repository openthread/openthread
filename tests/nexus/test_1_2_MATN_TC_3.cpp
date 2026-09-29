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
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * Prefix used for ULA addresses.
 */
static const char kUlaPrefix[] = "fd00::/8";

/**
 * Timeout value 0 for MLR deregistration attempt.
 */
static constexpr uint32_t kTimeoutZero = 0;

void TestMatnTc3(void)
{
    /**
     * 5.10.3 MATN-TC-03: Thread device incorrectly attempts Commissioners (de)registration functions
     *
     * 5.10.3.1 Topology
     * - BR_1 (DUT)
     * - BR_2
     * - Router
     * - Host
     *
     * 5.10.3.2 Purpose & Description
     * Verify that a Primary BBR ignores a Timeout TLV if included in an MLR.req that does not contain a Commissioner
     *   Session ID TLV nor is signed by a Commissioner.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Multicast Reg.   | 5.10.3
     */

    Core                nexus;
    Node               &br1    = nexus.CreateNode();
    Node               &br2    = nexus.CreateNode();
    Node               &router = nexus.CreateNode();
    Node               &host   = nexus.CreateNode();
    Ip6::Address        ma1;
    const Ip6::Address *hostUla;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("ROUTER");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation - DUT, BR_2, Router");

    /**
     * Step 0
     * - Device: Topology
     * - Description: Topology formation - DUT, BR_2, Router
     * - Pass Criteria:
     *   - N/A
     */
    AllowLinkBetween(br1, br2);
    AllowLinkBetween(br1, router);
    AllowLinkBetween(br2, router);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    router.Join(br1, Node::kAsFtd);
    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime * 2);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    hostUla = &host.mInfraIf.FindMatchingAddress(kUlaPrefix);

    /** Add variables to test info manually to ensure verify script sees them. */
    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("HOST_BACKBONE_ULA", hostUla->ToString().AsCString());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Harness instructs the device to register the multicast address MA1");

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register the multicast address MA1
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Harness instructs the device to deregister the multicast address, MA1, at BR_1.");

    /**
     * Step 2
     * - Device: Router
     * - Description: Harness instructs the device to deregister the multicast address, MA1, at BR_1. Router unicasts
     *   an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6
     *   Addresses TLV: MA1, Timeout TLV the value 0
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(router.Get<Mlr::Manager>().RegisterMulticastListeners(&ma1, 1, &kTimeoutZero, nullptr, nullptr));
    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_1 (DUT) automatically responds successfully, ignoring the timeout value.");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description: Automatically responds to the multicast registration successfully, ignoring the timeout value.
     * - Pass Criteria:
     *   - For DUT=BR_1:
     *   - The DUT MUST unicast an MLR.rsp CoAP response to Router as follows: 2.04 changed
     *   - Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
     */
    /** This is verified by the python script. */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.");

    /**
     * Step 4
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: BR_1 (DUT) automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 5
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
     *     Network encapsulated in an MPL packet.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router receives the MPL packet and automatically unicasts an ICMPv6 Echo (ping) Reply packet.");

    /**
     * Step 6
     * - Device: Router
     * - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
     *   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc3();
    printf("All tests passed\n");
    return 0;
}
