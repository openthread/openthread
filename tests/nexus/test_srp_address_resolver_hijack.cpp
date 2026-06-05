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

static constexpr uint32_t kFormNetworkTime  = 10 * Time::kOneSecondInMsec;
static constexpr uint32_t kRouterAttachTime = 200 * Time::kOneSecondInMsec;
static constexpr uint32_t kChildAttachTime  = 20 * Time::kOneSecondInMsec;
static constexpr uint32_t kSrpServerTime    = 30 * Time::kOneSecondInMsec;
static constexpr uint32_t kSrpUpdateTime    = 10 * Time::kOneSecondInMsec;
static constexpr uint32_t kEchoTimeout      = 5 * Time::kOneSecondInMsec;

static constexpr char     kHostName[]      = "srp-host";
static constexpr char     kServiceName[]   = "_ipps._tcp";
static constexpr char     kInstanceName[]  = "srp-service";
static constexpr uint16_t kServicePort     = 12345;

static const Srp::Server::Host *FindHost(Node &aServer, const char *aName)
{
    static constexpr uint16_t kNameStringSize = 400;

    const Srp::Server::Host *host = nullptr;
    String<kNameStringSize>  fullName;

    fullName.Append("%s.%s", aName, aServer.Get<Srp::Server>().GetDomain());

    while ((host = aServer.Get<Srp::Server>().GetNextHost(host)) != nullptr)
    {
        if (StringMatch(host->GetFullName(), fullName.AsCString()))
        {
            break;
        }
    }

    return host;
}

static bool HostHasAddress(const Srp::Server::Host &aHost, const Ip6::Address &aAddress)
{
    bool                hasAddress = false;
    const Ip6::Address *addresses;
    uint8_t             numAddresses;

    addresses = aHost.GetAddresses(numAddresses);

    for (uint8_t index = 0; index < numAddresses; index++)
    {
        if (addresses[index] == aAddress)
        {
            ExitNow(hasAddress = true);
        }
    }

exit:
    return hasAddress;
}

static const char *CacheStateToString(otCacheEntryState aState)
{
    const char *state;

    switch (aState)
    {
    case AddressResolver::EntryInfo::kStateCached:
        state = "cached";
        break;

    case AddressResolver::EntryInfo::kStateSnooped:
        state = "snooped";
        break;

    case AddressResolver::EntryInfo::kStateQuery:
        state = "query";
        break;

    case AddressResolver::EntryInfo::kStateRetryQuery:
        state = "retry-query";
        break;

    default:
        state = "unknown";
        break;
    }

    return state;
}

static bool FindCacheEntry(Node &aNode, const Ip6::Address &aTarget, AddressResolver::EntryInfo &aEntryInfo)
{
    bool                      found = false;
    AddressResolver::Iterator iterator;

    ClearAllBytes(iterator);
    ClearAllBytes(aEntryInfo);

    while (aNode.Get<AddressResolver>().GetNextCacheEntry(aEntryInfo, iterator) == kErrorNone)
    {
        if (AsCoreType(&aEntryInfo.mTarget) == aTarget)
        {
            ExitNow(found = true);
        }
    }

exit:
    return found;
}

static void AdvanceUntilClientRunning(Core &aNexus, Node &aClient)
{
    bool isRunning = false;

    for (uint8_t attempt = 0; attempt < 30; attempt++)
    {
        if (aClient.Get<Srp::Client>().IsRunning())
        {
            ExitNow(isRunning = true);
        }

        aNexus.AdvanceTime(Time::kOneSecondInMsec);
    }

exit:
    VerifyOrQuit(isRunning, "SRP client failed to start");
}

static void AdvanceUntilHostRegistered(Core &aNexus, Node &aClient)
{
    bool isRegistered = false;

    for (uint8_t attempt = 0; attempt < 30; attempt++)
    {
        if (aClient.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered)
        {
            ExitNow(isRegistered = true);
        }

        aNexus.AdvanceTime(Time::kOneSecondInMsec);
    }

exit:
    VerifyOrQuit(isRegistered, "SRP host update did not complete");
}

static void PrepareService(Srp::Client::Service &aService)
{
    ClearAllBytes(aService);
    aService.mName         = kServiceName;
    aService.mInstanceName = kInstanceName;
    aService.mPort         = kServicePort;
}

static void PrintScenarioResult(const char *aScenarioName,
                                uint16_t    aLeaderRloc16,
                                uint16_t    aVictimRloc16,
                                uint16_t    aAttackerRloc16,
                                uint16_t    aCacheBefore,
                                uint16_t    aCacheAfter,
                                const char *aCacheState,
                                uint16_t    aLeaderNextHop,
                                bool        aHostRegistered,
                                bool        aClaimedVictimEid,
                                bool        aUnauthorizedChange,
                                bool        aRuntimeConsumer)
{
    printf("%s: leader-rloc=0x%04x victim-rloc=0x%04x attacker-rloc=0x%04x cache-before=0x%04x "
           "cache-after=0x%04x cache-state=%s leader-next-hop=0x%04x host-registered=%u "
           "claimed-victim-eid=%u unauthorized-change=%u runtime-consumer=%u verdict=%s\n",
           aScenarioName, aLeaderRloc16, aVictimRloc16, aAttackerRloc16, aCacheBefore, aCacheAfter, aCacheState,
           aLeaderNextHop, aHostRegistered, aClaimedVictimEid, aUnauthorizedChange, aRuntimeConsumer,
           aUnauthorizedChange ? "BUG_PRESENT" : "SAFE_CONTROL");
}

void TestSrpAddressResolverHijack(void)
{
    Core                    nexus;
    Node                   &leader   = nexus.CreateNode();
    Node                   &victim   = nexus.CreateNode();
    Node                   &attacker = nexus.CreateNode();
    Srp::Client::Service    service;
    const Srp::Server::Host *host;
    const Ip6::Address     &victimEid   = victim.Get<Mle::Mle>().GetMeshLocalEid();
    const Ip6::Address     &attackerEid = attacker.Get<Mle::Mle>().GetMeshLocalEid();
    AddressResolver::EntryInfo cacheEntry;
    uint16_t                 leaderRloc16;
    uint16_t                 victimRloc16;
    uint16_t                 attackerRloc16;
    uint16_t                 cacheBefore;
    uint16_t                 cacheAfter;
    uint16_t                 leaderNextHop;
    bool                     hostRegistered;
    bool                     claimedVictimEid;
    bool                     unauthorizedChange;

    nexus.AdvanceTime(0);
    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    leader.SetName("leader");
    victim.SetName("victim");
    attacker.SetName("attacker");

    AllowLinkBetween(leader, victim);
    AllowLinkBetween(leader, attacker);
    UnallowLinkBetween(victim, attacker);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    SuccessOrQuit(leader.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    leader.Get<Srp::Server>().SetEnabled(true);

    victim.Join(leader);
    nexus.AdvanceTime(kRouterAttachTime);
    VerifyOrQuit(victim.Get<Mle::Mle>().IsRouter());

    attacker.Join(leader, Node::kAsFed);
    nexus.AdvanceTime(kChildAttachTime);
    VerifyOrQuit(attacker.Get<Mle::Mle>().IsChild());
    VerifyOrQuit(attacker.Get<Mle::Mle>().GetParent().GetExtAddress() == leader.Get<Mac::Mac>().GetExtAddress(),
                 "attacker did not attach directly to leader");

    nexus.AdvanceTime(kSrpServerTime);
    VerifyOrQuit(leader.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    attacker.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    AdvanceUntilClientRunning(nexus, attacker);

    leaderRloc16   = leader.Get<Mle::Mle>().GetRloc16();
    victimRloc16   = victim.Get<Mle::Mle>().GetRloc16();
    attackerRloc16 = attacker.Get<Mle::Mle>().GetRloc16();

    nexus.SendAndVerifyEchoRequest(leader, victimEid, 0, 64, kEchoTimeout);

    cacheBefore = leader.Get<AddressResolver>().LookUp(victimEid);
    VerifyOrQuit(cacheBefore == victimRloc16, "victim EID did not resolve to victim RLOC");
    VerifyOrQuit(FindCacheEntry(leader, victimEid, cacheEntry), "victim EID cache entry not found");

    SuccessOrQuit(attacker.Get<Srp::Client>().SetHostName(kHostName));
    SuccessOrQuit(attacker.Get<Srp::Client>().SetHostAddresses(&attackerEid, 1));
    PrepareService(service);
    SuccessOrQuit(attacker.Get<Srp::Client>().AddService(service));
    nexus.AdvanceTime(kSrpUpdateTime);
    AdvanceUntilHostRegistered(nexus, attacker);

    host = FindHost(leader, kHostName);
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(!host->IsDeleted());

    cacheAfter       = leader.Get<AddressResolver>().LookUp(victimEid);
    leaderNextHop    = leader.Get<RouterTable>().GetNextHop(cacheAfter);
    hostRegistered   = (host != nullptr) && !host->IsDeleted();
    claimedVictimEid = HostHasAddress(*host, victimEid);

    PrintScenarioResult("own-address-control", leaderRloc16, victimRloc16, attackerRloc16, cacheBefore, cacheAfter,
                        CacheStateToString(cacheEntry.mState), leaderNextHop, hostRegistered, claimedVictimEid,
                        false, leaderNextHop != victimRloc16);

    VerifyOrQuit(cacheAfter == victimRloc16, "own-address control changed victim cache entry");
    VerifyOrQuit(leaderNextHop == victimRloc16, "own-address control changed victim next hop");
    VerifyOrQuit(!claimedVictimEid, "own-address control unexpectedly claimed victim EID");

    SuccessOrQuit(attacker.Get<Srp::Client>().SetHostAddresses(&victimEid, 1));
    nexus.AdvanceTime(kSrpUpdateTime);
    AdvanceUntilHostRegistered(nexus, attacker);

    host = FindHost(leader, kHostName);
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(!host->IsDeleted());

    cacheAfter       = leader.Get<AddressResolver>().LookUp(victimEid);
    VerifyOrQuit(FindCacheEntry(leader, victimEid, cacheEntry), "victim EID cache entry missing after update");
    leaderNextHop    = leader.Get<RouterTable>().GetNextHop(cacheAfter);
    hostRegistered   = (host != nullptr) && !host->IsDeleted();
    claimedVictimEid = HostHasAddress(*host, victimEid);
    unauthorizedChange = (cacheAfter != victimRloc16) || (leaderNextHop != victimRloc16);

    PrintScenarioResult("victim-eid-hijack", leaderRloc16, victimRloc16, attackerRloc16, cacheBefore, cacheAfter,
                        CacheStateToString(cacheEntry.mState), leaderNextHop, hostRegistered, claimedVictimEid,
                        unauthorizedChange, leaderNextHop != victimRloc16);

    if (unauthorizedChange)
    {
        nexus.SendAndVerifyNoEchoResponse(leader, victimEid, 0, 64, kEchoTimeout);
    }
    else
    {
        nexus.SendAndVerifyEchoRequest(leader, victimEid, 0, 64, kEchoTimeout);
    }

    VerifyOrQuit(hostRegistered, "attacker host registration was lost");
    VerifyOrQuit(claimedVictimEid, "server did not accept the host AAAA update");
    VerifyOrQuit(!unauthorizedChange, "SRP host AAAA update overwrote victim EID routing");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpAddressResolverHijack();
    printf("All tests passed\n");
    return 0;
}
