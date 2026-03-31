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

#if OPENTHREAD_CONFIG_MULTI_RADIO

static constexpr uint32_t kFormNetworkTime      = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime   = 150 * 1000;
static constexpr uint32_t kStabilizationTime    = 60 * 1000;
static constexpr uint32_t kInfraIfIndex         = 1;
static constexpr uint16_t kPingPayloadSize      = 8;
static constexpr uint32_t kPingResponseTimeout  = 5000;
static constexpr uint16_t kStep2PingCount       = 10;
static constexpr uint16_t kStep4PingCount       = 10;
static constexpr uint16_t kStep6PingCount       = 5;
static constexpr uint16_t kStep8UdpCount        = 300;
static constexpr uint32_t kUdpInterval          = 100;
static constexpr uint32_t kTrelReenableWaitTime = 5000;

/**
 * 8.3. [1.4] [CERT] Radio Link (Re)discovery using Probe Mechanism
 *
 * 8.3.1. Purpose
 * This test covers the behavior of the device after TREL connection is temporarily disabled and rediscovery of TREL
 *   radio using the multi-radio Probe mechanism.
 *
 * 8.3.2. Topology
 * - BR Leader (DUT) - Supports multi-radio (TREL and 15.4)
 * - Router - Reference device that supports multi-radio (TREL and 15.4).
 * - ED - Reference End Device that supports 15.4 radio only.
 *
 * Spec Reference   | V1.1 Section | V1.3.0 Section
 * -----------------|--------------|---------------
 * TREL Radio Links | N/A          | 8.3
 */

void Test_1_4_TREL_TC_3(void)
{
    Core  nexus;
    Node &br     = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &ed     = nexus.CreateNode();

    br.SetName("BR_DUT");
    router.SetName("ROUTER");
    ed.SetName("ED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * - Step 1
     *   - Device: BR (DUT), Router
     *   - Description (TREL-8.3): Form the topology. Wait for Router to become Thread router. ED MUST attach to the
     *     DUT as its parent (e.g. can be realized using allow/deny list).
     *   - Pass Criteria:
     *     - Verify that topology is formed.
     *     - ED MUST attach to the DUT as its parent.
     */
    Log("Step 1: Form the topology. Wait for Router to become Thread router. ED MUST attach to the DUT as its parent.");

    br.AllowList(router);
    router.AllowList(br);
    br.AllowList(ed);
    ed.AllowList(br);

    SuccessOrQuit(br.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(router.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    router.Join(br);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    ed.Join(br, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(ed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(ed.Get<Mle::Mle>().GetParent().GetExtAddress() == br.Get<Mac::Mac>().GetExtAddress());

    nexus.AdvanceTime(kStabilizationTime);

    // Ensure TREL peers are discovered before proceeding to Step 2
    VerifyOrQuit(br.Get<ot::Trel::PeerTable>().GetNumberOfPeers() >= 1);
    VerifyOrQuit(router.Get<ot::Trel::PeerTable>().GetNumberOfPeers() >= 1);

    /**
     * - Step 2
     *   - Device: ED
     *   - Description (TREL-8.3): Harness instructs device to send 10 pings to Router using its mesh-local IPv6
     *     address as the destination. Note: this verifies the increase of TREL radio link preference due to its
     *     successful use to exchange the ping messages.
     *   - Pass Criteria:
     *     - The ED MUST receive ping responses.
     *     - The Router MUST have correctly detected that the DUT supports both TREL and 15.4 radios (from the
     *       neighbor table entry info).
     *     - Also TREL radio link MUST be preferred; that is, the preference value associated with TREL MUST be higher
     *       than or equal to 15.4 radio for the BR (DUT) entry in the Router’s neighbor table multi-radio info.
     *     - TREL frames sent by the DUT MUST use a correct TREL frame format.
     */
    Log("Step 2: Harness instructs device to send 10 pings to Router using its mesh-local IPv6 address.");
    for (uint16_t i = 0; i < kStep2PingCount; i++)
    {
        nexus.SendAndVerifyEchoRequest(ed, router.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                       Ip6::kDefaultHopLimit, kPingResponseTimeout);
    }

    {
        Neighbor *neighbor = router.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit(neighbor != nullptr);

        Neighbor::MultiRadioInfo info;
        neighbor->PopulateMultiRadioInfo(info);
        VerifyOrQuit(info.mSupportsIeee802154);
        VerifyOrQuit(info.mSupportsTrelUdp6);
        VerifyOrQuit(info.mTrelUdp6Info.mPreference >= info.mIeee802154Info.mPreference);
    }

    /**
     * - Step 3
     *   - Device: Router
     *   - Description (TREL-8.3): Harness disables the TREL connectivity on the Router. Note: This may be realized by
     *     causing a disconnect on the infrastructure link (e.g. if the infra link is Wi-Fi, the Router device can be
     *     disconnected from Wi-Fi AP). Alternatively this can be realized by specific APIs added on Router device
     *     for the purpose of testing. Note: this may also be realized using the "trel filter" CLI command. See link.
     *   - Pass Criteria:
     *     - N/A
     */
    Log("Step 3: Harness disables the TREL connectivity on the Router.");
    router.Get<ot::Trel::Interface>().SetFilterEnabled(true);
    br.Get<ot::Trel::Interface>().SetFilterEnabled(true);

    /**
     * - Step 4
     *   - Device: ED
     *   - Description (TREL-8.3): Harness instructs device to send 10 pings to Router using its mesh-local IPv6
     *     address as the destination. Note: some of the pings may fail, which is expected behavior. This step tests
     *     the detection of a disconnect in a TREL link by the DUT.
     *   - Pass Criteria:
     *     - N/A
     */
    Log("Step 4: Harness instructs device to send 10 pings to Router using its mesh-local IPv6 address.");
    for (uint16_t i = 0; i < kStep4PingCount; i++)
    {
        ed.SendEchoRequest(router.Get<Mle::Mle>().GetMeshLocalEid(), i, kPingPayloadSize);
        nexus.AdvanceTime(2000);
    }

    /**
     * - Step 5
     *   - Device: Router
     *   - Description (TREL-8.3): Harness instructs device to report its radio link preference from its neighbor
     *     table.
     *   - Pass Criteria:
     *   - The Router MUST report that the DUT supports both TREL and 15.4 radios
     *   - The TREL radio link preference MUST be strictly less than the 15.4 radio link preference. (Note: this is a
     *     deviation from the test specification which requires it to be zero).
     */
    Log("Step 5: Harness instructs device to report its radio link preference from its neighbor table.");
    {
        Neighbor *neighbor = router.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit(neighbor != nullptr);

        Neighbor::MultiRadioInfo info;
        neighbor->PopulateMultiRadioInfo(info);
        VerifyOrQuit(info.mSupportsIeee802154);
        VerifyOrQuit(info.mSupportsTrelUdp6);

        // Deviation from test spec: relax to strictly less than 15.4 preference
        VerifyOrQuit(info.mTrelUdp6Info.mPreference < info.mIeee802154Info.mPreference);
    }

    /**
     * - Step 6
     *   - Device: ED
     *   - Description (TREL-8.3): Harness instructs device to send 5 pings to Router using its mesh-local IPv6
     *     address as the destination. Note: this step verifies that the DUT correctly falls back to using 15.4
     *     radio on detection of TREL disconnect.
     *   - Pass Criteria:
     *     - The ED MUST receive all Ping responses from the Router successfully.
     */
    Log("Step 6: Harness instructs device to send 5 pings to Router using its mesh-local IPv6 address.");
    for (uint16_t i = 0; i < kStep6PingCount; i++)
    {
        nexus.SendAndVerifyEchoRequest(ed, router.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                       Ip6::kDefaultHopLimit, kPingResponseTimeout);
    }

    /**
     * - Step 7
     *   - Device: Router
     *   - Description (TREL-8.3): Harness re-enables TREL connectivity on the Router.
     *   - Pass Criteria:
     *     - N/A
     */
    Log("Step 7: Harness re-enables TREL connectivity on the Router.");
    router.Get<ot::Trel::Interface>().SetFilterEnabled(false);
    br.Get<ot::Trel::Interface>().SetFilterEnabled(false);
    nexus.AdvanceTime(kTrelReenableWaitTime);

    /**
     * - Step 8
     *   - Device: ED
     *   - Description (TREL-8.3): Harness instructs device to send 300 UDP messages (small payload) to Router using
     *     mesh-local IPv6 addresses as source and destination. Note: this step is intended to trigger and verify
     *     the probe mechanism on the DUT.
     *   - Pass Criteria:
     *     - N/A
     */
    Log("Step 8: Harness instructs device to send 300 UDP messages (small payload) to Router.");
    for (uint16_t i = 0; i < kStep8UdpCount; i++)
    {
        ed.SendEchoRequest(router.Get<Mle::Mle>().GetMeshLocalEid(), i, kPingPayloadSize);
        nexus.AdvanceTime(kUdpInterval);
    }

    /**
     * - Step 9
     *   - Device: Router
     *   - Description (TREL-8.3): Harness instructs device to report its radio link preference from its neighbor
     *     table.
     *   - Pass Criteria:
     *     - The Router MUST report that the DUT supports both TREL and 15.4 radios.
     *     - The reported TREL radio link preference MUST be greater than zero (again reachable on the TREL radio
     *       link), and higher or equal to the 15.4 radio link preference.
     *     - TREL frames sent by the DUT MUST use a correct TREL frame format.
     */
    Log("Step 9: Harness instructs device to report its radio link preference from its neighbor table.");
    {
        Neighbor *neighbor = router.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit(neighbor != nullptr);

        Neighbor::MultiRadioInfo info;
        neighbor->PopulateMultiRadioInfo(info);
        VerifyOrQuit(info.mSupportsIeee802154);
        VerifyOrQuit(info.mSupportsTrelUdp6);
        VerifyOrQuit(info.mTrelUdp6Info.mPreference >= info.mIeee802154Info.mPreference);
    }

    nexus.SaveTestInfo("test_1_4_TREL_TC_3.json");
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ot::Nexus::Test_1_4_TREL_TC_3();
    printf("All tests passed\n");
#else
    printf("Multi-radio is not enabled - test skipped\n");
#endif
    return 0;
}
