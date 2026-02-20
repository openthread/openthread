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

#include "mac/data_poll_sender.hpp"
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
static constexpr uint32_t kStabilizationTime = 20 * 1000;

/**
 * Time to advance for network data propagation.
 */
static constexpr uint32_t kNetworkDataPropagationTime = 2 * 1000;

/**
 * Poll period for SED in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 1000;

void Test5_6_3(void)
{
    /**
     * 5.6.3 Network data propagation (BR registers after attach) - Leader as BR
     *
     * 5.6.3.1 Topology
     *   - Leader is configured as Border Router.
     *   - MED_1 is configured to require complete network data.
     *   - SED_1 is configured to request only stable network data.
     *
     * 5.6.3.2 Purpose & Description
     *   The purpose of this test case is to show that the DUT correctly sets the Network Data (stable/non-stable)
     *     propagated by the Leader and sends it properly to devices already attached to it.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * NetData Propag   | 5.13 / 5.15  | 5.13 / 5.15
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("DUT");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    const char kPrefix1[] = "2001::/64";
    const char kPrefix2[] = "2002::/64";

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("Step 1: All");

    /**
     * Step 1: All
     *   - Description: Ensure the topology is formed correctly
     *   - Pass Criteria: N/A
     */

    /** Use AllowList to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(med1);
    med1.AllowList(router1);

    router1.AllowList(sed1);
    sed1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    // Set a short poll period for SED_1 before joining.
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));

    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     *   - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
     *     - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *     - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *     - Automatically sends multicast MLE Data Response with the new information, including the Network Data TLV
     *       with the following fields:
     *       - Prefix 1 and 2 TLVs, each including:
     *         - 6LoWPAN ID sub-TLV
     *         - Border Router sub-TLV
     *   - Pass Criteria: N/A
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("Step 3: Router_1 (DUT)");

    /**
     * Step 3: Router_1 (DUT)
     *   - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children
     *   - Pass Criteria: The DUT MUST multicast a MLE Data Response for each prefix sent by the Leader (Prefix 1 and
     *     Prefix 2)
     */

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("Step 4: MED_1");

    /**
     * Step 4: MED_1
     *   - Description: Automatically sends a MLE Child Update Request to the DUT, which includes the newly configured
     *     addresses in the Address Registration TLV
     *   - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("Step 5: Router_1 (DUT)");

    /**
     * Step 5: Router_1 (DUT)
     *   - Description: Automatically sends a MLE Child Update Response to MED_1
     *   - Pass Criteria:
     *     - The DUT MUST send a unicast MLE Child Update Response to MED_1, which includes the following TLVs:
     *       - Source Address TLV
     *       - Leader Data TLV
     *       - Address Registration TLV
     *         - Echoes back the addresses the child has configured
     *       - Mode TLV
     */

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("Step 6: Router_1 (DUT)");

    /**
     * Step 6A: Router_1 (DUT)
     *   - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Child
     *     Update Request
     *   - Pass Criteria:
     *     - The DUT MUST send a unicast MLE Child Update Request to SED_1, including the following TLVs:
     *       - Source Address TLV
     *       - Leader Data TLV
     *       - Network Data TLV
     *         - At least, the Prefix 1 TLV
     *         - The Prefix 2 TLV MUST NOT be included
     *         - The required prefix TLV MUST include the following:
     *           - P_border_router_16 <value = 0xFFFE>
     *       - Active Timestamp TLV
     *     - Goto Step 7
     *
     * Step 6B: Router_1 (DUT)
     *   - Description: Automatically sends notification of new stable network data to SED_1 via a unicast MLE Data
     *     Response
     *   - Pass Criteria:
     *     - The DUT MUST send a unicast MLE Data Response to SED_1, including the following TLVs:
     *       - Source Address TLV
     *       - Leader Data TLV
     *       - Network Data TLV
     *         - At least, the Prefix 1 TLV
     *         - The Prefix 2 TLV MUST NOT be included
     *         - The required prefix TLV MUST include the following:
     *           - P_border_router_16 <value = 0xFFFE>
     *       - Active Timestamp TLV
     */

    nexus.AdvanceTime(100 * 1000);

    Log("Step 7: SED_1");

    /**
     * Step 7: SED_1
     *   - Description: Automatically sends address configured in the Address Registration TLV to the DUT in a MLE Child
     *     Update Request command
     *   - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("Step 8: Router_1 (DUT)");

    /**
     * Step 8: Router_1 (DUT)
     *   - Description: Automatically responds with MLE Child Update Response to SED_1
     *   - Pass Criteria:
     *     - The DUT MUST send a MLE Child Update Response, which includes the following TLVs:
     *       - Source Address TLV
     *       - Leader Data TLV
     *       - Address Registration TLV
     *         - Echoes back the addresses the child has configured
     *       - Mode TLV
     */

    nexus.AdvanceTime(kNetworkDataPropagationTime);

    nexus.SaveTestInfo("test_5_6_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_3();
    printf("All tests passed\n");
    return 0;
}
