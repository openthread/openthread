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

static constexpr char kGua1Prefix[] = "2005:1234:abcd:100::/64";

static constexpr char kEth1Addr[] = "2005:1234:abcd:100::E1";
static constexpr char kEth2Addr[] = "2005:1234:abcd:100::E2";

static constexpr char kThreadGroup1[] = "threadgroup1.org";
static constexpr char kThreadGroup2[] = "threadgroup2.org";
static constexpr char kThreadGroup3[] = "threadgroup3.org";

static constexpr char kEth1Answer1[] = "2002:1234::E1:1";
static constexpr char kEth1Answer2[] = "2002:1234::E1:2";
static constexpr char kEth1Answer3[] = "2002:1234::E1:3";

static constexpr char kEth2Answer1[] = "2002:1234::E2:1";
static constexpr char kEth2Answer2[] = "2002:1234::E2:2";
static constexpr char kEth2Answer3[] = "2002:1234::E2:3";

struct Record
{
    const char *mName;
    const char *mAddress;
};

const Record kEth1Records[] = {
    {kThreadGroup1, kEth1Answer1},
    {kThreadGroup2, kEth1Answer2},
    {kThreadGroup3, kEth1Answer3},
};

const Record kEth2Records[] = {
    {kThreadGroup1, kEth2Answer1},
    {kThreadGroup2, kEth2Answer2},
    {kThreadGroup3, kEth2Answer3},
};

static Node *sEth1Node = nullptr;
static Node *sEth2Node = nullptr;

bool HandleUdpHook(Instance &aInstance, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Node         &node    = AsNode(&aInstance);
    const Record *records = nullptr;
    uint16_t      count   = 0;
    Dns::Header   dnsHeader;
    Dns::Question question;
    char          name[Dns::Name::kMaxNameSize];
    uint16_t      readPos;
    Message      *response;
    bool          handled = false;

    if (&node == sEth1Node)
    {
        records = kEth1Records;
        count   = GetArrayLength(kEth1Records);
    }
    else if (&node == sEth2Node)
    {
        records = kEth2Records;
        count   = GetArrayLength(kEth2Records);
    }

    VerifyOrExit(records != nullptr);
    VerifyOrExit(aMessageInfo.GetSockPort() == UpstreamDns::kDnsPort);

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), dnsHeader));
    VerifyOrExit(dnsHeader.GetType() == Dns::Header::kTypeQuery);
    VerifyOrExit(dnsHeader.GetQuestionCount() == 1);

    readPos = aMessage.GetOffset() + sizeof(dnsHeader);
    SuccessOrExit(Dns::Name::ReadName(aMessage, readPos, name));
    SuccessOrExit(aMessage.Read(readPos, question));

    Log("Node %u (%s) received DNS query for %s", node.GetInstance().GetId(), node.GetName(), name);

    response = node.Get<MessagePool>().Allocate(Message::kTypeIp6);
    VerifyOrExit(response != nullptr);

    dnsHeader.SetType(Dns::Header::kTypeResponse);
    dnsHeader.SetAnswerCount(0);
    dnsHeader.SetRecursionDesiredFlag();
    dnsHeader.SetRecursionAvailableFlag();

    for (uint16_t i = 0; i < count; i++)
    {
        if (StringMatch(name, records[i].mName, kStringCaseInsensitiveMatch))
        {
            Ip6::Address        address;
            Dns::ResourceRecord rr;

            SuccessOrQuit(address.FromString(records[i].mAddress));

            dnsHeader.SetAnswerCount(1);
            SuccessOrQuit(response->Append(dnsHeader));
            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            SuccessOrQuit(response->Append(question));

            SuccessOrQuit(Dns::Name::AppendName(name, *response));
            rr.Init(Dns::ResourceRecord::kTypeAaaa);
            rr.SetTtl(1800);
            rr.SetLength(sizeof(Ip6::Address));
            SuccessOrQuit(response->Append(rr));
            SuccessOrQuit(response->Append(address));

            Log("Node %u (%s) sending DNS response for %s with address %s", node.GetInstance().GetId(), node.GetName(),
                name, records[i].mAddress);
            break;
        }
    }

    if (dnsHeader.GetAnswerCount() == 0)
    {
        dnsHeader.SetResponseCode(Dns::Header::kResponseNameError);
        SuccessOrQuit(response->Append(dnsHeader));
        SuccessOrQuit(Dns::Name::AppendName(name, *response));
        SuccessOrQuit(response->Append(question));
        Log("Node %u (%s) sending DNS NXDOMAIN for %s", node.GetInstance().GetId(), node.GetName(), name);
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

static void SendRa(Node &aNode, const Ip6::Prefix &aPrefix, const Ip6::Address &aDnsAddr)
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

/**
 * 11.3. [1.4] [CERT] Selection of upstream DNS resolver
 *
 * 11.3.1. Purpose
 * To verify that the BR DUT:
 * - Selects the upstream DNS resolver based on ND RA RDNSS Option, sent by the infrastructure network, pointing to the
 *   default DNS resolver.
 * - Reacts to change of the upstream DNS resolver, e.g. due to a configuration change, new ISP, or other reasons.
 * - Can handle link-local and global IPv6 address pointing to upstream DNS resolver.
 *
 * 11.3.2. Topology
 * - BR_1 (DUT) - Border Router
 * - Router_1 - Thread Router Reference Device, attached to BR_1
 * - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role, attached to Router_1
 * - Eth_1 - Adjacent Infrastructure Link Linux Reference Device with DNS server
 * - Eth_2 - Adjacent Infrastructure Link Linux Reference Device with DNS server and RA daemon
 */
void Test_1_4_DNS_TC_3(const char *aJsonFileName)
{
    Core         nexus;
    Node        &br1     = nexus.CreateNode();
    Node        &router1 = nexus.CreateNode();
    Node        &ed1     = nexus.CreateNode();
    Node        &eth1    = nexus.CreateNode();
    Node        &eth2    = nexus.CreateNode();
    Ip6::Prefix  gua1Prefix;
    Ip6::Address eth1Addr;
    Ip6::Address eth2Addr;

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    br1.SetName("BR", 1);
    router1.SetName("Router", 1);
    ed1.SetName("ED", 1);
    eth1.SetName("Eth", 1);
    eth2.SetName("Eth", 2);

    sEth1Node = &eth1;
    sEth2Node = &eth2;

    eth1.mInfraIf.SetUdpHook(HandleUdpHook);
    eth2.mInfraIf.SetUdpHook(HandleUdpHook);

    AllowLinkBetween(br1, router1);
    AllowLinkBetween(router1, ed1);

    SuccessOrQuit(gua1Prefix.FromString(kGua1Prefix));
    SuccessOrQuit(eth1Addr.FromString(kEth1Addr));
    SuccessOrQuit(eth2Addr.FromString(kEth2Addr));

    /**
     * Step 1
     * - Device: Eth_1, Eth_2
     * - Description (DNS-11.3): Enable
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 1: Enable Eth_1, Eth_2");
    eth1.Get<ThreadNetif>().Up();
    eth2.Get<ThreadNetif>().Up();
    eth1.mInfraIf.AddAddress(eth1Addr);
    eth2.mInfraIf.AddAddress(eth2Addr);

    /**
     * Step 2
     * - Device: Eth_2
     * - Description (DNS-11.3): Harness instructs device to: Configure router
     *   advertisement daemon (radvd) to multicast ND RA with: Prefix Information
     *   Option (PIO) with a global on-link prefix GUA_1 (SLAAC is enabled (A=1),
     *   on-link (L=1)), Recursive DNS Server Option (25), with Addresses of IPv6
     *   Recursive DNS Servers field contains a single global IPv6 address of Eth_1.
     *   Configure DNS server with records: threadgroup1.org AAAA 2002:1234::E2:1,
     *   threadgroup2.org AAAA 2002:1234::E2:2, threadgroup3.org AAAA 2002:1234::E2:3.
     *   Note: prefix GUA_1 may be 2005:1234:abcd:100::/64.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 2: Eth_2 sends RA with global IPv6 address of Eth_1");
    SendRa(eth2, gua1Prefix, eth1Addr);

    /**
     * Step 3
     * - Device: Eth_1
     * - Description (DNS-11.3): Harness instructs device to: Configure DNS server
     *   with records: threadgroup1.org AAAA 2002:1234::E1:1, threadgroup2.org AAAA
     *   2002:1234::E1:2, threadgroup3.org AAAA 2002:1234::E1:3
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 3: Configure DNS records on Eth_1");
    // Handled by HandleUdpHook using kEth1Records.

    /**
     * Step 4
     * - Device: BR_1 (DUT), Router_1, ED_1
     * - Description (DNS-11.3): Enable
     * - Pass Criteria:
     *   - Single Thread Network forms.
     */
    Log("Step 4: Enable BR_1, Router_1, ED_1");
    br1.Form();
    nexus.AdvanceTime(10000);

    router1.Join(br1, Node::kAsFtd);
    ed1.Join(br1, Node::kAsFed);

    nexus.AdvanceTime(20000);

    VerifyOrQuit(br1.Get<Mle::Mle>().IsAttached());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsChild());

    /**
     * Step 5
     * - Device: BR_1 (DUT)
     * - Description (DNS-11.3): Automatically obtains / configures an OMR prefix for
     *   the Thread Network, and assigns an address for its AIL interface using SLAAC.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 5: BR_1 configures OMR prefix and AIL address");
    br1.Get<BorderRouter::InfraIf>().Init(1, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    SuccessOrQuit(br1.Get<Dns::ServiceDiscovery::Server>().Start());
    br1.Get<Dns::ServiceDiscovery::Server>().SetUpstreamQueryEnabled(true);

    {
        Dns::Client::QueryConfig dnsConfig;
        dnsConfig.Clear();
        AsCoreType(&dnsConfig.mServerSockAddr.mAddress) = br1.Get<Mle::Mle>().GetMeshLocalEid();
        dnsConfig.mServerSockAddr.mPort                 = UpstreamDns::kDnsPort;
        ed1.Get<Dns::Client>().SetDefaultConfig(dnsConfig);
    }

    // Resend RA to ensure BR_1 (which was just initialized) receives it.
    SendRa(eth2, gua1Prefix, eth1Addr);
    nexus.AdvanceTime(5000);

    /**
     * Step 6
     * - Device: ED_1
     * - Description (DNS-11.3): Harness instructs device to perform DNS query
     *   Qtype=AAAA, name threadgroup1.org. Automatically, the DNS query gets
     *   routed to the DUT
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 6: ED_1 performs DNS query for threadgroup1.org");
    ed1.Get<Dns::Client>().Stop();
    SuccessOrQuit(ed1.Get<Dns::Client>().Start());
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kThreadGroup1, HandleDnsResponse, nullptr));

    /**
     * Step 7
     * - Device: BR_1 (DUT)
     * - Description (DNS-11.3): Automatically processes the DNS query by requesting
     *   upstream to the Eth_1 DNS server. Then, it responds back with the answer
     *   to ED_1.
     * - Pass Criteria:
     *   - N/A
     *
     * Step 8
     * - Device: ED_1
     * - Description (DNS-11.3): Successfully receives DNS query result:
     *   threadgroup1.org AAAA 2002:1234::E1:1
     * - Pass Criteria:
     *   - ED_1 MUST receive correct DNS query answer from the DUT.
     */
    Log("Step 7 & 8: Verify BR_1 requests upstream to Eth_1 and ED_1 receives correct answer");
    nexus.AdvanceTime(5000);

    /**
     * Step 9
     * - Device: Eth_2
     * - Description (DNS-11.3): Harness instructs device to configure router
     *   advertisement daemon (radvd) to multicast ND RA with updated information,
     *   pointing to another DNS server: Prefix Information Option (PIO) - as
     *   before, Recursive DNS Server Option (25), with Addresses of IPv6
     *   Recursive DNS Servers field contains a single global IPv6 address of
     *   Eth_2. Harness waits for a time of at least RA_PERIOD + 1 seconds, where
     *   RA_PERIOD is the max period between multicast ND RA transmissions by
     *   Eth_2, to ensure that the new ND RA is received by DUT.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 9: Eth_2 sends RA with global IPv6 address of Eth_2");
    SendRa(eth2, gua1Prefix, eth2Addr);
    nexus.AdvanceTime(10000); // Wait for RA_PERIOD + 1 seconds

    /**
     * Step 10
     * - Device: ED_1
     * - Description (DNS-11.3): Harness instructs device to perform DNS query
     *   Qtype=AAAA, name threadgroup2.org. Automatically, the DNS query gets
     *   routed to the DUT.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 10: ED_1 performs DNS query for threadgroup2.org");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kThreadGroup2, HandleDnsResponse, nullptr));

    /**
     * Step 11
     * - Device: BR_1 (DUT)
     * - Description (DNS-11.3): Automatically processes the DNS query by requesting
     *   upstream to the Eth_2 DNS server. Then, it responds back with the answer
     *   to ED_1.
     * - Pass Criteria:
     *   - N/A
     *
     * Step 12
     * - Device: ED_1
     * - Description (DNS-11.3): Successfully receives DNS query result:
     *   threadgroup2.org AAAA 2002:1234::E2:2
     * - Pass Criteria:
     *   - ED_1 MUST receive correct DNS query answer from the DUT.
     */
    Log("Step 11 & 12: Verify BR_1 requests upstream to Eth_2 and ED_1 receives correct answer");
    nexus.AdvanceTime(5000);

    /**
     * Step 13
     * - Device: Eth_2
     * - Description (DNS-11.3): Harness instructs device to configure router
     *   advertisement daemon (radvd) to multicast ND RA with updated information,
     *   pointing to the first DNS server using link-local address: Prefix
     *   Information Option (PIO) - as before, Recursive DNS Server Option (25),
     *   with Addresses of IPv6 Recursive DNS Servers field contains a single
     *   link-local IPv6 address of Eth_1. Harness waits for a time of at least
     *   RA_PERIOD + 1 seconds, where RA_PERIOD is the max period between
     *   multicast ND RA transmissions by Eth_2, to ensure that the new RA is
     *   received by DUT.
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 13: Eth_2 sends RA with link-local IPv6 address of Eth_1");
    {
        const Ip6::Address &eth1Lla = eth1.mInfraIf.GetLinkLocalAddress();
        SendRa(eth2, gua1Prefix, eth1Lla);
    }
    nexus.AdvanceTime(10000); // Wait for RA_PERIOD + 1 seconds

    /**
     * Step 14
     * - Device: ED_1
     * - Description (DNS-11.3): Harness instructs device to perform DNS query
     *   Qtype=AAAA, name threadgroup3.org. Automatically, the DNS query gets
     *   routed to the DUT
     * - Pass Criteria:
     *   - N/A
     */
    Log("Step 14: ED_1 performs DNS query for threadgroup3.org");
    SuccessOrQuit(ed1.Get<Dns::Client>().ResolveAddress(kThreadGroup3, HandleDnsResponse, nullptr));

    /**
     * Step 15
     * - Device: BR_1 (DUT)
     * - Description (DNS-11.3): Automatically processes the DNS query by requesting
     *   upstream to the Eth_1 DNS server. Then, it responds back with the answer
     *   to ED_1.
     * - Pass Criteria:
     *   - N/A
     *
     * Step 16
     * - Device: ED_1
     * - Description (DNS-11.3): Successfully receives DNS query result:
     *   threadgroup3.org AAAA 2002:1234::E1:3
     * - Pass Criteria:
     *   - ED_1 MUST receive correct DNS query answer from the DUT.
     */
    Log("Step 15 & 16: Verify BR_1 requests upstream to Eth_1 and ED_1 receives correct answer");
    nexus.AdvanceTime(5000);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_4_DNS_TC_3((argc > 2) ? argv[2] : "test_1_4_dns_tc_3.json");
    ot::Nexus::Log("All tests passed");
    return 0;
}
