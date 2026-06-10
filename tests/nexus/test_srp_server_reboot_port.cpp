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
 * Time to advance for a node to form a network and become leader, in milliseconds.
 */
static constexpr uint32_t kFormNetworkTime = 13 * 1000;

/**
 * Time to advance for a node to join as a child and upgrade to a router.
 */
static constexpr uint32_t kAttachToRouterTime = 200 * 1000;

/**
 * Time to advance for SRP server information to be updated in Network Data.
 */
static constexpr uint32_t kSrpServerInfoUpdateTime = 60 * 1000;

/**
 * Time to advance for SRP registration, in milliseconds.
 */
static constexpr uint32_t kSrpRegistrationTime = 60 * 1000;

/**
 * Number of reboot times.
 */
static constexpr uint16_t kRebootTimes = 25;

void TestSrpServerRebootPort(void)
{
    Core nexus;

    Node &client = nexus.CreateNode();
    Node &server = nexus.CreateNode();

    client.SetName("CLIENT");
    server.SetName("SERVER");

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    //
    // 0. Start the server & client devices.
    //

    Log("0. Start the server & client devices.");
    client.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);
    server.Get<Mle::Mle>().SetRouterUpgradeThreshold(2);

    client.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    server.Join(client);
    nexus.AdvanceTime(kAttachToRouterTime);
    VerifyOrQuit(server.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(server.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server.Get<Srp::Server>().SetEnabled(true);

    // Wait for SRP server to be running (published in network data)
    for (int i = 0; i < 10; i++)
    {
        nexus.AdvanceTime(10000);
        if (server.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning)
        {
            break;
        }
    }
    VerifyOrQuit(server.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    //
    // 1. Check auto start mode on client and check that server is used.
    //

    Log("1. Check auto start mode on client and check that server is used.");
    // We set a host name so the client has something to register and thus "runs".
    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("my-host"));
    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    nexus.AdvanceTime(kSrpRegistrationTime);

    VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() == server.Get<Mle::Mle>().GetMeshLocalEid());

    //
    // 2. Reboot the server without any service registered. The server should
    // switch to a new port after re-enabling.
    //

    Log("2. Reboot the server without any service registered.");
    {
        uint16_t oldPort = server.Get<Srp::Server>().GetPort();
        server.Get<Srp::Server>().SetEnabled(false);
        nexus.AdvanceTime(5000);
        server.Get<Srp::Server>().SetEnabled(true);
        nexus.AdvanceTime(kSrpServerInfoUpdateTime);
        VerifyOrQuit(server.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);
        VerifyOrQuit(server.Get<Srp::Server>().GetPort() != oldPort);
    }

    //
    // 3. Register a service
    //

    Log("3. Register a service");
    static Srp::Client::Service service;
    SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());

    memset(&service, 0, sizeof(service));
    service.mName         = "_ipps._tcp";
    service.mInstanceName = "my-service";
    service.mPort         = 12345;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(service));

    nexus.AdvanceTime(kSrpRegistrationTime);

    // Reboot the SRP server several times
    for (uint16_t i = 0; i < kRebootTimes; i++)
    {
        Log("Reboot iteration %u", i);

        //
        // 4. Disable server and check client is stopped/disabled.
        //
        uint16_t oldPort = server.Get<Srp::Server>().GetPort();
        server.Get<Srp::Server>().SetEnabled(false);
        nexus.AdvanceTime(5000);

        //
        // 5. Enable server and check client starts again. Verify that the
        // server is using a different port, and the service have been
        // re-registered.
        //
        server.Get<Srp::Server>().SetEnabled(true);
        nexus.AdvanceTime(kSrpServerInfoUpdateTime);
        nexus.AdvanceTime(kSrpRegistrationTime);

        VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);
        VerifyOrQuit(server.Get<Srp::Server>().GetNextHost(nullptr)->GetServices().GetHead()->GetPort() == 12345);
        VerifyOrQuit(server.Get<Srp::Server>().GetPort() != oldPort);
    }

    Log("All steps completed.");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpServerRebootPort();

    printf("All tests passed\n");
    return 0;
}
