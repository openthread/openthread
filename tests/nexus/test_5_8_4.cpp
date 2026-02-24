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
#include "thread/mle.hpp"

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
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 32 * 1000;

/**
 * Time to advance for the CoAP response to be received.
 */
static constexpr uint32_t kResponseTime = 10 * 1000;

/**
 * Time to advance for the discovery scan.
 */
static constexpr uint32_t kScanTime = 10 * 1000;

/**
 * Security Policy flag masks.
 */
static constexpr uint8_t kObtainNetworkKeyMask    = 0x80;
static constexpr uint8_t kNativeCommissioningMask = 0x40;
static constexpr uint8_t kRoutersMask             = 0x20;
static constexpr uint8_t kBeaconsMask             = 0x08;

void Test5_8_4(void)
{
    /**
     * 5.8.4 Security Policy TLV
     *
     * 5.8.4.1 Topology
     * - Commissioner_1 is an On-mesh Commissioner.
     * - Commissioner_2 is not part of the original topology - it is introduced at step 11.
     * - Partition is formed with all Security Policy TLV bits set to 1.
     *
     * 5.8.4.2 Purpose & Description
     * The purpose of this test case is to verify network behavior when Security Policy TLV "O","N","R","B" bits are
     *   disabled. "C" bit is not tested as it requires an External Commissioner which is currently not part of Thread
     *   Certification.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &commr1 = nexus.CreateNode();
    Node &commr2 = nexus.CreateNode();

    leader.SetName("LEADER");
    commr1.SetName("COMMISSIONER_1");
    commr2.SetName("COMMISSIONER_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    /**
     * Step 1: All
     * - Description: Build Topology. Ensure topology is formed correctly.
     * - Pass Criteria: N/A.
     */

    /** Use AllowList to specify links between nodes. */
    leader.AllowList(commr1);
    commr1.AllowList(leader);

    leader.AllowList(commr2);
    commr2.AllowList(leader);

    /** Partition is formed with all Security Policy TLV bits set to 1. */
    {
        SecurityPolicy policy;
        uint8_t        flags[] = {0xff, 0xff};
        policy.SetFlags(flags, sizeof(flags));
        leader.Get<KeyManager>().SetSecurityPolicy(policy);
    }

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    commr1.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(commr1.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(commr1.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));
    nexus.AdvanceTime(kStabilizationTime);
    VerifyOrQuit(commr1.Get<MeshCoP::Commissioner>().IsActive());

    /** Add a joiner on commr1 to enable joining on the Leader. */
    SuccessOrQuit(commr1.Get<MeshCoP::Commissioner>().AddJoinerAny("123456", 100));
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner_1");

    /**
     * Step 2: Commissioner_1
     * - Description: Harness instructs the device to send MGMT_ACTIVE_GET.req to the DUT.
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload: <empty>
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Components comps;
        comps.Clear();
        SuccessOrQuit(commr1.Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(comps, nullptr, 0, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Security Policy TLV Bits "O","N","R","C" should be set to 1.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4 & 5: Commissioner_1");

    /**
     * Step 4 & 5: Commissioner_1
     * - Description: Harness instructs the device to send MGMT_ACTIVE_SET.req to the DUT (disable "O" bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 15 (> step 3), Security Policy TLV with "O"
     *     bit disabled.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;
        SuccessOrQuit(commr1.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
        timestamp.Clear();
        timestamp.SetSeconds(15);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        uint8_t flags[] = {static_cast<uint8_t>(~kObtainNetworkKeyMask), 0xff}; // Disable "O" bit
        dataset.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(flags, sizeof(flags));

        uint16_t sessionId = commr1.Get<MeshCoP::Commissioner>().GetSessionId();
        uint8_t  tlvs[4];
        tlvs[0] = MeshCoP::Tlv::kCommissionerSessionId;
        tlvs[1] = sizeof(uint16_t);
        BigEndian::WriteUint16(sessionId, &tlvs[2]);

        SuccessOrQuit(
            commr1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(dataset, tlvs, sizeof(tlvs), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01)).
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Commissioner_1");

    /**
     * Step 7: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_GET.req to the DUT.
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload: Get TLV specifying: Network Master Key TLV.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Components comps;
        comps.Clear();
        uint8_t tlvTypes[] = {MeshCoP::Tlv::kNetworkKey};
        SuccessOrQuit(
            commr1.Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(comps, tlvTypes, sizeof(tlvTypes), nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: Network Master Key TLV MUST NOT be included.
     */

    nexus.AdvanceTime(kResponseTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Commissioner_1");

    /**
     * Step 9: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT (disable "N" bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 20 (> step 5), Security Policy TLV with "N"
     *     bit disabled.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;
        SuccessOrQuit(commr1.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
        timestamp.Clear();
        timestamp.SetSeconds(20);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        uint8_t flags[] = {static_cast<uint8_t>(~(kObtainNetworkKeyMask | kNativeCommissioningMask)),
                           0xff}; // Disable "N" and "O" bits
        dataset.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(flags, sizeof(flags));

        uint16_t sessionId = commr1.Get<MeshCoP::Commissioner>().GetSessionId();
        uint8_t  tlvs[4];
        tlvs[0] = MeshCoP::Tlv::kCommissionerSessionId;
        tlvs[1] = sizeof(uint16_t);
        BigEndian::WriteUint16(sessionId, &tlvs[2]);

        SuccessOrQuit(
            commr1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(dataset, tlvs, sizeof(tlvs), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01)).
     */

    nexus.AdvanceTime(kResponseTime);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Commissioner_2");

    /**
     * Step 11: Commissioner_2
     * - Description: Harness instructs device to try to join the network as a Native Commissioner.
     * - Pass Criteria: N/A.
     */

    commr2.Get<ThreadNetif>().Up();
    SuccessOrQuit(commr2.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), 0xffff, /* aJoiner */ true,
                                                              /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                              nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically rejects Commissioner_2's attempt to join.
     * - Pass Criteria: The DUT MUST send a Discovery Response with Native Commissioning bit set to "Not Allowed".
     */

    nexus.AdvanceTime(kScanTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Commissioner_1");

    /**
     * Step 13: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT ("B" bit = 0).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 25 (> Step 9), Security Policy TLV with "B"
     *     bit = 0 (default).
     *   - Note: This step is a legacy V1.1 behavior which has been deprecated in V1.2.1. For simplicity sake, this step
     *     has been left as-is because the B-bit is now reserved - and the value of zero is the new default behavior.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;
        SuccessOrQuit(commr1.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
        timestamp.Clear();
        timestamp.SetSeconds(25);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        uint8_t flags[] = {static_cast<uint8_t>(~(kObtainNetworkKeyMask | kNativeCommissioningMask | kBeaconsMask)),
                           0xff}; // Disable "B", "N", and "O" bits
        dataset.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(flags, sizeof(flags));

        uint16_t sessionId = commr1.Get<MeshCoP::Commissioner>().GetSessionId();
        uint8_t  tlvs[4];
        tlvs[0] = MeshCoP::Tlv::kCommissionerSessionId;
        tlvs[1] = sizeof(uint16_t);
        BigEndian::WriteUint16(sessionId, &tlvs[2]);

        SuccessOrQuit(
            commr1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(dataset, tlvs, sizeof(tlvs), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader (DUT)");

    /**
     * Step 14: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01)).
     */

    nexus.AdvanceTime(kResponseTime);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Test Harness Device");

    /**
     * Step 15: Test Harness Device
     * - Description: Harness instructs device to discover network using beacons.
     * - Pass Criteria: N/A.
     */

    SuccessOrQuit(commr2.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), 0xffff, /* aJoiner */ false,
                                                              /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                              nullptr, nullptr));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    /**
     * Step 16: Leader (DUT)
     * - Description: Automatically responds with beacon response frame.
     * - Pass Criteria: The DUT MUST send beacon response frames. The beacon payload MUST either be empty OR the payload
     *   format MUST be different from the Thread Beacon payload. The Protocol ID and Version field values MUST be
     *   different from the values specified for the Thread beacon (Protocol ID= 3, Version = 2).
     */

    nexus.AdvanceTime(kScanTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Commissioner_1");

    /**
     * Step 17: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT (disable "R" bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload: Commissioner Session ID TLV, Active Timestamp TLV = 30 (> step 13), Security Policy TLV with
     * "R" bit disabled.
     * - Pass Criteria: N/A.
     */

    {
        MeshCoP::Dataset::Info dataset;
        MeshCoP::Timestamp     timestamp;
        SuccessOrQuit(commr1.Get<MeshCoP::ActiveDatasetManager>().Read(dataset));
        timestamp.Clear();
        timestamp.SetSeconds(30);
        dataset.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);
        uint8_t flags[] = {
            static_cast<uint8_t>(~(kObtainNetworkKeyMask | kNativeCommissioningMask | kBeaconsMask | kRoutersMask)),
            0xff}; // Disable "R", "B", "N", and "O" bits
        dataset.Update<MeshCoP::Dataset::kSecurityPolicy>().SetFlags(flags, sizeof(flags));

        uint16_t sessionId = commr1.Get<MeshCoP::Commissioner>().GetSessionId();
        uint8_t  tlvs[4];
        tlvs[0] = MeshCoP::Tlv::kCommissionerSessionId;
        tlvs[1] = sizeof(uint16_t);
        BigEndian::WriteUint16(sessionId, &tlvs[2]);

        SuccessOrQuit(
            commr1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(dataset, tlvs, sizeof(tlvs), nullptr, nullptr));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Leader (DUT)");

    /**
     * Step 18: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload: State TLV (value = Accept (0x01)).
     */

    nexus.AdvanceTime(kResponseTime);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader (DUT)");

    /**
     * Step 19: Leader (DUT)
     * - Description: Automatically sends multicast MLE Data Response. Commissioner_1 responds with MLE Data Request.
     * - Pass Criteria: The DUT MUST multicast MLE Data Response to the Link-Local All Nodes multicast address (FF02::1)
     *   with active timestamp value as set in Step 17.
     */

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Leader (DUT)");

    /**
     * Step 20: Leader (DUT)
     * - Description: Automatically sends unicast MLE Data Response to Commissioner_1.
     * - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Commissioner_1. The Active Operational Set MUST
     *   contain a Security Policy TLV with R bit set to 0.
     */

    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo("test_5_8_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test5_8_4();
    printf("All tests passed\n");
    return 0;
}
