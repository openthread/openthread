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

namespace {

struct NoEchoReplyContext
{
    bool     mReceived;
    uint16_t mId;
};

void HandleIcmpNoEchoReply(void *aContext, otMessage *, const otMessageInfo *, const otIcmp6Header *aIcmpHeader)
{
    NoEchoReplyContext      *ctx    = static_cast<NoEchoReplyContext *>(aContext);
    const Ip6::Icmp::Header *header = AsCoreTypePtr(aIcmpHeader);

    if (header->GetType() == Ip6::Icmp::Header::kTypeEchoReply && header->GetId() == ctx->mId)
    {
        ctx->mReceived = true;
    }
}

void SendAndVerifyNoEchoReply(Core &aNexus, Node &aSender, const Ip6::Address &aDestination, uint32_t aTimeout = 1000)
{
    static constexpr uint16_t kIdentifier = 0xabcd;

    NoEchoReplyContext context;
    context.mReceived = false;
    context.mId       = kIdentifier;

    Ip6::Icmp::Handler icmpHandler(HandleIcmpNoEchoReply, &context);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    aSender.SendEchoRequest(aDestination, kIdentifier);
    aNexus.AdvanceTime(aTimeout);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(!context.mReceived, "Echo reply received when none was expected");
}

} // namespace

void TestService(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER1");
    router2.SetName("ROUTER2");

    Log("---------------------------------------------------------------------------------------");
    Log("Forming network...");
    AllowLinkBetween(leader, router1);
    AllowLinkBetween(leader, router2);
    AllowLinkBetween(router1, router2);

    leader.Form();
    nexus.AdvanceTime(10000);

    router1.Join(leader);
    router2.Join(leader);

    // Wait for all to become routers
    nexus.AdvanceTime(300000);

    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    Ip6::Address aloc0;
    Ip6::Address aloc1;

    aloc0.SetToAnycastLocator(leader.Get<Mle::Mle>().GetMeshLocalPrefix(), Mle::Aloc16::FromServiceId(0));
    aloc1.SetToAnycastLocator(leader.Get<Mle::Mle>().GetMeshLocalPrefix(), Mle::Aloc16::FromServiceId(1));

    // Initial check: no ALOCs
    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(aloc0));
        VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(aloc1));
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Add Service 0 on ROUTER1");

    uint32_t enterpriseNumber0 = 123;
    uint8_t  serviceData0[]    = {0x31, 0x32, 0x33}; // "123"
    uint8_t  serverData0[]     = {0x61, 0x62, 0x63}; // "abc"

    NetworkData::ServiceData sData0;
    NetworkData::ServerData  rData0;

    sData0.Init(serviceData0, sizeof(serviceData0));
    rData0.Init(serverData0, sizeof(serverData0));

    SuccessOrQuit(router1.Get<NetworkData::Local>().AddService(enterpriseNumber0, sData0, true, rData0));
    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    // Verify Service 0 ALOC on ROUTER1
    // Note: Leader assigns Service IDs. First service should get ID 0.
    VerifyOrQuit(!leader.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router1.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router2.Get<ThreadNetif>().HasUnicastAddress(aloc0));

    Log("Pinging Service 0 ALOC %s from all nodes", aloc0.ToString().AsCString());
    for (Node &node : nexus.GetNodes())
    {
        nexus.SendAndVerifyEchoRequest(node, aloc0);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Add same Service 0 on LEADER");
    SuccessOrQuit(leader.Get<NetworkData::Local>().AddService(enterpriseNumber0, sData0, true, rData0));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router1.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router2.Get<ThreadNetif>().HasUnicastAddress(aloc0));

    Log("Pinging Service 0 ALOC from all nodes again");
    for (Node &node : nexus.GetNodes())
    {
        nexus.SendAndVerifyEchoRequest(node, aloc0);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Add Service 1 on ROUTER2");

    uint32_t enterpriseNumber1 = 234;
    uint8_t  serviceData1[]    = {0x34, 0x35, 0x36}; // "456"
    uint8_t  serverData1[]     = {0x64, 0x65, 0x66}; // "def"

    NetworkData::ServiceData sData1;
    NetworkData::ServerData  rData1;

    sData1.Init(serviceData1, sizeof(serviceData1));
    rData1.Init(serverData1, sizeof(serverData1));

    SuccessOrQuit(router2.Get<NetworkData::Local>().AddService(enterpriseNumber1, sData1, true, rData1));
    router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router1.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router2.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router2.Get<ThreadNetif>().HasUnicastAddress(aloc1));
    VerifyOrQuit(!leader.Get<ThreadNetif>().HasUnicastAddress(aloc1));
    VerifyOrQuit(!router1.Get<ThreadNetif>().HasUnicastAddress(aloc1));

    Log("Pinging both ALOCs from all nodes");
    for (Node &node : nexus.GetNodes())
    {
        nexus.SendAndVerifyEchoRequest(node, aloc0);
        nexus.SendAndVerifyEchoRequest(node, aloc1);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Remove Service 0 from ROUTER1");
    SuccessOrQuit(router1.Get<NetworkData::Local>().RemoveService(enterpriseNumber0, sData0));
    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router1.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router2.Get<ThreadNetif>().HasUnicastAddress(aloc1));

    Log("Pinging both ALOCs from all nodes again");
    for (Node &node : nexus.GetNodes())
    {
        nexus.SendAndVerifyEchoRequest(node, aloc0);
        nexus.SendAndVerifyEchoRequest(node, aloc1);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Remove Service 0 from LEADER");
    SuccessOrQuit(leader.Get<NetworkData::Local>().RemoveService(enterpriseNumber0, sData0));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    VerifyOrQuit(!leader.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router1.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(!router2.Get<ThreadNetif>().HasUnicastAddress(aloc0));
    VerifyOrQuit(router2.Get<ThreadNetif>().HasUnicastAddress(aloc1));

    Log("Service 0 ALOC should now be unreachable");
    for (Node &node : nexus.GetNodes())
    {
        SendAndVerifyNoEchoReply(nexus, node, aloc0);
    }
    Log("Service 1 ALOC should still be reachable");
    for (Node &node : nexus.GetNodes())
    {
        nexus.SendAndVerifyEchoRequest(node, aloc1);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Remove Service 1 from ROUTER2");
    SuccessOrQuit(router2.Get<NetworkData::Local>().RemoveService(enterpriseNumber1, sData1));
    router2.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(5000);

    for (Node &node : nexus.GetNodes())
    {
        VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(aloc0));
        VerifyOrQuit(!node.Get<ThreadNetif>().HasUnicastAddress(aloc1));
    }

    Log("Service 1 ALOC should now be unreachable");
    for (Node &node : nexus.GetNodes())
    {
        SendAndVerifyNoEchoReply(nexus, node, aloc1);
    }

    Log("All tests passed!");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestService();
    return 0;
}
