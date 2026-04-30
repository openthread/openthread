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
 * 8.1. [1.4] [CERT] Attach and connectivity (unicast/broadcast) between multi-radio and single-radio devices
 *
 * 8.1.1. Purpose
 * To test the following:
 *   1. Attaching Thread Devices with different radio links.
 *   2. Verifies the detection of radio link types supported by neighbors/children.
 *   3. Verifies connectivity between nodes (unicast and broadcast frame exchange with multi-radio support).
 *
 * 8.1.2. Topology
 * - BR Leader (DUT) - Support multi-radio (15.4 and TREL).
 * - Router_1 - Reference device that supports multi-radio (15.4 and TREL).
 * - Router_2 - Reference device that supports 15.4 radio only.
 *
 * Spec Reference           | V1.1 Section | V1.3.0 Section
 * -------------------------|--------------|---------------
 * TREL / Multi-radio Links | N/A          | 8.1
 */

#if OPENTHREAD_CONFIG_MULTI_RADIO

static constexpr uint32_t kFormNetworkTime     = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime  = 200 * 1000;
static constexpr uint32_t kStabilizationTime   = 30 * 1000;
static constexpr uint32_t kInfraIfIndex        = 1;
static constexpr uint16_t kPingPayloadSize     = 500;
static constexpr uint32_t kPingResponseTimeout = 10000;

void Test1_4_Trel_Tc_1(void)
{
    Core  nexus;
    Node &br      = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    br.SetName("BR_DUT");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1
     * - Device: BR (DUT), Router_1, Router_2
     * - Description (TREL-8.1): Form the topology. Wait for Router_1 and Router_2 to become routers. Note: Router_1
     *   and Router_2 MUST NOT be able to communicate directly and not be in each other’s neighbor table (e.g. this
     *   may be realized by using the allow/deny list mechanism).
     * - Pass Criteria:
     *   - Verify that topology is formed.
     *   - The DUT MUST be the network Leader.
     *   - Router_1 MUST only see Leader as its immediate neighbor (Router_2 MUST not be present in Router_1’s
     *     neighbor table).
     *   - Router_2 MUST only see Leader as its immediate neighbor and not Router_1.
     */
    Log("Step 1: Form the topology");

    SuccessOrQuit(br.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(router1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    AllowLinkBetween(br, router1);
    AllowLinkBetween(br, router2);

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    router1.Join(br);
    router2.Join(br);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    // Verify neighbor tables
    VerifyOrQuit(router1.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress()) != nullptr);
    VerifyOrQuit(router1.Get<NeighborTable>().FindNeighbor(router2.Get<Mac::Mac>().GetExtAddress()) == nullptr);
    VerifyOrQuit(router2.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress()) != nullptr);
    VerifyOrQuit(router2.Get<NeighborTable>().FindNeighbor(router1.Get<Mac::Mac>().GetExtAddress()) == nullptr);

    /**
     * Step 2
     * - Device: Router_1
     * - Description (TREL-8.1): Harness instructs device to report its neighbor multi-radio info (which indicates
     *   which radio links are detected per neighbor/parent)
     * - Pass Criteria:
     *   - Router_1 MUST have correctly detected that the DUT supports both TREL and 15.4 radios (multi-radio neighbor
     *     info).
     *   - The DUT MUST send TREL frames using the correct format
     */
    Log("Step 2: Router_1 reports its neighbor multi-radio info");

    {
        const Neighbor *neighbor = router1.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress());

        VerifyOrQuit(neighbor != nullptr);
        VerifyOrQuit(neighbor->GetSupportedRadioTypes().Contains(Mac::kRadioTypeIeee802154));
        VerifyOrQuit(neighbor->GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel));
    }

    /**
     * Step 3
     * - Device: Router_1
     * - Description (TREL-8.1): Harness instructs device to send a ping (of size 500 bytes or more) to the DUT using
     *   the link-local IPv6 address as the destination. Note: The link-local IPv6 address may be derived from its
     *   extended address by retrieving neighbor info on Router_1 (at step 2). A larger ping size is used to verify
     *   behavior with a larger MTU payload on TREL.
     * - Pass Criteria:
     *   - The DUT MUST send a ping response that is successfully received by Router_1.
     *   - The ping response packet MUST NOT be present in the 802.15.4 capture. (I.e. travels over TREL link.)
     *   - The TREL frames sent by DUT MUST use the correct TREL format.
     */
    Log("Step 3: Router_1 pings the DUT using link-local address");
    nexus.SendAndVerifyEchoRequest(router1, br.Get<Mle::Mle>().GetLinkLocalAddress(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 4
     * - Device: Router_2
     * - Description (TREL-8.1): Harness instructs device to send a ping (of size 500 bytes or more) to the DUT using
     *   the link-local IPv6 address as the destination.
     * - Pass Criteria:
     *   - The DUT MUST send a ping response that is successfully received by Router_2.
     */
    Log("Step 4: Router_2 pings the DUT using link-local address");
    nexus.SendAndVerifyEchoRequest(router2, br.Get<Mle::Mle>().GetLinkLocalAddress(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 5
     * - Device: Router_1
     * - Description (TREL-8.1): Harness instructs device to send a ping (of size 500 bytes or more) to Router_2 using
     *   mesh-local EID of Router_2 as the destination. Note: this step indirectly verifies the broadcast
     *   communication by performing address query/resolution.
     * - Pass Criteria:
     *   - The DUT MUST forward a ping response that is successfully received by Router_1
     *   - The TREL frames sent by DUT MUST use the correct TREL frame format.
     */
    Log("Step 5: Router_1 pings Router_2 using mesh-local EID");
    nexus.SendAndVerifyEchoRequest(router1, router2.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 6
     * - Device: Router_2
     * - Description (TREL-8.1): Harness instructs device to send a ping (of size 500 bytes or more) to Router_1 using
     *   mesh-local EID of Router_1 as destination.
     * - Pass Criteria:
     *   - The DUT MUST forward a ping response that is successfully received by Router_2.
     *   - TREL frames sent by DUT MUST use the correct TREL format.
     */
    Log("Step 6: Router_2 pings Router_1 using mesh-local EID");
    nexus.SendAndVerifyEchoRequest(router2, router1.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    nexus.SaveTestInfo("test_1_4_TREL_TC_1.json");
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ot::Nexus::Test1_4_Trel_Tc_1();
    printf("All tests passed\n");
#else
    printf("Multi-radio is not enabled - test skipped\n");
#endif
    return 0;
}
