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

#include "net/dhcp6_types.hpp"
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
 * Time to advance for the ping response, in milliseconds.
 */
static constexpr uint32_t kPingTime = 2 * 1000;

/**
 * DHCPv6-PD Server Port.
 */
static constexpr uint16_t kDhcp6ServerPort = 547;

/**
 * DHCPv6-PD Client Port.
 */
static constexpr uint16_t kDhcp6ClientPort = 546;

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
static const char kDelegatedPrefix[] = "2005:1234:abcd:0::/64";

/**
 * Eth_1 GUA address prefix.
 */
static const char kEth1Prefix[] = "2001:db8:1::/64";

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * DHCPv6 Server Hook to simulate DHCPv6-PD server.
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

            uint16_t      offset = aMessage.GetOffset() + sizeof(Dhcp6::Header);
            uint16_t      length = aMessage.GetLength() - offset;
            Dhcp6::Option option;
            uint32_t      iaid = 0;

            while (length >= sizeof(Dhcp6::Option))
            {
                SuccessOrQuit(aMessage.Read(offset, option));
                VerifyOrQuit(length >= option.GetSize());
                if (option.GetCode() == Dhcp6::Option::kClientId)
                {
                    SuccessOrQuit(reply->AppendBytesFromMessage(aMessage, offset, option.GetSize()));
                }
                else if (option.GetCode() == Dhcp6::Option::kIaPd)
                {
                    Dhcp6::IaPdOption iaPdSolicit;

                    VerifyOrQuit(option.GetSize() >= sizeof(iaPdSolicit));
                    SuccessOrQuit(aMessage.Read(offset, iaPdSolicit));
                    iaid = iaPdSolicit.GetIaid();
                }
                offset += option.GetSize();
                length -= option.GetSize();
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

    return handled;
}

static bool sDropInternetPings = false;

void Br1ReceiveCallback(otMessage *aMessage, void *aContext)
{
    if (sDropInternetPings)
    {
        Ip6::Header  header;
        Ip6::Address internetServer;

        SuccessOrQuit(AsCoreType(aMessage).Read(0, header));
        SuccessOrQuit(internetServer.FromString(kInternetServerAddr));

        if (header.GetDestination() == internetServer)
        {
            // Drop it to simulate "no route to host"
            return;
        }
    }

    Node::HandleIp6Receive(aMessage, aContext);
}

void Test_1_4_PIC_TC_3(void)
{
    /**
     * 10.3. IPv6 default route advertisement
     *
     * 10.3.1. Purpose
     *   - To verify that the BR DUT:
     *   - Correctly advertises a default route on the Thread Network, using the zero-length route ::/0 in a Prefix TLV.
     *   - Does not withdraws the advertised default route when the infrastructure network disables the default route,
     *     as long as there's still a non-ULA prefix active on the AIL.
     *   - Still advertises the default route, when the infrastructure network re-enables the default route.
     *   - TBD: a future test update needs to validate that when only a ULA prefix is set at the AIL, the default route
     *     gets withdrawn and replaced by fc00::/7 route.
     *
     * 10.3.2. Topology
     *   - BR_1 (DUT) - Border Router
     *   - Router_1 - Thread Router Reference Device, attached to BR_1
     *   - ED_1 - Thread Reference Device, End Device (e.g. FED/REED) role, attached to Router_1
     *   - Eth_1 - Adjacent Infrastructure Link Linux Reference Device
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
     *   - Description (PIC-10.3): Enable
     *   - Pass Criteria:
     *     - N/A
     */
    eth1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    eth2.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    eth1.Get<BorderRouter::RoutingManager>().Init();
    eth2.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(eth1.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    SuccessOrQuit(eth2.Get<BorderRouter::RoutingManager>().SetEnabled(false));

    {
        Ip6::Address internetServer;
        SuccessOrQuit(internetServer.FromString(kInternetServerAddr));
        eth2.mInfraIf.AddAddress(internetServer);
    }

    Ip6::Prefix pioPrefix;
    SuccessOrQuit(pioPrefix.FromString(kEth1Prefix));
    eth2.mInfraIf.StartRouterAdvertisement(pioPrefix);
    eth2.mInfraIf.SetUdpHook(HandleEth2Udp);

    Log("Step 2: Eth_2 default configuration");
    /**
     * Step 2
     *   - Device: Eth_2
     *   - Description (PIC-10.3): Harness instructs device to: (only default configuration is applied)
     *   - Pass Criteria:
     *     - N/A
     */
    nexus.AdvanceTime(kPingTime);

    Log("Step 3: Enable BR_1, Router_1, ED_1");
    /**
     * Step 3
     *   - Device: BR_1 (DUT), Router_1, ED_1
     *   - Description (PIC-10.3): Enable
     *   - Pass Criteria:
     *     - N/A
     */
    br1.AllowList(r1);
    r1.AllowList(br1);
    r1.AllowList(ed1);
    ed1.AllowList(r1);

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    br1.Get<BorderRouter::RoutingManager>().SetDhcp6PdEnabled(true);
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    br1.Get<Ip6::Ip6>().SetReceiveCallback(Br1ReceiveCallback, &br1);

    r1.Join(br1);
    nexus.AdvanceTime(kJoinNetworkTime);

    ed1.Join(r1);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("Step 4: BR_1 obtains OMR prefix via DHCPv6-PD");
    /**
     * Step 4
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.3): Automatically uses DHCPv6-PD client function to obtain a delegated prefix from the
     *     DHCPv6 server. It configures this prefix as the OMR prefix.
     *   - Pass Criteria:
     *     - N/A
     */
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 5: BR_1 automatically advertises a default route in its Thread Network Data");
    /**
     * Step 5
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.3): Automatically advertises a default route in its Thread Network Data.
     *   - Pass Criteria:
     *     - Thread Network Data MUST contain: Prefix TLV with an OMR prefix OMR_1 that starts with
     *       2005:1234:abcd:0::/56.
     *     - Border Router sub-TLV MUST indicate the RLOC16 of BR_1, in a P_border_router_16 field.
     *     - Flag R (P_default) MUST be ‘1’
     *     - Flag O (P_on_mesh) MUST be ‘1’
     *     - Thread Network Data MUST contain: Prefix TLV with the zero-length prefix ::/0
     *     - Has Route sub-TLV Flag NP MUST be ‘0’
     */
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 6: ED_1 pings internet address");
    /**
     * Step 6
     *   - Device: ED_1
     *   - Description (PIC-10.3): Harness instructs device to send ping to destination address 2002:1234::1234 with
     *     its (default) OMR source address, to validate that default route works.
     *   - Pass Criteria:
     *     - Ping response received by ED_1.
     */
    Ip6::Address internetServer;
    SuccessOrQuit(internetServer.FromString(kInternetServerAddr));
    ed1.SendEchoRequest(internetServer, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 7: Eth_2 stops advertising a default route");
    /**
     * Step 7
     *   - Device: Eth_2
     *   - Description (PIC-10.3): Harnes instructs device to reconfigure ND RA daemon to stop advertising a default
     *     route: “Router Lifetime” field set to 0 sec (indicating it is not a default router for IPv6).
     *   - Pass Criteria:
     *     - N/A
     */
    eth2.mInfraIf.StopRouterAdvertisement();
    nexus.AdvanceTime(kPingTime);

    for (int i = 0; i < 3; i++)
    {
        Ip6::Nd::RouterAdvert::TxMessage ra;
        Ip6::Nd::RouterAdvert::Header    header;
        Ip6::Nd::Icmp6Packet             packet;

        header.SetToDefault();
        header.SetRouterLifetime(0);
        SuccessOrQuit(ra.Append(header));
        SuccessOrQuit(ra.AppendPrefixInfoOption(pioPrefix, 1800, 1800,
                                                Ip6::Nd::PrefixInfoOption::kOnLinkFlag |
                                                    Ip6::Nd::PrefixInfoOption::kAutoConfigFlag));
        ra.GetAsPacket(packet);
        eth2.mInfraIf.SendIcmp6Nd(Ip6::Address::GetLinkLocalAllNodesMulticast(), packet.GetBytes(), packet.GetLength());
        nexus.AdvanceTime(1000);
    }
    nexus.AdvanceTime(200 * 1000);

    Log("Step 8: BR_1 does not stop advertising a default route");
    /**
     * Step 8
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.3): Does not stop advertising a default route in its Thread Network Data.
     *   - Pass Criteria:
     *     - Thread Network Data MUST contain: Prefix TLV with same OMR_1.
     *     - Border Router sub-TLV MUST indicate the RLOC16 of BR_1, in a P_border_router_16 field.
     *     - Flag R (P_default) MUST be ‘1’
     *     - Flag O (P_on_mesh) MUST be ‘1’
     *     - Thread Network Data MUST contain: Prefix TLV with the zero-length prefix ::/0
     *     - Has Route sub-TLV Flag NP MUST be ‘0’
     */
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 9: ED_1 pings Eth_1");
    /**
     * Step 9
     *   - Device: ED_1
     *   - Description (PIC-10.3): Harness instructs device to send ping to destination Eth_1 with its (default) OMR
     *     source address, to validate that route to AIL still works.
     *   - Pass Criteria:
     *     - Ping response received by ED_1.
     */
    Ip6::Address eth1Addr = eth1.mInfraIf.FindMatchingAddress(kEth1Prefix);
    ed1.SendEchoRequest(eth1Addr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 10: ED_1 pings internet address (should fail)");
    /**
     * Step 10
     *   - Device: ED_1
     *   - Description (PIC-10.3): Harness instructs device to send ping to destination address 2002:1234::1234 with
     *     its (default) OMR source address, to validate that default route does not work at the moment.
     *   - Pass Criteria:
     *     - Ping request MUST NOT be sent by DUT onto the AIL.
     *     - Ping response MUST NOT be received by ED_1.
     */
    sDropInternetPings = true;
    ed1.SendEchoRequest(internetServer, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(10 * 1000);
    sDropInternetPings = false;

    Log("Step 11: Eth_2 starts advertising a default route again");
    /**
     * Step 11
     *   - Device: Eth_2
     *   - Description (PIC-10.3): Harness instructs device to reconfigure ND RA daemon to start advertising a default
     *     route again: “Router Lifetime” field set to 875 sec (indicating it is a default router for IPv6).
     *   - Pass Criteria:
     *     - N/A
     */
    eth2.mInfraIf.StartRouterAdvertisement(pioPrefix);
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 12: BR_1 automatically advertises a default route (same as step 5)");
    /**
     * Step 12
     *   - Device: BR_1 (DUT)
     *   - Description (PIC-10.3): (As in step 5)
     *   - Pass Criteria:
     *     - (As in step 5)
     */
    nexus.AdvanceTime(kBrActionTime);

    Log("Step 13: ED_1 pings internet address (same as step 6)");
    /**
     * Step 13
     *   - Device: ED_1
     *   - Description (PIC-10.3): (As in step 6)
     *   - Pass Criteria:
     *     - (As in step 6)
     */
    ed1.SendEchoRequest(internetServer, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    Log("Step 14: ED_1 pings Eth_1 (same as step 9)");
    /**
     * Step 14
     *   - Device: ED_1
     *   - Description (PIC-10.3): (As in step 9)
     *   - Pass Criteria:
     *     - (As in step 9)
     */
    ed1.SendEchoRequest(eth1Addr, kEchoIdentifier, kEchoPayloadSize);
    nexus.AdvanceTime(kPingTime);

    nexus.AddTestVar("ETH_1_ADDR", eth1Addr.ToString().AsCString());
    nexus.AddTestVar("ETH_2_ADDR", eth2.mInfraIf.GetLinkLocalAddress().ToString().AsCString());
    nexus.AddTestVar("INTERNET_SERVER_ADDR", kInternetServerAddr);

    nexus.SaveTestInfo("test_1_4_PIC_TC_3.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_1_4_PIC_TC_3();
    printf("All tests passed\n");
    return 0;
}
