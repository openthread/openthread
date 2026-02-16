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

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

namespace {

static constexpr uint32_t kFormNetworkTime   = 10 * 1000;
static constexpr uint32_t kJoinNetworkTime   = 20 * 1000;
static constexpr uint32_t kStabilizationTime = 10 * 1000;

static constexpr char kGuaPrefix[]  = "2005:1234:abcd:100::/64";
static constexpr char kEth1Addr[]   = "2005:1234:abcd:100::E1";
static constexpr char kBr1AilAddr[] = "2005:1234:abcd:100::B1";

static constexpr char kThreadGroup1[] = "threadgroup1.org";
static constexpr char kThreadGroup2[] = "threadgroup2.org";

static constexpr char kThreadGroup1Aaaa[] = "2002:1234::E1:1";
static constexpr char kThreadGroup1A[]    = "192.168.217.1";
static constexpr char kThreadGroup2A[]    = "192.168.217.2";

static constexpr uint16_t kTypePrivateUse = 65285; // 0xFF05
static constexpr uint32_t kPrivateData    = 0x12345678;

static Node *sEth1Node = nullptr;

bool HandleUdpHook(Instance &aInstance, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Node       &node = AsNode(&aInstance);
    Dns::Header dnsHeader;
    char        name[Dns::Name::kMaxNameSize];
    uint16_t    readPos;
    uint16_t    qType;
    uint16_t    qClass;
    Message    *response;
    bool        handled = false;

    VerifyOrExit(&node == sEth1Node);
    VerifyOrExit(aMessageInfo.GetSockPort() == UpstreamDns::kDnsPort);

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), dnsHeader));
    VerifyOrExit(dnsHeader.GetType() == Dns::Header::kTypeQuery);
    VerifyOrExit(dnsHeader.GetQuestionCount() == 1);

    readPos = aMessage.GetOffset() + sizeof(dnsHeader);
    SuccessOrExit(Dns::Name::ReadName(aMessage, readPos, name));
    SuccessOrExit(aMessage.Read(readPos, qType));
    qType = BigEndian::HostSwap16(qType);
    readPos += sizeof(uint16_t);
    SuccessOrExit(aMessage.Read(readPos, qClass));
    qClass = BigEndian::HostSwap16(qClass);
    readPos += sizeof(uint16_t);

    Log("Eth_1 received DNS query for '%s' (type %u)", name, qType);

    response = node.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrExit(response != nullptr);

    dnsHeader.SetType(Dns::Header::kTypeResponse);
    dnsHeader.SetAnswerCount(0);
    dnsHeader.SetRecursionDesiredFlag();
    dnsHeader.SetRecursionAvailableFlag();

    if (Dns::Name::IsSameDomain(name, kThreadGroup1))
    {
        if (qType == Dns::ResourceRecord::kTypeAaaa)
        {
            Ip6::Address        address;
            Dns::ResourceRecord rr;

            SuccessOrQuit(address.FromString(kThreadGroup1Aaaa));
            dnsHeader.SetAnswerCount(1);
            SuccessOrQuit(response->Append(dnsHeader));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qType)));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qClass)));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            rr.Init(Dns::ResourceRecord::kTypeAaaa);
            rr.SetTtl(1800);
            rr.SetLength(sizeof(Ip6::Address));
            SuccessOrQuit(response->Append(rr));
            SuccessOrQuit(response->Append(address));
            Log("Eth_1 sending AAAA response for %s: %s", name, kThreadGroup1Aaaa);
        }
        else if (qType == Dns::ResourceRecord::kTypeA)
        {
            Ip4::Address        address;
            Dns::ResourceRecord rr;

            SuccessOrQuit(address.FromString(kThreadGroup1A));
            dnsHeader.SetAnswerCount(1);
            SuccessOrQuit(response->Append(dnsHeader));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qType)));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qClass)));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            rr.Init(Dns::ResourceRecord::kTypeA);
            rr.SetTtl(1800);
            rr.SetLength(sizeof(Ip4::Address));
            SuccessOrQuit(response->Append(rr));
            SuccessOrQuit(response->Append(address));
            Log("Eth_1 sending A response for %s: %s", name, kThreadGroup1A);
        }
    }
    else if (Dns::Name::IsSameDomain(name, kThreadGroup2))
    {
        if (qType == Dns::ResourceRecord::kTypeA)
        {
            Ip4::Address        address;
            Dns::ResourceRecord rr;

            SuccessOrQuit(address.FromString(kThreadGroup2A));
            dnsHeader.SetAnswerCount(1);
            SuccessOrQuit(response->Append(dnsHeader));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qType)));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qClass)));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            rr.Init(Dns::ResourceRecord::kTypeA);
            rr.SetTtl(1800);
            rr.SetLength(sizeof(Ip4::Address));
            SuccessOrQuit(response->Append(rr));
            SuccessOrQuit(response->Append(address));
            Log("Eth_1 sending A response for %s: %s", name, kThreadGroup2A);
        }
        else if (qType == kTypePrivateUse)
        {
            Dns::ResourceRecord rr;
            uint32_t            data = BigEndian::HostSwap32(kPrivateData);

            dnsHeader.SetAnswerCount(1);
            SuccessOrQuit(response->Append(dnsHeader));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qType)));
            SuccessOrQuit(response->Append(BigEndian::HostSwap16(qClass)));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            rr.Init(kTypePrivateUse);
            rr.SetTtl(1800);
            rr.SetLength(sizeof(uint32_t));
            SuccessOrQuit(response->Append(rr));
            SuccessOrQuit(response->Append(data));
            Log("Eth_1 sending PrivateUse response for %s", name);
        }
    }

    if (dnsHeader.GetAnswerCount() == 0)
    {
        Log("Eth_1 sending empty response for %s", name);
        SuccessOrQuit(response->Append(dnsHeader));
        SuccessOrQuit(Dns::Name::AppendName(name, *response));
        SuccessOrQuit(response->Append(BigEndian::HostSwap16(qType)));
        SuccessOrQuit(response->Append(BigEndian::HostSwap16(qClass)));
    }

    node.mInfraIf.SendUdp(aMessageInfo.GetSockAddr(), aMessageInfo.GetPeerAddr(), UpstreamDns::kDnsPort,
                          aMessageInfo.GetPeerPort(), *response);
    handled = true;

exit:
    return handled;
}

void HandleDnsResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aResponse);
    OT_UNUSED_VARIABLE(aContext);
}

void HandleDnsRecordResponse(otError aError, const otDnsRecordResponse *aResponse, void *aContext)
{
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aResponse);
    OT_UNUSED_VARIABLE(aContext);
}

void SendQuery(Node &aNode, const char *aName, uint16_t aType)
{
    if (aType == Dns::ResourceRecord::kTypeAaaa)
    {
        SuccessOrQuit(aNode.Get<Dns::Client>().ResolveAddress(aName, HandleDnsResponse, nullptr));
    }
    else
    {
        SuccessOrQuit(aNode.Get<Dns::Client>().QueryRecord(aType, nullptr, aName, HandleDnsRecordResponse, nullptr));
    }
}

void SendRa(Node &aNode, const Ip6::Address &aDnsAddr, const Ip6::Prefix &aPrefix)
{
    Ip6::Nd::RouterAdvert::TxMessage ra;
    Ip6::Nd::RouterAdvert::Header    header;
    Ip6::Nd::Icmp6Packet             packet;

    header.SetToDefault();
    header.SetRouterLifetime(1800);
    SuccessOrQuit(ra.Append(header));
    SuccessOrQuit(ra.AppendPrefixInfoOption(
        aPrefix, 1800, 1800, Ip6::Nd::PrefixInfoOption::kOnLinkFlag | Ip6::Nd::PrefixInfoOption::kAutoConfigFlag));
    SuccessOrQuit(ra.AppendRecursiveDnsServerOption(&aDnsAddr, 1, 1800));
    ra.GetAsPacket(packet);
    aNode.mInfraIf.SendIcmp6Nd(Ip6::Address::GetLinkLocalAllNodesMulticast(), packet.GetBytes(), packet.GetLength());
}

} // namespace

/**
 * 11.5. [1.4] [CERT] DNS record types and special cases
 *
 * 11.5.1. Purpose
 * To verify that the BR DUT:
 * - Can resolve A records
 * - Can resolve A records without doing IPv6 AAAA synthesis
 * - Can resolve AAAA records
 * - Can resolve records contributed by different sources (on-mesh, on-AIL, and upstream DNS server)
 * - Supports non-typical record types (RRTypes) for queries
 * - Supports queries using RR type 0xFF00-0xFFFE (65280-65534) “Private Use”
 * - Blocks the “ipv4only.arpa” query
 */
void Test_1_4_DNS_TC_5(const char *aJsonFileName)
{
    Core         nexus;
    Node        &br1  = nexus.CreateNode();
    Node        &ed1  = nexus.CreateNode();
    Node        &eth1 = nexus.CreateNode();
    Ip6::Address eth1Addr;
    Ip6::Address br1AilAddr;
    Ip6::Prefix  guaPrefix;

    br1.SetName("BR", 1);
    ed1.SetName("ED", 1);
    eth1.SetName("Eth", 1);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    sEth1Node = &eth1;
    eth1.mInfraIf.SetUdpHook(HandleUdpHook);

    AllowLinkBetween(br1, ed1);

    SuccessOrQuit(eth1Addr.FromString(kEth1Addr));
    SuccessOrQuit(br1AilAddr.FromString(kBr1AilAddr));
    SuccessOrQuit(guaPrefix.FromString(kGuaPrefix));

    /**
     * Step 1
     *   Device: Eth_1, BR_1, ED_1
     *   Description (DNS-11.5): Form topology. Thread network is formed.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 1: Form topology. Thread network is formed.");

    eth1.Get<ThreadNetif>().Up();
    eth1.mInfraIf.AddAddress(eth1Addr);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    ed1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.mInfraIf.AddAddress(br1AilAddr);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    SuccessOrQuit(br1.Get<Dns::ServiceDiscovery::Server>().Start());
    br1.Get<Dns::ServiceDiscovery::Server>().SetUpstreamQueryEnabled(true);

    {
        Dns::Client::QueryConfig dnsConfig;
        dnsConfig.Clear();
        AsCoreType(&dnsConfig.mServerSockAddr.mAddress) = br1.Get<Mle::Mle>().GetMeshLocalEid();
        dnsConfig.mServerSockAddr.mPort                 = 53;
        ed1.Get<Dns::Client>().SetDefaultConfig(dnsConfig);
    }

    // Send RA for BR to adopt the DNS server address and OMR prefix
    SendRa(eth1, eth1Addr, guaPrefix);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 2
     *   Device: Eth_1
     *   Description (DNS-11.5): Harness instructs device to configure DNS server with
     *     records: threadgroup1.org AAAA 2002:1234::E1:1, threadgroup1.org A 192.168.217.1,
     *     threadgroup2.org A 192.168.217.2, threadgroup2.org TYPE65285 \# 4 12345678.
     *     Note: the record type 0xFF05 is a private range record type, hence it has the
     *     symbolic name TYPE65285 associated.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 2: Eth_1 configures DNS server with records.");

    /**
     * Step 3
     *   Device: Eth_1
     *   Description (DNS-11.5): Harness instructs device to start advertising mDNS
     *     services on the AIL: Note: the PTR records for services are not shown below;
     *     however these are present as usual. $ORIGIN local. Service-test-1.
     *     _infra-test._udp ( SRV 0 0 55551 host-test-eth TXT dummy=abcd;thread-test=a49 )
     *     service-test-2._infra-test._udp ( SRV 0 0 55552 host-test-eth TXT a=2;thread-test
     *     =b50_test ) ) host-test-eth A <IPv4 address of Eth_1>.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 3: Eth_1 starts advertising mDNS services on the AIL.");
    {
        Dns::Multicast::Core::Service mdnsService;
        Ip6::Address                  eth1Ip4Addr;
        Ip4::Address                  ip4Addr;

        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetLocalHostName("host-test-eth"));
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().SetEnabled(true, 1));

        SuccessOrQuit(ip4Addr.FromString("192.168.217.100"));
        eth1Ip4Addr.SetToIp4Mapped(ip4Addr);
        eth1.mInfraIf.AddAddress(eth1Ip4Addr);
        eth1.mMdns.SignalIfAddresses(eth1.GetInstance());

        ClearAllBytes(mdnsService);
        mdnsService.mServiceInstance     = "service-test-1";
        mdnsService.mServiceType         = "_infra-test._udp";
        mdnsService.mHostName            = "host-test-eth";
        static const uint8_t kTxtData1[] = "\x0adummy=abcd\x0fthread-test=a49";
        mdnsService.mTxtData             = kTxtData1;
        mdnsService.mTxtDataLength       = sizeof(kTxtData1) - 1;
        mdnsService.mPort                = 55551;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().RegisterService(mdnsService, 1, nullptr));

        mdnsService.mServiceInstance     = "service-test-2";
        mdnsService.mPort                = 55552;
        static const uint8_t kTxtData2[] = "\x03a=2\x14thread-test=b50_test";
        mdnsService.mTxtData             = kTxtData2;
        mdnsService.mTxtDataLength       = sizeof(kTxtData2) - 1;
        SuccessOrQuit(eth1.Get<Dns::Multicast::Core>().RegisterService(mdnsService, 2, nullptr));
    }
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 4
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT
     *     with Qtype=AAAA for name “threadgroup1.org”.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 4: ED_1 sends a DNS query to the DUT with Qtype=AAAA for name threadgroup1.org.");
    SendRa(eth1, eth1Addr, guaPrefix);
    SendQuery(ed1, kThreadGroup1, Dns::ResourceRecord::kTypeAaaa);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 5
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS
     *     server, and provides the answer back to ED_1.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=0 (NoError) with answer record: threadgroup1.org
     *       AAAA 2002:1234::E1:1
     */
    Log("Step 5: BR_1 performs the DNS query via Eth_1 and provides the answer to ED_1.");

    /**
     * Step 6
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT,
     *     with Qtype=A for name “threadgroup1.org”.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 6: ED_1 sends a DNS query to the DUT with Qtype=A for name threadgroup1.org.");
    SendRa(eth1, eth1Addr, guaPrefix);
    SendQuery(ed1, kThreadGroup1, Dns::ResourceRecord::kTypeA);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 7
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS
     *     server, and provides the answer back to ED_1.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=0 (NoError) with answer record: threadgroup1.org A
     *       192.168.217.1
     */
    Log("Step 7: BR_1 performs the DNS query via Eth_1 and provides the answer to ED_1.");

    /**
     * Step 8
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send a DNS query to the DUT,
     *     with Qtype=AAAA for name “threadgroup2.org”.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 8: ED_1 sends a DNS query to the DUT with Qtype=AAAA for name threadgroup2.org.");
    SendRa(eth1, eth1Addr, guaPrefix);
    SendQuery(ed1, kThreadGroup2, Dns::ResourceRecord::kTypeAaaa);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 9
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically performs the DNS query via the Eth_1 DNS
     *     server, and provides the error answer back to ED_1. Note: this checks that the
     *     DUT does not perform AAAA synthesis from the A record.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=0 (NoError) with zero answer records (ANCOUNT=0).
     */
    Log("Step 9: BR_1 performs the DNS query via Eth_1 and provides the error answer to ED_1.");

    /**
     * Step 10
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send a DNS query, with
     *     Qtype=A for name “host-test-eth.default.service.arpa”.
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 10: ED_1 sends a DNS query with Qtype=A for host-test-eth.default.service.arpa.");
    SendQuery(ed1, "host-test-eth.default.service.arpa", Dns::ResourceRecord::kTypeA);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 11
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically performs the mDNS query on AIL
     *     (optionally, if still needed), and provides the answer back to ED_1.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=0 (NoError) with answer record: host-test-eth.
     *       default.service.arpa A <IPv4 address of Eth_1>
     */
    Log("Step 11: BR_1 performs mDNS query on AIL and provides the answer to ED_1.");

    /**
     * Step 12
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send DNS query Qtype=A for
     *     name “ipv4only.arpa”
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 12: ED_1 sends DNS query Qtype=A for name ipv4only.arpa.");
    SendQuery(ed1, "ipv4only.arpa", Dns::ResourceRecord::kTypeA);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 13
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically blocks the ipv4only.arpa query and
     *     responds NXDomain error.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=3 (NXDomain) with zero answer records (ANCOUNT=0).
     *     - The DUT MUST NOT send a DNS query to Eth_1.
     */
    Log("Step 13: BR_1 blocks the ipv4only.arpa query and responds NXDomain error.");

    /**
     * Step 14
     *   Device: ED_1
     *   Description (DNS-11.5): Harness instructs device to send DNS query with RR type
     *     in the private use range Qtype=0xFF05 for name “threadgroup2.org”. To do this
     *     with OT CLI, use the following: udp send <BR_1 ML-EID address> 53 -x
     *     e13c010000010000000000000c74687265616467726f757032036f726700ff050001 “”
     *   Pass Criteria:
     *     - N/A
     */
    Log("Step 14: ED_1 sends DNS query with RR type in private use range Qtype=0xFF05.");
    SendRa(eth1, eth1Addr, guaPrefix);
    SendQuery(ed1, kThreadGroup2, kTypePrivateUse);
    nexus.AdvanceTime(kStabilizationTime);

    /**
     * Step 15
     *   Device: BR_1 (DUT)
     *   Description (DNS-11.5): Automatically responds with the requested record.
     *   Pass Criteria:
     *     - The DUT MUST respond Rcode=0 (NoError) with answer record with binary data
     *       (4 bytes): threadgroup2.org 0xFF05 0x12345678
     */
    Log("Step 15: BR_1 responds with the requested record.");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_4_DNS_TC_5((argc > 2) ? argv[2] : "test_1_4_dns_tc_5.json");
    ot::Nexus::Log("All tests passed");
    return 0;
}
