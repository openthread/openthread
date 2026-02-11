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

#include <stdarg.h>
#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Payload size for a standard ICMPv6 Echo Request.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Payload size for a fragmented ICMPv6 Echo Request.
 * A size of 200 bytes will result in multiple 802.15.4 fragments.
 */
static constexpr uint16_t kFragmentedEchoPayloadSize = 200;

void Test5_3_1(void)
{
    /**
     * 5.3.1 Link-Local Addressing
     *
     * 5.3.1.1 Topology
     * - Leader
     * - Router_1 (DUT)
     *
     * 5.3.1.2 Purpose & Description
     * The purpose of this test case is to validate the Link-Local addresses that the DUT auto-configures.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Link-Local Scope | 5.2.3.1      | 5.2.1.1
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Router_1 and Leader");

    /**
     * Step 1: Router_1 and Leader
     * - Description: Build the topology as described and begin the wireless sniffer
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    dut.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    Ip6::Address allNodesMulticast;
    allNodesMulticast.SetToLinkLocalAllNodesMulticast();

    Ip6::Address allRoutersMulticast;
    allRoutersMulticast.SetToLinkLocalAllRoutersMulticast();

    const Ip6::Address &dutAddr = dut.Get<Mle::Mle>().GetLinkLocalAddress();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader sends Echo Request to DUT LL64 address");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT’s MAC extended
     *   address-based Link-Local address
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, dutAddr, kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader sends fragmented Echo Request to DUT LL64 address");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to DUT’s MAC extended
     *   address-based Link-Local address
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, dutAddr, kFragmentedEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader sends Echo Request to All Nodes multicast address");

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All Nodes
     *   multicast address (FF02::1)
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, allNodesMulticast, kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader sends fragmented Echo Request to All Nodes multicast address");

    /**
     * Step 5: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local All Nodes
     *   multicast address (FF02::1)
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, allNodesMulticast, kFragmentedEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader sends Echo Request to All Routers multicast address");

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All-Routers
     *   multicast address (FF02::2)
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, allRoutersMulticast, kEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader sends fragmented Echo Request to All Routers multicast address");

    /**
     * Step 7: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local
     *   All-Routers multicast address (FF02::2)
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, allRoutersMulticast, kFragmentedEchoPayloadSize);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader sends Echo Request to All Thread Nodes multicast address");

    /**
     * Step 8: Leader
     * - Description: Harness instructs the device to send a ICMPv6 Echo Request to the Link-Local All Thread Nodes
     *   multicast address
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
     */
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetLinkLocalAllThreadNodesAddress(), kEchoPayloadSize);

    nexus.SaveTestInfo("test_5_3_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_1();
    printf("All tests passed\n");
    return 0;
}
