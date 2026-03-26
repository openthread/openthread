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
 * Time to advance for the BR to perform automatic actions (RA, Network Data), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 20 * 1000;

/**
 * Time to advance for SRP server information to be updated in Network Data.
 */
static constexpr uint32_t kSrpServerInfoUpdateTime = 20 * 1000;

/**
 * Time to advance for SRP registration, in milliseconds.
 */
static constexpr uint32_t kSrpRegistrationTime = 10 * 1000;

/**
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP service registration parameters.
 */
static const char         kSrpServiceType[]   = "_thread-test._udp";
static const char         kSrpInstanceName[]  = "service-test-1";
static const char         kSrpHostName[]      = "host-test-1";
static constexpr uint16_t kSrpServicePort     = 1500;
static constexpr uint8_t  kSrpServicePriority = 8;
static constexpr uint8_t  kSrpServiceWeight   = 8;
static constexpr uint32_t kSrpLease           = 300; // 5 minutes
static constexpr uint32_t kSrpKeyLease        = 600; // 10 minutes

void Test_1_3_SRP_TC_11(const char *aJsonFileName)
{
    Srp::Client::Service service;

    /**
     * 2.11. [1.3] [CERT] Recovery after reboot
     *
     * 2.11.1. Purpose
     * To test the following:
     * - 1. Handle SRP update after reboot of Thread device.
     * - 2. Handle SRP update after reboot of Border Router
     * - 3. Respond to mDNS queries on infrastructure interface after reboot of infrastructure device.
     *
     * 2.11.2. Topology
     * - 1. BR_1 (DUT) - Thread Border Router, and the Leader
     * - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 3. Eth 1-Test Bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Server       | N/A          | 1.3
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    Instance::SetLogLevel(kLogLevelNote);

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: BR 1 (DUT) Enable");
    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    MeshCoP::Dataset::Info datasetInfo;
    SuccessOrQuit(br1.Get<MeshCoP::ActiveDatasetManager>().Read(datasetInfo));

    /**
     * Step 2
     * - Device: Eth 1, ED 1
     * - Description (SRP-2.11): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    // Wait for BR to advertise prefixes and ED to get an OMR address
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically adds SRP Server information in the Thread Network Data.
     * - Pass Criteria:
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data. This MUST be a
     *     DNS/SRP Unicast Dataset, or a DNS/SRP Anycast Dataset, or both.
     *   - In case of Anycast Dataset: record the SRP-SN value for later use.
     */
    Log("Step 3: BR 1 (DUT) Automatically adds SRP Server information in the Thread Network Data.");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    {
        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT_1", srpPortString.AsCString());
    }

    /**
     * Step 4
     * - Device: ED_1
     * - Description (SRP-2.11): Harness instructs the device to send SRP Update: $ORIGIN
     *   default.service.arpa. service-test-1._thread-test._udp ( SRV 8 1500 host-test-1 ) host-test-1
     *   AAAA <OMR address of ED_1> KEY <public key of ED_1> with the following options: Update Lease
     *   Option, Lease: 5 minutes, Key Lease: 10 minutes
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED_1 send SRP Update");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        service.mPriority     = kSrpServicePriority;
        service.mWeight       = kSrpServiceWeight;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) Automatically sends SRP Update Response.");

    /**
     * Step 6
     * - Device: Eth_1
     * - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
     *   "service-test-1. thread-test, udp.local.".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: Eth_1 send mDNS query QType SRV");
    {
        eth1.Reset();
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
        nexus.AdvanceTime(kJoinNetworkTime);

        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 7
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
     *     service-test-1 answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 8 8 1500
     *     host-test-1 )
     */
    Log("Step 7: BR 1 (DUT) Automatically sends mDNS Response.");

    /**
     * Step 8
     * - Device: ED 1
     * - Description (SRP-2.11): Harness instructs the device to reboot/reset. Note: ED 1 MUST
     *   persist its public/private keypair that is used for signing SRP Updates. So it is not a
     *   factory-reset.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: ED 1 reboot/reset");
    ed1.Reset();
    ed1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Step 9
     * - Device: ED 1
     * - Description (SRP-2.11): Harness instructs the device to send SRP Update, as in step 4.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 9: ED 1 send SRP Update");
    {
        ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        service.mPriority     = kSrpServicePriority;
        service.mWeight       = kSrpServiceWeight;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 10
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 10: BR 1 (DUT) Automatically sends SRP Update Response.");

    /**
     * Step 11
     * - Device: Eth_1
     * - Description (SRP-2.11): Harness instructs the device to send mDNS query QType=SRV for
     *   "service-test-1._thread-test_udp.local.".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 11: Eth_1 send mDNS query QType=SRV");
    {
        eth1.Reset();
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
        nexus.AdvanceTime(kJoinNetworkTime);

        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 12
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
     *     service-test-1 answer record: SORIGIN local. service-test-1. thread-test._udp ( SRV 1500
     *     host-test-1 )
     */
    Log("Step 12: BR 1 (DUT) Automatically sends mDNS Response.");

    /**
     * Step 13
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): User is instructed to reboot the border router (DUT). Note: In case
     *   DUT uses an Anycast Dataset, this will include a different SRP-SN (compared to the previous
     *   SRP-SN recorded in step 3). This will then trigger re-registration in the next step. In
     *   case DUT uses a Unicast Dataset, this will include a different port number (compared to the
     *   previous port value used in step 3). This will then trigger re-registration.
     * - Pass Criteria:
     *   - After reboot, the DUT's SRP Server information MUST appear in the Thread Network Data.
     *     This MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
     *   - For an Anycast Dataset, the SRP-SN SHOULD be different but MAY be equal (in case DUT
     *     persistently stores data, only).
     *   - For a Unicast Dataset, the port number MUST differ.
     */
    Log("Step 13: BR 1 (DUT) reboot");
    {
        br1.Reset();
        br1.Get<MeshCoP::ActiveDatasetManager>().SaveLocal(datasetInfo);
        br1.Get<ThreadNetif>().Up();
        SuccessOrQuit(br1.Get<Mle::Mle>().Start());
        SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
        br1.Get<Srp::Server>().SetEnabled(true);
        br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
        br1.Get<BorderRouter::RoutingManager>().Init();
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
        nexus.AdvanceTime(kFormNetworkTime);
        nexus.AdvanceTime(kSrpServerInfoUpdateTime);

        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT_2", srpPortString.AsCString());
    }

    /**
     * Step 14
     * - Device: ED_1
     * - Description (SRP-2.11): Automatically re-registers using SRP Update, as in step 4.
     * - Pass Criteria:
     *   - MUST automatically re-register with SRP Update as in step 4.
     */
    Log("Step 14: ED_1 Automatically re-registers using SRP Update");
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 15
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 15: BR 1 (DUT) Automatically sends SRP Update Response.");

    /**
     * Step 16
     * - Device: Eth_1
     * - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
     *   "service-test-1. thread-test_udp.local.".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 16: Eth_1 send mDNS query QType SRV");
    {
        eth1.Reset();
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
        nexus.AdvanceTime(kJoinNetworkTime);

        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 17
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
     *     service-test-1 answer record: SORIGIN local. service-test-1._thread-test._udp ( SRV 8 8 1500
     *     host-test-1 )
     */
    Log("Step 17: BR 1 (DUT) Automatically sends mDNS Response.");

    /**
     * Step 18
     * - Device: Eth 1
     * - Description (SRP-2.11): Harness instructs the device to reset
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 18: Eth 1 reset");
    eth1.Reset();
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);

    /**
     * Step 19
     * - Device: ED 1
     * - Description (SRP-2.11): Harness instructs the device to send SRP Update, as in step 4.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 19: ED 1 send SRP Update");
    {
        // ED_1 will automatically refresh when lease is close to expiry,
        // but here we manually trigger an update by clearing and re-adding if needed,
        // or just wait for next registration if the test implies another update.
        // The spec says "Harness instructs the device to send SRP Update, as in step 4".
        ed1.Get<Srp::Client>().ClearHostAndServices();
        SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

        ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
        ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        service.mPriority     = kSrpServicePriority;
        service.mWeight       = kSrpServiceWeight;
        SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    }
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 20
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 20: BR 1 (DUT) Automatically sends SRP Update Response.");

    /**
     * Step 21
     * - Device: Eth 1
     * - Description (SRP-2.11): Harness instructs the device to send mDNS query QType SRV for
     *   "service-test-1. thread-test._udp.local.".
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 21: Eth 1 send mDNS query QType SRV");
    {
        eth1.Reset();
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
        nexus.AdvanceTime(kJoinNetworkTime);

        Dns::Multicast::Core::SrvResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback        = [](otInstance *, const otPlatDnssdSrvResult *) {};
        resolver.mServiceInstance = kSrpInstanceName;
        resolver.mServiceType     = kSrpServiceType;
        resolver.mInfraIfIndex    = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartSrvResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopSrvResolver(resolver));
    }

    /**
     * Step 22
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.11): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response, with RCODE=0 (NoError); and contains
     *     service-test-1. SORIGIN local. service-test-1. thread-test._udp ( SRV 8 8 1500 host-test-1 )
     */
    Log("Step 22: BR 1 (DUT) Automatically sends mDNS Response.");

    Log("All steps completed.");

    {
        Ip6::Prefix omrPrefix;
        SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));
        nexus.AddTestVar("ED_1_OMR", ed1.FindMatchingAddress(omrPrefix.ToString().AsCString()).ToString().AsCString());
    }

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_11((argc > 2) ? argv[2] : "test_1_3_SRP_TC_11.json");

    printf("All tests passed\n");
    return 0;
}
