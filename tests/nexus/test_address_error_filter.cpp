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

void TestAddressErrorFilter(void)
{
    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    med.SetName("MED");

    Log("1. Form a network with a router and a MED");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, med);
    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader, Node::kAsFtd);
    med.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());

    const Ip6::Address routerRloc      = router.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address leaderRloc      = leader.Get<Mle::Mle>().GetMeshLocalRloc();
    const Ip6::Address routerMlEid     = router.Get<Mle::Mle>().GetMeshLocalEid();
    const Ip6::Address routerLinkLocal = router.Get<Mle::Mle>().GetLinkLocalAddress();
    const Ip6::Address childMlEid      = med.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address       leaderAloc;

    leader.Get<Mle::Mle>().ComposeLeaderAloc(leaderAloc);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerRloc));
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));

    Log("2. Confirm Address Errors remove ordinary EIDs on the router and leader");

    Ip6::Netif::UnicastAddress eid;

    eid.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::1"));
    SuccessOrQuit(router.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(med, routerRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::2"));
    SuccessOrQuit(leader.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(router, leaderRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!leader.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("3. Keep the router RLOC and leader ALOC");

    SendAddressError(med, routerRloc, routerRloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerRloc));
    nexus.SendAndVerifyEchoRequest(med, routerRloc);

    SendAddressError(router, leaderRloc, leaderAloc);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(leader.Get<ThreadNetif>().HasUnicastAddress(leaderAloc));
    nexus.SendAndVerifyEchoRequest(med, leaderAloc);

    Log("4. Keep the router ML-EID and link-local address");

    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerMlEid));
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerLinkLocal));

    SendAddressError(med, routerRloc, routerMlEid);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerMlEid));

    SendAddressError(med, routerRloc, routerLinkLocal);
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerLinkLocal));

    Log("5. Keep the child's ML-EID in its parent address table");

    Child *child = leader.Get<ChildTable>().FindChild(med.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);

    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(child->HasIp6Address(childMlEid));

    SendAddressError(router, leaderRloc, childMlEid);
    nexus.AdvanceTime(kPropagationTime);
    child = leader.Get<ChildTable>().FindChild(med.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(child != nullptr);
    VerifyOrQuit(child->HasIp6Address(childMlEid));

    Log("6. Keep global EID duplicate resolution with a locator-like IID");

    SuccessOrQuit(eid.GetAddress().FromString("2001:db8::ff:fe00:1234"));
    SuccessOrQuit(router.Get<ThreadNetif>().AddExternalUnicastAddress(eid));
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    SendAddressError(med, routerRloc, eid.GetAddress());
    nexus.AdvanceTime(kPropagationTime);
    VerifyOrQuit(!router.Get<ThreadNetif>().HasUnicastAddress(eid.GetAddress()));

    Log("7. Verify the router remains a functional router");

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router.Get<ThreadNetif>().HasUnicastAddress(routerRloc));
    nexus.SendAndVerifyEchoRequest(med, routerRloc);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAddressErrorFilter();
    printf("All tests passed\n");
    return 0;
}
