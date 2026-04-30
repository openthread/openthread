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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

namespace {

const uint16_t kNumSrpClients        = 25;
const uint16_t kInstanceNumPerClient = 20;
const uint16_t kServicePort          = 12345;
const char    *kServiceName          = "_srpclient._tcp";

struct ClientInfo
{
    Srp::Client::Service mServices[kInstanceNumPerClient];
    String<32>           mInstanceNames[kInstanceNumPerClient];
};

static ClientInfo sClientInfo[kNumSrpClients + 1];

/**
 * This function registers SRP services from a given node.
 *
 * @param[in] aNode      The node from which to register services.
 * @param[in] aClientId  The client ID to use for host and instance names.
 *
 */
void RegisterServices(Node &aNode, uint16_t aClientId)
{
    String<32> hostName;

    hostName.Append("client%u", aClientId);

    aNode.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(aNode.Get<Srp::Client>().SetHostName(hostName.AsCString()));
    SuccessOrQuit(aNode.Get<Srp::Client>().EnableAutoHostAddress());

    for (uint16_t j = 1; j <= kInstanceNumPerClient; j++)
    {
        String<32> &instanceName = sClientInfo[aClientId].mInstanceNames[j - 1];

        instanceName.Clear().Append("client%u_%u", aClientId, j);

        Srp::Client::Service &service = sClientInfo[aClientId].mServices[j - 1];

        // We use `memset` to zero-initialize the service object. This is
        // safer than `ClearAllBytes` as it correctly handles the
        // `LinkedListEntry` base class part.
        memset(&service, 0, sizeof(service));

        service.mName         = kServiceName;
        service.mInstanceName = instanceName.AsCString();
        service.mPort         = kServicePort;

        SuccessOrQuit(service.Init());

        Error error = aNode.Get<Srp::Client>().AddService(service);
        if (error != kErrorNone)
        {
            Log("AddService failed with error %s (%d) for client %u, instance %u", ErrorToString(error), error,
                aClientId, j);
        }
        SuccessOrQuit(error);

        if (j % 10 == 0 || j == kInstanceNumPerClient)
        {
            Core::Get().AdvanceTime(5 * 1000);
        }
    }
}

/**
 * This function verifies registered services on a given SRP client node.
 *
 * @param[in] aNode  The node to verify.
 *
 */
void VerifyClient(Node &aNode)
{
    const Srp::Client::Service *service = aNode.Get<Srp::Client>().GetServices().GetHead();
    uint16_t                    count   = 0;

    while (service != nullptr)
    {
        VerifyOrQuit(service->GetState() == Srp::Client::kRegistered);
        count++;
        service = service->GetNext();
    }

    VerifyOrQuit(count == kInstanceNumPerClient);
}

} // namespace

/**
 * This test verifies SRP function that SRP clients can register up to 500 services to one SRP server.
 *
 * Topology:
 *     LEADER (SRP server)
 *       |
 *       |
 *     ROUTER1 ... ROUTER13 (SRP clients)
 *       |            |
 *      FED14  ...   FED25  (SRP clients)
 *
 */
void TestSrpScale(void)
{
    const uint16_t kNumRouters = (kNumSrpClients / 2) + (kNumSrpClients % 2);
    const uint16_t kNumFeds    = kNumSrpClients - kNumRouters;

    Core  nexus;
    Node &leader = nexus.CreateNode();
    Node *routers[kNumRouters];
    Node *feds[kNumFeds];

    Log("Test SRP Scale: 500 services");

    leader.Form();
    nexus.AdvanceTime(15 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    leader.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(1000);

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        routers[i] = &nexus.CreateNode();
        routers[i]->SetName("router", i + 1);
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().SetRouterSelectionJitter(1) == kErrorNone);
        routers[i]->Join(leader);
    }

    // Wait for routers to become routers
    nexus.AdvanceTime(20 * 1000);

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        VerifyOrQuit(routers[i]->Get<Mle::Mle>().IsRouter());
    }

    for (uint16_t i = 0; i < kNumFeds; i++)
    {
        feds[i] = &nexus.CreateNode();
        feds[i]->SetName("fed", i + kNumRouters + 1);
        feds[i]->Join(*routers[i], Node::kAsFed);
    }

    nexus.AdvanceTime(10 * 1000);

    for (uint16_t i = 0; i < kNumFeds; i++)
    {
        VerifyOrQuit(feds[i]->Get<Mle::Mle>().IsChild());
    }

    Log("Registering services from routers...");
    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        RegisterServices(*routers[i], i + 1);
    }

    Log("Registering services from FEDs...");
    for (uint16_t i = 0; i < kNumFeds; i++)
    {
        RegisterServices(*feds[i], i + kNumRouters + 1);
    }

    // Give it some more time to settle
    nexus.AdvanceTime(20 * 1000);

    Log("Verifying registrations...");

    for (uint16_t i = 0; i < kNumRouters; i++)
    {
        VerifyClient(*routers[i]);
    }

    for (uint16_t i = 0; i < kNumFeds; i++)
    {
        VerifyClient(*feds[i]);
    }

    uint32_t                 totalServices = 0;
    const Srp::Server::Host *host          = nullptr;
    while ((host = leader.Get<Srp::Server>().GetNextHost(host)) != nullptr)
    {
        const Srp::Server::Service *service = nullptr;
        while ((service = host->GetNextService(service)) != nullptr)
        {
            if (!service->IsDeleted())
            {
                totalServices++;
            }
        }
    }

    Log("Total services registered on server: %u", totalServices);
    VerifyOrQuit(totalServices == kNumSrpClients * kInstanceNumPerClient);

    Log("Test passed!");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpScale();
    printf("All tests passed\n");
    return 0;
}
