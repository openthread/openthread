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
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 30 * 1000;

/**
 * Time to advance for processing ICMPv6 Echo Request/Reply.
 */
static constexpr uint32_t kEchoProcessingTime = 2 * 1000;

void Test5_3_4(void)
{
    /**
     * 5.3.4 MTD EID-to-RLOC Map Cache
     *
     * 5.3.4.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - SED_1 (Attached to DUT)
     * - MED_1 (Attached to Leader)
     * - MED_2 (Attached to Leader)
     * - MED_3 (Attached to Leader)
     * - MED_4 (Attached to Leader)
     *
     * 5.3.4.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to maintain an EID-to-RLOC Map Cache for a
     *   Sleepy End Device child attached to it. Each EID-to-RLOC Set MUST support at least four non-link-local
     *   unicast IPv6 addresses.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * EID-to-RLOC Map Cache | 5.5          | 5.5
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &med2    = nexus.CreateNode();
    Node &med3    = nexus.CreateNode();
    Node &med4    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    sed1.SetName("SED_1");
    med1.SetName("MED_1");
    med2.SetName("MED_2");
    med3.SetName("MED_3");
    med4.SetName("MED_4");

    nexus.AdvanceTime(0);

    Node *meds[] = {&med1, &med2, &med3, &med4};

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    for (Node *med : meds)
    {
        leader.AllowList(*med);
    }

    router1.AllowList(leader);
    router1.AllowList(sed1);

    sed1.AllowList(router1);

    for (Node *med : meds)
    {
        med->AllowList(leader);
    }

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    sed1.Join(router1, Node::kAsSed);
    for (Node *med : meds)
    {
        med->Join(leader, Node::kAsMed);
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: SED_1");

    /**
     * Step 2: SED_1
     * - Description: Harness instructs device to send ICMPv6 Echo Requests to MED_1, MED_2, MED_3, and MED_4.
     * - Pass Criteria:
     *   - The DUT MUST generate an Address Query Request on SED_1’s behalf to find each node’s RLOC.
     *   - The Address Query Requests MUST be sent to the Realm-Local All-Routers address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV
     */

    for (uint16_t i = 0; i < 4; i++)
    {
        sed1.SendEchoRequest(meds[i]->Get<Mle::Mle>().GetMeshLocalEid(), i + 1);
        nexus.AdvanceTime(kEchoProcessingTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically sends Address Notification Messages with RLOC of MED_1, MED_2, MED_3, MED_4.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: SED_1");

    /**
     * Step 4: SED_1
     * - Description: Harness instructs the device to send ICMPv6 Echo Requests to MED_1, MED_2, MED_3 and MED_4.
     * - Pass Criteria:
     *   - The DUT MUST cache the addresses in its EID-to-RLOC set for its child SED_1.
     *   - The DUT MUST NOT send an Address Query during this step; If an address query message is sent, the test
     *     fails.
     *   - A ICMPv6 Echo Reply MUST be sent for each ICMPv6 Echo Request from SED_1.
     */

    for (uint16_t i = 0; i < 4; i++)
    {
        sed1.SendEchoRequest(meds[i]->Get<Mle::Mle>().GetMeshLocalEid(), i + 5);
        nexus.AdvanceTime(kEchoProcessingTime);
    }

    nexus.SaveTestInfo("test_5_3_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_4();
    printf("All tests passed\n");
    return 0;
}
