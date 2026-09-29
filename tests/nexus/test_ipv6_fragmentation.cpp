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

/** Time to advance for a node to form a network and become leader, in milliseconds. */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/** Time to advance for a node to join as a child and upgrade to a router, in milliseconds. */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/** Time to wait for ICMPv6 Echo response, in milliseconds. */
static constexpr uint32_t kEchoTimeout = 10 * 1000;

static bool     gForwardedPacketReceived = false;
static uint16_t gForwardedPacketLen      = 0;

static void MyCustomIp6ReceiveCallback(otMessage *aMessage, void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);
    Message    &message = *AsCoreTypePtr(aMessage);
    Ip6::Header header;
    if (header.ParseFrom(message) == kErrorNone)
    {
        if (header.GetNextHeader() == Ip6::kProtoUdp && message.GetLength() == 640)
        {
            gForwardedPacketReceived = true;
            gForwardedPacketLen      = message.GetLength();
            Log("MyCustomIp6ReceiveCallback: Received forwarded packet of length %u", gForwardedPacketLen);
        }
    }
}

static void SendGapFragment1(Node &aRouter, const Ip6::Address &aLeaderEid, uint32_t aIdentification)
{
    Message *message = aRouter.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Outer IPv6 Header
    Ip6::Header outerHeader;
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetPayloadLength(48); // 8 bytes (Fragment Header) + 40 bytes (Inner IPv6 Header)
    outerHeader.SetNextHeader(Ip6::kProtoFragment);
    outerHeader.SetHopLimit(64);
    outerHeader.SetSource(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    outerHeader.SetDestination(aLeaderEid);
    SuccessOrQuit(message->Append(outerHeader));

    // 2. Fragment Header
    Ip6::FragmentHeader fragmentHeader;
    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(Ip6::kProtoIp6);
    fragmentHeader.SetOffset(0);
    fragmentHeader.SetMoreFlag();
    fragmentHeader.SetIdentification(aIdentification);
    SuccessOrQuit(message->Append(fragmentHeader));

    // 3. Inner IPv6 Header (Payload of Fragment 1)
    Ip6::Header innerHeader;
    innerHeader.InitVersionTrafficClassFlow();
    innerHeader.SetPayloadLength(600); // 600 bytes inner payload to stay under MTU limit
    innerHeader.SetNextHeader(Ip6::kProtoUdp);
    innerHeader.SetHopLimit(64);
    innerHeader.SetSource(aLeaderEid);
    innerHeader.SetDestination(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    SuccessOrQuit(message->Append(innerHeader));

    // Set message priority/etc
    SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));

    // Send it
    SuccessOrQuit(aRouter.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message)));
}

static void SendGapFragment2(Node &aRouter, const Ip6::Address &aLeaderEid, uint32_t aIdentification)
{
    Message *message = aRouter.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    // 1. Outer IPv6 Header
    Ip6::Header outerHeader;
    outerHeader.InitVersionTrafficClassFlow();
    outerHeader.SetPayloadLength(8); // 8 bytes (Fragment Header)
    outerHeader.SetNextHeader(Ip6::kProtoFragment);
    outerHeader.SetHopLimit(64);
    outerHeader.SetSource(aRouter.Get<Mle::Mle>().GetMeshLocalEid());
    outerHeader.SetDestination(aLeaderEid);
    SuccessOrQuit(message->Append(outerHeader));

    // 2. Fragment Header
    Ip6::FragmentHeader fragmentHeader;
    fragmentHeader.Init();
    fragmentHeader.SetNextHeader(Ip6::kProtoIp6);
    fragmentHeader.SetOffset(80);   // 80 * 8 = 640 bytes (under MTU limit)
    fragmentHeader.ClearMoreFlag(); // M = 0
    fragmentHeader.SetIdentification(aIdentification);
    SuccessOrQuit(message->Append(fragmentHeader));

    // Set message priority/etc
    SuccessOrQuit(message->SetPriority(Message::kPriorityNormal));

    // Send it
    SuccessOrQuit(aRouter.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message)));
}

void TestIPv6Fragmentation(void)
{
    /**
     * Test IPv6 Fragmentation
     *
     * Topology:
     * - Leader
     * - Router
     *
     * Description:
     * The purpose of this test case is to validate IPv6 fragmentation and reassembly.
     * Large ICMPv6 Echo Requests (exceeding 1280 bytes MTU) are sent between nodes.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Form network");

    AllowLinkBetween(leader, router);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("Step 2: Large Echo Request from Leader to Router (1952 bytes payload)");
    // 1952 bytes payload + 8 bytes ICMP header + 40 bytes IPv6 header = 2000 bytes.
    // This exceeds the 1280 bytes MTU and triggers IPv6 fragmentation.
    nexus.SendAndVerifyEchoRequest(leader, router.Get<Mle::Mle>().GetMeshLocalEid(), 1952, 64, kEchoTimeout);

    Log("Step 3: Large Echo Request from Router to Leader (1831 bytes payload)");
    // 1831 bytes payload + 8 bytes ICMP header + 40 bytes IPv6 header = 1879 bytes.
    // This exceeds the 1280 bytes MTU and triggers IPv6 fragmentation.
    nexus.SendAndVerifyEchoRequest(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), 1831, 64, kEchoTimeout);

    Log("Step 4: Fragment Reassembly Gap Check Test");
    gForwardedPacketReceived = false;
    gForwardedPacketLen      = 0;

    // Register the custom receive callback on Router to intercept the forwarded packet.
    router.Get<Ip6::Ip6>().SetReceiveCallback(MyCustomIp6ReceiveCallback, nullptr);

    {
        uint32_t identification = 0x12345678;
        Log("Sending Fragment 1 (offset=0, M=1)");
        SendGapFragment1(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), identification);
        nexus.AdvanceTime(100); // Short wait

        Log("Sending Fragment 2 (offset=640, M=0)");
        SendGapFragment2(router, leader.Get<Mle::Mle>().GetMeshLocalEid(), identification);
        nexus.AdvanceTime(1000); // Wait for reassembly and forwarding
    }

    // Check if the packet was received (Expect no packet received in patched version!)
    VerifyOrQuit(!gForwardedPacketReceived, "Reassembly gap check failed: forwarded packet was received!");
    Log("Success: No forwarded packet was received. Reassembly gap is properly handled!");

    // Restore default receive callback on Router
    router.Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, &router);

    nexus.SaveTestInfo("test_ipv6_fragmentation.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIPv6Fragmentation();
    printf("All tests passed\n");
    return 0;
}
