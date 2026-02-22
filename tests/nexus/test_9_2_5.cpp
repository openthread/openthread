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

#include "meshcop/dataset_manager.hpp"
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
 * Time to advance for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 2000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

static constexpr uint64_t kActiveTimestampStep2 = 20;
static constexpr uint32_t kChannelMaskStep2     = 0x07fff800;
static constexpr uint8_t  kExtendedPanIdStep2[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
static constexpr char     kNetworkNameStep2[]   = "nexus-test";
static constexpr uint8_t  kPskcStep2[]          = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
static constexpr uint16_t kRotationTimeStep2    = 3600;
static constexpr uint8_t  kSecurityFlagsStep2[] = {0xfb};

static constexpr uint64_t kActiveTimestampStep7 = 10;

static constexpr uint64_t kActiveTimestampStep9 = 30;
static constexpr uint32_t kChannelMaskStep9     = 0x001fffe0;
static constexpr uint8_t  kExtendedPanIdStep9[] = {0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
static constexpr char     kNetworkNameStep9[]   = "nexus-925";
static constexpr uint8_t  kPskcStep9[]          = {0x11, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
static constexpr uint16_t kRotationTimeStep9    = 7200;
static constexpr uint8_t  kSecurityFlagsStep9[] = {0x7b};
static constexpr uint8_t  kFutureTlv[]          = {130, 2, 0xaa, 0x55};

static constexpr uint64_t kActiveTimestampStep14 = 40;
static constexpr uint16_t kUnsupportedChannel    = 63;

void Test9_2_5(void)
{
    /**
     * 9.2.5 Updating the Active Operational Dataset via Thread Node
     *
     * 9.2.5.1 Topology
     * - DUT as Leader, Router_1
     *
     * 9.2.5.2 Purpose & Description
     * The purpose of this test case is to verify the DUT’s behavior when receiving MGMT_ACTIVE_SET.req from an active
     *   Thread node.
     *
     * Spec Reference                          | V1.1 Section | V1.3.0 Section
     * ----------------------------------------|--------------|---------------
     * Updating the Active Operational Dataset | 8.7.4        | 8.7.4
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    leader.AllowList(router1);
    router1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1");

    /**
     * Step 2: Router_1
     * - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
     *   Locator:
     *   - new, valid Timestamp TLV
     *   - all valid Active Operational Dataset parameters, with new values in the TLVs that don’t affect connectivity
     * - Pass Criteria:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Active Timestamp TLV (new valid value)
     *     - Channel Mask TLV (new value)
     *     - Extended PAN ID TLV (new value)
     *     - Mesh-Local Prefix (old value)
     *     - Network Name TLV (new value)
     *     - PSKc TLV (new value)
     *     - Security Policy TLV (new value)
     *     - Network Master Key (old value)
     *     - PAN ID (old value)
     *     - Channel (old value)
     *   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    MeshCoP::Dataset::Info datasetInfo;
    MeshCoP::Timestamp     timestamp;

    SuccessOrQuit(router1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

    timestamp.SetSeconds(kActiveTimestampStep2);
    timestamp.SetTicks(0);
    datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    datasetInfo.Set<MeshCoP::Dataset::kChannelMask>(kChannelMaskStep2);
    datasetInfo.Set<MeshCoP::Dataset::kExtendedPanId>(
        AsCoreType(reinterpret_cast<const otExtendedPanId *>(kExtendedPanIdStep2)));
    SuccessOrQuit(datasetInfo.Update<MeshCoP::Dataset::kNetworkName>().Set(kNetworkNameStep2));
    datasetInfo.Set<MeshCoP::Dataset::kPskc>(AsCoreType(reinterpret_cast<const otPskc *>(kPskcStep2)));
    datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().mRotationTime = kRotationTimeStep2;
    datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(kSecurityFlagsStep2, sizeof(kSecurityFlagsStep2));

    SuccessOrQuit(
        router1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(datasetInfo, nullptr, 0, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Router_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader (DUT)");

    /**
     * Step 4: Leader (DUT)
     * - Description: Automatically sends a Multicast MLE Data Response.
     * - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version field [incremented]
     *     - Stable Version field [incremented]
     *   - Network Data TLV
     *   - Active Timestamp TLV [new value set in Step 2]
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_1");

    /**
     * Step 5: Router_1
     * - Description: Automatically sends a unicast MLE Data Request to Leader, including the following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to Router_1.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Router_1, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *   - Active Operational Dataset TLV
     *     - Channel TLV
     *     - Channel Mask TLV [new value set in Step 2]
     *     - Extended PAN ID TLV [new value set in Step 2]
     *     - Network Mesh-Local Prefix TLV
     *     - Network Master Key TLV
     *     - Network Name TLV [new value set in Step 2]
     *     - PAN ID TLV
     *     - PSKc TLV [new value set in Step 2]
     *     - Security Policy TLV [new value set in Step 2]
     *   - Active Timestamp TLV [new value set in Step 2]
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1");

    /**
     * Step 7: Router_1
     * - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
     *   Locator:
     *   - old, invalid Active Timestamp TLV
     *   - all valid Active Operational Dataset parameters, with new values in the TLVs that don’t affect connectivity
     * - Pass Criteria:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Active Timestamp TLV (old, invalid value)
     *     - Channel Mask TLV (new value)
     *     - Extended PAN ID TLV (new value)
     *     - Mesh-Local Prefix (old value)
     *     - Network Name TLV (new value)
     *     - PSKc TLV (new value)
     *     - Security Policy TLV (new value)
     *     - Network Master Key (old value)
     *     - PAN ID (old value)
     *     - Channel (old value)
     *   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    timestamp.SetSeconds(kActiveTimestampStep7);
    datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

    SuccessOrQuit(
        router1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(datasetInfo, nullptr, 0, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1, with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1");

    /**
     * Step 9: Router_1
     * - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
     *   Locator:
     *   - new, valid Active Timestamp TLV
     *   - all of valid Commissioner Dataset parameters plus one bogus TLV, and new values in the TLVs that don’t affect
     *     connectivity
     * - Pass Criteria:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Active Timestamp TLV (new, valid value)
     *     - Channel Mask TLV (new value, different from Step 2)
     *     - Extended PAN ID TLV (new value, different from Step 2)
     *     - Mesh-Local Prefix (old value)
     *     - Network Name TLV (new value, different from Step 2)
     *     - PSKc TLV (new value, different from Step 2)
     *     - Security Policy TLV (new value, different from Step 2)
     *     - Network Master Key (old value)
     *     - PAN ID (old value)
     *     - Channel (old value)
     *     - Future TLV:
     *       - Type 130
     *       - Length 2
     *       - Value (aa 55)
     *   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    timestamp.SetSeconds(kActiveTimestampStep9);
    datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    datasetInfo.Set<MeshCoP::Dataset::kChannelMask>(kChannelMaskStep9);
    datasetInfo.Set<MeshCoP::Dataset::kExtendedPanId>(
        AsCoreType(reinterpret_cast<const otExtendedPanId *>(kExtendedPanIdStep9)));
    SuccessOrQuit(datasetInfo.Update<MeshCoP::Dataset::kNetworkName>().Set(kNetworkNameStep9));
    datasetInfo.Set<MeshCoP::Dataset::kPskc>(AsCoreType(reinterpret_cast<const otPskc *>(kPskcStep9)));
    datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().mRotationTime = kRotationTimeStep9;
    datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(kSecurityFlagsStep9, sizeof(kSecurityFlagsStep9));

    SuccessOrQuit(router1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(datasetInfo, kFutureTlv,
                                                                              sizeof(kFutureTlv), nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (01))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Leader (DUT)");

    /**
     * Step 11: Leader (DUT)
     * - Description: Automatically sends a multicast MLE Data Response.
     * - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *     - Data version field [incremented]
     *     - Stable Version field [incremented]
     *   - Network Data TLV
     *   - Active Timestamp TLV [new value set in Step 9]
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1");

    /**
     * Step 12: Router_1
     * - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT), including the following TLVs:
     *   - TLV Request TLV:
     *     - Network Data TLV
     *   - Active Timestamp TLV
     * - Pass Criteria: N/A.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Leader (DUT)");

    /**
     * Step 13: Leader (DUT)
     * - Description: Automatically sends a unicast MLE Data Response to Router_1.
     * - Pass Criteria: The following TLVs MUST be included in the Unicast MLE Data Response:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Network Data TLV
     *   - Stable flag set to 0
     *   - Active Operational Dataset TLV
     *     - Channel TLV
     *     - Channel Mask TLV [new value set in Step 9]
     *     - Extended PAN ID TLV [new value set in Step 9]
     *     - Network Mesh-Local Prefix TLV
     *     - Network Master Key TLV
     *     - Network Name TLV [new value set in Step 9]
     *     - PAN ID TLV
     *     - PSKc TLV [new value set in Step 9]
     *     - Security Policy TLV [new value set in Step 9]
     *   - Active Timestamp TLV [new value set in Step 9]
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_1");

    /**
     * Step 14: Router_1
     * - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
     *   Locator:
     *   - new, valid Active Timestamp TLV
     *   - attempt to set Channel TLV to an unsupported channel + all of other TLVs
     * - Pass Criteria:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Active Timestamp TLV (new, valid value)
     *     - Channel TLV (unsupported value = 63)
     *     - Channel Mask TLV (old value set in Step 9)
     *     - Extended PAN ID TLV (old value set in Step 9)
     *     - Mesh-Local Prefix (old value)
     *     - Network Name TLV (old value set in Step 9)
     *     - PSKc TLV (old value set in Step 9)
     *     - Security Policy TLV (old value set in Step 9)
     *     - Network Master Key (old value)
     *     - PAN ID (old value)
     *   - The DUT Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     */

    timestamp.SetSeconds(kActiveTimestampStep14);
    datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
    datasetInfo.Set<MeshCoP::Dataset::kChannel>(kUnsupportedChannel);

    SuccessOrQuit(
        router1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(datasetInfo, nullptr, 0, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Leader (DUT)");

    /**
     * Step 15: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Router_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Reject (ff))
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: All");

    /**
     * Step 16: All
     * - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
     * - Pass Criteria: The DUT must respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(router1, leader.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_5();
    printf("All tests passed\n");
    return 0;
}
