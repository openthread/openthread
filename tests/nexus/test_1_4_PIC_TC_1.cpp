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

#include "common/offset_range.hpp"
#include "net/dhcp6_types.hpp"
#include "net/dns_types.hpp"
#include "net/tcp6.hpp"
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
static constexpr uint32_t kJoinNetworkTime = 200 * 1000;

/**
 * Time to advance for the BR to perform automatic actions (DHCPv6-PD, RA), in milliseconds.
 */
static constexpr uint32_t kBrActionTime = 30 * 1000;

/**
 * Time to advance for the DNS query and response, in milliseconds.
 */
static constexpr uint32_t kDnsTime = 5 * 1000;

/**
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingTime = 2 * 1000;

/**
 * Time to advance for the TCP connection and data transfer, in milliseconds.
 */
static constexpr uint32_t kTcpTime = 10 * 1000;

/**
 * DHCPv6-PD Server Port.
 */
static constexpr uint16_t kDhcp6ServerPort = 547;

/**
 * DHCPv6-PD Client Port.
 */
static constexpr uint16_t kDhcp6ClientPort = 546;

/**
 * DNS Server Port.
 */
static constexpr uint16_t kDnsPort = 53;

/**
 * HTTP Server Port.
 */
static constexpr uint16_t kHttpPort = 80;

/**
 * Echo Request identifier.
 */
static constexpr uint16_t kEchoIdentifier = 0x1234;

/**
 * Echo Request payload size.
 */
static constexpr uint16_t kEchoPayloadSize = 10;

/**
 * Internet Server Address.
 */
static const char kInternetServerAddr[] = "2002:1234::1234";

/**
 * Delegated Prefix.
 */
static const char kDelegatedPrefix[] = "2005:1234:abcd:1::/64";

/**
 * Eth_1 GUA address prefix.
 */
static const char kEth1Prefix[] = "2001:db8:1::/64";

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * DHCPv6 Server Hook to simulate DHCPv6-PD server and DNS server.
 */
bool HandleEth2Udp(Instance &aInstance, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Node &eth2    = Node::From(&aInstance);
    bool  handled = false;

    if (aMessageInfo.GetSockPort() == kDhcp6ServerPort)
    {
        Dhcp6::Header header;
        SuccessOrQuit(aMessage.Read(aMessage.GetOffset(), header));

        if (header.GetMsgType() == Dhcp6::kMsgTypeSolicit || header.GetMsgType() == Dhcp6::kMsgTypeRequest)
        {
            Message *reply = eth2.Get<MessagePool>().Allocate(Message::kTypeIp6);
            VerifyOrQuit(reply != nullptr);

            Dhcp6::Header replyHeader;
            replyHeader.Clear();
            replyHeader.SetMsgType((header.GetMsgType() == Dhcp6::kMsgTypeSolicit) ? Dhcp6::kMsgTypeAdvertise
                                                                                   : Dhcp6::kMsgTypeReply);
            replyHeader.SetTransactionId(header.GetTransactionId());
            SuccessOrQuit(reply->Append(replyHeader));

            OffsetRange   offsetRange;
            Dhcp6::Option option;
            uint32_t      iaid = 0;

            offsetRange.InitFromRange(aMessage.GetOffset() + sizeof(Dhcp6::Header), aMessage.GetLength());

            while (offsetRange.Contains(sizeof(Dhcp6::Option)))
            {
                SuccessOrQuit(aMessage.Read(offsetRange.GetOffset(), option));
                VerifyOrQuit(offsetRange.Contains(option.GetSize()));

                if (option.GetCode() == Dhcp6::Option::kClientId)
                {
                    SuccessOrQuit(reply->AppendBytesFromMessage(aMessage, offsetRange.GetOffset(), option.GetSize()));
                }
                else if (option.GetCode() == Dhcp6::Option::kIaPd)
                {
                    Dhcp6::IaPdOption iaPdSolicit;

                    VerifyOrQuit(option.GetSize() >= sizeof(iaPdSolicit));
                    SuccessOrQuit(aMessage.Read(offsetRange.GetOffset(), iaPdSolicit));
                    iaid = iaPdSolicit.GetIaid();
                }
                offsetRange.AdvanceOffset(option.GetSize());
            }

            // Append Server ID (ETH_2 address)
            SuccessOrQuit(Dhcp6::ServerIdOption::AppendWithEui64Duid(*reply, eth2.Get<Mac::Mac>().GetExtAddress()));

            if (header.GetMsgType() == Dhcp6::kMsgTypeSolicit)
            {
                // Append Preference
                Dhcp6::PreferenceOption preference;
                preference.Init();
                preference.SetPreference(255);
                SuccessOrQuit(reply->Append(preference));
            }

            Dhcp6::IaPdOption iaPd;
            iaPd.Init();
            iaPd.SetIaid(iaid);
            iaPd.SetT1(1000);
            iaPd.SetT2(2000);
            SuccessOrQuit(reply->Append(iaPd));

            Dhcp6::IaPrefixOption prefixOption;
            prefixOption.Init();
            Ip6::Prefix prefix;
            SuccessOrQuit(prefix.FromString(kDelegatedPrefix));
            prefixOption.SetPrefix(prefix);
            prefixOption.SetPreferredLifetime(3600);
            prefixOption.SetValidLifetime(7200);
            SuccessOrQuit(reply->Append(prefixOption));

            Dhcp6::Option::UpdateOptionLengthInMessage(*reply,
                                                       reply->GetLength() - sizeof(iaPd) - sizeof(prefixOption));

            eth2.mInfraIf.SendUdp(eth2.mInfraIf.GetLinkLocalAddress(), aMessageInfo.GetPeerAddr(), kDhcp6ServerPort,
                                  kDhcp6ClientPort, *reply);
            handled = true;
        }
    }
    else if (aMessageInfo.GetSockPort() == kDnsPort)
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

            SuccessOrQuit(Dns::Name::AppendName("threadgroup.org", *reply));
            Dns::ResourceRecord rr;
            rr.SetType(Dns::ResourceRecord::kTypeAaaa);
            rr.SetClass(Dns::ResourceRecord::kClassInternet);
            rr.SetTtl(3600);
            rr.SetLength(sizeof(Ip6::Address));
            SuccessOrQuit(reply->Append(rr));
            Ip6::Address addr;
            SuccessOrQuit(addr.FromString(kInternetServerAddr));
            SuccessOrQuit(reply->Append(addr));

            eth2.mInfraIf.SendUdp(eth2.mInfraIf.GetLinkLocalAddress(), aMessageInfo.GetPeerAddr(), kDnsPort,
                                  aMessageInfo.GetPeerPort(), *reply);
            handled = true;
        }
    }

    return handled;
}

void Test_1_4_PIC_TC_1(void)
{
    /**
     * 10.1. IPv6 Connectivity using DHCPv6-PD delegated OMR prefix
     *
     * 10.1.1. Purpose
     *   - To verify that BR DUT:
     *     - uses DHCPv6-PD client functionality to obtain an OMR prefix for its Thread Network from the upstream CPE
     *       router
     *     - Offers IPv6 public internet connectivity to/from Thread Devices that configured an OMR address
     *     - Offers IPv6 local network connectivity to/from Thread Devices that configured an OMR address
     *     - Operates DNS recursive resolver to look up IPv6 server addresses on public internet
     *
     * 10.1.2. Topology
     *   - BR_1 (DUT) - Border Router
     *   - Router_1 - Thread Router Reference Device, attached to BR_1
     *   - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role, attached to Router_1
     *   - Eth_1 - Adjacent Infrastructure Link Linux Reference Device
     *     - Has HTTP web server with resource ‘/test2.html’.
     *   - Eth_2 - Adjacent Infrastructure Link SPIFF Reference Device
     */

    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &r1   = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();
    Node &eth2 = nexus.CreateNode();

    br1.SetName("BR_1");
    r1.SetName("ROUTER_1");
    ed1.SetName("ED_1");
    eth1.SetName("ETH_1");
    eth2.SetName("ETH_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Enable Eth_1 and Eth_2");
    /**
     * Step 1
     *   - Device: Eth_1, Eth_2
     *   - Description (PIC-10.1): Enable
     *   - Pass Criteria:
     *     - N/A
     */
    eth2.mInfraIf.SetIsDnsServer(true);

    {
        Ip6::Address internetServer;
        SuccessOrQuit(internetServer.FromString(kInternetServerAddr));
        eth2.mInfraIf.AddAddress(internetServer);
    }

    Ip6::Prefix eth1Prefix;
    SuccessOrQuit(eth1Prefix.FromString(kEth1Prefix));
    eth2.mInfraIf.StartRouterAdvertisement(eth1Prefix);
    eth2.mInfraIf.SetUdpHook(HandleEth2Udp);

    Log("Step 2: Eth_2 default configuration");
    /**
     * Step 2
     *   - Device: Eth_2 (SPIFF)
     *   - Description (PIC-10.1): Harness does not configure the device, other than the default configuration. Note:
     *     in previous versions of the test plan, explicit configuration was provided in this step. Note: Eth_1 will
     *     automatically configure a GUA using SLAAC and the RA-advertised prefix.
     *   - Pass Criteria:
     *     - N/A
     */
    nexus.AdvanceTime(kPingTime);

    Log("Step 3: Enable BR_1, Router_1, ED_1");
    /**
     * Step 3
     *   - Device: BR_1 (DUT), Router_1, ED_1
     *   - Description (PIC-10.1): Enable
     *   - Pass Criteria:
     *     - N/A
     */

    AllowLinkBetween(br1, r1);
    AllowLinkBetween(r1, ed1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    br1.Get<BorderRouter::RoutingManager>().SetDhcp6PdEnabled(true);
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Dns::ServiceDiscovery::Server>().Start());
    br1.Get<Dns::ServiceDiscovery::Server>().SetUpstreamQueryEnabled(true);

    r1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    ed1.Join(r1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("Step 4: BR_1 obtains OMR prefix via DHCPv6-PD");
    /**
     * Step 4
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.1): Automatically uses DHCPv6-PD client function to obtain a delegated prefix from the
     *     DHCPv6 server. It configures this prefix as the OMR prefix.
     *   - Pass Criteria:
     *     - An OMR prefix OMR_1 MUST appear in Thread Network Data in a Prefix TLV:
     *     - It MUST be subprefix of 2005:1234:abcd:0::/56.
     *     - It MUST be a /64 prefix
     *     - Border Router sub-TLV:
     *       - Prf bits = P_preference = '00' (medium)
     *       - R = P_default = '1'
     */
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 4b: BR_1 advertises route to OMR prefix on AIL");
    /**
     * Step 4b
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.1): Automatically advertises the route to its OMR prefix on the AIL, as a stub router
     *     (aka IETF SNAC router). Note: see 9.6.2 for Stub Router / SNAC Router flag detail.The flag is bit 6 of RA
     *     flags as specified by the TEMPORARY allocation made by IANA (link).
     *   - Pass Criteria:
     *     - The DUT MUST advertise the route in multicast ND RA messages:
     *     - IPv6 destination MUST be ff02::1
     *     - SNAC Router flag set to '1'
     *     - Route Information Option (RIO)
     *       - Prefix: OMR_1
     *       - Prf bits: '00' or '11'
     *       - Route Lifetime > 0
     */

    Log("Step 5: ED_1 performs DNS query for threadgroup.org");
    /**
     * Step 5
     *   - Device: ED_1
     *   - Description (PIC-10.1): Harness instructs device to perform DNS query QType=AAAA, name “threadgroup.org”.
     *     Automatically, the DNS query gets routed to BR_1.
     *   - Pass Criteria:
     *     - N/A
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
    SuccessOrQuit(Dns::Name::AppendName("threadgroup.org", *dnsQuery));
    Dns::Question dnsQuestion(Dns::ResourceRecord::kTypeAaaa);
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
     *   - Description (PIC-10.1): Automatically processes the DNS query by requesting upstream to the Eth_2 DNS
     *     server. Then, it responds back with the answer to ED_1.
     *   - Pass Criteria:
     *     - N/A
     */
    nexus.AdvanceTime(kDnsTime);

    Log("Step 7: ED_1 receives DNS result");
    /**
     * Step 7
     *   - Device: ED_1
     *   - Description (PIC-10.1): Successfully receives DNS query result: threadgroup.org AAAA 2002:1234::1234
     *   - Pass Criteria:
     *     - ED_1 MUST receive DNS query answer from DUT.
     */

    Log("Step 8: ED_1 pings internet server");
    /**
     * Step 8
     *   - Device: ED_1
     *   - Description (PIC-10.1): Harness instructs device to send ICMpv6 ping request to internet server, using
     *     resolved address above. It is routed via the DUT to the Eth_2 simulated network.
     *   - Pass Criteria:
     *     - ED_1 MUST receive ICMPv6 ping response from simulated server.
     */
    Ip6::Address internetServer;
    SuccessOrQuit(internetServer.FromString(kInternetServerAddr));
    ed1.SendEchoRequest(internetServer, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 8a: ED_1 sends UDP ping to internet server");
    /**
     * Step 8a
     *   - Device: ED_1
     *   - Description (PIC-10.1): Repeats same using a UDP ping.
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
        udpInfo.SetPeerAddr(internetServer);
        udpInfo.SetPeerPort(12345);
        SuccessOrQuit(udpSocket.SendTo(*udpMsg, udpInfo));
        SuccessOrQuit(udpSocket.Close());
    }
    nexus.AdvanceTime(kPingTime);

    Log("Step 9: ED_1 sends HTTP GET to internet server");
    /**
     * Step 9
     *   - Device: ED_1
     *   - Description (PIC-10.1): Harness instructs device to sends HTTP GET request to server. If ED_1 is a Linux
     *     device, it may use 'curl' to save the file: curl -o test.html http://threadgroup.org/test.html. In case ED_1
     *     is a non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init, tcp
     *     connect 2002:1234::1234 80, tcp send -x
     *     474554202F746573742E68746D6C20485454502F312E310D0A486F73743A2074687265616467726F75702E6F72670D0A0D0A, tcp
     *     sendend, tcp deinit
     *   - Pass Criteria:
     *     - In case Linux 'curl' was used: File test.html MUST equal the test.html file as stored on the simulated
     *       server on Eth_2.
     *     - In case OT CLI was used: Output on the CLI MUST be the HTML file test.html as stored on the simulator
     *       server on Eth_2.
     */
    {
        Ip6::Tcp::Endpoint          tcpEndpoint;
        otTcpEndpointInitializeArgs args;
        ClearAllBytes(args);
        SuccessOrQuit(tcpEndpoint.Initialize(ed1, args));
        Ip6::SockAddr internetSockAddr;
        internetSockAddr.SetAddress(internetServer);
        internetSockAddr.SetPort(kHttpPort);
        SuccessOrQuit(tcpEndpoint.Connect(internetSockAddr, 0));
        nexus.AdvanceTime(kTcpTime);

        const char     httpGet[] = "GET /test.html HTTP/1.1\r\nHost: threadgroup.org\r\n\r\n";
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
     *   - Description (PIC-10.1): Harness instructs device to send ping request to local server Eth_1. Automatically,
     *     it is routed via the DUT.
     *   - Pass Criteria:
     *     - ED_1 MUST receive ping response from Eth_1.
     */
    Ip6::Address eth1Addr = eth1.mInfraIf.FindMatchingAddress(kEth1Prefix);
    ed1.SendEchoRequest(eth1Addr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 11: ED_1 sends HTTP GET to local server Eth_1");
    /**
     * Step 11
     *   - Device: ED_1
     *   - Description (PIC-10.1): Harness instructs device to send HTTP GET request to local server Eth_1. If ED_1 is a
     *     Linux device, it may use 'curl' to save the file: curl -o test2.html http:///test2.html. In case ED_1 is a
     *     non-Linux Thread CLI device, it can use the OT CLI commands to send the HTTP GET request: tcp init, tcp
     *     connect <Eth_1_GUA_IPv6_addr> 80, tcp send -x
     *     474554202F74657374322E68746D6C20485454502F312E310D0A486F73743A206C6F63616C746573742E6F72670D0A0D0A, tcp
     *     sendend, tcp deinit
     *   - Pass Criteria:
     *     - In case Linux 'curl' was used: File test2.html MUST equal the test2.html file as stored on the Eth_1
     *       server.
     *     - In case OT CLI was used: Output on the CLI MUST be the HTML file test.html as stored on the simulator
     *       server on Eth_2.
     */
    {
        Ip6::Tcp::Endpoint          tcpEndpoint;
        otTcpEndpointInitializeArgs args;
        ClearAllBytes(args);
        SuccessOrQuit(tcpEndpoint.Initialize(ed1, args));
        Ip6::SockAddr eth1SockAddr;
        eth1SockAddr.SetAddress(eth1Addr);
        eth1SockAddr.SetPort(kHttpPort);
        SuccessOrQuit(tcpEndpoint.Connect(eth1SockAddr, 0));
        nexus.AdvanceTime(kTcpTime);

        const char     httpGet[] = "GET /test2.html HTTP/1.1\r\nHost: localtest.org\r\n\r\n";
        otLinkedBuffer linkedBuffer;
        ClearAllBytes(linkedBuffer);
        linkedBuffer.mData   = reinterpret_cast<const uint8_t *>(httpGet);
        linkedBuffer.mLength = sizeof(httpGet) - 1;
        SuccessOrQuit(tcpEndpoint.SendByReference(linkedBuffer, 0));
        nexus.AdvanceTime(kTcpTime);
        SuccessOrQuit(tcpEndpoint.Deinitialize());
    }

    nexus.AddTestVar("ETH_1_ADDR", eth1Addr.ToString().AsCString());
    nexus.AddTestVar("ETH_2_ADDR", eth2.mInfraIf.GetLinkLocalAddress().ToString().AsCString());
    nexus.AddTestVar("INTERNET_SERVER_ADDR", kInternetServerAddr);
    nexus.AddTestVar("DELEGATED_PREFIX", kDelegatedPrefix);

    nexus.SaveTestInfo("test_1_4_PIC_TC_1.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_4_PIC_TC_1();
    printf("All tests passed\n");
    return 0;
}
