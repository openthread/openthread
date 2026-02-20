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
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 5 * 1000;

/**
 * Child timeout value in seconds.
 */
static constexpr uint32_t kChildTimeout = 10;

/**
 * Time to wait for child timeout to expire.
 */
static constexpr uint32_t kChildTimeoutWaitTime = (kChildTimeout + 2) * 1000;

/**
 * Time to wait for ICMPv6 Echo response (Address Query).
 */
static constexpr uint32_t kEchoRequestWaitTime = 5 * 1000;

void Test5_1_2(void)
{
    /**
     * 5.1.2 Child Address Timeout
     *
     * 5.1.2.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - MED_1
     * - SED_1
     *
     * 5.1.2.2 Purpose & Description
     * The purpose of the test case is to verify that when the timer reaches the value of the Timeout TLV sent by the
     * Child, the Parent stops responding to Address Query on the Child's behalf.
     *
     * Spec Reference: Timing Out Children
     * V1.1 Section: 4.7.5
     * V1.3.0 Section: 4.6.3
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER_1");
    med.SetName("MED_1");
    sed.SetName("SED_1");

    nexus.AdvanceTime(0);

    // Use AllowList feature to restrict the topology.
    nexus.AllowLinkBetween(leader, router);
    nexus.AllowLinkBetween(router, med);
    nexus.AllowLinkBetween(router, sed);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Verify topology is formed correctly
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    med.Get<Mle::Mle>().SetTimeout(kChildTimeout);
    med.Join(router, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    sed.Get<Mle::Mle>().SetTimeout(kChildTimeout);
    sed.Join(router, Node::kAsSed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: MED_1, SED_1");

    /**
     * Step 2: MED_1, SED_1
     * - Description: Harness silently powers-off both devices and waits for the keep-alive timeout to expire
     * - Pass Criteria: N/A
     */
    med.Get<Mle::Mle>().Stop();
    sed.Get<Mle::Mle>().Stop();

    nexus.AdvanceTime(kChildTimeoutWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the Leader to send an ICMPv6 Echo Request to MED_1. As part of the process, the
     * Leader automatically attempts to perform address resolution by sending an Address Query Request
     * - Pass Criteria: N/A
     */
    leader.SendEchoRequest(med.Get<Mle::Mle>().GetMeshLocalEid(), 0x1234);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1 (DUT)");

    /**
     * Step 4: Router_1 (DUT)
     * - Description: Does not respond to Address Query Request
     * - Pass Criteria: The DUT MUST NOT respond with an Address Notification Message
     */
    nexus.AdvanceTime(kEchoRequestWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");

    /**
     * Step 6: Leader
     * - Description: Harness instructs the Leader to send an ICMPv6 Echo Request to SED_1. As part of the process, the
     * Leader automatically attempts to perform address resolution by sending an Address Query Request
     * - Pass Criteria: N/A
     */
    leader.SendEchoRequest(sed.Get<Mle::Mle>().GetMeshLocalEid(), 0x5678);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1 (DUT)");

    /**
     * Step 7: Router_1 (DUT)
     * - Description: Does not to Address Query Request
     * - Pass Criteria: The DUT MUST NOT respond with an Address Notification Message
     */
    nexus.AdvanceTime(kEchoRequestWaitTime);

    nexus.SaveTestInfo("test_5_1_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_2();
    printf("All tests passed\n");
    return 0;
}
