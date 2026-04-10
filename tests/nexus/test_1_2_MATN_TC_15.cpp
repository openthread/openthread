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

#include <initializer_list>

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
 * Time to advance for the Primary BBR transition to occur, in milliseconds.
 */
static constexpr uint32_t kPbbrTransitionTime = 300 * 1000;

/**
 * Multicast address MA1 (admin-local).
 */
static const char kMA1[] = "ff04::1234:777a:1";

void TestMatnTc15(void)
{
    /**
     * 5.10.11 MATN-TC-15: Change of Primary BBR triggers a re-registration
     *
     * 5.10.11.1 Topology
     * - BR_1
     * - BR_2
     * - Router
     * - TD (DUT)
     *
     * 5.10.11.2 Purpose & Description
     * The purpose of this test case is to verify that a Thread End Device detects a change of Primary BBR device and
     *   triggers a re-registration to its multicast groups.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Multicast        | 5.10.11      | 5.10.11
     */

    Core         nexus;
    Node        &br1    = nexus.CreateNode();
    Node        &br2    = nexus.CreateNode();
    Node        &router = nexus.CreateNode();
    Node        &dut    = nexus.CreateNode();
    Ip6::Address ma1;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router.SetName("Router");
    dut.SetName("TD");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation – BR_1, BR_2, Router. Topology formation - TD (DUT). Boot the DUT. Configure the "
        "DUT to register multicast address MA1.");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation – BR_1, BR_2, Router. Topology formation - TD (DUT). Boot the DUT. Configure
     *   the DUT to register multicast address MA1.
     * - Pass Criteria:
     *   - N/A
     */

    br1.AllowList(br2);
    br1.AllowList(router);

    br2.AllowList(br1);
    br2.AllowList(router);

    router.AllowList(br1);
    router.AllowList(br2);
    router.AllowList(dut);

    dut.AllowList(router);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    for (Node *br : {&br1, &br2})
    {
        br->Get<BorderRouter::InfraIf>().Init(1, true);
        br->Get<BorderRouter::RoutingManager>().Init();
        SuccessOrQuit(br->Get<BorderRouter::RoutingManager>().SetEnabled(true));
        br->Get<BackboneRouter::Local>().SetEnabled(true);
        br->Get<BackboneRouter::Local>().SetRegistrationJitter(1);
    }

    br2.Join(br1, Node::kAsFtd);
    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    dut.Join(router, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());

    SuccessOrQuit(dut.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kStabilizationTime);

    // Add multicast variables to test info manually to ensure verify script sees them.
    nexus.AddTestVar("MA1", kMA1);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(!br2.Get<BackboneRouter::Local>().IsPrimary());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Harness instructs the device to power down. Harness waits for BR_2 to become the Primary BBR.");

    /**
     * Step 1
     * - Device: BR_1
     * - Description: Harness instructs the device to power down. Harness waits for BR_2 to become the Primary BBR.
     * - Pass Criteria:
     *   - N/A
     */
    br1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kPbbrTransitionTime);

    for (int i = 0; i < 10; i++)
    {
        if (br2.Get<BackboneRouter::Local>().IsPrimary())
        {
            break;
        }
        nexus.AdvanceTime(kStabilizationTime);
    }
    VerifyOrQuit(br2.Get<BackboneRouter::Local>().IsPrimary());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Automatically detects the Primary BBR change and registers for multicast address, MA1, at BR_2.");

    /**
     * Step 2
     * - Device: TD (DUT)
     * - Description: Automatically detects the Primary BBR change and registers for multicast address, MA1, at BR_2.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request as follows: coap://[<BR_2 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains: IPv6 Addresses TLV: MA1
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Automatically forwards the registration request to BR_2.");

    /**
     * Step 3
     * - Device: Router
     * - Description: Automatically forwards the registration request to BR_2.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Automatically unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed. Where the payload "
        "contains: Status TLV: 0 [ST_MLR_SUCCESS].");

    /**
     * Step 4
     * - Device: BR_2
     * - Description: Automatically unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed. Where the payload
     *   contains: Status TLV: 0 [ST_MLR_SUCCESS].
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Automatically forwards the response to TD.");

    /**
     * Step 5
     * - Device: Router
     * - Description: Automatically forwards the response to TD.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Receives the CoAP registration response.");

    /**
     * Step 6
     * - Device: TD (DUT)
     * - Description: Receives the CoAP registration response.
     * - Pass Criteria:
     *   - The DUT MUST receive the CoAP response.
     */
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_1_2_MATN_TC_15.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc15();
    printf("All tests passed\n");
    return 0;
}
