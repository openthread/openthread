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

static void CheckTtl(Core &aNexus, Node &aServer, uint32_t aExpectedTtl)
{
    const Srp::Server::Host *host = nullptr;

    // We advance time enough to trigger a refresh from the client.
    // The lease interval is at most 240 seconds in our test cases.
    aNexus.AdvanceTime(300 * Time::kOneSecondInMsec);

    while ((host = aServer.Get<Srp::Server>().GetNextHost(host)) != nullptr)
    {
        if (StringMatch(host->GetFullName(), "my-host.default.service.arpa.", kStringCaseInsensitiveMatch))
        {
            break;
        }
    }

    VerifyOrQuit(host != nullptr);

    const Srp::Server::Service *serverService = host->GetNextService(nullptr);

    VerifyOrQuit(serverService != nullptr);
    Log("Expected TTL: %u, Actual TTL: %u", aExpectedTtl, serverService->GetTtl());
    VerifyOrQuit(serverService->GetTtl() == aExpectedTtl);
}

void TestSrpTtl(void)
{
    Core  nexus;
    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    Log("----------------------------------------------------------------------------------------------------------");
    Log("Testing SRP TTL");

    server.Form();
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    server.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    client.Join(server, Node::kAsFed);
    nexus.AdvanceTime(10 * Time::kOneSecondInMsec);
    VerifyOrQuit(client.Get<Mle::Mle>().IsChild());

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    nexus.AdvanceTime(1 * Time::kOneSecondInMsec);

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName("my-host"));

    Ip6::Address address;
    SuccessOrQuit(address.FromString("2001::1"));
    SuccessOrQuit(client.Get<Srp::Client>().SetHostAddresses(&address, 1));

    Srp::Client::Service service;
    ClearAllBytes(service);
    service.mName         = "_ipps._tcp";
    service.mInstanceName = "my-service";
    service.mPort         = 12345;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(service));

    // Case 1: CLIENT_TTL < TTL_MIN < LEASE_MAX ==> TTL_MIN
    Log("Case 1: CLIENT_TTL < TTL_MIN < LEASE_MAX ==> TTL_MIN");
    {
        Srp::Server::TtlConfig ttlConfig;
        ttlConfig.mMinTtl = 120;
        ttlConfig.mMaxTtl = 240;
        SuccessOrQuit(server.Get<Srp::Server>().SetTtlConfig(ttlConfig));

        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = 120;
        leaseConfig.mMaxLease    = 240;
        leaseConfig.mMinKeyLease = 240;
        leaseConfig.mMaxKeyLease = 240;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));

        client.Get<Srp::Client>().SetTtl(100);
        CheckTtl(nexus, server, 120);
    }

    // Case 2: TTL_MIN < CLIENT_TTL < TTL_MAX < LEASE_MAX ==> CLIENT_TTL
    Log("Case 2: TTL_MIN < CLIENT_TTL < TTL_MAX < LEASE_MAX ==> CLIENT_TTL");
    {
        Srp::Server::TtlConfig ttlConfig;
        ttlConfig.mMinTtl = 60;
        ttlConfig.mMaxTtl = 120;
        SuccessOrQuit(server.Get<Srp::Server>().SetTtlConfig(ttlConfig));

        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = 120;
        leaseConfig.mMaxLease    = 240;
        leaseConfig.mMinKeyLease = 240;
        leaseConfig.mMaxKeyLease = 240;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));

        client.Get<Srp::Client>().SetTtl(100);
        CheckTtl(nexus, server, 100);
    }

    // Case 3: TTL_MAX < LEASE_MAX < CLIENT_TTL ==> TTL_MAX
    Log("Case 3: TTL_MAX < LEASE_MAX < CLIENT_TTL ==> TTL_MAX");
    {
        Srp::Server::TtlConfig ttlConfig;
        ttlConfig.mMinTtl = 60;
        ttlConfig.mMaxTtl = 120;
        SuccessOrQuit(server.Get<Srp::Server>().SetTtlConfig(ttlConfig));

        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = 120;
        leaseConfig.mMaxLease    = 240;
        leaseConfig.mMinKeyLease = 240;
        leaseConfig.mMaxKeyLease = 240;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));

        client.Get<Srp::Client>().SetTtl(240);
        CheckTtl(nexus, server, 120);
    }

    // Case 4: LEASE_MAX < TTL_MAX < CLIENT_TTL ==> LEASE_MAX
    Log("Case 4: LEASE_MAX < TTL_MAX < CLIENT_TTL ==> LEASE_MAX");
    {
        Srp::Server::TtlConfig ttlConfig;
        ttlConfig.mMinTtl = 60;
        ttlConfig.mMaxTtl = 120;
        SuccessOrQuit(server.Get<Srp::Server>().SetTtlConfig(ttlConfig));

        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = 30;
        leaseConfig.mMaxLease    = 60;
        leaseConfig.mMinKeyLease = 240;
        leaseConfig.mMaxKeyLease = 240;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));

        client.Get<Srp::Client>().SetTtl(240);
        CheckTtl(nexus, server, 60);
    }

    Log("Test passed successfully");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestSrpTtl();
    printf("All tests passed\n");
    return 0;
}
