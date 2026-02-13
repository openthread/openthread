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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Leader reboot time in milliseconds (must be greater than Leader Timeout [default 120s]).
 */
static constexpr uint32_t kLeaderRebootTime = 150 * 1000;

/**
 * Parent selection time in milliseconds.
 */
static constexpr uint32_t kParentSelectionTime = 10 * 1000;

/**
 * Child ID exchange time in milliseconds.
 */
static constexpr uint32_t kChildIdExchangeTime = 5 * 1000;

/**
 * Address solicitation time in milliseconds.
 */
static constexpr uint32_t kAddressSolicitationTime = 200 * 1000;

void Test5_5_2(void)
{
    /**
     * 5.5.2 Leader Reboot > timeout (3 nodes)
     *
     * 5.5.2.1 Topology
     * - Leader
     * - Router_1
     * - MED (attached to Router_1)
     *
     * 5.5.2.2 Purpose & Description
     * The purpose of this test case is to show that the Router will become the leader of a new partition when the
     *   Leader is restarted, and remains rebooted longer than the leader timeout, and when the Leader is brought back
     *   it reattaches to the Router.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Losing           | 5.16.1       | 5.16.1
     *   Connectivity   |              |
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    med1.SetName("MED_1");

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(med1);
    med1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med1.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2: Leader, Router_1
     * - Description: Automatically transmit MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in the Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */
    Log("Step 2: Leader, Router_1");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3: Leader
     * - Description: Restart Leader.
     *   - If DUT=Leader and testing is manual, this is a UI pop-up box interaction.
     *   - Allowed Leader reboot time is 125 seconds (must be greater than Leader Timeout value [default 120
     *     seconds]).
     * - Pass Criteria:
     *   - For DUT = Leader: The DUT MUST stop sending MLE advertisements.
     */
    Log("Step 3: Leader");

    leader.Reset();

    /**
     * Step 4: Router_1
     * - Description: Automatically attempts to reattach to partition.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST attempt to reattach to its original partition by sending MLE Parent Requests
     *     to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (The E and R flags MUST be set)
     *     - Version TLV
     *   - The DUT MUST make two separate attempts to reconnect to its current Partition in this manner.
     *
     * Step 5: Leader
     * - Description: Does not respond to MLE Parent Requests.
     * - Pass Criteria:
     *   - For DUT = Leader: The DUT MUST NOT respond to the MLE Parent Requests.
     *
     * Step 6: Router_1
     * - Description: Automatically attempts to attach to any other Partition.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST attempt to attach to any other Partition within range by sending a MLE Parent
     *     Request.
     *   - The MLE Parent Request MUST be sent to the All-Routers multicast address (FF02::2) with an IP Hop Limit of
     *     255.
     *   - The following TLVs MUST be present and valid in the MLE Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV
     *     - Version TLV
     *
     * Step 7: Router_1
     * - Description: Automatically takes over Leader role of a new Partition and begins transmitting MLE
     *   Advertisements.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST send MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255.
     *   - MLE Advertisements MUST be sent to a Link-Local unicast address OR to the Link-Local All Nodes multicast
     *     address (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisement:
     *     - Leader Data TLV: The DUT MUST choose a new and random initial Partition ID, VN_Version, and
     *       VN_Stable_version.
     *     - Route64 TLV: The DUT MUST choose a new and random initial ID sequence number and delete all previous
     *       information from its routing table.
     *     - Source Address TLV
     */
    Log("Step 4: Router_1");
    Log("Step 5: Leader");
    Log("Step 6: Router_1");
    Log("Step 7: Router_1");

    nexus.AdvanceTime(kLeaderRebootTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());

    /**
     * Step 8: Router_1
     * - Description: The MED automatically sends MLE Child Update to Router_1. Router_1 automatically responds with
     *   MLE Child Update Response.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST respond with an MLE Child Update Response, with the updated TLVs of the new
     *     partition.
     *   - The following TLVs MUST be present in the MLE Child Update Response:
     *     - Leader Data TLV
     *     - Mode TLV
     *     - Source Address TLV
     *     - Address Registration TLV (optional)
     */
    Log("Step 8: Router_1");

    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med1.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    /**
     * Step 9: Leader
     * - Description: Automatically reattaches to network.
     * - Pass Criteria:
     *   - For DUT = Leader: The DUT MUST send properly formatted MLE Parent Requests to the All-Routers multicast
     *     address (FF02:2) with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present and valid in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (If the DUT sends multiple Parent Requests, the first one MUST be sent ONLY to All
     *       Routers; subsequent Parent Requests MAY be sent to All Routers and REEDS)
     *     - Version TLV
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     */
    Log("Step 9: Leader");

    leader.Join(router1);

    /**
     * Step 10: Router_1
     * - Description: Automatically sends MLE Parent Response.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST send an MLE Parent Response.
     *   - The following TLVs MUST be present in the MLE Parent Response:
     *     - Connectivity TLV
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Link Margin TLV
     *     - Response TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional; MAY be omitted if the sender uses the same internal counter for both
     *       link-layer and MLE security)
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     */
    Log("Step 10: Router_1");

    nexus.AdvanceTime(kParentSelectionTime);

    /**
     * Step 11: Leader
     * - Description: Automatically sends MLE Child ID Request.
     * - Pass Criteria:
     *   - For DUT = Leader: The following TLVs MUST be present in the MLE Child ID Request:
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV: Address16 TLV, Network Data, and/or Route64 TLV (optional)
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional; MAY be omitted if the sender uses the same internal counter for both
     *       link-layer and MLE security)
     *   - A REED MAY request a Route64 TLV as an aid in determining whether or not it should become an active
     *     Router.
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     *
     * Step 12: Router_1
     * - Description: Automatically sends MLE Child ID Response.
     * - Pass Criteria:
     *   - For DUT = Router: The following TLVs MUST be present in the MLE Child ID Response:
     *     - Address16 TLV
     *     - Leader Data TLV
     *     - Source Address TLV
     *     - Network Data TLV (provided if requested in MLE Child ID Request)
     *     - Route64 TLV (provided if requested in MLE Child ID Request)
     */
    Log("Step 11: Leader");
    Log("Step 12: Router_1");

    nexus.AdvanceTime(kChildIdExchangeTime);

    /**
     * Step 13: Leader
     * - Description: Automatically sends Address Solicit Request.
     * - Pass Criteria:
     *   - For DUT = Leader: The Address Solicit Request message MUST be properly formatted:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/as
     *     - CoAP Payload:
     *       - MAC Extended Address TLV
     *       - RLOC16 TLV (optional)
     *       - Status TLV
     *
     * Step 14: Router_1
     * - Description: Automatically sends Address Solicit Response.
     * - Pass Criteria:
     *   - For DUT = Router: The Address Solicit Response message MUST be properly formatted:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload:
     *       - Status TLV (value = Success)
     *       - RLOC16 TLV
     *       - Router Mask TLV
     */
    Log("Step 13: Leader");
    Log("Step 14: Router_1");

    nexus.AdvanceTime(kAddressSolicitationTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsRouter());

    /**
     * Step 15: Leader
     * - Description: Optionally sends a multicast Link Request.
     * - Pass Criteria:
     *   - For DUT = Leader: The DUT MAY send a multicast Link Request message.
     *   - If sent, the following TLVs MUST be present in the Link Request Message:
     *     - Challenge TLV
     *     - Leader Data TLV
     *     - Request TLV: RSSI
     *     - Source Address TLV
     *     - Version TLV
     *
     * Step 16: Router_1
     * - Description: Conditionally (automatically) sends a unicast Link Accept.
     * - Pass Criteria:
     *   - For DUT = Router: If the Leader in the prior step sent a multicast Link Request, the DUT MUST send a unicast
     *     Link Accept Message to the Leader.
     *   - If sent, the following TLVs MUST be present in the Link Accept message:
     *     - Leader Data TLV
     *     - Link-layer Frame Counter TLV
     *     - Link Margin TLV
     *     - Response TLV
     *     - Source Address TLV
     *     - Version TLV
     *     - Challenge TLV (optional)
     *     - MLE Frame Counter TLV (optional)
     */
    Log("Step 15: Leader");
    Log("Step 16: Router_1");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 17: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the Router_1 link local address.
     * - Pass Criteria:
     *   - For DUT = Router: The DUT MUST respond with an ICMPv6 Echo Reply.
     */
    Log("Step 17: All");

    med1.SendEchoRequest(router1.Get<Mle::Mle>().GetLinkLocalAddress(), 0);
    leader.SendEchoRequest(med1.Get<Mle::Mle>().GetLinkLocalAddress(), 0);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_5_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_2();
    printf("All tests passed\n");
    return 0;
}
