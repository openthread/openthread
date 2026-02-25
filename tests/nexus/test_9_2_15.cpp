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

#include "meshcop/commissioner.hpp"
#include "meshcop/dataset.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace Nexus {

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinTime = 10 * 1000;

/**
 * Time to advance for a node to upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kRouterUpgradeTime = 200 * 1000;

/**
 * Time to advance for a commissioner to become active, in milliseconds.
 */
static constexpr uint32_t kPetitionTime = 5 * 1000;

/**
 * Time to wait for a response, in milliseconds.
 */
static constexpr uint32_t kResponseTime = 5000;

/**
 * Active Timestamp for Step 3.
 */
static constexpr uint64_t kActiveTimestampStep3 = 70;

/**
 * Pending Timestamp for Step 3.
 */
static constexpr uint64_t kPendingTimestampStep3 = 10;

/**
 * Mesh Local Prefix for Step 3.
 */
static const char kMeshLocalPrefixStep3[] = "fd00:0db9::";

/**
 * Delay Timer for Step 3.
 */
static constexpr uint32_t kDelayTimerStep3 = 600 * 1000;

/**
 * Active Timestamp for Step 11.
 */
static constexpr uint64_t kActiveTimestampStep11 = 80;

/**
 * Pending Timestamp for Step 11.
 */
static constexpr uint64_t kPendingTimestampStep11 = 20;

/**
 * Mesh Local Prefix for Step 11.
 */
static const char kMeshLocalPrefixStep11[] = "fd00:0db7::";

/**
 * Delay Timer for Step 11.
 */
static constexpr uint32_t kDelayTimerStep11 = 200 * 1000;

/**
 * Pan ID for Step 11.
 */
static constexpr uint16_t kPanIdStep11 = 0xabcd;

/**
 * Wait time for Step 9.
 */
static constexpr uint32_t kWaitTimeStep9 = 120 * 1000;

/**
 * Power down time for Step 10.
 */
static constexpr uint32_t kPowerDownTimeStep10 = 200 * 1000;

/**
 * Wait time for Step 18.
 */
static constexpr uint32_t kWaitTimeStep18 = 200 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5000;

void SendPendingSet(Node           &aCommissioner,
                    uint16_t        aSessionId,
                    uint64_t        aActiveTimestamp,
                    uint64_t        aPendingTimestamp,
                    const char     *aMeshLocalPrefix,
                    uint32_t        aDelayTimer,
                    const uint16_t *aPanId = nullptr)
{
    Tmf::Agent    &agent   = aCommissioner.Get<Tmf::Agent>();
    Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, aSessionId));
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(aActiveTimestamp);
        timestamp.SetTicks(0);
        SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
    }
    {
        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(aPendingTimestamp);
        timestamp.SetTicks(0);
        SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
    }
    {
        Ip6::Address address;
        SuccessOrQuit(address.FromString(aMeshLocalPrefix));
        SuccessOrQuit(Tlv::Append<MeshCoP::MeshLocalPrefixTlv>(*message, address.GetPrefix()));
    }
    SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, aDelayTimer));

    if (aPanId != nullptr)
    {
        SuccessOrQuit(Tlv::Append<MeshCoP::PanIdTlv>(*message, *aPanId));
    }

    Tmf::MessageInfo messageInfo(aCommissioner.GetInstance());
    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrQuit(agent.SendMessage(*message, messageInfo));
}

void Test9_2_15(void)
{
    /**
     * 9.2.15 Attaching with different Pending Operational Dataset
     *
     * 9.2.15.1 Topology
     * - Commissioner
     * - Leader
     * - Router_1
     * - Router_2 (DUT)
     *
     * 9.2.15.2 Purpose & Description
     * The purpose of this test case is to verify synchronization of a Pending Operational Dataset between an
     *   attaching Router and an existing Router.
     *
     * Spec Reference           | V1.1 Section | V1.3.0 Section
     * -------------------------|--------------|---------------
     * Dissemination of Datasets | 8.4.3        | 8.4.3
     */

    Core nexus;

    Node &commissioner = nexus.CreateNode();
    Node &leader       = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &dut          = nexus.CreateNode();

    commissioner.SetName("COMMISSIONER");
    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    dut.SetName("DUT");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Commissioner, Leader, Router_1");

    /**
     * Step 1: Commissioner, Leader, Router_1
     * - Description: Setup the topology without the DUT. Ensure topology is formed correctly. Verify Commissioner,
     *   Leader and Router_1 are sending MLE advertisements.
     * - Pass Criteria: N/A
     */

    commissioner.AllowList(leader);
    leader.AllowList(commissioner);

    leader.AllowList(router1);
    router1.AllowList(leader);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

    router1.Join(leader);
    nexus.AdvanceTime(kRouterUpgradeTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(commissioner.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kPetitionTime);
    VerifyOrQuit(commissioner.Get<MeshCoP::Commissioner>().IsActive());

    uint16_t sessionId = commissioner.Get<MeshCoP::Commissioner>().GetSessionId();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Router_2 (DUT)");

    /**
     * Step 2: Router_2 (DUT)
     * - Description: Configuration: Router_2 is configured out-of-band with Network Credentials of existing network.
     * - Pass Criteria: N/A
     */

    /**
     * Link between the following node pairs:
     * - Commissioner and Leader
     * - Leader and Router 1
     * - Router 1 and Router 2
     */
    dut.AllowList(router1);
    router1.AllowList(dut);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Commissioner");

    /**
     * Step 3: Commissioner
     * - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader Routing or
     *   Anycast Locator, setting a subset of the Active Operational Dataset:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - new, valid Active Timestamp TLV (70s)
     *     - new, valid Pending Timestamp TLV (10s)
     *     - new Network Mesh-Local Prefix TLV value (fd:00:0d:b9:00:00:00:00)
     *     - Delay Timer TLV: 600s
     * - Pass Criteria: N/A
     */

    SendPendingSet(commissioner, sessionId, kActiveTimestampStep3, kPendingTimestampStep3, kMeshLocalPrefixStep3,
                   kDelayTimerStep3);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_2 (DUT)");

    /**
     * Step 5: Router_2 (DUT)
     * - Description: Begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255, including the following TLVs:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (Verify sent to routers only)
     *     - Version TLV
     *   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
     */

    dut.Join(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Commissioner, Leader, Router_1");

    /**
     * Step 6: Commissioner, Leader, Router_1
     * - Description: All devices automatically respond by sending MLE Parent Response to Router_2 (DUT).
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_2 (DUT)");

    /**
     * Step 7: Router_2 (DUT)
     * - Description: Automatically sends Child ID Request to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Request to Router_1, including the following TLVs:
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The following TLVs MUST NOT be present in the Child ID Request:
     *     - Address Registration TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_1");

    /**
     * Step 8: Router_1
     * - Description: Automatically sends MLE Child ID Response to Router_2 (DUT), including the following TLVs:
     *   - Active Timestamp TLV
     *   - Address16 TLV
     *   - Leader Data TLV
     *   - Pending Operational Dataset TLV (corresponding to step 3)
     *   - Pending Timestamp TLV (corresponding to step 3)
     *   - Source Address TLV
     *   - Address Registration TLV (optional)
     *   - Network Data TLV (optional)
     *   - Route64 TLV (optional)
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Test Harness");

    /**
     * Step 9: Test Harness
     * - Description: Wait 120 seconds to allow the DUT to upgrade to a Router.
     * - Pass Criteria: N/A (implied)
     */

    nexus.AdvanceTime(kWaitTimeStep9);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_2 (DUT)");

    /**
     * Step 10: Router_2 (DUT)
     * - Description: Power down DUT for 200 seconds.
     * - Pass Criteria: N/A
     */

    dut.Get<Mle::Mle>().Stop();
    dut.GetInstance().Get<ThreadNetif>().Down();

    nexus.AdvanceTime(kPowerDownTimeStep10);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Commissioner");

    /**
     * Step 11: Commissioner
     * - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader Routing or Anycast
     *   Locator setting a subset of the Active Operational Dataset:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - new, valid Active Timestamp TLV (80s)
     *     - new, valid Pending Timestamp TLV (20s)
     *     - new value for Network Mesh-Local Prefix TLV (fd:00:0d:b7:00:00:00:00)
     *     - Delay Timer TLV: 200s
     *     - new Pan ID TLV (abcd)
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    SendPendingSet(commissioner, sessionId, kActiveTimestampStep11, kPendingTimestampStep11, kMeshLocalPrefixStep11,
                   kDelayTimerStep11, &kPanIdStep11);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader");

    /**
     * Step 12: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Router_2 (DUT)");

    /**
     * Step 13: Router_2 (DUT)
     * - Description: Power up after 200 seconds.
     * - Pass Criteria: N/A (implied)
     */

    dut.GetInstance().Get<ThreadNetif>().Up();
    SuccessOrQuit(dut.GetInstance().Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Router_2 (DUT)");

    /**
     * Step 14: Router_2 (DUT)
     * - Description: Begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT must send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255, including the following TLVs:
     *     - Challenge TLV
     *     - Mode TLV
     *     - Scan Mask TLV (Verify sent to routers only)
     *     - Version TLV
     *   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Commissioner, Leader, Router_1");

    /**
     * Step 15: Commissioner, Leader, Router_1
     * - Description: All devices automatically send MLE Parent Response to Router_2 (DUT).
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kJoinTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Router_2 (DUT)");

    /**
     * Step 16: Router_2 (DUT)
     * - Description: Automatically sends MLE Child ID Request to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Child ID Request to Router_1, including the following TLVs:
     *     - Active Timestamp TLV
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The following TLV MUST NOT be present in the MLE Child ID Request:
     *     - Address Registration TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1");

    /**
     * Step 17: Router_1
     * - Description: Automatically sends MLE Child ID Response to Router_2, including the following TLVs:
     *   - Active Timestamp TLV
     *   - Address16 TLV
     *   - Leader Data TLV
     *   - Pending Operational Dataset TLV (corresponding to step 11)
     *   - Pending Timestamp TLV (corresponding to step 11)
     *   - Source Address TLV
     *   - Address Registration TLV (optional)
     *   - Network Data TLV (optional)
     *   - Route64 TLV (optional)
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Test Harness");

    /**
     * Step 18: Test Harness
     * - Description: Wait 200 seconds to allow the DUT to upgrade to a Router.
     * - Pass Criteria: N/A (implied)
     */

    nexus.AdvanceTime(kWaitTimeStep18);
    VerifyOrQuit(dut.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader");

    /**
     * Step 19: Leader
     * - Description: Harness instructs Leader to send a MGMT_ACTIVE_GET.req to Router_2 (DUT) to get the Active
     *   Operational Dataset. (Request entire Active Operational Dataset by not including the Get TLV):
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload: <empty>
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = leader.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveGet);
        VerifyOrQuit(message != nullptr);

        Tmf::MessageInfo messageInfo(leader.GetInstance());
        messageInfo.SetPeerAddr(dut.Get<Mle::Mle>().GetMeshLocalEid());
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Router_2 (DUT)");

    /**
     * Step 20: Router_2 (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST send a MGMT_ACTIVE_GET.rsp to the Leader:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: <entire active operational data set>
     *   - The PAN ID TLV MUST have a value of abcd.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Commissioner");

    /**
     * Step 21: Commissioner
     * - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
     *   the DUT mesh local address.
     * - Pass Criteria: The DUT must respond with an ICMPv6 Echo Reply.
     */

    static constexpr uint16_t kEchoPayloadSize = 0;
    static constexpr uint8_t  kEchoHopLimit    = 64;

    nexus.SendAndVerifyEchoRequest(commissioner, dut.Get<Mle::Mle>().GetMeshLocalEid(), kEchoPayloadSize, kEchoHopLimit,
                                   kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_15.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_15();
    printf("All tests passed\n");
    return 0;
}
