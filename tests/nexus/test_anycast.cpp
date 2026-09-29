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
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

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
 * Time to advance for the network to stabilize after nodes have attached.
 */
static constexpr uint32_t kStabilizationTime = 60 * 1000;

static void AddPrefix(Node &aNode, const char *aPrefixString, const char *aFlags)
{
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mOnMesh = true;

    for (const char *f = aFlags; *f != '\0'; f++)
    {
        switch (*f)
        {
        case 'p':
            config.mPreferred = true;
            break;
        case 'a':
            config.mSlaac = true;
            break;
        case 'd':
            config.mDhcp = true;
            break;
        case 'c':
            config.mConfigure = true;
            break;
        case 'n':
            config.mNdDns = true;
            break;
        case 'r':
            config.mDefaultRoute = true;
            break;
        case 'o':
            config.mOnMesh = true;
            break;
        case 's':
            config.mStable = true;
            break;
        }
    }

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

static void RemovePrefix(Node &aNode, const char *aPrefixString)
{
    Ip6::Prefix prefix;

    SuccessOrQuit(prefix.FromString(aPrefixString));
    SuccessOrQuit(aNode.Get<NetworkData::Local>().RemoveOnMeshPrefix(prefix));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

static Ip6::Address GetAloc(Node &aNode)
{
    Ip6::Address aloc;

    aloc.Clear();

    for (const Ip6::Netif::UnicastAddress &addr : aNode.Get<Ip6::Netif>().GetUnicastAddresses())
    {
        if (addr.GetAddress().GetIid().IsAnycastLocator())
        {
            // We expect the prefix-based ALOC (DHCPv6 or ND Agent).
            // Leader ALOC is 0xfc00.
            if (addr.GetAddress().GetIid().GetLocator() != 0xfc00)
            {
                aloc = addr.GetAddress();
                break;
            }
        }
    }

    return aloc;
}

void TestAnycast(void)
{
    /**
     * Test Anycast Address Routing
     *
     * Topology:
     * R1 (Leader) -- R2 -- R3 -- R4 -- R5
     */

    Core nexus;

    Node &r1 = nexus.CreateNode();
    Node &r2 = nexus.CreateNode();
    Node &r3 = nexus.CreateNode();
    Node &r4 = nexus.CreateNode();
    Node &r5 = nexus.CreateNode();

    r1.SetName("R1");
    r2.SetName("R2");
    r3.SetName("R3");
    r4.SetName("R4");
    r5.SetName("R5");

    AllowLinkBetween(r1, r2);
    AllowLinkBetween(r2, r3);
    AllowLinkBetween(r3, r4);
    AllowLinkBetween(r4, r5);

    nexus.AdvanceTime(0);

    Log("Step 0: Form network");
    r1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(r1.Get<Mle::Mle>().IsLeader());

    r2.Join(r1);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(r2.Get<Mle::Mle>().IsRouter());

    r3.Join(r2);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(r3.Get<Mle::Mle>().IsRouter());

    r4.Join(r3);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(r4.Get<Mle::Mle>().IsRouter());

    r5.Join(r4);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(r5.Get<Mle::Mle>().IsRouter());

    const char *kPrefix      = "2001:dead:beef:cafe::/64";
    const char *kTestFlags[] = {"ds", "cs"};

    for (const char *flags : kTestFlags)
    {
        Log("---------------------------------------------------------------------------------------");
        Log("Testing anycast with flags: %s", flags);

        // 1. Add prefix on R2
        Log("Step 1: Add prefix on R2");
        AddPrefix(r2, kPrefix, flags);
        nexus.AdvanceTime(kStabilizationTime);

        Ip6::Address aloc2 = GetAloc(r2);
        VerifyOrQuit(!aloc2.IsUnspecified(), "R2 should have an ALOC");
        Log("R2 ALOC: %s", aloc2.ToString().AsCString());

        // 2. Ping ALOC from R3 -> should go to R2
        Log("Step 2: Ping ALOC from R3 (expected R2)");
        nexus.SendAndVerifyEchoRequest(r3, aloc2);

        // 3. Add same prefix on R5
        Log("Step 3: Add prefix on R5");
        AddPrefix(r5, kPrefix, flags);
        nexus.AdvanceTime(kStabilizationTime);

        Ip6::Address aloc5 = GetAloc(r5);
        VerifyOrQuit(!aloc5.IsUnspecified(), "R5 should have an ALOC");
        VerifyOrQuit(aloc5 == aloc2, "R5 should have the same ALOC as R2");
        Log("R5 ALOC: %s", aloc5.ToString().AsCString());

        // 4. Ping ALOC from R3 -> should go to R2 (closest)
        Log("Step 4: Ping ALOC from R3 (expected R2 - closest)");
        nexus.SendAndVerifyEchoRequest(r3, aloc2);

        // 5. Ping ALOC from R4 -> should go to R5 (closest)
        Log("Step 5: Ping ALOC from R4 (expected R5 - closest)");
        nexus.SendAndVerifyEchoRequest(r4, aloc2);

        // 6. Remove prefix on R2
        Log("Step 6: Remove prefix on R2");
        RemovePrefix(r2, kPrefix);
        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(GetAloc(r2).IsUnspecified(), "R2 should no longer have the ALOC");

        // 7. Ping ALOC from R3 -> should go to R5
        Log("Step 7: Ping ALOC from R3 (expected R5)");
        nexus.SendAndVerifyEchoRequest(r3, aloc2);

        // 8. Ping ALOC from R4 -> should go to R5
        Log("Step 8: Ping ALOC from R4 (expected R5)");
        nexus.SendAndVerifyEchoRequest(r4, aloc2);

        // 9. Remove prefix on R5
        Log("Step 9: Remove prefix on R5");
        RemovePrefix(r5, kPrefix);
        nexus.AdvanceTime(kStabilizationTime);
        VerifyOrQuit(GetAloc(r5).IsUnspecified(), "R5 should no longer have the ALOC");
    }

    nexus.SaveTestInfo("test_anycast.json");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAnycast();
    printf("All tests passed\n");
    return 0;
}
