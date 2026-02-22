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

#include "mac/mac.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/child_table.hpp"
#include "thread/neighbor.hpp"
#include "thread/neighbor_table.hpp"
#include "thread/network_data_local.hpp"
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
 * Time to advance for a node to join as a child, in milliseconds.
 */
static constexpr uint32_t kAttachAsChildTime = 5 * 1000;

/**
 * Child timeout value in seconds.
 */
static constexpr uint32_t kChildTimeout = 10;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * IPv6 Prefix 1.
 */
static const char kPrefix1[] = "2001::/64";

/**
 * IPv6 Prefix 2.
 */
static const char kPrefix2[] = "2002::/64";

void Test7_1_8(const char *aJsonFile)
{
    /**
     * 7.1.8 Network data propagation – Border Router as End Device in Thread network; registers new server data
     *   information after network is formed
     *
     * 7.1.8.1 Topology
     * - FED_1 is configured to require complete network data. (Mode TLV)
     *
     * 7.1.8.2 Purpose & Description
     * The purpose of this test case is to verify that when global prefix information is set on the FED, the DUT
     *   properly disseminates the associated network data. It also verifies that the DUT sends revised server data
     *   information to the Leader when the FED is removed.
     *
     * Spec Reference                             | V1.1 Section       | V1.3.0 Section
     * -------------------------------------------|--------------------|--------------------
     * Thread Network Data / Stable Thread        | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15
     *   Network Data / Network Data Propagation  |                    |
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode(); // DUT
    Node &fed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    fed1.SetName("FED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Topology Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    /** Use AllowList feature to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(fed1);
    fed1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    fed1.Get<Mle::Mle>().SetTimeout(kChildTimeout);
    fed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(fed1.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: FED_1");

    /**
     * Step 2: FED_1
     * - Description: Harness configures device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     *   - Prefix 2: P_Prefix=2002::/64 P_stable=0 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     *   - Automatically sends a CoAP Server Data Notification message with the server’s information (Prefix, Border
     *     Router) to the Leader.
     * - Pass Criteria: N/A.
     */

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        IgnoreError(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mDefaultRoute = true;
        config.mSlaac        = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        IgnoreError(fed1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        IgnoreError(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = false;
        config.mDefaultRoute = true;
        config.mSlaac        = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        IgnoreError(fed1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        fed1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically transmits
     *   multicast MLE Data Response with the new information collected, adding also 6LoWPAN ID TLV for the prefix set
     *   on FED_1.
     * - Pass Criteria: N/A.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Router_1 (DUT)");

    /**
     * Step 4: Router_1 (DUT)
     * - Description: Automatically transmits multicast MLE Data Response with the new information collected, adding
     *   also 6LoWPAN ID TLV for the prefix set on FED_1.
     * - Pass Criteria: The DUT MUST send a multicast MLE Data Response.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: FED_1");

    /**
     * Step 5: FED_1
     * - Description: Harness silently powers-down FED_1 and waits for Router_1 to remove FED_1 from its neighbor
     *   table.
     * - Pass Criteria: N/A.
     */

    fed1.Get<Mle::Mle>().Stop();

    // Wait for the child to be timed out by the parent router.
    nexus.AdvanceTime((kChildTimeout + 2) * 1000);

    // Verify that the child is removed from the neighbor table.
    VerifyOrQuit(router1.Get<NeighborTable>().FindNeighbor(fed1.Get<Mac::Mac>().GetExtAddress()) == nullptr,
                 "FED_1 should be removed after timeout");

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1 (DUT)");

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically notifies Leader of removed server’s (FED_1’s) RLOC16.
     * - Pass Criteria: The DUT MUST send a CoAP Server Data Notification message to the Leader containing only the
     *   removed server’s RLOC16:
     *   - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *   - CoAP Payload: RLOC16 TLV.
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test7_1_8((argc > 2) ? argv[2] : "test_7_1_8.json");
    printf("All tests passed\n");
    return 0;
}
