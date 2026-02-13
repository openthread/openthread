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
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

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
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 30 * 1000;

/**
 * Time to wait for ICMPv6 Echo response.
 */
static constexpr uint32_t kEchoResponseWaitTime = 5 * 1000;

/**
 * ICMPv6 Echo Request payload size, in bytes.
 */
static constexpr uint16_t kEchoPayloadSize = 0;

/**
 * ICMPv6 Echo Request Hop Limit.
 */
static constexpr uint8_t kEchoHopLimit = 64;

/**
 * Add an on-mesh prefix to a node.
 *
 * @param[in] aNode          The node to add the prefix to.
 * @param[in] aPrefixString  The prefix to add.
 * @param[in] aDhcp          Whether the prefix is for DHCPv6.
 */
static void AddPrefix(Node &aNode, const char *aPrefixString, bool aDhcp)
{
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mOnMesh    = true;
    config.mStable    = true;
    config.mPreferred = true;
    config.mDhcp      = aDhcp;
    config.mSlaac     = !aDhcp;

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

void Test5_3_8(void)
{
    /**
     * 5.3.8 MTD Child Address Set
     *
     * 5.3.8.1 Topology
     * - Leader (DUT)
     * - Border Router
     * - MED_1
     * - MED_2
     *
     * 5.3.8.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT MTD Child Address Set can hold at least 4 IPv6
     *   non-link-local addresses.
     *
     * Spec Reference        | V1.1 Section | V1.3.0 Section
     * ----------------------|--------------|---------------
     * MTD Child Address Set | 5.4.1.2      | 5.4.1.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &br     = nexus.CreateNode();
    Node &med1   = nexus.CreateNode();
    Node &med2   = nexus.CreateNode();

    leader.SetName("LEADER");
    br.SetName("BR");
    med1.SetName("MED_1");
    med2.SetName("MED_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /** Use AllowList feature to restrict the topology. */
    leader.AllowList(br);
    br.AllowList(leader);

    leader.AllowList(med1);
    med1.AllowList(leader);

    leader.AllowList(med2);
    med2.AllowList(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Border Router");

    /**
     * Step 1: Border Router
     * - Description: Harness configures the device to be a DHCPv6 server for prefixes 2001:: & 2002:: & 2003::.
     * - Pass Criteria: N/A
     */

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    br.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

    AddPrefix(br, "2001::/64", true);
    AddPrefix(br, "2002::/64", true);
    AddPrefix(br, "2003::/64", true);

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader (DUT)");

    /**
     * Step 2: Leader (DUT)
     * - Description: Automatically transmits MLE advertisements.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Advertisements.
     *   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
     *     (FF02::1).
     *   - The following TLVs MUST be present in the MLE Advertisements,:
     *     - Leader Data TLV
     *     - Route64 TLV
     *     - Source Address TLV
     */

    // This step is verified in the python script.
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: MED_1 and MED_2");

    /**
     * Step 3: MED_1 and MED_2
     * - Description: Harness attaches end devices.
     * - Pass Criteria: N/A
     */

    med1.Join(leader, Node::kAsMed);
    med2.Join(leader, Node::kAsMed);

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(med2.Get<Mle::Mle>().IsChild());

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: MED_1");

    /**
     * Step 4: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 ML-EID.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query Request.
     *   - MED_2 MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(med1, med2.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize, kEchoHopLimit,
                                   kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: MED_1");

    /**
     * Step 5: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2001:: GUA.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query Request.
     *   - MED_2 MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(med1, med2.FindMatchingAddress("2001::/64"), kEchoPayloadSize, kEchoHopLimit,
                                   kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: MED_1");

    /**
     * Step 6: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2002:: GUA.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query Request.
     *   - MED_2 MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(med1, med2.FindMatchingAddress("2002::/64"), kEchoPayloadSize, kEchoHopLimit,
                                   kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: MED_1");

    /**
     * Step 7: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the MED_2 2003:: GUA.
     * - Pass Criteria:
     *   - The DUT MUST NOT send an Address Query Request.
     *   - MED_2 MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(med1, med2.FindMatchingAddress("2003::/64"), kEchoPayloadSize, kEchoHopLimit,
                                   kEchoResponseWaitTime);

    nexus.SaveTestInfo("test_5_3_8.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_8();
    printf("All tests passed\n");
    return 0;
}
