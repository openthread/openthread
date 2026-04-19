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
static constexpr uint32_t kStabilizationTime = 10 * 1000;

/**
 * Time to wait for ICMPv6 Echo response.
 */
static constexpr uint32_t kEchoResponseWaitTime = 5 * 1000;

/**
 * ICMPv6 Echo Request payload size, in bytes.
 */
static constexpr uint16_t kEchoPayloadSize = 0;

/**
 * Add an on-mesh prefix to a node.
 */
static void AddPrefix(Node &aNode, const char *aPrefixString, bool aDefaultRoute)
{
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mOnMesh       = true;
    config.mStable       = true;
    config.mPreferred    = true;
    config.mSlaac        = true;
    config.mDefaultRoute = aDefaultRoute;

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddOnMeshPrefix(config));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

/**
 * Remove an on-mesh prefix from a node.
 */
static void RemovePrefix(Node &aNode, const char *aPrefixString)
{
    Ip6::Prefix prefix;
    SuccessOrQuit(prefix.FromString(aPrefixString));
    SuccessOrQuit(aNode.Get<NetworkData::Local>().RemoveOnMeshPrefix(prefix));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

/**
 * Add an external route to a node.
 */
static void AddExternalRoute(Node &aNode, const char *aPrefixString)
{
    NetworkData::ExternalRouteConfig config;

    config.Clear();
    SuccessOrQuit(config.GetPrefix().FromString(aPrefixString));
    config.mStable     = true;
    config.mPreference = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(aNode.Get<NetworkData::Local>().AddHasRoutePrefix(config));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

/**
 * Remove an external route from a node.
 */
static void RemoveExternalRoute(Node &aNode, const char *aPrefixString)
{
    Ip6::Prefix prefix;
    SuccessOrQuit(prefix.FromString(aPrefixString));
    SuccessOrQuit(aNode.Get<NetworkData::Local>().RemoveHasRoutePrefix(prefix));
    aNode.Get<NetworkData::Notifier>().HandleServerDataUpdated();
}

/**
 * Add a manual IPv6 address to a node.
 */
static void AddIpAddress(Node &aNode, otNetifAddress &aAddress, const char *aAddressString)
{
    ClearAllBytes(aAddress);
    SuccessOrQuit(AsCoreType(&aAddress.mAddress).FromString(aAddressString));
    aAddress.mPrefixLength  = 64;
    aAddress.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;
    aAddress.mPreferred     = true;
    aAddress.mValid         = true;

    SuccessOrQuit(otIp6AddUnicastAddress(&aNode.GetInstance(), &aAddress));
}

/**
 * Remove a manual IPv6 address from a node.
 */
static void RemoveIpAddress(Node &aNode, const char *aAddressString)
{
    Ip6::Address address;
    SuccessOrQuit(address.FromString(aAddressString));
    SuccessOrQuit(otIp6RemoveUnicastAddress(&aNode.GetInstance(), &address));
}

struct IcmpResponseContext
{
    IcmpResponseContext(uint16_t aIdentifier)
        : mIdentifier(aIdentifier)
        , mResponseReceived(false)
    {
    }

    uint16_t mIdentifier;
    bool     mResponseReceived;
};

static void HandleIcmpResponse(void *aContext, otMessage *, const otMessageInfo *, const otIcmp6Header *aIcmpHeader)
{
    IcmpResponseContext     *context = static_cast<IcmpResponseContext *>(aContext);
    const Ip6::Icmp::Header *header  = AsCoreTypePtr(aIcmpHeader);

    if ((header->GetType() == Ip6::Icmp::Header::kTypeEchoReply) && (header->GetId() == context->mIdentifier))
    {
        context->mResponseReceived = true;
    }
}

/**
 * Send an ICMPv6 Echo Request and verify that NO response is received.
 */
static void SendAndVerifyNoEchoResponse(Core               &aNexus,
                                        Node               &aSender,
                                        const Ip6::Address &aDestination,
                                        uint32_t            aWaitTime)
{
    static constexpr uint16_t kIdentifier = 0xabcd;

    IcmpResponseContext icmpContext(kIdentifier);
    Ip6::Icmp::Handler  icmpHandler(HandleIcmpResponse, &icmpContext);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().RegisterHandler(icmpHandler));

    aSender.SendEchoRequest(aDestination, kIdentifier);
    aNexus.AdvanceTime(aWaitTime);

    SuccessOrQuit(aSender.Get<Ip6::Icmp>().UnregisterHandler(icmpHandler));

    VerifyOrQuit(!icmpContext.mResponseReceived, "Received unexpected Echo Reply");
}

void TestZeroLenExternalRoute(void)
{
    Core           nexus;
    otNetifAddress router1Addr;
    otNetifAddress router2Addr;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER_1");
    router2.SetName("ROUTER_2");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Form network and join routers");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router1.Join(leader);
    router2.Join(leader);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(router1.Get<Mle::Mle>().IsRouter());
    VerifyOrQuit(router2.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Add prefix on ROUTER_2 and external route and address on ROUTER_1");

    // Add an on-mesh prefix with SLAAC on router2 (without default route flag).
    AddPrefix(router2, "fd00:1234::/64", false);

    // Add a zero length external route on router1. Also add an IPv6
    // address on router1.
    AddExternalRoute(router1, "::/0");
    AddIpAddress(router1, router1Addr, "fd00:abcd::1");

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Ping from LEADER to the address on ROUTER_1");

    // Ping from leader the address added on router1. The zero
    // length external route should ensure the message is routed
    // to router1.
    Ip6::Address destAddress;
    SuccessOrQuit(destAddress.FromString("fd00:abcd::1"));
    nexus.SendAndVerifyEchoRequest(leader, destAddress, kEchoPayloadSize, Ip6::kDefaultHopLimit, kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Change prefix on ROUTER_2 to have default route flag");

    // Change the on-mesh prefix on router2 to now also have
    // the default route flag.
    RemovePrefix(router2, "fd00:1234::/64");
    AddPrefix(router2, "fd00:5678::/64", true);

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Ping again from LEADER. External route ::/0 should still be preferred");

    // Again ping from leader the same address. The explicit
    // external route (even with zero length) on router1 should
    // still be preferred over the default route flag of the
    // on-mesh prefix from router2.
    nexus.SendAndVerifyEchoRequest(leader, destAddress, kEchoPayloadSize, Ip6::kDefaultHopLimit, kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Remove external route on ROUTER_1 and verify ping failure");

    // Remove the external route on router1.
    RemoveExternalRoute(router1, "::/0");

    nexus.AdvanceTime(kStabilizationTime);

    // Now the ping should fail since the message would be routed
    // to router2 (due to its on-mesh prefix with default route flag).
    SendAndVerifyNoEchoResponse(nexus, leader, destAddress, kEchoResponseWaitTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 7: Move address to ROUTER_2 and verify ping success");

    // Remove the address from router1 and add it on router2 and
    // ping it again from leader to verify that the message is
    // being routed to router2.
    RemoveIpAddress(router1, "fd00:abcd::1");
    AddIpAddress(router2, router2Addr, "fd00:abcd::1");

    nexus.AdvanceTime(kStabilizationTime);
    nexus.SendAndVerifyEchoRequest(leader, destAddress, kEchoPayloadSize, Ip6::kDefaultHopLimit, kEchoResponseWaitTime);

    printf("All tests passed\n");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestZeroLenExternalRoute();
    return 0;
}
