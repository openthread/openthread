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
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for the network data to propagate and nodes to update.
 */
static constexpr uint32_t kPropagationTime = 10 * 1000;

void Test5_6_5(void)
{
    /**
     * 5.6.5 Network data updates – Router as BR
     *
     * 5.6.5.1 Topology
     * - Router_1 is configured as Border Router.
     * - MED_1 is configured to require complete network data.
     * - SED_1 is configured to request only stable network data.
     *
     * 5.6.5.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT, as Leader, properly updates the network data - after
     *   receiving new information from the routers in the network containing three Prefix configurations - and
     *   disseminates it correctly throughout the network.
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

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure the topology is formed correctly.
     * - Pass Criteria: N/A
     */

    /**
     * Use AllowList to specify links between nodes. There is a link between the following node pairs:
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

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 3: P_Prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0
     *   - Automatically sends a CoAP Server Data Notification frame with the server’s information to the DUT:
     *     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A
     */

    {
        const struct
        {
            const char *mPrefix;
            bool        mStable;
            bool        mDefaultRoute;
        } kPrefixConfigs[] = {
            {"2001::/64", true, true},
            {"2002::/64", false, true},
            {"2003::/64", true, false},
        };

        for (const auto &configInfo : kPrefixConfigs)
        {
            NetworkData::OnMeshPrefixConfig config;

            config.Clear();
            SuccessOrQuit(config.GetPrefix().FromString(configInfo.mPrefix));
            config.mStable       = configInfo.mStable;
            config.mOnMesh       = true;
            config.mPreferred    = true;
            config.mSlaac        = true;
            config.mDefaultRoute = configInfo.mDefaultRoute;
            config.mPreference   = Preference::kMedium;

            SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        }

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kPropagationTime);

    sed1.Get<DataPollSender>().SendFastPolls(5);

    nexus.AdvanceTime(kPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends a CoAP Response to Router_1.
     * - Pass Criteria: The DUT MUST transmit a 2.04 Changed CoAP response to Router_1.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response with the new network information including:
     *   - At least the Prefix 1, 2 and 3 TLVs, each including:
     *     - 6LoWPAN ID sub-TLV
     *     - Border Router sub-TLV
     *   - Leader Data TLV
     *     - Data Version field <incremented>
     *     - Stable Data Version field <incremented>
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically multicasts the MLE Data Response sent by the DUT.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: MED_1");

    /**
     * Step 6: MED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to MED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Address Registration TLV - Echoes back the addresses the child has configured
     *   - Mode TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Leader (DUT) Note: Depending upon the DUT’s device implementation, two different behavior paths (A,B) are
     *   allowable for transmitting the new stable network data to SED_1:
     * - Path A: Notification via MLE Child Update Request, steps 8A-9
     * - Path B: Notification via MLE Data Response, steps 8B-9
     */

    /**
     * Step 8A: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
     *   Update Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field <incremented>
     *     - Stable Data Version field <incremented>
     *   - Network Data TLV
     *     - At least the Prefix 1 and 3 TLVs
     *       - Prefix 2 TLV MUST NOT be included
     *     - The required prefix TLVs MUST each include:
     *       - Border Router sub-TLV: P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     * - Goto Step 9
     */

    /**
     * Step 8B: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
     *   Response.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to SED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data Version field <incremented>
     *     - Stable Data Version field <incremented>
     *   - Network Data TLV
     *     - At least the Prefix 1 and 3 TLVs
     *       - Prefix 2 TLV MUST NOT be included
     *     - The required prefix TLVs MUST each include:
     *       - Border Router sub-TLV: P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: SED_1");

    /**
     * Step 9: SED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *   Update Request command.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to SED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Address Registration TLV - Echoes back the addresses the child has configured
     *   - Mode TLV
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_6_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_5();
    printf("All tests passed\n");
    return 0;
}
