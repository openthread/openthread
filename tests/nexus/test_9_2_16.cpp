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
 * Time to advance for a node to join and synchronize datasets, in milliseconds.
 */
static constexpr uint32_t kJoinSyncTime = 60 * 1000;

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
static constexpr uint32_t kResponseTime = 5 * 1000;

/**
 * Time to wait for dataset dissemination, in milliseconds.
 */
static constexpr uint32_t kDisseminateTime = 30 * 1000;

/**
 * Time to wait for a node to upgrade to a router, in milliseconds.
 */
static constexpr uint32_t kWaitRouterTime = 120 * 1000;

/**
 * Time to wait for a node to upgrade after second attach, in milliseconds.
 */
static constexpr uint32_t kWaitRouterTime2 = 300 * 1000;

/**
 * Time to power down a node, in milliseconds.
 */
static constexpr uint32_t kPowerDownTime = 200 * 1000;

/**
 * Time to wait for ICMPv6 Echo response, in milliseconds.
 */
static constexpr uint32_t kEchoTimeout = 5 * 1000;

/**
 * Active Timestamp for Step 3.
 */
static constexpr uint64_t kActiveTimestampStep3 = 10;

/**
 * Pending Timestamp for Step 3.
 */
static constexpr uint64_t kPendingTimestampStep3 = 10;

/**
 * Mesh Local Prefix for Step 3.
 */
static const char kMeshLocalPrefixStep3[] = "fd00:0db9::/64";

/**
 * Delay Timer for Step 3.
 */
static constexpr uint32_t kDelayTimerStep3 = 600 * 1000;

/**
 * Active Timestamp for Step 11.
 */
static constexpr uint64_t kActiveTimestampStep11 = 20;

/**
 * Pending Timestamp for Step 11.
 */
static constexpr uint64_t kPendingTimestampStep11 = 20;

/**
 * Mesh Local Prefix for Step 11.
 */
static const char kMeshLocalPrefixStep11[] = "fd00:0db7::/64";

/**
 * PAN ID for Step 11.
 */
static constexpr uint16_t kPanIdStep11 = 0xabcd;

/**
 * Delay Timer for Step 11.
 */
static constexpr uint32_t kDelayTimerStep11 = 230 * 1000;

/**
 * Active Timestamp for Step 13.
 */
static constexpr uint64_t kActiveTimestampStep13 = 15;

/**
 * Network Name for Step 13.
 */
static const char kNetworkNameStep13[] = "threadCert";

/**
 * PSKc for Step 13.
 */
static constexpr uint8_t kPSKcStep13[] = {0x74, 0x68, 0x72, 0x65, 0x61, 0x64, 0x6a, 0x70,
                                          0x61, 0x6b, 0x65, 0x74, 0x65, 0x73, 0x74, 0x03};

void Test9_2_16(void)
{
    /**
     * 9.2.16 Attaching with different Active and Pending Operational Dataset
     *
     * 9.2.16.1 Topology
     * - Commissioner
     * - Leader
     * - Router_1
     * - Router_2 (DUT)
     *
     * 9.2.16.2 Purpose & Description
     * The purpose of this test case is to verify synchronization of Active and Pending Operational Datasets between an
     *   attaching Router and an existing Router.
     *
     * Spec Reference            | V1.1 Section | V1.3.0 Section
     * --------------------------|--------------|---------------
     * Dissemination of Datasets | 8.4.3        | 8.4.3
     */

    Core nexus;

    Node &commissioner = nexus.CreateNode();
    Node &leader       = nexus.CreateNode();
    Node &router1      = nexus.CreateNode();
    Node &router2      = nexus.CreateNode();

    commissioner.SetName("COMMISSIONER");
    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("DUT");

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

    router1.AllowList(router2);
    router2.AllowList(router1);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    nexus.AdvanceTime(kRouterUpgradeTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouterOrLeader());

    commissioner.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(commissioner.Get<Mle::Mle>().IsAttached());

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

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Commissioner");

    /**
     * Step 3: Commissioner
     * - Description: Harness instructs the Commissioner to send MGMT_PENDING_SET.req to the Leader RLOC or Anycast
     *   Locator setting a subset of the Active Operational Dataset:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - new, valid Active Timestamp TLV (10s)
     *     - new, valid Pending Timestamp TLV (10s)
     *     - new values for Network Mesh-Local Prefix TLV (fd:00:0d:b9:00:00:00:00)
     *     - Delay Timer TLV (600s)
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep3);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kPendingTimestampStep3);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
        }
        {
            Ip6::Prefix        prefix;
            Ip6::NetworkPrefix networkPrefix;

            SuccessOrQuit(prefix.FromString(kMeshLocalPrefixStep3));
            SuccessOrQuit(networkPrefix.SetFrom(prefix));
            SuccessOrQuit(Tlv::Append<MeshCoP::MeshLocalPrefixTlv>(*message, networkPrefix));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, kDelayTimerStep3));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Leader");

    /**
     * Step 4: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to Commissioner:
     *   - Response code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDisseminateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Router_2 (DUT)");

    /**
     * Step 5: Router_2 (DUT)
     * - Description: Begins attach process by sending a multicast MLE Parent Request.
     * - Pass Criteria:
     *   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
     *     Hop Limit of 255, including the following TLVs:
     *     - Mode TLV
     *     - Challenge TLV
     *     - Scan Mask TLV (Verify sent to routers only)
     *     - Version TLV
     *   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
     */

    router2.Join(leader);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Router_1");

    /**
     * Step 6: Router_1
     * - Description: Automatically responds with MLE Parent Response.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Router_2 (DUT)");

    /**
     * Step 7: Router_2 (DUT)
     * - Description: Automatically sends MLE Child ID Request to Router_1.
     * - Pass Criteria:
     *   - The DUT MUST send an MLE Child ID Request, including the following TLVs:
     *     - Link-layer Frame Counter TLV
     *     - Mode TLV
     *     - Response TLV
     *     - Timeout TLV
     *     - TLV Request TLV
     *     - Version TLV
     *     - MLE Frame Counter TLV (optional)
     *   - The following TLV MUST NOT be present in the Child ID Request:
     *     - Address Registration TLV
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Router_1");

    /**
     * Step 8: Router_1
     * - Description: Automatically sends Child ID Response to the DUT, including the following TLVs:
     *   - Active Timestamp TLV
     *   - Address16 TLV
     *   - Leader Data TLV
     *   - Pending Timestamp TLV (corresponding to step 3)
     *   - Pending Operational Dataset TLV (corresponding to step 3)
     *   - Source Address TLV
     *   - Address Registration TLV (optional)
     *   - Network Data TLV (optional)
     *   - Route64 TLV (optional)
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Test Harness");

    /**
     * Step 9: Test Harness
     * - Description: Wait 120 seconds to allow the DUT to upgrade to a Router.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWaitRouterTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouterOrLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Router_2 (DUT)");

    /**
     * Step 10: Router_2 (DUT)
     * - Description: Power down for 200 seconds.
     * - Pass Criteria: N/A
     */

    router2.Get<ThreadNetif>().Down();
    router2.Get<Mle::Mle>().Stop();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Commissioner");

    /**
     * Step 11: Commissioner
     * - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader RLOC or ALOC
     *   setting a subset of the Active Operational Dataset:
     *   - CoAP Request URI: coap://[<L>]:MM/c/ps
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - new, valid Active Timestamp TLV (20s)
     *     - new, valid Pending Timestamp TLV (20s)
     *     - new values for Network Mesh-Local Prefix TLV (fd00:0d:b7:00:00:00:00)
     *     - new value for PAN ID TLV (abcd)
     *     - Delay Timer TLV (230s)
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriPendingSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep11);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kPendingTimestampStep11);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::PendingTimestampTlv>(*message, timestamp));
        }
        {
            Ip6::Prefix        prefix;
            Ip6::NetworkPrefix networkPrefix;

            SuccessOrQuit(prefix.FromString(kMeshLocalPrefixStep11));
            SuccessOrQuit(networkPrefix.SetFrom(prefix));
            SuccessOrQuit(Tlv::Append<MeshCoP::MeshLocalPrefixTlv>(*message, networkPrefix));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameStep13));
        SuccessOrQuit(Tlv::Append<MeshCoP::PanIdTlv>(*message, kPanIdStep11));
        SuccessOrQuit(Tlv::Append<MeshCoP::DelayTimerTlv>(*message, kDelayTimerStep11));

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader");

    /**
     * Step 12: Leader
     * - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDisseminateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Commissioner");

    /**
     * Step 13: Commissioner
     * - Description: Harness instructs the Commissioner to send a MGMT_ACTIVE_SET.req to the Leader RLOC or Anycast
     *   Locator setting a subset of the Active Operational Dataset:
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - valid Commissioner Session ID TLV
     *     - new, valid Active Timestamp TLV (15s)
     *     - new value for Network Name TLV (“threadCert”)
     *     - new value for PSKc TLV: (74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:03)
     *   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
     * - Pass Criteria: N/A
     */

    {
        Tmf::Agent    &agent   = commissioner.Get<Tmf::Agent>();
        Coap::Message *message = agent.NewPriorityConfirmablePostMessage(kUriActiveSet);
        VerifyOrQuit(message != nullptr);

        SuccessOrQuit(Tlv::Append<MeshCoP::CommissionerSessionIdTlv>(*message, sessionId));
        {
            MeshCoP::Timestamp timestamp;
            timestamp.SetSeconds(kActiveTimestampStep13);
            timestamp.SetTicks(0);
            SuccessOrQuit(Tlv::Append<MeshCoP::ActiveTimestampTlv>(*message, timestamp));
        }
        SuccessOrQuit(Tlv::Append<MeshCoP::NetworkNameTlv>(*message, kNetworkNameStep13));
        {
            Pskc pskc;
            memcpy(pskc.m8, kPSKcStep13, sizeof(pskc.m8));
            SuccessOrQuit(Tlv::Append<MeshCoP::PskcTlv>(*message, pskc));
        }

        Tmf::MessageInfo messageInfo(commissioner.GetInstance());
        messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader");

    /**
     * Step 14: Leader
     * - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to the Commissioner:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01))
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kDisseminateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Router_2 (DUT)");

    /**
     * Step 15: Router_2 (DUT)
     * - Description: Power up after 200 seconds.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kPowerDownTime - (2 * kDisseminateTime));

    router2.Get<ThreadNetif>().Up();
    SuccessOrQuit(router2.Get<Mle::Mle>().Start());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Router_2 (DUT)");

    /**
     * Step 16: Router_2 (DUT)
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

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Router_1");

    /**
     * Step 17: Router_1
     * - Description: Automatically responds with MLE Parent Response.
     * - Pass Criteria: N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Router_2 (DUT)");

    /**
     * Step 18: Router_2 (DUT)
     * - Description: Automatically sends Child ID Request to Router_1.
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
    Log("Step 19: Router_1");

    /**
     * Step 19: Router_1
     * - Description: Automatically sends Child ID Response to Router_2 (DUT), including the following TLVs:
     *   - Source Address TLV
     *   - Leader Data TLV
     *   - Address16 TLV
     *   - Network Data TLV (optional)
     *   - Route64 TLV (optional)
     *   - Address Registration TLV (optional)
     *   - Active Timestamp TLV (15s)
     *   - Active Operational Dataset TLV
     *     - includes Network Name sub-TLV (“threadCert”) corresponding to step 13
     *   - Pending Timestamp TLV (corresponding to step 10 / 11)
     *   - Pending Operational Dataset TLV (corresponding to step 11)
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kJoinSyncTime);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Test Harness");

    /**
     * Step 20: Test Harness
     * - Description: Wait 200 seconds to allow the DUT to upgrade to a Router.
     * - Pass Criteria: N/A
     */

    nexus.AdvanceTime(kWaitRouterTime2);
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouterOrLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 21: Leader");

    /**
     * Step 21: Leader
     * - Description: Harness instructs Leader to send MGMT_ACTIVE_GET.req to Router_2 (DUT) to get the Active
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
        messageInfo.SetSockAddrToRlocPeerAddrTo(router2.Get<Mle::Mle>().GetRloc16());
        SuccessOrQuit(agent.SendMessage(*message, messageInfo));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 22: Router_2 (DUT)");

    /**
     * Step 22: Router_2 (DUT)
     * - Description: Automatically responds to the Leader with a MGMT_ACTIVE_GET.rsp.
     * - Pass Criteria:
     *   - The DUT MUST send a MGMT_ACTIVE_GET.rsp to the Leader:
     *     - CoAP Response Code: 2.04 Changed
     *     - CoAP Payload: (complete active operational dataset)
     *   - The PAN ID TLV MUST have a value of abcd.
     *   - The Network Mesh-Local Prefix TLV MUST have a value of fd:00:0d:b7.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 23: Commissioner");

    /**
     * Step 23: Commissioner
     * - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
     *   the DUT mesh local address.
     * - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
     */

    nexus.SendAndVerifyEchoRequest(commissioner, router2.Get<Mle::Mle>().GetMeshLocalEid(), 0, 64, kEchoTimeout);

    nexus.SaveTestInfo("test_9_2_16.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test9_2_16();
    printf("All tests passed\n");
    return 0;
}
