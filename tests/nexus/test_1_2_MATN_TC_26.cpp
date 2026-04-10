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
 *     names of its contributors may be endorse or promote products
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
 * Time to advance for the BBR selection to complete, in milliseconds.
 */
static constexpr uint32_t kBbrSelectionTime = 10 * 1000;

/**
 * Reregistration Delay in seconds.
 */
static constexpr uint16_t kReregistrationDelay = 5;

/**
 * MLR Timeout in seconds.
 */
static constexpr uint32_t kMlrTimeout = 600;

/**
 * Multicast address MA1.
 */
static const char kMA1[] = "ff04::1234:777a:1";

void TestMatnTc26(void)
{
    /**
     * 5.10.22 MATN-TC-26: Multicast registrations error handling by Thread Device
     *
     * 5.10.22.1 Topology
     * - BR_1: Test bed BR device operating as the Primary BBR
     * - TD (DUT): Device operating as a Thread device - FED or Router
     *
     * 5.10.22.2 Purpose & Description
     * The purpose of this test case is to verify that a Thread Device can correctly handle multicast registration
     *   errors, whether that is due to a BBR running out of resources or a BBR responding with general failure.
     *
     * Spec Reference   | V1.2 Section
     * -----------------|-------------
     * Multicast        | 5.10.1
     */

    Core         nexus;
    Node        &br1 = nexus.CreateNode();
    Node        &td  = nexus.CreateNode();
    Ip6::Address ma1;

    br1.SetName("BR_1");
    td.SetName("TD");

    SuccessOrQuit(ma1.FromString(kMA1));

    nexus.AddTestVar("MA1", kMA1);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation – BR_1. Topology formation – TD (DUT).");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - BR_1. Topology formation - TD (DUT). The DUT must be booted and joined to
     *   the network.
     * - Pass Criteria:
     * - N/A
     */
    br1.AllowList(td);
    td.AllowList(br1);

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
        config.mMlrTimeout = kMlrTimeout;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }

    td.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(td.Get<Mle::Mle>().IsFullThreadDevice());

    nexus.AdvanceTime(kBbrSelectionTime);
    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    Log("Step 1: Harness instructs the device to not update its multicast listener table, and to prepare a MLR.rsp "
        "error condition.");

    /**
     * Step 1
     * - Device: BR_1
     * - Description: Harness instructs the device to not update its multicast listener table, and to prepare a
     *   MLE.rsp error condition.
     * - Pass Criteria:
     * - N/A
     */
    br1.Get<BackboneRouter::Manager>().ConfigNextMulticastListenerRegistrationResponse(kMlrNoResources);

    Log("Step 2: The DUT must be configured to request registration of Multicast address MA1.");

    /**
     * Step 2
     * - Device: TD (DUT)
     * - Description: The DUT must be configured to request registration of Multicast address MA1.
     * - Pass Criteria:
     * - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     * - Where the payload contains: IPv6 Addresses TLV: MA1
     */
    SuccessOrQuit(td.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: Automatically responds to the Multicast registration with an error.");

    /**
     * Step 3
     * - Device: BR_1
     * - Description: Automatically responds to the Multicast registration with an error. Informative: Both the
     *   actions together: set_next_mlr_status_rsp() Unicasts a MLR.rsp CoAP response to TD: 2.04 Changed Where the
     *   payload contains: Status TLV: 4 [ST_MLR_NO_RESOURCES] IPv6 Addresses TLV: MA1
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 3b: Receives above MLR.rsp CoAP response from BR_1. Within <Reregistration Delay> seconds, "
        "automatically retries the registration.");

    /**
     * Step 3b
     * - Device: TD (DUT)
     * - Description: Receives above MLR.rsp CoAP response from BR_1. Within <Reregistration Delay> seconds,
     *   automatically retries the registration.
     * - Pass Criteria:
     * - Within <Reregistration Delay> seconds after receiving the MLR.rsp from BR_1, the DUT MUST retry the
     *   registration using the CoAP message as in step 1 pass criteria.
     */
    nexus.AdvanceTime(kReregistrationDelay * 1000);

    Log("Step 3c: Automatically updates its multicast listeners table and responds to the multicast registration.");

    /**
     * Step 3c
     * - Device: BR_1
     * - Description: Automatically updates its multicast listeners table and responds to the multicast registration.
     *   Unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains: Status TLV: 0
     *   [ST_MLR_SUCCESS]
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 4: Harness instructs the device to not update its multicast listener table, and to prepare a MLR.rsp "
        "error condition.");

    /**
     * Step 4
     * - Device: BR_1
     * - Description: Harness instructs the device to not update its multicast listener table, and to prepare a
     *   MLE.rsp error condition.
     * - Pass Criteria:
     * - N/A
     */
    br1.Get<BackboneRouter::Manager>().ConfigNextMulticastListenerRegistrationResponse(kMlrGeneralFailure);

    Log("Step 4 (cont.): Harness instructs the device to updates the network data (BBR Dataset).");

    /**
     * Step 4 (cont.)
     * - Device: BR_1
     * - Description: Harness instructs the device to updates the network data (BBR Dataset) as follows: BBR Sequence
     *   number: <New value = Previous Value +1>. The device then automatically starts MLE dissemination of the new
     *   dataset.
     * - Pass Criteria:
     * - N/A
     */
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mSequenceNumber++;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 5: After noticing the change in BBR Sequence number, the DUT automatically requests to register "
        "multicast address, MA1, at BR_1.");

    /**
     * Step 5
     * - Device: TD (DUT)
     * - Description: After noticing the change in BBR Sequence number, the DUT automatically requests to register
     *   multicast address, MA1, at BR_1.
     * - Pass Criteria:
     * - Within <Reregistration Delay> seconds of receiving the new network data, the DUT MUST unicasts an MLR.req
     *   CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     * - Where the payload contains: IPv6 Addresses TLV: MA1
     */

    Log("Step 7: Automatically responds to the DUT’s Multicast registration with another error.");

    /**
     * Step 7
     * - Device: BR_1
     * - Description: Automatically responds to the DUT’s Multicast registration with another error. Informative:
     *   Both the actions together: set_next_mlr_status_rsp() Automatically unicasts a MLR.rsp CoAP response to TD:
     *   2.04 Changed Where the payload contains: Status TLV: 6 [ST_MLR_GENERAL_FAILURE]
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 7b: The DUT receives above CoAP response from BR_1. Within <Reregistration Delay> seconds, it retries "
        "the registration.");

    /**
     * Step 7b
     * - Device: TD (DUT)
     * - Description: The DUT receives above CoAP response from BR_1. Within <Reregistration Delay> seconds, it
     *   retries the registration.
     * - Pass Criteria:
     * - Within <Reregistration Delay> seconds after receiving the MLR.rsp from BR_1, the DUT MUST retries the
     *   registration using the CoAP message as in step 5 of pass criteria.
     */
    nexus.AdvanceTime(kReregistrationDelay * 1000);

    Log("Step 7c: Automatically updates its multicast listeners table and responds to the multicast registration.");

    /**
     * Step 7c
     * - Device: BR_1
     * - Description: Automatically updates its multicast listeners table and responds to the multicast registration.
     *   Unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains: Status TLV: 0
     *   [ST_MLR_SUCCESS]
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 8: Harness instructs the device to Updates the network data (BBR dataset).");

    /**
     * Step 8
     * - Device: BR_1
     * - Description: Harness instructs the device to Updates the network data (BBR dataset) as follows: BBR Sequence
     *   number: <New value := Previous Value +1>. The device then automatically starts MLE dissemination of the new
     *   dataset.
     * - Pass Criteria:
     * - N/A
     */
    {
        BackboneRouter::Config config;
        br1.Get<BackboneRouter::Local>().GetConfig(config);
        config.mSequenceNumber++;
        SuccessOrQuit(br1.Get<BackboneRouter::Local>().SetConfig(config));
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 9: After noticing the change in BBR Sequence number, the DUT automatically registers for multicast "
        "address, MA1, at BR_1.");

    /**
     * Step 9
     * - Device: TD (DUT)
     * - Description: After noticing the change in BBR Sequence number, the DUT automatically registers for
     *   multicast address, MA1, at BR_1.
     * - Pass Criteria:
     * - Within <Reregistration Delay> seconds of receiving the new network data, the DUT MUST unicast an MLR.req
     *   CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
     * - Where the payload contains: IPv6 Addresses TLV: MA1
     */

    Log("Step 10: Automatically updates its multicast listener table.");

    /**
     * Step 10
     * - Device: BR_1
     * - Description: Automatically updates its multicast listener table.
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 11: Automatically responds to the multicast registration.");

    /**
     * Step 11
     * - Device: BR_1
     * - Description: Automatically responds to the multicast registration. Unicasts an MLR.rsp CoAP response to TD
     *   as follows: 2.04 changed Where the payload contains: Status TLV: 0 [ST_MLR_SUCCESS]
     * - Pass Criteria:
     * - N/A
     */

    Log("Step 12: Does not retry the registration within the next (0.5 * MLR Timeout).");

    /**
     * Step 12
     * - Device: TD (DUT)
     * - Description: Does not retry the registration within the next (0.5 * MLR Timeout).
     * - Pass Criteria:
     * - The DUT MUST NOT send an MLR.req message containing MA1 for at least 150 seconds.
     */
    nexus.AdvanceTime(150 * 1000);

    nexus.SaveTestInfo("test_1_2_MATN_TC_26.json");

    Log("TestMatnTc26 passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestMatnTc26();
    printf("All tests passed\n");
    return 0;
}
