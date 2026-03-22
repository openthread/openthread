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
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;

/**
 * Time to advance for the BBR registration process, in milliseconds.
 */
static constexpr uint32_t kBbrRegistrationTime = 10 * 1000;

/**
 * Time to advance for the MLR registration process, in milliseconds.
 */
static constexpr uint32_t kMlrRegistrationTime = 5 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingResponseTime = 5 * 1000;

/**
 * Leader weight to prevent the DUT from creating its own partition.
 */
static constexpr uint8_t kLeaderWeight = 72;

/**
 * Multicast address MA1 used in the test.
 */
static const char kMa1Address[] = "ff04::1234";

/**
 * Context for ICMP Echo Response.
 */
struct EchoContext
{
    uint16_t mIdentifier;
    bool     mReceived;
};

/**
 * Handler for ICMP Echo Response.
 */
static void HandleEchoResponse(void *aContext, const Ip6::Address &aSource, uint16_t aId, uint16_t aSequence)
{
    OT_UNUSED_VARIABLE(aSource);
    OT_UNUSED_VARIABLE(aSequence);

    EchoContext *context = static_cast<EchoContext *>(aContext);

    if (aId == context->mIdentifier)
    {
        context->mReceived = true;
    }
}

/**
 * ICMP Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * ICMP Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * ICMP Echo Request hop limit.
 */
static constexpr uint8_t kEchoHopLimit = 64;

/**
 * Backbone interface index.
 */
static constexpr uint32_t kBackboneIfIndex = 1;

void Test_1_2_BBR_TC_1(void)
{
    /**
     * 5.11.1 BBR-TC-01: Device MUST send its BBR dataset to Leader if none exists
     *
     * 5.11.1.1 Topology
     * - BR_1 (DUT): BR device operating as a BBR.
     * - Leader: Test bed bed device operating as a Thread Leader.
     * - Router: Test bed device operating as a Thread Router.
     * - Host: Test bed BR device operating as a non-Thread device.
     *
     * 5.11.1.2 Purpose & Description
     * The purpose of this test case is to verify that if no BBR dataset is present in a network (i.e. not sent around
     *   by Leader), then a BBR function MUST send its BBR dataset to the Leader.
     *
     * Spec Reference  | V1.2 Section
     * ----------------|-------------
     * BBR Dataset     | 5.21.3
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();
    Node &host   = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    dut.SetName("BR_1");
    host.SetName("HOST");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Topology formation - Leader, Router");

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - Leader, Router
     * - Pass Criteria: N/A
     */

    leader.Get<Mle::Mle>().SetLeaderWeight(kLeaderWeight);
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Topology addition - BR_1 (DUT)");

    /**
     * Step 1
     * - Device: BR_1 (DUT)
     * - Description: Topology addition - BR_1 (DUT). The DUT must be booted and configured join the Thread Network.
     * - Pass Criteria:
     *   - The DUT MUST attach to the Thread Network.
     */

    dut.Get<BorderRouter::InfraIf>().Init(kBackboneIfIndex, true);
    dut.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(dut.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    dut.Get<BackboneRouter::Local>().SetEnabled(true);

    dut.Join(leader);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader Automatically sends Network Data with no BBR Dataset.");

    /**
     * Step 2
     * - Device: Leader
     * - Description: Automatically sends Network Data with no BBR Dataset.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR_1 (DUT) sends its BBR Dataset to the Leader.");

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description: Checks received Network Data and automatically determines that it needs to send its BBR Dataset
     *   to the Leader to become primary BBR.
     * - Pass Criteria:
     *   - The DUT MUST unicast a SVR_DATA.ntf CoAP notification to Leader.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader responds to the Server Data notification.");

    /**
     * Step 4
     * - Device: Leader
     * - Description: Automatically responds to the Server Data notification.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader multicasts MLE Data Response message.");

    /**
     * Step 5
     * - Device: Leader
     * - Description: Automatically multicasts MLE Data Response message with BR_1's short address in the BBR Dataset,
     *   electing BR_1 as Primary.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kBbrRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: BR_1 (DUT) becomes the Primary BBR.");

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description: Automatically acknowledges the presence of its short address in the BBR Dataset and becomes the
     *   Primary BBR.
     * - Pass Criteria: None
     */

    VerifyOrQuit(dut.Get<BackboneRouter::Local>().GetState() == BackboneRouter::Local::kStatePrimary);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router registers the multicast address, MA1, at BR_1 (DUT).");

    /**
     * Step 7
     * - Device: Router
     * - Description: Harness instructs the device to register the multicast address, MA1, at BR_1 (DUT). Unicasts an
     *   MLR.req CoAP request to BR_1 (DUT).
     * - Pass Criteria: N/A
     */

    Ip6::Address ma1;
    SuccessOrQuit(ma1.FromString(kMa1Address));
    SuccessOrQuit(router.Get<ThreadNetif>().SubscribeExternalMulticast(ma1));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: BR_1 (DUT) internally registers the multicast address MA1.");

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description: Automatically internally registers the multicast address MA1 in its Multicast Listeners Table.
     * - Pass Criteria: None.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: BR_1 (DUT) responds to the multicast listener registration.");

    /**
     * Step 9
     * - Device: BR_1 (DUT)
     * - Description: Automatically responds to the multicast listener registration.
     * - Pass Criteria:
     *   - The DUT MUST unicast a MLR.rsp CoAP response to Router.
     */

    nexus.AdvanceTime(kMlrRegistrationTime);
    VerifyOrQuit(dut.Get<BackboneRouter::MulticastListenersTable>().Has(ma1));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: BR_1 (DUT) multicasts an MLDv2 message.");

    /**
     * Step 10
     * - Device: BR_1 (DUT)
     * - Description: Automatically multicasts an MLDv2 message to start listening to the group MA1.
     * - Pass Criteria:
     *   - The DUT MUST multicast an MLDv2 message of type "Version 2 Multicast Listener Report".
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Host sends ICMPv6 Echo Request packet to destination MA1.");

    /**
     * Step 11
     * - Device: Host
     * - Description: Harness instructs the device to sends ICMPv6 Echo (ping) Request packet to destination MA1.
     * - Pass Criteria: N/A
     */

    const Ip6::Address &hostAddr = host.mInfraIf.FindMatchingAddress("fd00::/8");

    nexus.AddTestVar("HOST_BACKBONE_ULA", hostAddr.ToString().AsCString());

    EchoContext echoContext;
    echoContext.mIdentifier = kEchoIdentifier;
    echoContext.mReceived   = false;
    host.mInfraIf.SetEchoReplyHandler(HandleEchoResponse, &echoContext);

    host.mInfraIf.SendEchoRequest(hostAddr, ma1, kEchoIdentifier, kEchoPayloadSize, kEchoHopLimit);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: BR_1 (DUT) determines that the multicast packet needs to be forwarded.");

    /**
     * Step 12
     * - Device: BR_1 (DUT)
     * - Description: Internally determines that the multicast packet to MA1 needs to be forwarded.
     * - Pass Criteria: None.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: BR_1 (DUT) forwards the multicast ping request packet.");

    /**
     * Step 13
     * - Device: BR_1 (DUT)
     * - Description: Automatically forwards the multicast ping request packet to Router.
     * - Pass Criteria: Not explicitly validated in this step.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router receives ping request packet and responds back to Host.");

    /**
     * Step 14
     * - Device: Router
     * - Description: Receives ping request packet and automatically responds back to Host via BR_1.
     * - Pass Criteria: Receives the ping packet sent by Host.
     */

    nexus.AdvanceTime(kPingResponseTime);
    VerifyOrQuit(echoContext.mReceived);

    nexus.SaveTestInfo("test_1_2_BBR_TC_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_2_BBR_TC_1();
    printf("All tests passed\n");
    return 0;
}
