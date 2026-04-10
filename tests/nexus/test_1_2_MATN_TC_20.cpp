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
 * Multicast Listener Registration minimum timeout, in seconds.
 */
static constexpr uint32_t kMlrTimeoutMin = 300;

/**
 * Multicast Listener Registration extended timeout, in seconds.
 */
static constexpr uint32_t kMlrTimeoutExtended = kMlrTimeoutMin + 30;

/**
 * Multicast address MA2 (site-local).
 */
static const char kMA2[] = "ff05::1234:777a:1";

void TestMatnTc20(void)
{
    /**
     * 5.10.16 MATN-TC-20: Automatic re-registration by Parent Router on behalf of MTD
     *
     * 5.10.16.1 Topology
     * - Router (DUT)
     * - MED
     * - BR_1
     * - BR_2
     *
     * 5.10.16.2 Purpose & Description
     * The purpose of this test case is to verify that a Parent Router handling a multicast registration on behalf of an
     *   MTD re-registers the multicast address on behalf of its child, before the MLR timeout expires.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Multicast        | 5.10.16
     */

    Core nexus;

    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();
    Node &br1    = nexus.CreateNode();
    Node &br2    = nexus.CreateNode();

    router.SetName("Router");
    med.SetName("MED");
    br1.SetName("BR_1");
    br2.SetName("BR_2");

    Ip6::Address ma2;
    SuccessOrQuit(ma2.FromString(kMA2));

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Topology formation - BR_1, BR_2. Topology formation - Router (DUT). The DUT is booted and added to "
        "the network. Topology formation - MED.");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - BR_1, BR_2. Topology formation - Router (DUT). The DUT is booted and added
     *   to the network. Topology formation - MED.
     * - Pass Criteria:
     *   - N/A
     */

    /** Use AllowList to specify links between nodes. */
    br1.AllowList(br2);
    br1.AllowList(router);

    br2.AllowList(br1);
    br2.AllowList(router);

    router.AllowList(br1);
    router.AllowList(br2);
    router.AllowList(med);

    med.AllowList(router);

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

    br2.Get<BorderRouter::InfraIf>().Init(2, true);
    br2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br2.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br2.Get<BackboneRouter::Local>().SetEnabled(true);

    router.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    med.Join(router, Node::kAsMed);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    Log("Step 2: Harness configures the value of the MLR timeout in the BBR Dataset of the device to be "
        "(MLR_TIMEOUT_MIN + 30) seconds and distribute the BBR Dataset.");

    /**
     * Step 2
     * - Device: BR_1
     * - Description: Harness configures the value of the MLR timeout in the BBR Dataset of the device to be
     *   (MLR_TIMEOUT_MIN + 30) seconds and distribute the BBR Dataset.
     * - Pass Criteria:
     *   - N/A
     */
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout          = kMlrTimeoutExtended;
        config.mReregistrationDelay = 1;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().AddService(BackboneRouter::Local::kForceRegistration));
    }

    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: Automatically distributes the new network data to MED.");

    /**
     * Step 3
     * - Device: Router (DUT)
     * - Description: Automatically distributes the new network data to MED.
     * - Pass Criteria:
     *   - None.
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 4: Harness instructs the device to register multicast address MA2 with its parent. The device unicasts "
        "an MLE Child Update Request to the DUT, where the payload includes a single TLV with single address as "
        "below: Address Registration TLV: MA2");

    /**
     * Step 4
     * - Device: MED
     * - Description: Harness instructs the device to register multicast address MA2 with its parent. The device
     *   unicasts an MLE Child Update Request to the DUT, where the payload includes a single TLV with single address
     *   as below: Address Registration TLV: MA2
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(med.Get<Ip6::Netif>().SubscribeExternalMulticast(ma2));
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 5: Automatically responds to the registration request.");

    /**
     * Step 5
     * - Device: Router (DUT)
     * - Description: Automatically responds to the registration request.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
     *     fields) this TLV with two addresses inside:
     *   - Address Registration TLV: ML-EID of MTD (M1g)
     *   - MA2
     */
    /** Handled by AdvanceTime in Step 4 */

    Log("Step 6: Automatically requests to register multicast address MA2 at BR_1");

    /**
     * Step 6
     * - Device: Router (DUT)
     * - Description: Automatically requests to register multicast address MA2 at BR_1
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains:
     *   - IPv6 Addresses TLV: MA2
     */
    /** Handled by AdvanceTime in Step 4 */

    Log("Step 7: Before MLR timeout, automatically requests to re-register multicast address, MA2, at BR_1.");

    /**
     * Step 7
     * - Device: Router (DUT)
     * - Description: Before MLR timeout, automatically requests to re-register multicast address, MA2, at BR_1.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains one TLV with at least one address as follows:
     *   - IPv6 Addresses TLV: MA2
     *   - The CoAP request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 4
     */
    nexus.AdvanceTime(kMlrTimeoutExtended * 1000);

    Log("Step 8: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.");

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    /** Handled by AdvanceTime in Step 7 */

    Log("Step 9: Before MLR timeout, automatically re-registers for multicast address MA2.");

    /**
     * Step 9
     * - Device: Router (DUT)
     * - Description: Before MLR timeout, automatically re-registers for multicast address MA2.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains one TLV with at least one address as follows:
     *   - IPv6 Addresses TLV: MA2
     *   - The request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 6,
     */
    nexus.AdvanceTime(kMlrTimeoutExtended * 1000);

    Log("Step 10: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.");

    /**
     * Step 10
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    /** Handled by AdvanceTime in Step 9 */

    Log("Step 11: Harness configures the value of the device MLR timeout to be MLR_TIMEOUT_MIN seconds and "
        "distributes the BBR Dataset.");

    /**
     * Step 11
     * - Device: BR_1
     * - Description: Harness configures the value of the device MLR timeout to be MLR_TIMEOUT_MIN seconds and
     *   distributes the BBR Dataset.
     * - Pass Criteria:
     *   - N/A
     */
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mMlrTimeout          = kMlrTimeoutMin;
        config.mReregistrationDelay = 1;
        config.mSequenceNumber++;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().AddService(BackboneRouter::Local::kForceRegistration));
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 12: Automatically recognizes the BBR Dataset update and re-registers MA2.");

    /**
     * Step 12
     * - Device: Router (DUT)
     * - Description: Automatically recognizes the BBR Dataset update and re-registers MA2.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains at least MA2:
     *   - IPv6 Addresses TLV: MA2
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 13: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.");

    /**
     * Step 13
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    /** Handled by AdvanceTime in Step 12 */

    Log("Step 14: Before MLR timeout, automatically re-registers for multicast address MA2.");

    /**
     * Step 14
     * - Device: Router (DUT)
     * - Description: Before MLR timeout, automatically re-registers for multicast address MA2.
     * - Pass Criteria:
     *   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     *   - Where the payload contains at least MA2:
     *   - IPv6 Addresses TLV: MA2
     *   - The request MUST be sent within MLR_TIMEOUT_MIN seconds of step 11,
     */
    nexus.AdvanceTime(500 * 1000);

    Log("Step 15: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.");

    /**
     * Step 15
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
     * - Pass Criteria:
     *   - N/A
     */
    /** Handled by AdvanceTime in Step 14 */

    nexus.AddTestVar("MA2", kMA2);
    nexus.SaveTestInfo("test_1_2_MATN_TC_20.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc20();
    printf("All tests passed\n");
    return 0;
}
