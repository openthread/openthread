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
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for REED to become a router.
 */
static constexpr uint32_t kReedToRouterTime = 5 * 1000;

/**
 * Time to advance for the SED to send its first Data Request.
 */
static constexpr uint32_t kDataRequestTime = 5 * 1000;

/**
 * Time to advance for the ED to send its first Child Update Request.
 */
static constexpr uint32_t kChildUpdateRequestTime = 5 * 1000;

/**
 * The MLE timeout for the ED, in seconds.
 */
static constexpr uint32_t kMleTimeout = 4;

/**
 * The SED poll period, in milliseconds.
 */
static constexpr uint32_t kSedPollPeriod = 1000;

/**
 * The echo request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

enum Topology
{
    kTopologyA,
    kTopologyB,
};

void RunTest6_1_2(Topology aTopology, const char *aJsonFile)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &reed   = nexus.CreateNode();
    Node &dut    = nexus.CreateNode();

    leader.SetName("LEADER");
    reed.SetName("REED_1");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Begin wireless sniffer and ensure the Leader is sending MLE Advertisements and is connected to
     *   REED_1.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    leader.AllowList(reed);
    reed.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    reed.Join(leader);
    nexus.AdvanceTime(kReedToRouterTime);
    VerifyOrQuit(reed.Get<Mle::Mle>().IsChild());

    /**
     * Step 2: ED_1 / SED_1 (DUT)
     * - Description: Automatically begins attach process by sending multicast MLE Parent Requests.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (Value = 0x80 [active Routers])
     *     - Version TLV
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ED_1 / SED_1 (DUT)");

    reed.AllowList(dut);
    dut.AllowList(reed);

    if (aTopology == kTopologyA)
    {
        dut.Get<Mle::Mle>().SetTimeout(kMleTimeout);
        dut.Join(leader, Node::kAsMed);
    }
    else
    {
        SuccessOrQuit(dut.Get<DataPollSender>().SetExternalPollPeriod(kSedPollPeriod));
        dut.Join(leader, Node::kAsSed);
    }

    /**
     * Step 3: REED_1
     * - Description: Does not respond to Parent Request.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: REED_1");

    /**
     * Step 4: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Parent Request with Scan Mask TLV set to Routers and REEDs.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (Value = 0xC0 [Routers and REEDs])
     *     - Version TLV
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED_1 / SED_1 (DUT)");

    /**
     * Step 5: REED_1
     * - Description: Automatically responds with MLE Parent Response.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: REED_1");

    /**
     * Step 6: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Child ID Request in response.
     * - Pass Criteria:
     *   - The DUT MUST send an MLE Child ID Request containing the following TLVs:
     *     - Address Registration TLV
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - Version TLV
     *     - TLV Request TLV (Address16 TLV, Network Data TLV, Route64 TLV [optional])
     *     - MLE Frame Counter TLV (optional)
     *   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
     *     to ‘0x02’.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: ED_1 / SED_1 (DUT)");

    /**
     * Step 7: REED_1
     * - Description: Automatically sends an Address Solicit Request to the Leader. Leader automatically responds with
     *   an Address Solicit Response and REED_1 becomes an active router. REED_1 automatically sends a MLE Child ID
     *   Response with DUT’s new 16-bit Address.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: REED_1");

    nexus.AdvanceTime(kStabilizationTime);
    nexus.AdvanceTime(kReedToRouterTime);

    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(dut.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(reed.Get<Mle::Mle>().IsRouter());

    if (aTopology == kTopologyA)
    {
        /**
         * Step 8: ED_1 (DUT)
         * - Description: If the DUT is a Rx-On-When-Idle Device (End Device - ED): Automatically sends periodic MLE
         *   Child Update Request messages as part of the keep-alive message.
         * - Pass Criteria:
         *   - The DUT MUST send a MLE Child Update Request message containing the following TLVs:
         *     - Leader Data TLV
         *     - Mode TLV
         *     - Source Address TLV
         */
        Log("---------------------------------------------------------------------------------------");
        Log("Step 8: ED_1 (DUT)");

        nexus.AdvanceTime(kChildUpdateRequestTime);

        /**
         * Step 9: REED_1
         * - Description: If the DUT is a Rx-On-When-Idle Device (End Device - ED): Automatically responds with MLE
         *   Child Update Responses.
         * - Pass Criteria: N/A
         */
        Log("---------------------------------------------------------------------------------------");
        Log("Step 9: REED_1");

        nexus.AdvanceTime(kStabilizationTime);
    }
    else
    {
        /**
         * Step 10: SED_1 (DUT)
         * - Description: If the DUT is a Rx-Off-When-Idle Device (Sleepy End Device - SED): Automatically sends
         *   periodic 802.15.4 Data Request messages as part of the keep-alive message.
         * - Pass Criteria:
         *   - The DUT MUST send a 802.15.4 Data Request command to the parent device.
         */
        Log("---------------------------------------------------------------------------------------");
        Log("Step 10: SED_1 (DUT)");

        nexus.AdvanceTime(kDataRequestTime * 2);
    }

    /**
     * Step 11: REED_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
     *   link-local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: REED_1");

    reed.SendEchoRequest(dut.Get<Mle::Mle>().GetLinkLocalAddress(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo(aJsonFile);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "A") == 0)
    {
        printf("Running Topology A...\n");
        ot::Nexus::RunTest6_1_2(ot::Nexus::kTopologyA, (argc > 2) ? argv[2] : "test_6_1_2_A.json");
    }
    else if (argc > 1 && strcmp(argv[1], "B") == 0)
    {
        printf("Running Topology B...\n");
        ot::Nexus::RunTest6_1_2(ot::Nexus::kTopologyB, (argc > 2) ? argv[2] : "test_6_1_2_B.json");
    }
    else
    {
        printf("Running Topology A...\n");
        ot::Nexus::RunTest6_1_2(ot::Nexus::kTopologyA, "test_6_1_2_A.json");
        printf("Running Topology B...\n");
        ot::Nexus::RunTest6_1_2(ot::Nexus::kTopologyB, "test_6_1_2_B.json");
    }

    printf("All tests passed\n");
    return 0;
}
