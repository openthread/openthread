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
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Leader power off time in milliseconds (must be greater than Leader Timeout [default 120s]).
 */
static constexpr uint32_t kLeaderPowerOffTime = 140 * 1000;

/**
 * Parent selection time in milliseconds.
 */
static constexpr uint32_t kParentSelectionTime = 10 * 1000;

/**
 * Time to advance for the Leader to rejoin and upgrade to a router.
 */
static constexpr uint32_t kRejoinTime = 250 * 1000;

void Test5_5_7(void)
{
    /**
     * 5.5.7 Split/Merge Routers: Three-way Separated
     *
     * 5.5.7.1 Topology
     * - Topology A
     * - Topology B
     *
     * 5.5.7.2 Purpose & Description
     * The purpose of this test case is to show that Router_1 will create a new partition once the Leader is removed
     *   from the network for a time period longer than the leader timeout (120 seconds), and the network will merge
     *   once the Leader is reintroduced to the network.
     *
     * Spec Reference             | V1.1 Section | V1.3.0 Section
     * ---------------------------|--------------|---------------
     * Thread Network Partitions  | 5.16         | 5.16
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");
    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, router2);
    nexus.AllowLinkBetween(leader, router3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    router3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader, Router_1");
    /**
     * Step 2: Leader, Router_1
     * - Description: Transmit MLE advertisements.
     * - Pass Criteria:
     *   - Devices MUST send properly formatted MLE Advertisements.
     *   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements:
     *     - Source Address TLV
     *     - Leader Data TLV
     *     - Route64 TLV
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");
    /**
     * Step 3: Leader
     * - Description: Power off Leader for 140 seconds.
     * - Pass Criteria: The Leader stops sending MLE advertisements.
     */
    leader.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_2, Router_3");
    /**
     * Step 4: Router_2, Router_3
     * - Description: Each router forms a partition with the lowest possible partition ID.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");
    /**
     * Step 5: Router_1
     * - Description: Automatically attempts to reattach to previous partition.
     * - Pass Criteria:
     *   - Router_1 MUST send MLE Parent Requests to the Link-Local All-Routers multicast address with an IP Hop
     *     Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (MUST have E and R flags set)
     *     - Version TLV
     *   - The Router MUST make two separate attempts to reconnect to its current Partition in this manner.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader");
    /**
     * Step 6: Leader
     * - Description: Does NOT respond to MLE Parent Requests.
     * - Pass Criteria: The Leader does not respond to the Parent Requests.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");
    /**
     * Step 7: Router_1
     * - Description: Automatically attempts to reattach to any partition.
     * - Pass Criteria:
     *   - Router_1 MUST attempt to reattach to any partition by sending MLE Parent Requests to the All-Routers
     *     multicast address with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV
     *     - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_1");
    /**
     * Step 8: Router_1
     * - Description: Automatically takes over leader role of a new Partition and begins transmitting MLE
     *   Advertisements.
     * - Pass Criteria:
     *   - Router_1 MUST send MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255, either to a Link-Local unicast address OR to
     *     the Link-Local All-Nodes multicast address (FF02::1).
     *   - The following TLVs MUST be present in the Advertisements:
     *     - Leader Data TLV (DUT MUST choose a new and random initial Partition ID, VN_Version, and
     *       VN_Stable_version.)
     *     - Route64 TLV (DUT MUST choose a new and random initial ID sequence number and delete all previous
     *       information from its routing tables.)
     *     - Source Address TLV
     */
    nexus.AdvanceTime(kLeaderPowerOffTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader");
    /**
     * Step 9: Leader
     * - Description: Automatically reattaches to network.
     * - Pass Criteria:
     *   - The Leader MUST send a properly formatted MLE Parent Request to the Link-Local All-Routers multicast
     *     address with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present and valid in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (active Routers) (If the DUT sends multiple Parent Requests)
     *     - Version TLV
     */
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_1");
    /**
     * Step 10: Router_1
     * - Description: Automatically sends MLE Parent Response.
     * - Pass Criteria:
     *   - Router_1 MUST send an MLE Parent Response.
     *   - The following TLVs MUST be present in the MLE Parent Response:
     *     - Challenge TLV
     *     - Connectivity TLV
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Link Margin TLV
     *     - Response TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional) (The MLE Frame Counter TLV MAY be omitted if the sender uses the same
     *       internal counter for both link-layer and MLE security)
     */
    nexus.AdvanceTime(kParentSelectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");
    /**
     * Step 11: Leader
     * - Description: Automatically sends MLE Child ID Request (to Router_1) and Address Solicit Request and rejoins
     *   network.
     * - Pass Criteria: The MLE Child ID Request and Address Solicit Request MUST be properly formatted (See 5.1.1
     *   Attaching for formatting).
     */
    nexus.AdvanceTime(kRejoinTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: All");
    /**
     * Step 12: All
     * - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: DUT (Router or Leader) MUST respond with a ICMPv6 Echo Reply.
     */
    nexus.AdvanceTime(kAttachToRouterTime);

    {
        Node *nodes[] = {&leader, &router1, &router2, &router3};

        for (Node *sender : nodes)
        {
            for (Node *receiver : nodes)
            {
                if (sender == receiver)
                {
                    continue;
                }

                nexus.SendAndVerifyEchoRequest(*sender, receiver->Get<Mle::Mle>().GetMeshLocalEid());
            }
        }
    }

    nexus.SaveTestInfo("test_5_5_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_7();
    printf("All tests passed\n");
    return 0;
}
