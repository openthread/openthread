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
 * Time to advance for a child to register its address.
 */
static constexpr uint32_t kChildUpdateWaitTime = 10 * 1000;

void Test7_1_4(const char *aJsonFile)
{
    /**
     * 7.1.4 Network data propagation – Border Router as Router in Thread network; registers new server data information
     *   after network is formed
     *
     * 7.1.4.1 Topology
     * - MED_1 is configured to require complete network data. (Mode TLV)
     * - SED_1 is configured to request only stable network data. (Mode TLV)
     *
     * 7.1.4.2 Purpose & Description
     * The purpose of this test case is to verify that when global prefix information is set on the DUT, the DUT
     *   properly unicasts information to the Leader using COAP frame (Server Data Notification). In addition, the DUT
     *   must correctly set Network Data (stable/non-stable) aggregated and disseminated by the Leader and transmit it
     *   properly to all devices already attached to it.
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

    /** Use AllowList to specify links between nodes. */
    leader.AllowList(router1);
    router1.AllowList(leader);

    router1.AllowList(med1);
    med1.AllowList(router1);

    router1.AllowList(sed1);
    sed1.AllowList(router1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Topology Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(1000));
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2: Router_1 (DUT)
     * - Description: User configures the DUT with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     * - Pass Criteria: N/A
     */
    Log("Step 2: Router_1 (DUT)");
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2001::/64"));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2002::/64"));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically transmits a CoAP Server Data Notification to the Leader
     * - Pass Criteria: The DUT MUST send a CoAP Server Data Notification frame with the server’s information (Prefix,
     *   Border Router) to the Leader:
     *   - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
     *   - CoAP Payload: Network Data TLV
     */
    Log("Step 3: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4: Leader
     * - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically multicasts a MLE
     *   Data Response, including the new information collected from the DUT.
     * - Pass Criteria: N/A
     */
    Log("Step 4: Leader");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Automatically sends new network data to MED_1
     * - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
     *   - At least two Prefix TLVs (Prefix 1 and Prefix 2):
     *     - 6LowPAN ID TLV
     *     - Border Router TLV
     */
    Log("Step 5: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 6: MED_1
     * - Description: Automatically sends the address configured to Router_1 (DUT) via the Address Registration TLV,
     *   included as part of the Child Update request command.
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Response to MED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Address Registration TLV (Echoes back the addresses MED_1 has configured)
     *   - Mode TLV
     */
    Log("Step 6: MED_1");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    /**
     * Step 7: Router_1 (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     *   Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Request or MLE Data Response to SED_1.
     */
    Log("Step 7: Router_1 (DUT)");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    /**
     * Step 8: SED_1
     * - Description: After receiving the MLE Data Response or MLE Child Update Request, automatically sends the global
     *   address configured to Router_1 (DUT), via the Address Registration TLV, included as part of the Child Update
     *   request command.
     * - Pass Criteria: N/A
     */
    Log("Step 8: SED_1");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    /**
     * Step 9: Router_1 (DUT)
     * - Description: Automatically sends a Child Update Response to SED_1, echoing back the configured addresses
     *   reported by SED_1
     * - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SED_1. The following TLVs MUST be included in
     *   the Child Update Response:
     *   - Source Address TLV
     *   - Address Registration TLV (Echoes back the addresses SED_1 has configured)
     *   - Mode TLV
     */
    Log("Step 9: Router_1 (DUT)");
    nexus.AdvanceTime(kChildUpdateWaitTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test7_1_4((argc > 2) ? argv[2] : "test_7_1_4.json");
    printf("All tests passed\n");
    return 0;
}
