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
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * IPv6 Prefix 1.
 */
static const char kPrefix1[] = "2001::";

/**
 * IPv6 Prefix 2.
 */
static const char kPrefix2[] = "2002::";

/**
 * Prefix length for both prefixes.
 */
static constexpr uint8_t kPrefixLength = 64;

void Test7_1_2(void)
{
    /**
     * 7.1.2 Network data propagation - Border Router as Router in a Thread Network; correctly sets and propagates
     *   network data information during attach
     *
     * 7.1.2.1 Topology
     * - MED_1 is configured to require complete network data. (Mode TLV)
     * - SED_1 is configured to request only stable network data. (Mode TLV)
     *
     * 7.1.2.2 Purpose & Description
     * The purpose of this test case is to verify that when global prefix information is set on the DUT, it properly
     *   unicasts information to the Leader using CoAP (Server Data Notification). This test case also verifies that
     *   the DUT correctly sets Network Data aggregated and disseminated by the Leader (stable/non-stable) of the
     *   network and transmits it properly to devices during the attach procedure.
     *
     * Spec Reference                                                         | V1.1 Section | V1.3.0 Section
     * -----------------------------------------------------------------------|--------------|---------------
     * Thread Network Data / Stable Thread Network Data / Network Data Prop.  | 5.13-5.15    | 5.13-5.15
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode(); // DUT
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: Leader
     * - Description: Forms the network.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: Leader");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    /**
     * Step 2: Router_1 (DUT)
     * - Description: User joins the DUT to the network and configures On-Mesh Prefix Set (it is also allowed to
     *   configure the On-Mesh Prefix Set on the DUT first and then attach DUT to Leader).
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Advertisements properly.
     *   - The DUT MUST send a CoAP Server Data Notification message with the server’s information (Prefix, Border
     *     Router) to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     */
    Log("Step 2: Router_1 (DUT)");

    /** Use AllowList feature to specify links between nodes. */
    router1.AllowList(leader);
    leader.AllowList(router1);

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        IgnoreError(static_cast<Ip6::Address &>(config.mPrefix.mPrefix).FromString(kPrefix1));
        config.mPrefix.mLength = kPrefixLength;
        config.mStable         = true;
        config.mOnMesh         = true;
        config.mPreferred      = true;
        config.mSlaac          = true;
        config.mDefaultRoute   = true;
        IgnoreError(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        IgnoreError(static_cast<Ip6::Address &>(config.mPrefix.mPrefix).FromString(kPrefix2));
        config.mPrefix.mLength = kPrefixLength;
        config.mStable         = false;
        config.mOnMesh         = true;
        config.mPreferred      = true;
        config.mSlaac          = true;
        config.mDefaultRoute   = true;
        IgnoreError(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 3: Leader
     * - Description: Automatically transmits a 2.04 Changed CoAP response to the DUT. Automatically transmits
     *   multicast MLE Data Response with the new information collected, adding also 6LoWPAN ID TLV for the prefix set
     *   on the DUT.
     * - Pass Criteria: N/A.
     */
    Log("Step 3: Leader");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4: Router_1 (DUT)
     * - Description: Automatically broadcasts new network data.
     * - Pass Criteria: The DUT MUST multicast the MLE Data Response message sent by the Leader.
     */
    Log("Step 4: Router_1 (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5: MED_1
     * - Description: Harness instructs MED_1 to attach to Router_1 (DUT) and request complete network data.
     * - Pass Criteria: N/A.
     */
    Log("Step 5: MED_1");

    /** Use AllowList feature to specify links between nodes. */
    med1.AllowList(router1);
    router1.AllowList(med1);

    /**
     * Step 6: Router_1 (DUT)
     * - Description: Automatically sends the new network data to MED_1.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Response to MED_1, including the Network Data TLV with the following:
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2), each including:
     *       - 6LoWPAN ID sub-TLV
     *       - Border Router sub-TLV
     */
    Log("Step 6: Router_1 (DUT)");
    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 7: SED_1
     * - Description: Harness instructs SED_1 to attach to Router_1 (DUT) and request only stable data.
     * - Pass Criteria: N/A.
     */
    Log("Step 7: SED_1");

    /** Use AllowList feature to specify links between nodes. */
    sed1.AllowList(router1);
    router1.AllowList(sed1);

    /**
     * Step 8: Router_1 (DUT)
     * - Description: Automatically sends the stable Network Data to SED_1.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Response to SED_1, including the Network Data TLV (only stable Network
     *     Data) with the following:
     *     - At least one Prefix TLV (Prefix 1), including
     *       - Border Router TLV
     *         - P_border_router_16 <0xFFFE>
     *     - Prefix 2 TLV MUST NOT be included.
     */
    Log("Step 8: Router_1 (DUT)");
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 9: MED_1, SED_1
     * - Description: After attaching, each device automatically sends its configured global address to the DUT, in
     *   the Address Registration TLV via the MLE Child Update Request command.
     * - Pass Criteria: N/A.
     */
    Log("Step 9: MED_1, SED_1");
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically replies to each Child with a Child Update Response.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child Update Responses, each to MED_1 & SED_1, each including the following TLVs:
     *     - Address Registration TLV
     *       - Echoes back addresses configured by the Child in step 9
     *     - Mode TLV
     *     - Source Address TLV
     */
    Log("Step 10: Leader (DUT)");
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_7_1_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test7_1_2();
    printf("All tests passed\n");
    return 0;
}
