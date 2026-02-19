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
 * Time to advance for a child to register its address.
 */
static constexpr uint32_t kChildUpdateWaitTime = 10 * 1000;

void Test7_1_1(const char *aJsonFile)
{
    /**
     * 7.1.1 Network data propagation - Border Router as Leader of Thread Network; correctly sends Network Data
     *   information during attach
     *
     * 7.1.1.1 Topology
     * - MED_1 is configured to require complete network data. (Mode TLV)
     * - SED_1 is configured to request only stable network data. (Mode TLV)
     *
     * 7.1.1.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT, as a Border Router, acts properly as a Leader device
     *   in a Thread network, correctly sets the Network Data (stable/non-stable) and successfully propagates the
     *   Network Data to the devices that attach to it.
     *
     * Spec Reference                             | V1.1 Section    | V1.3.0 Section
     * -------------------------------------------|-----------------|-----------------
     * Thread Network Data / Stable Thread        | 5.13 / 5.14 /   | 5.13 / 5.14 /
     *   Network Data / Network Data and          | 5.15            | 5.15
     *   Propagation                              |                 |
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

    leader.AllowList(router1);
    router1.AllowList(leader);

    leader.AllowList(med1);
    med1.AllowList(leader);

    leader.AllowList(sed1);
    sed1.AllowList(leader);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: Leader (DUT)
     * - Description: Forms the network.
     * - Pass Criteria: The DUT MUST properly send MLE Advertisements.
     */
    Log("Step 1: Leader (DUT) forms the network.");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    /**
     * Step 2: Leader (DUT)
     * - Description: The user must configure the following On-Mesh Prefix Set on the device:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     * - Pass Criteria: The DUT MUST correctly aggregate configured information to create the Network Data (No OTA
     *   validation).
     */
    Log("Step 2: Leader (DUT) configures On-Mesh Prefixes.");
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

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2002::/64"));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    /**
     * Step 3: Router_1
     * - Description: Harness instructs device to join the network; it requests complete network data.
     * - Pass Criteria: N/A
     */
    Log("Step 3: Router_1 joins the network.");
    router1.Join(leader, Node::kAsFtd);

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends the requested network data to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Response to Router_1, including the following TLVs:
     *     - Network Data TLV
     *       - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
     *         - 6LoWPAN ID sub-TLV
     *         - Border Router sub-TLV
     */
    Log("Step 4: Leader (DUT) automatically sends requested network data to Router_1.");
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsFullThreadDevice());

    /**
     * Step 5: SED_1
     * - Description: Harness instructs device to join the network; it requests only stable data.
     * - Pass Criteria: N/A
     */
    Log("Step 5: SED_1 joins the network.");
    sed1.Join(leader, Node::kAsSed);

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends the requested stable network data to SED_1.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Response to SED_1, including the Network Data TLV (only stable Network Data)
     *     and the following TLVs:
     *     - At least one Prefix TLV (Prefix 1), including:
     *       - 6LoWPAN ID sub-TLV
     *       - Border Router sub-TLV
     *       - P_border_router_16 <0xFFFE>
     *     - Prefix 2 TLV MUST NOT be included.
     */
    Log("Step 6: Leader (DUT) automatically sends requested stable network data to SED_1.");
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 7: MED_1
     * - Description: Harness instructs device to join the network; it requests complete network data.
     * - Pass Criteria: N/A
     */
    Log("Step 7: MED_1 joins the network.");
    med1.Join(leader, Node::kAsMed);

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically sends the requested network data to MED_1.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Response to MED_1, which includes the following TLVs:
     *     - Network Data TLV
     *       - At least two prefix TLVs (Prefix 1 and Prefix 2), each including:
     *         - 6LoWPAN ID sub-TLV
     *         - Border Router sub-TLV
     */
    Log("Step 8: Leader (DUT) automatically sends requested network data to MED_1.");
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 9: MED_1, SED_1
     * - Description: After attaching, each Child automatically sends its global address configured to the Leader, in
     *   the Address Registration TLV from the Child Update request command.
     * - Pass Criteria: N/A
     */
    Log("Step 9: MED_1 and SED_1 automatically send Address Registration.");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically replies to each Child with a Child Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child Update Response, each, to MED_1 & SED_1.
     *   - The following TLVs MUST be present in the Child Update Response:
     *     - Source Address TLV
     *     - Address Registration TLV (Echoes back addresses configured in step 9)
     *     - Mode TLV
     */
    Log("Step 10: Leader (DUT) automatically replies to each Child with a Child Update Response.");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test7_1_1((argc > 2) ? argv[2] : "test_7_1_1.json");
    printf("All tests passed\n");
    return 0;
}
