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
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 5 * 1000;

/**
 * Time to wait for SRP renewal.
 */
static constexpr uint32_t kSrpRenewalTime = 30 * 1000;

/**
 * SRP Lease time in seconds.
 */
static constexpr uint32_t kSrpLease = 30;

/**
 * SRP Key Lease time in seconds.
 */
static constexpr uint32_t kSrpKeyLease = 600;

/**
 * SRP service port.
 */
static constexpr uint16_t kSrpServicePort = 33333;

/**
 * Time to wait for SRP server switch.
 */
static constexpr uint32_t kSrpServerSwitchTime = 300 * 1000;

void Test_1_3_SRPC_TC_1(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 3.1. [1.3] [CERT] [COMPONENT] Re-register service with active SRP server - Multiple BRs - No AIL
     *
     * 3.1.1. Purpose
     * To test the following:
     * - DNS/SRP Unicast Dataset(s) offered in the Thread network
     * - Detects unresponsiveness of the SRP server and sends SRP update to another active SRP server
     * - Detects a second, numerically lower, SRP server and DUT stays with its current SRP server and does not switch
     *   to the numerically lowest SRP server
     * - Registered service is automatically re-registered with active SRP server before timeout
     * - In the absence of an OMR prefix, mesh-local (ML-EID) address will be used by SRP client.
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Client       | N/A          | 2.3.2
     */

    Core nexus;

    Node &br1     = nexus.CreateNode();
    Node &br2     = nexus.CreateNode();
    Node &ed1     = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();

    br1.SetName("BR_1");
    br2.SetName("BR_2");
    ed1.SetName("ED_1");
    router1.SetName("Router_1");

    // Set Mesh-Local IID to be deterministic and ensure BR_1 has a numerically lower ML-EID than BR_2.
    {
        Ip6::InterfaceIdentifier iid;

        iid.Clear();
        iid.mFields.m8[7] = 1;
        SuccessOrQuit(br1.Get<Mle::Mle>().SetMeshLocalIid(iid));

        iid.mFields.m8[7] = 2;
        SuccessOrQuit(br2.Get<Mle::Mle>().SetMeshLocalIid(iid));
    }

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Router 1 Enable. Automatically, becomes Leader.");

    /**
     * Step 0
     * - Device: Router 1
     * - Description (SRPC-3.1): Enable. Automatically, becomes Leader.
     * - Pass Criteria:
     *   - N/A
     */

    router1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: BR_1 Enable: Harness configures the device with a numerically lower ML-EID.");

    /**
     * Step 1
     * - Device: BR_1
     * - Description (SRPC-3.1): Enable: Harness configures the device with a numerically lower ML-EID address than
     *   BR_2 will have. Note: due to absence of adjacent infrastructure link (AIL), no OMR address is configured by
     *   the BR.
     * - Pass Criteria:
     *   - N/A
     */

    AllowLinkBetween(br1, br2);
    AllowLinkBetween(br1, router1);
    AllowLinkBetween(br2, router1);
    AllowLinkBetween(router1, ed1);

    br1.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime * 2);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ED 1 (DUT) Enable; automatically connects to Router_1.");

    /**
     * Step 2
     * - Device: ED 1 (DUT)
     * - Description (SRPC-3.1): Enable; automatically connects to Router_1.
     * - Pass Criteria:
     *   - N/A
     */

    ed1.Join(router1, Node::kAsFed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: BR 1 Automatically adds its SRP Server information in the Thread Network Data.");

    /**
     * Step 3
     * - Device: BR 1
     * - Description (SRPC-3.1): Automatically adds its SRP Server information in the Thread Network Data by using a
     *   DNS/SRP Unicast Dataset, indicating its ML-EID address.
     * - Pass Criteria:
     *   - N/A
     */

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.AddTestVar("BR_1_SRP_PORT", br1.Get<Srp::Server>().GetPort());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: ED 1 (DUT) Harness instructs DUT to add a service to the device.");

    /**
     * Step 4
     * - Device: ED 1 (DUT)
     * - Description (SRPC-3.1): Harness instructs DUT to add a service to the device: $ORIGIN default.service.arpa.
     *   service-test-1._thread-test._udp ( SRV 33333 host-test-1 ) host-test-1 AAAA <ML-EID address of ED_1> with the
     *   following options: Update Lease Option Lease: 30 seconds Key Lease: 10 minutes
     * - Pass Criteria:
     *   - The DUT ED_1 MUST send SRP Update to BR_1 containing the service-test-1.
     *   - The IPv6 address in the AAAA record of the SRP Update MUST be an ML-EID, specifically the ML-EID of the DUT.
     *   - It MUST NOT contain any other IPv6 addresses in AAAA records.
     */

    ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName("host-test-1"));
    SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());
    ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
    ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

    ClearAllBytes(service);
    service.mName         = "_thread-test._udp";
    service.mInstanceName = "service-test-1";
    service.mPort         = kSrpServicePort;
    SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: BR_1 Automatically sends DNS Response (Rcode=0, NoError).");

    /**
     * Step 5
     * - Device: BR_1
     * - Description (SRPC-3.1): Automatically sends DNS Response (Rcode=0, NoError).
     * - Pass Criteria:
     *   - N/A
     */

    VerifyOrQuit(ed1.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: BR 2 Enable. Harness configures device with a numerically higher ML-EID.");

    /**
     * Step 6
     * - Device: BR 2
     * - Description (SRPC-3.1): Enable. Harness configures device with a numerically higher ML-EID address than BR 1.
     *   This ensures that the BR 2 SRP service will be less preferred to the DUT. Note: due to absence of adjacent
     *   infrastructure link (AIL), no OMR address is configured by the BR.
     * - Pass Criteria:
     *   - N/A
     */

    br2.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    /** Verify BR_1 has a numerically lower ML-EID than BR_2. */
    VerifyOrQuit(br1.Get<Mle::Mle>().GetMeshLocalEid() < br2.Get<Mle::Mle>().GetMeshLocalEid());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: BR 2 Harness instructs the device to add its SRP Server information.");

    /**
     * Step 7
     * - Device: BR 2
     * - Description (SRPC-3.1): Harness instructs the device to add its SRP Server information in the Thread Network
     *   Data using a DNS/SRP Unicast Dataset. Note: normally BR_2 does not add this information, since an SRP server
     *   is already advertised on the Thread Network. The Harness overrides this behavior.
     * - Pass Criteria:
     *   - The DUT MUST NOT send SRP Update to BR_2.
     */

    SuccessOrQuit(br2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br2.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(30 * 1000);

    nexus.AddTestVar("BR_2_SRP_PORT", br2.Get<Srp::Server>().GetPort());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8: BR 1 Harness deactivates the device.");

    /**
     * Step 8
     * - Device: BR 1
     * - Description (SRPC-3.1): Harness deactivates the device.
     * - Pass Criteria:
     *   - N/A
     */

    br1.Get<Srp::Server>().SetEnabled(false);
    br1.Get<Mle::Mle>().Stop();
    br1.Get<ThreadNetif>().Down();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 8b: Harness waits 30 seconds. Within this time, the next step must happen.");

    /**
     * Step 8b
     * - Device: N/A
     * - Description (SRPC-3.1): Harness waits 30 seconds. Within this time, the next step must happen.
     * - Pass Criteria:
     *   - N/A
     */

    nexus.AdvanceTime(kSrpRenewalTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 9: ED 1 (DUT) Automatically sends SRP Update to BR_1 to renew service.");

    /**
     * Step 9
     * - Device: ED 1 (DUT)
     * - Description (SRPC-3.1): Automatically sends SRP Update to BR_1 to renew service.
     * - Pass Criteria:
     *   - The DUT MUST send SRP Update to BR_1 as in step 4.
     */

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 10: BR 1 Does not respond.");

    /**
     * Step 10
     * - Device: BR 1
     * - Description (SRPC-3.1): Does not respond.
     * - Pass Criteria:
     *   - N/A
     */

    Log("---------------------------------------------------------------------------------------");
    Log("Step 11: ED 1 (DUT) Automatically sends SRP Update to BR_2");

    /**
     * Step 11
     * - Device: ED 1 (DUT)
     * - Description (SRPC-3.1): Automatically sends SRP Update to BR_2
     * - Pass Criteria:
     *   - The DUT MUST send SRP Update like in step 4 but now to BR_2
     */

    nexus.AdvanceTime(kSrpServerSwitchTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 12: BR 2 Automatically sends SRP Update Response (Rcode=0, NoError).");

    /**
     * Step 12
     * - Device: BR 2
     * - Description (SRPC-3.1): Automatically sends SRP Update Response (Rcode=0, NoError).
     * - Pass Criteria:
     *   - N/A
     */

    VerifyOrQuit(!AsConst(br2.Get<Srp::Server>()).GetHosts().IsEmpty());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 13: BR_1 Harness re-enables the device.");

    /**
     * Step 13
     * - Device: BR_1
     * - Description (SRPC-3.1): Harness re-enables the device.
     * - Pass Criteria:
     *   - N/A
     */

    br1.Get<ThreadNetif>().Up();
    SuccessOrQuit(br1.Get<Mle::Mle>().Start());
    br1.Join(router1);
    nexus.AdvanceTime(kAttachToRouterTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 14: BR_1 Harness instructs the device to add its SRP Server information.");

    /**
     * Step 14
     * - Device: BR_1
     * - Description (SRPC-3.1): Harness instructs the device to add its SRP Server information in the Thread Network
     *   Data using a DNS/SRP Unicast Dataset. Note: normally BR_1 does not add this information, since an SRP server
     *   is already advertised on the Thread Network. The Harness overrides this behavior.
     * - Pass Criteria:
     *   - N/A
     */

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicastForceAdd));
    br1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 15: ED 1 (DUT) Does not send SRP Update to BR_1, but stays with its current server.");

    /**
     * Step 15
     * - Device: ED 1 (DUT)
     * - Description (SRPC-3.1): Does not send SRP Update to BR_1, but stays with its current SRP server BR 2..
     * - Pass Criteria:
     *   - The DUT MUST NOT send SRP Update to BR_1.
     */

    nexus.AdvanceTime(kSrpRegistrationTime);

    Log("---------------------------------------------------------------------------------------");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRPC_TC_1((argc > 2) ? argv[2] : "test_1_3_SRPC_TC_1.json");
    printf("All tests passed\n");
    return 0;
}
