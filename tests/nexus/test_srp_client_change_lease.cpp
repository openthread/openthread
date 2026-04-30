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
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * SRP Lease time in seconds.
 */
static constexpr uint32_t kLeaseTime    = 60;
static constexpr uint32_t kNewLeaseTime = 120;
static constexpr uint32_t kKeyLeaseTime = 240;
static constexpr uint32_t kNewTtl       = 10;

/**
 * SRP service and host names.
 */
static const char         kSrpServiceType[]  = "_ipps._tcp";
static const char         kSrpInstanceName[] = "my-service";
static const char         kSrpHostName[]     = "my-host";
static const char         kSrpHostAddress[]  = "2001::1";
static constexpr uint16_t kSrpServicePort    = 12345;

void TestSrpClientChangeLease(const char *aJsonFileName)
{
    Core                 nexus;
    Ip6::Address         srpHostAddress;
    Srp::Client::Service srpService;

    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SRP_SERVER");
    client.SetName("SRP_CLIENT");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Step 0: Start the server and client devices.");

    server.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    {
        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = kLeaseTime;
        leaseConfig.mMaxLease    = kLeaseTime;
        leaseConfig.mMinKeyLease = kKeyLeaseTime;
        leaseConfig.mMaxKeyLease = kKeyLeaseTime;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));
    }
    server.Get<Srp::Server>().SetEnabled(true);
    nexus.AdvanceTime(5 * 1000);

    client.Join(server, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("Step 1: Register a single service and verify that it works.");

    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    SuccessOrQuit(client.Get<Srp::Client>().SetHostName(kSrpHostName));

    SuccessOrQuit(srpHostAddress.FromString(kSrpHostAddress));
    SuccessOrQuit(client.Get<Srp::Client>().SetHostAddresses(&srpHostAddress, 1));

    ClearAllBytes(srpService);
    srpService.mName         = kSrpServiceType;
    srpService.mInstanceName = kSrpInstanceName;
    srpService.mPort         = kSrpServicePort;
    SuccessOrQuit(client.Get<Srp::Client>().AddService(srpService));

    nexus.AdvanceTime(2 * 1000);

    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("Step 2: Change server lease range and client lease interval.");
    {
        Srp::Server::LeaseConfig leaseConfig;
        leaseConfig.mMinLease    = kNewLeaseTime;
        leaseConfig.mMaxLease    = kNewLeaseTime;
        leaseConfig.mMinKeyLease = kKeyLeaseTime;
        leaseConfig.mMaxKeyLease = kKeyLeaseTime;
        SuccessOrQuit(server.Get<Srp::Server>().SetLeaseConfig(leaseConfig));
    }
    client.Get<Srp::Client>().SetLeaseInterval(kNewLeaseTime);
    nexus.AdvanceTime(kNewLeaseTime * 1000);
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("Step 3: Wait for KEY_LEASE_TIME * 2.");
    nexus.AdvanceTime(kKeyLeaseTime * 2 * 1000);
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("Step 4: Change client TTL.");
    client.Get<Srp::Client>().SetTtl(kNewTtl);
    nexus.AdvanceTime(kNewLeaseTime * 1000);
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    Log("Step 5: Set TTL to 0.");
    client.Get<Srp::Client>().SetTtl(0);
    nexus.AdvanceTime(kNewLeaseTime * 1000);
    VerifyOrQuit(client.Get<Srp::Client>().GetHostInfo().GetState() == Srp::Client::kRegistered);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::TestSrpClientChangeLease((argc > 2) ? argv[2] : "test_srp_client_change_lease.json");
    printf("All tests passed\n");
    return 0;
}
