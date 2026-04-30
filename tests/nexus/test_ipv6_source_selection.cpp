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

void TestIPv6SourceSelection(void)
{
    /**
     * Test IPv6 Source Address Selection
     *
     * Purpose & Description:
     * This test verifies that the correct source IPv6 address is selected based on the destination address.
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    AllowLinkBetween(leader, router);

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 0: Form network");

    leader.Form();
    nexus.AdvanceTime(13 * 1000); // kFormNetworkTime
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(200 * 1000); // kAttachToRouterTime
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Ip6::Address leaderRloc      = leader.Get<Mle::Mle>().GetMeshLocalRloc();
    Ip6::Address leaderMleid     = leader.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address leaderLinkLocal = leader.Get<Mle::Mle>().GetLinkLocalAddress();
    Ip6::Address leaderAloc;
    leader.Get<Mle::Mle>().GetLeaderAloc(leaderAloc);

    Ip6::Address routerRloc      = router.Get<Mle::Mle>().GetMeshLocalRloc();
    Ip6::Address routerMleid     = router.Get<Mle::Mle>().GetMeshLocalEid();
    Ip6::Address routerLinkLocal = router.Get<Mle::Mle>().GetLinkLocalAddress();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: RLOC source for RLOC destination");
    nexus.SendAndVerifyEchoRequest(router, routerRloc, leaderRloc, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: ML-EID source for ALOC destination");
    nexus.SendAndVerifyEchoRequest(router, routerMleid, leaderAloc, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: ML-EID source for ML-EID destination");
    nexus.SendAndVerifyEchoRequest(router, routerMleid, leaderMleid, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Link-local source for Link-local destination");
    nexus.SendAndVerifyEchoRequest(router, routerLinkLocal, leaderLinkLocal, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: ML-EID source for Realm-local multicast destination");
    Ip6::Address multicastAddr;
    SuccessOrQuit(multicastAddr.FromString("ff03::1"));
    nexus.SendAndVerifyEchoRequest(router, routerMleid, multicastAddr, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: GUA source for GUA destination");

    NetworkData::OnMeshPrefixConfig config;
    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString("2001::/64"));
    config.mPreferred    = true;
    config.mSlaac        = true;
    config.mOnMesh       = true;
    config.mDefaultRoute = true;
    config.mStable       = true;

    SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(10 * 1000);

    const Ip6::Address &leaderGua = leader.FindMatchingAddress("2001::/64");
    const Ip6::Address &routerGua = router.FindMatchingAddress("2001::/64");

    VerifyOrQuit(!leaderGua.IsUnspecified());
    VerifyOrQuit(!routerGua.IsUnspecified());

    nexus.SendAndVerifyEchoRequest(router, routerGua, leaderGua, 0, Ip6::kDefaultHopLimit, 1000, false);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: GUA source for external address (default route)");

    Ip6::Address externalAddr;
    SuccessOrQuit(externalAddr.FromString("2007::1"));

    // Add externalAddr to leader so it responds to pings
    otNetifAddress extNetifAddr;
    memset(&extNetifAddr, 0, sizeof(extNetifAddr));
    AsCoreType(&extNetifAddr.mAddress) = externalAddr;
    extNetifAddr.mPrefixLength         = 128;
    extNetifAddr.mPreferred            = true;
    extNetifAddr.mValid                = true;
    SuccessOrQuit(otIp6AddUnicastAddress(&leader.GetInstance(), &extNetifAddr));

    nexus.SendAndVerifyEchoRequest(router, routerGua, externalAddr, 0, Ip6::kDefaultHopLimit, 1000, false);

    nexus.SaveTestInfo("test_ipv6_source_selection.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestIPv6SourceSelection();
    printf("All tests passed\n");
    return 0;
}
