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

#include "core/net/ip4_types.hpp"
#include "core/net/nat64_translator.hpp"
#include "net/checksum.hpp"
#include "net/dhcp6_types.hpp"
#include "net/dns_types.hpp"
#include "net/tcp6.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;
static constexpr uint32_t kBrActionTime    = 30 * 1000;
static constexpr uint32_t kDnsTime         = 5 * 1000;
static constexpr uint32_t kPingTime        = 2 * 1000;
static constexpr uint32_t kTcpTime         = 10 * 1000;
static constexpr uint16_t kDnsPort         = 53;
static constexpr uint16_t kHttpPort        = 80;
static constexpr uint16_t kEchoIdentifier  = 0x1234;
static constexpr uint16_t kEchoPayloadSize = 10;
static constexpr uint32_t kInfraIfIndex    = 1;

static const char kInternetServerIp4Addr[] = "38.106.217.165";
static const char kLocalServerIp4Addr[]    = "192.168.1.4";
static const char kEth2Prefix[]            = "2001:db8:1::/64";
static const char kNat64Cidr[]             = "192.168.100.0/24";

static bool HandleEth2Udp(Instance &aInstance, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Node &eth2    = Node::From(&aInstance);
    bool  handled = false;

    if (aMessageInfo.GetSockPort() == kDnsPort)
    {
        Dns::Header header;
        SuccessOrQuit(aMessage.Read(aMessage.GetOffset(), header));

        if (header.GetType() == Dns::Header::kTypeQuery)
        {
            Message *reply = eth2.Get<MessagePool>().Allocate(Message::kTypeIp6);
            VerifyOrQuit(reply != nullptr);

            Dns::Header replyHeader;
            replyHeader.SetMessageId(header.GetMessageId());
            replyHeader.SetType(Dns::Header::kTypeResponse);
            replyHeader.SetQueryType(Dns::Header::kQueryTypeStandard);
            replyHeader.SetRecursionDesiredFlag();
            replyHeader.SetRecursionAvailableFlag();
            replyHeader.SetQuestionCount(1);
            replyHeader.SetAnswerCount(1);
            SuccessOrQuit(reply->Append(replyHeader));

            uint16_t offset      = aMessage.GetOffset() + sizeof(Dns::Header);
            uint16_t questionLen = aMessage.GetLength() - offset;
            SuccessOrQuit(reply->AppendBytesFromMessage(aMessage, offset, questionLen));

            SuccessOrQuit(Dns::Name::AppendName("threadv4.org", *reply));

            Dns::ResourceRecord rr;
            rr.SetType(Dns::ResourceRecord::kTypeA);
            rr.SetClass(Dns::ResourceRecord::kClassInternet);
            rr.SetTtl(3600);
            rr.SetLength(sizeof(Ip4::Address));
            SuccessOrQuit(reply->Append(rr));

            Ip4::Address addr;
            SuccessOrQuit(addr.FromString(kInternetServerIp4Addr));
            SuccessOrQuit(reply->Append(addr));

            eth2.mInfraIf.SendUdp(eth2.mInfraIf.GetLinkLocalAddress(), aMessageInfo.GetPeerAddr(), kDnsPort,
                                  aMessageInfo.GetPeerPort(), *reply);
            handled = true;
        }
    }

    return handled;
}

static void HandleIp4Receive(otMessage *aMessage, void *aContext)
{
    Node        *br1     = static_cast<Node *>(aContext);
    Message     *message = static_cast<Message *>(aMessage);
    Ip4::Headers ip4Headers;

    VerifyOrQuit(ip4Headers.ParseFrom(*message) == kErrorNone);

    if (ip4Headers.IsIcmp4())
    {
        Log("Received ICMPv4 packet");
        Ip4::Icmp::Header icmpHeader;
        SuccessOrQuit(message->Read(sizeof(Ip4::Header), icmpHeader));

        if (icmpHeader.GetType() == Ip4::Icmp::Header::kTypeEchoRequest)
        {
            Log("Received ICMPv4 Echo Request, sending reply");
            uint16_t payloadLen = message->GetLength() - sizeof(Ip4::Header) - sizeof(Ip4::Icmp::Header);

            OwnedPtr<Message> replyMessage;
            replyMessage.Reset(br1->Get<MessagePool>().Allocate(Message::kTypeIp4));
            VerifyOrQuit(replyMessage != nullptr);

            Ip4::Header ip4Header;
            ip4Header.Clear();
            ip4Header.SetVersionIhl(0x45);
            ip4Header.SetTotalLength(sizeof(Ip4::Header) + sizeof(Ip4::Icmp::Header) + payloadLen);
            ip4Header.SetProtocol(Ip4::kProtoIcmp);
            ip4Header.SetTtl(64);
            ip4Header.SetSource(ip4Headers.GetDestinationAddress());
            ip4Header.SetDestination(ip4Headers.GetSourceAddress());
            SuccessOrQuit(replyMessage->Append(ip4Header));

            Ip4::Icmp::Header replyIcmpHeader;
            replyIcmpHeader.Clear();
            replyIcmpHeader.SetType(Ip4::Icmp::Header::kTypeEchoReply);
            replyIcmpHeader.SetRestOfHeader(icmpHeader.GetRestOfHeader());
            SuccessOrQuit(replyMessage->Append(replyIcmpHeader));

            // Append the rest of the ICMP request (Id, Sequence, and Payload)
            SuccessOrQuit(replyMessage->AppendBytesFromMessage(
                *message, sizeof(Ip4::Header) + sizeof(Ip4::Icmp::Header), payloadLen));

            replyMessage->SetOffset(sizeof(Ip4::Header));
            Checksum::UpdateMessageChecksum(*replyMessage, ip4Header.GetSource(), ip4Header.GetDestination(),
                                            ip4Header.GetProtocol());
            replyMessage->SetOffset(0);

            Checksum::UpdateIp4HeaderChecksum(ip4Header);
            replyMessage->Write(0, ip4Header);

            SuccessOrQuit(br1->Get<Nat64::Translator>().SendMessage(replyMessage.PassOwnership()));
        }
    }
}

void Test_1_4_PIC_TC_4(void)
{
    /**
     * 10.4 IPv4 Connectivity using BR built-in NAT64
     * 10.4.1. Purpose
     *   - To verify that BR DUT:
     *     - Offers IPv4 internet connectivity to/from Thread Devices using its built-in NAT64 translator, when the
     *       infrastructure network does not provide a NAT64 translator.
     *     - Offers IPv4 local network connectivity to/from Thread Devices using its built-in NAT64 translator
     *     - Operates DNS recursive resolver to look up IPv4 server addresses
     *     - Withdraws its own NAT64 prefix when another Border Router advertises a preferred NAT64 prefix. TBD - to
     *       be added.
     *   - Note: the present test requires a /96 NAT64 prefix, which is stricter than what the specification requires.
     *     RFC 6052 (Section 2.2) allows other lengths as well; however the present test case is not yet written to
     *     support all these different options.
     * 10.4.2. Topology
     *   - BR_1 (DUT) - Border Router
     *   - Router_1 - Thread Router Reference Device
     *   - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role
     *   - Eth_1 - Adjacent Infrastructure Link Linux Reference Device on AIL
     *     - Has HTTP web server with resource ‘/test2.html’.
     *   - Eth_2 - Adjacent Infrastructure Link SPIFF Reference Device on AIL
     *     - Has HTTP web server on simulated-internet with resource ‘/testv4.html’
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &r1   = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth2 = nexus.CreateNode();

    br1.SetName("BR_1");
    r1.SetName("ROUTER_1");
    ed1.SetName("ED_1");
    eth2.SetName("ETH_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Enable Eth_1 and Eth_2");
    /**
     * Step 1
     *   - Device: Eth_1, Eth_2
     *   - Description (PIC-10.4): Enable Harness configures Eth_1 with IPv4 address 192.168.1.4.
     *   - Pass Criteria: N/A
     */
    eth2.mInfraIf.SetIsDnsServer(true);
    eth2.mInfraIf.SetUdpHook(HandleEth2Udp);

    Ip6::Prefix eth2Prefix;
    SuccessOrQuit(eth2Prefix.FromString(kEth2Prefix));
    eth2.mInfraIf.StartRouterAdvertisement(eth2Prefix);

    Log("Step 2: Eth_2 default configuration");
    /**
     * Step 2
     *   - Device: Eth_2
     *   - Description (PIC-10.4): Harness instructs device to: Enable DHCP IPv4 server.
     *   - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kPingTime);

    Log("Step 3: Enable BR_1, Router_1, ED_1");
    /**
     * Step 3
     *   - Device: BR_1 (DUT), Router_1, ED_1
     *   - Description (PIC-10.4): Enable
     *   - Pass Criteria: N/A
     */
    br1.AllowList(r1);
    r1.AllowList(br1);
    r1.AllowList(ed1);
    ed1.AllowList(r1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Dns::ServiceDiscovery::Server>().Start());
    br1.Get<Dns::ServiceDiscovery::Server>().SetUpstreamQueryEnabled(true);

    r1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    ed1.Join(r1);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("Step 4: BR_1 configures IPv4 address and NAT64 prefix");
    /**
     * Step 4
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.4): Automatically configures an IPv4 address on the AIL; and identifies the IPv4 gateway
     *     on Eth_2. Automatically configures a NAT64 prefix in the Thread Network Data. Note: the check for length 96
     *     is not due to it being required by, but because the current test script is only designed to handle this
     *     case. Other cases are TBD. See note in Section 10.4.1 above.
     *   - Pass Criteria:
     *     - The DUT MUST add one NAT64 prefix in the Thread Network Data, which is a route entry published as follows:
     *       - Prefix TLV
     *         - Prefix Length field MUST be 96.
     */
    Ip4::Cidr nat64Cidr;
    SuccessOrQuit(nat64Cidr.FromString(kNat64Cidr));

    br1.Get<Ip6::Ip6>().SetNat64ReceiveIp4Callback(HandleIp4Receive, &br1);
    SuccessOrQuit(br1.Get<Nat64::Translator>().SetIp4Cidr(nat64Cidr));
    br1.Get<BorderRouter::RoutingManager>().SetNat64PrefixManagerEnabled(true);

    nexus.AdvanceTime(kBrActionTime);

    Ip6::Prefix nat64Prefix;
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().GetNat64Prefix(nat64Prefix));

    br1.Get<Nat64::Translator>().SetNat64Prefix(nat64Prefix);
    br1.Get<Nat64::Translator>().SetEnabled(true);

    Log("Step 5: ED_1 performs DNS query for threadv4.org");
    /**
     * Step 5
     *   - Device: ED_1
     *   - Description (PIC-10.4): Harness instructs device to perform DNS query QType=ANY, name “threadv4.org”.
     *     Automatically, the DNS query gets routed to the DUT.
     *   - Pass Criteria: N/A
     */
    Ip6::Address dnsServerAddr;
    dnsServerAddr = br1.Get<Mle::Mle>().GetMeshLocalEid();

    Ip6::Udp::Socket dnsSocket(ed1, nullptr, nullptr);
    SuccessOrQuit(dnsSocket.Open(Ip6::kNetifThreadInternal));

    Message *dnsQuery = dnsSocket.NewMessage();
    VerifyOrQuit(dnsQuery != nullptr);

    Dns::Header dnsHeader;
    dnsHeader.SetType(Dns::Header::kTypeQuery);
    dnsHeader.SetQuestionCount(1);
    dnsHeader.SetRecursionDesiredFlag();
    SuccessOrQuit(dnsQuery->Append(dnsHeader));
    SuccessOrQuit(Dns::Name::AppendName("threadv4.org", *dnsQuery));
    Dns::Question dnsQuestion(Dns::ResourceRecord::kTypeAny);
    SuccessOrQuit(dnsQuery->Append(dnsQuestion));

    Ip6::MessageInfo dnsMessageInfo;
    dnsMessageInfo.SetPeerAddr(dnsServerAddr);
    dnsMessageInfo.SetPeerPort(kDnsPort);
    SuccessOrQuit(dnsSocket.SendTo(*dnsQuery, dnsMessageInfo));
    SuccessOrQuit(dnsSocket.Close());

    Log("Step 6: BR_1 processes DNS query upstream");
    /**
     * Step 6
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.4): Automatically processes the DNS query by requesting upstream to the Eth_2 DNS server.
     *     Then, it responds back with the answer to ED_1.
     *   - Pass Criteria: N/A
     */
    nexus.AdvanceTime(kDnsTime);

    Log("Step 7: ED_1 receives DNS result");
    /**
     * Step 7
     *   - Device: ED_1
     *   - Description (PIC-10.4): Successfully receives DNS query result.
     *   - Pass Criteria:
     *     - ED_1 MUST receive DNS query answer from DUT:
     *       - threadv4.org A 38.106.217.165
     */

    Log("Step 8: ED_1 pings internet server");
    /**
     * Step 8
     *   - Device: ED_1
     *   - Description (PIC-10.4): Harness instructs device to send ICMPv6 ping request to internet server, using
     *     synthesized IPv6 address: <NAT64-prefix>:266a:d9a5 Request is routed via the DUT to the Eth_2 simulated
     *     network. The DUT translates the above IPv6 address into the IPv4 destination address 38.106.217.165.
     *   - Pass Criteria:
     *     - ED_1 MUST receive ICMPv6 ping response from simulated server.
     */
    Ip4::Address internetServerIp4;
    SuccessOrQuit(internetServerIp4.FromString(kInternetServerIp4Addr));
    Ip6::Address internetServerIp6;
    internetServerIp6.SynthesizeFromIp4Address(nat64Prefix, internetServerIp4);

    ed1.SendEchoRequest(internetServerIp6, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 8a: ED_1 sends UDP ping to internet server");
    /**
     * Step 8a
     *   - Device: ED_1
     *   - Description (PIC-10.4): Repeats same using a UDP ping.
     *   - Pass Criteria:
     *     - ED_1 MUST receive UDP ping response from simulated server.
     */
    {
        Ip6::Udp::Socket udpSocket(ed1, nullptr, nullptr);
        SuccessOrQuit(udpSocket.Open(Ip6::kNetifThreadInternal));
        Message *udpMsg = udpSocket.NewMessage();
        VerifyOrQuit(udpMsg != nullptr);
        SuccessOrQuit(udpMsg->SetLength(kEchoPayloadSize));
        Ip6::MessageInfo udpInfo;
        udpInfo.SetPeerAddr(internetServerIp6);
        udpInfo.SetPeerPort(12345);
        SuccessOrQuit(udpSocket.SendTo(*udpMsg, udpInfo));
        SuccessOrQuit(udpSocket.Close());
    }
    nexus.AdvanceTime(kPingTime);

    Log("Step 9: ED_1 sends HTTP GET to internet server");
    /**
     * Step 9
     *   - Device: ED_1
     *   - Description (PIC-10.4): Harness instructs device to send HTTP GET request to server. If ED_1 is a Linux
     *     device, it may use 'curl' to save the file: curl -o testv4.html http://threadv4.org/testv4.html In case ED_1
     *     is a non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init
     *     tcp connect <NAT64-prefix>:266a:d9a5 80 tcp send -x 474554202F74... tcp sendend tcp deinit
     *   - Pass Criteria:
     *     - In case Linux 'curl' was used: File testv4.html MUST equal the testv4.html file as stored on the simulated
     *       server on Eth_2.
     *     - In case OT CLI was used: Output on the CLI MUST be the HTML file testv4.html as stored on the simulator
     *       server on Eth_2.
     */
    {
        Ip6::Tcp::Endpoint          tcpEndpoint;
        otTcpEndpointInitializeArgs args;
        ClearAllBytes(args);
        SuccessOrQuit(tcpEndpoint.Initialize(ed1, args));
        Ip6::SockAddr internetSockAddr;
        internetSockAddr.SetAddress(internetServerIp6);
        internetSockAddr.SetPort(kHttpPort);
        SuccessOrQuit(tcpEndpoint.Connect(internetSockAddr, 0));
        nexus.AdvanceTime(kTcpTime);

        const char     httpGet[] = "GET /testv4.html HTTP/1.1\r\nHost: threadv4.org\r\n\r\n";
        otLinkedBuffer linkedBuffer;
        ClearAllBytes(linkedBuffer);
        linkedBuffer.mData   = reinterpret_cast<const uint8_t *>(httpGet);
        linkedBuffer.mLength = sizeof(httpGet) - 1;
        SuccessOrQuit(tcpEndpoint.SendByReference(linkedBuffer, 0));
        nexus.AdvanceTime(kTcpTime);
        SuccessOrQuit(tcpEndpoint.Deinitialize());
    }

    Log("Step 10: ED_1 pings local server Eth_1");
    /**
     * Step 10
     *   - Device: ED_1
     *   - Description (PIC-10.4): Harness instructs device to send ping request to local IPv4 server Eth_1 which has
     *     IPv4 destination address 192.168.1.4; using synthesized IPv6 address: <NAT64-prefix>:c0a8:0104 Automatically,
     *     it is routed via BR_1 DUT to Eth_1.
     *   - Pass Criteria:
     *     - ED_1 MUST receive ping response from Eth_1.
     */
    Ip4::Address localServerIp4;
    SuccessOrQuit(localServerIp4.FromString(kLocalServerIp4Addr));
    Ip6::Address localServerIp6;
    localServerIp6.SynthesizeFromIp4Address(nat64Prefix, localServerIp4);

    ed1.SendEchoRequest(localServerIp6, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 11: ED_1 sends HTTP GET to local server Eth_1");
    /**
     * Step 11
     *   - Device: ED_1
     *   - Description (PIC-10.4): Harness instructs device to send HTTP GET request to local server Eth_1. In case ED_1
     *     is a Linux device, it can use 'curl' to save the file locally: curl -o test2.html http:///test2.html In case
     *     ED_1 is a non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init
     *     tcp connect <NAT64-prefix>:c0a8:0104 80 tcp send -x 474554202F... tcp sendend tcp deinit
     *   - Pass Criteria:
     *     - In case Linux 'curl' was used: File test2.html MUST equal the test2.html file as stored on the Eth_1
     *       server.
     *     - In case OT CLI was used: Output on the CLI MUST be the HTML file test2.html as stored on the server on
     *       Eth_1.
     */
    {
        Ip6::Tcp::Endpoint          tcpEndpoint;
        otTcpEndpointInitializeArgs args;
        ClearAllBytes(args);
        SuccessOrQuit(tcpEndpoint.Initialize(ed1, args));
        Ip6::SockAddr localSockAddr;
        localSockAddr.SetAddress(localServerIp6);
        localSockAddr.SetPort(kHttpPort);
        SuccessOrQuit(tcpEndpoint.Connect(localSockAddr, 0));
        nexus.AdvanceTime(kTcpTime);

        const char     httpGet[] = "GET /test2.html HTTP/1.1\r\nHost: localv4.org\r\n\r\n";
        otLinkedBuffer linkedBuffer;
        ClearAllBytes(linkedBuffer);
        linkedBuffer.mData   = reinterpret_cast<const uint8_t *>(httpGet);
        linkedBuffer.mLength = sizeof(httpGet) - 1;
        SuccessOrQuit(tcpEndpoint.SendByReference(linkedBuffer, 0));
        nexus.AdvanceTime(kTcpTime);
        SuccessOrQuit(tcpEndpoint.Deinitialize());
    }

    nexus.AddTestVar("ETH_2_ADDR", eth2.mInfraIf.GetLinkLocalAddress().ToString().AsCString());
    nexus.AddTestVar("INTERNET_SERVER_ADDR", internetServerIp6.ToString().AsCString());
    nexus.AddTestVar("LOCAL_SERVER_ADDR", localServerIp6.ToString().AsCString());
    nexus.AddTestVar("NAT64_PREFIX", nat64Prefix.ToString().AsCString());

    nexus.SaveTestInfo("test_1_4_PIC_TC_4.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_4_PIC_TC_4();
    printf("All tests passed\n");
    return 0;
}
