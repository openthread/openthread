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

namespace ot {
namespace Nexus {

void TestPingLlaSrc(void)
{
    /**
     * Test ping with LLA source address.
     *
     * Topology:
     *   ROUTER_2 ----- ROUTER_1 ---- ROUTER_3
     *
     * ROUTER_1 is leader. ROUTER_2 and ROUTER_3 are routers.
     */

    Core nexus;

    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();

    router1.SetName("Router_1");
    router2.SetName("Router_2");
    router3.SetName("Router_3");

    AllowLinkBetween(router1, router2);
    AllowLinkBetween(router1, router3);

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network");

    router1.Form();
    nexus.AdvanceTime(13 * 1000); // kFormNetworkTime
    VerifyOrQuit(router1.Get<Mle::Mle>().IsLeader());

    router2.Join(router1);
    nexus.AdvanceTime(200 * 1000); // kAttachToRouterTime
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    router3.Join(router1);
    nexus.AdvanceTime(200 * 1000); // kAttachToRouterTime
    VerifyOrQuit(router3.Get<Mle::Mle>().IsRouter());

    Ip6::Address router1Mleid     = router1.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address router2LinkLocal = router2.Get<Mle::Mle>().GetLinkLocalAddress();
    Ip6::Address router3Mleid     = router3.Get<Mle::Mle>().GetMeshLocalEid();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Ping a neighbor with LLA src should succeed");
    // router2 pings router1's MLEID using its own LLA as source.
    nexus.SendAndVerifyEchoRequest(router2, router2LinkLocal, router1Mleid);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Ping a non-neighbor with LLA src should fail");
    // router2 pings router3's MLEID using its own LLA as source.
    // It should fail because LLA is only valid for one-hop.
    nexus.SendAndVerifyNoEchoResponse(router2, router2LinkLocal, router3Mleid);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Make sure external routes are not used for LLA src");

    NetworkData::ExternalRouteConfig routeConfig;
    routeConfig.Clear();
    SuccessOrQuit(routeConfig.GetPrefix().FromString("2001::/64"));
    routeConfig.mStable = true;

    SuccessOrQuit(router1.Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig));
    router1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10 * 1000);

    Ip6::Address externalAddr;
    SuccessOrQuit(externalAddr.FromString("2001::1"));

    // router2 pings external address using its own LLA as source.
    // External routes should not be used for LLA source.
    nexus.SendAndVerifyNoEchoResponse(router2, router2LinkLocal, externalAddr);

    nexus.SaveTestInfo("test_ping_lla_src.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestPingLlaSrc();
    printf("All tests passed\n");
    return 0;
}
