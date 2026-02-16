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
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Network ID timeout for Router 3, in seconds.
 */
static constexpr uint8_t kRouter3NetworkIdTimeout = 55;

/**
 * Network ID timeout for Router 2 and Router 4, in seconds.
 */
static constexpr uint8_t kRouter2Router4NetworkIdTimeout = 140;

/**
 * Network ID timeout for the DUT, in seconds.
 */
static constexpr uint8_t kNetworkIdTimeout = 120;

/**
 * Maximum Partition ID value.
 */
static constexpr uint32_t kMaxPartitionId = 0xffffffff;

/**
 * Time to wait after Router 3 sends its first MLE Advertisement, in milliseconds.
 */
static constexpr uint32_t kWaitTimeAfterRouter3Adv = 10 * 1000;

void Test5_5_4_2(void)
{
    /**
     * 5.5.4 Split and Merge with Routers
     *
     * 5.5.4.2 Topology B (DUT as Router)
     *
     * Purpose & Description
     * The purpose of this test case is to show that:
     * - DUT device (R1) will join a new partition once the Leader is removed from the network for a time period
     *   longer than the leader timeout (120 seconds).
     * - If DUT device, before NETWORK_ID_TIMEOUT expires, hears MLE advertisements from a singleton Thread
     *   Partition (with higher partition id), it will consider its partition has a higher priority and will not
     *   try to join the singleton Thread partition.
     * - The network will merge once the Leader is reintroduced to the network.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Partitioning     | 4.8          | 4.6
     * Merging          | 4.9          | 4.7
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &router4 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");
    router4.SetName("ROUTER_4");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: All");

    /** Use AllowList to specify links between nodes. */
    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(leader, router2);
    nexus.AllowLinkBetween(router1, router3);
    nexus.AllowLinkBetween(router2, router4);

    /** Set NETWORK_ID_TIMEOUT of Router_3 to 55 seconds. */
    router3.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter3NetworkIdTimeout);

    /** Set NETWORK_ID_TIMEOUT of Router_2 and Router_4 to 140 seconds. */
    router2.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter2Router4NetworkIdTimeout);
    router4.Get<Mle::Mle>().SetNetworkIdTimeout(kRouter2Router4NetworkIdTimeout);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    router3.Join(router1);
    router4.Join(router2);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router4.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2: Leader, Router_1
     * - Description: Automatically transmit MLE advertisements.
     * - Pass Criteria:
     *   - Devices are sending properly formatted MLE Advertisements.
     *   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All-Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present:
     *     - Source Address TLV
     *     - Leader Data TLV
     *     - Route64 TLV.
     */
    Log("Step 2: Leader, Router_1");

    /**
     * Step 3: Router_3
     * - Description: Harness sets Partition ID on the device to maximum value. (This will take effect after
     *   partitioning and when Router_3 creates a new partition).
     * - Pass Criteria: N/A.
     */
    Log("Step 3: Router_3");
    router3.Get<Mle::Mle>().SetPreferredLeaderPartitionId(kMaxPartitionId);

    /**
     * Step 4: Leader
     * - Description: Harness powers the device down for 200 seconds.
     * - Pass Criteria: The device stops sending MLE advertisements.
     */
    Log("Step 4: Leader");
    leader.Get<Mle::Mle>().Stop();

    /**
     * Step 5: Router_3
     * - Description: After NETWORK_ID_TIMEOUT=55s expires, automatically forms new partition with maximum
     *   Partition ID, takes leader role and begins transmitting MLE Advertisements.
     * - Pass Criteria: N/A.
     */
    Log("Step 5: Router_3");
    nexus.AdvanceTime(static_cast<uint32_t>(kRouter3NetworkIdTimeout) * 1000);

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Does not try to join Router_3’s partition.
     * - Pass Criteria: During the ~10 seconds after the first MLE Advertisement is sent by Router_3 (with max
     *   Partition ID), the DUT MUST NOT send a Child ID Request frame – as an attempt to join Router_3’s partition.
     */
    Log("Step 6: Router_1 (DUT)");
    nexus.AdvanceTime(kWaitTimeAfterRouter3Adv);

    /**
     * Step 7: Router_1 (DUT)
     * - Description: After NETWORK_ID_TIMEOUT=120s expires, automatically attempts to reattach to previous
     *   partition.
     * - Pass Criteria:
     *   - The DUT MUST attempt to reattach to its original partition by sending MLE Parent Requests to the
     *     Link-Local All-Routers multicast address with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present:
     *     - Mode TLV
     *     - Challenge TLV
     *     - Scan Mask TLV (MUST have E and R flags set)
     *     - Version TLV
     *   - Router_1 MUST make two separate attempts to reconnect to its current Partition in this manner,.
     */
    Log("Step 7: Router_1 (DUT)");
    nexus.AdvanceTime((static_cast<uint32_t>(kNetworkIdTimeout - kRouter3NetworkIdTimeout) * 1000) -
                      kWaitTimeAfterRouter3Adv);

    /**
     * Step 8: Router_1 (DUT)
     * - Description: Automatically attaches to Router_3 partition.
     * - Pass Criteria: DUT attaches to the new partition by sending Parent Request, Child ID Request, and Address
     *   Solicit Request messages (See 5.1.1 Attaching for formatting).
     */
    Log("Step 8: Router_1 (DUT)");
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsAttached());

    /**
     * Step 9: Leader
     * - Description: Harness powers the device back up; it reattaches to the network.
     * - Pass Criteria:
     *   - Leader sends a properly formatted MLE Parent Request to the Link-Local All-Routers multicast address
     *     with an IP Hop Limit of 255.
     *   - The following TLVs MUST be present in the MLE Parent Request:
     *     - Mode TLV
     *     - Challenge TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV.
     */
    Log("Step 9: Leader");
    SuccessOrQuit(leader.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 10: Harness
     * - Description: Waits for Network to merge.
     * - Pass Criteria: N/A.
     */
    Log("Step 10: Harness");
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 11: Router_4
     * - Description: Harness instructs device to send an ICMPv6 ECHO Request to the DUT.
     * - Pass Criteria: Router_4 MUST get an ICMPv6 ECHO Reply from DUT.
     */
    Log("Step 11: Router_4");
    nexus.SendAndVerifyEchoRequest(router4, router1.Get<Mle::Mle>().GetMeshLocalEid());

    nexus.SaveTestInfo("test_5_5_4_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_5_4_2();
    printf("All tests passed\n");
    return 0;
}
