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

#include "border_router/routing_manager.hpp"
#include "mac/data_poll_sender.hpp"
#include "net/nd6.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime    = 13 * 1000;
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;
static constexpr uint32_t kBrActionTime       = 20 * 1000;
static constexpr uint32_t kInfraIfIndex       = 1;

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

static bool FindOnMeshPrefixInNetData(Node &aNode, const Ip6::Prefix &aPrefix, NetworkData::OnMeshPrefixConfig &aConfig)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    bool                  found    = false;

    while (otNetDataGetNextOnMeshPrefix(&aNode.GetInstance(), &iterator, &aConfig) == OT_ERROR_NONE)
    {
        if (aConfig.GetPrefix() == aPrefix)
        {
            found = true;
            break;
        }
    }

    return found;
}

static void SendRaWithPrefixes(Node &aEthNode, const Ip6::Prefix *aPrefixes, uint16_t aCount, uint32_t aLifetime)
{
    for (int i = 0; i < 3; i++)
    {
        Ip6::Nd::RouterAdvert::TxMessage ra;
        Ip6::Nd::RouterAdvert::Header    header;
        Ip6::Nd::Icmp6Packet             packet;

        header.SetToDefault();
        header.SetRouterLifetime(1800);
        SuccessOrQuit(ra.Append(header));

        for (uint16_t j = 0; j < aCount; j++)
        {
            SuccessOrQuit(ra.AppendPrefixInfoOption(aPrefixes[j], aLifetime, aLifetime,
                                                    Ip6::Nd::PrefixInfoOption::kOnLinkFlag |
                                                        Ip6::Nd::PrefixInfoOption::kAutoConfigFlag));
        }

        ra.GetAsPacket(packet);
        aEthNode.mInfraIf.SendIcmp6Nd(Ip6::Address::GetLinkLocalAllNodesMulticast(), packet.GetBytes(),
                                      packet.GetLength());
    }
}

void TestAilPrefixCompression(void)
{
    Core nexus;

    Node &br1  = nexus.CreateNode();
    Node &ed1  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    ed1.SetName("ED_1");
    eth1.SetName("ETH_1");

    Ip6::Prefix prefix1;
    Ip6::Prefix prefix2;
    Ip6::Prefix prefix3;

    SuccessOrQuit(prefix1.FromString("2001:db8:1::/64"));
    SuccessOrQuit(prefix2.FromString("2001:db8:2::/64"));
    SuccessOrQuit(prefix3.FromString("2001:db8:3::/64"));

    AllowLinkBetween(br1, ed1);
    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("Step 1: Form Thread network with BR_1 as Leader and attach ED_1");
    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(br1.Get<Mle::Mle>().IsLeader());

    ed1.Join(br1, Node::kAsFtd);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(ed1.Get<Mle::Mle>().IsRouter());

    Log("Step 2: Enable Border Routing on BR_1 and initialize ETH_1 on infrastructure link");
    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));
    eth1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);

    nexus.AdvanceTime(kBrActionTime);

    Log("Step 3: ETH_1 advertises two on-link prefixes (prefix2: 2001:db8:2::/64 and prefix3: 2001:db8:3::/64)");
    {
        Ip6::Prefix prefixes[] = {prefix2, prefix3};
        SendRaWithPrefixes(eth1, prefixes, 2, 1800);
    }

    nexus.AdvanceTime(kBrActionTime);

    Log("Step 4: Verify prefix2 and prefix3 are published in Network Data with Context IDs");
    {
        NetworkData::OnMeshPrefixConfig config2;
        NetworkData::OnMeshPrefixConfig config3;

        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix2, config2));
        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix3, config3));

        VerifyOrQuit(config2.mStable);
        VerifyOrQuit(!config2.mOnMesh);
        VerifyOrQuit(!config2.mSlaac);

        VerifyOrQuit(config3.mStable);
        VerifyOrQuit(!config3.mOnMesh);
        VerifyOrQuit(!config3.mSlaac);

        // Verify ED_1 did NOT generate SLAAC addresses for these AIL prefixes
        VerifyOrQuit(!HasAddressWithPrefix(ed1, "2001:db8:2::/64"));
        VerifyOrQuit(!HasAddressWithPrefix(ed1, "2001:db8:3::/64"));

        // Verify Context IDs allocated on Leader for both prefixes
        Lowpan::Context context;
        Ip6::Address    testAddr2;
        Ip6::Address    testAddr3;

        SuccessOrQuit(testAddr2.FromString("2001:db8:2::100"));
        SuccessOrQuit(testAddr3.FromString("2001:db8:3::100"));

        br1.Get<NetworkData::Leader>().FindContextForAddress(testAddr2, context);
        VerifyOrQuit(context.IsValid());
        VerifyOrQuit(context.GetCompressFlag());
        VerifyOrQuit(context.GetContextId() > 0);

        br1.Get<NetworkData::Leader>().FindContextForAddress(testAddr3, context);
        VerifyOrQuit(context.IsValid());
        VerifyOrQuit(context.GetCompressFlag());
        VerifyOrQuit(context.GetContextId() > 0);

        // Verify ED_1 also resolved contexts from Network Data
        ed1.Get<NetworkData::Leader>().FindContextForAddress(testAddr2, context);
        VerifyOrQuit(context.IsValid());
        VerifyOrQuit(context.GetCompressFlag());
    }

    Log("Step 5: ETH_1 advertises 3 prefixes (prefix1, prefix2, prefix3). Top 2 lowest (prefix1, prefix2) must be "
        "published");
    {
        Ip6::Prefix prefixes[] = {prefix1, prefix2, prefix3};
        SendRaWithPrefixes(eth1, prefixes, 3, 1800);
    }

    nexus.AdvanceTime(kBrActionTime);

    {
        NetworkData::OnMeshPrefixConfig config;

        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix1, config));
        VerifyOrQuit(config.mStable);
        VerifyOrQuit(!config.mOnMesh);
        VerifyOrQuit(!config.mSlaac);

        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix2, config));
        VerifyOrQuit(!FindOnMeshPrefixInNetData(br1, prefix3, config)); // prefix3 was preempted

        // Verify 6LoWPAN compression for destination in prefix1 (AIL) vs uncompressed prefix
        Ip6::Header    ip6Header;
        Mac::Addresses macAddrs;
        uint8_t        frameBufferCompressed[128];
        uint8_t        frameBufferUncompressed[128];
        FrameBuilder   fbCompressed;
        FrameBuilder   fbUncompressed;

        macAddrs.mSource.SetShort(ed1.Get<Mle::Mle>().GetRloc16());
        macAddrs.mDestination.SetShort(br1.Get<Mle::Mle>().GetRloc16());

        fbCompressed.Init(frameBufferCompressed, sizeof(frameBufferCompressed));
        fbUncompressed.Init(frameBufferUncompressed, sizeof(frameBufferUncompressed));

        Message *msgCompressed = ed1.Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgCompressed != nullptr);
        ip6Header.InitVersionTrafficClassFlow();
        ip6Header.SetPayloadLength(0);
        ip6Header.SetNextHeader(Ip6::kProtoNone);
        ip6Header.SetHopLimit(64);
        ip6Header.SetSource(ed1.Get<Mle::Mle>().GetMeshLocalEid());
        SuccessOrQuit(ip6Header.GetDestination().FromString("2001:db8:1::100"));
        SuccessOrQuit(msgCompressed->Append(ip6Header));

        SuccessOrQuit(ed1.Get<Lowpan::Lowpan>().Compress(*msgCompressed, macAddrs, fbCompressed));

        Message *msgUncompressed = ed1.Get<MessagePool>().Allocate(Message::kTypeIp6);
        VerifyOrQuit(msgUncompressed != nullptr);
        SuccessOrQuit(ip6Header.GetDestination().FromString("2001:db8:99::100"));
        SuccessOrQuit(msgUncompressed->Append(ip6Header));

        SuccessOrQuit(ed1.Get<Lowpan::Lowpan>().Compress(*msgUncompressed, macAddrs, fbUncompressed));

        VerifyOrQuit(fbCompressed.GetLength() < fbUncompressed.GetLength());

        msgCompressed->Free();
        msgUncompressed->Free();
    }

    Log("Step 6: Deprecate prefix1 (send RA with lifetime=0). prefix3 must be restored");
    {
        Ip6::Prefix validPrefixes[] = {prefix2, prefix3};
        SendRaWithPrefixes(eth1, validPrefixes, 2, 1800);

        Ip6::Prefix deprecatedPrefixes[] = {prefix1};
        SendRaWithPrefixes(eth1, deprecatedPrefixes, 1, 0);
    }

    nexus.AdvanceTime(kBrActionTime);

    {
        NetworkData::OnMeshPrefixConfig config;

        VerifyOrQuit(!FindOnMeshPrefixInNetData(br1, prefix1, config));
        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix2, config));
        VerifyOrQuit(FindOnMeshPrefixInNetData(br1, prefix3, config));
    }

    Log("Step 7: Disable Border Routing on BR_1. All AIL prefixes must be unpublished");
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(false));
    nexus.AdvanceTime(kBrActionTime);

    {
        NetworkData::OnMeshPrefixConfig config;

        VerifyOrQuit(!FindOnMeshPrefixInNetData(br1, prefix1, config));
        VerifyOrQuit(!FindOnMeshPrefixInNetData(br1, prefix2, config));
        VerifyOrQuit(!FindOnMeshPrefixInNetData(br1, prefix3, config));
    }

    nexus.SaveTestInfo("test_ail_prefix_compression.json");
    Log("Test passed successfully");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestAilPrefixCompression();
    printf("All tests passed\n");
    return 0;
}
