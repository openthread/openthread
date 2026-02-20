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
 * Timeout for a router ID to be expired by the leader.
 * MAX_NEIGHBOR_AGE (100s) + INFINITE_COST_TIMEOUT (90s) + ID_REUSE_DELAY (100s) +
 * ROUTER_SELECTION_JITTER (120s) + NETWORK_ID_TIMEOUT (120s) + propagation time (90s) = 620 s.
 */
static constexpr uint32_t kRouterIdTimeout = 620 * 1000;

/**
 * Time to advance for CoAP and MLE Data Response, in milliseconds.
 */
static constexpr uint32_t kDataPropagationTime = 20 * 1000;

/**
 * Time to advance for short intervals between steps.
 */
static constexpr uint32_t kShortIntervalTime = 20 * 1000;

void Test5_6_6(void)
{
    /**
     * 5.6.6 Network data expiration
     *
     * 5.6.6.1 Topology
     * - Router_1 is configured as Border Router.
     * - MED_1 is configured to require complete network data.
     * - SED_1 is configured to request only stable network data.
     *
     * 5.6.6.2 Purpose and Description
     * The purpose of this test case is to verify that network data is properly updated when deleting a prefix or
     * removing a server from the network.
     *
     * Spec Reference                                     | V1.1 Section | V1.3.0 Section
     * ---------------------------------------------------|--------------|---------------
     * Thread Network Data / Network Data and Propagation | 5.13 / 5.15  | 5.13 / 5.15
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    const char *kPrefix1 = "2001::/64";
    const char *kPrefix2 = "2002::/64";
    const char *kPrefix3 = "2003::/64";

    /**
     * - Use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - Leader (DUT) and Router 1
     * - Leader (DUT) and MED 1
     * - Leader (DUT) and SED 1
     */
    leader.AllowList(router1);
    leader.AllowList(med1);
    leader.AllowList(sed1);

    router1.AllowList(leader);
    med1.AllowList(leader);
    sed1.AllowList(leader);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure the topology is formed correctly.
     * - Pass Criteria: N/A.
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    sed1.Join(leader, Node::kAsSed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(1000));

    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness configures Router_1 as Border Router with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
     *   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
     *   - Prefix 3: P_Prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0.
     *   - Automatically sends a CoAP Server Data Notification frame with the server’s information to the DUT:
     *     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd.
     *     - CoAP Payload: Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */
    {
        NetworkData::OnMeshPrefixConfig config;

        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        SuccessOrQuit(config.GetPrefix().FromString(kPrefix3));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = false;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1");

    /**
     * Step 3: Router_1
     * - Description: Harness instructs the device to remove Prefix 3 from its Prefix Set. Automatically sends a CoAP
     *   Server Data Notification frame with the server’s information to the DUT:
     *   - CoAP Request URI: coap://[<DUT address>]:MM/a/sd.
     *   - CoAP Payload: Thread Network Data TLV.
     * - Pass Criteria: N/A.
     */
    {
        Ip6::Prefix prefix;

        SuccessOrQuit(prefix.FromString(kPrefix3));
        SuccessOrQuit(router1.Get<NetworkData::Local>().RemoveOnMeshPrefix(prefix));

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends a CoAP Response to Router_1.
     * - Pass Criteria: The DUT MUST send a 2.04 Changed CoAP response to Router_1.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader (DUT)");

    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically multicasts new network information to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information collected from
     *   Router_1 including:
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV.
     *     - At least three Prefix TLVs (Prefix 1, 2 and 3).
     *     - The Prefix 1 and Prefix 2 TLVs MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV.
     *     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <Compression flag = 0>.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: MED_1");

    /**
     * Step 6: MED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to MED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Address Registration TLV (Echoes back the addresses the Child has configured).
     *   - Mode TLV.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Leader (DUT) Note: Depending on the DUT’s device implementation, two different behavior paths (A, B) are
     * allowable for transmitting the new stable network data to SED_1:
     * - Path A: Notification via MLE Child Update, steps 8A-9.
     * - Path B: Notification via MLE Data Response, steps 8B-9.
     *
     * Step 8A: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
     *   Update Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV:.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 3).
     *     - The Prefix 2 TLV MUST NOT be included.
     *     - The Prefix 1 TLV MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV: P_border_router_16 <value =
     *       0xFFFE>.
     *     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <compression flag set to 0>.
     *   - Active Timestamp TLV.
     *   - Goto Step 9.
     *
     * Step 8B: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
     *   Response.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *     - Data Version field <incremented>.
     *     - Stable Data Version field <incremented>.
     *   - Network Data TLV:.
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 3).
     *     - The Prefix 2 TLV MUST NOT be included.
     *     - The Prefix 1 TLV MUST include: 6LoWPAN ID sub-TLV, Border Router sub-TLV: P_border_router_16 <value =
     *       0xFFFE>.
     *     - The Prefix 3 TLV MUST include: 6LoWPAN ID sub-TLV <compression flag set to 0>.
     *   - Active Timestamp TLV.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: SED_1");

    /**
     * Step 9: SED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to SED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Address Registration TLV (Echoes back the addresses the child has configured).
     *   - Mode TLV.
     */
    // Handled automatically by the stack.
    nexus.AdvanceTime(kShortIntervalTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Router_1");

    /**
     * Step 11: Router_1
     * - Description: Harness silently powers-off the device.
     * - Pass Criteria: N/A.
     */
    router1.Reset();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically updates Router ID Set and removes Router_1 from Network Data TLV.
     * - Pass Criteria: The DUT MUST detect that Router_1 is removed from the network and update the Router ID Set
     *   accordingly:
     *   - Remove the Network Data sections corresponding to Router_1.
     *   - Increment the Data Version and Stable Data Version fields.
     */
    nexus.AdvanceTime(kRouterIdTimeout);
    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader (DUT)");

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically multicasts new network information to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information including:
     *   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
     *   - Network Data TLV.
     */
    // Handled automatically by the stack.
    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: MED_1");

    /**
     * Step 14: MED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Leader (DUT)");

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to MED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Address Registration TLV (Echoes back the addresses the child has configured).
     *   - Mode TLV.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    /**
     * Leader (DUT) Note: Depending upon the DUT’s device implementation, two different behavior paths (A,B) are
     * allowable for transmitting the new stable network data to SED_1:
     * - Path A: Notification via MLE Child Update Request, steps 16A-17.
     * - Path B: Notification via MLE Data Response, steps 16B-17.
     *
     * Step 16A: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
     *   Update Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
     *   - Network Data TLV.
     *   - Active Timestamp TLV.
     *   - Goto Step 17.
     *
     * Step 16B: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
     *   Response.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV (Data Version field <incremented>, Stable Data Version field <incremented>).
     *   - Network Data TLV.
     *   - Active Timestamp TLV.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: SED_1");

    /**
     * Step 17: SED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A.
     */
    // Handled automatically by the stack.

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Leader (DUT)");

    /**
     * Step 18: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to SED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV.
     *   - Leader Data TLV.
     *   - Address Registration TLV (Echoes back the addresses the child has configured).
     *   - Mode TLV.
     */
    // Handled automatically by the stack.
    nexus.AdvanceTime(kShortIntervalTime);

    nexus.SaveTestInfo("test_5_6_6.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_6();
    printf("All tests passed\n");
    return 0;
}
