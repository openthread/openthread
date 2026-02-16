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
 * Time to advance for a node to join and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Wait period after leader reboot.
 */
static constexpr uint32_t kWaitPeriod = 250 * 1000;

/**
 * High partition weight.
 */
static constexpr uint8_t kHighWeight = 72;

/**
 * The identifier used for Echo Request.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

void Test5_5_3(void)
{
    /**
     * 5.5.3 Split and Merge: Branch with Child
     *
     *   5.5.3.1 Topology
     *     - Topology A: DUT, Router_1, MED_2, Router_2, MED_3
     *     - Topology B: Leader, Router_2, DUT, MED_2, MED_3
     *
     *   5.5.3.2 Purpose & Description
     *     The purpose of this test case is to show that the Router_1 will create a new partition once the Leader is
     *       removed from the network for a time period longer than the Leader timeout (120 seconds), and the network
     *       will merge back together once the Leader device is reintroduced to the network.
     *
     * Spec Reference            | V1.1 Section | V1.3.0 Section
     * --------------------------|--------------|---------------
     * Thread Network Partitions | 5.16         | 5.16
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &med2    = nexus.CreateNode();
    Node &med3    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med2.SetName("MED_2");
    med3.SetName("MED_3");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: All");

    /**
     * Step 0: All
     *   - Description: Topology formation.
     *   - Pass Criteria: N/A
     */

    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, router2);

    nexus.AllowLinkBetween(router1, med2);
    nexus.AllowLinkBetween(router2, med3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    med2.Join(router1, Node::kAsMed);
    med3.Join(router2, Node::kAsMed);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(med2.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med3.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Router_2");

    /**
     * Step 1: Router_2
     *   - Description: Harness configures the device to form new partitions with the high partition weight (72).
     *   - Pass Criteria: N/A
     */

    router2.Get<Mle::Mle>().SetLeaderWeight(kHighWeight);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader, Router_1 (DUT)");

    /**
     * Step 2: Leader, Router_1 (DUT)
     *   - Description: Automatically transmit MLE advertisements.
     *   - Pass Criteria:
     *     - The DUT MUST send MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All Nodes multicast
     *       address (FF02::1).
     *     - The following TLVs MUST be present in the MLE Advertisement:
     *       - Leader Data TLV
     *       - Route64 TLV
     *       - Source Address TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     *   - Description: Leader device is restarted.
     *     - If DUT=Leader and testing is manual, this is a UI pop-up box interaction.
     *     - Allowed Leader reboot time is 125 seconds (must be greater than Leader Timeout value [default 120
     *       seconds]).
     *     - Harness begins a 250 second wait period during which steps 4-13 are expected to happen automatically.
     *   - Pass Criteria:
     *     - For DUT = Leader: The DUT MUST stop sending MLE advertisements.
     */

    leader.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kWaitPeriod);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1");

    /**
     * Step 4: Router_1
     *   - Description: Automatically attempts to reattach to previous partition.
     *   - Pass Criteria:
     *     - For DUT = Router:
     *       - The DUT MUST attempt to reattach to its original partition by sending MLE Parent Requests to the
     *         Link-Local All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
     *       - The following TLVs MUST be present in the MLE Parent Request:
     *         - Challenge TLV
     *         - Mode TLV
     *         - Scan Mask TLV (value = 0xc0)
     *         - Version TLV
     *       - The DUT MUST make two separate attempts to reconnect to its current partition in this manner.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader");

    /**
     * Step 5: Leader
     *   - Description: Does not respond to MLE Parent Requests.
     *   - Pass Criteria:
     *     - For DUT = Leader: The DUT MUST NOT respond to the MLE Parent Requests.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     *   - Description: Automatically attempts to attach to any other Partition.
     *   - Pass Criteria:
     *     - For DUT = Router:
     *       - The DUT MUST attempt to attach to any other partition within range by sending a MLE Parent Request to
     *         the Link-Local All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
     *       - The following TLVs MUST be present and valid in the MLE Parent Request:
     *         - Challenge TLV
     *         - Mode TLV
     *         - Scan Mask TLV (value = 0x80)
     *         - Version TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     *   - Description: Automatically starts a new partition as Leader and begins transmitting MLE Advertisements.
     *   - Pass Criteria:
     *     - For DUT = Router:
     *       - The DUT MUST send MLE Advertisements with an IP Hop Limit of 255 to the Link-Local all-nodes multicast
     *         address (FF02::1).
     *       - The following TLVs MUST be present in the MLE Advertisements:
     *         - Source Address TLV
     *         - Leader Data TLV (Router_1 MUST choose a new and random initial Partition ID, VN_Version, and
     *           VN_Stable_version.)
     *         - Route64 TLV (DUT MUST choose a new and random initial ID sequence number and delete all previous
     *           information from its routing tables.)
     */

    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Original Leader device");

    /**
     * Step 8: Original Leader device
     *   - Description: Automatically reattaches to the network.
     *   - Pass Criteria:
     *     - For DUT = Leader:
     *       - The DUT MUST send properly formatted MLE Parent Requests to the Link-Local All-Routers multicast address
     *         with an IP Hop Limit of 255.
     *       - The following TLVs MUST be present and valid in the MLE Parent Request:
     *         - Challenge TLV
     *         - Mode TLV
     *         - Scan Mask TLV (If the DUT sends multiple Parent Requests, the first one MUST be sent only to All
     *           Routers; subsequent ones MAY be sent to routers and REEDS)
     *         - Version TLV
     */

    SuccessOrQuit(leader.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Original Leader device");

    /**
     * Step 9: Original Leader device
     *   - Description: Automatically sends MLE Child ID Request to Router_2.
     *   - Pass Criteria:
     *     - For DUT = Leader:
     *       - The DUT MUST unicast MLE Child ID Request to Router_2.
     *       - The following TLVs MUST be present in the Child ID Request:
     *         - Link-layer Frame Counter TLV
     *         - Mode TLV
     *         - Response TLV
     *         - Timeout TLV
     *         - TLV Request TLV, containing: Address16 TLV, Network Data TLV, Route64 TLV (optional)
     *         - Version TLV
     *         - MLE Frame Counter TLV (optional; MAY be omitted if the sender uses the same internal counter for both
     *           link-layer and MLE security)
     */

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Original Leader device");

    /**
     * Step 10: Original Leader device
     *   - Description: Automatically sends MLE Advertisements.
     *   - Pass Criteria:
     *     - For DUT = Leader:
     *       - The DUT MUST send MLE advertisements with an IP Hop Limit of 255 to the Link-Local All Nodes multicast
     *         address (FF02::1).
     *       - The following TLVs MUST be present in the Advertisement:
     *         - Leader Data TLV
     *         - Route64 TLV
     *         - Source Address TLV
     */

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router_1");

    /**
     * Step 11: Router_1
     *   - Description: Router_1 and Router_2 network partitions automatically merge.
     *   - Pass Criteria:
     *     - For DUT = Router:
     *       - The DUT MUST attach to Router_2’s network partition.
     *       - The DUT’s Address Solicit Request MUST be formatted as below:
     *         - CoAP Request URI: coap://<leader address>:MM/a/as
     *         - CoAP Payload:
     *           - MAC Extended Address TLV
     *           - Status TLV (value = 4 [PARENT_PARTITION_CHANGE])
     *           - RLOC16 TLV (optional)
     */

    nexus.AllowLinkBetween(leader, router1);

    nexus.AdvanceTime(kWaitPeriod);

    VerifyOrQuit(router1.Get<Mle::Mle>().GetLeaderData().GetPartitionId() ==
                 router2.Get<Mle::Mle>().GetLeaderData().GetPartitionId());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: MED_2");

    /**
     * Step 12: MED_2
     *   - Description: Automatically sends MLE Child Update Request to Router_1 at its keep-alive interval.
     *   - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router_1");

    /**
     * Step 13: Router_1
     *   - Description: Automatically responds with MLE Child Update Response.
     *   - Pass Criteria:
     *     - For DUT = Router:
     *       - The DUT MUST unicast MLE Child Update Response to MED_2, with the updated TLVs of the new partition.
     *       - The following TLVs MUST be present in the MLE Child Update Response:
     *         - Mode TLV
     *         - Leader Data TLV
     *         - Source Address TLV
     *         - Address Registration TLV (optional)
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: MED_2");

    /**
     * Step 14: MED_2
     *   - Description: Harness instructs the device to send an ICMPv6 Echo Request to MED_3.
     *   - Pass Criteria:
     *     - MED_2 MUST receive an ICMPv6 Echo Reply from MED_3, validating the network merge and the connectivity
     *       between the partitions.
     */

    med2.SendEchoRequest(med3.Get<Mle::Mle>().GetMeshLocalEid(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_5_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_3();
    printf("All tests passed\n");
    return 0;
}
