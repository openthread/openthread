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
 * Time to advance for nodes to process network data updates.
 */
static constexpr uint32_t kUpdateProcessTime = 60 * 1000;

/**
 * Time to advance for nodes to send Child Update Requests.
 */
static constexpr uint32_t kChildUpdateRequestTime = 300 * 1000;

/**
 * Prefix 1 address string.
 */
static const char kPrefix1[] = "2001::/64";

/**
 * Prefix 2 address string.
 */
static const char kPrefix2[] = "2002::/64";

void Test5_6_4(void)
{
    /**
     * 5.6.4 Network data propagation (BR registers after attach) - Router as BR
     *
     * 5.6.4.1 Topology
     * - Router_1 is configured as Border Router.
     * - MED_1 is configured to require complete network data.
     * - SED_1 is configured to request only stable network data.
     *
     * 5.6.4.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT, as Leader, collects network data information
     *   (stable/non-stable) from the network and propagates it properly in an already formed network.
     *   (2-hops away).
     *
     * Spec Reference                                     | V1.1 Section | V1.3.0 Section
     * ---------------------------------------------------|--------------|---------------
     * Thread Network Data / Network Data and Propagation | 5.13 / 5.15  | 5.13 / 5.15
     */

    Core nexus;

    Node &dut     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    dut.SetName("DUT");
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

    dut.AllowList(router1);
    dut.AllowList(med1);
    dut.AllowList(sed1);

    router1.AllowList(dut);
    med1.AllowList(dut);
    sed1.AllowList(dut);

    dut.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsLeader());

    router1.Join(dut, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    // Wait for router link to be fully established.
    nexus.AdvanceTime(kUpdateProcessTime);

    med1.Join(dut, Node::kAsMed);
    nexus.AdvanceTime(kUpdateProcessTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());

    sed1.Join(dut, Node::kAsSed);
    nexus.AdvanceTime(kUpdateProcessTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    nexus.AdvanceTime(kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Automatically sends a CoAP Sever Data Notification frame with the server’s information to the Leader (DUT):
     *     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        IgnoreError(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        IgnoreError(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        IgnoreError(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        IgnoreError(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends a CoAP Response frame to Router_1.
     * - Pass Criteria: The DUT MUST transmit a 2.04 Changed CoAP response to Router_1.
     */

    nexus.AdvanceTime(kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST send a multicast MLE Data Response with the new network information collected
     *   from Router_1, including:
     *   - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
     *     - 6LoWPAN ID sub-TLV
     *     - Border Router sub-TLV
     */

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically sets Network Data after receiving multicast MLE Data Response sent by the DUT.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: MED_1");

    /**
     * Step 6: MED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE
     *   Child Update Request command.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kChildUpdateRequestTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader (DUT)");

    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically responds to MED_1 with MLE Child Update Response.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Address Registration TLV
     *     - Echoes back the addresses the child has configured
     *   - Mode TLV
     */

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Step 8: Leader (DUT)
     * - Description: Depending upon the DUT’s device implementation, two different behavior paths (A,B) are
     *   allowable for transmitting the new stable network data to SED_1:
     *   - Path A: Notification via MLE Child Update Request, steps 9A-10
     *   - Path B: Notification via MLE Data Response, steps 9B-10
     */

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader (DUT)");

    /**
     * Step 9A: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
     *   Update Request.
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Request to SED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1 TLV)
     *     - The Prefix 2 TLV MUST NOT be included
     *     - The required prefix TLV MUST include the following:
     *       - P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     * - Goto Step 10
     *
     * Step 9B: Leader (DUT)
     * - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
     *   Response.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to SED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *     - At least one Prefix TLV (Prefix 1 TLV)
     *     - The Prefix 2 TLV MUST NOT be included
     *     - The required prefix TLV MUST include the following:
     *       - P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     */

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: SED_1");

    /**
     * Step 10: SED_1
     * - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE
     *   Child Update Request command.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kChildUpdateRequestTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader (DUT)");

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically responds with MLE Child Update Response to SED_1.
     * - Pass Criteria: The DUT MUST send an MLE Child Update Response, which includes the follwing TLVs:
     *   - Address Registration TLV - Echoes back the addresses the child has configured
     *   - Leader Data TLV
     *   - Mode TLV
     *   - Source Address TLV
     */

    nexus.AdvanceTime(2 * kUpdateProcessTime);

    nexus.SaveTestInfo("test_5_6_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_4();
    printf("All tests passed\n");
    return 0;
}
