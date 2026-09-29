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

void TestIPv6Recursion(void)
{
    /**
     * Test IPv6 Recursion
     *
     * Topology:
     * - Leader
     *
     * Description:
     * The purpose of this test case is to validate that both Ip6::HandleDatagram
     * and lowpan compression enforce their respective kMaxRecursionDepth limit
     * to prevent unbounded stack recursion/excessive recursive stack usage.
     */

    Core  nexus;
    Node &leader = nexus.CreateNode();

    leader.SetName("LEADER");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.Form();
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Ip6::Address selfAddress = leader.Get<Mle::Mle>().GetMeshLocalEid();

    // Test Case 1: Recursion depth 3 (acceptable limit <= 4)
    {
        Log("Test Case 1: Send nested IPv6 packet with depth 3 (within limit)");
        Message *message = leader.Get<Ip6::Ip6>().NewMessage();
        VerifyOrQuit(message != nullptr);

        // Construct 3 nested headers:
        // - i = 2 (innermost): NH = kProtoNone, PayloadLen = 0
        // - i = 1: NH = kProtoIp6, PayloadLen = 40
        // - i = 0 (outermost): NH = kProtoIp6, PayloadLen = 80
        for (int i = 0; i < 3; i++)
        {
            Ip6::Header header;
            header.InitVersionTrafficClassFlow();
            header.SetSource(selfAddress);
            header.SetDestination(selfAddress);
            if (i == 2)
            {
                header.SetNextHeader(Ip6::kProtoNone);
                header.SetPayloadLength(0);
            }
            else
            {
                header.SetNextHeader(Ip6::kProtoIp6);
                header.SetPayloadLength((2 - i) * sizeof(Ip6::Header));
            }
            SuccessOrQuit(message->Append(header));
        }

        Error error = leader.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error != kErrorDrop);
        Log("Test Case 1: Passed successfully!");
    }

    // Test Case 2: Recursion depth 6 (exceeds limit of 4 in Ip6::HandleDatagram)
    {
        Log("Test Case 2: Send nested IPv6 packet with depth 6 (exceeds limit)");
        Message *message = leader.Get<Ip6::Ip6>().NewMessage();
        VerifyOrQuit(message != nullptr);

        // Construct 6 nested headers:
        // - i = 5 (innermost): NH = kProtoNone, PayloadLen = 0
        // - i = 0 to 4: NH = kProtoIp6, PayloadLen = (5 - i) * 40
        for (int i = 0; i < 6; i++)
        {
            Ip6::Header header;
            header.InitVersionTrafficClassFlow();
            header.SetSource(selfAddress);
            header.SetDestination(selfAddress);
            if (i == 5)
            {
                header.SetNextHeader(Ip6::kProtoNone);
                header.SetPayloadLength(0);
            }
            else
            {
                header.SetNextHeader(Ip6::kProtoIp6);
                header.SetPayloadLength((5 - i) * sizeof(Ip6::Header));
            }
            SuccessOrQuit(message->Append(header));
        }

        Error error = leader.Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(message));
        VerifyOrQuit(error == kErrorDrop);
        Log("Test Case 2: Passed successfully (packet with depth 6 was dropped with kErrorDrop)!");
    }

    // Test Case 3: Direct call to Lowpan::Compress with depth 6 nested packet
    {
        Log("Test Case 3: Call Lowpan::Compress with depth 6 nested packet (should cap and fall back gracefully)");
        Message *message = leader.Get<Ip6::Ip6>().NewMessage();
        VerifyOrQuit(message != nullptr);

        // Construct 6 nested headers (similar to Test Case 1)
        for (int i = 0; i < 6; i++)
        {
            Ip6::Header header;
            header.InitVersionTrafficClassFlow();
            header.SetSource(selfAddress);
            header.SetDestination(selfAddress);
            if (i == 5)
            {
                header.SetNextHeader(Ip6::kProtoNone);
                header.SetPayloadLength(0);
            }
            else
            {
                header.SetNextHeader(Ip6::kProtoIp6);
                header.SetPayloadLength((5 - i) * sizeof(Ip6::Header));
            }
            SuccessOrQuit(message->Append(header));
        }

        Mac::Addresses macAddrs;
        macAddrs.mSource.SetShort(leader.Get<Mac::Mac>().GetShortAddress());
        macAddrs.mDestination.SetShort(leader.Get<Mac::Mac>().GetShortAddress());

        uint8_t      frameBuffer[512];
        FrameBuilder frameBuilder;
        frameBuilder.Init(frameBuffer, sizeof(frameBuffer));

        // This should succeed (return kErrorNone) because it will cap recursion at depth 4
        // and emit the remaining nested headers inline as opaque payload.
        Error error = leader.Get<Lowpan::Lowpan>().Compress(*message, macAddrs, frameBuilder);
        VerifyOrQuit(error == kErrorNone);
        Log("Test Case 3: Passed successfully (returned kErrorNone without excessive stack usage)!");
        message->Free();
    }

    nexus.SaveTestInfo("test_ipv6_recursion.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIPv6Recursion();
    printf("All tests passed\n");
    return 0;
}
