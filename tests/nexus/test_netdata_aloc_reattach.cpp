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

static constexpr uint32_t kFormNetworkTime      = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime   = 25 * 1000;
static constexpr uint32_t kNetDataSyncTime      = 10 * 1000;
static constexpr uint32_t kRoutePropagateTime   = 5 * 1000;
static constexpr uint32_t kWaitStepMs           = 10;
static constexpr uint32_t kMaxWaitSteps         = 3000;
static constexpr uint8_t  kRouterSelectionDelay = 2;

static void AddOnMeshPrefix(Node &aNode, const char *aPrefixString, bool aDhcp, bool aNdDns)
{
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mOnMesh    = true;
    config.mStable    = true;
    config.mPreferred = true;
    config.mDhcp      = aDhcp;
    config.mSlaac     = aNdDns;
    config.mNdDns     = aNdDns;

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
}

static void AddExternalRoute(Node &aNode, const char *aPrefixString)
{
    NetworkData::ExternalRouteConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mStable = true;

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddHasRoutePrefix(config));
}

static Ip6::Address ComposePrefixAloc(Node &aNode, const char *aPrefixString, bool aIsDhcp)
{
    Ip6::Prefix     prefix;
    Lowpan::Context context;
    uint16_t        aloc16;
    Ip6::Address    aloc;

    SuccessOrQuit(prefix.FromString(aPrefixString));
    aNode.Get<NetworkData::Leader>().FindContextForAddress(AsCoreType(&prefix.mPrefix), context);
    VerifyOrQuit(context.IsValid());

    aloc16 = aIsDhcp ? Mle::Aloc16::FromDhcpAgentContextId(context.GetContextId())
                     : Mle::Aloc16::FromNdAgentContextId(context.GetContextId());
    aNode.Get<Mle::Mle>().ComposeAloc(aloc16, aloc);

    return aloc;
}

void TestAlocRestoredAfterRouterBriefDetach(void)
{
    /**
     * Verifies that Service, DHCPv6 Agent, and Neighbor Discovery Agent ALOCs on a Border Router
     * are restored when the router briefly detaches, reattaches as a child receiving newer Network
     * Data (which removes its ALOCs while its temporary child RLOC16 does not match its retained
     * router entries), and then upgrades back to router reclaiming its previous RLOC16 without any
     * further Network Data change.
     */

    static const char         kDhcpPrefix[]       = "2001:db8:1::/64";
    static const char         kNdPrefix[]         = "2001:db8:2::/64";
    static const char         kLeaderRoute[]      = "2001:db8:99::/64";
    static constexpr uint32_t kEnterpriseNumber   = 44970;
    static const uint8_t      kServiceDataBytes[] = {0x5c, 0x01};
    static const uint8_t      kServerDataBytes[]  = {0x00};

    Core                     nexus;
    Node                    &leader = nexus.CreateNode();
    Node                    &br     = nexus.CreateNode();
    NetworkData::ServiceData serviceData;
    NetworkData::ServerData  serverData;
    Ip6::Address             serviceAloc;
    Ip6::Address             dhcpAloc;
    Ip6::Address             ndAloc;
    uint16_t                 brRouterRloc16;
    uint8_t                  netDataVersionBeforeDetach;
    uint8_t                  netDataVersionBeforeUpgrade;

    leader.SetName("LEADER");
    br.SetName("BR");

    AllowLinkBetween(leader, br);
    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network with LEADER and BR router");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    br.Get<Mle::Mle>().SetRouterSelectionJitter(kRouterSelectionDelay);
    br.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());

    brRouterRloc16 = br.Get<Mle::Mle>().GetRloc16();

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Register Service, DHCPv6 prefix, and ND DNS prefix on BR");

    serviceData.Init(kServiceDataBytes, sizeof(kServiceDataBytes));
    serverData.Init(kServerDataBytes, sizeof(kServerDataBytes));
    SuccessOrQuit(br.Get<NetworkData::Local>().AddService(kEnterpriseNumber, serviceData, true, serverData));
    AddOnMeshPrefix(br, kDhcpPrefix, /* aDhcp */ true, /* aNdDns */ false);
    AddOnMeshPrefix(br, kNdPrefix, /* aDhcp */ false, /* aNdDns */ true);
    br.Get<NetworkData::Notifier>().HandleServerDataUpdated();

    nexus.AdvanceTime(kNetDataSyncTime);

    leader.Get<Mle::Mle>().ComposeServiceAloc(0, serviceAloc);
    dhcpAloc = ComposePrefixAloc(leader, kDhcpPrefix, /* aIsDhcp */ true);
    ndAloc   = ComposePrefixAloc(leader, kNdPrefix, /* aIsDhcp */ false);

    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(serviceAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(dhcpAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(ndAloc));

    nexus.SendAndVerifyEchoRequest(leader, serviceAloc);
    nexus.SendAndVerifyEchoRequest(leader, dhcpAloc);
    nexus.SendAndVerifyEchoRequest(leader, ndAloc);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Briefly detach BR while LEADER increments Network Data version");

    netDataVersionBeforeDetach = leader.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet);

    UnallowLinkBetween(leader, br);
    SuccessOrQuit(br.Get<Mle::Mle>().BecomeDetached());
    nexus.AdvanceTime(kWaitStepMs);
    VerifyOrQuit(br.Get<Mle::Mle>().IsDetached());

    AddExternalRoute(leader, kLeaderRoute);
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(kRoutePropagateTime);

    VerifyOrQuit(leader.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet) != netDataVersionBeforeDetach);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Reconnect BR so it attaches as Child and then upgrades reclaiming its RLOC16");

    AllowLinkBetween(leader, br);

    for (uint32_t i = 0; (i < kMaxWaitSteps) && !br.Get<Mle::Mle>().IsChild(); i++)
    {
        nexus.AdvanceTime(kWaitStepMs);
    }

    VerifyOrQuit(br.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(br.Get<Mle::Mle>().GetRloc16() != brRouterRloc16);

    nexus.AdvanceTime(kWaitStepMs);
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(serviceAloc));
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(dhcpAloc));
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(ndAloc));

    netDataVersionBeforeUpgrade = leader.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet);

    for (uint32_t i = 0; (i < kMaxWaitSteps) && !br.Get<Mle::Mle>().IsRouter(); i++)
    {
        nexus.AdvanceTime(kWaitStepMs);
    }

    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br.Get<Mle::Mle>().GetRloc16() == brRouterRloc16);

    nexus.AdvanceTime(kWaitStepMs);
    VerifyOrQuit(leader.Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet) == netDataVersionBeforeUpgrade);

    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(serviceAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(dhcpAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(ndAloc));

    nexus.AdvanceTime(kRoutePropagateTime);
    nexus.SendAndVerifyEchoRequest(leader, serviceAloc);
    nexus.SendAndVerifyEchoRequest(leader, dhcpAloc);
    nexus.SendAndVerifyEchoRequest(leader, ndAloc);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify stopping and restarting MLE on BR removes and restores ALOCs");

    br.Get<Mle::Mle>().Stop();
    nexus.AdvanceTime(kWaitStepMs);
    VerifyOrQuit(br.Get<Mle::Mle>().IsDisabled());
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(serviceAloc));
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(dhcpAloc));
    VerifyOrQuit(!br.Get<ThreadNetif>().HasUnicastAddress(ndAloc));

    SuccessOrQuit(br.Get<Mle::Mle>().Start());
    for (uint32_t i = 0; (i < kMaxWaitSteps) && !br.Get<Mle::Mle>().IsRouter(); i++)
    {
        nexus.AdvanceTime(kWaitStepMs);
    }

    VerifyOrQuit(br.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(br.Get<Mle::Mle>().GetRloc16() == brRouterRloc16);

    nexus.AdvanceTime(kRoutePropagateTime);
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(serviceAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(dhcpAloc));
    VerifyOrQuit(br.Get<ThreadNetif>().HasUnicastAddress(ndAloc));

    nexus.SendAndVerifyEchoRequest(leader, serviceAloc);
    nexus.SendAndVerifyEchoRequest(leader, dhcpAloc);
    nexus.SendAndVerifyEchoRequest(leader, ndAloc);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAlocRestoredAfterRouterBriefDetach();
    printf("All tests passed\n");
    return 0;
}
