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
#include "thread/child_table.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kPropagationTime    = 10 * 1000;

static void SendAddressError(Node &aSender, const Ip6::Address &aDestination, const Ip6::Address &aTarget)
{
    Tmf::Message *message = aSender.Get<Tmf::Agent>().AllocateAndInitPostMessageTo(kUriAddressError, aDestination);

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(Tlv::Append<ThreadTargetTlv>(*message, aTarget));
    SuccessOrQuit(Tlv::Append<ThreadMeshLocalEidTlv>(*message, aSender.Get<Mle::Mle>().GetMeshLocalEid().GetIid()));
    SuccessOrQuit(aSender.Get<Tmf::Agent>().SendMessageTo(*message, aDestination));
}

void TestAddressErrorLocator(void)
{
    Core  nexus;
    Node &leader          = nexus.CreateNode();
    Node &victimRouter    = nexus.CreateNode();
    Node &joinedEndDevice = nexus.CreateNode();

    leader.SetName("LEADER");
    victimRouter.SetName("VICTIM_ROUTER");
    joinedEndDevice.SetName("JOINED_END_DEVICE");

    Log("1. Form a network with a router and an end device");

    AllowLinkBetween(leader, victimRouter);
    AllowLinkBetween(leader, joinedEndDevice);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    victimRouter.Join(leader, Node::kAsFtd);
    joinedEndDevice.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(victimRouter.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(joinedEndDevice.Get<Mle::Mle>().IsChild());

    const Ip6::Address victimRloc      = victimRouter.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address leaderRloc      = leader.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address victimMle       = victimRouter.Get<Mle::Mle>().GetMeshLocalEid();
    const Ip6::Address victimLinkLocal = victimRouter.Get<Mle::Mle>().GetLinkLocalAddress();
    const Ip6::Address childMle        = joinedEndDevice.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address       leaderAloc;

    leader.Get<Mle::Mle>().ComposeLeaderAloc(leaderAloc);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimRloc));
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));

    Log("2. Confirm Address Errors remove ordinary EIDs on the router and leader");

    Ip6::Netif::UnicastAddress eid;

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(victimRouter.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(joinedEndDevice, victimRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::2"));
    SuccessOrQuit(leader.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(victimRouter, leaderRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!leader.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("3. Keep the router RLOC and leader ALOC");

    SendAddressError(joinedEndDevice, victimRloc, victimRloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimRloc));
    nexus.SendAndVerifyEchoRequest(joinedEndDevice, victimRloc);

    SendAddressError(victimRouter, leaderRloc, leaderAloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));

    Log("4. Keep the router ML-EID and link-local address");

    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimMle));
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimLinkLocal));

    SendAddressError(joinedEndDevice, victimRloc, victimMle);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimMle));

    SendAddressError(joinedEndDevice, victimRloc, victimLinkLocal);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimLinkLocal));

    Log("5. Keep the child's ML-EID in its parent address table");

    Child *child =
        leader.Get<ChildTable>().FindChild(joinedEndDevice.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);

    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(child->HasIp6Address(childMle));

    SendAddressError(victimRouter, leaderRloc, childMle);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(child->HasIp6Address(childMle));

    Log("6. Keep global EID duplicate resolution with a locator-like IID");

    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::ff:fe00:1234"));
    SuccessOrQuit(victimRouter.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(joinedEndDevice, victimRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));
    VerifyOrQuit(victimRouter.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimRloc));
    nexus.SendAndVerifyEchoRequest(joinedEndDevice, victimRloc);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAddressErrorLocator();
    printf("All tests passed\n");
    return 0;
}
