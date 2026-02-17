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
 * Time to advance for the DUT to send Parent Request and receive Parent Responses.
 */
static constexpr uint32_t kParentSelectionTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_1_3(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.1.3 Attaching to a Router with better connectivity
     *
     * 6.1.3.1 Topology
     *   - Ensure link quality between all nodes is set to 3.
     *   - Leader
     *   - Router_1
     *   - Router_2
     *   - Router_3
     *   - DUT as ED_1 (Topology A) or SED_1 (Topology B)
     *
     * 6.1.3.2 Purpose & Description
     *   The purpose of this test case is to verify that the DUT chooses to attach to a Router with better
     *     connectivity.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * Attaching to a Parent | 4.7.1        | 4.5.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");
    router3.SetName("ROUTER_3");

    if (aTopology == kTopologyA)
    {
        dut.SetName("ED_1");
    }
    else
    {
        dut.SetName("SED_1");
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    if (aTopology == kTopologyA)
    {
        Log("Topology A: ED_1 (DUT)");
    }
    else
    {
        Log("Topology B: SED_1 (DUT)");
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader, Router_1, _2, _3");

    /**
     * Step 1: Leader, Router_1, _2, _3
     *   - Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE
     *     advertisements.
     *   - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    leader.AllowList(router2);
    leader.AllowList(router3);

    router1.AllowList(leader);
    router1.AllowList(router3);

    router2.AllowList(leader);
    router2.AllowList(dut);

    router3.AllowList(leader);
    router3.AllowList(router1);
    router3.AllowList(dut);

    dut.AllowList(router2);
    dut.AllowList(router3);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    router3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ED_1 / SED_1 (DUT)");

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     *   - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     *   - Pass Criteria:
     *     - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an
     *       IP Hop Limit of 255.
     *     - The following TLVs MUST be present in the Parent Request:
     *       - Challenge TLV
     *       - Mode TLV
     *       - Scan Mask TLV = 0x80 (active Routers)
     *       - Version TLV
     */

    if (aTopology == kTopologyA)
    {
        dut.Join(leader, Node::kAsMed);
    }
    else
    {
        dut.Join(leader, Node::kAsSed);
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Router_2, Router_3");

    /**
     * Step 3: Router_2, Router_3
     *   - Description: Automatically responds with MLE Parent Response.
     *   - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kParentSelectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED_1 / SED_1 (DUT)");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     *   - Description: Automatically sends MLE Child ID Request to Router_3 due to better connectivity.
     *   - Pass Criteria:
     *     - The DUT MUST unicast MLE Child ID Request to Router_3.
     *     - The following TLVs MUST be present in the Child ID Request:
     *       - Address Registration TLV
     *       - Link-layer Frame Counter TLV
     *       - Mode TLV
     *       - Response TLV
     *       - Timeout TLV
     *       - TLV Request TLV
     *       - Version TLV
     *       - MLE Frame Counter TLV (optional)
     */

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == router3.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_3");

    /**
     * Step 5: Router_3
     *   - Description: Harness verifies connectivity by instructing device to send an ICMPv6 Echo Request to the
     *     DUT link local address.
     *   - Pass Criteria:
     *     - The DUT MUST respond with ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(router3, dut.Get<Mle::Mle>().GetLinkLocalAddress());

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_1_3(ot::Nexus::kTopologyA, "test_6_1_3_A.json");
        ot::Nexus::RunTest6_1_3(ot::Nexus::kTopologyB, "test_6_1_3_B.json");
    }
    else
    {
        ot::Nexus::Topology topology;
        const char         *defaultJsonFile;

        if (strcmp(argv[1], "A") == 0)
        {
            topology        = ot::Nexus::kTopologyA;
            defaultJsonFile = "test_6_1_3_A.json";
        }
        else if (strcmp(argv[1], "B") == 0)
        {
            topology        = ot::Nexus::kTopologyB;
            defaultJsonFile = "test_6_1_3_B.json";
        }
        else
        {
            fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
            return 1;
        }

        ot::Nexus::RunTest6_1_3(topology, (argc > 2) ? argv[2] : defaultJsonFile);
    }

    printf("All tests passed\n");
    return 0;
}
