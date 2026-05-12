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
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_publisher.hpp"
#include "thread/network_data_service.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime   = 20 * 1000;
static constexpr uint32_t kJoinNetworkTime   = 10 * 1000;
static constexpr uint32_t kNetDataUpdateTime = 55 * 1000;

static constexpr char kOnMeshPrefix[]  = "fd00:1234::/64";
static constexpr char kExternalRoute[] = "fd00:abce::/64";

static constexpr uint8_t kAnycastSeqNum = 4;

static const char *const  kDnsSrpAddrStr = "fd00::cdef";
static constexpr uint16_t kDnsSrpPort    = 49152;

static constexpr uint8_t kDesiredNumAnycast       = 8;
static constexpr uint8_t kDesiredNumUnicast       = 2;
static constexpr uint8_t kDesiredNumOnMeshPrefix  = 3;
static constexpr uint8_t kDesiredNumExternalRoute = 10;

static constexpr uint32_t kThreadEnterpriseNumber = 44970;
static constexpr uint8_t  kAnycastServiceNum      = 0x5c;
static constexpr uint8_t  kUnicastServiceNum      = 0x5d;

static uint8_t GetNumServices(Node &aNode)
{
    NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
    NetworkData::ServiceConfig serviceConfig;
    uint8_t                    num    = 0;
    const NetworkData::Leader &leader = aNode.Get<NetworkData::Leader>();

    while (leader.GetNext(iterator, serviceConfig) == kErrorNone)
    {
        num++;
    }

    return num;
}

static void VerifyAnycastService(const NetworkData::ServiceConfig &aService)
{
    VerifyOrQuit(aService.mEnterpriseNumber == kThreadEnterpriseNumber);
    VerifyOrQuit(aService.mServiceDataLength >= 2);
    VerifyOrQuit(aService.mServiceData[0] == kAnycastServiceNum);
    VerifyOrQuit(aService.mServiceData[1] == kAnycastSeqNum);
    VerifyOrQuit(aService.mServerConfig.mStable);
}

static void VerifyUnicastService(const NetworkData::ServiceConfig &aService)
{
    VerifyOrQuit(aService.mEnterpriseNumber == kThreadEnterpriseNumber);
    VerifyOrQuit(aService.mServiceDataLength >= 1);
    VerifyOrQuit(aService.mServiceData[0] == kUnicastServiceNum);
    VerifyOrQuit(aService.mServerConfig.mStable);
}

enum ServiceType : uint8_t
{
    kAsAnycastService,
    kAsUnicastService,
};

static void VerifyServices(Node &aNode, ServiceType aType)
{
    NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
    NetworkData::ServiceConfig serviceConfig;
    const NetworkData::Leader &leader = aNode.Get<NetworkData::Leader>();

    while (leader.GetNext(iterator, serviceConfig) == kErrorNone)
    {
        if (aType == kAsAnycastService)
        {
            VerifyAnycastService(serviceConfig);
        }
        else
        {
            VerifyUnicastService(serviceConfig);
        }
    }
}

static void CheckNumOfPrefixes(Node &aNode, uint8_t aNumLow, uint8_t aNumMed, uint8_t aNumHigh)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    const NetworkData::Leader      &leader   = aNode.Get<NetworkData::Leader>();
    uint8_t                         numTotal = 0;
    uint8_t                         numLow   = 0;
    uint8_t                         numMed   = 0;
    uint8_t                         numHigh  = 0;

    while (leader.GetNext(iterator, prefixConfig) == kErrorNone)
    {
        numTotal++;
        if (prefixConfig.mPreference == NetworkData::kRoutePreferenceLow)
        {
            numLow++;
        }
        else if (prefixConfig.mPreference == NetworkData::kRoutePreferenceMedium)
        {
            numMed++;
        }
        else if (prefixConfig.mPreference == NetworkData::kRoutePreferenceHigh)
        {
            numHigh++;
        }
    }

    VerifyOrQuit(numTotal ==
                 Min<uint16_t>(static_cast<uint16_t>(aNumHigh) + aNumMed + aNumLow, kDesiredNumOnMeshPrefix));
    VerifyOrQuit(numHigh == Min(aNumHigh, kDesiredNumOnMeshPrefix));
    VerifyOrQuit(numMed ==
                 Min(aNumMed, static_cast<uint8_t>(Max<int>(0, (int)kDesiredNumOnMeshPrefix - (int)aNumHigh))));
    VerifyOrQuit(numLow == Min(aNumLow, static_cast<uint8_t>(
                                            Max<int>(0, (int)kDesiredNumOnMeshPrefix - (int)aNumHigh - (int)numMed))));
}

static void CheckNumOfRoutes(Node &aNode, uint8_t aNumLow, uint8_t aNumMed, uint8_t aNumHigh)
{
    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::ExternalRouteConfig routeConfig;
    const NetworkData::Leader       &leader   = aNode.Get<NetworkData::Leader>();
    uint8_t                          numTotal = 0;
    uint8_t                          numLow   = 0;
    uint8_t                          numMed   = 0;
    uint8_t                          numHigh  = 0;

    while (leader.GetNext(iterator, routeConfig) == kErrorNone)
    {
        numTotal++;
        if (routeConfig.mPreference == NetworkData::kRoutePreferenceLow)
        {
            numLow++;
        }
        else if (routeConfig.mPreference == NetworkData::kRoutePreferenceMedium)
        {
            numMed++;
        }
        else if (routeConfig.mPreference == NetworkData::kRoutePreferenceHigh)
        {
            numHigh++;
        }
    }

    VerifyOrQuit(numTotal ==
                 Min<uint16_t>(static_cast<uint16_t>(aNumHigh) + aNumMed + aNumLow, kDesiredNumExternalRoute));
    VerifyOrQuit(numHigh == Min(aNumHigh, kDesiredNumExternalRoute));
    VerifyOrQuit(numMed ==
                 Min(aNumMed, static_cast<uint8_t>(Max<int>(0, (int)kDesiredNumExternalRoute - (int)aNumHigh))));
    VerifyOrQuit(numLow == Min(aNumLow, static_cast<uint8_t>(
                                            Max<int>(0, (int)kDesiredNumExternalRoute - (int)aNumHigh - (int)numMed))));
}

void Test_NetDataPublisher(void)
{
    Core nexus;

    Node &leader  = nexus.CreateNode();
    Node &router1 = nexus.CreateNode();
    Node &router2 = nexus.CreateNode();
    Node &router3 = nexus.CreateNode();
    Node &router4 = nexus.CreateNode();
    Node &router5 = nexus.CreateNode();
    Node &ed1     = nexus.CreateNode();
    Node &ed2     = nexus.CreateNode();
    Node &ed3     = nexus.CreateNode();
    Node &ed4     = nexus.CreateNode();
    Node &ed5     = nexus.CreateNode();

    leader.SetName("LEADER");
    router1.SetName("ROUTER1");
    router2.SetName("ROUTER2");
    router3.SetName("ROUTER3");
    router4.SetName("ROUTER4");
    router5.SetName("ROUTER5");
    ed1.SetName("END_DEV1");
    ed2.SetName("END_DEV2");
    ed3.SetName("END_DEV3");
    ed4.SetName("END_DEV4");
    ed5.SetName("END_DEV5");

    Node *routers[] = {&router1, &router2, &router3, &router4, &router5};
    Node *eds[]     = {&ed1, &ed2, &ed3, &ed4, &ed5};
    Node *nodes[]   = {&leader, &router1, &router2, &router3, &router4, &router5, &ed1, &ed2, &ed3, &ed4, &ed5};

    nexus.AdvanceTime(0);

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    for (Node *router : routers)
    {
        router->Join(leader);
        nexus.AdvanceTime(kJoinNetworkTime);
    }

    for (Node *ed : eds)
    {
        ed->Join(leader, Node::kAsFed);
        nexus.AdvanceTime(kJoinNetworkTime);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP anycast entries - equal version number");

    leader.Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);
    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);
    }
    nexus.AdvanceTime(kNetDataUpdateTime);

    VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(1 + GetArrayLength(routers), kDesiredNumAnycast));
    VerifyServices(leader, kAsAnycastService);

    for (Node *ed : eds)
    {
        ed->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);
    }
    nexus.AdvanceTime(kNetDataUpdateTime);

    VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(GetArrayLength(nodes), kDesiredNumAnycast));
    VerifyServices(leader, kAsAnycastService);

    uint8_t num = GetArrayLength(nodes);
    for (Node *node : nodes)
    {
        node->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumAnycast));
        VerifyServices(leader, kAsAnycastService);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP anycast entries - different version numbers");

    uint8_t version = 0;
    leader.Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, version);
    num = 1;

    for (Node *router : routers)
    {
        version++;
        router->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, version);
        num++;
    }
    nexus.AdvanceTime(kNetDataUpdateTime);

    VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumAnycast));
    VerifyServices(leader, kAsAnycastService);

    for (Node *ed : eds)
    {
        ed->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, version);
        num++;
        nexus.AdvanceTime(kNetDataUpdateTime);

        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumAnycast));
        VerifyServices(leader, kAsAnycastService);

        // Check that the entry from this ED is present by checking its RLOC16
        bool                       found    = false;
        NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
        NetworkData::ServiceConfig serviceConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, serviceConfig) == kErrorNone)
        {
            if (serviceConfig.mServerConfig.mRloc16 == ed->Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    for (Node *node : nodes)
    {
        node->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumAnycast));
        VerifyServices(leader, kAsAnycastService);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP service data unicast entries - equal version number");

    Ip6::Address dnsSrpAddr;
    SuccessOrQuit(dnsSrpAddr.FromString(kDnsSrpAddrStr));

    num = 0;
    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(dnsSrpAddr, kDnsSrpPort, 0);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    for (Node *router : routers)
    {
        router->Get<Srp::Server>().SetEnabled(true);
        nexus.AdvanceTime(kNetDataUpdateTime);
    }

    uint8_t runningCount = 0;
    uint8_t stoppedCount = 0;
    for (Node *router : routers)
    {
        if (router->Get<Srp::Server>().GetState() == Srp::Server::kStateRunning)
        {
            runningCount++;
        }
        else if (router->Get<Srp::Server>().GetState() == Srp::Server::kStateStopped)
        {
            stoppedCount++;
        }
    }
    VerifyOrQuit(runningCount == Min<uint16_t>(GetArrayLength(routers), kDesiredNumUnicast));
    VerifyOrQuit(stoppedCount ==
                 static_cast<uint8_t>(Max<int>(0, (int)GetArrayLength(routers) - (int)kDesiredNumUnicast)));

    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    for (Node *router : routers)
    {
        router->Get<Srp::Server>().SetEnabled(false);
        VerifyOrQuit(router->Get<Srp::Server>().GetState() == Srp::Server::kStateDisabled);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP service data unicast entries - different version numbers");

    num = 0;
    for (uint8_t i = 0; i < GetArrayLength(routers); i++)
    {
        routers[i]->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(dnsSrpAddr, kDnsSrpPort, num);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);

        // The most recent service should win as it uses a higher version number.
        bool                       found    = false;
        NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
        NetworkData::ServiceConfig serviceConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, serviceConfig) == kErrorNone)
        {
            if (serviceConfig.mServerConfig.mRloc16 == routers[i]->Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    for (int i = static_cast<int>(GetArrayLength(routers)) - 1; i >= 0; i--)
    {
        routers[i]->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP server data unicast entries - equal version number");

    num = 0;
    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(kDnsSrpPort, 0);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    for (Node *router : routers)
    {
        router->Get<Srp::Server>().SetEnabled(true);
        nexus.AdvanceTime(kNetDataUpdateTime);
    }
    runningCount = 0;
    stoppedCount = 0;
    for (Node *router : routers)
    {
        if (router->Get<Srp::Server>().GetState() == Srp::Server::kStateRunning)
        {
            runningCount++;
        }
        else if (router->Get<Srp::Server>().GetState() == Srp::Server::kStateStopped)
        {
            stoppedCount++;
        }
    }
    VerifyOrQuit(runningCount == Min<uint16_t>(GetArrayLength(routers), kDesiredNumUnicast));
    VerifyOrQuit(stoppedCount ==
                 static_cast<uint8_t>(Max<int>(0, (int)GetArrayLength(routers) - (int)kDesiredNumUnicast)));

    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    for (Node *router : routers)
    {
        router->Get<Srp::Server>().SetEnabled(false);
        VerifyOrQuit(router->Get<Srp::Server>().GetState() == Srp::Server::kStateDisabled);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP server data unicast entries - different version numbers");

    num = 0;
    for (uint8_t i = 0; i < GetArrayLength(routers); i++)
    {
        routers[i]->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(kDnsSrpPort, num);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);

        bool                       found    = false;
        NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
        NetworkData::ServiceConfig serviceConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, serviceConfig) == kErrorNone)
        {
            if (serviceConfig.mServerConfig.mRloc16 == routers[i]->Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    for (int i = static_cast<int>(GetArrayLength(routers)) - 1; i >= 0; i--)
    {
        routers[i]->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    num = 0;
    for (uint8_t i = 0; i < GetArrayLength(routers); i++)
    {
        routers[i]->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(kDnsSrpPort, 20 - num);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);

        // The service from first router should win as it uses the highest version number.
        bool                       found    = false;
        NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
        NetworkData::ServiceConfig serviceConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, serviceConfig) == kErrorNone)
        {
            if (serviceConfig.mServerConfig.mRloc16 == routers[0]->Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
        nexus.AdvanceTime(kNetDataUpdateTime);
        num--;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP server data unicast vs anycast");

    num = 0;
    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(kDnsSrpPort, 0);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
        VerifyServices(leader, kAsUnicastService);
    }

    leader.Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);

    // Verify that publishing an anycast entry will update the
    // limit for the server data unicast address entry and all are
    // removed quickly. We validate this by waiting for a short
    // time (5000 msec) after publishing anycast.
    nexus.AdvanceTime(5000);
    VerifyOrQuit(GetNumServices(leader) == 1);
    VerifyServices(leader, kAsAnycastService);

    // Removing the anycast entry will cause the lower priority
    // server data unicast entries to be added again.
    leader.Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    nexus.AdvanceTime(kNetDataUpdateTime);

    VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
    VerifyServices(leader, kAsUnicastService);

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP server data unicast vs service data unicast");

    leader.Get<NetworkData::Publisher>().PublishDnsSrpServiceUnicast(dnsSrpAddr, kDnsSrpPort, 0);
    nexus.AdvanceTime(kNetDataUpdateTime);
    VerifyOrQuit(GetNumServices(leader) == 1);
    VerifyServices(leader, kAsUnicastService);

    leader.Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    nexus.AdvanceTime(kNetDataUpdateTime);

    VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumUnicast));
    VerifyServices(leader, kAsUnicastService);

    for (Node *router : routers)
    {
        router->Get<NetworkData::Publisher>().UnpublishDnsSrpService();
    }
    nexus.AdvanceTime(kNetDataUpdateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("DNS/SRP entries: Verify publisher preference when removing entries");

    num                      = 0;
    Node *testRoutersNodes[] = {&leader, &router1, &router2};
    for (Node *node : testRoutersNodes)
    {
        node->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == num);
        VerifyServices(leader, kAsAnycastService);
    }
    for (Node *node : eds)
    {
        node->Get<NetworkData::Publisher>().PublishDnsSrpServiceAnycast(kAnycastSeqNum, 0);
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        VerifyOrQuit(GetNumServices(leader) == Min<uint16_t>(num, kDesiredNumAnycast));
        VerifyServices(leader, kAsAnycastService);
    }

    VerifyOrQuit(GetNumServices(leader) == kDesiredNumAnycast);

    for (Node *node : {&router3, &router4, &router5})
    {
        // Manually add service to router
        NetworkData::ServiceData serviceData;
        NetworkData::ServerData  serverData;
        uint8_t                  sData[] = {kAnycastServiceNum, kAnycastSeqNum};

        serviceData.Init(sData, sizeof(sData));
        serverData.Init(nullptr, 0);

        SuccessOrQuit(
            node->Get<NetworkData::Local>().AddService(kThreadEnterpriseNumber, serviceData, true, serverData));
        node->Get<NetworkData::Notifier>().HandleServerDataUpdated();
        nexus.AdvanceTime(kNetDataUpdateTime);

        VerifyOrQuit(GetNumServices(leader) == kDesiredNumAnycast);
        VerifyServices(leader, kAsAnycastService);
    }

    // Final check for all routers being present.
    {
        Node *allRouters[] = {&leader, &router1, &router2, &router3, &router4, &router5};
        for (Node *router : allRouters)
        {
            bool                       found    = false;
            NetworkData::Iterator      iterator = NetworkData::kIteratorInit;
            NetworkData::ServiceConfig serviceConfig;
            while (leader.Get<NetworkData::Leader>().GetNext(iterator, serviceConfig) == kErrorNone)
            {
                if (serviceConfig.mServerConfig.mRloc16 == router->Get<Mle::Mle>().GetRloc16())
                {
                    found = true;
                    break;
                }
            }
            VerifyOrQuit(found);
        }
    }

    nexus.AdvanceTime(kNetDataUpdateTime);

    Log("---------------------------------------------------------------------------------------");
    Log("On-mesh prefix");

    Ip6::Prefix onMeshPrefix;
    SuccessOrQuit(onMeshPrefix.FromString(kOnMeshPrefix));

    uint8_t numLow  = 0;
    uint8_t numMed  = 0;
    uint8_t numHigh = 0;

    for (Node *ed : eds)
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceLow;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(ed->Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
        nexus.AdvanceTime(kNetDataUpdateTime);
        numLow++;
        CheckNumOfPrefixes(leader, numLow, numMed, numHigh);
    }

    for (Node *router : routers)
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceMedium;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(
            router->Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
        nexus.AdvanceTime(kNetDataUpdateTime);
        numMed++;
        CheckNumOfPrefixes(leader, numLow, numMed, numHigh);
    }

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceHigh;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(
            leader.Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    numHigh++;
    CheckNumOfPrefixes(leader, numLow, numMed, numHigh);

    for (Node *router : routers)
    {
        SuccessOrQuit(router->Get<NetworkData::Publisher>().UnpublishPrefix(onMeshPrefix));
        nexus.AdvanceTime(kNetDataUpdateTime);
        numMed--;
        CheckNumOfPrefixes(leader, numLow, numMed, numHigh);
    }

    SuccessOrQuit(leader.Get<NetworkData::Publisher>().UnpublishPrefix(onMeshPrefix));
    nexus.AdvanceTime(kNetDataUpdateTime);
    numHigh--;
    CheckNumOfPrefixes(leader, numLow, numMed, numHigh);

    for (Node *ed : eds)
    {
        SuccessOrQuit(ed->Get<NetworkData::Publisher>().UnpublishPrefix(onMeshPrefix));
        nexus.AdvanceTime(kNetDataUpdateTime);
        numLow--;
        CheckNumOfPrefixes(leader, numLow, numMed, numHigh);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Verify prefix removal preference");

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceMedium;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(ed1.Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    CheckNumOfPrefixes(leader, 0, 1, 0);

    for (Node *router : routers)
    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceMedium;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(
            router->Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
        nexus.AdvanceTime(kNetDataUpdateTime);
    }
    CheckNumOfPrefixes(leader, 0, 1 + GetArrayLength(routers), 0);

    // Check ed1 is in the list
    {
        bool                            found    = false;
        NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
        NetworkData::OnMeshPrefixConfig prefixConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
        {
            if (prefixConfig.mRloc16 == ed1.Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = onMeshPrefix;
        config.mPreference = NetworkData::kRoutePreferenceHigh;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(
            leader.Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    CheckNumOfPrefixes(leader, 0, 1 + GetArrayLength(routers), 1);

    // Publish same prefix now with `high` preference on leader.
    // Since it is `high` preference, it is added to network data
    // which leads to total number of entries to go above the desired
    // number temporarily and trigger other nodes to try to remove
    // their entry. The entries from routers should be preferred over
    // the one from `end_dev1` so that is the one we expect to be
    // removed. We check that this is the case (i.e., the entry from
    // `end_dev1` is no longer present in network data).
    //
    // Check ed1 is NO LONGER in the list
    {
        bool                            found    = false;
        NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
        NetworkData::OnMeshPrefixConfig prefixConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
        {
            if (prefixConfig.mRloc16 == ed1.Get<Mle::Mle>().GetRloc16())
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(!found);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("External route");

    Ip6::Prefix externalRoute;
    SuccessOrQuit(externalRoute.FromString(kExternalRoute));

    num = 0;
    for (Node *node : nodes)
    {
        NetworkData::ExternalRouteConfig config;
        config.Clear();
        config.GetPrefix() = externalRoute;
        config.mPreference = NetworkData::kRoutePreferenceLow;
        config.mStable     = true;
        SuccessOrQuit(
            node->Get<NetworkData::Publisher>().PublishExternalRoute(config, NetworkData::Publisher::kFromUser));
        nexus.AdvanceTime(kNetDataUpdateTime);
        num++;
        CheckNumOfRoutes(leader, num, 0, 0);
    }

    {
        NetworkData::ExternalRouteConfig config;
        config.Clear();
        config.GetPrefix() = externalRoute;
        config.mPreference = NetworkData::kRoutePreferenceHigh;
        config.mStable     = true;
        SuccessOrQuit(
            leader.Get<NetworkData::Publisher>().PublishExternalRoute(config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    CheckNumOfRoutes(leader, num - 1, 0, 1);

    {
        Ip6::Prefix defaultRoute;
        SuccessOrQuit(defaultRoute.FromString("::/0"));
        NetworkData::ExternalRouteConfig config;
        config.Clear();
        config.GetPrefix() = defaultRoute;
        config.mPreference = NetworkData::kRoutePreferenceMedium;
        config.mStable     = true;
        SuccessOrQuit(leader.Get<NetworkData::Publisher>().ReplacePublishedExternalRoute(
            externalRoute, config, NetworkData::Publisher::kFromUser));
        nexus.AdvanceTime(kNetDataUpdateTime);

        bool                             found    = false;
        NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
        NetworkData::ExternalRouteConfig routeConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, routeConfig) == kErrorNone)
        {
            if (AsCoreType(&routeConfig.mPrefix) == defaultRoute)
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);

        // Replace it back
        config.GetPrefix() = externalRoute;
        config.mPreference = NetworkData::kRoutePreferenceHigh;
        SuccessOrQuit(leader.Get<NetworkData::Publisher>().ReplacePublishedExternalRoute(
            defaultRoute, config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    CheckNumOfRoutes(leader, num - 1, 0, 1);

    {
        NetworkData::OnMeshPrefixConfig config;
        config.Clear();
        config.GetPrefix() = externalRoute;
        config.mPreference = NetworkData::kRoutePreferenceLow;
        config.mSlaac      = true;
        config.mOnMesh     = true;
        config.mStable     = true;
        config.mPreferred  = true;
        SuccessOrQuit(
            leader.Get<NetworkData::Publisher>().PublishOnMeshPrefix(config, NetworkData::Publisher::kFromUser));
    }
    nexus.AdvanceTime(kNetDataUpdateTime);
    CheckNumOfRoutes(leader, num - 1, 0, 0);

    {
        bool                            found    = false;
        NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
        NetworkData::OnMeshPrefixConfig prefixConfig;
        while (leader.Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
        {
            if (AsCoreType(&prefixConfig.mPrefix) == externalRoute)
            {
                found = true;
                break;
            }
        }
        VerifyOrQuit(found);
    }

    Log("All steps completed.");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::Test_NetDataPublisher();
    printf("All tests passed\n");
    return 0;
}
