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

static constexpr uint32_t kFormNetworkTime     = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime  = 150 * 1000;
static constexpr uint32_t kStabilizationTime   = 60 * 1000;
static constexpr uint32_t kInfraIfIndex        = 1;
static constexpr uint16_t kPingPayloadSize     = 500;
static constexpr uint32_t kPingResponseTimeout = 10000;

void Test_1_4_TREL_TC_2(void)
{
    /**
     * 8.2. [1.4] [CERT] Multi-hop routing with device (in the path) with different radio types and MTUs
     *
     * 8.2.1. Purpose
     * This test covers the use of 6LoWPAN “Mesh Header” messages (messages sent over multi-hop) when
     * underlying routes (in the path) can support different radio link types with different frame MTU sizes.
     * Different types of devices (15.4, TREL, Router, End Device) are connected to the DUT.
     *
     * 8.2.2. Topology
     * - BR Router_1 (DUT) - Border Router DUT that supports multi-radio (TREL and 15.4)
     * - Leader - Reference device that supports multi-radio (TREL and 15.4).
     * - Router_2 - Reference device that supports 15.4 only
     * - ED_1 - Reference end device that supports 15.4 only
     * - Router_3 - Reference device that supports 15.4 only
     * - Router_4 - Reference device that supports multi-radio (TREL and 15.4).
     *
     * Spec Reference           | V1.1 Section | V1.3.0 Section
     * -------------------------|--------------|---------------
     * TREL / Multi-radio Links | N/A          | 8.2
     */

    Core  nexus;
    Node &router1 = nexus.CreateNode(); // DUT
    Node &leader  = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &ed1     = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &router4 = nexus.CreateNode();

    router1.SetName("ROUTER_1");
    leader.SetName("LEADER");
    router2.SetName("ROUTER_2");
    ed1.SetName("ED_1");
    router3.SetName("ROUTER_3");
    router4.SetName("ROUTER_4");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1
     * - Device: Leader, Router_1 (DUT), Router_2, ED_1, Router_3, Router_4
     * - Description (TREL-8.2): Form the topology. Wait for Router_1, Router_2, Router_3 and Router_4 to become
     *   routers. Note: any devices not connected by line in the topology figure MUST NOT be able to communicate
     *   directly via a Thread Link or TREL link and not be in each other’s neighbor table (e.g. this may be realized
     *   by using the allow/deny list mechanism). ED_1 MUST be configured as a non-sleepy MTD child (mode `rn`). Note:
     *   ED_1 MUST attach to the Router_1 (DUT) as its parent and not the other routers (e.g., can be realized by using
     *   denylist on ED_1). Note: The requirement for ED_1 to attach as a non-sleepy MTD child to Router_1 (DUT) is
     *   important to ensure messages from ED_1 are sent to its parent as is without mesh header encapsulation, to
     *   then be forwarded within the mesh and that the fragmentation and adding of 6LoWPAN “mesh header” is performed
     *   by Router_1 (DUT) - which is being verified in the next step.
     * - Pass Criteria:
     *   - Verify that topology is formed.
     *   - Leader MUST see both the DUT and Router_2 as its immediate neighbors.
     *   - Router_2 MUST only see Leader as its immediate neighbor and not the DUT.
     *   - ED_1 MUST see the DUT as its parent.
     */
    Log("Step 1: Form the topology");

    // Leader <-> Router_1 (DUT)
    AllowLinkBetween(leader, router1);

    // Leader <-> Router_2
    AllowLinkBetween(leader, router2);

    // Router_1 (DUT) <-> Router_3
    AllowLinkBetween(router1, router3);

    // Router_1 (DUT) <-> Router_4
    AllowLinkBetween(router1, router4);

    // Router_1 (DUT) <-> ED_1
    AllowLinkBetween(router1, ed1);

    // Enable TREL (mDNS) on all TREL nodes.
    SuccessOrQuit(router1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(leader.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    SuccessOrQuit(router4.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    router3.Join(leader);
    router4.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    ed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router4.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(ed1.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    // Verify neighbors
    {
        const Neighbor *neighbor = leader.Get<NeighborTable>().FindNeighbor(router1.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit(neighbor != nullptr);
        VerifyOrQuit(neighbor->GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel));
    }
    {
        const Neighbor *neighbor = router1.Get<NeighborTable>().FindNeighbor(router4.Get<Mac::Mac>().GetExtAddress());
        VerifyOrQuit(neighbor != nullptr);
        VerifyOrQuit(neighbor->GetSupportedRadioTypes().Contains(Mac::kRadioTypeTrel));
    }
    VerifyOrQuit(leader.Get<NeighborTable>().FindNeighbor(router2.Get<Mac::Mac>().GetExtAddress()) != nullptr);
    VerifyOrQuit(router2.Get<NeighborTable>().FindNeighbor(leader.Get<Mac::Mac>().GetExtAddress()) != nullptr);
    VerifyOrQuit(router2.Get<NeighborTable>().FindNeighbor(router1.Get<Mac::Mac>().GetExtAddress()) == nullptr);

    /**
     * Step 2
     * - Device: ED_1
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more to
     *   destination Router_2 using the ML-EID address of Router_2 as the destination address. Note: this verifies the
     *   DUT behavior of forwarding 6LoWPAN fragmented frames from a Child (incoming over 15.4) over multiple hops.
     * - Pass Criteria:
     *   - ED_1 MUST successfully receive a ping reply from Router_2, routed via DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
     *     frames.
     */
    Log("Step 2: ED_1 pings Router_2 ML-EID");
    nexus.SendAndVerifyEchoRequest(ed1, router2.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 3
     * - Device: Router_2
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
     *   Router_2 to ED_1 using the ML-EID address of ED_1 as the destination address.
     * - Pass Criteria:
     *   - Router_2 MUST successfully receive a ping reply from ED_1, routed via the DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
     *     frames.
     */
    Log("Step 3: Router_2 pings ED_1 ML-EID");
    nexus.SendAndVerifyEchoRequest(router2, ed1.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 4
     * - Device: Router_4
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
     *   Router_4 to destination Router_2 using the ML-EID address of Router_2 as the destination address. Warning: see
     *   TESTPLAN-626 for an open issue (TBD) in this step. Note: this intends to verify the DUT behavior of
     *   forwarding 6LoWPAN “Mesh Header” messages, incoming over TREL, over multi-hop routes.
     * - Pass Criteria:
     *   - Router_4 MUST successfully receive a ping reply from Router_2, routed via the DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
     *     frames.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Router_4, and MUST be correct
     *     TREL frames.
     */
    Log("Step 4: Router_4 pings Router_2 ML-EID");
    nexus.SendAndVerifyEchoRequest(router4, router2.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 5
     * - Device: Router_2
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
     *   Router_2 to Router_4 using the ML-EID address of Router_4 as the destination address.
     * - Pass Criteria:
     *   - Router_2 MUST successfully receive a ping reply from Router_4, routed via the DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between DUT and Leader, and MUST be correct TREL
     *     frames.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Router_4, and MUST be correct
     *     TREL frames.
     */
    Log("Step 5: Router_2 pings Router_4 ML-EID");
    nexus.SendAndVerifyEchoRequest(router2, router4.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 6
     * - Device: Router_3
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
     *   Router_3 to destination Router_2 using the ML-EID address of Router_2 as the destination address. Note: this
     *   verifies the DUT behavior of forwarding 6LoWPAN “Mesh Header” messages, incoming over 15.4, over multi-hop
     *   routes.
     * - Pass Criteria:
     *   - Router_3 MUST successfully receive a ping reply from Router_2, routed via the DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Leader, and MUST be correct
     *     TREL frames.
     */
    Log("Step 6: Router_3 pings Router_2 ML-EID");
    nexus.SendAndVerifyEchoRequest(router3, router2.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    /**
     * Step 7
     * - Device: Router_2
     * - Description (TREL-8.2): Harness instructs device to send a ping of payload size of 500 bytes or more from
     *   Router_2 to Router_3 using the ML-EID address of Router_3 as the destination address.
     * - Pass Criteria:
     *   - Router_2 MUST successfully receive a ping reply from Router_3, routed via the DUT.
     *   - TREL UDP frames MUST be used for ping/ping-reply transport between the DUT and Leader, and MUST be correct
     *     TREL frames.
     */
    Log("Step 7: Router_2 pings Router_3 ML-EID");
    nexus.SendAndVerifyEchoRequest(router2, router3.Get<Mle::Mle>().GetMeshLocalEid(), kPingPayloadSize,
                                   Ip6::kDefaultHopLimit, kPingResponseTimeout);

    nexus.SaveTestInfo("test_1_4_TREL_TC_2.json");
}

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Nexus
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_MULTI_RADIO
    ot::Nexus::Test_1_4_TREL_TC_2();
    printf("All tests passed\n");
#else
    printf("Multi-radio is not enabled - test skipped\n");
#endif
    return 0;
}
