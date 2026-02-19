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

#include "mac/data_poll_sender.hpp"
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
 * RSSI value to enable a link quality of 3 (good).
 */
static constexpr int8_t kRssiLinkQuality3 = -70;

/**
 * RSSI value to enable a link quality of 1 (bad).
 */
static constexpr int8_t kRssiLinkQuality1 = -95;

/**
 * The SED poll period, in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 1000;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_1_6(Topology aTopology, const char *aJsonFile)
{
    /**
     * 6.1.6 Attaching to a REED with Better Link Quality
     *
     * 6.1.6.1 Topology
     *   - Topology A: DUT as End Device (ED_1)
     *   - Topology B: DUT as Sleepy End Device (SED_1)
     *   - Leader
     *   - Router_1: Link quality = 1
     *   - REED_1: Link quality = 3
     *
     * 6.1.6.2 Purpose & Description
     *   The purpose of this test is to verify that the DUT sends a second Parent Request to the all-routers and
     *     all-reeds multicast address if it gets a reply from the first Parent Request to the all-routers address
     *     with a bad link quality.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Parent Selection | 4.7.2        | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &reed1   = nexus.CreateNode();
    Node &dut     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    reed1.SetName("REED_1");

    switch (aTopology)
    {
    case kTopologyA:
        dut.SetName("ED_1");
        break;
    case kTopologyB:
        dut.SetName("SED_1");
        break;
    }

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     *   - Description: Setup the topology without the DUT. Ensure all routers and leader are sending
     *     MLE advertisements.
     *   - Pass Criteria: N/A
     */
    Log("Step 1: All");

    leader.AllowList(router1);
    leader.AllowList(reed1);

    router1.AllowList(leader);
    reed1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    reed1.Join(leader);
    reed1.Get<Mle::Mle>().SetRouterUpgradeThreshold(0);
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());

    /**
     * Step 2: Router_1
     *   - Description: Harness configures the device to broadcast a link quality of 1 (bad).
     *   - Pass Criteria: N/A
     */
    Log("Step 2: Router_1");

    dut.AllowList(router1);
    dut.AllowList(reed1);

    router1.AllowList(dut);
    reed1.AllowList(dut);

    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(router1.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality1));
    SuccessOrQuit(router1.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality1));

    SuccessOrQuit(dut.Get<Mac::Filter>().AddRssIn(reed1.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));
    SuccessOrQuit(reed1.Get<Mac::Filter>().AddRssIn(dut.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality3));

    /**
     * Step 3: ED_1 / SED_1 (DUT)
     *   - Description: Automatically begins attach process by sending a multicast MLE Parent Request
     *     to the All-Routers multicast address with the Scan Mask TLV set for all Routers.
     *   - Pass Criteria:
     *     - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address
     *       (FF02::2) with an IP Hop Limit of 255.
     *     - The following TLVs MUST be present in the Parent Request:
     *       - Challenge TLV
     *       - Mode TLV
     *       - Scan Mask TLV (Value = 0x80 (active Routers))
     *       - Version TLV
     */
    Log("Step 3: ED_1 / SED_1 (DUT)");

    switch (aTopology)
    {
    case kTopologyA:
        dut.Join(leader, Node::kAsMed);
        break;
    case kTopologyB:
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
        dut.Join(leader, Node::kAsSed);
        break;
    }

    /**
     * Step 4: Router_1
     *   - Description: Automatically responds with MLE Parent Response.
     *   - Pass Criteria: N/A
     */
    Log("Step 4: Router_1");

    /**
     * Step 5: ED_1 / SED_1 (DUT)
     *   - Description: Automatically sends another multicast MLE Parent Request to the All-Routers
     *     multicast with the Scan Mask TLV set for all Routers and REEDs.
     *   - Pass Criteria:
     *     - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address
     *       (FF02::2) with an IP Hop Limit of 255.
     *     - The following TLVs MUST be present in the Parent Request:
     *       - Challenge TLV
     *       - Mode TLV
     *       - Scan Mask TLV (Value = 0xC0 (Routers and REEDs))
     *       - Version TLV
     */
    Log("Step 5: ED_1 / SED_1 (DUT)");

    nexus.AdvanceTime(kParentSelectionTime);

    /**
     * Step 6: REED_1
     *   - Description: Automatically responds with MLE Parent Response (in addition to Router_1).
     *   - Pass Criteria: N/A
     */
    Log("Step 6: REED_1");

    /**
     * Step 7: ED_1 / SED_1 (DUT)
     *   - Description: Automatically sends MLE Child ID Request to REED_1 due to better link quality.
     *   - Pass Criteria:
     *     - The DUT MUST unicast MLE Child ID Request to REED_1.
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
    Log("Step 7: ED_1 / SED_1 (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(dut.Get<Mle::Mle>().GetParent().GetExtAddress() == reed1.Get<Mac::Mac>().GetExtAddress());

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        ot::Nexus::RunTest6_1_6(ot::Nexus::kTopologyA, "test_6_1_6_A.json");
        ot::Nexus::RunTest6_1_6(ot::Nexus::kTopologyB, "test_6_1_6_B.json");
    }
    else
    {
        ot::Nexus::Topology topology;
        const char         *defaultJsonFile;

        if (strcmp(argv[1], "A") == 0)
        {
            topology        = ot::Nexus::kTopologyA;
            defaultJsonFile = "test_6_1_6_A.json";
        }
        else if (strcmp(argv[1], "B") == 0)
        {
            topology        = ot::Nexus::kTopologyB;
            defaultJsonFile = "test_6_1_6_B.json";
        }
        else
        {
            fprintf(stderr, "Error: Invalid topology '%s'. Must be 'A' or 'B'.\n", argv[1]);
            return 1;
        }

        ot::Nexus::RunTest6_1_6(topology, (argc > 2) ? argv[2] : defaultJsonFile);
    }

    printf("All tests passed\n");
    return 0;
}
