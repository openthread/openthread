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
 * MLR_TIMEOUT_MIN in seconds.
 */
static constexpr uint32_t kMlrTimeoutMin = 300;

/**
 * Buffer time after timeout expiry, in seconds.
 */
static constexpr uint32_t kTimeoutBufferTime = 2;

/**
 * Backbone Router Sequence Number.
 */
static constexpr uint8_t kBbrSequenceNumber = 100;

/**
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

void Test1_2_MATN_TC_4(void)
{
    /**
     * 5.10.4 MATN-TC-04: Removal of multicast listener by timeout expiry
     *
     * 5.10.4.1 Topology
     * - BR_1 (DUT)
     * - BR_2
     * - Router
     * - Host
     *
     * 5.10.4.2 Purpose & Description
     * The purpose of this test case is to verify that a Primary BBR can remove an entry from a Multicast Listeners
     *   Table, when the entry expires. The test case also verifies that the Primary BBR accepts a new registration to
     *   the previously expired multicast group.
     *
     * Spec Reference           | V1.2 Section
     * -------------------------|-------------
     * Multicast Listener Table | 5.10.4
     */

    Core         nexus;
    Node        &br1    = nexus.CreateNode();
    Node        &br2    = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &host   = nexus.CreateNode();
    Ip6::Address ma1;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("Router");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Topology formation - BR_1 (DUT). Boot DUT. Configure the MLR timeout value of the DUT to be "
        "MLR_TIMEOUT_MIN seconds.");

    /**
     * Step 0
     * - Device: BR_1 (DUT)
     * - Description: Topology formation - BR_1 (DUT). Boot DUT. Configure the MLR timeout value of the DUT to be
     *   MLR_TIMEOUT_MIN seconds.
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

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout     = kMlrTimeoutMin;
        config.mSequenceNumber = kBbrSequenceNumber;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }

    Log("Step 0 (cont.): Topology formation - BR_2, Router");

    /**
     * Step 0 (cont.)
     * - Device: N/A
     * - Description: Topology formation - BR_2, Router
     * - Pass Criteria:
     *   - N/A
     */
    router.Join(br1, Node::kAsFtd);
    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);
    {
        BackboneRouter::Config config;
        br2.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout     = kMlrTimeoutMin;
        config.mSequenceNumber = kBbrSequenceNumber;
        SuccessOrQuit(br2.Get<BackboneRouter::Local>().SetConfig(config));
    }

    nexus.AdvanceTime(kStabilizationTime * 10);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    const Ip6::Address *hostUla = &host.mInfraIf.FindMatchingAddress("fd00::/8");

    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("HOST_BACKBONE_ULA", hostUla->ToString().AsCString());

    Log("Step 1: Harness instructs the device to register multicast address, MA1");

    /**
     * Step 1
     * - Device: Router
     * - Description: Harness instructs the device to register multicast address, MA1
     * - Pass Criteria:
     *   - N/A
     *   - Note: Checks for BLMR.ntf are intentionally skipped.
     */
    SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kMlrRegistrationTime);

    // Force the entry to have 300s timeout in BR_1's table, overriding whatever the Router requested.
    SuccessOrQuit(br1.Get<BackboneRouter::Manager>().GetMulticastListenersTable().Add(
        ma1, TimerMilli::GetNow() + TimeMilli::SecToMsec(kMlrTimeoutMin)));

    Log("Step 2: Harness instructs the device to sends an ICMPv6 Echo (ping) Request packet to the multicast address, "
        "MA1.");

    /**
     * Step 2
     * - Device: Host
     * - Description: Harness instructs the device to sends an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 3: Automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
     *     encapsulated in an MPL packet.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 4: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and "
        "automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.");

    /**
     * Step 4
     * - Device: Router
     * - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
     *   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 5: Harness instructs the device to stop listening to the multicast address, MA1.");

    /**
     * Step 5
     * - Device: Router
     * - Description: Harness instructs the device to stop listening to the multicast address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    router.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 6: After (MLR_TIMEOUT_MIN+2) seconds, the DUT automatically multicasts MLDv2 message to deregister from "
        "multicast group MA1.");

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description: After (MLR_TIMEOUT_MIN+2) seconds, the DUT automatically multicasts MLDv2 message to deregister
     *   from multicast group MA1.
     * - Pass Criteria:
     *   - The DUT MUST multicast an MLDv2 message of type “Version 2 Multicast Listener Report” (see [RFC 3810] Section
     *     5.2). Where: Nr of Mcast Address Records (M): at least 1.
     *   - The Multicast Address Record [1] contains the following: Record Type: 3 (CHANGE_TO_INCLUDE_MODE), Number of
     *     Sources (N): 0, Multicast Address: MA1.
     *   - The message MUST occur after MLR_TIMEOUT_MIN+2) seconds [after what].
     *   - Note: Checks for MLDv2 are intentionally skipped.
     */
    nexus.AdvanceTime((kMlrTimeoutMin + kTimeoutBufferTime) * 1000);

    Log("Step 7: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast address, "
        "MA1.");

    /**
     * Step 7
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 8: Does not forward the ping request packet to its Thread Network.");

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description: Does not forward the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread
     *     Network.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 9: Harness instructs the device to register multicast address, MA1, at BR_1");

    /**
     * Step 9
     * - Device: Router
     * - Description: Harness instructs the device to register multicast address, MA1, at BR_1
     * - Pass Criteria:
     *   - N/A
     *   - Note: Checks for BLMR.ntf are intentionally skipped.
     */
    SuccessOrQuit(router.Get<Mle::Mle>().Start());
    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    if (!router.Get<Ip6::Netif>().IsMulticastSubscribed(ma1))
    {
        SuccessOrQuit(router.Get<Ip6::Netif>().SubscribeExternalMulticast(ma1));
    }
    nexus.AdvanceTime(kMlrRegistrationTime);

    Log("Step 10: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast address, "
        "MA1.");

    /**
     * Step 10
     * - Device: Host
     * - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet to the multicast
     *   address, MA1.
     * - Pass Criteria:
     *   - N/A
     */
    host.mInfraIf.SendEchoRequest(*hostUla, ma1, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(0);

    Log("Step 11: Automatically forwards the ping request packet to its Thread Network.");

    /**
     * Step 11
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the ping request packet to its Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA1, to its Thread Network
     *     encapsulated in an MPL packet.
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    Log("Step 12: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and "
        "automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.");

    /**
     * Step 12
     * - Device: Router
     * - Description: Receives the MPL packet containing an encapsulated ping request packet to MA1, sent by Host, and
     *   automatically unicasts an ICMPv6 Echo (ping) Reply packet back to Host.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kEchoReplyWaitTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test1_2_MATN_TC_4();
    printf("All tests passed\n");
    return 0;
}
