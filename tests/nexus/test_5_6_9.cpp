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
 * Time to advance for network data propagation.
 */
static constexpr uint32_t kNetDataPropagationTime = 30 * 1000;

/**
 * Child timeout to prevent children from detaching during the test, in seconds.
 */
static constexpr uint32_t kLargeTimeout = 3600;

/**
 * Short poll period for SED_1 to ensure it receives data in a timely manner, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 2000;

static constexpr uint16_t kEchoId6  = 0x1234;
static constexpr uint16_t kEchoId7  = 0x5678;
static constexpr uint16_t kEchoId11 = 0xABCD;
static constexpr uint16_t kEchoId15 = 0xFEBA;

void Test5_6_9(void)
{
    /**
     * 5.6.9 Router Behavior - External Route
     *
     * 5.6.9.1 Topology
     * - Leader and Router_2 are configured as Border Routers.
     *
     * 5.6.9.2 Purpose & Description
     * The purpose of this test case is to verify that the DUT properly forwards data packets to a Border Router based
     *   on Network Data information.
     *
     * Spec Reference | V1.1 Section | V1.3.0 Section
     * ---------------|--------------|---------------
     * Server Behavior| 5.15.6       | 5.15.6
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode(); // DUT
    Node &router2 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");

    const char *kPrefix1 = "2001::/64";
    const char *kPrefix2 = "2002::/64";

    const char *kAddress2002_0 = "2002::0";
    const char *kAddress2007_0 = "2007::0";

    Ip6::Address dest2002_0;
    Ip6::Address dest2007_0;

    SuccessOrQuit(dest2002_0.FromString(kAddress2002_0));
    SuccessOrQuit(dest2007_0.FromString(kAddress2007_0));

    /**
     * - Leader and Router 1 (DUT)
     * - Leader and Router 2
     * - Router 1 (DUT) and MED 1
     * - Router 1 (DUT) and SED 1
     */
    leader.AllowList(router1);
    leader.AllowList(router2);
    router1.AllowList(leader);
    router2.AllowList(leader);

    router1.AllowList(med1);
    router1.AllowList(sed1);
    med1.AllowList(router1);
    sed1.AllowList(router1);

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    med1.Get<Mle::Mle>().SetTimeout(kLargeTimeout);
    sed1.Get<Mle::Mle>().SetTimeout(kLargeTimeout);

    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */
    Log("Step 1: Ensure topology is formed correctly.");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader, Node::kAsFtd);
    router2.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 2: Leader
     * - Description: Harness configures the device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=0 (Medium) P_slaac_=1
     *     P_default = 1 (True).
     *   - Harness configures the device with the following External Route Set:
     *   - Prefix 2: R_prefix=2002::/64 R_stable=1 R_preference=0 (Medium).
     *   - The device automatically sends multicast MLE Data Response with the new information, including the
     *     Network Data TLV with the following TLVs:
     *     - Prefix 1 TLV, including: 6LoWPAN ID sub-TLV, Border Router sub-TLV.
     *     - Prefix 2 TLV, including: Has Route sub-TLV.
     * - Pass Criteria: N/A.
     */
    Log("Step 2: Leader configures Prefix 1 (On-Mesh) and Prefix 2 (External Route).");
    {
        NetworkData::OnMeshPrefixConfig config1;
        config1.Clear();
        SuccessOrQuit(config1.GetPrefix().FromString(kPrefix1));
        config1.mStable       = true;
        config1.mOnMesh       = true;
        config1.mPreference   = NetworkData::kRoutePreferenceMedium;
        config1.mSlaac        = true;
        config1.mDefaultRoute = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config1));

        NetworkData::ExternalRouteConfig config2;
        config2.Clear();
        SuccessOrQuit(config2.GetPrefix().FromString(kPrefix2));
        config2.mStable     = true;
        config2.mPreference = NetworkData::kRoutePreferenceMedium;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddHasRoutePrefix(config2));

        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kNetDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 3: Router_2
     * - Description: Harness configures the device with the following On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred = 0 (Medium) P_slaac = 1
     *     P_default=0 (false).
     *   - Harness configures the device with the following External Route Set:
     *   - Prefix 2: R_prefix=2002::/64 R_stable=1 R_preference=1 (High).
     *   - The device automatically sends a CoAP Server Data Notification frame with the new server information
     *     (Prefix) to the Leader:
     *     - CoAP Request URI: coap://[<leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */
    Log("Step 3: Router_2 configures Prefix 1 (On-Mesh) and Prefix 2 (External Route High).");
    {
        NetworkData::OnMeshPrefixConfig config1;
        config1.Clear();
        SuccessOrQuit(config1.GetPrefix().FromString(kPrefix1));
        config1.mStable       = true;
        config1.mOnMesh       = true;
        config1.mPreference   = NetworkData::kRoutePreferenceMedium;
        config1.mSlaac        = true;
        config1.mDefaultRoute = false;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config1));

        NetworkData::ExternalRouteConfig config2;
        config2.Clear();
        SuccessOrQuit(config2.GetPrefix().FromString(kPrefix2));
        config2.mStable     = true;
        config2.mPreference = NetworkData::kRoutePreferenceHigh;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddHasRoutePrefix(config2));

        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kNetDataPropagationTime);
    VerifyOrQuit(router1.Get<ChildTable>().GetNumChildren(Child::kInStateValid) >= 2);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 4: Router_1 (DUT)
     * - Description: Automatically multicasts the new network data to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response containing the full Network Data, including: At
     *   least two Prefix TLVs (Prefix 1 & Prefix 2).
     */
    Log("Step 4: Router_1 (DUT) multicasts the new network data.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 5: Router_1 (DUT)
     * - Description: Depending on the DUT’s implementation, automatically sends new stable network data to SED_1 via
     *   a unicast MLE Child Update Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST send EITHER a unicast MLE Child Update Request OR a unicast MLE Data Response to
     *   SED_1, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV:
     *     - At least 2 Prefix TLVs (Prefix 1 and Prefix 2)
     *     - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
     *     - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     */
    Log("Step 5: Router_1 (DUT) unicasts the new stable network data to SED_1.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 6: SED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request with Destination address of 2002::0
     *   (address not allocated inside the Thread network).
     * - Pass Criteria:
     *   - The DUT MUST add mesh header with: Destination: R2 RLOC16, Source: R1 RLOC16.
     *   - The DUT MUST forward the ICMPv6 Echo Request to Router_2 due to higher preference.
     */
    Log("Step 6: SED_1 sends an ICMPv6 Echo Request with Destination address of 2002::0.");
    sed1.SendEchoRequest(dest2002_0, kEchoId6);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 7: MED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request with Destination address of 2007::0
     *   (address not allocated inside the Thread network).
     * - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Request to the Leader due to default route.
     */
    Log("Step 7: MED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.");
    med1.SendEchoRequest(dest2007_0, kEchoId7);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 8: Router_2
     * - Description: Harness configures the device with the following updated On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_on_mesh=1 P_preferred=1 (High) P_slaac=1 P_default = 1 (True).
     *   - The device automatically sends a CoAP Server Data Notification frame with the new server information
     *     (Prefix) to the Leader:
     *     - CoAP Request URI: coap://[<Leader address>]:MM/n/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */
    Log("Step 8: Router_2 updates Prefix 1 (On-Mesh High Default).");
    {
        NetworkData::OnMeshPrefixConfig config1;
        config1.Clear();
        SuccessOrQuit(config1.GetPrefix().FromString(kPrefix1));
        config1.mStable       = true;
        config1.mOnMesh       = true;
        config1.mPreference   = NetworkData::kRoutePreferenceHigh;
        config1.mSlaac        = true;
        config1.mDefaultRoute = true;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config1));

        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kNetDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 9: Router_1 (DUT)
     * - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children
     *   (MED_1).
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response containing the full Network Data, including: At
     *   least two Prefix TLVs (Prefix 1 & Prefix 2).
     */
    Log("Step 9: Router_1 (DUT) multicasts the new network information.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 10: Router_1 (DUT)
     * - Description: Depending on the the DUT’s implementation, automatically sends new stable network data to SED_1
     *   via a unicast MLE Child Update Request or MLE Data Response.
     * - Pass Criteria: The DUT MUST send EITHER a unicast MLE Child Update Request OR a unicast MLE Data Response to
     *   SED_1, which includes the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV:
     *     - At least two Prefix TLVs (Prefix 1 and Prefix 2)
     *     - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
     *     - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
     *   - Active Timestamp TLV
     */
    Log("Step 10: Router_1 (DUT) unicasts the new stable network data to SED_1.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 11: SED_1
     * - Description: Harness instructs SED_1 to send an ICMPv6 Echo Request with Destination address of 2007::0
     *   (Address not allocated inside the Thread network).
     * - Pass Criteria:
     *   - The DUT MUST add mesh header with: Destination: Router_2 RLOC16, Source: Router_1 RLOC16.
     *   - The DUT MUST forward the ICMPv6 Echo Request to Router_2 due to default route with higher preference.
     */
    Log("Step 11: SED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.");
    sed1.SendEchoRequest(dest2007_0, kEchoId11);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 12: Router_2
     * - Description: Harness configures the device with the following updated On-Mesh Prefix Set:
     *   - Prefix 1: P_prefix=2001::/64 P_stable=1 P_preference=0 (Medium) P_on_mesh=1 P_slaac=1 P_default = 1 (True).
     *   - The device automatically sends a CoAP Server Data Notification frame with the new server information
     *     (Prefix) to the Leader:
     *     - CoAP Request URI: coap://[<Leader address>]:MM/a/sd
     *     - CoAP Payload: Thread Network Data TLV
     * - Pass Criteria: N/A.
     */
    Log("Step 12: Router_2 updates Prefix 1 (On-Mesh Medium Default).");
    {
        NetworkData::OnMeshPrefixConfig config1;
        config1.Clear();
        SuccessOrQuit(config1.GetPrefix().FromString(kPrefix1));
        config1.mStable       = true;
        config1.mOnMesh       = true;
        config1.mPreference   = NetworkData::kRoutePreferenceMedium;
        config1.mSlaac        = true;
        config1.mDefaultRoute = true;
        SuccessOrQuit(router2.Get<NetworkData::Local>().AddOnMeshPrefix(config1));

        router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }
    nexus.AdvanceTime(kNetDataPropagationTime);

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 13: Router_1 (DUT)
     * - Description: Automatically multicasts the new network information to neighbors and rx-on-when-idle Children.
     * - Pass Criteria: The DUT MUST multicast a MLE Data Response, including: At least two Prefix TLVs (Prefix 1 &
     *   Prefix 2).
     */
    Log("Step 13: Router_1 (DUT) multicasts the new network information.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 14: Router_1 (DUT)
     * - Description: Automatically unicasts the new network information to SED_1.
     * - Pass Criteria: Depending on its implementation, the DUT MUST send EITHER a unicast MLE Data Response OR a
     *   unicast MLE Child Update Request to SED_1, containing only stable Network Data, which includes:
     *   - At least two Prefix TLVs (Prefix 1 & Prefix 2)
     *   - Prefix 1 TLV MUST include: 6LoWPAN ID TLV, Border Router TLV: P_border_router_16 <value = 0xFFFE>
     *   - Prefix 2 TLV MUST include: 6LoWPAN ID TLV, Has Route TLV: P_border_router_16 <value = 0xFFFE>
     */
    Log("Step 14: Router_1 (DUT) unicasts the new network information to SED_1.");

    Log("---------------------------------------------------------------------------------------");
    /**
     * Step 15: SED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request with a Destination address of
     *   2007::0 (Address not allocated inside the Thread network).
     * - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Request to Leader due to default route with lowest mesh
     *   path cost.
     */
    Log("Step 15: SED_1 sends an ICMPv6 Echo Request with Destination address of 2007::0.");
    sed1.SendEchoRequest(dest2007_0, kEchoId15);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_6_9.json");
    Log("Test 5.6.9 passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_6_9();
    printf("All tests passed\n");
    return 0;
}
