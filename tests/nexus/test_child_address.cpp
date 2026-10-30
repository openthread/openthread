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

#include "mac/data_poll_sender.hpp"
#include "net/checksum.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint16_t kPort    = 12345;
static constexpr uint32_t kPayload = 0x1234abcd;

static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *)
{
    uint32_t payload;
    Message &message = AsCoreType(aMessage);

    SuccessOrQuit(message.Read(message.GetOffset(), payload));
    VerifyOrQuit(payload == kPayload);
    ++*static_cast<uint16_t *>(aContext);
}

static void SendDatagram(Node &aSender, const Ip6::Address &aDestination, const Ip6::Address *aSource = nullptr)
{
    Ip6::Udp::Socket socket(aSender, nullptr, nullptr);
    Ip6::MessageInfo info;
    Message         *message;

    SuccessOrQuit(socket.Open(Ip6::kNetifThreadInternal));
    message = socket.NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Append(kPayload));
    info.SetSockAddr(aSource == nullptr ? aSender.Get<Mle::Mle>().GetMeshLocalEid() : *aSource);
    info.SetPeerAddr(aDestination);
    info.SetPeerPort(kPort);
    SuccessOrQuit(socket.SendTo(*message, info));
    SuccessOrQuit(socket.Close());
}

static void SendHostDatagram(Node &aRouter, const Ip6::Address &aSource, const Ip6::Address &aDestination)
{
    Message       *message = aRouter.Get<MessagePool>().Allocate(Message::kTypeIp6);
    Ip6::Header    header;
    Ip6::UdpHeader udp;

    VerifyOrQuit(message != nullptr);
    header.InitVersionTrafficClassFlow();
    header.SetPayloadLength(sizeof(udp) + sizeof(kPayload));
    header.SetNextHeader(Ip6::kProtoUdp);
    header.SetHopLimit(Ip6::kDefaultHopLimit);
    header.SetSource(aSource);
    header.SetDestination(aDestination);
    udp.Clear();
    udp.SetSourcePort(kPort);
    udp.SetDestinationPort(kPort);
    udp.SetLength(sizeof(udp) + sizeof(kPayload));
    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(message->Append(udp));
    SuccessOrQuit(message->Append(kPayload));
    message->SetOffset(sizeof(header));
    Checksum::UpdateMessageChecksum(*message, aSource, aDestination, Ip6::kProtoUdp);
    message->SetOffset(0);
    message->SetLinkSecurityEnabled(true);
    message->SetOrigin(Message::kOriginHostUntrusted);
    SuccessOrQuit(aRouter.Get<Ip6::Ip6>().SendRaw(OwnedPtr<Message>(message)));
}

static void HandleHostReceive(otMessage *aMessage, void *aContext)
{
    Message    &message = AsCoreType(aMessage);
    Ip6::Header header;
    uint32_t    payload;

    SuccessOrQuit(header.ParseFrom(message));
    if (header.GetNextHeader() == Ip6::kProtoUdp &&
        message.GetLength() == sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader) + sizeof(payload))
    {
        SuccessOrQuit(message.Read(sizeof(Ip6::Header) + sizeof(Ip6::UdpHeader), payload));
        if (payload == kPayload)
        {
            ++*static_cast<uint16_t *>(aContext);
        }
    }
    message.Free();
}

static void VerifyRegistration(Node &aRouter, Node &aChild, const Ip6::Address &aAddress)
{
    Child *child = aRouter.Get<ChildTable>().FindChild(aChild.Get<Mle::Mle>().GetRloc16(), Child::kInStateValid);

    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(!child->IsFullThreadDevice());
    VerifyOrQuit(child->HasIp6Address(aAddress));
    VerifyOrQuit(child->GetLinkFailures() < Mle::kFailedChildTransmissions);
    VerifyOrQuit(!aRouter.Get<Ip6::Ip6>().IsOnLink(aAddress));
}

static void VerifyDelivery(Core               &aNexus,
                           Node               &aLeader,
                           Node               &aRouter,
                           const Ip6::Address &aAddress,
                           uint16_t           &aReceived,
                           const Ip6::Address *aSource = nullptr)
{
    uint16_t rloc16;
    uint16_t count = aReceived;

    // Start with an empty cache to prove the parent still answers Address Queries.
    aLeader.Get<AddressResolver>().RemoveEntryForAddress(aAddress);
    VerifyOrQuit(aLeader.Get<AddressResolver>().Resolve(aAddress, rloc16) == kErrorAddressQuery);
    aNexus.AdvanceTime(1000);
    SuccessOrQuit(aLeader.Get<AddressResolver>().Resolve(aAddress, rloc16));
    VerifyOrQuit(rloc16 == aRouter.Get<Mle::Mle>().GetRloc16());
    Log("Address Query resolved registered child to parent");

    SendDatagram(aLeader, aAddress, aSource);
    aNexus.AdvanceTime(2000);
    VerifyOrQuit(aReceived == count + 1, "Registered child must receive exactly one datagram");
}

void TestChildAddress(void)
{
    Core                             nexus;
    Node                            &leader       = nexus.CreateNode();
    Node                            &router       = nexus.CreateNode();
    Node                            &med          = nexus.CreateNode();
    Node                            &sed          = nexus.CreateNode();
    uint16_t                         medReceived  = 0;
    uint16_t                         sedReceived  = 0;
    uint16_t                         hostReceived = 0;
    Ip6::Udp::Socket                 medSocket(med, HandleUdpReceive, &medReceived);
    Ip6::Udp::Socket                 sedSocket(sed, HandleUdpReceive, &sedReceived);
    Ip6::SockAddr                    sockAddr;
    NetworkData::OnMeshPrefixConfig  prefix;
    NetworkData::ExternalRouteConfig route;
    Ip6::Address                     medAddress;
    Ip6::Address                     sedAddress;
    Ip6::Address                     source;
    Ip6::Address                     routerSource;
    Ip6::Address                     hostSource;
    Ip6::Address                     unregistered;
    uint16_t                         rloc16;
    Child                           *child;

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    med.SetName("MED");
    sed.SetName("SED");
    AllowLinkBetween(leader, router);
    AllowLinkBetween(router, med);
    AllowLinkBetween(router, sed);
    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));
    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    router.Join(leader);
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    med.Join(router, Node::kAsMed);
    sed.Join(router, Node::kAsSed);
    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(500));
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    otIp6SetSlaacEnabled(&router, false);
    prefix.Clear();
    SuccessOrQuit(prefix.GetPrefix().FromString("fd00:1234::/64"));
    prefix.mPreferred = true;
    prefix.mSlaac     = true;
    prefix.mOnMesh    = true;
    prefix.mStable    = true;
    SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(prefix));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(30 * 1000);
    medAddress = med.FindMatchingAddress("fd00:1234::/64");
    sedAddress = sed.FindMatchingAddress("fd00:1234::/64");
    // Keep the remaining scripted time (~270 s) below OPENTHREAD_CONFIG_IP6_SLAAC_DEPRECATION_INTERVAL (300 s).
    SuccessOrQuit(leader.Get<NetworkData::Local>().RemoveOnMeshPrefix(prefix.GetPrefix()));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(30 * 1000);

    VerifyOrQuit(leader.Get<Ip6::Ip6>().IsOnLink(medAddress));
    VerifyRegistration(router, med, medAddress);
    VerifyRegistration(router, sed, sedAddress);
    sockAddr.Clear();
    sockAddr.SetPort(kPort);
    SuccessOrQuit(medSocket.Open(Ip6::kNetifThreadInternal));
    SuccessOrQuit(medSocket.Bind(sockAddr));
    SuccessOrQuit(sedSocket.Open(Ip6::kNetifThreadInternal));
    SuccessOrQuit(sedSocket.Bind(sockAddr));

    Log("Deliver to deprecated addresses with no Network Data route");
    VerifyDelivery(nexus, leader, router, medAddress, medReceived);
    VerifyDelivery(nexus, leader, router, sedAddress, sedReceived);

    Log("Child registrations take precedence over a covering external route");
    // Route lookup requires an on-mesh source prefix, even when the destination matches an external route.
    otIp6SetSlaacEnabled(&router, true);
    SuccessOrQuit(prefix.GetPrefix().FromString("fd00:5678::/64"));
    SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(prefix));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    route.Clear();
    SuccessOrQuit(route.GetPrefix().FromString("fc00::/7"));
    route.mStable = true;
    SuccessOrQuit(router.Get<NetworkData::Local>().AddHasRoutePrefix(route));
    router.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10 * 1000);
    source = leader.FindMatchingAddress("fd00:5678::/64");
    SuccessOrQuit(router.Get<NetworkData::Leader>().RouteLookup(source, medAddress, rloc16));
    VerifyOrQuit(rloc16 == router.Get<Mle::Mle>().GetRloc16());
    VerifyDelivery(nexus, leader, router, medAddress, medReceived, &source);
    VerifyDelivery(nexus, leader, router, sedAddress, sedReceived, &source);

    routerSource = router.FindMatchingAddress("fd00:5678::/64");
    router.Get<Ip6::Ip6>().SetReceiveCallback(HandleHostReceive, &hostReceived);

    Log("Parent-originated datagrams reach registered children without host forwarding");
    SendDatagram(router, medAddress, &routerSource);
    nexus.AdvanceTime(2000);
    VerifyOrQuit(medReceived == 3 && hostReceived == 0);
    SendDatagram(router, sedAddress, &routerSource);
    nexus.AdvanceTime(2000);
    VerifyOrQuit(sedReceived == 3 && hostReceived == 0);

    Log("Off-mesh host-untrusted datagrams reach registered children without host forwarding");
    SuccessOrQuit(hostSource.FromString("fd5c:51:51::1"));
    VerifyOrQuit(!router.Get<NetworkData::Leader>().IsOnMesh(hostSource));
    VerifyOrQuit(!router.Get<ThreadNetif>().HasUnicastAddress(hostSource));
    VerifyOrQuit(router.Get<NetworkData::Leader>().RouteLookup(hostSource, medAddress, rloc16) == kErrorNoRoute);
    VerifyOrQuit(router.Get<NetworkData::Leader>().RouteLookup(hostSource, sedAddress, rloc16) == kErrorNoRoute);
    SendHostDatagram(router, hostSource, medAddress);
    nexus.AdvanceTime(2000);
    VerifyOrQuit(medReceived == 4 && hostReceived == 0);
    SendHostDatagram(router, hostSource, sedAddress);
    nexus.AdvanceTime(2000);
    VerifyOrQuit(sedReceived == 4 && hostReceived == 0);

    Log("An unregistered address still goes to the host");
    SuccessOrQuit(unregistered.FromString("fd00:1234::1"));
    VerifyOrQuit(unregistered != medAddress && unregistered != sedAddress);
    SendDatagram(router, unregistered, &routerSource);
    nexus.AdvanceTime(2000);
    Log("Host received %u test datagrams", hostReceived);
    VerifyOrQuit(hostReceived == 1);

    Log("Children with failed links do not claim registered destinations");
    child = router.Get<ChildTable>().FindChild(med.Get<Mle::Mle>().GetRloc16(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);

    // Set failures directly to isolate the shared eligibility predicate.
    for (uint8_t i = 0; i < Mle::kFailedChildTransmissions; i++)
    {
        child->IncrementLinkFailures();
    }
    SendDatagram(router, medAddress, &routerSource);
    nexus.AdvanceTime(100);
    VerifyOrQuit(hostReceived == 2);
    VerifyOrQuit(medReceived == 4);
    child->ResetLinkFailures();

    router.Get<Ip6::Ip6>().SetReceiveCallback(Node::HandleIp6Receive, &router);

    Log("Parent reset restores registrations for deprecated addresses");
    router.Reset();
    AllowLinkBetween(leader, router);
    AllowLinkBetween(router, med);
    AllowLinkBetween(router, sed);
    router.Get<ThreadNetif>().Up();
    SuccessOrQuit(router.Get<Mle::Mle>().Start());
    nexus.AdvanceTime(200 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyRegistration(router, med, medAddress);
    VerifyRegistration(router, sed, sedAddress);
    VerifyDelivery(nexus, leader, router, medAddress, medReceived);
    VerifyDelivery(nexus, leader, router, sedAddress, sedReceived);
    VerifyOrQuit(medReceived == 5 && sedReceived == 5);
    SuccessOrQuit(medSocket.Close());
    SuccessOrQuit(sedSocket.Close());
    Log("Test passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildAddress();
    printf("All tests passed\n");
    return 0;
}
