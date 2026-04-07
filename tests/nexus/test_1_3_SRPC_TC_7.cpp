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
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP Lease time in seconds.
 */
static constexpr uint32_t kSrpLease1h = 3600;

/**
 * SRP Key Lease time in seconds.
 */
static constexpr uint32_t kSrpKeyLease1d = 86400;

/**
 * SRP service port.
 */
static constexpr uint16_t kSrpServicePort = 33333;

/**
 * SRP service and host names.
 */
static const char kSrpServiceType[]  = "_thread-test._udp";
static const char kSrpInstanceName[] = "service-test-1";
static const char kSrpHostName[]     = "host-test-1";

void Test_1_3_SRPC_TC_7(const char *aJsonFileName)
{
    /**
     * 3.7. [1.3] [CERT] [COMPONENT] Thread Device Reboots - Re-registers service with same KEY
     *
     * 3.7.1. Purpose
     * To verify that the Thread Component DUT:
     * - Is able to re-register a service again after reboot.
     * - Uses the same key for signing the SRP Update, as indicated in the KEY record.
     *
     * 3.7.2. Topology
     * - BR 1 Border Router Reference Device and Leader
     * - Router 1-Thread Router reference device
     * - TD_1 (DUT)-Any Thread Device (FTD or MTD): Thread Component DUT
     * - Eth 1-IPv6 host reference device on the AIL
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Client       | N/A          | 2.3.2
     */

    Core nexus;

    Node &br1     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &td1     = nexus.CreateNode();
    Node &eth1    = nexus.CreateNode();

    Srp::Client::Service service;

    br1.SetName("BR_1");
    router1.SetName("Router_1");
    td1.SetName("TD_1");
    eth1.SetName("Eth_1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Enable & form topology");

    /**
     * Step 1
     * - Device: Eth 1, BR 1, Router 1, TD 1
     * - Description (SRPC-3.7): Enable & form topology
     * - Pass Criteria:
     *   - Single Thread Network is formed
     */

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    router1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    td1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("TD_1_MLEID_ADDR", td1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("TD_1_OMR_ADDR", td1.FindGlobalAddress().ToString().AsCString());
    nexus.AddTestVar("BR_1_SRP_PORT", br1.Get<Srp::Server>().GetPort());

    Log("Step 2: Harness instructs the DUT to register the service");

    /**
     * Step 2
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.7): Harness instructs the DUT to register the service: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 33333 host-test-1 ) host-test-1 AAAA <OMR address of TD_1> with the
     *   following options: Update Lease Option Lease: 60 minutes Key Lease: 1 day
     * - Pass Criteria:
     *   - The DUT MUST send an SRP Update to BR_1
     *   - BR_1 MUST respond Rcode=0 (NoError).
     */

    td1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(td1.Get<Srp::Client>().SetHostName(kSrpHostName));
    SuccessOrQuit(td1.Get<Srp::Client>().EnableAutoHostAddress());
    td1.Get<Srp::Client>().SetLeaseInterval(kSrpLease1h);
    td1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease1d);

    {
        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(td1.Get<Srp::Client>().AddService(service));
    }

    nexus.AdvanceTime(kSrpRegistrationTime);

    VerifyOrQuit(td1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("Step 3: Harness instructs device to reboot.");

    /**
     * Step 3
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.7): Harness instructs device to reboot.
     * - Pass Criteria:
     *   - N/A
     */

    td1.Reset();
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 4: (Repeat step 2)");

    /**
     * Step 4
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.7): (Repeat step 2)
     * - Pass Criteria:
     *   - The DUT MUST send an SRP Update to BR_1
     *   - The KEY record in the SRP Update MUST have an equal value to the KEY record that was sent in step 2
     *   - BR_1 MUST respond Rcode=0 (NoError).
     */

    td1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    td1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(td1.Get<Srp::Client>().SetHostName(kSrpHostName));
    SuccessOrQuit(td1.Get<Srp::Client>().EnableAutoHostAddress());
    td1.Get<Srp::Client>().SetLeaseInterval(kSrpLease1h);
    td1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease1d);

    {
        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(td1.Get<Srp::Client>().AddService(service));
    }

    nexus.AdvanceTime(kSrpRegistrationTime);

    VerifyOrQuit(td1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRPC_TC_7((argc > 2) ? argv[2] : "test_1_3_SRPC_TC_7.json");
    printf("All tests passed\n");
    return 0;
}
