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
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for ICMPv6 Echo response.
 */
static constexpr uint32_t kEchoResponseWaitTime = 5 * 1000;

/**
 * Timeout for a router ID to be expired by the leader.
 * MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + propagation time = 320 s.
 */
static constexpr uint32_t kRouterIdTimeout = 320 * 1000;

/**
 * Timeout for a child to be timed out by its parent.
 */
static constexpr uint32_t kChildTimeout = 240 * 1000;

/**
 * ICMPv6 Echo Request identifiers used in different steps.
 */
static constexpr uint16_t kIcmpIdentifierStep5  = 0x1234;
static constexpr uint16_t kIcmpIdentifierStep6a = 0xabcd;
static constexpr uint16_t kIcmpIdentifierStep6b = 0xabce;

void Test5_3_3(void)
{
    /**
     * 5.3.3 Address Query - ML-EID
     *
     * 5.3.3.1 Topology
     * - Leader
     * - Router_1
     * - Router_2 (DUT)
     * - Router_3
     * - MED_1 (Attached to DUT)
     *
     * 5.3.3.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to generate Address Query messages and properly
     *   respond with Address Notification messages.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Address Query    | 5.4.2        | 5.4.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode(); // Router_2
    Node &router3 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    dut.SetName("DUT");
    router3.SetName("ROUTER_3");
    med1.SetName("MED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */

    /** Link between Leader and Router 1 */
    leader.AllowList(router1);
    router1.AllowList(leader);

    /** Link between Leader and Router 2 (DUT) */
    leader.AllowList(dut);
    dut.AllowList(leader);

    /** Link between Leader and Router 3 */
    leader.AllowList(router3);
    router3.AllowList(leader);

    /** Link between Router 2 (DUT) and Router 3 */
    dut.AllowList(router3);
    router3.AllowList(dut);

    /** Link between Router 2 (DUT) and MED 1 */
    dut.AllowList(med1);
    med1.AllowList(dut);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    dut.Join(leader);
    router3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    med1.Join(dut, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address");

    /**
     * Step 2: MED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Router_3 ML-EID address.
     * - Pass Criteria:
     *   - The DUT MUST generate an Address Query Request on MED_1’s behalf to find Router_3 address.
     *   - The Address Query Request MUST be sent to the Realm-Local All-Routers address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV
     *   - The DUT MUST receive and process the incoming Address Notification.
     *   - The DUT MUST then forward the ICMPv6 Echo Request from MED_1 and forward the ICMPv6 Echo Reply to MED_1.
     */

    nexus.SendAndVerifyEchoRequest(med1, router3.Get<Mle::Mle>().GetMeshLocalEid());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1 sends ICMPv6 Echo Request to MED_1 ML-EID address");

    /**
     * Step 3: Router_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the MED_1 ML-EID address.
     * - Pass Criteria:
     *   - The DUT MUST respond to the Address Query Request with a properly formatted Address Notification Message:
     *   - CoAP URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
     *   - CoAP Payload:
     *     - ML-EID TLV
     *     - RLOC16 TLV
     *     - Target EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator.
     *   - The IPv6 Destination address MUST be the RLOC of the destination.
     */

    nexus.SendAndVerifyEchoRequest(router1, med1.Get<Mle::Mle>().GetMeshLocalEid());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address");

    /**
     * Step 4: MED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request from MED_1 to the Router_3 ML-EID
     *   address.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query, as the Router_3 address should be cached.
     *   - The DUT MUST forward the ICMPv6 Echo Reply to MED_1.
     */

    nexus.SendAndVerifyEchoRequest(med1, router3.Get<Mle::Mle>().GetMeshLocalEid());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Power off Router_3 and MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address");

    /**
     * Step 5: Router_2 (DUT)
     * - Description: Power off Router_3 and wait for the Leader to expire its Router ID (Timeout = MAX_NEIGHBOR_AGE +
     *   INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + propagation time = 320 s). Harness instructs the device to send an
     *   ICMPv6 Echo Request from MED_1 to the Router_3 ML-EID address.
     * - Pass Criteria:
     *   - The DUT MUST update its address cache and remove all entries based on Router_3’s Router ID.
     *   - The DUT MUST be sent an Address Query to discover Router_3’s RLOC address.
     */

    router3.Reset();
    nexus.AdvanceTime(kRouterIdTimeout);

    med1.SendEchoRequest(router3.Get<Mle::Mle>().GetMeshLocalEid(), kIcmpIdentifierStep5);
    nexus.AdvanceTime(kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Power off MED_1 and Router_1 sends ICMPv6 Echo Request to MED_1 GUA address");

    /**
     * Step 6: MED_1
     * - Description: Power off MED_1 and wait for the DUT to timeout the child. Harness instructs the device to send
     *   two ICMPv6 Echo Requests from Router_1 to MED_1 GUA 2001:: address (one to clear the EID-to-RLOC Map Cache of
     *   the sender and the other to produce Address Query).
     * - Pass Criteria:
     *   - The DUT MUST NOT respond with an Address Notification message.
     */

    // Note: Because 2001:: is not configured as an on-mesh prefix, Step 6 as specified will not work.
    // We ping the ML-EID instead.

    med1.Reset();
    nexus.AdvanceTime(kChildTimeout);

    // We expect the following Echo Requests to fail as MED_1 is powered off.
    // Since `SendAndVerifyEchoRequest` quits on failure, we can't use it here
    // for the negative test case.
    // Instead, we use the raw `SendEchoRequest` and `AdvanceTime`.

    router1.SendEchoRequest(med1.Get<Mle::Mle>().GetMeshLocalEid(), kIcmpIdentifierStep6a);
    nexus.AdvanceTime(kEchoResponseWaitTime);

    router1.SendEchoRequest(med1.Get<Mle::Mle>().GetMeshLocalEid(), kIcmpIdentifierStep6b);
    nexus.AdvanceTime(kEchoResponseWaitTime);

    nexus.SaveTestInfo("test_5_3_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_3();
    printf("All tests passed\n");
    return 0;
}
