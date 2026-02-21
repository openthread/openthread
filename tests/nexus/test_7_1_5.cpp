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

/** Time to advance for a node to form a network and become leader, in milliseconds. */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/** Time to advance for a node to join as a child and upgrade to a router, in milliseconds. */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/** Time to advance for a child to attach to its parent. */
static constexpr uint32_t kAttachToParentTime = 10 * 1000;

/** Time to advance for the network to stabilize. */
static constexpr uint32_t kStabilizationTime = 30 * 1000;

void Test7_1_5(void)
{
    /**
     * 7.1.5 Network data updates - 3 Prefixes
     *
     * 7.1.5.1 Topology
     * - MED is configured to require complete network data. (Mode TLV)
     * - SED is configured to request only stable network data. (Mode TLV)
     *
     * 7.1.5.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT sends properly formatted Server Data Notification CoAP
     *   frame when a third global prefix information is set on the DUT. The DUT must also correctly set Network Data
     *   aggregated and disseminated by the Leader and transmit it properly to all child devices already attached to it.
     *
     * Spec Reference                   | V1.1 Section       | V1.3.0 Section
     * ---------------------------------|--------------------|--------------------
     * Thread Network Data / Stable     | 5.13 / 5.14 / 5.15 | 5.13 / 5.14 / 5.15
     *   Thread Network Data / Network  |                    |
     *   Data and Propagation           |                    |
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

    /**
     * Step 1: All
     * - Description: Topology Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("Step 1: All");

    /** Use AllowList feature to restrict the topology. */
    router1.AllowList(leader);
    router1.AllowList(med1);
    router1.AllowList(sed1);

    leader.AllowList(router1);
    med1.AllowList(router1);
    sed1.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToParentTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    sed1.Join(router1, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(1000));
    nexus.AdvanceTime(kAttachToParentTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    /**
     * Step 2: Router_1 (DUT)
     * - Description: User configures the DUT with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 3: P_prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=0
     * - Pass Criteria: N/A
     */
    Log("Step 2: Router_1 (DUT)");

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

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString("2003::/64"));
    config.mStable       = true;
    config.mOnMesh       = true;
    config.mPreferred    = true;
    config.mSlaac        = true;
    config.mDefaultRoute = false;
    SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically transmits a CoAP Server Data Notification to the Leader
     * - Pass Criteria: The DUT MUST send a CoAP Server Data Notification frame to the Leader including the server's
     *   information (Prefix, Border Router) for all three prefixes (Prefix 1, 2 and 3):
     *   - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
     *   - CoAP Payload: Thread Network Data TLV
     */
    Log("Step 3: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4: Leader
     * - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT for each of the three Prefixes
     *   configured in Step 2. Automatically transmits multicast MLE Data Response with the new information collected
     *   from the DUT.
     * - Pass Criteria: N/A
     */
    Log("Step 4: Leader");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5: Router_1 (DUT)
     * - Description: Automatically sends new network data to MED_1
     * - Pass Criteria: The DUT MUST multicast an MLE Data Response, including at least three Prefix TLVs (Prefix 1,
     *   Prefix2, and Prefix 3).
     */
    Log("Step 5: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 6: MED_1
     * - Description: Automatically sends MLE Child Update Request to its parent (DUT), reporting its configured global
     *   addresses in the Address Registration TLV
     * - Pass Criteria: N/A
     */
    Log("Step 6: MED_1");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7: Router_1 (DUT)
     * - Description: Automatically sends a MLE Child Update Response to MED_1, echoing back the configured addresses
     *   reported by MED_1
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Response to MED_1, which includes the following
     *   TLVs:
     *   - Source Address TLV
     *   - Address Registration TLV
     *     - Echoes back the addresses configured by MED_1
     *   - Mode TLV
     */
    Log("Step 7: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Router_1 (DUT) Note: Depending upon the DUT's device implementation, two different behavior paths (A,B) are
     *   allowable for transmitting the new network data to SED_1:
     * - Path A: Notification via MLE Child Update Request, steps 8A-9
     * - Path B: Notification via MLE Data Response, steps 8B-9
     */

    /**
     * Step 8A/B: Router_1 (DUT)
     * - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
     * Request or MLE Data Response
     * - Pass Criteria: The DUT MUST send MLE Child Update Request or Data Response to SED_1, which includes the
     * following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *   - At least two Prefix TLVs (Prefix 1 and Prefix 3)
     *     - Border Router TLV
     *       - P_border_router_16 <0xFFFE>
     *   - Prefix 2 TLV MUST NOT be included
     *   - Active Timestamp TLV
     */
    Log("Step 8A/B: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 9: SED_1
     * - Description: Automatically sends global address configured to parent, in the Address Registration TLV from the
     *   Child Update request command.
     * - Pass Criteria: N/A
     */
    Log("Step 9: SED_1");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 10: Router_1 (DUT)
     * - Description: Automatically sends a Child Update Response to SED_1, echoing back the configured addresses
     * reported by SED_1
     * - Pass Criteria: The DUT MUST send a unicast MLE Child Update Response to SED_1, including the following TLVs:
     *   - Source Address TLV
     *   - Address Registration TLV
     *     - Echoes back the addresses configured by SED_1
     *   - Mode TLV
     */
    Log("Step 10: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_7_1_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test7_1_5();
    printf("All tests passed\n");
    return 0;
}
