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
 * Time to advance for SRP server information to be updated in Network Data.
 */
static constexpr uint32_t kSrpServerInfoUpdateTime = 20 * 1000;

/**
 * Time to advance for SRP registration, in milliseconds.
 */
static constexpr uint32_t kSrpRegistrationTime = 15 * 1000;

/**
 * Time to advance for DNS query processing, in milliseconds.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * SRP service registration parameters.
 */
static const char         kSrpServiceType[]     = "_srv._udp";
static const char         kSrpInstanceName[]    = "ins1";
static const char         kSrpHostName[]        = "host";
static const char         kSrpFullServiceType[] = "_srv._udp.default.service.arpa.";
static constexpr uint16_t kSrpServicePort       = 1313;

static constexpr uint8_t  kSrpServerAnycastSeqNum = 17;
static constexpr uint16_t kSrpServerAnycastPort   = 53;

/**
 * Test context for DNS browse resolution.
 */
struct BrowseContext
{
    Node *mClientNode;
    bool  mDone;
};

static void HandleBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    BrowseContext                     *context  = static_cast<BrowseContext *>(aContext);
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);
    char                               label[Dns::Name::kMaxLabelSize];
    Dns::Client::ServiceInfo           serviceInfo;
    char                               hostName[Dns::Name::kMaxNameSize];

    SuccessOrQuit(aError);

    // Since there is only one match the server should include the service info in additional section.
    SuccessOrQuit(response.GetServiceInstance(0, label, sizeof(label)));
    VerifyOrQuit(StringMatch(label, kSrpInstanceName, kStringCaseInsensitiveMatch));

    serviceInfo.mHostNameBuffer     = hostName;
    serviceInfo.mHostNameBufferSize = sizeof(hostName);
    serviceInfo.mTxtData            = nullptr;
    serviceInfo.mTxtDataSize        = 0;

    SuccessOrQuit(response.GetServiceInfo(label, serviceInfo));

    VerifyOrQuit(serviceInfo.mPort == kSrpServicePort);
    VerifyOrQuit(StringStartsWith(serviceInfo.mHostNameBuffer, kSrpHostName, kStringCaseInsensitiveMatch));
    VerifyOrQuit(AsCoreType(&serviceInfo.mHostAddress) == context->mClientNode->Get<Mle::Mle>().GetMeshLocalEid());

    context->mDone = true;
}

void TestSrpServerAnycastMode(const char *aJsonFileName)
{
    // This test covers the SRP server's behavior using different address
    // modes (unicast or anycast). The address mode indicates how the SRP
    // server determines its address and port and how this info is
    // published in the Thread Network Data. When using anycast address
    // mode, the SRP server and DNS server/resolver will both listen on port
    // 53 and they re-use the same socket. This test verifies the behavior of
    // both modules in such a situation.

    Core nexus;

    Node &server  = nexus.CreateNode();
    Node &client  = nexus.CreateNode();
    Node &browser = nexus.CreateNode();

    server.SetName("SERVER");
    client.SetName("CLIENT");
    browser.SetName("BROWSER");

    server.Form();
    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Test SRP Server Anycast and Unicast modes");

    nexus.AdvanceTime(kFormNetworkTime);

    client.Join(server);
    browser.Join(server);
    nexus.AdvanceTime(kJoinNetworkTime);

    const Srp::Server::AddressMode kAddressModes[] = {Srp::Server::kAddressModeAnycast,
                                                      Srp::Server::kAddressModeUnicast};

    for (Srp::Server::AddressMode addrMode : kAddressModes)
    {
        Log("Address Mode: %s", (addrMode == Srp::Server::kAddressModeAnycast) ? "anycast" : "unicast");

        // 1. Set the SRP server address mode and start the SRP server.
        SuccessOrQuit(server.Get<Srp::Server>().SetAddressMode(addrMode));

        if (addrMode == Srp::Server::kAddressModeAnycast)
        {
            SuccessOrQuit(server.Get<Srp::Server>().SetAnycastModeSequenceNumber(kSrpServerAnycastSeqNum));
            VerifyOrQuit(server.Get<Srp::Server>().GetAnycastModeSequenceNumber() == kSrpServerAnycastSeqNum);
        }

        server.Get<Srp::Server>().SetEnabled(true);
        nexus.AdvanceTime(kSrpServerInfoUpdateTime);

        // Verify that the SRP server information is correctly published in Network Data.
        {
            NetworkData::Service::Iterator iterator(client.Get<Instance>());

            if (addrMode == Srp::Server::kAddressModeAnycast)
            {
                NetworkData::Service::DnsSrpAnycastInfo anycastInfo;

                SuccessOrQuit(iterator.GetNextDnsSrpAnycastInfo(anycastInfo));
                VerifyOrQuit(anycastInfo.mSequenceNumber == kSrpServerAnycastSeqNum);
                VerifyOrQuit(iterator.GetNextDnsSrpAnycastInfo(anycastInfo) == kErrorNotFound);
            }
            else
            {
                NetworkData::Service::DnsSrpUnicastInfo unicastInfo;

                SuccessOrQuit(iterator.GetNextDnsSrpUnicastInfo(NetworkData::Service::kAddrInServerData, unicastInfo));
                VerifyOrQuit(unicastInfo.mSockAddr.GetAddress() == server.Get<Mle::Mle>().GetMeshLocalEid());
                VerifyOrQuit(unicastInfo.mSockAddr.GetPort() == server.Get<Srp::Server>().GetPort());
                VerifyOrQuit(iterator.GetNextDnsSrpUnicastInfo(NetworkData::Service::kAddrInServerData, unicastInfo) ==
                             kErrorNotFound);
            }
        }

        // 2. Enable auto-start on SRP client and browser.
        client.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        browser.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
        nexus.AdvanceTime(kSrpRegistrationTime);

        // 3. Verify client finds the server and uses proper address/port.
        if (addrMode == Srp::Server::kAddressModeAnycast)
        {
            // In anycast mode, the client should find the Anycast ALOC address and port 53.
            VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == kSrpServerAnycastPort);
            const Ip6::Address &serverAddr = client.Get<Srp::Client>().GetServerAddress().GetAddress();
            VerifyOrQuit(serverAddr.GetIid().IsAnycastServiceLocator());
        }
        else
        {
            // In unicast mode, it uses ML-EID and a dynamic port.
            VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetAddress() ==
                         server.Get<Mle::Mle>().GetMeshLocalEid());
            VerifyOrQuit(client.Get<Srp::Client>().GetServerAddress().GetPort() == server.Get<Srp::Server>().GetPort());
        }

        // 4. Add a service on SRP client and verify registration.
        SuccessOrQuit(client.Get<Srp::Client>().SetHostName(kSrpHostName));
        SuccessOrQuit(client.Get<Srp::Client>().EnableAutoHostAddress());

        Srp::Client::Service service;
        ClearAllBytes(service);
        service.mName         = kSrpServiceType;
        service.mInstanceName = kSrpInstanceName;
        service.mPort         = kSrpServicePort;
        SuccessOrQuit(client.Get<Srp::Client>().AddService(service));

        nexus.AdvanceTime(kSrpRegistrationTime);

        // Check if service is registered on client side.
        VerifyOrQuit(service.GetState() == Srp::Client::kRegistered);

        // 5. Browse for the service from the browser node and verify result.
        BrowseContext context;
        context.mClientNode = &client;
        context.mDone       = false;

        SuccessOrQuit(browser.Get<Dns::Client>().Browse(kSrpFullServiceType, HandleBrowseResponse, &context));
        nexus.AdvanceTime(kDnsQueryTime);
        VerifyOrQuit(context.mDone);

        // 6. Clear host on client and stop both client and server.
        client.Get<Srp::Client>().ClearHostAndServices();
        client.Get<Srp::Client>().Stop();
        server.Get<Srp::Server>().SetEnabled(false);
        nexus.AdvanceTime(kSrpServerInfoUpdateTime);
    }

    Log("All steps completed.");
    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::TestSrpServerAnycastMode((argc > 2) ? argv[2] : "test_srp_server_anycast_mode.json");

    printf("All tests passed\n");
    return 0;
}
