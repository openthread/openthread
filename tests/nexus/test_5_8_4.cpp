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

static constexpr uint32_t kFormNetworkTime   = 13 * 1000;
static constexpr uint32_t kJoinTime          = 20 * 1000;
static constexpr uint32_t kStabilizationTime = 10 * 1000;

void DummyHandler(otActiveScanResult *aResult, void *aContext)
{
    OT_UNUSED_VARIABLE(aResult);
    OT_UNUSED_VARIABLE(aContext);
}

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
     * The purpose of this test case is to verify network behavior when Security Policy TLV “O”, ”N”, ”R”, ”B” bits are
     *   disabled. “C” bit is not tested as it requires an External Commissioner which is currently not part of Thread
     *   Certification.
     *
     * Spec Reference           | V1.1 Section | V1.3.0 Section
     * -------------------------|--------------|---------------
     * Security Policy TLV (12) | 8.10.1.15    | 8.10.1.15
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &comm1  = nexus.CreateNode();
    Node &comm2  = nexus.CreateNode();

    leader.SetName("LEADER");
    comm1.SetName("COMMISSIONER_1");
    comm2.SetName("COMMISSIONER_2");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    /**
     * Step 1: All
     * - Description: Build Topology. Ensure topology is formed correctly.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: All");

    // Topology configuration using AllowList
    leader.AllowList(comm1);
    leader.AllowList(comm2);

    comm1.AllowList(leader);
    comm2.AllowList(leader);

    // Initialize Comm2 early to ensure MAC and socket states are ready
    comm2.Get<ThreadNetif>().Up();
    IgnoreError(comm2.Get<Mle::Mle>().Disable()); // Ensure clean state
    SuccessOrQuit(comm2.Get<Mle::Mle>().Enable());
    comm2.Get<Mac::Mac>().SetRxOnWhenIdle(true);

    // Leader forms the network
    {
        MeshCoP::Dataset::Info datasetInfo;
        SuccessOrQuit(datasetInfo.GenerateRandom(leader.GetInstance()));

        static const uint8_t kTestNetworkKey[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        NetworkKey           key;
        memcpy(key.m8, kTestNetworkKey, sizeof(key.m8));
        datasetInfo.Set<MeshCoP::Dataset::kNetworkKey>(key);

        SecurityPolicy &policy = datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>();
        policy.mRotationTime                   = 3600;
        policy.mObtainNetworkKeyEnabled        = true;
        policy.mNativeCommissioningEnabled     = true;
        policy.mRoutersEnabled                 = true;
        policy.mExternalCommissioningEnabled   = true;
        policy.mCommercialCommissioningEnabled = false;
        policy.mAutonomousEnrollmentEnabled    = false;
        policy.mNetworkKeyProvisioningEnabled  = false;
        policy.mTobleLinkEnabled               = true;
        policy.mNonCcmRoutersEnabled           = true;
        policy.mVersionThresholdForRouting     = 0;

        datasetInfo.Set<MeshCoP::Dataset::kChannel>(11);

        leader.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);

        leader.Get<ThreadNetif>().Up();
        SuccessOrQuit(leader.Get<Mle::Mle>().Start());

        // Ensure KeyManager has the key and sequence 0 immediately for pktverify
        // Move after Start to avoid overwrite
        leader.Get<KeyManager>().SetNetworkKey(key);
        leader.Get<KeyManager>().SetCurrentKeySequence(0, KeyManager::kForceUpdate);
    }

    // leader.Form(); // Replaced above
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    // Commissioner_1 joins
    comm1.Join(leader);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(comm1.Get<Mle::Mle>().IsAttached());

    // Make Commissioner_1 an active commissioner
    SuccessOrQuit(comm1.Get<MeshCoP::Commissioner>().Start(nullptr, nullptr, nullptr));

    // Wait for Commissioner to become Active
    {
        bool isActive = false;
        for (int i = 0; i < 30; i++)
        {
            nexus.AdvanceTime(1000);
            if (comm1.Get<MeshCoP::Commissioner>().IsActive())
            {
                isActive = true;
                break;
            }
        }
        VerifyOrQuit(isActive, "Commissioner_1 failed to become active");
    }

    VerifyOrQuit(comm1.Get<Mle::Mle>().IsAttached()); // Re-verify attachment

    /**
     * Step 2: Commissioner_1
     * - Description: Harness instructs the device to send MGMT_ACTIVE_GET.req to the DUT.
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload: <empty>
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Commissioner_1");

    {
        MeshCoP::Dataset::Components components;
        components.Clear();
        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(
            components, nullptr, 0, &leader.Get<Mle::Mle>().GetLinkLocalAddress()));
    }

    /**
     * Step 3: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Security Policy TLV Bits “O”, ”N”, ”R”, ”C” should be set to 1.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4 & 5: Commissioner_1
     * - Description: Harness instructs the device to send MGMT_ACTIVE_SET.req to the DUT
     *   (disable “O” bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Active Timestamp TLV = 15 (> step 3)
     *     - Security Policy TLV with “O” bit disabled.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 4 & 5: Commissioner_1");

    nexus.AdvanceTime(5000);

    // Dummy GET to prime address resolution
    {
        MeshCoP::Dataset::Components components;
        components.Clear();
        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(components, nullptr, 0, nullptr));
    }
    nexus.AdvanceTime(2000);

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();

        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(15);
        timestamp.SetTicks(0);
        timestamp.SetAuthoritative(true);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        SecurityPolicy &policy = datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>();
        policy.mRotationTime                   = 3600;
        policy.mObtainNetworkKeyEnabled        = false; // Disable "O" bit
        policy.mNativeCommissioningEnabled     = true;
        policy.mRoutersEnabled                 = true;
        policy.mExternalCommissioningEnabled   = true;
        policy.mCommercialCommissioningEnabled = false;
        policy.mAutonomousEnrollmentEnabled    = false;
        policy.mNetworkKeyProvisioningEnabled  = false;
        policy.mTobleLinkEnabled               = true;
        policy.mNonCcmRoutersEnabled           = true;
        policy.mVersionThresholdForRouting     = 0;

        MeshCoP::CommissionerSessionIdTlv sessionIdTlv;
        sessionIdTlv.Init();
        sessionIdTlv.SetCommissionerSessionId(comm1.Get<MeshCoP::Commissioner>().GetSessionId());

        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(
            datasetInfo, reinterpret_cast<const uint8_t *>(&sessionIdTlv), sessionIdTlv.GetSize(), nullptr, nullptr));
    }

    /**
     * Step 6: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - State TLV (value = Accept (0x01))
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_GET.req to the DUT.
     *   - CoAP Request URI: coap://[<L>]:MM/c/ag
     *   - CoAP Payload:
     *     - Get TLV specifying: Network Master Key TLV
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Commissioner_1");

    nexus.AdvanceTime(5000);

    {
        MeshCoP::Dataset::Components components;
        components.Clear();
        uint8_t tlvTypes[] = {MeshCoP::Tlv::kNetworkKey};
        SuccessOrQuit(
            comm1.Get<MeshCoP::ActiveDatasetManager>().SendGetRequest(components, tlvTypes, sizeof(tlvTypes), nullptr));
    }

    /**
     * Step 8: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - Network Master Key TLV MUST NOT be included.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 9: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
     *   (disable “N” bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Active Timestamp TLV = 20 (> step 5)
     *     - Security Policy TLV with “N” bit disabled.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Commissioner_1");

    nexus.AdvanceTime(5000);

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();

        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(20);
        timestamp.SetTicks(0);
        timestamp.SetAuthoritative(true);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        SecurityPolicy &policy = datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>();
        policy.mRotationTime                   = 3600;
        policy.mObtainNetworkKeyEnabled        = false; // Previously disabled
        policy.mNativeCommissioningEnabled     = false; // Disable "N" bit
        policy.mRoutersEnabled                 = true;
        policy.mExternalCommissioningEnabled   = true;
        policy.mCommercialCommissioningEnabled = false;
        policy.mAutonomousEnrollmentEnabled    = false;
        policy.mNetworkKeyProvisioningEnabled  = false;
        policy.mTobleLinkEnabled               = true;
        policy.mNonCcmRoutersEnabled           = true;
        policy.mVersionThresholdForRouting     = 0;

        MeshCoP::CommissionerSessionIdTlv sessionIdTlv;
        sessionIdTlv.Init();
        sessionIdTlv.SetCommissionerSessionId(comm1.Get<MeshCoP::Commissioner>().GetSessionId());

        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(
            datasetInfo, reinterpret_cast<const uint8_t *>(&sessionIdTlv), sessionIdTlv.GetSize(), nullptr, nullptr));
    }

    /**
     * Step 10: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - State TLV (value = Accept (0x01))
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 11: Commissioner_2
     * - Description: Harness instructs device to try to join the network as a Native
     *   Commissioner.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Commissioner_2 Discovery Request");

    SuccessOrQuit(comm2.Get<Mle::DiscoverScanner>().Discover(Mac::ChannelMask(0), 0xffff, /* aJoiner */ false,
                                                             /* aFilter */ false, /* aFilterIndexes */ nullptr,
                                                             DummyHandler, nullptr));
    nexus.AdvanceTime(5000);

    /**
     * Step 12: Leader (DUT)
     * - Description: Automatically rejects Commissioner_2’s attempt to join.
     * - Pass Criteria:
     *   - The DUT MUST send a Discovery Response with Native Commissioning bit set to
     *     “Not Allowed”.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 13: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
     *   (“B” bit = 0).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Active Timestamp TLV = 25 (> Step 9)
     *     - Security Policy TLV with “B” bit = 0 (default)
     *   - Note: This step is a legacy V1.1 behavior which has been deprecated in V1.2.1.
     *     For simplicity sake, this step has been left as-is because the B-bit is now
     *     reserved – and the value of zero is the new default behavior.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: Commissioner_1");

    nexus.AdvanceTime(5000);

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();

        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(25);
        timestamp.SetTicks(0);
        timestamp.SetAuthoritative(true);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        SecurityPolicy &policy = datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>();
        policy.mRotationTime                 = 3600;
        policy.mObtainNetworkKeyEnabled      = false;
        policy.mNativeCommissioningEnabled   = false;
        policy.mRoutersEnabled               = true;
        policy.mExternalCommissioningEnabled = true;
        // mBeaconsEnabled (B bit) removed/reserved
        policy.mCommercialCommissioningEnabled = false;
        policy.mAutonomousEnrollmentEnabled    = false;
        policy.mNetworkKeyProvisioningEnabled  = false;
        policy.mTobleLinkEnabled               = true;
        policy.mNonCcmRoutersEnabled           = true;
        policy.mVersionThresholdForRouting     = 0;

        MeshCoP::CommissionerSessionIdTlv sessionIdTlv;
        sessionIdTlv.Init();
        sessionIdTlv.SetCommissionerSessionId(comm1.Get<MeshCoP::Commissioner>().GetSessionId());

        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(
            datasetInfo, reinterpret_cast<const uint8_t *>(&sessionIdTlv), sessionIdTlv.GetSize(), nullptr, nullptr));
    }

    /**
     * Step 14: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - State TLV (value = Accept (0x01))
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 15: Test Harness Device
     * - Description: Harness instructs device to discover network using beacons.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: Test Harness Device");

    // Use comm1 to scan/discover
    SuccessOrQuit(comm1.Get<Mac::Mac>().ActiveScan(1 << 11, 0, DummyHandler, nullptr));

    /**
     * Step 16: Leader (DUT)
     * - Description: Automatically responds with beacon response frame.
     * - Pass Criteria:
     *   - The DUT MUST send beacon response frames.
     *   - The beacon payload MUST either be empty OR the payload format MUST be
     *     different from the Thread Beacon payload.
     *   - The Protocol ID and Version field values MUST be different from the values
     *     specified for the Thread beacon (Protocol ID= 3, Version = 2).
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 16: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 17: Commissioner_1
     * - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
     *   (disable “R” bit).
     *   - CoAP Request URI: coap://[<L>]:MM/c/as
     *   - CoAP Payload:
     *     - Commissioner Session ID TLV
     *     - Active Timestamp TLV = 30 (> step 13)
     *     - Security Policy TLV with “R” bit disabled.
     * - Pass Criteria: N/A
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 17: Commissioner_1");

    nexus.AdvanceTime(5000);

    {
        MeshCoP::Dataset::Info datasetInfo;
        datasetInfo.Clear();

        MeshCoP::Timestamp timestamp;
        timestamp.SetSeconds(30);
        timestamp.SetTicks(0);
        timestamp.SetAuthoritative(true);
        datasetInfo.Set<MeshCoP::Dataset::kActiveTimestamp>(timestamp);

        SecurityPolicy &policy = datasetInfo.Update<MeshCoP::Dataset::kSecurityPolicy>();
        policy.mRotationTime                 = 3600;
        policy.mObtainNetworkKeyEnabled      = false;
        policy.mNativeCommissioningEnabled   = false;
        policy.mRoutersEnabled               = false; // Disable "R" bit
        policy.mExternalCommissioningEnabled = true;
        // mBeaconsEnabled (B bit) removed/reserved
        policy.mCommercialCommissioningEnabled = false;
        policy.mAutonomousEnrollmentEnabled    = false;
        policy.mNetworkKeyProvisioningEnabled  = false;
        policy.mTobleLinkEnabled               = true;
        policy.mNonCcmRoutersEnabled           = true;
        policy.mVersionThresholdForRouting     = 0;

        MeshCoP::CommissionerSessionIdTlv sessionIdTlv;
        sessionIdTlv.Init();
        sessionIdTlv.SetCommissionerSessionId(comm1.Get<MeshCoP::Commissioner>().GetSessionId());

        SuccessOrQuit(comm1.Get<MeshCoP::ActiveDatasetManager>().SendSetRequest(
            datasetInfo, reinterpret_cast<const uint8_t *>(&sessionIdTlv), sessionIdTlv.GetSize(), nullptr, nullptr));
    }

    /**
     * Step 18: Leader (DUT)
     * - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
     *   - CoAP Response Code: 2.04 Changed
     *   - CoAP Payload:
     *     - State TLV (value = Accept (0x01))
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 18: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 19: Leader (DUT)
     * - Description: Automatically sends multicast MLE Data Response. Commissioner_1
     *   responds with MLE Data Request.
     * - Pass Criteria:
     *   - The DUT MUST multicast MLE Data Response to the Link-Local All Nodes
     *     multicast address (FF02::1) with active timestamp value as set in Step 17.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 19: Leader (DUT)");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 20: Leader (DUT)
     * - Description: Automatically sends unicast MLE Data Response to Commissioner_1.
     * - Pass Criteria:
     *   - The DUT MUST send a unicast MLE Data Response to Commissioner_1.
     *   - The Active Operational Set MUST contain a Security Policy TLV with R bit set
     *     to 0.
     */
    Log("---------------------------------------------------------------------------------------");
    Log("Step 20: Leader (DUT)");

    // Trigger MLE Data Request from Comm1 if it hasn't happened automatically (though step 19 says Comm1 responds)
    // Multicast MLE Data Response usually triggers MLE Data Request from children if they detect new data.
    // The AdvanceTime above should handle it.

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
