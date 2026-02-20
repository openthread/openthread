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

#include "common/as_core_type.hpp"
#include "common/encoding.hpp"
#include "mac/data_poll_sender.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/** Time to advance for a node to form a network and become leader, in milliseconds. */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/** Time to advance for a node to join as a child and upgrade to a router, in milliseconds. */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/** Time to advance for a node to join as a child. */
static constexpr uint32_t kAttachToChildTime = 10 * 1000;

/** Time to advance for the network to stabilize after routers have attached. */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/** Large payload size to ensure fragmentation. */
static constexpr uint16_t kLargePayloadSize = 200;

/** Poll period for SED in milliseconds. */
static constexpr uint32_t kSedPollPeriod = 200;

void Test5_3_2(void)
{
    /**
     * 5.3.2 Realm-Local Addressing
     *
     * 5.3.2.1 Topology
     * - Leader
     * - Router 1
     * - Router 2 (DUT)
     * - SED 1
     *
     * 5.3.2.2 Purpose & Description
     * The purpose of this test case is to validate the Realm-Local addresses that the DUT configures.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Realm-Local Scope| 5.2.3.2      | 5.2.1.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    dut.SetName("DUT");
    sed1.SetName("SED_1");

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(router1, dut);
    nexus.AllowLinkBetween(dut, sed1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    dut.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);

    sed1.Join(dut, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
    nexus.AdvanceTime(kAttachToChildTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT ML-EID.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    Log("Step 2: Leader");
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetMeshLocalEid());

    /**
     * Step 3: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the DUT ML-EID.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    Log("Step 3: Leader");
    nexus.SendAndVerifyEchoRequest(leader, dut.Get<Mle::Mle>().GetMeshLocalEid(), kLargePayloadSize);

    /**
     * Step 4: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All-Nodes
     *   multicast address (FF03::1).
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     *   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
     */
    Log("Step 4: Leader");
    nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllNodesMulticast());

    /**
     * Step 5: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
     *   All-Nodes multicast address (FF03::1).
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     *   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
     */
    Log("Step 5: Leader");
    nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllNodesMulticast(), kLargePayloadSize);

    /**
     * Step 6: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All-Routers
     *   multicast address (FF03::2).
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     *   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
     */
    Log("Step 6: Leader");
    nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllRoutersMulticast());

    /**
     * Step 7: Leader
     * - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
     *   All-Routers multicast address (FF03::2).
     * - Pass Criteria:
     *   - The DUT MUST respond with an ICMPv6 Echo Reply.
     *   - The DUT MUST NOT forward the ICMPv6 Echo Request to SED_1.
     */
    Log("Step 7: Leader");
    nexus.SendAndVerifyEchoRequest(leader, Ip6::Address::GetRealmLocalAllRoutersMulticast(), kLargePayloadSize);

    /**
     * Step 8: Leader
     * - Description: Harness instructs the device to send a Fragmented ICMPv6 Echo Request to the Realm-Local All
     *   Thread Nodes multicast address.
     * - Pass Criteria:
     *   - The Realm-Local All Thread Nodes multicast address MUST be a realm-local Unicast Prefix-Based Multicast
     *     Address [RFC 3306], with:
     *     - flgs set to 3 (P = 1 and T = 1)
     *     - scop set to 3
     *     - plen set to the Mesh Local Prefix length
     *     - network prefix set to the Mesh Local Prefix
     *     - group ID set to 1
     *   - The DUT MUST use IEEE 802.15.4 indirect transmissions to forward packet to SED_1.
     */
    Log("Step 8: Leader");
    {
        Ip6::Address multicastAddr;
        multicastAddr.Clear();
        multicastAddr.mFields.m8[0] = 0xff;
        multicastAddr.mFields.m8[1] = 0x33;
        multicastAddr.SetMulticastNetworkPrefix(dut.Get<Mle::Mle>().GetMeshLocalPrefix());
        multicastAddr.mFields.m32[3] = BigEndian::HostSwap32(1);
        nexus.SendAndVerifyEchoRequest(leader, multicastAddr, kLargePayloadSize);
    }

    nexus.SaveTestInfo("test_5_3_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_2();
    printf("All tests passed\n");
    return 0;
}
