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
 * Time to advance for the discovery scan to complete.
 */
static constexpr uint32_t kDiscoveryTime = 5 * 1000;

/**
 * Time to advance for the diagnostic response to be received.
 */
static constexpr uint32_t kDiagResponseTime = 5 * 1000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

enum Topology
{
    kTopologyA, // FED
    kTopologyB, // MED
    kTopologyC, // SED
};

void Test_1_3_GEN_TC_1(Topology aTopology, const char *aJsonFileName)
{
    /**
     * 9.1. [1.3] [CERT] Thread Version is '4' or higher
     *
     * 9.1.1. Purpose
     * Verify that the Thread Version TLV uses the value '4' or higher. '4' is required for Thread 1.3.x devices. '5'
     *   is required for Thread 1.4.x devices.
     *
     * 9.1.2. Topology
     * - BR_1: Thread Border Router DUT
     * - Router_1: Thread Router DUT
     * - ED_1: Thread End Device DUT, including FEDs, MEDs and SEDs
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Thread Version   | 5.18.2.1     | 5.18.2.1
     */

    Core nexus;

    Node &br1     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &ed1     = nexus.CreateNode();

    br1.SetName("BR_1");
    router1.SetName("Router_1");
    ed1.SetName("ED_1");

    /**
     * In the cpp, use AllowList to specify links between nodes. There is a link between the following node pairs:
     * - BR_1 and Router_1
     * - Router_1 and ED_1
     */
    br1.AllowList(router1);
    router1.AllowList(br1);
    router1.AllowList(ed1);
    ed1.AllowList(router1);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    switch (aTopology)
    {
    case kTopologyA:
        Log("Topology A: ED_1 as FED");
        break;
    case kTopologyB:
        Log("Topology B: ED_1 as MED");
        break;
    case kTopologyC:
        Log("Topology C: ED_1 as SED");
        break;
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: BR_1 Enable");

    /**
     * - Step 1
     *   - Device: BR_1
     *   - Description (GEN-9.1): Enable
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = BR_1: The DUT MUST become Leader of the Thread Network.
     */
    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_1 Enable");

    /**
     * - Step 2
     *   - Device: Router_1
     *   - Description (GEN-9.1): Enable
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    router1.Join(br1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_1 Automatically sends MLE Parent Request multicast.");

    /**
     * - Step 3
     *   - Device: Router_1
     *   - Description (GEN-9.1): Automatically sends MLE Parent Request multicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = Router: The DUT MUST sends MLE Parent Request multicast, with MLE Version TLV with value >= 4.
     * - Step 4
     *   - Device: BR_1
     *   - Description (GEN-9.1): Automatically sends MLE Parent Response unicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = BR_1: The DUT MUST sends MLE Parent Response unicast, with MLE Version TLV with value >= 4.
     * - Step 5
     *   - Device: Router_1
     *   - Description (GEN-9.1): Automatically completes attachment procedure and joins the network.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = Router: The DUT MUST send MLE Child ID Request message with MLE Version TLV with value >= 4.
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: ED_1 Enable, sends MLE Parent Request multicast.");

    /**
     * - Step 6
     *   - Device: ED_1
     *   - Description (GEN-9.1): Automatically sends MLE Parent Request multicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = ED_1: The DUT MUST sends MLE Parent Request multicast with MLE Version TLV with value >= 4.
     */
    switch (aTopology)
    {
    case kTopologyA:
        ed1.Join(router1, Node::kAsFed);
        break;
    case kTopologyB:
        ed1.Join(router1, Node::kAsMed);
        break;
    case kTopologyC:
        ed1.Join(router1, Node::kAsSed);
        SuccessOrQuit(ed1.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
        break;
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_1 Automatically sends MLE Parent Response unicast.");

    /**
     * - Step 7
     *   - Device: Router_1
     *   - Description (GEN-9.1): Automatically sends MLE Parent Response unicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = Router_1: The DUT MUST sends MLE Parent Response unicast with MLE Version TLV with value >= 4.
     * - Step 8
     *   - Device: ED_1
     *   - Description (GEN-9.1): Automatically completes attachment procedure and joins the network.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = ED_1: The DUT MUST send MLE Child ID Request message with MLE Version TLV with value >= 4.
     */
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Router_1 Harness instructs device to send MLE Discovery Request multicast.");

    /**
     * - Step 9
     *   - Device: Router_1
     *   - Description (GEN-9.1): Note: only performed if BR_1 is DUT. Harness instructs device to send MLE Discovery
     *     Request multicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    SuccessOrQuit(router1.Get<Mle::DiscoverScanner>().Discover(
        Mac::ChannelMask(1 << br1.Get<Mac::Mac>().GetPanChannel()), 0xffff, /* aJoiner */ false, /* aFilter */ false,
        /* aFilterIndexes */ nullptr, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: BR_1 Automatically responds with MLE Discovery Response unicast.");

    /**
     * - Step 10
     *   - Device: BR_1
     *   - Description (GEN-9.1): Note: only performed if BR_1 is DUT. Automatically responds with MLE Discovery
     *     Response unicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = BR_1: The DUT MUST send MLE Discovery Response with MLE Discovery Response TLV with a value >= 4
     *       of the 'Ver' field bits.
     */
    nexus.AdvanceTime(kDiscoveryTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: ED_1 Harness instructs device to send MLE Discovery Request multicast.");

    /**
     * - Step 11
     *   - Device: ED_1
     *   - Description (GEN-9.1): Note: only performed if Router_1 is DUT. Harness instructs device to send MLE
     *     Discovery Request multicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    SuccessOrQuit(ed1.Get<Mle::DiscoverScanner>().Discover(
        Mac::ChannelMask(1 << router1.Get<Mac::Mac>().GetPanChannel()), 0xffff, /* aJoiner */ false,
        /* aFilter */ false, /* aFilterIndexes */ nullptr, nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Router_1 Automatically responds with MLE Discovery Response unicast.");

    /**
     * - Step 12
     *   - Device: Router_1
     *   - Description (GEN-9.1): Note: only performed if Router_1 is DUT. Automatically responds with MLE Discovery
     *     Response unicast.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = Router_1: The DUT MUST send MLE Discovery Response with MLE Discovery Response TLV with a value
     *       >= 4 of the 'Ver' field bits.
     */
    nexus.AdvanceTime(kDiscoveryTime);

    // Diagnostics for v1.4 devices
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_4
    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router_1 Harness instructs device to send TMF Get Diagnostic Request unicast to the DUT.");

    /**
     * - Step 13
     *   - Device: Router_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if BR_1 is DUT and >= v1.4 Device. Harness instructs device to
     *     send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV
     *     (Type 24)
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    Ip6::Address br1Rloc;
    br1Rloc.SetToRoutingLocator(router1.Get<Mle::Mle>().GetMeshLocalPrefix(), br1.Get<Mle::Mle>().GetRloc16());
    uint8_t tlvTypes[] = {NetworkDiagnostic::Tlv::kVersion};
    SuccessOrQuit(router1.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(br1Rloc, tlvTypes, sizeof(tlvTypes),
                                                                             nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: BR_1 Automatically responds with Get Diagnostic Response unicast to Router_1.");

    /**
     * - Step 14
     *   - Device: BR_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if BR_1 is DUT and >= v1.4 Device. Automatically responds with
     *     Get Diagnostic Response (DIAG_GET.rsp) unicast to Router_1.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = BR_1 and >= v1.4 Device: The DUT MUST send DIAG_GET.RSP with Version TLV (Type 24)
     *       with value >= 5.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: BR_1 Harness instructs device to send TMF Get Diagnostic Request unicast to the DUT.");

    /**
     * - Step 15
     *   - Device: BR_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if Router_1 is DUT and >= v1.4 Device. Harness instructs device
     *     to send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV (Type
     *     24)
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    Ip6::Address router1Rloc;
    router1Rloc.SetToRoutingLocator(br1.Get<Mle::Mle>().GetMeshLocalPrefix(), router1.Get<Mle::Mle>().GetRloc16());
    SuccessOrQuit(br1.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(router1Rloc, tlvTypes, sizeof(tlvTypes),
                                                                         nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Router_1 Automatically responds with Get Diagnostic Response unicast to the DUT.");

    /**
     * - Step 16
     *   - Device: Router_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if Router_1 is DUT and >= v1.4 Device. Automatically responds
     *     with Get Diagnostic Response (DIAG_GET.rsp) unicast to the DUT.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = Router_1 and >= v1.4 device: The DUT MUST send DIAG_GET.RSP with Version TLV (Type 24) with value
     *       >= 5.
     */
    nexus.AdvanceTime(kDiagResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1 Harness instructs device to send TMF Get Diagnostic Request unicast to the DUT.");

    /**
     * - Step 17
     *   - Device: Router_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if ED_1 is DUT and >= v1.4 Device. Harness instructs device to
     *     send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV
     *     (Type 24)
     *   - Pass Criteria (only applies if Device == DUT):
     *     - N/A
     */
    Ip6::Address ed1Rloc;
    ed1Rloc.SetToRoutingLocator(router1.Get<Mle::Mle>().GetMeshLocalPrefix(), ed1.Get<Mle::Mle>().GetRloc16());
    SuccessOrQuit(router1.Get<NetworkDiagnostic::Client>().SendDiagnosticGet(ed1Rloc, tlvTypes, sizeof(tlvTypes),
                                                                             nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: ED_1 Optionally responds with Get Diagnostic Response unicast to Router_1.");

    /**
     * - Step 18
     *   - Device: ED_1 [1.4]
     *   - Description (GEN-9.1): Note: only performed if ED_1 is DUT and >= v1.4 Device. Optionally responds with Get
     *     Diagnostic Response (DIAG_GET.rsp) unicast to Router_1.
     *   - Pass Criteria (only applies if Device == DUT):
     *     - For DUT = ED_1 and >= v1.4 device: The DUT MUST respond in either of below two ways:
     *     - 1. DIAG_GET.rsp Response is successfully received, and MUST contain the Version TLV (Type 24)
     *       with value >= 5.
     *     - 2. CoAP error response is received with code 4.04 Not Found.
     *     - Note: in case 2, the DUT does not support the optional diagnostics request.
     */
    nexus.AdvanceTime(kDiagResponseTime);
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_4

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        // If no argument, run all topologies.
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyA, "test_1_3_GEN_TC_1_A.json");
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyB, "test_1_3_GEN_TC_1_B.json");
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyC, "test_1_3_GEN_TC_1_C.json");
    }
    else if (strcmp(argv[1], "A") == 0)
    {
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_1_3_GEN_TC_1_A.json");
    }
    else if (strcmp(argv[1], "B") == 0)
    {
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_1_3_GEN_TC_1_B.json");
    }
    else if (strcmp(argv[1], "C") == 0)
    {
        ot::Nexus::Test_1_3_GEN_TC_1(ot::Nexus::kTopologyC, (argc > 2) ? argv[2] : "test_1_3_GEN_TC_1_C.json");
    }
    else
    {
        fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A', 'B', or 'C'.\n", argv[1]);
        return 1;
    }

    printf("All tests passed\n");
    return 0;
}
