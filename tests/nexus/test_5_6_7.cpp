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
static constexpr uint32_t kStabilizationTime = 20 * 1000;

/**
 * Time to advance for network data propagation.
 */
static constexpr uint32_t kNetworkDataPropagationTime = 2 * 1000;

/**
 * Time for REED advertisement interval (570s REED_ADVERTISEMENT_INTERVAL + 60s REED_ADVERTISEMENT_MAX_JITTER).
 */
static constexpr uint32_t kReedAdvertisementInterval = (570 + 60) * 1000;

/**
 * RF isolation time (< 30s).
 */
static constexpr uint32_t kRfIsolationTime = 20 * 1000;

void Test5_6_7(void)
{
    /**
     * 5.6.7 Request Network Data Updates – REED device
     *
     * 5.6.7.1 Topology
     * - RF isolation is required for this test case.
     * - An additional, live stand-alone sniffer is recommended to monitor the DUT’s Child Update Request cycle
     *   timing.
     * - Leader is configured as Border Router.
     * - Build a topology that has a total of 16 active routers on the network, including the Leader, with no
     *   communication constraints.
     *
     * 5.6.7.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT identifies that it has an old version of the Network
     *   Data and requests an update from its parent.
     *
     * Spec Reference               | V1.1 Section | V1.3.0 Section
     * -----------------------------|--------------|---------------
     * Network Data and Propagation | 5.15         | 5.15
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &r1     = nexus.CreateNode();
    Node &r2     = nexus.CreateNode();
    Node &r3     = nexus.CreateNode();
    Node &r4     = nexus.CreateNode();
    Node &r5     = nexus.CreateNode();
    Node &r6     = nexus.CreateNode();
    Node &r7     = nexus.CreateNode();
    Node &r8     = nexus.CreateNode();
    Node &r9     = nexus.CreateNode();
    Node &r10    = nexus.CreateNode();
    Node &r11    = nexus.CreateNode();
    Node &r12    = nexus.CreateNode();
    Node &r13    = nexus.CreateNode();
    Node &r14    = nexus.CreateNode();
    Node &r15    = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    r1.SetName("ROUTER_1");
    r2.SetName("ROUTER_2");
    r3.SetName("ROUTER_3");
    r4.SetName("ROUTER_4");
    r5.SetName("ROUTER_5");
    r6.SetName("ROUTER_6");
    r7.SetName("ROUTER_7");
    r8.SetName("ROUTER_8");
    r9.SetName("ROUTER_9");
    r10.SetName("ROUTER_10");
    r11.SetName("ROUTER_11");
    r12.SetName("ROUTER_12");
    r13.SetName("ROUTER_13");
    r14.SetName("ROUTER_14");
    r15.SetName("ROUTER_15");
    dut.SetName("REED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    leader.AllowList(r1);
    leader.AllowList(r2);
    leader.AllowList(r3);
    leader.AllowList(r4);
    leader.AllowList(r5);
    leader.AllowList(r6);
    leader.AllowList(r7);
    leader.AllowList(r8);
    leader.AllowList(r9);
    leader.AllowList(r10);
    leader.AllowList(r11);
    leader.AllowList(r12);
    leader.AllowList(r13);
    leader.AllowList(r14);
    leader.AllowList(r15);

    r1.AllowList(leader);
    r1.AllowList(dut);
    r2.AllowList(leader);
    r3.AllowList(leader);
    r4.AllowList(leader);
    r5.AllowList(leader);
    r6.AllowList(leader);
    r7.AllowList(leader);
    r8.AllowList(leader);
    r9.AllowList(leader);
    r10.AllowList(leader);
    r11.AllowList(leader);
    r12.AllowList(leader);
    r13.AllowList(leader);
    r14.AllowList(leader);
    r15.AllowList(leader);

    dut.AllowList(r1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    r1.Join(leader);
    r2.Join(leader);
    r3.Join(leader);
    r4.Join(leader);
    r5.Join(leader);
    r6.Join(leader);
    r7.Join(leader);
    r8.Join(leader);
    r9.Join(leader);
    r10.Join(leader);
    r11.Join(leader);
    r12.Join(leader);
    r13.Join(leader);
    r14.Join(leader);
    r15.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(r1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(r15.Get<Mle::Mle>().IsRouter());

    dut.Join(r1, Node::kAsFtd);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: All");

    /**
     * Step 2: All
     * - Description: Wait for 630 seconds to elapse (570s REED_ADVERTISEMENT_INTERVAL + 60s
     *   REED_ADVERTISEMENT_MAX_JITTER).
     * - Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kReedAdvertisementInterval);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: REED_1 (DUT)");

    /**
     * Step 3: REED_1 (DUT)
     * - Description: User places the DUT in RF isolation for time < REED timeout value (30 seconds). It is useful to
     *   monitor the DUT’s Child Update Request cycle timing and, if prudent, wait to execute this step until just
     *   after the cycle has completed. If the Child Update cycle occurs while the DUT is in RF isolation, the test
     *   will fail because the DUT will go through (re) attachment when it emerges.
     * - Pass Criteria: N/A.
     */
    dut.UnallowList(r1);
    r1.UnallowList(dut);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Harness updates the Network Data by configuring the Leader with the following Prefix Set:
     *   - Prefix 1: P_Prefix=2003::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1.
     *   - The Leader multicasts an MLE Data Response containing the new information. The Network Data TLV includes
     *     the following fields:
     *     - Prefix TLV, including:
     *       - Border Router sub-TLV
     *       - 6LoWPAN ID sub-TLV.
     * - Pass Criteria: N/A.
     */
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2003::/64"));
        config.mStable       = true;
        config.mDefaultRoute = true;
        config.mSlaac        = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;

        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: All Routers");

    /**
     * Step 5: All Routers
     * - Description: Automatically multicast the MLE Data Response sent by the Leader device.
     * - Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kNetworkDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: REED_1 (DUT)");

    /**
     * Step 6: REED_1 (DUT)
     * - Description: User removes the RF isolation after time < REED timeout value (30 seconds).
     * - Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kRfIsolationTime);
    dut.AllowList(r1);
    r1.AllowList(dut);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: All");

    /**
     * Step 7: All
     * - Description: Wait for 630 seconds to elapse (570s REED_ADVERTISEMENT_INTERVAL + 60s
     *   REED_ADVERTISEMENT_MAX_JITTER).
     * - Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kReedAdvertisementInterval);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: REED_1 (DUT)");

    /**
     * Step 8: REED_1 (DUT)
     * - Description: Hears an incremented Data Version in the MLE Advertisement messages sent by its Parent and
     *   automatically requests the updated network data.
     * - Pass Criteria:
     *   - The DUT MUST send an MLE Data Request to its parent to get the new Network Dataset.
     *   - The MLE Data Request MUST include a TLV Request TLV for the Network Data TLV.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: REED_1 (DUT)");

    /**
     * Step 9: REED_1 (DUT)
     * - Description: Receives an MLE Data Response from its Parent.
     * - Pass Criteria: N/A.
     */
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: REED_1 (DUT)");

    /**
     * Step 10: REED_1 (DUT)
     * - Description: Automatically broadcasts an MLE Advertisement.
     * - Pass Criteria: The VN_version in the Leader Data TLV of the advertisement MUST be incremented for new network
     *   data.
     */
    nexus.AdvanceTime(kReedAdvertisementInterval);

    nexus.SaveTestInfo("test_5_6_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_7();
    printf("All tests passed\n");
    return 0;
}
