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

/**
 * Nexus test for SRP many services MTU check.
 *
 * Verifies that the SRP client handles cases where many services cause the SRP Update message to exceed the
 * IPv6 MTU size (1280 bytes). In such cases, the SRP client should register services by splitting them into
 * multiple update messages.
 *
 */
void TestSrpManyServicesMtuCheck(void)
{
    Core nexus;

    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SRP_SERVER");
    client.SetName("SRP_CLIENT");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Start the server & client devices.");

    server.Form();
    SuccessOrQuit(server.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(20 * 1000);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    client.Join(server, Node::kAsFed);
    nexus.AdvanceTime(20 * 1000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsAttached());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Register 15 services with long names and sub-types to hit MTU and trigger splitting.");

    const uint16_t kNumServices = 15;

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());
    SuccessOrQuit(
        client.Get<Srp::Client>().SetHostName("hosthosthosthosthosthosthosthosthosthosthosthosthosthosthosthos"));

    static Srp::Client::Service services[15];
    static String<64>           serviceNames[15];
    static const char *const    kSubTypeLabels[] = {"_sub1", nullptr};
    static const Dns::TxtEntry  kTxtEntries[]    = {
        Dns::TxtEntry("xyz", reinterpret_cast<const uint8_t *>("987654321"), 9),
        Dns::TxtEntry("uvw", reinterpret_cast<const uint8_t *>("abcdefghijklmnopqrstu"), 21),
        Dns::TxtEntry("qwp", reinterpret_cast<const uint8_t *>("564321abcdefghij"), 16),
    };

    for (uint16_t i = 0; i < kNumServices; i++)
    {
        serviceNames[i].Clear().AppendCharMultipleTimes('a' + i, 63);

        ClearAllBytes(services[i]);
        services[i].mName          = "_longlongsrvname._udp";
        services[i].mInstanceName  = serviceNames[i].AsCString();
        services[i].mSubTypeLabels = kSubTypeLabels;
        services[i].mTxtEntries    = kTxtEntries;
        services[i].mNumTxtEntries = GetArrayLength(kTxtEntries);
        services[i].mPort          = 1977 + i;

        SuccessOrQuit(client.Get<Srp::Client>().AddService(services[i]));
    }

    nexus.AdvanceTime(200 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Make sure all services are successfully registered.");

    Log("Client Running: %s", client.Get<Srp::Client>().IsRunning() ? "Yes" : "No");
    Log("Host state: %s", Srp::Client::ItemStateToString(client.Get<Srp::Client>().GetHostInfo().GetState()));

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    for (uint16_t i = 0; i < kNumServices; i++)
    {
        // Find the service in client and check state
        const Srp::Client::Service *service = client.Get<Srp::Client>().GetServices().GetHead();
        bool                        found   = false;
        while (service != nullptr)
        {
            if (StringMatch(service->mInstanceName, serviceNames[i].AsCString(), kStringCaseInsensitiveMatch))
            {
                VerifyOrQuit(service->GetState() == Srp::Client::kRegistered);
                found = true;
                break;
            }
            service = service->GetNext();
        }
        VerifyOrQuit(found);
    }

    {
        const Srp::Server::Host *host = server.Get<Srp::Server>().GetNextHost(nullptr);
        VerifyOrQuit(host != nullptr);

        uint16_t registeredCount = 0;
        for (const Srp::Server::Service *service = host->GetNextService(nullptr); service != nullptr;
             service                             = host->GetNextService(service))
        {
            if (!service->IsDeleted())
            {
                registeredCount++;
            }
        }
        VerifyOrQuit(registeredCount == kNumServices);
    }

    Log("Registration worked for 15 long services on both client and server (MTU exceeded).");

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Remove all 15 services.");

    for (uint16_t i = 0; i < kNumServices; i++)
    {
        SuccessOrQuit(client.Get<Srp::Client>().RemoveService(services[i]));
    }

    nexus.AdvanceTime(200 * 1000);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Verify services are removed.");

    VerifyOrQuit(client.Get<Srp::Client>().GetServices().IsEmpty());

    {
        const Srp::Server::Host *host = server.Get<Srp::Server>().GetNextHost(nullptr);
        VerifyOrQuit(host != nullptr);
        const Srp::Server::Service *service      = host->GetNextService(nullptr);
        uint16_t                    deletedCount = 0;
        while (service != nullptr)
        {
            VerifyOrQuit(service->IsDeleted());
            deletedCount++;
            service = host->GetNextService(service);
        }
        VerifyOrQuit(deletedCount == kNumServices);
    }

    Log("All services removed and marked as deleted on server.");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpManyServicesMtuCheck();
    printf("All tests passed\n");
    return 0;
}
