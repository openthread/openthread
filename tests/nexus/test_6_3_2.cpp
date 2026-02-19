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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_3_2(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.3.2 Network Data Update
     *
     * 6.3.2.1 Topology
     * - Topology A: DUT as Minimal End Device (MED_1) attached to Leader. (RF Isolation required)
     * - Topology B: DUT as Sleepy End Device (SED_1) attached to Leader. (No RF Isolation required)
     * - Leader: Configured as Border Router.
     *
     * 6.3.2.2 Purpose & Description
     * - For a MED (Minimal End Device) DUT: This test case verifies that the DUT identifies it has an old version of
     * the Network Data and then requests an update from its parent. This scenario requires short-term RF isolation for
     *   one device.
     * - For a SED (Sleepy End Device) DUT: This test case verifies that the DUT will receive new Network Data and
     *   respond with a MLE Child Update Request. This scenario does not require RF isolation.
     *
     * Spec Reference      | V1.1 Section | V1.3.0 Section
     * --------------------|--------------|---------------
     * Thread Network Data | 5.13         | 5.13
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");

    switch (aTopology)
    {
    case kTopologyA:
        dut.SetName("MED_1");
        break;
    case kTopologyB:
        dut.SetName("SED_1");
        break;
    }

    /**
     * Use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - Leader and SED 1 (DUT)
     */
    leader.AllowList(dut);
    dut.AllowList(leader);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    switch (aTopology)
    {
    case kTopologyA:
        dut.Join(leader, Node::kAsMed);
        break;
    case kTopologyB:
        dut.Join(leader, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
        break;
    }

    nexus.AdvanceTime(kAttachTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     * - Description: Harness updates the Network Data by configuring the device with the following Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Leader automatically sends new network information to the Child (DUT) using the appropriate method:
     *     - For DUT = MED: The Leader multicasts a MLE Data Response.
     *     - For DUT = SED: Depending on its own implementation, the Leader automatically sends new network data to the
     *       DUT using EITHER a MLE Data Response OR a MLE Child Update Request.
     * - Pass Criteria: N/A
     */
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2001::/64"));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: MED_1 / SED_1 (DUT)");

    /**
     * Step 3: MED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Child Update Request to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child Update Request to the Leader, which includes the following TLVs:
     *     - Leader Data TLV
     *     - Address Registration TLV (contains the global address configured by DUT device based on advertised prefix
     *       2001:: by checking the CID and ML-EID)
     *     - Mode TLV
     *     - Timeout TLV
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends MLE Child Update response frame to the DUT.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: SED_1 (DUT)");

    /**
     * Step 5: SED_1 (DUT)
     * - Description: For DUT = SED: The SED test ends here, the MED test continues.
     * - Pass Criteria: N/A
     */

    VerifyOrExit(aTopology == kTopologyA);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: User");

    /**
     * Step 6: User
     * - Description: The user places the Leader OR the DUT in RF isolation to disable communication between them.
     * - Pass Criteria: N/A
     */
    leader.UnallowList(dut);
    dut.UnallowList(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Test Harness");

    /**
     * Step 7: Test Harness
     * - Description: Test Harness prompt reads: Place DUT in RF Enclosure for time: t < child timeout, Press “OK”
     *   immediately after placing DUT in RF enclosure.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader");

    /**
     * Step 8: Leader
     * - Description: Harness updates the Network Data by configuring the Leader with the following changes to the
     *   Prefix Set:
     *   - Prefix 2: P_prefix=2002::/64 P_stable=1 P_on_mesh=1 P_slaac=1 P_default=1
     *   - The Leader automatically sends a multicast MLE Data Response with the new network information.
     *   - The DUT is currently isolated from the Leader, so it does not hear this Data Response.
     * - Pass Criteria: N/A
     */
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2002::/64"));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: User");

    /**
     * Step 9: User
     * - Description: The user must remove the RF isolation between the Leader and the DUT after the MLE Data Response
     *   is sent by the Leader for Prefix 2, but before the DUT timeout expires. (If the DUT timeout expires while in
     *   RF isolation, the test will fail because the DUT will go through re-attachment when it emerges.)
     * - Pass Criteria: N/A
     */
    leader.AllowList(dut);
    dut.AllowList(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: MED_1 (DUT)");

    /**
     * Step 10: MED_1 (DUT)
     * - Description: Detects the updated Data Version in the Leader advertisement and automatically sends MLE Data
     *   Request to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Data Request frame to request the updated Network Data.
     *   - The following TLVs MUST be included in the MLE Data Request:
     *     - TLV Request TLV: Network Data TLV
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader");

    /**
     * Step 11: Leader
     * - Description: Automatically sends the network data to the DUT.
     * - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: MED_1 (DUT)");

    /**
     * Step 12: MED_1 (DUT)
     * - Description: Automatically sends MLE Child Update Request to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child Update Request to the Leader, which includes the following TLVs:
     *     - Address Registration TLV (contains the global addresses configured by DUT based on both advertised prefixes
     *       - 2001:: and 2002:: - and ML-EID address by checking the CID)
     *     - Leader Data TLV
     *     - Mode TLV
     *     - Timeout TLV
     */
    nexus.AdvanceTime(kStabilizationTime);

exit:
    nexus.SaveTestInfo(aJsonFile);

    Log("Test 6.3.2 passed");
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_3_2(ot::Nexus::kTopologyA, "test_6_3_2_A.json");
        ot::Nexus::RunTest6_3_2(ot::Nexus::kTopologyB, "test_6_3_2_B.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::RunTest6_3_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_3_2_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::RunTest6_3_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_3_2_B.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
