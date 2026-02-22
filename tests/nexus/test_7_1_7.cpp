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
static constexpr uint32_t kStabilizationTime = 60 * 1000;

/**
 * Network ID Timeout for Step 7.
 */
static constexpr uint32_t kNetworkIdTimeout = 50 * 1000;

/**
 * Wait time for Router_2 to start its own partition.
 */
static constexpr uint32_t kPartitionStartWaitTime = 120 * 1000;

/**
 * Prefix 1.
 */
static const char kPrefix1[] = "2001:db8:1::/64";

/**
 * Prefix 2.
 */
static const char kPrefix2[] = "2001:db8:2::/64";

void Test7_1_7(const char *aJsonFile)
{
    /**
     * 7.1.7 Network data updates - BR device rejoins network
     *
     * 7.1.7.1 Topology
     * - Leader (DUT)
     * - Router_1
     * - Router_2
     * - MED_1
     * - SED_1
     *
     * 7.1.7.2 Purpose & Description
     * The purpose of this test case is to verify that network data is properly updated when a server from the network
     *   leaves and rejoins.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Server Behavior  | 5.15.6       | 5.15.6
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
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

    /** Use AllowList to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    leader.AllowList(router2);
    router2.AllowList(leader);

    leader.AllowList(med1);
    med1.AllowList(leader);

    leader.AllowList(sed1);
    sed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader, Node::kAsFtd);
    router2.Join(leader, Node::kAsFtd);
    med1.Join(leader, Node::kAsMed);
    sed1.Join(leader, Node::kAsSed);

    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    /**
     * Set on Router_2 device:
     * - NETWORK_ID_TIMEOUT = 50s.
     * - generated Partition ID to min.
     */
    router2.Get<Mle::Mle>().SetNetworkIdTimeout(kNetworkIdTimeout / 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness configures the device with the following On-Mesh Prefix Set: Prefix 1: P_prefix =
     *   2001:db8:1::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1. Automatically sends a CoAP Server Data
     *   Notification message with the server's information (Prefix, Border Router) to the Leader:
     *   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
     *   - CoAP Payload: Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_2");

    /**
     * Step 3: Router_2
     * - Description: Harness configures the device with the following On-Mesh Prefix Set: Prefix 1: P_prefix =
     *   2001:db8:1::/64 P_stable=0 P_on_mesh=1 P_slaac=1 P_default=1. Automatically sends a CoAP Server Data
     *   Notification message with the server's information (Prefix, Border Router) to the Leader:
     *   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
     *   - CoAP Payload: Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends a CoAP ACK frame to each of Router_1 and Router_2.
     * - Pass Criteria:
     *   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_1.
     *   - The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader (DUT)");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically sends new network data to neighbors and rx-on-when idle Children (MED_1).
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_1 and
     *   Router_2, including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV.
     *     - At least one Prefix TLV (Prefix 1).
     *       - Stable Flag set.
     *       - Two Border Router sub-TLVs.
     *         - Border Router1 TLV: Stable Flag set.
     *         - Border Router2 TLV : Stable Flag not set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    /**
     * Step 6A: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, containing the stable Network
     *   Data
     *   and including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Network Data TLV.
     *     - At least one Prefix TLV (Prefix 1), including:
     *       - Stable Flag set.
     *       - Border Router sub-TLV (corresponding to Router_1).
     *         - Stable flag set.
     *         - P_border_router_16 <0xFFFE>.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable flag set.
     *   - Active Timestamp TLV.
     *   - Goto step 7.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_2");

    /**
     * Step 7: Router_2
     * - Description: Harness removes connectivity between Router_2 and the Leader (DUT), and waits ~50s.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kStabilizationTime);

    router2.UnallowList(leader);
    leader.UnallowList(router2);

    nexus.AdvanceTime(kPartitionStartWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_2");

    /**
     * Step 8: Router_2
     * - Description: After Router_2 starts its own partition, the harness modifies Router_2's network data information:
     *   - Removes the 2001:db8:1::/64 prefix.
     *   - Adds the 2001:db8:2::/64 prefix.
     *   - Prefix 2: P_prefix = 2001:db8:2::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1.
     * - Pass Criteria: N/A.
     */

    VerifyOrQuit(router2.Get<Mle::Mle>().IsLeader());

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        SuccessOrQuit(router2.Get<NetworkData::Local>().RemoveOnMeshPrefix(config.GetPrefix()));

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_2");

    /**
     * Step 9: Router_2
     * - Description: Harness enables connectivity between Router_2 and the Leader (DUT).
     * - Pass Criteria: N/A.
     */

    router2.AllowList(leader);
    leader.AllowList(router2);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_2");

    /**
     * Step 10: Router_2
     * - Description: Automatically reattaches to the Leader and sends a CoAP Server Data Notification message with the
     *   server's information (Prefix, Border Router) to the Leader:
     *   - CoAP Request URI: coap://[<leader address>]:MM/a/sd.
     *   - CoAP Payload: Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader (DUT)");

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically sends a CoAP ACK frame to Router_2.
     * - Pass Criteria: The DUT MUST send a CoAP ACK frame (2.04 Changed) to Router_2.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_2,
     *   including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
     *     - Prefix 1 TLV.
     *       - Stable Flag set.
     *       - Only one Border Router sub-TLV - corresponding to Router_1.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *     - Prefix 2 TLV.
     *       - Stable Flag set.
     *       - One Border Router sub-TLV - corresponding to Router_2.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader (DUT)");

    /**
     * Step 13A: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, containing the stable Network
     *   Data
     *   and including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Network Data TLV.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
     *     - Prefix 1 TLV.
     *       - Stable Flag set.
     *       - Border Router sub-TLV - corresponding to Router_1.
     *         - P_border_router_16 <0xFFFE>.
     *         - Stable flag set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *     - Prefix 2 TLV.
     *       - Stable Flag set.
     *       - Border Router sub-TLV - corresponding to Router_2.
     *         - P_border_router_16 <0xFFFE>.
     *         - Stable flag set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *   - Active Timestamp TLV.
     *   - Goto step 14.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1, SED_1");

    /**
     * Step 14: Router_1, SED_1
     * - Description: Harness verifies connectivity by sending ICMPv6 Echo Requests from Router_1 and SED_1 to the DUT
     *   Prefix_1 and Prefix_2-based addresses.
     * - Pass Criteria: The DUT MUST respond with ICMPv6 Echo Replies.
     */

    nexus.AdvanceTime(kStabilizationTime);

    {
        uint16_t id = 0x1234;

        router1.SendEchoRequest(leader.FindMatchingAddress(kPrefix1), id++);
        nexus.AdvanceTime(kStabilizationTime);
        router1.SendEchoRequest(leader.FindMatchingAddress(kPrefix2), id++);
        nexus.AdvanceTime(kStabilizationTime);
        sed1.SendEchoRequest(leader.FindMatchingAddress(kPrefix1), id++);
        nexus.AdvanceTime(kStabilizationTime);
        sed1.SendEchoRequest(leader.FindMatchingAddress(kPrefix2), id++);
        nexus.AdvanceTime(kStabilizationTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Router_2");

    /**
     * Step 15: Router_2
     * - Description: Harness removes the 2001:db8:2::/64 address from Router_2. Router_2 sends a CoAP Server Data
     *   Notification (SVR_DATA.ntf) with empty server data payload to the Leader:
     *   - CoAP Request URI: coap://[<leader RLOC or ALOC>]:MM/a/sd.
     *   - CoAP Payload: zero-length Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        SuccessOrQuit(router2.Get<NetworkData::Local>().RemoveOnMeshPrefix(config.GetPrefix()));
        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    /**
     * Step 16: Leader (DUT)
     * - Description: Automatically sends a CoAP Response to Router_2.
     * - Pass Criteria: The DUT MUST send a CoAP response (2.04 Changed) to Router_2.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Leader (DUT)");

    /**
     * Step 17: Leader (DUT)
     * - Description: Automatically sends new updated network data to neighbors and rx-on-when idle Children (MED_1).
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new information collected from Router_2,
     *   including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
     *     - Prefix 1 TLV.
     *       - Stable Flag set.
     *       - Only one Border Router sub-TLV - corresponding to Router_1.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *     - Prefix 2 TLV.
     *       - Stable Flag set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *       - compression flag set to 0.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Leader (DUT)");

    /**
     * Step 18A: Leader (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, containing the stable Network
     *   Data
     *   and including the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Network Data TLV.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2).
     *     - Prefix 1 TLV.
     *       - Stable Flag set.
     *       - Border Router sub-TLV - corresponding to Router_1.
     *         - P_border_router_16 <0xFFFE>.
     *         - Stable flag set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *     - Prefix 2 TLV.
     *       - Stable Flag set.
     *       - 6LoWPAN ID sub-TLV.
     *       - Stable Flag set.
     *       - Compression flag set to 0.
     *   - Active Timestamp TLV.
     *   - Goto step 19.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: End of test");

    /**
     * Step 19: End of test
     * - Description: End of test.
     * - Pass Criteria: N/A.
     */

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test7_1_7((argc > 2) ? argv[2] : "test_7_1_7.json");
    printf("All tests passed\n");
    return 0;
}
