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
 * Multicast address MA1 (admin-local).
 */
static const char kMA1[] = "ff04::1234:777a:1";

/**
 * Multicast address MA2 (site-local).
 */
static const char kMA2[] = "ff05::1234:777a:1";

/**
 * Multicast address MA3 (global).
 */
static const char kMA3[] = "ff0e::1234:777a:1";

/**
 * Multicast address ALL_MPL_FORWARDERS (site-local).
 */
static const char kAllMplForwarders[] = "ff05::2";

/**
 * Multicast address MA6 (link-local).
 */
static const char kMA6[] = "ff02::1234:777a:1";

void TestMatnTc1(void)
{
    /**
     * 5.10.1 MATN-TC-01: Default blocking of multicast from backbone
     *
     * 5.10.1.1 Topology
     * - BR_1 (DUT)
     * - Router
     *
     * 5.10.1.2 Purpose & Description
     * Verify that a Primary BBR by default blocks IPv6 multicast from the backbone to its Thread Network when no
     *   devices have registered for multicast groups.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Multicast        | 5.10.1
     */

    Core         nexus;
    Node        &br1    = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &host   = nexus.CreateNode();
    Ip6::Address ma1;
    Ip6::Address ma2;
    Ip6::Address ma3;
    Ip6::Address allMplForwarders;
    Ip6::Address ma6;

    br1.SetName("BR_1");
    router.SetName("Router");
    host.SetName("Host");

    SuccessOrQuit(ma1.FromString(kMA1));
    SuccessOrQuit(ma2.FromString(kMA2));
    SuccessOrQuit(ma3.FromString(kMA3));
    SuccessOrQuit(allMplForwarders.FromString(kAllMplForwarders));
    SuccessOrQuit(ma6.FromString(kMA6));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Topology - BR_1 (DUT), Router");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology - BR_1 (DUT), Router
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

    host.mInfraIf.Init(host);
    host.mInfraIf.AddAddress(host.mInfraIf.GetLinkLocalAddress());

    nexus.AdvanceTime(kStabilizationTime);

    // Add multicast variables to test info manually to ensure verify script sees them.
    nexus.AddTestVar("MA1", kMA1);
    nexus.AddTestVar("MA2", kMA2);
    nexus.AddTestVar("MA3", kMA3);
    nexus.AddTestVar("ALL_MPL_FORWARDERS", kAllMplForwarders);
    nexus.AddTestVar("MA6", kMA6);

    const struct
    {
        const Ip6::Address &mAddress;
        const char         *mDescription;
    } kMulticastTests[] = {
        {ma1, "multicast address MA1"},
        {ma2, "multicast address MA2 (site-local scope)"},
        {ma3, "multicast address MA3 (global scope)"},
        {allMplForwarders, "site-local scope multicast address ALL_MPL_FORWARDERS"},
        {ma6, "link local address MA6"},
    };

    for (uint8_t i = 0; i < GetArrayLength(kMulticastTests); i++)
    {
        const auto &test = kMulticastTests[i];
        uint8_t     step = i + 1;

        Log("Step %d: Harness instructs the device to send ICMPv6 Echo (ping) Request packet to the %s.", step,
            test.mDescription);
        host.mInfraIf.SendEchoRequest(host.mInfraIf.GetLinkLocalAddress(), test.mAddress, kEchoIdentifier,
                                      kEchoPayloadSize);
        nexus.AdvanceTime(kStabilizationTime);
        Log("Step %da: Does not forward the ping request packet to its Thread Network.", step);
    }

    nexus.SaveTestInfo("test_1_2_MATN_TC_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc1();
    printf("All tests passed\n");
    return 0;
}
