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
