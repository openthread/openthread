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

void Test5_6_1(void)
{
    /**
     * 5.6.1 Network data propagation (BR exists during attach) – Leader as BR
     *
     * 5.6.1.1 Topology
     * - Leader is configured as a Border Router.
     * - MED_1 is configured to require the full network data.
     * - SED_1 is configured to request only the stable network data.
     *
     * 5.6.1.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT correctly sets the Network Data (stable/non-stable)
     *   received during the attaching procedure and propagates it properly to devices that attach to it.
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
    const char *kPrefix4 = "2004::/64";

    /**
     * - Use AllowList to specify links between nodes. There is a link between the following node pairs:
     *   - Router 1 (DUT) and Leader
     *   - Router 1 (DUT) and MED 1
     *   - Router 1 (DUT) and SED 1
     */
    router1.AllowList(leader);
    router1.AllowList(med1);
    router1.AllowList(sed1);

    leader.AllowList(router1);
    med1.AllowList(router1);
    sed1.AllowList(router1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1: Leader
     * - Description: Forms the network and sends MLE Advertisements.
     */
    Log("Step 1: Leader forms the network and sends MLE Advertisements.");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2: Leader
     * - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 3: P_prefix=2003::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 4: P_prefix=2004::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     */
    Log("Step 2: Leader configures as a Border Router with 4 prefixes.");
    {
        const struct
        {
            const char *mPrefix;
            bool        mIsStable;
        } kPrefixes[] = {
            {kPrefix1, true},
            {kPrefix2, false},
            {kPrefix3, true},
            {kPrefix4, true},
        };

        for (const auto &prefixInfo : kPrefixes)
        {
            NetworkData::OnMeshPrefixConfig config;

            config.Clear();
            SuccessOrQuit(config.GetPrefix().FromString(prefixInfo.mPrefix));
            config.mStable       = prefixInfo.mIsStable;
            config.mOnMesh       = true;
            config.mPreferred    = true;
            config.mSlaac        = true;
            config.mDefaultRoute = true;
            SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        }

        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3: Router_1 (DUT)
     * - Description: Automatically attaches to the Leader.
     */
    Log("Step 3: Router_1 (DUT) automatically attaches to the Leader.");
    router1.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4: Leader
     * - Description: Automatically includes the Network Data TLV in the MLE Child ID Response with the following
     *   fields:
     *   - Four Prefix TLVs (one for each prefix set 1-4), each including:
     *     - 6LoWPAN ID sub-TLV
     *     - Border Router sub-TLV
     *   - (Router_1 requests complete network data (Mode TLV))
     */
    Log("Step 4: Leader includes the Network Data TLV in the MLE Child ID Response.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5: SED_1
     * - Description: Harness instructs the device to attach to the DUT; SED_1 requests only the stable Network Data.
     */
    Log("Step 5: SED_1 attaches to the DUT; SED_1 requests only the stable Network Data.");
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically sends MLE Parent Response and MLE Child ID Response to SED_1.
     */
    Log("Step 6: Router_1 (DUT) automatically sends MLE Parent Response and MLE Child ID Response to SED_1.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 7: MED_1
     * - Description: Harness instructs the device to attach to the DUT; MED_1 requests the full Network Data.
     */
    Log("Step 7: MED_1 attaches to the DUT; MED_1 requests the full Network Data.");
    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 8: Router_1 (DUT)
     * - Description: Automatically sends MLE Parent Response and MLE Child ID Response to MED_1.
     */
    Log("Step 8: Router_1 (DUT) automatically sends MLE Parent Response and MLE Child ID Response to MED_1.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 9: SED_1, MED_1
     * - Description: Automatically send addresses configured in the Address Registration TLV to their parent in a MLE
     *   Child Update Request command.
     */
    Log("Step 9: SED_1, MED_1 automatically send addresses in a MLE Child Update Request.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 10: Router_1 (DUT)
     * - Description: Automatically sends MLE Child Update Response to SED_1 and MED_1.
     */
    Log("Step 10: Router_1 (DUT) automatically sends MLE Child Update Response to SED_1 and MED_1.");
    nexus.AdvanceTime(20 * 1000);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 11: Leader
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT GUA addresses configured
     *   from Prefix 1, Prefix 2, Prefix 3 and Prefix 4.
     */
    Log("Step 11: Leader sends an ICMPv6 Echo Request to the DUT GUA addresses.");
    nexus.AdvanceTime(kStabilizationTime);
    nexus.SendAndVerifyEchoRequest(leader, router1.FindMatchingAddress(kPrefix1));
    nexus.SendAndVerifyEchoRequest(leader, router1.FindMatchingAddress(kPrefix2));
    nexus.SendAndVerifyEchoRequest(leader, router1.FindMatchingAddress(kPrefix3));
    nexus.SendAndVerifyEchoRequest(leader, router1.FindMatchingAddress(kPrefix4));

    nexus.SaveTestInfo("test_5_6_1.json");

    Log("Test 5.6.1 passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_1();
    printf("All tests passed\n");
    return 0;
}
