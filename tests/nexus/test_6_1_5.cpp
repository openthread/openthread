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
 * RSSI value to enable a link quality of 2 (medium).
 * Link margin > 10 dB gives link quality 2.
 * Noise floor is -100 dBm.
 */
static constexpr int8_t kRssiLinkQuality2 = -85;

/**
 * ICMPv6 Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Data poll period for SED, in milliseconds.
 */
static constexpr uint32_t kPollPeriod = 500;

/**
 * Time to advance at the start of the test.
 */
static constexpr uint32_t kStartTime = 0;

void Test6_1_5(void)
{
    /**
     * 6.1.5 Attaching to a Router with Better Link Quality
     *
     * 6.1.5.1 Topology
     * - Topology A: DUT as End Device (ED_1)
     * - Topology B: DUT as Sleepy End Device (SED_1)
     * - Leader
     * - Router_1
     * - Router_2
     *
     * 6.1.5.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT will choose a router with better link quality as a
     *   parent.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * Parent Selection | 4.7.2        | 4.5.2
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &reed2   = nexus.CreateNode();
    Node &sed1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    reed2.SetName("ROUTER_2");
    sed1.SetName("SED_1");

    nexus.AdvanceTime(kStartTime);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Setup the topology without the DUT. Ensure all routers and leader are sending MLE advertisements.");

    /**
     * Step 1: All
     * - Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE
     *   advertisements.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(router1);
    leader.AllowList(reed2);

    router1.AllowList(leader);
    reed2.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    reed2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    // REED_2 can be either Router or End Device depending on the network state, but it should be attached.
    VerifyOrQuit(reed2.Get<Mle::Mle>().IsAttached());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Harness configures the device to broadcast a link quality of 2 (medium).");

    /**
     * Step 2: Router_2
     * - Description: Harness configures the device to broadcast a link quality of 2 (medium).
     * - Pass Criteria: N/A
     */

    /** Restricted topology for DUT. */
    sed1.AllowList(router1);
    sed1.AllowList(reed2);

    router1.AllowList(sed1);
    reed2.AllowList(sed1);

    IgnoreError(sed1.Get<Mac::Filter>().AddRssIn(reed2.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));
    IgnoreError(reed2.Get<Mac::Filter>().AddRssIn(sed1.Get<Mac::Mac>().GetExtAddress(), kRssiLinkQuality2));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Automatically begins attach process by sending a multicast MLE Parent Request.");

    /**
     * Step 3: ED_1 / SED_1 (DUT)
     * - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255.
     *   - The following TLVs MUST be present in the Parent Request:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV = 0x80 (active Routers)
     *     - Version TLV
     */

    sed1.Join(leader, Node::kAsSed);
    SuccessOrQuit(sed1.Get<DataPollSender>().SetExternalPollPeriod(kPollPeriod));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Both devices automatically send MLE Parent Response.");

    /**
     * Step 4: Router_1, Router_2
     * - Description: Both devices automatically send MLE Parent Response.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kParentSelectionTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Automatically sends MLE Child ID Request to Router_1 due to better link quality.");

    /**
     * Step 5: ED_1 / SED_1 (DUT)
     * - Description: Automatically sends MLE Child ID Request to Router_1 due to better link quality.
     * - Pass Criteria:
     *   - The DUT MUST unicast MLE Child ID Request to Router_1.
     *   - The following TLVs MUST be present in the Child ID Request:
     *     - Address Registration TLV
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     */

    nexus.AdvanceTime(kStabilizationTime);

    VerifyOrQuit(sed1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(sed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed1.Get<Mle::Mle>().GetParent().GetExtAddress() == router1.Get<Mac::Mac>().GetExtAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT.");

    /**
     * Step 6: Router_1
     * - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
     *   DUT link local address.
     * - Pass Criteria:
     *   - The DUT MUST respond with ICMPv6 Echo Reply.
     */

    router1.SendEchoRequest(sed1.Get<Mle::Mle>().GetLinkLocalAddress(), kEchoIdentifier);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_6_1_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test6_1_5();
    printf("All tests passed\n");
    return 0;
}
