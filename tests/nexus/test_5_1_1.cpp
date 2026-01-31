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
 * This duration accounts for MLE attach process and ROUTER_SELECTION_JITTER.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to wait for ICMPv6 Echo Reply.
 */
static constexpr uint32_t kEchoResponseTime = 1000;

static void HandleEchoReply(void                *aContext,
                            otMessage           *aMessage,
                            const otMessageInfo *aMessageInfo,
                            const otIcmp6Header *aIcmpHeader)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    if (aIcmpHeader->mType == OT_ICMP6_TYPE_ECHO_REPLY)
    {
        *static_cast<bool *>(aContext) = true;
    }
}

static void SendAndVerifyEchoRequest(Core &aNexus, Node &aSender, Node &aReceiver, bool &aReceivedEchoReply)
{
    Message         *message = aSender.Get<Ip6::Icmp>().NewMessage();
    Ip6::MessageInfo messageInfo;

    VerifyOrQuit(message != nullptr);

    messageInfo.SetPeerAddr(aReceiver.Get<Mle::Mle>().GetLinkLocalAddress());
    messageInfo.SetHopLimit(64);

    aReceivedEchoReply = false;
    SuccessOrQuit(aSender.Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, 0x1234));

    aNexus.AdvanceTime(kEchoResponseTime);
    VerifyOrQuit(aReceivedEchoReply, "Echo Reply not received");
}

void Test5_1_1(void)
{
    /**
     * 5.1.1 Attaching
     *
     * 5.1.1.1 Topology
     * - Topology A
     * - Topology B
     *
     * 5.1.1.2 Purpose & Description
     * The purpose of this test case is to show that the DUT is able to both form and attach to a network.
     * This test case must be executed twice, first - where the DUT is a Leader and forms a network,
     * and second - where the DUT is a router and attaches to a network.
     *
     * Spec Reference          | V1.1 Section | V1.3.0 Section
     * ------------------------|--------------|---------------
     * Attaching to a Parent   | 4.7.1        | 4.5.1
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelInfo);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader forms network");

    /**
     * Step 1: Leader
     * - Description: Automatically transmits MLE advertisements.
     * - Pass Criteria:
     *   - Leader is sending properly formatted MLE Advertisements.
     *   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     * (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2-10: Router attaches to Leader and becomes a router");

    /**
     * Step 2: Router_1
     * - Description: Automatically begins the attach process by sending a multicast MLE Parent Request.
     *
     * Step 3: Leader
     * - Description: Automatically responds with a MLE Parent Response.
     *
     * Step 4: Router_1
     * - Description: Automatically responds to the MLE Parent Response by sending a MLE Child ID Request.
     *
     * Step 5: Leader
     * - Description: Automatically unicasts a MLE Child ID Response.
     *
     * Step 6: Router_1
     * - Description: Automatically sends an Address Solicit Request.
     *
     * Step 7: Leader
     * - Description: Automatically sends an Address Solicit Response.
     *
     * Step 8: Router_1
     * - Description: Automatically multicasts a Link Request Message (optional).
     *
     * Step 9: Leader
     * - Description: Automatically unicasts a Link Accept message (conditional).
     *
     * Step 10: Router_1
     * - Description: Automatically transmits MLE advertisements.
     */
    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Verify connectivity using ICMPv6 Echo");

    /**
     * Step 11: Leader Or Router_1 (not the DUT)
     * - Description: Harness verifies connectivity by instructing the reference device to send a ICMPv6 Echo Request to
     *   the DUT link-local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply
     */
    bool               routerReceivedEchoReply = false;
    Ip6::Icmp::Handler routerIcmpHandler(HandleEchoReply, &routerReceivedEchoReply);
    bool               leaderReceivedEchoReply = false;
    Ip6::Icmp::Handler leaderIcmpHandler(HandleEchoReply, &leaderReceivedEchoReply);

    // 1. Verify Leader as DUT: Router (Reference) sends Echo Request to Leader (DUT) Link-Local address
    SuccessOrQuit(router.Get<Ip6::Icmp>().RegisterHandler(routerIcmpHandler));

    Log("Step 11.1: Sending Echo Request from Router to Leader Link-Local: %s",
        leader.Get<Mle::Mle>().GetLinkLocalAddress().ToString().AsCString());
    SendAndVerifyEchoRequest(nexus, router, leader, routerReceivedEchoReply);
    Log("Leader (as DUT) responded with Echo Reply successfully");

    // 2. Verify Router as DUT: Leader (Reference) sends Echo Request to Router (DUT) Link-Local address
    SuccessOrQuit(leader.Get<Ip6::Icmp>().RegisterHandler(leaderIcmpHandler));

    Log("Step 11.2: Sending Echo Request from Leader to Router Link-Local: %s",
        router.Get<Mle::Mle>().GetLinkLocalAddress().ToString().AsCString());
    SendAndVerifyEchoRequest(nexus, leader, router, leaderReceivedEchoReply);
    Log("Router (as DUT) responded with Echo Reply successfully");

    nexus.SaveTestInfo("test_5_1_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_1();
    printf("All tests passed\n");
    return 0;
}
