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
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to advance for leader failover and BBR promotion, in milliseconds.
 */
static constexpr uint32_t kLeaderFailoverTime = 400 * 1000;

/**
 * Partition weight for Router_1 to ensure it becomes leader.
 */
static constexpr uint8_t kRouter1PartitionWeight = 74;

/**
 * BBR Sequence Number used by BR_2 to trigger role switch in DUT.
 */
static constexpr uint8_t kHighBbrSequenceNumber = 255;

/**
 * Multicast address MA1 for MLR.req test.
 */
static constexpr char kMa1Address[] = "ff04::1234:777a:1";

void Test_1_2_BBR_TC_2(void)
{
    /**
     * 5.11.2 BBR-TC-02: BBR MUST remove its BBR Dataset from Network Data
     *
     * 5.11.2.1 Topology
     * - BR_1 (DUT)
     * - BR_2
     * - Router_1
     *
     * 5.11.2.2 Purpose & Description
     * The purpose of this test case is to verify that if two BBR Datasets are present in a network, the DUT BR not
     *   being elected as Primary will delete its own BBR Dataset from the Network Data according to the rules of
     *   [ThreadSpec] Section 5.10.3.6, which are referred from Section 5.21.4.3. Note: in the present test procedure,
     *   only a subset of Section 5.10.3.6/5.21.4.3 rules is checked; other checks should be added in the future.
     *
     * Spec Reference   | V1.2 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * BBR Role Switch  | 5.10.3.6     | 5.21.4.3
     */

    Core nexus;

    Node &br1     = nexus.CreateNode();
    Node &br2     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    router1.SetName("Router_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 0
     * - Device: N/A
     * - Description: Topology formation - Router_1 (Leader). Topology addition - BR_1 (DUT). The DUT must be booted
     *   and joined to the test network
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 0: Topology formation");

    /** P1: Router_1 is configured with a Partition Weight 74 to force it to become Leader and stay Leader. */
    router1.Get<Mle::Mle>().SetLeaderWeight(kRouter1PartitionWeight);

    router1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);

    /**
     * Step 1
     * - Device: BR_1 (DUT)
     * - Description: Automatically receives Network Data and determines to send its BBR Dataset to the Leader.
     * - Pass Criteria:
     *   - The DUT MUST unicast SVR_DATA.ntf to the Leader as follows: coap://[<Router_1 RLOC>]:MM/a/sd
     *   - Where the payload contains inside the Thread Network Data TLV: Service TLV: BBR Dataset with
     *     - T=1
     *     - S_service_data Length = 1
     *     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
     *     - One Server TLV as sub-TLV formatted per 5.21.3.2 [ThreadSpec].
     */
    Log("Step 1: BR_1 (DUT) sends its BBR Dataset to the Leader");

    br1.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2
     * - Device: Router_1
     * - Description: Automatically responds to the Server Data notification as follows: 2.04 changed. Also it MUST be
     *   verified here (or alternatively at the end of the test) that the received BBR Dataset contains: BBR Sequence
     *   Number < 127
     * - Pass Criteria:
     *   - The Received BBR Dataset MUST contain: BBR Sequence Number < 127
     */
    Log("Step 2: Router_1 responds to the Server Data notification");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 3
     * - Device: BR_2
     * - Description: Topology addition - BR_2. Harness instructs the device to attach to the network. It automatically
     *   receives the Network Data, detects that there is already a BBR Dataset, and does not send its own BBR Dataset
     *   to Leader.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 3: Topology addition - BR_2");

    br2.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);
    br2.Get<BackboneRouter::Local>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4
     * - Device: BR_2
     * - Description: Harness instructs the device to set the BBR Sequence Number to 255. [Harness instructs the device
     *   to send a Coap message formatted like SVR_DATA.ntf to Leader with BBR Dataset with highest possible BBR
     *   Sequence Number (255). SVR_DATA.ntf contains a Thread Network Data TLV, with inside: Service TLV: BBR Dataset
     *   with
     *   - T=1
     *   - S_service_data Length = 1
     *   - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
     *   - One Server TLV as sub-TLV formatted per 5.21.3.2 [ThreadSpec] and values as specified below. In above Server
     *     TLV,
     *     - BBR Sequence Number is 255
     *     - Reregistration Delay is 5
     *     - MLR Timeout is 3600]
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: BR_2 sets the BBR Sequence Number to 255");

    {
        BackboneRouter::Config config;
        br2.Get<BackboneRouter::Local>().GetConfig(config);
        config.mSequenceNumber = kHighBbrSequenceNumber;
        SuccessOrQuit(br2.Get<BackboneRouter::Local>().SetConfig(config));
        SuccessOrQuit(br2.Get<BackboneRouter::Local>().AddService(BackboneRouter::Local::kForceRegistration));
    }
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5
     * - Device: Router_1
     * - Description: Automatically distributes new Network Data including the "fake" BBR Dataset of BR_2 that was
     *   submitted in the previous step. This means the Network Data now contains 2 BBR Datasets.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 5: Router_1 distributes new Network Data");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 6
     * - Device: BR_1 (DUT)
     * - Description: Receives new Network Data, automatically detects that there are two BBR Datasets; and that its
     *   own BBR Sequence Number is lower than the other's i.e. < 255. Automatically switches from Primary to Secondary
     *   BBR role and sends SVR_DATA.ntf to the Leader, this time excluding its BBR Dataset, in order to delete its BBR
     *   Dataset at the Leader.
     * - Pass Criteria:
     *   - The DUT MUST unicast SVR_DATA.ntf to Leader as follows: coap://[<Router_1 RLOC>]:MM/a/sd
     *   - The Thread Network Data TLV MUST NOT contain the following: Service TLV: BBR Dataset
     *     - T=1
     *     - S_service_data Length = 1
     *     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
     */
    Log("Step 6: BR_1 (DUT) switches to Secondary BBR role and deletes its BBR Dataset");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 6b
     * - Device: BR_2
     * - Description: Receives new Network Data, detects there there are two BBR Datasets; and that its own BBR Dataset
     *   is included with BBR Sequence Number higher than the other's (i.e. it is 255). It then automatically switches
     *   from Secondary to Primary BBR role. Informational: the BBR Sequence Number of BR_2 will remain 255 after the
     *   role switch.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6b: BR_2 switches to Primary BBR role");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7
     * - Device: Router_1
     * - Description: Receives the SVR_DATA.ntf and automatically responds 2.04 Changed. Automatically distributes the
     *   new Network Data with only one BBR Dataset (the one of BR_2).
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 7: Router_1 distributes new Network Data with only BR_2's BBR Dataset");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 8
     * - Device: BR_1 (DUT)
     * - Description: Receives the new Network Data and automatically determines not to reregister any new BBR Dataset.
     *   It remains in Secondary BBR role.
     * - Pass Criteria:
     *   - The DUT MAY send SVR_DATA.ntf
     *   - In SVR_DATA.ntf, the Thread Network Data TLV MUST NOT contain the following (as in step 6): Service TLV: BBR
     *     Dataset
     *     - T=1
     *     - S_service_data Length = 1
     *     - S_service_data = 0x01 (THREAD_SERVICE_DATA_BBR)
     */
    Log("Step 8: BR_1 (DUT) remains in Secondary BBR role");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 9
     * - Device: BR_2
     * - Description: Receives the new Network Data and remains in Primary BBR role.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 9: BR_2 remains in Primary BBR role");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 10
     * - Device: Router_1
     * - Description: Harness instructs the device to send a MLR.req message to register for multicast group MA1 to the
     *   Secondary BBR, BR_1 (the DUT).
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: Router_1 sends MLR.req to BR_1 (DUT)");

    {
        Ip6::Address    ma1;
        Coap::Message  *message;
        Ip6AddressesTlv addressesTlv;
        Ip6::Address    destAddr;

        SuccessOrQuit(ma1.FromString(kMa1Address));

        message = router1.Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMlr);
        VerifyOrQuit(message != nullptr);

        addressesTlv.Init();
        addressesTlv.SetLength(sizeof(Ip6::Address));
        SuccessOrQuit(message->Append(addressesTlv));
        SuccessOrQuit(message->Append(ma1));

        destAddr = br1.Get<Mle::Mle>().GetMeshLocalEid();
        SuccessOrQuit(router1.Get<Tmf::Agent>().SendMessageTo(*message, destAddr, nullptr, nullptr));
    }
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 11
     * - Device: BR_1 (DUT)
     * - Description: Automatically responds with an error to the MLR.req, since it is not Primary BBR anymore.
     * - Pass Criteria:
     *   - unicast an MLR.rsp CoAP response to Router_1 as follows: 2.04 changed
     *   - Where the payload contains: Status TLV: 5 [ST_MLR_BBR_NOT_PRIMARY]
     */
    Log("Step 11: BR_1 (DUT) responds with an error to the MLR.req");

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 12
     * - Device: BR_2
     * - Description: Harness instructs the device to stop being a Router, so that in the coming steps it cannot
     *   assume the Leader role. Note: in OT CLI this can be done by: routereligible disable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 12: BR_2 stops being a Router");

    SuccessOrQuit(br2.Get<Mle::Mle>().SetRouterEligible(false));
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 13
     * - Device: Router_1
     * - Description: Harness instructs the device to stop being a Leader. Note: in OT CLI this can be done by:
     *   routereligible disable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 13: Router_1 stops being a Leader");

    SuccessOrQuit(router1.Get<Mle::Mle>().SetRouterEligible(false));
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 14
     * - Device: BR_1 (DUT)
     * - Description: After some time, automatically becomes the Leader and Primary BBR.
     * - Pass Criteria:
     *   - MUST become the Leader of the network.
     *   - MUST be Primary BBR. This can be verified by checking that the network data in Router_1 contains the BBR
     *     Dataset of DUT. The OT CLI command "bbr" on Router_1 can also be used for verification.
     */
    Log("Step 14: BR_1 (DUT) becomes Leader and Primary BBR");

    nexus.AdvanceTime(kLeaderFailoverTime); // Time for leader failover and BBR promotion (milliseconds)

    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(br1.Get<BackboneRouter::Local>().IsPrimary());

    nexus.SaveTestInfo("test_1_2_BBR_TC_2.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_2_BBR_TC_2();
    printf("All tests passed\n");
    return 0;
}
