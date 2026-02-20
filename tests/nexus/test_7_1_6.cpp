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

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "thread/network_data_notifier.hpp"

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
static constexpr uint32_t kStabilizationTime = 30 * 1000;

/**
 * Time to allow Leader to detect Router removal, in milliseconds.
 */
static constexpr uint32_t kRouterRemovalDetectionTime = 720 * 1000;

/**
 * Timeout for ping response, in milliseconds.
 */
static constexpr uint32_t kPingTimeout = 30 * 1000;

/**
 * Test prefix.
 */
static constexpr char kPrefix[] = "2001:db8:1::/64";

/**
 * Delay for SED echo response, in milliseconds.
 */
static constexpr uint32_t kSedEchoResponseDelay = 180 * 1000;

void Test7_1_6(void)
{
    /**
     * 7.1.6 Network data propagation when BR Leaves the network, rejoins and updates server data
     *
     * 7.1.6.1 Topology
     * - Router_1 is configured as Border Router for prefix 2001:db8:1::/64.
     * - Router_2 is configured as Border Router for prefix 2001:db8:1::/64.
     * - MED_1 is configured to require complete network data.
     * - SED_1 is configured to request only stable network data.
     *
     * 7.1.6.2 Purpose & Description
     * The purpose of this test case is to verify that network data is properly updated when a server from the network
     *   leaves and rejoins.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Server Behavior  | 5.15.6       | 5.15.6
     */

    Core nexus;

    Node &dut     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    dut.SetName("DUT");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    dut.AllowList(router1);
    dut.AllowList(router2);
    dut.AllowList(med1);
    dut.AllowList(sed1);

    router1.AllowList(dut);
    router2.AllowList(dut);
    med1.AllowList(dut);
    sed1.AllowList(dut);

    dut.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());

    router1.Join(dut);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    router2.Join(dut);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    med1.Join(dut, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    sed1.Join(dut, Node::kAsSed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness configures the device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix = 2001:db8:1::/64 P_stable = 1 P_on_mesh = 1 P_slaac = 1 P_default = 1
     *   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border
     *     Router) to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;

        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_2");

    /**
     * Step 3: Router_2
     * - Description: Harness configures the device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_Prefix = 2001:db8:1::/64 P_stable = 0 P_on_mesh = 1 P_slaac = 1 P_default = 1
     *   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border
     *     Router) to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;

        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends a CoAP ACK frame to each of Router_1 and Router_2.
     * - Pass Criteria:
     *   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
     *   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader (DUT)");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically sends new network data to neighbors and rx-on-when idle Children (MED_1) via a
     *   multicast MLE Data Response to address FF02::1.
     * - Pass Criteria: The DUT MUST multicast MLE Data Response with the new information collected from Router_1 and
     *   Router_2, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field <incremented>
     *     - Stable Data Version field <incremented>
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1)
     *       - Two Border Router sub-TLVs
     *       - 6LoWPAN ID sub-TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST send MLE Child Update Request or Data Response to SED_1, which contains the stable
     *   Network Data and includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version numbers must be the same as the ones sent in the multicast data response in step 5
     *   - Network Data TLV
     *     - At least one TLV (Prefix 1) TLV, including:
     *       - Border Router sub-TLV (corresponding to Router_1)
     *       - 6LoWPAN ID sub-TLV
     *       - P_border_router_16 <0xFFFE>
     *   - Active Timestamp TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     * - Description: Harness silently powers-off Router_1 and waits 720 seconds to allow Leader (DUT) to detect the
     *   change.
     * - Pass Criteria: N/A.
     */

    router1.Reset();
    nexus.AdvanceTime(kRouterRemovalDetectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically detects removal of Router_1 and updates the Router ID Set accordingly.
     * - Pass Criteria:
     *   - The DUT MUST detect that Router_1 is removed from the network and update the Router ID Set.
     *   - The DUT MUST remove the Network Data section corresponding to Router_1 and increment the Data Version and
     *     Stable Data Version.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader (DUT)");

    /**
     * Step 9: Leader (DUT)
     * - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
     * - Pass Criteria: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast address (FF02::1),
     *   including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV
     *     - Router_1’s Network Data section MUST be removed
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Request or Data Response to SED_1, containing the updated
     *   Network Data:
     *   - Source Address TLV
     *   - Network Data TLV
     *   - Active Timestamp TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router_1");

    /**
     * Step 11: Router_1
     * - Description: Harness silently powers-up Router_1; it automatically begins the attach procedure.
     * - Pass Criteria: N/A.
     */

    dut.AllowList(router1);
    router1.AllowList(dut);
    router1.Join(dut, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically attaches Router_1 as a Child.
     * - Pass Criteria: The DUT MUST send MLE Child ID Response to Router_1, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Address16 TLV
     *   - Route64 TLV
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1) including:
     *       - Border Router sub-TLV corresponding to Router_2
     *       - 6LoWPAN ID sub-TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router_1");

    /**
     * Step 13: Router_1
     * - Description: Harness (re)configures the device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix = 2001:db8:1::/64 P_stable = 1 P_on_mesh = 1 P_slaac = 1 P_default = 1
     *   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border
     *     Router) to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */

    SuccessOrQuit(router1.Get<Mle::Mle>().SetRouterEligible(true));
    SuccessOrQuit(router1.Get<Mle::Mle>().BecomeRouter(Mle::kReasonTooFewRouters));
    nexus.AdvanceTime(kStabilizationTime);

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;

        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader (DUT)");

    /**
     * Step 14: Leader (DUT)
     * - Description: Automatically sends a CoAP ACK frame to Router_1.
     * - Pass Criteria: The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Leader (DUT)");

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_1,
     *   including the following fields:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version field <incremented>
     *     - Stable Version field <incremented>
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1) including:
     *       - Two Border Router sub-TLVs – corresponding to Router_1 and Router_2
     *       - 6LoWPAN ID sub-TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    /**
     * Step 16: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request or Data Response to SED_1, containing the
     *   stable Network Data and including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version numbers must be the same as those sent in the multicast data response in step 15
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1), including:
     *       - Border Router sub-TLV (corresponding to Router_1)
     *       - 6LoWPAN ID sub-TLV
     *       - P_border_router_16 <0xFFFE>
     *   - Active Timestamp TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1, SED_1");

    /**
     * Step 17: Router_1, SED_1
     * - Description: Harness verifies connectivity by sending ICMPv6 Echo Requests from Router_1 and SED_1 to the DUT
     *   Prefix_1 based address.
     * - Pass Criteria: The DUT MUST respond with ICMPv6 Echo Replies.
     */

    {
        const Ip6::Address &dutAddress = dut.FindMatchingAddress(kPrefix);

        router1.SendEchoRequest(dutAddress, 1);
        nexus.AdvanceTime(kPingTimeout);

        sed1.SendEchoRequest(dutAddress, 2);
        nexus.AdvanceTime(kSedEchoResponseDelay);
    }

    nexus.SaveTestInfo("test_7_1_6.json");
}

} /* namespace Nexus */
} /* namespace ot */

int main(void)
{
    ot::Nexus::Test7_1_6();
    printf("All tests passed\n");
    return 0;
}
