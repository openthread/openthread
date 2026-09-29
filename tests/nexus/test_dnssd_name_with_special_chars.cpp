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

#include "net/dnssd_server.hpp"
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
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 10 * 1000;

/**
 * Time to advance for DNS query processing.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * SRP service and host names.
 */
static const char         kSrpServiceType[]      = "_srv._udp";
static const char         kSrpFullServiceType[]  = "_srv._udp.default.service.arpa.";
static const char         kSrpHostName[]         = "host1";
static const char         kSpecialInstanceName[] = "O\\T 网关";
static constexpr uint16_t kSrpServicePort        = 1977;

/**
 * Test context to track progress.
 */
struct TestContext
{
    uint8_t mBrowseCount;
    uint8_t mResolveCount;
};

void HandleResolveResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    TestContext             *context = static_cast<TestContext *>(aContext);
    Dns::Client::ServiceInfo serviceInfo;
    Dns::Name::Buffer        hostName;

    if (aError != kErrorNone)
    {
        Log("HandleResolveResponse error: %s", ErrorToString(aError));
    }
    SuccessOrQuit(aError);

    {
        const Dns::Client::ServiceResponse &response = AsCoreType(aResponse);

        ClearAllBytes(serviceInfo);
        serviceInfo.mHostNameBuffer     = hostName;
        serviceInfo.mHostNameBufferSize = sizeof(hostName);

        SuccessOrQuit(response.GetServiceInfo(serviceInfo));

        VerifyOrQuit(serviceInfo.mPort == kSrpServicePort);
        VerifyOrQuit(StringMatch(hostName, "host1.default.service.arpa."));
    }

    context->mResolveCount++;
}

void HandleBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    TestContext           *context = static_cast<TestContext *>(aContext);
    Dns::Name::LabelBuffer label;

    if (aError != kErrorNone)
    {
        Log("HandleBrowseResponse error: %s", ErrorToString(aError));
    }
    SuccessOrQuit(aError);

    {
        const Dns::Client::BrowseResponse &response = AsCoreType(aResponse);

        for (uint16_t index = 0; response.GetServiceInstance(index, label, sizeof(label)) == kErrorNone; index++)
        {
            Log("Found service instance %s", label);
            if (StringMatch(label, kSpecialInstanceName))
            {
                context->mBrowseCount++;
            }
        }
    }
}

/**
 * Verifies SRP/DNS client/server support for Service Instance names with special and unicode characters.
 *
 * Topology:
 *   SERVER (SRP server)
 *     |
 *   CLIENT (SRP client)
 *
 */
void TestDnssdNameWithSpecialChars(void)
{
    Core  nexus;
    Node &server = nexus.CreateNode();
    Node &client = nexus.CreateNode();

    server.SetName("SERVER");
    client.SetName("CLIENT");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("Forming network");
    server.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(server.Get<Mle::Mle>().IsLeader());

    client.Join(server);
    nexus.AdvanceTime(kJoinNetworkTime);
    VerifyOrQuit(client.Get<Mle::Mle>().IsFullThreadDevice());

    SuccessOrQuit(server.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    server.Get<Srp::Server>().SetEnabled(true);
    SuccessOrQuit(server.Get<Dns::ServiceDiscovery::Server>().Start());
    client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    nexus.AdvanceTime(kStabilizationTime);

    Log("Registering service with special characters: %s", kSpecialInstanceName);
    SuccessOrQuit(client.Get<Srp::Client>().SetHostName(kSrpHostName));
    SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());

    Srp::Client::Service service;
    ClearAllBytes(service);
    service.mName         = kSrpServiceType;
    service.mInstanceName = kSpecialInstanceName;
    service.mPort         = kSrpServicePort;
    SuccessOrQuit(service.Init());
    SuccessOrQuit(client.Get<Srp::Client>().AddService(service));

    nexus.AdvanceTime(kSrpRegistrationTime);

    {
        Srp::Client::ItemState state = client.Get<Srp::Client>().GetHostInfo().GetState();
        Log("SRP Host State: %d", state);
        VerifyOrQuit(state == Srp::Client::kRegistered);
    }

    Log("Checking service discovery via DNS browse");
    TestContext context;
    context.mBrowseCount  = 0;
    context.mResolveCount = 0;

    Dns::Client::QueryConfig queryConfig = client.Get<Dns::Client>().GetDefaultConfig();
    AsCoreType(&queryConfig.mServerSockAddr).SetAddress(server.Get<Mle::Mle>().GetMeshLocalEid());
    queryConfig.mServerSockAddr.mPort = 53;

    SuccessOrQuit(client.Get<Dns::Client>().Browse(kSrpFullServiceType, HandleBrowseResponse, &context, &queryConfig));
    nexus.AdvanceTime(kDnsQueryTime);

    VerifyOrQuit(context.mBrowseCount == 1);

    Log("Checking service discovery via DNS resolve");
    SuccessOrQuit(client.Get<Dns::Client>().ResolveService(kSpecialInstanceName, kSrpFullServiceType,
                                                           HandleResolveResponse, &context, &queryConfig));
    nexus.AdvanceTime(kDnsQueryTime);

    VerifyOrQuit(context.mResolveCount == 1);

    Log("Checking service discovery via DNS resolve (case-insensitive)");
    // Note: DNS is generally case-insensitive for ASCII.
    // Unicode case-insensitivity might be more complex, but let's see if it works.
    SuccessOrQuit(client.Get<Dns::Client>().ResolveService("o\\t 网关", kSrpFullServiceType, HandleResolveResponse,
                                                           &context, &queryConfig));
    nexus.AdvanceTime(kDnsQueryTime);

    VerifyOrQuit(context.mResolveCount == 2);
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestDnssdNameWithSpecialChars();
    printf("All tests passed\n");
    return 0;
}
