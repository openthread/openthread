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
 * Time to advance for a node to join a network, in milliseconds.
 */
static constexpr uint32_t kJoinNetworkTime = 10 * 1000;

/**
 * Time to advance for the network to stabilize, in milliseconds.
 */
static constexpr uint32_t kStabilizationTime = 15 * 1000;

/**
 * Default DNS server address for testing.
 */
static const char kDefaultAddress[] = "fd00:1:2:3:4:5:6:7";

void TestDnsClientConfigAutoStart(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    leader.SetName("LEADER");
    router.SetName("ROUTER");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    leader.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(kJoinNetworkTime + kStabilizationTime);
    VerifyOrQuit(router.Get<Mle::Mle>().IsRouter());

    Log("---------------------------------------------------------------------------------------");
    Log("On leader set DNS config explicitly");

    {
        Dns::Client::QueryConfig dnsConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        SuccessOrQuit(AsCoreType(&dnsConfig.mServerSockAddr.mAddress).FromString(kDefaultAddress));
        dnsConfig.mServerSockAddr.mPort = 53;
        leader.Get<Dns::Client>().SetDefaultConfig(dnsConfig);

        const Dns::Client::QueryConfig &currentConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        VerifyOrQuit(currentConfig.GetServerSockAddr().GetAddress() == AsCoreType(&dnsConfig.mServerSockAddr.mAddress));
        VerifyOrQuit(currentConfig.GetServerSockAddr().GetPort() == 53);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Start leader to act as SRP server");

    leader.Get<Srp::Server>().SetEnabled(true);

    Log("---------------------------------------------------------------------------------------");
    Log("Enable SRP client auto start on both nodes");

    leader.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    router.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Verify that on router the default DNS config is changed to the SRP server address");

    Ip6::Address srpServerAddress;

    srpServerAddress = router.Get<Srp::Client>().GetServerAddress().GetAddress();
    VerifyOrQuit(!srpServerAddress.IsUnspecified());
    VerifyOrQuit(srpServerAddress == leader.Get<Srp::Client>().GetServerAddress().GetAddress());

    {
        const Dns::Client::QueryConfig &dnsConfig = router.Get<Dns::Client>().GetDefaultConfig();
        VerifyOrQuit(dnsConfig.GetServerSockAddr().GetAddress() == srpServerAddress);
        VerifyOrQuit(dnsConfig.GetServerSockAddr().GetPort() == 53);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Verify that on leader the default DNS config remains as before");

    {
        const Dns::Client::QueryConfig &dnsConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        Ip6::Address                    expectedAddress;
        SuccessOrQuit(expectedAddress.FromString(kDefaultAddress));
        VerifyOrQuit(dnsConfig.GetServerSockAddr().GetAddress() == expectedAddress);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("On leader clear DNS config and verify it adopts the SRP server address");

    {
        Dns::Client::QueryConfig dnsConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        AsCoreType(&dnsConfig.mServerSockAddr.mAddress).Clear();
        leader.Get<Dns::Client>().SetDefaultConfig(dnsConfig);

        const Dns::Client::QueryConfig &currentConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        VerifyOrQuit(currentConfig.GetServerSockAddr().GetAddress() == srpServerAddress);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("On leader set DNS config explicitly again");

    {
        Dns::Client::QueryConfig dnsConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        SuccessOrQuit(AsCoreType(&dnsConfig.mServerSockAddr.mAddress).FromString(kDefaultAddress));
        leader.Get<Dns::Client>().SetDefaultConfig(dnsConfig);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Stop SRP server on leader and start it on router");

    leader.Get<Srp::Server>().SetEnabled(false);
    router.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Verify that SRP client on router switched to new SRP server");

    Ip6::Address newSrpServerAddress;
    newSrpServerAddress = router.Get<Srp::Client>().GetServerAddress().GetAddress();
    VerifyOrQuit(!newSrpServerAddress.IsUnspecified());
    VerifyOrQuit(newSrpServerAddress != srpServerAddress);
    VerifyOrQuit(newSrpServerAddress == leader.Get<Srp::Client>().GetServerAddress().GetAddress());

    Log("---------------------------------------------------------------------------------------");
    Log("Verify that config on router gets changed while on leader it remains unchanged");

    {
        const Dns::Client::QueryConfig &dnsConfig = router.Get<Dns::Client>().GetDefaultConfig();
        VerifyOrQuit(dnsConfig.GetServerSockAddr().GetAddress() == newSrpServerAddress);
    }

    {
        const Dns::Client::QueryConfig &dnsConfig = leader.Get<Dns::Client>().GetDefaultConfig();
        Ip6::Address                    expectedAddress;
        SuccessOrQuit(expectedAddress.FromString(kDefaultAddress));
        VerifyOrQuit(dnsConfig.GetServerSockAddr().GetAddress() == expectedAddress);
    }
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDnsClientConfigAutoStart();
    printf("All tests passed\n");
    return 0;
}
