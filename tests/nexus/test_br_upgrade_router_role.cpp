/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

/**
 * Time to advance for a node to form a network and become leader.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 125 * 1000;

/**
 * Time to wait for BR to upgrade to router.
 */
static constexpr uint32_t kBrUpgradeTime = 15 * 1000;

void TestBrUpgradeRouterRole(void)
{
    // This test verifies behavior of Border Routers (which provide IP connectivity) requesting router role within
    // Thread mesh.

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &br1    = nexus.CreateNode();
    Node &br2    = nexus.CreateNode();
    Node &br3    = nexus.CreateNode();

    leader.SetName("Leader");
    router.SetName("Router");
    br1.SetName("BR_1");
    br2.SetName("BR_2");
    br3.SetName("BR_3");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Set the router upgrade threshold to 2 on all nodes.");

    leader.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);
    router.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);
    br1.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);
    br2.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);
    br3.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);

    Log("---------------------------------------------------------------------------------------");
    Log("Start the leader and router.");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Start all three BRs, we expect all to stay as child since there are already two");
    Log("routers in the Thread mesh.");

    br1.Join(leader);
    br2.Join(leader);
    br3.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(br2.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(br3.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Add an external route on `br1`, it should now try to become a router requesting");
    Log("with status BorderRouterRequest. Verify that leader allows it to become router.");

    {
        NetworkData::ExternalRouteConfig routeConfig;
        routeConfig.Clear();
        SuccessOrQuit(routeConfig.GetPrefix().FromString("2001:dead:beef:cafe::/64"));
        routeConfig.mStable = true;
        SuccessOrQuit(br1.Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig));
        br1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kBrUpgradeTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Add a prefix with default route on `br2`, it should also become a router.");

    {
        NetworkData::OnMeshPrefixConfig prefixConfig;
        prefixConfig.Clear();
        SuccessOrQuit(prefixConfig.GetPrefix().FromString("2001:dead:beef:2222::/64"));
        prefixConfig.mSlaac        = true;
        prefixConfig.mPreferred    = true;
        prefixConfig.mOnMesh       = true;
        prefixConfig.mDefaultRoute = true;
        prefixConfig.mStable       = true;
        SuccessOrQuit(br2.Get<NetworkData::Local>().AddOnMeshPrefix(prefixConfig));
        br2.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kBrUpgradeTime);
    VerifyOrQuit(br2.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Add an external route on `br3`, it should not become a router since we already have");
    Log("two that requested router role upgrade as border router reason.");

    {
        NetworkData::ExternalRouteConfig routeConfig;
        routeConfig.Clear();
        SuccessOrQuit(routeConfig.GetPrefix().FromString("2001:dead:beef:cafe::/64"));
        routeConfig.mStable = true;
        SuccessOrQuit(br3.Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig));
        br3.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br3.Get<Mle::Mle>().IsChild());

    Log("---------------------------------------------------------------------------------------");
    Log("Remove the external route on `br1`. This should now trigger `br3` to request a");
    Log("router role upgrade since number of BRs acting as router in network data is now");
    Log("below the threshold of two.");

    {
        Ip6::Prefix prefix;
        SuccessOrQuit(prefix.FromString("2001:dead:beef:cafe::/64"));
        SuccessOrQuit(br1.Get<NetworkData::Local>().RemoveHasRoutePrefix(prefix));
        br1.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    nexus.AdvanceTime(kBrUpgradeTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br3.Get<Mle::Mle>().IsRouter());

    nexus.SaveTestInfo("test_br_upgrade_router_role.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestBrUpgradeRouterRole();
    printf("All tests passed\n");
    return 0;
}
