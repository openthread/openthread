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

#include "common/string.hpp"
#include "mac/data_poll_sender.hpp"

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

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
 * Number of MED children.
 */
static constexpr uint16_t kNumMeds = 4;

/**
 * Number of SED children.
 */
static constexpr uint16_t kNumSeds = 6;

/**
 * IPv6 header size in octets.
 */
static constexpr uint16_t kIp6HeaderSize = 40;

/**
 * ICMPv6 header size in octets.
 */
static constexpr uint16_t kIcmp6HeaderSize = 8;

/**
 * Small ICMPv6 Echo Request datagram size in octets.
 */
static constexpr uint16_t kSmallDatagramSize = 106;

/**
 * Large ICMPv6 Echo Request datagram size in octets.
 */
static constexpr uint16_t kLargeDatagramSize = 1280;

/**
 * Time to wait for ICMPv6 Echo replies.
 */
static constexpr uint32_t kEchoResponseWaitTime = 5 * 1000;

/**
 * ICMPv6 Echo Request identifier for MED children.
 */
static constexpr uint16_t kMedEchoId = 1001;

/**
 * ICMPv6 Echo Request identifier for SED children.
 */
static constexpr uint16_t kSedEchoId = 2001;

static void SendEchoRequest(Node &aSender, const Ip6::Address &aPeerAddr, uint16_t aDatagramSize, uint16_t aIdentifier)
{
    Error            error   = kErrorNone;
    Message         *message = aSender.Get<Ip6::Icmp>().NewMessage();
    Ip6::MessageInfo messageInfo;
    uint16_t         payloadLength;

    VerifyOrQuit(message != nullptr);

    VerifyOrExit(aDatagramSize >= kIp6HeaderSize + kIcmp6HeaderSize, error = kErrorInvalidArgs);
    payloadLength = aDatagramSize - kIp6HeaderSize - kIcmp6HeaderSize;

    error = message->SetLength(payloadLength);
    SuccessOrExit(error);

    messageInfo.SetPeerAddr(aPeerAddr);
    messageInfo.SetHopLimit(64);

    error = aSender.Get<Ip6::Icmp>().SendEchoRequest(*message, messageInfo, aIdentifier);
    SuccessOrExit(error);

    message = nullptr;

exit:
    if (message != nullptr)
    {
        message->Free();
    }

    SuccessOrQuit(error);
}

static void CreateNodes(Core &aNexus, Node *aNodes[], uint16_t aCount, const char *aNamePrefix)
{
    for (uint16_t i = 0; i < aCount; i++)
    {
        aNodes[i] = &aNexus.CreateNode();

        String<16> name;

        name.Append("%s_%u", aNamePrefix, i + 1);
        aNodes[i]->SetName(name.AsCString());
    }
}

static void AllowListNodes(Node &aRouter, Node *aNodes[], uint16_t aCount)
{
    for (uint16_t i = 0; i < aCount; i++)
    {
        aRouter.AllowList(*aNodes[i]);
        aNodes[i]->AllowList(aRouter);
    }
}

static void VerifyChildren(Node *aNodes[], uint16_t aCount)
{
    for (uint16_t i = 0; i < aCount; i++)
    {
        VerifyOrQuit(aNodes[i]->Get<Mle::Mle>().IsChild());
    }
}

void Test5_1_7(void)
{
    /**
     * 5.1.7 Minimum Supported Children – IPv6 Datagram Buffering
     *
     * 5.1.7.1 Topology
     * - Leader
     * - Router_1 (DUT)
     * - MED_1 through MED_4
     * - SED_1 through SED_6
     *
     * 5.1.7.2 Purpose & Description
     * The purpose of this test case is to validate the minimum conformance requirements for router-capable devices:
     * - a) Minimum number of supported children.
     * - b) Minimum MTU requirement when sending/forwarding an IPv6 datagram to a SED.
     * - c) Minimum number of sent/forwarded IPv6 datagrams to SED children.
     *
     * Spec Reference       | V1.1 Section | V1.3.0 Section
     * ---------------------|--------------|---------------
     * Conformance Document | 2.2          | 2.2
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node *meds[kNumMeds];
    Node *seds[kNumSeds];

    leader.SetName("LEADER");
    router.SetName("ROUTER_1");

    CreateNodes(nexus, meds, kNumMeds, "MED");
    CreateNodes(nexus, seds, kNumSeds, "SED");

    nexus.AdvanceTime(0);

    // Use AllowList feature to restrict the topology.
    leader.AllowList(router);
    router.AllowList(leader);

    AllowListNodes(router, meds, kNumMeds);
    AllowListNodes(router, seds, kNumSeds);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Leader, Router_1 (DUT), Children");

    /**
     * Step 1: Leader, Router_1 (DUT), Children
     * - Description: Create topology and attach MED_1, MED_2…MED_4, SED_1, SED_2…SED_6 children to the Router.
     * - Pass Criteria:
     *   - The DUT MUST send properly formatted MLE Parent Response and MLE Child ID Response to each child.
     */
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    for (uint16_t i = 0; i < kNumMeds; i++)
    {
        meds[i]->Join(router, Node::kAsMed);
    }
    nexus.AdvanceTime(kAttachAsChildTime);

    for (uint16_t i = 0; i < kNumSeds; i++)
    {
        seds[i]->Join(router, Node::kAsSed);
        SuccessOrQuit(seds[i]->Get<DataPollSender>().SetExternalPollPeriod(1000));
    }
    nexus.AdvanceTime(kAttachAsChildTime);

    VerifyChildren(meds, kNumMeds);
    VerifyChildren(seds, kNumSeds);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Leader");

    /**
     * Step 2: Leader
     * - Description: Harness instructs the Leader to send an ICMPv6 Echo Request with IPv6 datagram size of 106 octets
     *   to each MED.
     * - Pass Criteria:
     *   - The DUT MUST properly forward ICMPv6 Echo Requests to all MED children.
     *   - The DUT MUST properly forward ICMPv6 Echo Replies to the Leader.
     */
    for (uint16_t i = 0; i < kNumMeds; i++)
    {
        SendEchoRequest(leader, meds[i]->Get<Mle::Mle>().GetMeshLocalEid(), kSmallDatagramSize, kMedEchoId + i);
    }
    nexus.AdvanceTime(kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader");

    /**
     * Step 3: Leader
     * - Description: Harness instructs the Leader to send an ICMPv6 Echo Request with IPv6 datagram size of 1280 octets
     *   to SED_1 and ICMPv6 Echo Requests with IPv6 datagram size of 106 octets to SED_2, SED_3, SED_4, SED_5 and SED_6
     *   without waiting for ICMPv6 Echo Replies.
     * - Pass Criteria:
     *   - The DUT MUST buffer all IPv6 datagrams.
     *   - The DUT MUST properly forward ICMPv6 Echo Requests to all SED children.
     *   - The DUT MUST properly forward ICMPv6 Echo Replies to the Leader.
     */
    SendEchoRequest(leader, seds[0]->Get<Mle::Mle>().GetMeshLocalEid(), kLargeDatagramSize, kSedEchoId);
    for (uint16_t i = 1; i < kNumSeds; i++)
    {
        SendEchoRequest(leader, seds[i]->Get<Mle::Mle>().GetMeshLocalEid(), kSmallDatagramSize, kSedEchoId + i);
    }
    nexus.AdvanceTime(kEchoResponseWaitTime);

    nexus.SaveTestInfo("test_5_1_7.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_1_7();
    printf("All tests passed\n");
    return 0;
}
