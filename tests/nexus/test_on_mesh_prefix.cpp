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
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for the network to stabilize after routers have attached.
 */
static constexpr uint32_t kStabilizationTime = 130 * 1000;

static bool HasAddressWithPrefix(Node &aNode, const char *aPrefixString)
{
    Ip6::Prefix prefix;
    bool        found = false;

    SuccessOrQuit(prefix.FromString(aPrefixString));

    for (const Ip6::Netif::UnicastAddress &addr : aNode.Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (addr.GetAddress().MatchesPrefix(prefix))
        {
            found = true;
            break;
        }
    }

    return found;
}

void TestOnMeshPrefix(void)
{
    /**
     * Test On-Mesh Prefix functionality.
     *
     * Topology:
     *   LEADER <-> ROUTER <-> MED
     *                  \ <-> SED
     */

    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();
    Node &med    = nexus.CreateNode();
    Node &sed    = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    med.SetName("MED");
    sed.SetName("SED");

    const char *kPrefix1 = "2001:2:0:1::/64";
    const char *kPrefix2 = "2001:2:0:2::/64";
    const char *kPrefix3 = "2002:2:0:3::/64";

    AllowLinkBetween(leader, router);
    AllowLinkBetween(router, med);
    AllowLinkBetween(router, sed);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Form network and attach nodes");
    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader, Node::kAsFtd);
    med.Join(router, Node::kAsMed);
    sed.Join(router, Node::kAsSed);

    SuccessOrQuit(med.Get<DataPollSender>().SetExternalPollPeriod(1000));
    SuccessOrQuit(sed.Get<DataPollSender>().SetExternalPollPeriod(1000));

    nexus.AdvanceTime(kAttachToRouterTime);

    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(med.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(sed.Get<Mle::Mle>().IsChild());

    Log("Step 2: Add prefixes on ROUTER");
    // Prefix 1: 'paros' -> Preferred, SLAAC, Router, On-Mesh, Stable
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix1));
        config.mPreferred = true;
        config.mSlaac     = true;
        config.mOnMesh    = true;
        config.mStable    = true;
        SuccessOrQuit(router.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    // Prefix 2: 'paro' -> Preferred, SLAAC, Router, On-Mesh, NOT Stable
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix2));
        config.mPreferred = true;
        config.mSlaac     = true;
        config.mOnMesh    = true;
        config.mStable    = false;
        SuccessOrQuit(router.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 3: Verify addresses on MED and SED");
    // MED should have both
    VerifyOrQuit(HasAddressWithPrefix(med, kPrefix1));
    VerifyOrQuit(HasAddressWithPrefix(med, kPrefix2));

    // SED should have only Prefix 1 (stable)
    VerifyOrQuit(HasAddressWithPrefix(sed, kPrefix1));
    VerifyOrQuit(!HasAddressWithPrefix(sed, kPrefix2));

    Log("Step 4: Verify pings from LEADER");
    nexus.SendAndVerifyEchoRequest(leader, med.FindMatchingAddress(kPrefix1));
    nexus.SendAndVerifyEchoRequest(leader, med.FindMatchingAddress(kPrefix2));
    nexus.SendAndVerifyEchoRequest(leader, sed.FindMatchingAddress(kPrefix1));

    Log("Step 5: Add prefix 3 (NOT On-Mesh)");
    // Prefix 3: 'pars' -> Preferred, SLAAC, Router, Stable, NOT On-Mesh
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString(kPrefix3));
        config.mPreferred = true;
        config.mSlaac     = true;
        config.mOnMesh    = false;
        config.mStable    = true;
        SuccessOrQuit(router.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    }

    router.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kStabilizationTime);

    Log("Step 6: Verify address with Prefix 3 exists on MED and SED");
    VerifyOrQuit(HasAddressWithPrefix(med, kPrefix3));
    VerifyOrQuit(HasAddressWithPrefix(sed, kPrefix3));

    nexus.SaveTestInfo("test_on_mesh_prefix.json");

    Log("Test passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestOnMeshPrefix();
    printf("All tests passed\n");
    return 0;
}
