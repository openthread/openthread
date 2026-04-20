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

static const Srp::Server::Host *FindHost(Node &aServer, const char *aName)
{
    static constexpr uint16_t kNameStringSize = 400;

    const Srp::Server::Host *host = nullptr;
    String<kNameStringSize>  fullName;

    fullName.Append("%s.%s", aName, aServer.Get<Srp::Server>().GetDomain());

    while (true)
    {
        host = aServer.Get<Srp::Server>().GetNextHost(host);

        if (host == nullptr)
        {
            break;
        }

        if (StringMatch(host->GetFullName(), fullName.AsCString()))
        {
            break;
        }
    }

    return host;
}

void TestSrpClientRemoveHost(void)
{
    Core nexus;

    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("server");
    client.SetName("client");

    Log("Step 0: Start the server and client devices.");

    server.Form();
    nexus.AdvanceTime(13000);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    client.Join(server);
    nexus.AdvanceTime(200000);
    VerifyOrQuit(client.Get<Mle::Mle>().IsRouter());

    server.Get<Srp::Server>().SetEnabled(true);
    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    nexus.AdvanceTime(15000);

    Log("Step 1: Register a single service and verify that it worked.");

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("host"));

    Ip6::Address         address;
    Srp::Client::Service service;

    SuccessOrQuit(address.FromString("2001::1"));
    SuccessOrQuit(client.Get<Srp::Client>().SetHostAddresses(&address, 1));

    service.mName          = "_srv._udp";
    service.mInstanceName  = "ins";
    service.mSubTypeLabels = nullptr;
    service.mTxtEntries    = nullptr;
    service.mNumTxtEntries = 0;
    service.mPort          = 1977;
    service.mPriority      = 0;
    service.mWeight        = 0;
    service.mLease         = 0;
    service.mKeyLease      = 0;
    SuccessOrQuit(service.Init());
    SuccessOrQuit(client.Get<Srp::Client>().AddService(service));

    nexus.AdvanceTime(2000);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    const Srp::Server::Host *host = FindHost(server, "host");
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(!host->IsDeleted());

    uint8_t             numAddresses;
    const Ip6::Address *addresses = host->GetAddresses(numAddresses);
    VerifyOrQuit(numAddresses == 1);
    {
        Ip6::Address expectedAddress;
        SuccessOrQuit(expectedAddress.FromString("2001::1"));
        VerifyOrQuit(addresses[0] == expectedAddress);
    }

    VerifyOrQuit(!host->GetServices().IsEmpty());
    const Srp::Server::Service *serverService = host->GetServices().GetHead();
    VerifyOrQuit(!serverService->IsDeleted());
    VerifyOrQuit(serverService->GetPort() == 1977);

    Log("Step 2: Clear the info on client, and verify that it is still present on server.");

    client.Get<Srp::Client>().ClearHostAndServices();
    nexus.AdvanceTime(2000);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetName() == nullptr);
    VerifyOrQuit(FindHost(server, "host") != nullptr);

    Log("Step 3: Set host name and request 'host remove' requiring update to server.");

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("host"));
    SuccessOrQuit(
        client.Get<Srp::Client>().RemoveHostAndServices(/* aRemoveKeyLease */ false, /* aSendUnregToServer */ true));
    nexus.AdvanceTime(2000);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetName() == nullptr);

    host = FindHost(server, "host");
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(host->IsDeleted());
    VerifyOrQuit(!host->GetServices().IsEmpty());
    VerifyOrQuit(host->GetServices().GetHead()->IsDeleted());

    Log("Step 4: Request 'host remove' with `remove_key`.");

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("host"));
    SuccessOrQuit(
        client.Get<Srp::Client>().RemoveHostAndServices(/* aRemoveKeyLease */ true, /* aSendUnregToServer */ true));
    nexus.AdvanceTime(2000);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetName() == nullptr);
    VerifyOrQuit(FindHost(server, "host") == nullptr);

    Log("Test passed successfully");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpClientRemoveHost();
    printf("All tests passed\n");
    return 0;
}
