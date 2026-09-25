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
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kPropagationTime    = 10 * 1000;

void SendAddressError(Node &aSender, const Ip6::Address &aDestination, const Ip6::Address &aTarget)
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

    const Ip6::Address victimRloc = victimRouter.Get<Mle::Mle>().GetMeshLocalRloc();
    Ip6::Address       leaderAloc;

    leader.Get<Mle::Mle>().ComposeLeaderAloc(leaderAloc);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimRloc));
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));

    // A joined end device's Address Error must not remove a router's RLOC.
    SendAddressError(joinedEndDevice, victimRloc, victimRloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(victimRloc));
    nexus.SendAndVerifyEchoRequest(joinedEndDevice, victimRloc);

    // The same protection applies to a leader's anycast locator.
    SendAddressError(joinedEndDevice, leader.Get<Mle::Mle>().GetMeshLocalRloc(), leaderAloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));

    // Duplicate resolution must still be able to remove an ordinary EID.
    Ip6::Netif::UnicastAddress eid;

    eid.InitAsThreadOriginGlobalScope();
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(victimRouter.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(joinedEndDevice, victimRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    // A non-mesh-local EID remains eligible even if its IID resembles a locator.
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::ff:fe00:1234"));
    SuccessOrQuit(victimRouter.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(joinedEndDevice, victimRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!victimRouter.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    nexus.AdvanceTime(10 * 60 * 1000);
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
