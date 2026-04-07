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
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 5 * 1000;

/**
 * Time to advance for DNS query processing.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP Lease time in seconds.
 */
static constexpr uint32_t kSrpLease5m = 300;
static constexpr uint32_t kSrpLease3m = 180;

/**
 * SRP Key Lease time in seconds.
 */
static constexpr uint32_t kSrpKeyLease10m = 600;

/**
 * SRP service port.
 */
static constexpr uint16_t kSrpServicePort1 = 55555;
static constexpr uint16_t kSrpServicePort2 = 55556;

/**
 * SRP service and host names.
 */
static const char kSrpServiceType[]     = "_thread-test._udp";
static const char kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static const char kSrpInstanceName1[]   = "service-test-1";
static const char kSrpInstanceName2[]   = "service-test-2";
static const char kSrpHostName1[]       = "host-test-1";

void Test_1_3_SRPC_TC_4(const char *aJsonFileName)
{
    Srp::Client::Service service1;
    Srp::Client::Service service2;

    /**
     * 3.4. [1.3] [CERT] [COMPONENT] Remove one service only by SRP client
     *
     * 3.4.1. Purpose
     * To test the following:
     * - SRP client needs to remove one service while other service remains registered.
     * - Correct usage of SRP Update to remove 1 service.
     * - Note: a Thread Product DUT must skip this test. Thread Product DUTs do not have a test case equivalent to this
     *   one, since they would then need to be instructed to register 2 services and then remove 1 service again. This
     *   seems unlikely to be practical.
     *
     * 3.4.2. Topology
     * - BR_1 - Test Bed border router device operating as a Thread Border Router, and the Leader
     * - Eth_1 - Test Bed border router device on an Adjacent Infrastructure Link
     * - TD_1 (DUT) - Thread End Device or Thread Router Component DUT
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Client       | N/A          | 2.3.2
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();
    Node &td1  = nexus.CreateNode();

    br1.SetName("BR_1");
    eth1.SetName("Eth_1");
    td1.SetName("TD_1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: BR_1 Description (SRPC-3.4): Enable");

    /**
     * Step 1
     * - Device: BR_1
     * - Description (SRPC-3.4): Enable
     * - Pass Criteria:
     *   - N/A
     */

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: TD_1 (DUT) Description (SRPC-3.4): Enable");

    /**
     * Step 2
     * - Device: TD_1 (DUT)
     * - Description (SRPC-3.4): Enable
     * - Pass Criteria:
     *   - N/A
     */

    td1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: BR 1 Description (SRPC-3.4): Automatically adds its SRP Server information");

    /**
     * Step 3
     * - Device: BR 1
     * - Description (SRPC-3.4): Automatically adds its SRP Server information in the DNSSRP/ Unicast Dataset in the
     *   Thread Network Data. Automatically configures an OMR prefix in Thread Network Data.
     * - Pass Criteria:
     *   - N/A
     */

    eth1.mInfraIf.Init(eth1);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);

    /** Add test variables for Python script. */
    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("TD_1_MLEID_ADDR", td1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("TD_1_OMR_ADDR", td1.FindGlobalAddress().ToString().AsCString());
    nexus.AddTestVar("BR_1_SRP_PORT", br1.Get<Srp::Server>().GetPort());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: TD 1 (DUT) Description (SRPC-3.4): Harness instructs device to register a new service");

    /**
     * Step 4
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.4): Harness instructs device to register a new service: $ORIGIN default.service.arpa.
     *   service-test-1. thread-test._udp ( SRV 1 1 55555 host-test-1 ) host-test-1 AAAA <OMR address of TD_1> with
     *   the following options: Update Lease Option Lease: 5 minutes Key Lease: 10 minutes.
     * - Pass Criteria:
     *   - N/A (see step 5 instead)
     */

    td1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(td1.Get<Srp::Client>().SetHostName(kSrpHostName1));
    SuccessOrQuit(td1.Get<Srp::Client>().EnableAutoHostAddress());
    td1.Get<Srp::Client>().SetLeaseInterval(kSrpLease5m);
    td1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease10m);

    ClearAllBytes(service1);
    service1.mName         = kSrpServiceType;
    service1.mInstanceName = kSrpInstanceName1;
    service1.mPort         = kSrpServicePort1;
    service1.mPriority     = 1;
    service1.mWeight       = 1;
    SuccessOrQuit(td1.Get<Srp::Client>().AddService(service1));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: BR 1 Description (SRPC-3.4): Automatically responds 'SUCCESS'");

    /**
     * Step 5
     * - Device: BR 1
     * - Description (SRPC-3.4): Automatically responds 'SUCCESS'.
     * - Pass Criteria:
     *   - MUST respond 'SUCCESS'.
     *   - Note: this verification step is done on the BR 1 response to ensure that the DUT in step 4 sent a correct
     *     SRP Update. This is a quick way to verify the Update was correct.
     */

    VerifyOrQuit(!AsConst(br1.Get<Srp::Server>()).GetHosts().IsEmpty());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: TD 1 (DUT) Description (SRPC-3.4): Harness instructs device to register a new, second "
        "service");

    /**
     * Step 6
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.4): Harness instructs device to register a new, second service: $ORIGIN
     *   default.service.arpa. service-test-2._thread-test._udp ( SRV 1 1 55556 host-test-1 ) host-test-1 AAAA <OMR
     *   address of TD 1> with the following options: Update Lease Option Lease: 3 minutes Key Lease: 10 minutes
     * - Pass Criteria:
     *   - N/A (see step 7 instead)
     */

    td1.Get<Srp::Client>().SetLeaseInterval(kSrpLease3m);

    ClearAllBytes(service2);
    service2.mName         = kSrpServiceType;
    service2.mInstanceName = kSrpInstanceName2;
    service2.mPort         = kSrpServicePort2;
    service2.mPriority     = 1;
    service2.mWeight       = 1;
    SuccessOrQuit(td1.Get<Srp::Client>().AddService(service2));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Device: BR 1 Description (SRPC-3.4): Automatically responds SUCCESS");

    /**
     * Step 7
     * - Device: BR 1
     * - Description (SRPC-3.4): Automatically responds SUCCESS.
     * - Pass Criteria:
     *   - MUST respond 'SUCCESS'.
     *   - Note: this verification step is done on the BR_1 response to ensure that the DUT in step 6 sent a correct
     *     SRP Update. This is a quick way to verify the Update was correct.
     */

    VerifyOrQuit(td1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: Device: TD 1 (DUT) Description (SRPC-3.4): Harness instructs device to remove its first service");

    /**
     * Step 8
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.4): Harness instructs device to remove its first service.
     * - Pass Criteria:
     *   - The DUT MUST send an SRP Update message such that the Service Discovery Instruction contains a single Delete
     *     an RR from an RRset ([RFC 2136], Section 2.5.4) update that deletes named records: $ORIGIN
     *     default.service.arpa. service-test-1._thread-test. udp ANY <ttl=0>
     *   - Note: if this is difficult to verify here then instead the SRP server response in step 11 can be used as
     *     validation that the correct service was removed.
     */

    SuccessOrQuit(td1.Get<Srp::Client>().RemoveService(service1));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: Device: BR_1 Description (SRPC-3.4): Automatically responds 'SUCCESS'");

    /**
     * Step 9
     * - Device: BR_1
     * - Description (SRPC-3.4): Automatically responds 'SUCCESS'.
     * - Pass Criteria:
     *   - MUST respond 'SUCCESS'.
     */

    VerifyOrQuit(td1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: Device: TD 1 (DUT) Description (SRPC-3.4): Harness instructs device via TH API to send unicast DNS "
        "query QType PTR");

    /**
     * Step 10
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.4): Harness instructs device via TH API to send unicast DNS query QType PTR to BR_1 for
     *   any services of type_thread-test._udp.default.service.arpa.". Note: this is done to validate that the SRP
     *   server removed the first service and kept the second service.
     * - Pass Criteria:
     *   - N/A
     */

    SuccessOrQuit(td1.Get<Dns::Client>().Browse(kSrpFullServiceType, nullptr, nullptr));

    nexus.AdvanceTime(kDnsQueryTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: Device: BR_1 Description (SRPC-3.4): Responds with the second service only");

    /**
     * Step 11
     * - Device: BR_1
     * - Description (SRPC-3.4): Responds with the second service only, not the first service.
     * - Pass Criteria:
     *   - MUST respond with the following service DNS-SD response to requesting client in the Answer records section:
     *     $ORIGIN default.service.arpa. _thread-test._udp ( PTR service-test-2._thread-test._udp )
     *   - DUT MUST NOT respond with further records in the Answer records section.
     *   - Response MAY contain in the Additional records section: $ORIGIN default.service.arpa.
     *     service-test-2._thread-test._udp ( SRV 1 1 55556 host-test-1 ) host-test-1 AAAA <OMR address of TD_1>
     */

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRPC_TC_4((argc > 2) ? argv[2] : "test_1_3_SRPC_TC_4.json");
    printf("All tests passed\n");
    return 0;
}
