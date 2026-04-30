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
#include "thread/network_diagnostic.hpp"
#include "utils/mesh_diag.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 15 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the diagnostic response to be received.
 */
static constexpr uint32_t kDiagResponseTime = 5 * 1000;

/**
 * MLE Timeout for SED_1 in seconds.
 */
static constexpr uint32_t kSed1Timeout = 312;

static void SaveOmr(Core &aNexus, const char *aName, Node &aNode)
{
    Ip6::Prefix omrPrefix;
    IgnoreError(omrPrefix.FromString("fd00:db8::/64"));
    for (const Ip6::Netif::UnicastAddress &addr : aNode.Get<Ip6::Netif>().GetUnicastAddresses())
    {
        if (addr.GetAddress().MatchesPrefix(omrPrefix))
        {
            aNexus.AddTestVar(aName, addr.GetAddress().ToString().AsCString());
            return;
        }
    }
    aNexus.AddTestVar(aName, "");
}

void TestDiagTc1(const char *aJsonFileName)
{
    /**
     * 4.1. [1.4] [CERT] Get Diagnostics and Child Information - Router
     *
     * 4.1.2. Topology
     * - Router_1 (DUT) – Thread Router or Thread Border Router (BR)
     * - FED_1 – FED reference device (can be any Thread version)
     * - MED_1 – MED reference device (can be any Thread version)
     * - SED_1 – SED reference device (can be any Thread version)
     * - REED_1 – REED reference device, configured to not upgrade to Router role (can be any Thread version)
     * - Leader – Thread (BR or non-BR) reference device configured as Leader (Thread v1.3.x device).
     *
     * Spec Reference   | Section
     * -----------------|---------
     * Thread 1.4       | 4.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &fed1    = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();
    Node &reed1   = nexus.CreateNode();

    leader.SetName("Leader");
    router1.SetName("Router_1");
    fed1.SetName("FED_1");
    med1.SetName("MED_1");
    sed1.SetName("SED_1");
    reed1.SetName("REED_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    AllowLinkBetween(router1, leader);
    AllowLinkBetween(router1, fed1);
    AllowLinkBetween(router1, med1);
    AllowLinkBetween(router1, sed1);
    AllowLinkBetween(router1, reed1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Enable the devices in order. Leader configures its Network Data with an OMR prefix.");

    /**
     * Step 1
     * - Device: Leader, Router_1 (DUT)
     * - Description (DIAG-4.1): Enable the devices in order. The remaining (child) devices are not yet activated.
     *   Leader configures its Network Data with an OMR prefix. Note: the OMR prefix can be added using an OT CLI
     *   command such as: "prefix add 2001:dead:beef:cafe::/64 paros med"
     * - Pass Criteria:
     *   - Single Thread Network is formed with Leader and the DUT
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    NetworkData::OnMeshPrefixConfig omrPrefixConfig;
    SuccessOrQuit(omrPrefixConfig.GetPrefix().FromString("fd00:db8::/64"));
    omrPrefixConfig.mPreferred    = true;
    omrPrefixConfig.mSlaac        = true;
    omrPrefixConfig.mDhcp         = false;
    omrPrefixConfig.mDefaultRoute = true;
    omrPrefixConfig.mOnMesh       = true;
    omrPrefixConfig.mStable       = true;
    omrPrefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;
    SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(30000); // Wait for network data propagation

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader sends DIAG_GET.req to DUT's RLOC for multiple Diagnostic TLV types.");

    /**
     * Step 2
     * - Device: Leader
     * - Description (DIAG-4.1): Harness instructs device to send DIAG_GET.req to DUT's RLOC for the following
     *   Diagnostic TLV types: Type 19: Max Child Timeout TLV, Type 23: EUI-64 TLV, Type 24: Version TLV, Type 25:
     *   Vendor Name TLV, Type 26: Vendor Model TLV, Type 27: Vendor SW Version TLV, Type 28: Thread Stack Version TLV
     * - Pass Criteria:
     *   - N/A
     */
    Ip6::Address dutRloc;
    dutRloc.SetToRoutingLocator(leader.Get<Mle::Mle>().GetMeshLocalPrefix(), router1.Get<Mle::Mle>().GetRloc16());

    uint8_t tlvTypesStep2[] = {
        NetworkDiagnostic::Tlv::kMaxChildTimeout,
        NetworkDiagnostic::Tlv::kEui64,
        NetworkDiagnostic::Tlv::kVersion,
        NetworkDiagnostic::Tlv::kVendorName,
        NetworkDiagnostic::Tlv::kVendorModel,
        NetworkDiagnostic::Tlv::kVendorSwVersion,
        NetworkDiagnostic::Tlv::kThreadStackVersion,
    };
    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypesStep2,
                                                                            sizeof(tlvTypesStep2), nullptr, nullptr));

    /**
     * Step 3
     * - Device: Router_1 (DUT)
     * - Description (DIAG-4.1): Automatically responds with DIAG_GET.rsp containing the requested Diagnostic TLVs.
     * - Pass Criteria:
     *   - The DUT MUST respond with DIAG_GET.rsp
     *   - The presence of each TLV (as requested in step 2) MUST be validated, with the exception of Max Child Timeout
     *     TLV (Type 19) which MAY be present.
     *   - Value of Max Child Timeout TLV (Type 19), if present, MUST be '0' (zero).
     *   - Value of EUI-64 TLV (Type 23) MUST be non-zero.
     *   - Value of Version TLV (Type 24) MUST be >= '5' .
     *   - Value of Vendor Name TLV (Type 25) MUST have length >= 1.
     *   - Value of Vendor Model TLV (Type 26) MUST have length >= 1.
     *   - Value of Vendor SW Version TLV (Type 27) MUST have length >= 5.
     *   - Value of Thread Stack Version TLV (Type 28) MUST have length >= 5.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Enable devices FED_1, MED_1, SED_1, REED_1 with 1 second delay between each.");

    /**
     * Step 4
     * - Device: FED_1, MED_1, SED_1, REED_1
     * - Description (DIAG-4.1): Enable devices (any order is ok) with 1 second delay between each device enablement.
     *   SED_1 MUST attach using a specified Supervision Interval TLV with value '129'. (This is the OT default.) Note:
     *   this 1 second delay is only added to test role time diagnostic values, retrieved in the next step. Note: the
     *   total time of attaching the devices as Children is expected to be not longer than ~ 10 seconds. If longer,
     *   failures may result on some checks like "Age" in step 8.
     * - Pass Criteria:
     *   - Single Thread Network is formed between all devices. Wait until all reference devices attached.
     *   - DUT MUST allow all end devices to attach quickly (if still in REED role, the DUT will automatically upgrade
     *     to Router role first).
     */
    // C1: device SED_1 is configured with MLE timeout 312.
    sed1.Get<Mle::Mle>().SetTimeout(kSed1Timeout);

    // REED_1 configured to not upgrade to Router role.
    SuccessOrQuit(reed1.Get<Mle::Mle>().SetRouterEligible(false));

    fed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(1000);
    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(1000);
    sed1.Join(router1, Node::kAsSed);
    nexus.AdvanceTime(1000);
    reed1.Join(router1, Node::kAsFed);

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(fed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Leader sends DIAG_GET.req unicast to DUT's RLOC for Max Child Timeout and MLE Counters TLVs.");

    /**
     * Step 5
     * - Device: Leader
     * - Description (DIAG-4.1): Harness instructs device to send DIAG_GET.req unicast to DUT's RLOC for the following
     *   Diagnostic TLV types: Type 19: Max Child Timeout TLV, Type 34: MLE Counters TLV Note: this step must be
     *   performed as soon as possible after the previous step.
     * - Pass Criteria:
     *   - N/A
     */
    uint8_t tlvTypesStep5[] = {NetworkDiagnostic::Tlv::kMaxChildTimeout, NetworkDiagnostic::Tlv::kMleCounters};
    SuccessOrQuit(leader.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(dutRloc, tlvTypesStep5,
                                                                            sizeof(tlvTypesStep5), nullptr, nullptr));

    /**
     * Step 6
     * - Device: Router_1 (DUT)
     * - Description (DIAG-4.1): Automatically responds with DIAG_GET.rsp containing the requested Diagnostic TLVs.
     * - Pass Criteria:
     *   - The DUT MUST respond with DIAG_GET.rsp
     *   - Presence of each TLV (as requested in step 5) MUST be validated.
     *   - Value of Max Child Timeout TLV (Type 19) MUST be '312'.
     *   - In the MLE Counters TLV (Type 34): Radio Disabled Counter MUST be '0', Detached Role Counter MUST be '1',
     *     Child Role Counter MUST be '1', Router Role Counter MUST be '1', Leader Role Counter MUST be '0', Attach
     *     Attempts Counter MUST be >= 1, Partition ID Changes Counter MUST be '1', New Parent Counter MUST be '0',
     *     Total Tracking Time MUST be > 4000, Child Role Time MUST be >= 1, Router Role Time MUST be > 3000, Leader
     *     Role Time MUST be '0' Note: tuning above time conditions can still be done later (TBD), after the script has
     *     been run a few times and tighter values can be determined based on the Pcap file.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Leader sends DIAG_GET.qry unicast to DUT's RLOC for Child TLV.");

    /**
     * Step 7
     * - Device: Leader
     * - Description (DIAG-4.1): Harness instructs device to send DIAG_GET.qry unicast to DUT's RLOC for the following
     *   Diagnostic TLV types: Type 29: Child TLV Note: this can done using the OT CLI command “meshdiag childtable”
     * - Pass Criteria:
     *   - N/A
     */

    // Force traffic from children to reset Age before Query
    Node *nodes[] = {&fed1, &med1, &sed1, &reed1};
    for (Node *node : nodes)
    {
        node->SendEchoRequest(router1.Get<Mle::Mle>().GetMeshLocalRloc());
    }
    nexus.AdvanceTime(2000);

    SuccessOrQuit(leader.Get<Utils::MeshDiag>().QueryChildTable(router1.Get<Mle::Mle>().GetRloc16(), nullptr, nullptr));

    /**
     * Step 8
     * - Device: Router_1 (DUT)
     * - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Child information for all its
     *   children.
     * - Pass Criteria:
     *   - The DUT MUST respond with DIAG_GET.rsp
     *   - The response MUST contain four (4) times a Child TLV (type 29) with following fields per TLV. The order is
     *     not important.
     *   - Child TLV for FED_1: R == 1, D == 1, N == 1, C == 0, E MAY be 0 or 1, RLOC16 == <RLOC16 of FED_1>, Extended
     *     Address == <Extended Address of FED_1>, Thread Version == <Thread Version of FED_1>, Timeout == <configured
     *     Child Timeout value of FED_1>, Age MUST be <= 20, Connection Time MUST be <= 20, Supervision Interval == 0,
     *     Link Margin MUST be > 10 and MUST be < 120, Average RSSI MUST be < 0 and MUST be > -120, Last RSSI MUST be <
     *     0 and MUST be > -120, Frame Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be <
     *     0x8000), Message Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be < 0x8000),
     *     Queued Message Count == 0, CSL Period == 0, CSL Timeout == 0, CSL Channel == 0
     *   - Child TLV for MED_1: R == 1, D == 0, N MAY be 0 or 1, C == 0, E MUST be same value as ‘E’ for FED_1, RLOC16
     *     == <RLOC16 of MED_1>, Extended Address == <Extended Address of MED_1>, Thread Version == <Thread Version of
     *     MED_1>, Timeout == <configured Child Timeout value of MED_1>, Age MUST be <= 20, Connection Time MUST be <=
     *     20, Supervision Interval == 0, Link Margin MUST be > 10 and MUST be < 120, Average RSSI MUST be < 0 and MUST
     *     be > -120, Last RSSI MUST be < 0 and MUST be > -120, Frame Error Rate (In case E == 0, field MUST be ‘0’, In
     *     case E == 1, field MUST be < 0x8000), Message Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1,
     *     field MUST be < 0x8000), Queued Message Count == 0, CSL Period == 0, CSL Timeout == 0, CSL Channel == 0
     *   - Child TLV for SED_1: R == 0, D == 0, N MAY be 0 or 1, C == 0, E MUST be same value as ‘E’ for FED_1, RLOC16
     *     == <RLOC16 of SED_1>, Extended Address == <Extended Address of SED_1>, Thread Version == <Thread Version of
     *     SED_1>, Timeout == 312, Age MUST be <= 20, Connection Time MUST be <= 20, Supervision Interval == 129, Link
     *     Margin MUST be > 10 and MUST be < 120, Average RSSI MUST be < 0 and MUST be > -120, Last RSSI MUST be < 0
     *     and MUST be > -120, Frame Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be <
     *     0x8000), Message Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be < 0x8000),
     *     Queued Message Count MUST be < 10, CSL Period == 0, CSL Timeout == 0, CSL Channel == 0
     *   - Child TLV for REED_1: R == 1, D == 1, N == 1, C == 0, E MUST be same value as ‘E’ for FED_1, RLOC16 ==
     *     <RLOC16 of REED_1>, Extended Address == <Extended Address of REED_1>, Thread Version == <Thread Version of
     *     REED_1>, Timeout == <configured Child Timeout value of REED_1>, Age MUST be <= 20, Connection Time MUST be
     *     <= 20, Supervision Interval == 0, Link Margin MUST be > 10 and MUST be < 120, Average RSSI MUST be < 0 and
     *     MUST be > -120, Last RSSI MUST be < 0 and MUST be > -120, Frame Error Rate (In case E == 0, field MUST be
     *     ‘0’, In case E == 1, field MUST be < 0x8000), Message Error Rate (In case E == 0, field MUST be ‘0’, In case
     *     E == 1, field MUST be < 0x8000), Queued Message Count == 0, CSL Period == 0, CSL Timeout == 0, CSL Channel
     *     == 0
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Leader sends DIAG_GET.qry unicast to DUT's RLOC for Child IPv6 Address List TLV.");

    /**
     * Step 9
     * - Device: Leader
     * - Description (DIAG-4.1): Harness instructs device to send DIAG_GET.qry unicast to DUT's RLOC for the following
     *   Diagnostic TLV types: Type 30: Child IPv6 Address List TLV Note: this can done using the OT CLI command
     *   “meshdiag childip6”.
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(
        leader.Get<Utils::MeshDiag>().QueryChildrenIp6Addrs(router1.Get<Mle::Mle>().GetRloc16(), nullptr, nullptr));

    /**
     * Step 10
     * - Device: Router_1 (DUT)
     * - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Child IPv6 address information
     *   for all its MTD children.
     * - Pass Criteria:
     *   - The DUT MUST respond with DIAG_GET.rsp
     *   - The response MUST contain four (2) times a Child IPv6 Address TLV (type 30) with following fields per TLV.
     *     The order is not important.
     *   - Child IPv6 TLV for MED_1: RLOC16 == <RLOC16 of MED_1>, The IPv6 Address N fields MUST include: ML-EID of
     *     MED_1, OMR Address of MED_1, The IPv6 Address N fields MUST NOT include an RLOC.
     *   - Child IPv6 TLV for SED_1: RLOC16 == <RLOC16 of SED_1>, The IPv6 Address N fields MUST include: ML-EID of
     *     SED_1, OMR Address of SED_1, The IPv6 Address N fields MUST NOT include an RLOC.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader sends DIAG_GET.qry unicast to DUT's RLOC for Router Neighbor TLV.");

    /**
     * Step 11
     * - Device: Leader
     * - Description (DIAG-4.1): Harness instructs device to send DIAG_GET.qry unicast to DUT's RLOC for the following
     *   Diagnostic TLV types: Type 31: Router Neighbor TLV Note: this can done using the OT CLI command “meshdiag
     *   routerneighbortable”.
     * - Pass Criteria:
     *   - N/A
     */
    SuccessOrQuit(
        leader.Get<Utils::MeshDiag>().QueryRouterNeighborTable(router1.Get<Mle::Mle>().GetRloc16(), nullptr, nullptr));

    /**
     * Step 12
     * - Device: Router_1 (DUT)
     * - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Router neighbor information from
     *   all its neighbor routers.
     * - Pass Criteria:
     *   - The DUT MUST respond with DIAG_GET.rsp
     *   - THe Response MUST contain one (1) Router Neighbor TLV, with the following values:
     *   - Leader's Router Neighbor TLV: E MAY be 0 or 1, RLOC16 == <RLOC16 of Leader>, Extended Address == <Extended
     *     Address of Leader>, Thread Version == <Thread Version of Leader>, Connection Time MUST be > 4, Link Margin
     *     MUST be > 10 and MUST be < 120, Average RSSI MUST be < 0 and MUST be > -120, Last RSSI MUST be < 0 and MUST
     *     be > -120, Frame Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be < 0x8000),
     *     Message Error Rate (In case E == 0, field MUST be ‘0’, In case E == 1, field MUST be < 0x8000)
     */
    nexus.AdvanceTime(kDiagResponseTime);

    nexus.AdvanceTime(120 * 1000); // Wait for addresses to stabilize

    SaveOmr(nexus, "MED_1_OMR", med1);
    SaveOmr(nexus, "SED_1_OMR", sed1);

    router1.Get<Mle::Mle>().Stop();
    leader.Get<Mle::Mle>().Stop();
    fed1.Get<Mle::Mle>().Stop();
    med1.Get<Mle::Mle>().Stop();
    sed1.Get<Mle::Mle>().Stop();
    reed1.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(1000);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFile = (argc > 1) ? argv[argc - 1] : "test_1_3_DIAG_TC_1.json";

    ot::Nexus::TestDiagTc1(jsonFile);
    printf("All tests passed\n");
    return 0;
}
