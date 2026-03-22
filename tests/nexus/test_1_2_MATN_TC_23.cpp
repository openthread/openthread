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
 * MLR Timeout Minimum value in seconds.
 */
static constexpr uint32_t kMlrTimeoutMin = 300;

/**
 * MLR Timeout value to use in seconds.
 */
static constexpr uint32_t kMlrTimeout = kMlrTimeoutMin + 30;

/**
 * Multicast address MA1.
 */
static const char kMulticastAddr1[] = "ff04::1234:777a:1";

void TestMatnTc23(void)
{
    /**
     * 5.10.19 MATN-TC-23: Automatic re-registration by Thread Device
     *
     * 5.10.19.1 Topology
     * - BR_1
     * - BR_2
     * - TD (DUT)
     *
     * 5.10.19.2 Purpose & Description
     * The purpose of this test case is to verify that a Thread Device DUT can re-register its multicast address
     *   before the MLR timeout expires. See also MATN-TC-05 which tests the BBR function in a similar situation.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Multicast        | 5.10.19      | 5.10.19
     */

    Core         nexus;
    Node        &br1 = nexus.CreateNode();
    Node        &br2 = nexus.CreateNode();
    Node        &td  = nexus.CreateNode();
    Ip6::Address ma1;

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    td.SetName("TD");

    SuccessOrQuit(ma1.FromString(kMulticastAddr1));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - BR_1, BR_2, TD (DUT)
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation - BR_1, BR_2, TD (DUT)");

    br1.AllowList(br2);
    br1.AllowList(td);
    br2.AllowList(br1);
    br2.AllowList(td);
    td.AllowList(br1);
    td.AllowList(br2);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<BackboneRouter::Local>().SetEnabled(true);

    br2.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    br2.Get<BorderRouter::InfraIf>().Init(1, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    td.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(td.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());
    VerifyOrQuit(br2.Get<BackboneRouter::Local>().GetState() == BackboneRouter::Local::kStateSecondary);

    nexus.AddTestVar("MA1", kMulticastAddr1);
    {
        String<16> timeoutStr;

        timeoutStr.Append("%u", kMlrTimeout);
        nexus.AddTestVar("MLR_TIMEOUT", timeoutStr.AsCString());
    }

    /**
     * Step 1
     * - Device: BR_1
     * - Description: Harness instructs the device to configure the value of the MLR timeout in the BBR Dataset of
     *   BR_1 to be (MLR_TIMEOUT_MIN + 30) seconds and distribute the BBR Dataset in network data. Wait until TD
     *   has received the new network data.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR_1 configures MLR timeout and distributes BBR Dataset.");
    {
        BackboneRouter::Config bbrConfig;
        br1.Get<BackboneRouter::Local>().GetConfig(bbrConfig);
        bbrConfig.mMlrTimeout = kMlrTimeout;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(bbrConfig));
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().AddService(BackboneRouter::Local::kForceRegistration));
    }
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2
     * - Device: TD (DUT)
     * - Description: The DUT must be configured to register the multicast address, MA1. This causes the DUT to
     *   automatically send out MLR.req.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains one TLV only with at least one address MA1 inside (there may be more
     *     addresses): IPv6 Addresses TLV: the MA1
     */
    Log("Step 2: TD (DUT) registers multicast address, MA1.");
    SuccessOrQuit(td.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3
     * - Device: BR_1
     * - Description: Automatically responds with MLR.rsp with status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 3: BR_1 responds with MLR.rsp.");
    // Handled automatically.

    /**
     * Step 4
     * - Device: TD (DUT)
     * - Description: Within (MLR_TIMEOUT_MIN + 30) seconds of step 3, the DUT automatically re-registers for
     *   multicast address, MA1.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains one TLV only with at least one address MA inside: IPv6 Addresses TLV: the
     *     multicast address MA1
     *   - The MLR.req CoAP request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 3
     */
    Log("Step 4: TD (DUT) automatically re-registers for MA1.");
    nexus.AdvanceTime(kMlrTimeout * 1000);

    /**
     * Step 5
     * - Device: BR_1
     * - Description: Automatically responds with MLR.rsp and status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 5: BR_1 responds with MLR.rsp.");
    // Handled automatically.

    nexus.SaveTestInfo("test_1_2_MATN_TC_23.json", &br1);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc23();
    printf("All tests passed\n");
    return 0;
}
