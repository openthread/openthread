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

#include "common/clearable.hpp"

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
static const char         kSrpServiceType[]  = "_thread-test._udp";
static const char         kSrpInstanceName[] = "service-test-1";
static const char         kSrpHostName[]     = "host-test-1";
static const char         kSrpFullHostName[] = "host-test-1.default.service.arpa.";
static constexpr uint16_t kSrpServicePort    = 55555;
static constexpr uint32_t kSrpLease          = 300;     // 5 minutes
static constexpr uint32_t kSrpKeyLease       = 2419200; // 4 weeks

void Test_1_3_SRP_TC_13(const char *aJsonFileName)
{
    /**
     * 2.13. [1.3] [CERT] Thread Device address update
     *
     * 2.13.1. Purpose
     * To test the following:
     * - 1. Handle SRP update with change in IPv6 address(es) of the Thread Device
     *
     * 2.13.2. Topology
     * - 1. BR 1 (DUT) - Thread Border Router, and the Leader
     * - 2. ED 1-Test Bed device operating as a Thread End Device, attached to BR_1
     * - 3. Eth 1-Test bed border router device on an Adjacent Infrastructure Link
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Server       | N/A          | 1.3
     */

    Core                 nexus;
    Srp::Client::Service service;
    Ip6::Address         addresses[2];

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("Eth_1");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");

    /**
     * Step 1
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.13): Enable
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

    /**
     * Step 2
     * - Device: Eth 1, ED 1
     * - Description (SRP-2.13): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth 1, ED 1 Enable");
    ed1.Join(br1, Node::kAsFed);
    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));
    nexus.AdvanceTime(kJoinNetworkTime);
    nexus.AdvanceTime(kBrActionTime);

    /**
     * Step 3
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.13): Automatically adds its SRP Server information in the Thread Network Data.
     *   Automatically adds an OMR Prefix in the Network Data.
     * - Pass Criteria:
     *   - The DUT MUST register an OMR Prefix in the Thread Network Data.
     *   - The DUT's SRP Server information MUST appear in the Thread Network Data.
     *   - It MUST be a DNS/SRP Unicast Dataset, or DNS/SRP Anycast Dataset, or both.
     */
    Log("Step 3: BR 1 (DUT) SRP Server information and OMR Prefix in Network Data");
    nexus.AdvanceTime(kSrpServerInfoUpdateTime);

    Ip6::Prefix omrPrefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetOmrPrefix(omrPrefix));

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_1_MLEID_ADDR", ed1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    {
        String<10> srpPortString;
        srpPortString.Append("%u", br1.Get<Srp::Server>().GetPort());
        nexus.AddTestVar("BR_1_SRP_PORT", srpPortString.AsCString());
    }

    /**
     * Step 4
     * - Device: ED_1
     * - Description (SRP-2.13): Harness instructs the device to send SRP Update: $ORIGIN default.service.arpa.
     *   _thread-test. udp PTR service-test-1._thread-test._udp service-test-1. thread-test._udp. ( SRV 55555
     *   host-test-1 ) host-test-1 AAAA address_1 with the following options: Update Lease Option, Lease: 5 minutes,
     *   Key Lease: 4 weeks and where address_1 is the OMR address of ED_1.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 4: ED_1 send SRP Update with address_1");
    Ip6::Address address1 = ed1.FindMatchingAddress(omrPrefix.ToString().AsCString());
    nexus.AddTestVar("ADDRESS_1", address1.ToString().AsCString());
    ed1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostName(kSrpHostName));
    SuccessOrQuit(ed1.Get<Srp::Client>().EnableAutoHostAddress());

    ed1.Get<Srp::Client>().SetLeaseInterval(kSrpLease);
    ed1.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease);

    ClearAllBytes(service);
    service.mName         = kSrpServiceType;
    service.mInstanceName = kSrpInstanceName;
    service.mPort         = kSrpServicePort;
    SuccessOrQuit(ed1.Get<Srp::Client>().AddService(service));
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 5
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.13): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 5: BR 1 (DUT) Automatically sends SRP Update Response");

    /**
     * Step 6
     * - Device: ED 1
     * - Description (SRP-2.13): Harness instructs the device to send unicast QType=AAAA record DNS query for
     *   host-test-1.default.service.arpa.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED 1 send DNS query for host-test-1");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kSrpFullHostName, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 7
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.13): Automatically send DNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response containing: $ORIGIN default.service.arpa. host-test-1 AAAA address_1
     */
    Log("Step 7: BR 1 (DUT) Automatically send DNS Response");

    /**
     * Step 8
     * - Device: ED 1
     * - Description (SRP-2.13): Harness instructs the device to send SRP update with an address change: $ORIGIN
     *   default.service.arpa. _thread-test. udp PTR service-test-1._thread-test._udp service-test-1. thread-test._udp (
     *   SRV 55555 host-test-1 ) host-test-1 AAAA address 2 host-test-1 AAAA address_3 with the following options:
     *   Update Lease Option, Lease: 5 minutes, Key Lease: 4 weeks where address_2 is the ML-EID address of ED 1. And
     *   address_3 is a new Harness-configured address that has the OMR prefix. Note: using the OT command srp client
     *   host address address 2 address 3, these two addresses can be set for the SRP registration. Note: the OMR
     *   prefix can be obtained automatically by the Harness by querying netdata on ED_1.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 8: ED 1 send SRP update with address_2 and address_3");
    Ip6::Address address2 = ed1.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address address3 = address1;
    address3.mFields.m8[15] ^= 0xff; // Change the last byte to make it a "new" address
    nexus.AddTestVar("ADDRESS_2", address2.ToString().AsCString());
    nexus.AddTestVar("ADDRESS_3", address3.ToString().AsCString());
    addresses[0] = address2;
    addresses[1] = address3;
    SuccessOrQuit(ed1.Get<Srp::Client>().SetHostAddresses(addresses, 2));
    nexus.AdvanceTime(kSrpRegistrationTime);

    /**
     * Step 9
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.13): Automatically sends SRP Update Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid SRP Update Response, with RCODE=0 (NoError).
     */
    Log("Step 9: BR 1 (DUT) Automatically sends SRP Update Response");

    /**
     * Step 10
     * - Device: ED_1
     * - Description (SRP-2.13): Harness instructs the device to send QType=AAAA record unicast DNS query for
     *   host-test-1.default.service.arpa.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED 1 send DNS query for host-test-1");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kSrpFullHostName, nullptr, nullptr));
    nexus.AdvanceTime(kDnsQueryTime);

    /**
     * Step 11
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.13): Automatically sends DNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid DNS Response which contains the answer records: $ORIGIN default.service.arpa.
     *     host-test-1 AAAA address_2 host-test-1 AAAA address_3
     *   - The DNS Response MUST NOT contain address_1.
     */
    Log("Step 11: BR 1 (DUT) Automatically sends DNS Response");

    /**
     * Step 12
     * - Device: Eth_1
     * - Description (SRP-2.13): Harness instructs the device to send QType=AAAA record multicast mDNS query for
     *   host-test-1.local.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 12: Eth 1 send mDNS query for host-test-1.local");
    {
        Dns::Multicast::Core::AddressResolver resolver;
        ClearAllBytes(resolver);
        resolver.mCallback     = [](otInstance *, const otPlatDnssdAddressResult *) {};
        resolver.mHostName     = kSrpHostName;
        resolver.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartIp6AddressResolver(resolver));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopIp6AddressResolver(resolver));
    }

    /**
     * Step 13
     * - Device: BR_1 (DUT)
     * - Description (SRP-2.13): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response which contains at least one address: $ORIGIN local. Host-test-1 AAAA
     *     address_3
     *   - The response MAY further include the ML-EID address: $ORIGIN local. Host-test-1 AAAA address_2
     *   - mDNS Response MUST NOT contain address_1.
     */
    Log("Step 13: BR 1 (DUT) Automatically sends mDNS Response");

    /**
     * Step 14
     * - Device: Eth_1
     * - Description (SRP-2.13): Harness instructs the device to send QType KEY record multicast mDNS query for
     *   host-test-1.local.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 14: Eth 1 send mDNS query for KEY record");
    {
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(false, kInfraIfIndex));
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, kInfraIfIndex));

        Dns::Multicast::Core::RecordQuerier querier;
        ClearAllBytes(querier);
        querier.mCallback     = [](otInstance *, const otPlatDnssdRecordResult *) {};
        querier.mFirstLabel   = kSrpHostName;
        querier.mRecordType   = Dns::ResourceRecord::kTypeKey;
        querier.mInfraIfIndex = kInfraIfIndex;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StartRecordQuerier(querier));
        nexus.AdvanceTime(kDnsQueryTime);
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().StopRecordQuerier(querier));
    }

    /**
     * Step 15
     * - Device: BR 1 (DUT)
     * - Description (SRP-2.13): Automatically sends mDNS Response.
     * - Pass Criteria:
     *   - The DUT MUST send a valid mDNS Response, with RCODE=0.
     *   - It MAY contain either 0 Answer records, or it MAY contain 1 Answer record as follows: $ORIGIN local.
     *     Host-test-1 KEY some public key
     *   - The value some public key MAY be a dummy key value, for example zero.
     */
    Log("Step 15: BR 1 (DUT) Automatically sends mDNS Response");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRP_TC_13((argc > 2) ? argv[2] : "test_1_3_SRP_TC_13.json");

    printf("All tests passed\n");
    return 0;
}
