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
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for a child to attach to its parent, in milliseconds.
 */
static constexpr uint32_t kAttachToParentTime = 2 * 1000;

/**
 * Address Query timeout interval, in milliseconds.
 */
static constexpr uint32_t kAqTimeout = OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_TIMEOUT * 1000;

/**
 * Initial Address Query retry delay, in milliseconds.
 */
static constexpr uint32_t kAqRetryDelay = OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_INITIAL_RETRY_DELAY * 1000;

/**
 * Echo request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Random IID for non-existent address.
 */
static constexpr uint64_t kNonExistentIid = 0x1234567812345678ULL;

void Test5_3_11(void)
{
    /**
     * 5.3.11 Address Query Timeout Intervals
     *
     * 5.3.11.1 Topology
     * - DUT (Router)
     * - MED_1
     *
     * 5.3.11.2 Purpose & Description
     * The purpose of this test case is to validate the way AQ_TIMEOUT and AQ_RETRY_TIMEOUT intervals are used in the
     *   Address Query transmission algorithm.
     *
     * Spec Reference                         | V1.1 Section | V1.3.0 Section
     * ---------------------------------------|--------------|---------------
     * Transmission of Address Query Messages | 5.4.2.1      | 5.4.2.1
     */

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &med1    = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("DUT");
    med1.SetName("MED_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Build the topology as described and begin the wireless sniffer.
     * - Pass Criteria: N/A
     */

    /** Use AllowList feature to restrict the topology. */
    nexus.AllowLinkBetween(leader, router1);
    nexus.AllowLinkBetween(router1, med1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    med1.Join(router1, Node::kAsMed);
    nexus.AdvanceTime(kAttachToParentTime);
    VerifyOrQuit(med1.Get<Mle::Mle>().IsChild());

    Ip6::Address x = router1.Get<Mle::Mle>().GetMeshLocalEid();
    BigEndian::WriteUint64(kNonExistentIid, x.GetIid().mFields.m8);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: MED_1");

    /**
     * Step 2: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X.
     * - Pass Criteria:
     *   - The DUT MUST issue an Address Query Request on MED_1’s behalf.
     *   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV – non-existent mesh-local address X
     *   - An Address Query Notification MUST NOT be received within AQ_TIMEOUT interval.
     */
    med1.SendEchoRequest(x, kEchoIdentifier);
    nexus.AdvanceTime(kAqTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: MED_1");

    /**
     * Step 3: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X
     *   before ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expires.
     * - Pass Criteria:
     *   - The DUT MUST NOT initiate a new Address Query frame.
     */
    nexus.AdvanceTime(kAqRetryDelay / 2);
    med1.SendEchoRequest(x, kEchoIdentifier);
    nexus.AdvanceTime(kAqTimeout);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: MED_1");

    /**
     * Step 4: MED_1
     * - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X
     *   after ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expires.
     * - Pass Criteria:
     *   - The DUT MUST issue an Address Query Request on MED_1’s behalf.
     *   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
     *   - CoAP URI-Path: NON POST coap://<FF03::2>
     *   - CoAP Payload:
     *     - Target EID TLV – non-existent mesh-local address X
     */
    nexus.AdvanceTime(kAqRetryDelay);
    med1.SendEchoRequest(x, kEchoIdentifier);
    nexus.AdvanceTime(kAqTimeout);

    nexus.SaveTestInfo("test_5_3_11.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_3_11();
    printf("All tests passed\n");
    return 0;
}
