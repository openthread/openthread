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
 * Time to wait for SRP registration to complete.
 */
static constexpr uint32_t kSrpRegistrationTime = 10 * 1000;

/**
 * Time to advance for DNS query processing.
 */
static constexpr uint32_t kDnsQueryTime = 5 * 1000;

/**
 * Infrastructure interface index.
 */
static constexpr uint32_t kInfraIfIndex = 1;

/**
 * SRP Lease time in seconds.
 */
static constexpr uint32_t kSrpLease5m = 300;

/**
 * SRP Key Lease time in seconds.
 */
static constexpr uint32_t kSrpKeyLease10m = 600;

/**
 * SRP service ports.
 */
static constexpr uint16_t kSrpServicePort1 = 55551;
static constexpr uint16_t kSrpServicePort2 = 55552;
static constexpr uint16_t kSrpServicePort3 = 55553;
static constexpr uint16_t kSrpServicePort4 = 55554;
static constexpr uint16_t kSrpServicePort5 = 55555;

/**
 * SRP service and host names.
 */
static const char kSrpServiceType[]     = "_thread-test._udp";
static const char kSrpFullServiceType[] = "_thread-test._udp.default.service.arpa.";
static const char kSrpHostName[]        = "host-test-2";

/**
 * Expected values for "thread-test" key in TXT records.
 */
static const char *const kExpectedTxtValues[] = {
    "a49", "b50_test", "", "1", "C51",
};

/**
 * Test context to track progress.
 */
struct TestContext
{
    Node   *mNode;
    uint8_t mBrowseCount;
    uint8_t mResolveCount;
    uint8_t mUdpCount;
};

void HandleResolveResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    TestContext                        *context  = static_cast<TestContext *>(aContext);
    const Dns::Client::ServiceResponse &response = *static_cast<const Dns::Client::ServiceResponse *>(aResponse);
    Dns::Client::ServiceInfo            serviceInfo;
    Ip6::Address                        address;
    uint32_t                            ttl;
    Dns::Name::Buffer                   hostName;
    static constexpr uint16_t           kMaxTxtDataSize = 254;
    uint8_t                             txtData[kMaxTxtDataSize];
    Ip6::MessageInfo                    messageInfo;
    Message                            *message;

    if (aError != OT_ERROR_NONE)
    {
        Log("HandleResolveResponse error: %s", ErrorToString(aError));
        SuccessOrQuit(aError);
    }

    serviceInfo.mHostNameBuffer     = hostName;
    serviceInfo.mHostNameBufferSize = sizeof(hostName);
    serviceInfo.mTxtData            = txtData;
    serviceInfo.mTxtDataSize        = sizeof(txtData);

    SuccessOrQuit(response.GetServiceInfo(serviceInfo));

    context->mResolveCount++;

    /**
     * Step 7
     * - Device: TD_1 (DUT)
     * - Description (SRPC-3.5): Harness receives each of the N service responses via API from the DUT Component.
     *   For each service, it uses the API to extract the TXT record key "thread-test" and compares against the
     *   expected value.
     * - Pass Criteria:
     *   - The DUT MUST return the correct values for the "thread-test" key as defined below for each of the
     *     respective N services:
     *     - 1. a49
     *     - 2. b50 test
     *     - 3. (empty string of 0 characters)
     *     - 4. 1
     *     - 5. C51
     */
    Log("Step 7: Verify TXT record key 'thread-test' for resolved service on port %u", serviceInfo.mPort);

    Dns::TxtEntry::Iterator iterator;
    Dns::TxtEntry           entry;
    bool                    found = false;

    iterator.Init(serviceInfo.mTxtData, serviceInfo.mTxtDataSize);

    while (iterator.GetNextEntry(entry) == kErrorNone)
    {
        if (entry.MatchesKey("thread-test"))
        {
            uint8_t index = 0;

            switch (serviceInfo.mPort)
            {
            case kSrpServicePort1:
                index = 0;
                break;
            case kSrpServicePort2:
                index = 1;
                break;
            case kSrpServicePort3:
                index = 2;
                break;
            case kSrpServicePort4:
                index = 3;
                break;
            case kSrpServicePort5:
                index = 4;
                break;
            default:
                VerifyOrQuit(false, "Unknown port");
                break;
            }

            if (entry.mValueLength == strlen(kExpectedTxtValues[index]))
            {
                if (entry.mValueLength == 0 || memcmp(entry.mValue, kExpectedTxtValues[index], entry.mValueLength) == 0)
                {
                    found = true;
                    break;
                }
            }
        }
    }
    VerifyOrQuit(found, "thread-test key not found or value mismatch");

    /**
     * Step 8
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.5): Harness uses API to resolve for each of the N services the IPv6 address and port of
     *   the service. For each returned/resolved service, Harness uses the API to send a UDP packet to the service's
     *   IPv6 address at the respective returned port number.
     * - Pass Criteria:
     *   - The DUT MUST transmit 5 UDP packets to ED_2, to the following port numbers, which may be in any order:
     *     - 1. 55551
     *     - 2. 55552
     *     - 3. 55553
     *     - 4. 55554
     *     - 5. 55555
     *   - The DUT MAY send to either the ML-EID or OMR address of ED 2.
     */
    Log("Step 8: Resolve service address and send UDP packet to port %u", serviceInfo.mPort);

    {
        bool hasMeshLocal = false;
        for (uint16_t i = 0; response.GetHostAddress(hostName, i, address, ttl) == kErrorNone; i++)
        {
            if (context->mNode->Get<Mle::Mle>().IsMeshLocalAddress(address))
            {
                hasMeshLocal = true;
                break;
            }
        }

        if (!hasMeshLocal)
        {
            // If no mesh-local address was found, get the first address.
            // This will fail and quit if there are no addresses at all.
            SuccessOrQuit(response.GetHostAddress(hostName, 0, address, ttl));
        }
    }

    messageInfo.SetPeerAddr(address);
    messageInfo.SetPeerPort(serviceInfo.mPort);

    message = context->mNode->Get<Ip6::Udp>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->AppendBytes("hello", 5));

    SuccessOrQuit(context->mNode->Get<Ip6::Udp>().SendDatagram(*message, messageInfo));

    context->mUdpCount++;
}

void HandleBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    TestContext                       *context  = static_cast<TestContext *>(aContext);
    const Dns::Client::BrowseResponse &response = *static_cast<const Dns::Client::BrowseResponse *>(aResponse);
    Dns::Name::LabelBuffer             label;

    if (aError != OT_ERROR_NONE)
    {
        Log("HandleBrowseResponse error: %s", ErrorToString(aError));
        SuccessOrQuit(aError);
    }

    for (uint16_t index = 0; response.GetServiceInstance(index, label, sizeof(label)) == kErrorNone; index++)
    {
        context->mBrowseCount++;

        Log("Step 5/6: Found service instance %s, starting resolve", label);

        SuccessOrQuit(context->mNode->Get<Dns::Client>().ResolveService(label, kSrpFullServiceType,
                                                                        HandleResolveResponse, context));
    }
}

void Test_1_3_SRPC_TC_5(const char *aJsonFileName)
{
    /**
     * 3.5. [1.3] [CERT] [COMPONENT] DNS-SD client discovers services
     *
     * 3.5.1. Purpose
     * To test the following:
     * - DNS-SD client discovers services and sends out correctly formatted DNS query
     * - Client handling multiple service responses
     * - Client handling of both mesh-local discovered services and off-mesh services
     * - Note: Thread Product DUTs must skip this test.
     *
     * 3.5.2. Topology
     * - BR 1-Border Router Reference Device and Leader
     * - TD_1 (DUT)-Thread End Device or Thread Router DUT (Component DUT)
     * - ED 2-Thread Reference End Device
     * - Eth 1-Host Reference Device on AIL
     *
     * Spec Reference   | V1.1 Section | V1.3.0 Section
     * -----------------|--------------|---------------
     * SRP Client       | N/A          | 2.3.2
     */

    Core         nexus;
    Ip6::Address addresses[3];

    Node &br1  = nexus.CreateNode();
    Node &td1  = nexus.CreateNode();
    Node &ed2  = nexus.CreateNode();
    Node &eth1 = nexus.CreateNode();

    br1.SetName("BR_1");
    td1.SetName("TD_1");
    ed2.SetName("ED_2");
    eth1.SetName("Eth_1");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    Log("---------------------------------------------------------------------------------------");
    Log("Step 1: Device: BR 1. ED 2, Eth 1 Description (SRPC-3.5): Enable");

    /**
     * Step 1
     * - Device: BR 1. ED 2, Eth 1
     * - Description (SRPC-3.5): Enable
     * - Pass Criteria:
     *   - N/A
     */

    br1.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    ed2.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    eth1.mInfraIf.Init(eth1);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 2: Device: BR 1 Description (SRPC-3.5): Automatically adds its SRP Server information");

    /**
     * Step 2
     * - Device: BR 1
     * - Description (SRPC-3.5): Automatically adds its SRP Server information in the DNS/SRP Unicast Dataset in the
     *   Thread Network Data. Automatically configures an OMR prefix in Thread Network Data.
     * - Pass Criteria:
     *   - N/A
     */

    br1.Get<BorderRouter::InfraIf>().Init(kInfraIfIndex, true);
    br1.Get<BorderRouter::RoutingManager>().Init();
    SuccessOrQuit(br1.Get<BorderRouter::RoutingManager>().SetEnabled(true));

    SuccessOrQuit(br1.Get<Srp::Server>().SetAddressMode(Srp::Server::kAddressModeUnicast));
    br1.Get<Srp::Server>().SetEnabled(true);

    nexus.AdvanceTime(kStabilizationTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 3: Device: ED 2 Description (SRPC-3.5): Harness instructs device to register multiple N=5 services");

    /**
     * Step 3
     * - Device: ED 2
     * - Description (SRPC-3.5): Harness instructs device to register multiple N=5 services, each with TXT record per
     *   DNS-SD key/value formatting:
     *   - service-test-1. thread-test._udp ( SRV 8 8 55551 host-test-2 TXT dummy=abcd; thread-test=a49 )
     *   - service-test-2._thread-test._udp ( SRV 8 1 55552 host-test-2 TXT a=2; thread-test=b50_test )
     *   - service-test-3. thread-test._udp ( SRV 1  55553 host-test-2 TXT a_dummy=42; thread-test= )
     *   - service-test-4. thread-test._udp ( SRV 1 1 55554 host-test-2 TXT thread-test=1;ignorethis; thread-test= )
     *   - service-test-5. thread-test.udp ( SRV 2 2 55555 host-test-2 TXT a=88;thread- THREAD-TEST=C51; ignorethis;
     *     thread-test )
     *   - host-test-2 AAAA <non-routable address 2001::1234>
     *   - host-test-2 AAAA <OMR address of ED 2>
     *   - host-test-2 AAAA <ML-EID address of ED_2>
     *   - with the following options: Update Lease Option Lease: 5 minutes Key Lease: 10 minutes.
     * - Pass Criteria:
     *   - N/A
     */

    ed2.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    addresses[0] = ed2.FindGlobalAddress();
    addresses[1] = ed2.Get<Mle::Mle>().GetMeshLocalEid();
    SuccessOrQuit(addresses[2].FromString("2001::1234"));
    SuccessOrQuit(ed2.Get<Srp::Client>().SetHostAddresses(addresses, 3));

    SuccessOrQuit(ed2.Get<Srp::Client>().SetHostName(kSrpHostName));
    ed2.Get<Srp::Client>().SetLeaseInterval(kSrpLease5m);
    ed2.Get<Srp::Client>().SetKeyLeaseInterval(kSrpKeyLease10m);

    Srp::Client::Service services[5];
    Dns::TxtEntry        txtEntries[5][5];

    ClearAllBytes(services);
    ClearAllBytes(txtEntries);

    // Service 1
    services[0].mName             = kSrpServiceType;
    services[0].mInstanceName     = "service-test-1";
    services[0].mPort             = kSrpServicePort1;
    services[0].mPriority         = 8;
    services[0].mWeight           = 8;
    txtEntries[0][0].mKey         = "dummy";
    txtEntries[0][0].mValue       = reinterpret_cast<const uint8_t *>("abcd");
    txtEntries[0][0].mValueLength = 4;
    txtEntries[0][1].mKey         = "thread-test";
    txtEntries[0][1].mValue       = reinterpret_cast<const uint8_t *>("a49");
    txtEntries[0][1].mValueLength = 3;
    services[0].mTxtEntries       = txtEntries[0];
    services[0].mNumTxtEntries    = 2;
    SuccessOrQuit(services[0].Init());
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(services[0]));

    // Service 2
    services[1].mName             = kSrpServiceType;
    services[1].mInstanceName     = "service-test-2";
    services[1].mPort             = kSrpServicePort2;
    services[1].mPriority         = 8;
    services[1].mWeight           = 1;
    txtEntries[1][0].mKey         = "a";
    txtEntries[1][0].mValue       = reinterpret_cast<const uint8_t *>("2");
    txtEntries[1][0].mValueLength = 1;
    txtEntries[1][1].mKey         = "thread-test";
    txtEntries[1][1].mValue       = reinterpret_cast<const uint8_t *>("b50_test");
    txtEntries[1][1].mValueLength = 8;
    services[1].mTxtEntries       = txtEntries[1];
    services[1].mNumTxtEntries    = 2;
    SuccessOrQuit(services[1].Init());
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(services[1]));

    // Service 3
    services[2].mName             = kSrpServiceType;
    services[2].mInstanceName     = "service-test-3";
    services[2].mPort             = kSrpServicePort3;
    services[2].mPriority         = 1;
    services[2].mWeight           = 0;
    txtEntries[2][0].mKey         = "a_dummy";
    txtEntries[2][0].mValue       = reinterpret_cast<const uint8_t *>("42");
    txtEntries[2][0].mValueLength = 2;
    txtEntries[2][1].mKey         = "thread-test";
    txtEntries[2][1].mValue       = reinterpret_cast<const uint8_t *>("");
    txtEntries[2][1].mValueLength = 0;
    services[2].mTxtEntries       = txtEntries[2];
    services[2].mNumTxtEntries    = 2;
    SuccessOrQuit(services[2].Init());
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(services[2]));

    // Service 4
    services[3].mName             = kSrpServiceType;
    services[3].mInstanceName     = "service-test-4";
    services[3].mPort             = kSrpServicePort4;
    services[3].mPriority         = 1;
    services[3].mWeight           = 1;
    txtEntries[3][0].mKey         = "thread-test";
    txtEntries[3][0].mValue       = reinterpret_cast<const uint8_t *>("1");
    txtEntries[3][0].mValueLength = 1;
    txtEntries[3][1].mKey         = "ignorethis";
    txtEntries[3][1].mValue       = nullptr;
    txtEntries[3][1].mValueLength = 0;
    txtEntries[3][2].mKey         = "thread-test";
    txtEntries[3][2].mValue       = reinterpret_cast<const uint8_t *>("");
    txtEntries[3][2].mValueLength = 0;
    services[3].mTxtEntries       = txtEntries[3];
    services[3].mNumTxtEntries    = 3;
    SuccessOrQuit(services[3].Init());
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(services[3]));

    // Service 5
    services[4].mName             = kSrpServiceType;
    services[4].mInstanceName     = "service-test-5";
    services[4].mPort             = kSrpServicePort5;
    services[4].mPriority         = 2;
    services[4].mWeight           = 2;
    txtEntries[4][0].mKey         = "a";
    txtEntries[4][0].mValue       = reinterpret_cast<const uint8_t *>("88");
    txtEntries[4][0].mValueLength = 2;
    txtEntries[4][1].mKey         = "thread-";
    txtEntries[4][1].mValue       = nullptr;
    txtEntries[4][1].mValueLength = 0;
    txtEntries[4][2].mKey         = "THREAD-TEST";
    txtEntries[4][2].mValue       = reinterpret_cast<const uint8_t *>("C51");
    txtEntries[4][2].mValueLength = 3;
    txtEntries[4][3].mKey         = "ignorethis";
    txtEntries[4][3].mValue       = nullptr;
    txtEntries[4][3].mValueLength = 0;
    txtEntries[4][4].mKey         = "thread-test";
    txtEntries[4][4].mValue       = nullptr;
    txtEntries[4][4].mValueLength = 0;
    services[4].mTxtEntries       = txtEntries[4];
    services[4].mNumTxtEntries    = 5;
    SuccessOrQuit(services[4].Init());
    SuccessOrQuit(ed2.Get<Srp::Client>().AddService(services[4]));

    nexus.AdvanceTime(kSrpRegistrationTime);
    {
        Srp::Client::ItemState state = ed2.Get<Srp::Client>().GetHostInfo().GetState();
        VerifyOrQuit(state == Srp::Client::kRegistered || state == Srp::Client::kToRefresh ||
                     state == Srp::Client::kRefreshing);
    }

    Log("---------------------------------------------------------------------------------------");
    Log("Step 4: Device: TD 1 (DUT) Description (SRPC-3.5): Enable");

    /**
     * Step 4
     * - Device: TD 1 (DUT)
     * - Description (SRPC-3.5): Enable
     * - Pass Criteria:
     *   - N/A
     */

    td1.Join(br1, Node::kAsFed);
    nexus.AdvanceTime(kJoinNetworkTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 5: Device: TD_1 (DUT) Description (SRPC-3.5): Harness instructs device to query for any services");

    /**
     * Step 5
     * - Device: TD_1 (DUT)
     * - Description (SRPC-3.5): Harness instructs device to query for any services of type
     *   "_thread-test._udp.default.service.arpa." using unicast DNS query QType=PTR
     * - Pass Criteria:
     *   - The DUT MUST send a correctly formatted DNS query.
     */

    TestContext context;
    context.mNode         = &td1;
    context.mBrowseCount  = 0;
    context.mResolveCount = 0;
    context.mUdpCount     = 0;

    Dns::Client::QueryConfig queryConfig = td1.Get<Dns::Client>().GetDefaultConfig();

    AsCoreType(&queryConfig.mServerSockAddr).SetAddress(br1.Get<Mle::Mle>().GetMeshLocalEid());
    queryConfig.mServerSockAddr.mPort = 53;
    td1.Get<Dns::Client>().SetDefaultConfig(queryConfig);

    SuccessOrQuit(td1.Get<Dns::Client>().Browse(kSrpFullServiceType, HandleBrowseResponse, &context));

    nexus.AdvanceTime(kDnsQueryTime);

    Log("---------------------------------------------------------------------------------------");
    Log("Step 6: Device: BR 1 Description (SRPC-3.5): Automatically responds with all N services");

    /**
     * Step 6
     * - Device: BR 1
     * - Description (SRPC-3.5): Automatically responds with all N services.
     * - Pass Criteria:
     *   - Responds with DNS answer with all N services as indicated in step 3 in the Answer records section.
     *   - DNS response Rcode MUST be 'NoError' (0).
     *   - DNS response MUST contain N=5 answer records.
     */

    nexus.AdvanceTime(kDnsQueryTime * 2);

    // Verify all steps completed
    VerifyOrQuit(context.mBrowseCount == 5);
    VerifyOrQuit(context.mResolveCount == 5);
    VerifyOrQuit(context.mUdpCount == 5);

    nexus.AddTestVar("BR_1_MLEID_ADDR", br1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("TD_1_MLEID_ADDR", td1.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_2_MLEID_ADDR", ed2.Get<Mle::Mle>().GetMeshLocalEid().ToString().AsCString());
    nexus.AddTestVar("ED_2_OMR_ADDR", ed2.FindGlobalAddress().ToString().AsCString());

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::Test_1_3_SRPC_TC_5((argc > 2) ? argv[2] : "test_1_3_SRPC_TC_5.json");
    printf("All tests passed\n");
    return 0;
}
