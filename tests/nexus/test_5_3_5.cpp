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

/**
 * @file
 *   This file implements the Nexus test 5.3.5 Routing - Link Quality.
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
static constexpr uint32_t kStabilizationTime = 20 * 1000;

/**
 * Time to advance for the routing cost to be updated, in milliseconds.
 */
static constexpr uint32_t kRouteUpdateWaitTime = 60 * 1000;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * RSSI values for different link qualities (assuming -100 dBm noise floor).
 */
static constexpr int8_t kRssiLinkQuality3 = -70;  /**< Margin 30 */
static constexpr int8_t kRssiLinkQuality2 = -85;  /**< Margin 15 */
static constexpr int8_t kRssiLinkQuality1 = -95;  /**< Margin 5 */
static constexpr int8_t kRssiLinkQuality0 = -110; /**< Margin < 0 (unusable) */

void Test5_3_5(void)
{
    /**
     * 5.3.5 Routing - Link Quality
     *
     * 5.3.5.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - Router_2
     * - Router_3
     *
     * 5.3.5.2 Purpose & Description
     * The purpose of this test case is to ensure that the DUT routes traffic properly when link qualities between the
     *   nodes are adjusted.
     *
     * Spec Reference                                   | V1.1 Section   | V1.3.0 Section
     * -------------------------------------------------|----------------|---------------
     * Routing Protocol / Full Thread Device Forwarding | 5.9 / 5.10.1.1 | 5.9 / 5.10.1.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();

    leader.SetName("LEADER");
    dut.SetName("DUT");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    leader.AllowList(dut);
    leader.AllowList(router2);

    dut.AllowList(leader);
    dut.AllowList(router2);
    dut.AllowList(router3);

    router2.AllowList(leader);
    router2.AllowList(dut);

    router3.AllowList(dut);

    /** Leader and Router 2 Link Quality 3 */
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(router2.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));
    SuccessOrQuit(router2.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));

    /** Router 1 (DUT) and Router 2 Link Quality 3 */
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(router2.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));
    SuccessOrQuit(router2.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));

    /** Router 1 (DUT) and Router 3 Link Quality 3 */
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(router3.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));
    SuccessOrQuit(router3.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    dut.Join(leader);
    router2.Join(leader);
    router3.Join(leader);

    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Harness");

    /**
     * Step 2: Harness
     * - Description: Modifies the link quality between the DUT and the Leader to be 3.
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));

    nexus.AdvanceTime(kRouteUpdateWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_3");

    /**
     * Step 3: Router_3
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
     * - Pass Criteria:
     *   - The ICMPv6 Echo Request MUST take the shortest path: Router_3 -> DUT -> Leader.
     *   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
     */
    router3.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Harness");

    /**
     * Step 4: Harness
     * - Description: Sets the link quality between the Leader and the DUT to 1.
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality1));
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality1));

    nexus.AdvanceTime(kRouteUpdateWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_3");

    /**
     * Step 5: Router_3
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
     * - Pass Criteria:
     *   - The ICMPv6 Echo Request MUST take the longer path: Router_3 -> DUT -> Router_2 -> Leader.
     *   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
     */
    router3.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Harness");

    /**
     * Step 6: Harness
     * - Description: Sets the link quality between the Leader and the DUT to 2.
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));

    nexus.AdvanceTime(kRouteUpdateWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_3");

    /**
     * Step 7: Router_3
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST have two paths with the same cost, and MUST prioritize sending to a direct neighbor:
     *     Router_3 -> DUT -> Leader.
     *   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
     */
    router3.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Harness");

    /**
     * Step 8: Harness
     * - Description: Sets the link quality between the Leader and the DUT to 0 (infinite).
     * - Pass Criteria: N/A
     */
    SuccessOrQuit(leader.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality0));
    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(leader.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality0));

    nexus.AdvanceTime(kRouteUpdateWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_3");

    /**
     * Step 9: Router_3
     * - Description: Harness instructs device to send an ICMPv6 Echo Request from to the Leader.
     * - Pass Criteria:
     *   - The ICMPv6 Echo Request MUST follow the longer path: Router_3 -> DUT -> Router_2 -> Leader.
     *   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
     */
    router3.SendEchoRequest(leader.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_3_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_5();
    printf("All tests passed\n");
    return 0;
}
