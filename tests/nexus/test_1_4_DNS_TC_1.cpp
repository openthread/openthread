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
 * 11.1. [1.3] [CERT] Handling of multi-question DNS queries
 *
 * 11.1.1. Purpose
 *   - To verify that the Thread Border Router DUT can successfully handle DNS queries with
 *     multiple questions (QDCOUNT > 1) by either:
 *   - Responding with an error status (signaling to the querier that it needs to retry with
 *     single-question queries)
 *   - Providing the answers to the multiple questions (as defined by Thread) if the questions
 *     are for the same QNAME but different record types.
 *
 * 11.1.2. Topology
 *   - Eth_1 host registering a service
 *   - BR_1 Border Router DUT
 *   - Thread Router_1 registering service(s) (Connected to BR DUT)
 *   - Thread ED_1 sending QDCOUNT > 1 queries (Child of Router_1)
 */

namespace {

static constexpr uint32_t kFormNetworkTime   = 13 * 1000;
static constexpr uint32_t kJoinNetworkTime   = 20 * 1000;
static constexpr uint32_t kStabilizationTime = 10 * 1000;
static constexpr uint16_t kServicePort       = 55556;

void SendMultiQuestionQuery(Node &aNode, const Ip6::Address &aDest, const char *aName, uint16_t aType1, uint16_t aType2)
{
    Ip6::Udp::Socket socket(aNode, nullptr, nullptr);
    SuccessOrQuit(socket.Open(Ip6::kNetifUnspecified));

    Message *message = socket.NewMessage();
    VerifyOrQuit(message != nullptr);

    Dns::Header header;
    header.SetType(Dns::Header::kTypeQuery);
    header.SetQuestionCount(2);
    SuccessOrQuit(message->Append(header));

    SuccessOrQuit(Dns::Name::AppendName(aName, *message));
    Dns::Question question1(aType1);
    SuccessOrQuit(message->Append(question1));

    SuccessOrQuit(Dns::Name::AppendName(aName, *message));
    Dns::Question question2(aType2);
    SuccessOrQuit(message->Append(question2));

    Ip6::MessageInfo messageInfo;
    messageInfo.SetPeerAddr(aDest);
    messageInfo.SetPeerPort(53);

    SuccessOrQuit(socket.SendTo(*message, messageInfo));
    SuccessOrQuit(socket.Close());
}

void SendSingleQuestionQuery(Node &aNode, const Ip6::Address &aDest, const char *aName, uint16_t aType)
{
    Ip6::Udp::Socket socket(aNode, nullptr, nullptr);
    SuccessOrQuit(socket.Open(Ip6::kNetifUnspecified));

    Message *message = socket.NewMessage();
    VerifyOrQuit(message != nullptr);

    Dns::Header header;
    header.SetType(Dns::Header::kTypeQuery);
    header.SetQuestionCount(1);
    SuccessOrQuit(message->Append(header));

    SuccessOrQuit(Dns::Name::AppendName(aName, *message));
    Dns::Question question(aType);
    SuccessOrQuit(message->Append(question));

    Ip6::MessageInfo messageInfo;
    messageInfo.SetPeerAddr(aDest);
    messageInfo.SetPeerPort(53);

    SuccessOrQuit(socket.SendTo(*message, messageInfo));
    SuccessOrQuit(socket.Close());
}

} // namespace

void Test_1_4_DNS_TC_1(const char *aJsonFileName)
{
    Core nexus;

    Node &eth1 = nexus.CreateNode();
    Node &br1  = nexus.CreateNode();
    Node &r1   = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();

    Dns::Multicast::Core::Host    host;
    Ip6::Address                  eth1Addr;
    Dns::Multicast::Core::Service mdnsService;
    Srp::Client::Service          srpService;
    static const Dns::TxtEntry    kTxtEntries[] = {{"key1", reinterpret_cast<const uint8_t *>("value1"), 6}};

    eth1.SetName("Eth", 1);
    br1.SetName("BR", 1);
    r1.SetName("Router", 1);
    ed1.SetName("ED", 1);

    IgnoreError(Instance::SetGlobalLogLevel(kLogLevelNote));

    /**
     * Step 1
     *   Device: All
     *   Description (DNS-11.1): Start topology
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 1: Topology: Start Eth_1, BR_1, Router_1, and ED_1.");

    // BR_1 and Router_1
    br1.AllowList(r1);
    r1.AllowList(br1);

    // Router_1 and ED_1
    r1.AllowList(ed1);
    ed1.AllowList(r1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    r1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    ed1.Join(r1);
    nexus.AdvanceTime(kJoinNetworkTime);

    // Setup BR_1 as Border Router
    br1.Get<BorderRouter::InfraIf>().Init(Mdns::kInfraIfIndex, true);
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    br1.Get<Srp::Server>().SetEnabled(true);
    SuccessOrQuit(br1.Get<Dns::ServiceDiscovery::Server>().Start());

    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2
     *   Device: Eth_1
     *   Description (DNS-11.1): Harness instructs device to advertise a test service using
     *     mDNS: $ORIGIN local. Service-test-2._thread-test._udp ( SRV 0 0 55556 host-eth-1
     *     TXT key2=value2 ) host-eth-1 AAAA <ULA address of Eth_1>
     *   Pass Criteria:
     *     - mDNS service MUST be registered without name conflict or service name change.
     */
    Log("Step 2: Eth_1 host registers a service using mDNS.");

    SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, Mdns::kInfraIfIndex));
    {
        SuccessOrQuit(eth1Addr.FromString("fd00:1234:5678:abcd::1"));
        eth1.mInfraIf.AddAddress(eth1Addr);

        ClearAllBytes(host);
        host.mHostName        = "host-eth-1";
        host.mAddresses       = &eth1Addr;
        host.mAddressesLength = 1;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().RegisterHost(host, 0, nullptr));

        ClearAllBytes(mdnsService);
        mdnsService.mServiceInstance = "service-test-2";
        mdnsService.mServiceType     = "_thread-test._udp";
        mdnsService.mHostName        = "host-eth-1";
        mdnsService.mTxtData         = reinterpret_cast<const uint8_t *>("\x0bkey2=value2");
        mdnsService.mTxtDataLength   = 12;
        mdnsService.mPort            = kServicePort;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().RegisterService(mdnsService, 1, nullptr));
    }
    nexus.AdvanceTime(kStabilizationTime);

    Ip6::Address br1Addr = br1.Get<Mle::Mle>().GetMeshLocalEid();

    /**
     * Step 3
     *   Device: Router_1
     *   Description (DNS-11.1): Harness instructs device to register a test service using
     *     SRP: $ORIGIN default.service.arpa. service-test-1._thread-test._udp ( SRV 0 0
     *     55556 host-router-1 TXT key1=value1 ) host-router-1 AAAA <OMR address of
     *     Router_1>
     *   Pass Criteria:
     *     - The DUT MUST respond with success status (RCODE=0) to the SRP registration.
     */
    Log("Step 3: Router_1 host registers a service using SRP.");

    {
        Ip6::SockAddr serverSockAddr;
        serverSockAddr.SetAddress(br1Addr);
        serverSockAddr.SetPort(br1.Get<Srp::Server>().GetPort());
        SuccessOrQuit(r1.Get<Srp::Client>().Start(serverSockAddr));
    }

    SuccessOrQuit(r1.Get<Srp::Client>().SetHostName("host-router-1"));
    SuccessOrQuit(r1.Get<Srp::Client>().EnableAutoHostAddress());

    {
        ClearAllBytes(srpService);
        srpService.mName          = "_thread-test._udp";
        srpService.mInstanceName  = "service-test-1";
        srpService.mPort          = kServicePort;
        srpService.mTxtEntries    = kTxtEntries;
        srpService.mNumTxtEntries = 1;
        SuccessOrQuit(srpService.Init());
        SuccessOrQuit(r1.Get<Srp::Client>().AddService(srpService));
    }
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs device to perform DNS query with QDCOUNT=2:
     *     First question QNAME=service-test-1._thread-test._udp. Default.service.arpa QTYPE
     *     =SRV Second question QNAME=service-test-1._thread-test._udp. Default.service.arpa
     *     QTYPE =TXT
     *   Pass Criteria:
     *     - The DUT MUST either: 3. Respond with error RCODE=1 (FormErr) and 0 answers; or 3.
     *       Respond with success RCODE=0 (NoError) and two answer records in the Answers
     *       section as follows: $ORIGIN default.service.arpa.
     *       service-test-1._thread-test._udp ( SRV 0 0 55556 host-router-1 TXT key1=value1 )
     *     - Furthermore the Additional records section MAY contain : host-router-1 AAAA <OMR
     *       address of Router_1>
     *     - If this record is present, address MUST be same as in step 9.
     */
    Log("Step 4: ED_1 performs a DNS query with two questions (QDCOUNT=2).");
    SendMultiQuestionQuery(ed1, br1Addr, "service-test-1._thread-test._udp.default.service.arpa",
                           Dns::ResourceRecord::kTypeSrv, Dns::ResourceRecord::kTypeTxt);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs devie to perform DNS query with QDCOUNT=2:
     *     First question QNAME=service-test-2._thread-test._udp. Default.service.arpa QTYPE
     *     =SRV Second question QNAME=service-test-2._thread-test._udp. Default.service.arpa
     *     QTYPE =TXT
     *   Pass Criteria:
     *     - The DUT MUST either: 3. 3. Respond with error RCODE=1 (FormErr) and 0 answers;
     *       or Respond with success RCODE=0 (NoError) and two answer records in the
     *       Answers section as follows: $ORIGIN default.service.arpa.
     *       service-test-2._thread-test._udp ( SRV 0 0 55556 host-eth-1 TXT key2=value2 )
     *     - Furthermore the Additional records section MAY contain : host-eth-1 AAAA <ULA
     *       address of Eth_1>
     *     - If this record is present, address MUST be same as in step 8.
     */
    Log("Step 5: ED_1 performs a DNS query with two questions (QDCOUNT=2).");
    SendMultiQuestionQuery(ed1, br1Addr, "service-test-2._thread-test._udp.default.service.arpa",
                           Dns::ResourceRecord::kTypeSrv, Dns::ResourceRecord::kTypeTxt);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 6
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs device to perform DNS query with
     *     QDCOUNT=1: First question QNAME=service-test-1_thread-test.
     *     _udp.default.service.arpa QTYPE=SRV
     *   Pass Criteria:
     *     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
     *       the Answers section as follows: $ORIGIN default.service.arpa.
     *       service-test-1._thread-test._udp ( SRV 0 0 55556 host-router-1 )
     *     - Furthermore the Additional records section MAY contain : host-router-1 AAAA
     *       <OMR address of Router_1>
     */
    Log("Step 6: ED_1 performs a DNS query for service-test-1 (SRV).");
    SendSingleQuestionQuery(ed1, br1Addr, "service-test-1._thread-test._udp.default.service.arpa",
                            Dns::ResourceRecord::kTypeSrv);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs device to perform DNS query with
     *     QDCOUNT=1: First question QNAME=service-test-2_thread-test.
     *     _udp.default.service.arpa QTYPE=TXT
     *   Pass Criteria:
     *     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
     *       the Answers section as follows: $ORIGIN default.service.arpa.
     *       service-test-2._thread-test._udp ( TXT key2=value2 )
     */
    Log("Step 7: ED_1 performs a DNS query for service-test-2 (TXT).");
    SendSingleQuestionQuery(ed1, br1Addr, "service-test-2._thread-test._udp.default.service.arpa",
                            Dns::ResourceRecord::kTypeTxt);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 8
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs device to perform DNS query with
     *     QDCOUNT=1: First question QNAME=host-eth-1.default.service.arpa QTYPE=AAAA
     *   Pass Criteria:
     *     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
     *       the Answers section as follows: $ORIGIN default.service.arpa. host-eth-1 AAAA
     *       <ULA address of Eth_1>
     *     - The DUT MUST NOT include a link-local address in an AAAA record in the
     *       answer(s).
     */
    Log("Step 8: ED_1 performs a DNS query for host-eth-1 (AAAA).");
    SendSingleQuestionQuery(ed1, br1Addr, "host-eth-1.default.service.arpa", Dns::ResourceRecord::kTypeAaaa);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 9
     *   Device: ED_1
     *   Description (DNS-11.1): Harness instructs device to perform DNS query with
     *     QDCOUNT=1: First question QNAME=host-router-1.default.service.arpa QTYPE=AAAA
     *   Pass Criteria:
     *     - The DUT MUST respond with success RCODE=0 (NoError) and one answer record in
     *       the Answers section as follows: $ORIGIN default.service.arpa. host-router-1
     *       AAAA <OMR address of Router_1>
     *     - The DUT MUST NOT include a link-local address in an AAAA record in the
     *       answer(s).
     */
    Log("Step 9: ED_1 performs a DNS query for host-router-1 (AAAA).");
    SendSingleQuestionQuery(ed1, br1Addr, "host-router-1.default.service.arpa", Dns::ResourceRecord::kTypeAaaa);
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_4_DNS_TC_1((argc > 2) ? argv[2] : "test_1_4_dns_tc_1.json");
    ot::Nexus::Log("All tests passed");
    return 0;
}
