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

#include <initializer_list>
#include <stdio.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

struct LocateResult
{
    Error        mError;
    Ip6::Address mMeshLocalAddress;
    uint16_t     mRloc16;
    bool         mCallbackInvoked;

    void Clear(void)
    {
        mError = kErrorNone;
        mMeshLocalAddress.Clear();
        mRloc16          = 0;
        mCallbackInvoked = false;
    }
};

static void HandleLocateResult(void *aContext, otError aError, const otIp6Address *aAddress, uint16_t aRloc16)
{
    LocateResult *result = static_cast<LocateResult *>(aContext);

    result->mError           = aError;
    result->mCallbackInvoked = true;
    result->mRloc16          = aRloc16;

    if (aAddress != nullptr)
    {
        result->mMeshLocalAddress = AsCoreType(aAddress);
    }
}

static void Locate(Core &aNexus, Node &aNode, const Ip6::Address &aAnycastAddress, LocateResult &aResult)
{
    aResult.Clear();
    SuccessOrQuit(aNode.Get<AnycastLocator>().Locate(aAnycastAddress, HandleLocateResult, &aResult));

    for (uint32_t i = 0; i < 10000; i++)
    {
        aNexus.AdvanceTime(10);
        if (aResult.mCallbackInvoked)
        {
            break;
        }
    }

    VerifyOrQuit(aResult.mCallbackInvoked);
    SuccessOrQuit(aResult.mError);
}

void TestAnycastLocator(void)
{
    // Topology:
    //   LEADER -- ROUTER1 -- ROUTER2 -- ROUTER3 -- ROUTER4

    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &router4 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER1");
    router2.SetName("ROUTER2");
    router3.SetName("ROUTER3");
    router4.SetName("ROUTER4");

    Node *nodes[] = {&leader, &router1, &router2, &router3, &router4};

    nexus.AdvanceTime(0);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network with line topology");

    AllowLinkBetween(leader, router1);
    AllowLinkBetween(router1, router2);
    AllowLinkBetween(router2, router3);
    AllowLinkBetween(router3, router4);

    leader.Form();
    nexus.AdvanceTime(20 * 1000);

    for (uint8_t i = 1; i < GetArrayLength(nodes); i++)
    {
        nodes[i]->Join(*nodes[i - 1]);
        nexus.AdvanceTime(20 * 1000);
    }

    // Wait for all to become routers and network to stabilize
    nexus.AdvanceTime(300 * 1000);

    for (Node *node : nodes)
    {
        VerifyOrQuit(node->Get<Mle::Mle>().IsRouter() || node->Get<Mle::Mle>().IsLeader());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("1. Locate leader ALOC from all nodes");

    Ip6::Address leaderAloc;
    leader.Get<Mle::Mle>().GetLeaderAloc(leaderAloc);

    LocateResult result;

    for (Node *node : nodes)
    {
        Log("Node %s locating leader ALOC", node->GetName());
        Locate(nexus, *node, leaderAloc, result);
        VerifyOrQuit(result.mMeshLocalAddress == leader.Get<Mle::Mle>().GetMeshLocalEid());
        VerifyOrQuit(result.mRloc16 == leader.Get<Mle::Mle>().GetRloc16());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("2. Add a service on router4 and locate its ALOC");

    uint32_t                 enterpriseNumber = 44970;
    NetworkData::ServiceData serviceData;
    NetworkData::ServerData  serverData;
    uint8_t                  sData[] = {0x57, 0x12, 0x34};
    uint8_t                  rData[] = {0x00};

    serviceData.Init(sData, sizeof(sData));
    serverData.Init(rData, sizeof(rData));

    SuccessOrQuit(router4.Get<NetworkData::Local>().AddService(enterpriseNumber, serviceData, true, serverData));
    router4.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(20 * 1000);

    // Get the service ALOC from router4
    Ip6::Address serviceAloc;
    bool         found = false;

    for (const Ip6::Netif::UnicastAddress &addr : router4.Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (router4.Get<Mle::Mle>().IsAnycastLocator(addr.GetAddress()) &&
            addr.GetAddress().GetIid().IsAnycastLocator())
        {
            // The leader ALOC is also an ALOC, so we need to make sure it's not the leader ALOC.
            if (addr.GetAddress() != leaderAloc)
            {
                serviceAloc = addr.GetAddress();
                found       = true;
                break;
            }
        }
    }
    VerifyOrQuit(found);

    for (Node *node : nodes)
    {
        Log("Node %s locating service ALOC", node->GetName());
        Locate(nexus, *node, serviceAloc, result);
        VerifyOrQuit(result.mMeshLocalAddress == router4.Get<Mle::Mle>().GetMeshLocalEid());
        VerifyOrQuit(result.mRloc16 == router4.Get<Mle::Mle>().GetRloc16());
    }

    Log("---------------------------------------------------------------------------------------");
    Log("3. Add same service on leader and ensure we locate the closest ALOC destination");

    SuccessOrQuit(leader.Get<NetworkData::Local>().AddService(enterpriseNumber, serviceData, true, serverData));
    leader.Get<NetworkData::Notifier>().HandleServerDataUpdated();
    nexus.AdvanceTime(20 * 1000);

    // leader and router1 should locate leader as closest service ALOC
    for (Node *node : {&leader, &router1})
    {
        Log("Node %s locating service ALOC (expected leader)", node->GetName());
        Locate(nexus, *node, serviceAloc, result);
        VerifyOrQuit(result.mMeshLocalAddress == leader.Get<Mle::Mle>().GetMeshLocalEid());
        VerifyOrQuit(result.mRloc16 == leader.Get<Mle::Mle>().GetRloc16());
    }

    // router3 and router4 should locate router4
    for (Node *node : {&router3, &router4})
    {
        Log("Node %s locating service ALOC (expected router4)", node->GetName());
        Locate(nexus, *node, serviceAloc, result);
        VerifyOrQuit(result.mMeshLocalAddress == router4.Get<Mle::Mle>().GetMeshLocalEid());
        VerifyOrQuit(result.mRloc16 == router4.Get<Mle::Mle>().GetRloc16());
    }

    // router2 is in middle and can locate either one
    Log("Node %s locating service ALOC (expected either leader or router4)", router2.GetName());
    Locate(nexus, router2, serviceAloc, result);
    VerifyOrQuit((result.mMeshLocalAddress == leader.Get<Mle::Mle>().GetMeshLocalEid() &&
                  result.mRloc16 == leader.Get<Mle::Mle>().GetRloc16()) ||
                 (result.mMeshLocalAddress == router4.Get<Mle::Mle>().GetMeshLocalEid() &&
                  result.mRloc16 == router4.Get<Mle::Mle>().GetRloc16()));

    Log("All tests passed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAnycastLocator();
    return 0;
}
