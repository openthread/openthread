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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/network_diagnostic.hpp"

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
 * Time to wait in Step 4, in milliseconds.
 */
static constexpr uint32_t kWaitTime = 4 * 1000;

void TestDiagTc2(const char *aJsonFile)
{
    /**
     * 4.2. [1.4] [CERT] Get Diagnostics – End Device
     *
     * 4.2.2. Topology
     * - Leader_1
     * - Router_1
     * - TD_1 (DUT)
     *
     * Spec Reference   | Section
     * -----------------|---------
     * Thread 1.4       | 4.2
     */

    Core nexus;

    Node &leader1 = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &td1     = nexus.CreateNode();

    leader1.SetName("Leader_1");
    router1.SetName("Router_1");
    td1.SetName("TD_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * - Leader_1 and Router_1
     * - Router_1 and TD_1
     */
    leader1.AllowList(router1);
    router1.AllowList(leader1);

    router1.AllowList(td1);
    td1.AllowList(router1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Enable the devices in order. Leader configures its Network Data with an OMR prefix.");

    /**
     * Step 1
     * - Device: Leader, Router_1
     * - Description (DIAG-4.2): Enable the devices in order. The DUT is not yet activated. Leader
     *   configures its Network Data with an OMR prefix. Note: the OMR prefix can be added using an
     *   OT CLI command such as: "prefix add 2001:dead:beef:cafe::/64 paros med"
     * - Pass Criteria:
     *   - Single Thread Network is formed with Leader and Router_1.
     */
    leader1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader1.Get<Mle::Mle>().IsLeader());

    router1.Join(leader1);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    NetworkData::OnMeshPrefixConfig omrPrefixConfig;
    SuccessOrQuit(omrPrefixConfig.GetPrefix().FromString("2001:dead:beef:cafe::/64"));
    omrPrefixConfig.mPreferred    = true;
    omrPrefixConfig.mSlaac        = true;
    omrPrefixConfig.mDhcp         = false;
    omrPrefixConfig.mDefaultRoute = true;
    omrPrefixConfig.mOnMesh       = true;
    omrPrefixConfig.mStable       = true;
    omrPrefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;
    SuccessOrQuit(leader1.Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig));
    leader1.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(30 * 1000); // Wait for network data propagation

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1b: TD_1 (DUT) Enable.");

    /**
     * Step 1b
     * - Device: TD_1 (DUT)
     * - Description (DIAG-4.2): Enable.
     * - Pass Criteria:
     *   - N/A
     */
    td1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(td1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader instructs device to send DIAG_GET.req to DUT's RLOC for Diagnostic TLV types.");

    /**
     * Step 2
     * - Device: Leader
     * - Description (DIAG-4.2): Harness instructs device to send DIAG_GET.req to DUT's RLOC for the
     *   following Diagnostic TLV types: Type 19: Max Child Timeout TLV, Type 23: EUI-64 TLV, Type
     *   24: Version TLV, Type 25: Vendor Name TLV, Type 26: Vendor Model TLV, Type 27: Vendor SW
     *   Version TLV, Type 28: Thread Stack Version TLV
     * - Pass Criteria:
     *   - N/A
     */
    Ip6::Address td1Rloc;
    td1Rloc.SetToRoutingLocator(leader1.Get<Mle::Mle>().GetMeshLocalPrefix(), td1.Get<Mle::Mle>().GetRloc16());

    uint8_t tlvTypesStep2[] = {
        NetworkDiagnostic::Tlv::kMaxChildTimeout,
        NetworkDiagnostic::Tlv::kEui64,
        NetworkDiagnostic::Tlv::kVersion,
        NetworkDiagnostic::Tlv::kVendorName,
        NetworkDiagnostic::Tlv::kVendorModel,
        NetworkDiagnostic::Tlv::kVendorSwVersion,
        NetworkDiagnostic::Tlv::kThreadStackVersion,
    };
    SuccessOrQuit(leader1.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(td1Rloc, tlvTypesStep2,
                                                                             sizeof(tlvTypesStep2), nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: TD_1 (DUT) Automatically responds either with 1. DIAG_GET.rsp or 2. CoAP response 4.04.");

    /**
     * Step 3
     * - Device: TD_1 (DUT)
     * - Description (DIAG-4.2): Automatically responds either with 1. DIAG_GET.rsp containing the
     *   requested Diagnostic TLVs, or a particular subset of those TLVs; 2. CoAP response 4.04 Not
     *   Found in case no diagnostics are supported at all.
     * - Pass Criteria:
     *   - For case 1, the CoAP status code MUST be 2.04 Changed.
     *   - Each TLV (as requested in step 2) that is present in the response MUST be validated. Any
     *     number of TLVs MAY be present, from 0 to 7, depending on DUT's diagnostics support level.
     *   - The Max Child Timeout TLV (Type 19) is not expected to be present. However, if present it
     *     MUST be '0' (zero).
     *   - Value of EUI-64 TLV (Type 23) MUST be non-zero.
     *   - Value of Version TLV (Type 24) MUST be >= '5'.
     *   - Value of Vendor Name TLV (Type 25) MUST have length >= 1.
     *   - Value of Vendor Model TLV (Type 26) MUST have length >= 1.
     *   - Value of Vendor SW Version TLV (Type 27) MUST have length >= 5.
     *   - Value of Thread Stack Version TLV (Type 28) MUST have length >= 5.
     *   - For case 2, the CoAP status code is 4.04 and TLVs MUST NOT be present.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Harness waits for 4 seconds.");

    /**
     * Step 4
     * - Device: N/A
     * - Description (DIAG-4.2): Harness waits for 4 seconds. Note: this is done only to influence
     *   diagnostic Time fields that are verified in later steps.
     * - Pass Criteria:
     *   - N/A
     */
    nexus.AdvanceTime(kWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader instructs device to send DIAG_GET.req unicast to DUT's RLOC for MLE Counters TLV.");

    /**
     * Step 4
     * - Device: Leader
     * - Description (DIAG-4.2): Harness instructs device to send DIAG_GET.req unicast to DUT's RLOC
     *   for the following Diagnostic TLV types: Type 34: MLE Counters TLV
     * - Pass Criteria:
     *   - N/A
     */
    uint8_t tlvTypesStep4[] = {NetworkDiagnostic::Tlv::kMleCounters};
    SuccessOrQuit(leader1.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(td1Rloc, tlvTypesStep4,
                                                                             sizeof(tlvTypesStep4), nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: TD_1 (DUT) Automatically responds with DIAG_GET.rsp containing the requested Diagnostic TLVs.");

    /**
     * Step 5
     * - Device: TD_1 (DUT)
     * - Description (DIAG-4.2): Automatically responds with DIAG_GET.rsp containing the requested
     *   Diagnostic TLVs.
     * - Pass Criteria:
     *   - The DUT must send DIAG_GET.rsp
     *   - The presence of each TLV (as requested in the previous step) MUST be validated.
     *   - In the MLE Counters TLV (Type 34):
     *   - Radio Disabled Counter MUST be '0'
     *   - Detached Role Counter MUST be '1'
     *   - Child Role Counter MUST be '1'
     *   - Router Role Counter MUST be '0'
     *   - Leader Role Counter MUST be '0'
     *   - Attach Attempts Counter MUST be >= 1
     *   - Partition ID Changes Counter MUST be '1'
     *   - New Parent Counter MUST be '0'
     *   - Total Tracking Time MUST be > 4000
     *   - Child Role Time MUST be >= 1
     *   - Router Role Time MUST be '0'
     *   - Leader Role Time MUST be '0'
     */
    nexus.AdvanceTime(kDiagResponseTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    const char *jsonFile = (argc > 1) ? argv[argc - 1] : "test_1_3_DIAG_TC_2.json";

    ot::Nexus::TestDiagTc2(jsonFile);
    printf("All tests passed\n");
    return 0;
}
