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
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for CoAP and MLE Data Response, in milliseconds.
 */
static constexpr uint32_t kDataPropagationTime = 20 * 1000;

void Test5_6_2(void)
{
    /**
     * 5.6.2 Network data propagation (BR exists during attach) - Router as BR
     *
     * 5.6.2.1 Topology
     * - Router_1 is configured as Border Router.
     * - MED_1 is configured to require complete network data. (Mode TLV)
     * - SED_1 is configured to request only stable network data. (Mode TLV)
     *
     * 5.6.2.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT, as Leader, collects network data information
     *   (stable/non-stable) from the network and propagates it properly during the attach procedure.
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

    /**
     * - Use AllowList to specify links between nodes. There is a link between the following node pairs:
     *   - Leader (DUT) and Router 1
     *   - Leader (DUT) and MED 1
     *   - Leader (DUT) and SED 1
     */
    leader.AllowList(router1);
    leader.AllowList(med1);
    leader.AllowList(sed1);

    router1.AllowList(leader);
    med1.AllowList(leader);
    sed1.AllowList(leader);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1: Leader (DUT)
     * - Description: Forms the network and sends MLE Advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements, with an IP Hop Limit of 255, to the
     *     Link-Local All Nodes multicast address (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */
    Log("Step 1: Leader (DUT) forms the network and sends MLE Advertisements.");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2: Router_1
     * - Description: Harness instructs the device to attach to the DUT. Router_1 requests Network Data TLV
     *   during the attaching procedure when sending the MLE Child ID Request frame.
     * - Pass Criteria: N/A
     */
    Log("Step 2: Router_1 attaches to the DUT.");
    router1.Join(leader, Node::kAsFtd);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends MLE Parent Response and MLE Child ID Response to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST properly attach Router_1 device to the network (See 5.1.1 Attaching for
     *     formatting), and transmit Network Data during the attach phase in the Child ID Response
     *     frame of the Network Data TLV.
     */
    Log("Step 3: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to Router_1.");
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4: Router_1
     * - Description: Harness configures the device as a Border Router with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Prefix 2: P_prefix=2002::/64 P_stable=0 P_on_mesh=1 P_preferred=1 P_slaac=1 P_default=1
     *   - Router_1 automatically sends a CoAP Server Data Notification frame with the server’s information
     *     to the DUT:
     *     - CoAP Request URI: coap://[<DUT address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A
     */
    Log("Step 4: Router_1 configures as a Border Router and sends CoAP Server Data Notification.");
    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mStable       = true;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        config.mPreference   = Preference::kMedium;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        config.mStable       = false;
        config.mOnMesh       = true;
        config.mPreferred    = true;
        config.mSlaac        = true;
        config.mDefaultRoute = true;
        config.mPreference   = Preference::kMedium;
        SuccessOrQuit(router1.Get<NetworkData::Local>().AddOnMeshPrefix(config));

        router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5: Leader (DUT)
     * - Description: Automatically sends a CoAP Response frame and MLE Data Response message.
     * - Pass Criteria:
     *   - The DUT MUST transmit a 2.04 Changed CoAP response code to Router_1.
     *   - The DUT MUST multicast an MLE Data Response message with the new information collected,
     *     adding also the 6LoWPAN ID TLV for the prefix set on Router_1.
     */
    Log("Step 5: Leader (DUT) sends a CoAP Response frame and MLE Data Response message.");
    nexus.AdvanceTime(kDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 6: SED_1
     * - Description: Harness instructs the device to attach to the DUT. SED_1 requests only the stable
     *   Network Data (Mode TLV in Child ID Request frame has “N” bit set to 0).
     * - Pass Criteria: N/A
     */
    Log("Step 6: SED_1 attaches to the DUT and requests only stable Network Data.");
    sed1.Join(leader, Node::kAsSed);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 7: Leader (DUT)
     * - Description: Automatically sends MLE Parent Response and MLE Child ID Response.
     * - Pass Criteria:
     *   - The DUT MUST send an MLE Child ID Response to SED_1, containing only stable Network Data, including:
     *     - At least the Prefix 1 TLV - The Prefix 2 TLV MUST NOT be included.
     *     - The required Prefix TLV MUST include the following fields:
     *       - 6LoWPAN ID sub-TLV
     *       - Border Router sub-TLV
     *       - P_border_router_16 <value = 0xFFFE>
     */
    Log("Step 7: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to SED_1.");
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 8: MED_1
     * - Description: Harness instructs the device to attach to the DUT. MED_1 requests the complete
     *   Network Data (Mode TLV in Child ID Request frame has “N” bit set to 1).
     * - Pass Criteria: N/A
     */
    Log("Step 8: MED_1 attaches to the DUT and requests complete Network Data.");
    med1.Join(leader, Node::kAsMed);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 9: Leader (DUT)
     * - Description: Automatically sends MLE Parent Response and MLE Child ID Response.
     * - Pass Criteria:
     *   - The DUT MUST send an MLE Child ID Response to MED_1, containing the full Network Data, including:
     *     - At least two Prefix TLVs (one for Prefix set 1 and 2), each including:
     *       - 6LoWPAN ID sub-TLV
     *       - Border Router sub-TLV
     */
    Log("Step 9: Leader (DUT) sends MLE Parent Response and MLE Child ID Response to MED_1.");
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 10: SED_1, MED_1
     * - Description: Automatically send global address configured in the Address Registration TLV to
     *   their parent in a MLE Child Update Request command.
     * - Pass Criteria: N/A
     */
    Log("Step 10: SED_1 and MED_1 send MLE Child Update Request with Address Registration TLV.");
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically sends MLE Child Update Response to MED_1 and SED_1.
     * - Pass Criteria:
     *   - The following TLVs MUST be present in the MLE Child Update Response:
     *     - Address Registration TLV (Echoes back the addresses the child has configured)
     *     - Leader Data TLV
     *     - Mode TLV
     *     - Source Address TLV
     */
    Log("Step 11: Leader (DUT) sends MLE Child Update Response to MED_1 and SED_1.");
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_6_2.json");
    Log("Test 5.6.2 passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_2();
    printf("All tests passed\n");
    return 0;
}
