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

#include <stdarg.h>
#include <stdio.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a node to join as a child.
 */
static constexpr uint32_t kAttachAsChildTime = 5 * 1000;

/**
 * Time to wait for ICMPv6 Echo response.
 */
static constexpr uint32_t kEchoResponseTime = 5000;

/**
 * Number of routers in the topology.
 */
static constexpr uint16_t kRouterCount = 14;

void Test5_2_5(void)
{
    /**
     * 5.2.5 Address Query - REED
     *
     * 5.2.5.1 Topology
     * - Build a topology that has a total of 16 active routers, including the Leader, with no communication
     *   constraints.
     * - The Leader is configured as a DHCPv6 server for prefix 2001::
     * - The Border Router is configured as a SLAAC server for prefix 2002::
     * - Each router numbered 1 through 14 have a link to leader
     * - MED_1 has a link to the leader
     * - Border Router has a link to the leader
     * - REED_1 has a link to router 1
     *
     * 5.2.5.2 Purpose & Description
     * The purpose of this test case is to validate that the DUT is able to generate Address Notification messages in
     * response to Address Query messages.
     *
     * Spec Reference | V1.1 Section | V1.3.0 Section
     * ---------------|--------------|---------------
     * Address Query  | 5.4.2        | 5.4.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node *routers[kRouterCount];
    for (uint16_t i = 0; i < kRouterCount; i++)
    {
        routers[i] = &nexus.CreateNode();
    }
    Node &br    = nexus.CreateNode();
    Node &med1  = nexus.CreateNode();
    Node &reed1 = nexus.CreateNode();

    leader.SetName("LEADER");
    for (uint16_t i = 0; i < kRouterCount; i++)
    {
        char name[16];
        snprintf(name, sizeof(name), "ROUTER_%u", i + 1);
        routers[i]->SetName(name);
    }
    br.SetName("BR");
    med1.SetName("MED_1");
    reed1.SetName("REED_1");

    nexus.AdvanceTime(0);
    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Use AllowList feature to restrict the topology.
     */
    for (uint16_t i = 0; i < kRouterCount; i++)
    {
        nexus.AllowLinkBetween(leader, *routers[i]);
    }

    nexus.AllowLinkBetween(leader, br);
    nexus.AllowLinkBetween(leader, med1);
    nexus.AllowLinkBetween(*routers[0], reed1);

    Log("Step 1: Configure the Leader to be a DHCPv6 Border Router for prefix 2001::");

    /**
     * Step 1: Leader
     * - Description: Configure the Leader to be a DHCPv6 Border Router for prefix 2001::
     * - Pass Criteria: N/A
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2001::/64"));
        config.mOnMesh       = true;
        config.mDefaultRoute = true;
        config.mStable       = true;
        config.mDhcp         = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("Step 2: Attach the Border_Router to the network and configure the below On-Mesh Prefix");

    /**
     * Step 2: Border_Router
     * - Description: Attach the Border_Router to the network and configure the below On-Mesh Prefix:
     *   - Prefix 1: P_Prefix=2002::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
     * - Pass Criteria: N/A
     */
    br.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("2002::/64"));
        config.mOnMesh       = true;
        config.mDefaultRoute = true;
        config.mStable       = true;
        config.mSlaac        = true;
        config.mPreferred    = true;
        SuccessOrQuit(br.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        br.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    Log("Step 3: Ensure topology is formed correctly without the DUT.");

    /**
     * Step 3: All
     * - Description: Ensure topology is formed correctly without the DUT.
     * - Pass Criteria: N/A
     */
    for (uint16_t i = 0; i < kRouterCount; i++)
    {
        routers[i]->Join(leader);
    }
    nexus.AdvanceTime(kAttachToRouterTime);

    med1.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);

    for (uint16_t i = 0; i < kRouterCount; i++)
    {
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouter());
    }
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Log("Step 4: Cause the DUT to attach to Router_1 (2-hops from the leader).");

    /**
     * Step 4: REED_1 (DUT)
     * - Description: Cause the DUT to attach to Router_1 (2-hops from the leader).
     * - Pass Criteria:
     *   - The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
     *   - If the DUT sends Address Solicit Request, the test fails.
     */
    reed1.Join(*routers[0]);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());

    /**
     * Wait some time to ensure it doesn't try to become a router.
     */
    nexus.AdvanceTime(20 * 1000);
    VerifyOrQuit(reed1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!reed1.Get<Mle::Mle>().IsRouter());

    Log("Step 5: Harness enables a link between the DUT and BR to create a one-way link.");

    /**
     * Step 5: REED_1 (DUT), Border Router
     * - Description: Harness enables a link between the DUT and BR to create a one-way link.
     * - Pass Criteria: N/A
     */
    reed1.AllowList(br);

    Log("Step 6: MED_1 sends ICMPv6 Echo Request to the DUT (REED_1) using ML-EID.");

    /**
     * Step 6: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to the DUT (REED_1) using ML-EID.
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted Address Notification message:
     *     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
     *     - CoAP Payload:
     *       - Target EID TLV
     *       - RLOC16 TLV
     *       - ML-EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator (DUT).
     *   - The IPv6 Destination address MUST be the RLOC of the destination.
     *   - The DUT MUST send an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(med1, reed1.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoResponseTime);

    Log("Step 7: MED_1 sends ICMPv6 Echo Request to REED_1 (DUT) using 2001:: EID.");

    /**
     * Step 7: MED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to REED_1 (DUT) using 2001:: EID.
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted Address Notification message:
     *     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
     *     - CoAP Payload:
     *       - Target EID TLV
     *       - RLOC16 TLV
     *       - ML-EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator.
     *   - The IPv6 Destination address MUST be the RLOC of the destination.
     *   - The DUT MUST send an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(med1, reed1.FindMatchingAddress("2001::/64"), 0, 64, kEchoResponseTime);

    Log("Step 8: MED_1 sends ICMPv6 Echo Request to REED_1 (DUT) using 2002:: EID.");

    /**
     * Step 8: MED_1
     * - Description: Harness instructs the device to send an ICMPv6 Echo Request to REED_1 (DUT) using 2002:: EID.
     * - Pass Criteria:
     *   - The DUT MUST send a properly formatted Address Notification message:
     *     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
     *     - CoAP Payload:
     *       - Target EID TLV
     *       - RLOC16 TLV
     *       - ML-EID TLV
     *   - The IPv6 Source address MUST be the RLOC of the originator.
     *   - The IPv6 Destination address MUST be the RLOC of the destination.
     *   - The DUT MUST send an ICMPv6 Echo Reply.
     */
    nexus.SendAndVerifyEchoRequest(med1, reed1.FindMatchingAddress("2002::/64"), 0, 64, kEchoResponseTime);

    nexus.SaveTestInfo("test_5_2_5.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_2_5();
    printf("All tests passed\n");
    return 0;
}
