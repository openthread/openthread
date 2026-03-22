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
 * MLR Timeout Minimum value in seconds.
 */
static constexpr uint32_t kMlrTimeoutMin = 300;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * ICMPv6 Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * UDP payload size.
 */
static constexpr uint16_t kUdpPayloadSize = 20;

/**
 * CoAP port.
 */
static constexpr uint16_t kCoapPort = 5683;

/**
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * ULA prefix for host address lookup.
 */
static const char kUlaPrefix[] = "fd00::/8";

void TestMatnTc5(void)
{
    /**
     * 5.10.5 MATN-TC-05: Re-registration to same Multicast Group
     *
     * 5.10.5.1 Topology
     * - BR_1 (DUT)
     * - BR_2
     * - Router
     * - Host
     *
     * 5.10.5.2 Purpose & Description
     * Verify that a Primary BBR (DUT) can manage a re-registration of a device on its network to remain receiving
     *   multicasts. The test also verifies the usage of UDP multicast packets across backbone and internal Thread
     *   network.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Multicast        | 5.10.5       | 5.10.5
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
    router.SetName("Router");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: BR_1 (DUT)
     * - Description: Topology formation – BR_1 (DUT). Boot DUT. Configure the MLR timeout value of the DUT to be
     *   MLR_TIMEOUT_MIN seconds.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation – BR_1 (DUT)");

    br1.AllowList(br2);
    br1.AllowList(router);
    br2.AllowList(br1);
    br2.AllowList(router);
    router.AllowList(br1);
    router.AllowList(br2);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout = kMlrTimeoutMin;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }

    /**
     * Step 0 (cont.)
     * - Device: N/A
     * - Description: Topology formation – BR_2, Router
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0 (cont.): Topology formation – BR_2, Router");

    router.Join(br1, Node::kAsFtd);
    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    host.mInfraIf.Init(host);
    host.mInfraIf.AddAddress(host.mInfraIf.GetLinkLocalAddress());

    nexus.AdvanceTime(kStabilizationTime * 2);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    hostUla = &host.mInfraIf.FindMatchingAddress(kUlaPrefix);

    // Add multicast variables to test info manually to ensure verify script sees them.
    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("HOST_ULA", hostUla->ToString().AsCString());
    {
        String<16> timeoutString;
        timeoutString.Append("%lu", ToUlong(kMlrTimeoutMin));
        nexus.AddTestVar("MLR_TIMEOUT_MIN", timeoutString.AsCString());
    }

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register multicast address, MA1
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: Router registers multicast address, MA1");
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 2
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Host sends an ICMPv6 Echo (ping) Request packet to the multicast address, MA1.");
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
     *     Network encapsulated in an MPL packet.
     */
    Log("Step 3: BR_1 (DUT) automatically forwards the ping request packet to its Thread Network.");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4
     * - Device: Router
     * - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
     *   automatically unicasts an ICMPv6 (ping) Reply packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: Router automatically unicasts an ICMPv6 (ping) Reply packet back to Host.");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4a
     * - Device: Router
     * - Description: Within MLR_TIMEOUT_MIN seconds of initial registration, Harness instructs the device to
     *   re-register for multicast address, MA1, at BR_1 by performing test case MATN-TC-02 (see 5.10.2) using Router
     *   as TD with the respective pass criteria.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4a: Router re-registers for multicast address, MA1, at BR_1.");
    IgnoreError(router.Get<Ip6::Netif>().UnsubscribeExternalMulticast(ma1));
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    /**
     * Step 5
     * - Device: Host
     * - Description: Within MLR_TIMEOUT_MIN seconds, Harness instructs the device to send a UDP packet to the
     *   multicast address, MA1. The destination port 5683 is used for the UDP Multicast packet transmission.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 5: Host sends a UDP packet to the multicast address, MA1, port 5683.");
    host.mInfraIf.SendUdp(*hostUla, ma1, kCoapPort, kCoapPort, kUdpPayloadSize);
    nexus.AdvanceTime(0);

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the UDP ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST forward the UDP packet with multicast address, MA1, to its Thread Network
     *     encapsulated in an MPL packet.
     */
    Log("Step 6: BR_1 (DUT) automatically forwards the UDP packet to its Thread Network.");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7
     * - Device: Router
     * - Description: Receives the ping request packet. Automatically uses the port 5683 (CoAP port) to verify that the
     *   UDP Multicast packet is received.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 7: Router receives the UDP multicast packet.");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7a
     * - Device: Router
     * - Description: Harness instructs the device to stop listening to the multicast address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 7a: Router stops listening to the multicast address, MA1.");
    SuccessOrQuit(router.Get<Ip6::Netif>().UnsubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 8
     * - Device: Host
     * - Description: After (MLR_TIMEOUT_MIN+2) seconds, harness instructs the device to multicast an ICMPv6 Echo (ping)
     *   Request packet to multicast address, MA1, on the backbone link.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: After (MLR_TIMEOUT_MIN+2) seconds, Host multicasts an ICMPv6 Echo Request to MA1.");
    nexus.AdvanceTime((kMlrTimeoutMin + 2) * 1000);
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    /**
     * Step 9
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
     *     Network.
     */
    Log("Step 9: BR_1 (DUT) does not forward the ping request packet to its Thread Network.");
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc5();
    printf("All tests passed\n");
    return 0;
}
