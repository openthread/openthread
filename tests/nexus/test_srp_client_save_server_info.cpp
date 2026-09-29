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

/**
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for the network to stabilize.
 */
static constexpr uint32_t kWaitTime = 16 * 1000;

/**
 * SRP service port.
 */
static constexpr uint16_t kSrpServicePort = 12345;

void TestSrpClientSaveServerInfo(const char *aJsonFileName)
{
    Core nexus;

    Node &client  = nexus.CreateNode();
    Node &server1 = nexus.CreateNode();
    Node &server2 = nexus.CreateNode();
    Node &server3 = nexus.CreateNode();

    client.SetName("CLIENT");
    server1.SetName("SERVER1");
    server2.SetName("SERVER2");
    server3.SetName("SERVER3");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Start the server & client devices.");

    client.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(client.Get<Mle::Mle>().IsLeader());

    AllowLinkBetween(client, server1);
    AllowLinkBetween(client, server2);
    AllowLinkBetween(client, server3);

    server1.Join(client);
    server2.Join(client);
    server3.Join(client);
    nexus.AdvanceTime(Time::SecToMsec(200));

    VerifyOrQuit(server1.Get<Mle::Mle>().IsRouterOrLeader());
    VerifyOrQuit(server2.Get<Mle::Mle>().IsRouterOrLeader());
    VerifyOrQuit(server3.Get<Mle::Mle>().IsRouterOrLeader());

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Enable auto start mode on client and check that server1 is used.");

    SuccessOrQuit(server1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kWaitTime);

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("host"));
    SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());
    {
        Srp::Client::Service service;
        ClearAllBytes(service);
        service.mName         = "_ipps._tcp";
        service.mInstanceName = "my-service";
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(client.Get<Srp::Client>().AddService(service));
    }

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(client.Get<Srp::Client>().IsAutoStartModeEnabled());

    nexus.AdvanceTime(kWaitTime);

    VerifyOrQuit(client.Get<Srp::Client>().IsRunning());
    VerifyOrQuit(
        server1.Get<ThreadNetif>().HasUnicastAddress(client.Get<Srp::Client>().GetServerAddress().GetAddress()));
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Enable server2 and server3 and check that server1 is still used.");

    SuccessOrQuit(server2.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server2.Get<Srp::Server>().SetEnabled(true);
    SuccessOrQuit(server3.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server3.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kWaitTime);
    VerifyOrQuit(
        server1.Get<ThreadNetif>().HasUnicastAddress(client.Get<Srp::Client>().GetServerAddress().GetAddress()));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Stop and restart the client (multiple times) and verify that server1 is always picked.");

    for (int iter = 0; iter < 5; iter++)
    {
        client.Get<Srp::Client>().Stop();
        client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        nexus.AdvanceTime(kWaitTime);
        VerifyOrQuit(
            server1.Get<ThreadNetif>().HasUnicastAddress(client.Get<Srp::Client>().GetServerAddress().GetAddress()));
        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Stop the client, then stop server1 and restart client and verify that server1 is no longer used.");

    client.Get<Srp::Client>().Stop();
    server1.Get<Srp::Server>().SetEnabled(false);
    nexus.AdvanceTime(kWaitTime);

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    nexus.AdvanceTime(kWaitTime);

    Ip6::Address serverAddress = client.Get<Srp::Client>().GetServerAddress().GetAddress();
    VerifyOrQuit(!server1.Get<ThreadNetif>().HasUnicastAddress(serverAddress));
    VerifyOrQuit(server2.Get<ThreadNetif>().HasUnicastAddress(serverAddress) ||
                 server3.Get<ThreadNetif>().HasUnicastAddress(serverAddress));
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Start back server1, then restart client and verify that now we remain with the new saved server "
        "info.");

    server1.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(kWaitTime);

    for (int iter = 0; iter < 5; iter++)
    {
        client.Get<Srp::Client>().Stop();
        nexus.AdvanceTime(kWaitTime);
        client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        nexus.AdvanceTime(kWaitTime);
        VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == serverAddress);
        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);
    }

    Log("---------------------------------------------------------------------------------------");

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::TestSrpClientSaveServerInfo((argc > 2) ? argv[2] : "test_srp_client_save_server_info.json");
    printf("All tests passed\n");
    return 0;
}
