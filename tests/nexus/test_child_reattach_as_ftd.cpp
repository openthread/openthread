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

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kAttachAsChildTime  = 10 * 1000;
static constexpr uint32_t kStabilizationTime  = 10 * 1000;

static const Mle::DeviceMode kFedDeviceMode(Mle::DeviceMode::kModeRxOnWhenIdle |
                                            Mle::DeviceMode::kModeFullThreadDevice |
                                            Mle::DeviceMode::kModeFullNetworkData);

void TestChildReattachAsFtd(void)
{
    Core                       nexus;
    Node                      &leader = nexus.CreateNode();
    Node                      &router = nexus.CreateNode();
    Node                      &child  = nexus.CreateNode();
    Ip6::Netif::UnicastAddress unicastAddr;
    Ip6::Address               childMlEid;
    Ip6::Address               meshLocalAddr;
    Child                     *childEntry;

    leader.SetName("LEADER");
    router.SetName("ROUTER");
    child.SetName("CHILD");

    AllowLinkBetween(leader, router);
    AllowLinkBetween(leader, child);

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 1: Form network with LEADER and ROUTER, and publish on-mesh prefix");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    {
        NetworkData::OnMeshPrefixConfig config;

        config.Clear();
        SuccessOrQuit(config.GetPrefix().FromString("fd00:db8:1::/64"));
        config.mPreferred = true;
        config.mOnMesh    = true;
        config.mStable    = true;
        SuccessOrQuit(leader.Get<NetworkData::Local>().AddOnMeshPrefix(config));
        leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    router.Join(leader, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("Step 2: CHILD adds on-mesh address and attaches to LEADER as MED");

    unicastAddr.InitAsSlaacOrigin(/* aPrefixLength */ 64, /* aPreferred */ true);
    SuccessOrQuit(unicastAddr.GetAddress().FromString("fd00:db8:1::1234"));
    SuccessOrQuit(child.Get<ThreadNetif>().AddExternalUnicastAddress(unicastAddr));

    child.Join(leader, Node::kAsMed);
    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(!child.Get<Mle::Mle>().IsFullThreadDevice());

    childMlEid = child.Get<Mle::Mle>().GetMeshLocalEid();

    childEntry = leader.Get<ChildTable>().FindChild(child.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(childEntry != nullptr);
    VerifyOrQuit(!childEntry->IsFullThreadDevice());
    VerifyOrQuit(childEntry->HasIp6Address(unicastAddr.GetAddress()));
    VerifyOrQuit(childEntry->HasIp6Address(childMlEid));

    nexus.SendAndVerifyEchoRequest(leader, unicastAddr.GetAddress());

    Log("Step 3: CHILD detaches, removes on-mesh address, and reattaches to LEADER as FED");

    SuccessOrQuit(child.Get<Mle::Mle>().BecomeDetached());
    SuccessOrQuit(child.Get<ThreadNetif>().RemoveExternalUnicastAddress(unicastAddr.GetAddress()));
    SuccessOrQuit(child.Get<Mle::Mle>().SetDeviceMode(kFedDeviceMode));
    SuccessOrQuit(child.Get<Mle::Mle>().SetRouterEligible(false));

    nexus.AdvanceTime(kAttachAsChildTime);
    VerifyOrQuit(child.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(child.Get<Mle::Mle>().IsFullThreadDevice());

    childEntry = leader.Get<ChildTable>().FindChild(child.Get<Mac::Mac>().GetExtAddress(), Child::kInStateValid);
    VerifyOrQuit(childEntry != nullptr);
    VerifyOrQuit(childEntry->IsFullThreadDevice());
    VerifyOrQuit(!childEntry->HasIp6Address(unicastAddr.GetAddress()));
    VerifyOrQuit(!childEntry->HasIp6Address(childMlEid));
    VerifyOrQuit(childEntry->GetIp6Addresses().IsEmpty());
    VerifyOrQuit(childEntry->GetMeshLocalIp6Address(meshLocalAddr) == kErrorNotFound);

    Log("Step 4: ROUTER adds the on-mesh address and LEADER sends traffic to it");

    SuccessOrQuit(router.Get<ThreadNetif>().AddExternalUnicastAddress(unicastAddr));
    nexus.AdvanceTime(kStabilizationTime);

    nexus.SendAndVerifyEchoRequest(leader, unicastAddr.GetAddress());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestChildReattachAsFtd();
    printf("All tests passed\n");
    return 0;
}
