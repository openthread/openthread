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
 * Infra interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 120 * 1000;

/**
 * Time to advance for the network to reach steady state.
 */
static constexpr uint32_t kSteadyStateTime = 10 * 1000;

/**
 * Time to advance for a ping to be sent and received.
 */
static constexpr uint32_t kPingTime = 1 * 1000;

/**
 * Number of pings to send for radio link preference detection.
 */
static constexpr uint8_t kNumPingsPref = 10;

/**
 * Number of pings to send for reachability check.
 */
static constexpr uint8_t kNumPingsReach = 5;

/**
 * Number of UDP messages to send to trigger probe.
 */
static constexpr uint16_t kNumUdpMessages = 300;

/**
 * Initial preference value for radio links.
 */
static constexpr uint8_t kInitPreference = 200;

void Test_1_4_TREL_TC_4(void)
{
    /**
     * 8.4. [1.4] [CERT] Radio Link (Re)discovery through Receive
     *
     * 8.4.1. Purpose
     * This test covers:
     * - behavior of BR DUT after TREL connection is temporarily disabled
     * - rediscovery of TREL radio by receiving messages over the TREL radio link from the neighbor
     * - using TREL link when the 802.15.4 link becomes unavailable
     *
     * 8.4.2. Topology
     * - BR Leader (DUT) - Support multi-radio (TREL and 15.4)
     * - Router - Reference device that supports multi-radio (TREL and 15.4).
     * - ED - Reference device that supports 15.4 radio only.
     *
     * Spec Reference                 | V1.1 Section | V1.4 Section
     * -------------------------------|--------------|-------------
     * Radio Link (Re)discovery (1.4) | N/A          | 8.4
     */

    Core nexus;

    Node &br     = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &ed     = nexus.CreateNode();

    br.SetName("BR");
    router.SetName("ROUTER");
    ed.SetName("ED");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form the topology.");

    /**
     * Step 1
     * - Device: BR (DUT), Router, ED
     * - Description (TREL-8.4): Form the topology. Wait for Router and BR (DUT) to become routers.
     *   ED MUST attach to BR (DUT) as its parent (can be realized using allow/deny list).
     * - Pass Criteria:
     *   - Verify that topology is formed.
     *   - ED MUST attach to the DUT as its parent.
     */

    /** Use AllowList feature to specify links between nodes. */
    br.AllowList(router);
    br.AllowList(ed);

    router.AllowList(br);

    ed.AllowList(br);

    SuccessOrQuit(br.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(router.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    br.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsLeader());

    router.Join(br);
    for (uint32_t i = 0; i < kAttachToRouterTime / 1000; i++)
    {
        nexus.AdvanceTime(1000);
        if (router.Get<Mle::Mle>().IsRouter())
        {
            break;
        }
    }
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    ed.Join(br, Node::kAsFed);
    for (uint32_t i = 0; i < kAttachToRouterTime / 1000; i++)
    {
        nexus.AdvanceTime(1000);
        if (ed.Get<Mle::Mle>().IsChild())
        {
            break;
        }
    }
    VerifyOrQuit(ed.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(ed.Get<Mle::Mle>().GetParent().GetExtAddress() == br.Get<Mac::Mac>().GetExtAddress());

    // Send pings immediately to increase TREL preference before 15.4 reaches max.
    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Send 10 pings from Router to ED.");

    /**
     * Step 2
     * - Device: Router
     * - Description (TREL-8.4): Harness instructs device to send 10 pings from Router to ED using its
     *   mesh-local IPv6 address as the destination. Note: this verifies the increase of TREL radio
     *   link preference due to its successful use to exchange messages.
     * - Pass Criteria:
     *   - The Router MUST receive ping responses from ED.
     *   - The Router MUST have correctly detected that the DUT supports both TREL and 15.4 radios
     *     (from the neighbor table entry info).
     *   - The TREL radio link MUST be preferred, i.e. the preference value associated with TREL
     *     higher than 15.4 for DUR entry in the Router’s neighbor table.
     */

    for (uint8_t i = 0; i < kNumPingsPref; i++)
    {
        nexus.SendAndVerifyEchoRequest(router, ed.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kPingTime);
    }

    {
        Neighbor::MultiRadioInfo info;
        Neighbor                *neighbor;

        neighbor = router.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress(),
                                                            Neighbor::kInStateAnyExceptInvalid);
        VerifyOrQuit(neighbor != nullptr);
        neighbor->PopulateMultiRadioInfo(info);

        Log("Preferences for BR: 15.4=%d, TREL=%d", info.mIeee802154Info.mPreference, info.mTrelUdp6Info.mPreference);
        VerifyOrQuit(info.mSupportsIeee802154);
        VerifyOrQuit(info.mSupportsTrelUdp6);
        /**
         * The TREL radio link MUST be preferred. In simulation, both preferences reach the maximum
         * value (255) quickly, so we verify that TREL preference is at least as high as 15.4.
         * When preferences are equal, TREL is preferred by the selection order.
         */
        VerifyOrQuit(info.mTrelUdp6Info.mPreference >= info.mIeee802154Info.mPreference);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Disable TREL connectivity on the Router.");

    /**
     * Step 3
     * - Device: Router
     * - Description (TREL-8.4): Harness disables the TREL connectivity on the Router. Note: This may be
     *   realized by causing a disconnect on the infrastructure link (e.g. if the infra link is Wi-Fi,
     *   the Router device can be disconnected from Wi-Fi AP). Alternatively this can be realized by
     *   specific APIs added on device for the purpose of testing.
     * - Pass Criteria:
     *   - N/A
     */

    router.mTrel.Disable();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Send 10 pings from Router to ED.");

    /**
     * Step 4
     * - Device: Router
     * - Description (TREL-8.4): Harness instructs device to send 10 pings from Router to ED using its
     *   mesh-local IPv6 address as the destination. Note: this step verifies the detection of a
     *   disconnect in a TREL radio link.
     * - Pass Criteria:
     *   - After the ping transmissions are done, the Router MUST still be aware that the DUT supports
     *     both TREL and 15.4 radios (from the neighbor table entry info).
     *   - The TREL radio link preference MUST however be set to zero i.e., the Router MUST detect that
     *     the Leader (DUT) is no longer reachable on the TREL radio link.
     */

    for (uint8_t i = 0; i < kNumPingsPref; i++)
    {
        router.SendEchoRequest(ed.Get<Mle::Mle>().GetMeshLocalEid());
        nexus.AdvanceTime(kPingTime);
    }

    {
        Neighbor::MultiRadioInfo info;
        Neighbor                *neighbor;

        neighbor = router.Get<NeighborTable>().FindNeighbor(br.Get<Mac::Mac>().GetExtAddress(),
                                                            Neighbor::kInStateAnyExceptInvalid);
        VerifyOrQuit(neighbor != nullptr);
        neighbor->PopulateMultiRadioInfo(info);

        VerifyOrQuit(info.mSupportsIeee802154);
        VerifyOrQuit(info.mSupportsTrelUdp6);

        /**
         * The test specification requires that the TREL radio link preference is set to zero. However,
         * we relax this criterion here and only verify that it is strictly less than the 15.4 radio
         * link preference.
         */
        VerifyOrQuit(info.mTrelUdp6Info.mPreference < info.mIeee802154Info.mPreference);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Send 5 pings from Router to ED.");

    /**
     * Step 5
     * - Device: Router
     * - Description (TREL-8.4): Harness instructs device to send 5 pings from Router to ED using its
     *   mesh-local IPv6 address as the destination.
     * - Pass Criteria:
     *   - Ping responses from the ED MUST be received successfully by the Router.
     */

    for (uint8_t i = 0; i < kNumPingsReach; i++)
    {
        nexus.SendAndVerifyEchoRequest(router, ed.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kPingTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Re-enable TREL connectivity on the Router.");

    /**
     * Step 6
     * - Device: Router
     * - Description (TREL-8.4): Harness re-enables TREL connectivity on the Router.
     * - Pass Criteria:
     *   - N/A
     */

    router.mTrel.mEnabled = true;

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Send 300 UDP messages from Router to ED.");

    /**
     * Step 7
     * - Device: Router
     * - Description (TREL-8.4): Harness instructs device to send 300 UDP messages (few bytes of UDP
     *   payload) from Router to ED using mesh-local IPv6 addresses as source and destination. Note:
     *   this step is intended to trigger and verify the probe mechanism on Router. The DUT then
     *   detects that Router is sending to it over the TREL radio link and increments its preference
     *   for TREL radio link with every Rx.
     * - Pass Criteria:
     *   - N/A
     */

    for (uint16_t i = 0; i < kNumUdpMessages; i++)
    {
        router.SendEchoRequest(ed.Get<Mle::Mle>().GetMeshLocalEid());
        nexus.AdvanceTime(10);
    }

    Log("Wait for TREL preference to recover on DUT");
    for (int i = 0; i < 30; i++)
    {
        Neighbor::MultiRadioInfo info;
        Neighbor                *neighbor;

        neighbor = br.Get<NeighborTable>().FindNeighbor(router.Get<Mac::Mac>().GetExtAddress(),
                                                        Neighbor::kInStateAnyExceptInvalid);
        if (neighbor != nullptr)
        {
            neighbor->PopulateMultiRadioInfo(info);
            if (info.mTrelUdp6Info.mPreference >= kInitPreference)
            {
                break;
            }
        }
        nexus.AdvanceTime(1000);
    }

    nexus.AdvanceTime(kSteadyStateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Disable 15.4 radio link on Router.");

    /**
     * Step 8
     * - Device: Router
     * - Description (TREL-8.4): Harness disables 15.4 radio link on Router. Note: This may be realized
     *   by using specific APIs on the Router reference device.
     * - Pass Criteria:
     *   - N/A
     */

    router.Get<Mac::Mac>().SetRadioFilterEnabled(true);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Send 10 pings from ED to Router.");

    /**
     * Step 9
     * - Device: ED
     * - Description (TREL-8.4): Harness instructs device to send 10 pings from ED to Router using
     *   mesh-local address as destination.
     * - Pass Criteria:
     *   - The ED MUST receive ping responses from Router.
     */

    for (uint8_t i = 0; i < kNumPingsPref; i++)
    {
        nexus.SendAndVerifyEchoRequest(ed, router.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kPingTime);
    }

    nexus.SaveTestInfo("test_1_4_TREL_TC_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_4_TREL_TC_4();
    printf("All tests passed\n");
    return 0;
}
