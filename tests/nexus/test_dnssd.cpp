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

#include "common/as_core_type.hpp"
#include "common/string.hpp"
#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

namespace {

static constexpr uint32_t kFormNetworkTime   = 13 * 1000;
static constexpr uint32_t kJoinNetworkTime   = 10 * 1000;
static constexpr uint32_t kStabilizationTime = 200 * 1000;
static constexpr uint32_t kDnsQueryTime      = 5 * 1000;

static const char kService[]             = "_ipps._tcp";
static const char kFullServiceType[]     = "_ipps._tcp.default.service.arpa.";
static const char kSubtype1ServiceType[] = "_s1._sub._ipps._tcp.default.service.arpa.";
static const char kSubtype2ServiceType[] = "_s2._sub._ipps._tcp.default.service.arpa.";

struct BrowseContext
{
    Error                  mError;
    uint8_t                mCount;
    Dns::Name::LabelBuffer mLabels[10];

    void Reset(void)
    {
        mError = kErrorNone;
        mCount = 0;
    }

    bool ContainsInstance(const char *aLabel) const
    {
        bool found = false;

        for (uint8_t i = 0; i < mCount; i++)
        {
            if (StringMatch(mLabels[i], aLabel, kStringCaseInsensitiveMatch))
            {
                found = true;
                break;
            }
        }

        return found;
    }
};

struct ResolveContext
{
    Error                    mError;
    Dns::Name::LabelBuffer   mInstanceLabel;
    Dns::Name::Buffer        mFullServiceName;
    Dns::Name::Buffer        mHostName;
    uint8_t                  mTxtData[128];
    Dns::Client::ServiceInfo mServiceInfo;

    void Reset(void)
    {
        mError              = kErrorNone;
        mInstanceLabel[0]   = '\0';
        mFullServiceName[0] = '\0';
        mHostName[0]        = '\0';
        ClearAllBytes(mServiceInfo);
        mServiceInfo.mHostNameBuffer     = mHostName;
        mServiceInfo.mHostNameBufferSize = sizeof(mHostName);
        mServiceInfo.mTxtData            = mTxtData;
        mServiceInfo.mTxtDataSize        = sizeof(mTxtData);
    }
};

struct AddressContext
{
    Error        mError;
    Ip6::Address mAddresses[10];
    uint8_t      mCount;

    void Reset(void)
    {
        mError = kErrorNone;
        mCount = 0;
    }
};

struct RecordContext
{
    Error             mError;
    uint8_t           mCount;
    uint16_t          mType;
    uint16_t          mLength;
    uint32_t          mTtl;
    Dns::Name::Buffer mName;

    void Reset(void)
    {
        mError   = kErrorNone;
        mCount   = 0;
        mType    = 0;
        mLength  = 0;
        mTtl     = 0;
        mName[0] = '\0';
    }
};

void HandleBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    BrowseContext                     *context  = static_cast<BrowseContext *>(aContext);
    const Dns::Client::BrowseResponse &response = AsCoreType(aResponse);

    context->mError = static_cast<Error>(aError);
    SuccessOrExit(context->mError);

    while (response.GetServiceInstance(context->mCount, context->mLabels[context->mCount],
                                       sizeof(context->mLabels[context->mCount])) == kErrorNone)
    {
        context->mCount++;
    }

exit:
    return;
}

void HandleResolveResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    ResolveContext                     *context  = static_cast<ResolveContext *>(aContext);
    const Dns::Client::ServiceResponse &response = AsCoreType(aResponse);

    context->mError = static_cast<Error>(aError);
    SuccessOrExit(context->mError);

    SuccessOrExit(response.GetServiceName(context->mInstanceLabel, sizeof(context->mInstanceLabel),
                                          context->mFullServiceName, sizeof(context->mFullServiceName)));
    SuccessOrExit(response.GetServiceInfo(context->mServiceInfo));

exit:
    return;
}

void HandleAddressResolveResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    AddressContext                     *context  = static_cast<AddressContext *>(aContext);
    const Dns::Client::AddressResponse &response = AsCoreType(aResponse);
    uint32_t                            ttl;

    context->mError = static_cast<Error>(aError);
    SuccessOrExit(context->mError);

    while (response.GetAddress(context->mCount, context->mAddresses[context->mCount], ttl) == kErrorNone)
    {
        context->mCount++;
    }

exit:
    return;
}

void HandleRecordQueryResponse(otError aError, const otDnsRecordResponse *aResponse, void *aContext)
{
    RecordContext                     *context  = static_cast<RecordContext *>(aContext);
    const Dns::Client::RecordResponse &response = AsCoreType(aResponse);
    Dns::Client::RecordInfo            recordInfo;

    context->mError = static_cast<Error>(aError);
    SuccessOrExit(context->mError);

    recordInfo.mNameBuffer     = context->mName;
    recordInfo.mNameBufferSize = sizeof(context->mName);

    for (uint16_t index = 0; response.GetRecordInfo(index, recordInfo) == kErrorNone; index++)
    {
        if (recordInfo.mRecordType == context->mType)
        {
            context->mCount++;
            context->mLength = recordInfo.mRecordLength;
            context->mTtl    = recordInfo.mTtl;
            break;
        }
    }

exit:
    return;
}

} // namespace

void TestDnssd(const char *aJsonFileName)
{
    Core  nexus;
    Node &server  = nexus.CreateNode();
    Node &client1 = nexus.CreateNode();
    Node &client2 = nexus.CreateNode();
    Node &client3 = nexus.CreateNode();

    static Srp::Client::Service srv1, srv2, srv3;
    static const char *const    kSubtypes1[] = {"_s1", "_s2", nullptr};
    static const char *const    kSubtypes3[] = {"_s1", nullptr};

    static Ip6::Address addrs1[2], addrs2[2], addrs3[2];

    server.SetName("Server");
    client1.SetName("Client1");
    client2.SetName("Client2");
    client3.SetName("Client3");

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelNote));

    //-----------------------------------------------------------------------------------------
    // Step 1: Start topology
    Log("Step 1: Start topology");

    server.Form();
    nexus.AdvanceTime(kFormNetworkTime);

    client1.Join(server);
    client2.Join(server);
    client3.Join(server);
    nexus.AdvanceTime(kJoinNetworkTime);

    server.Get<Srp::Server>().SetEnabled(true);
    SuccessOrQuit(server.Get<Dns::ServiceDiscovery::Server>().Start());

    nexus.AdvanceTime(kStabilizationTime);

    //-----------------------------------------------------------------------------------------
    // Step 2: Register services on clients
    Log("Step 2: Register services on clients");

    client1.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    client2.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);
    client3.Get<Srp::Client>().EnableAutoStartMode(nullptr, nullptr);

    addrs1[0] = client1.Get<Mle::Mle>().GetMeshLocalEid();
    addrs1[1] = client1.Get<Mle::Mle>().GetMeshLocalRloc();
    SuccessOrQuit(client1.Get<Srp::Client>().SetHostName("host1"));
    SuccessOrQuit(client1.Get<Srp::Client>().SetHostAddresses(addrs1, 2));
    ClearAllBytes(srv1);
    srv1.mName          = kService;
    srv1.mInstanceName  = "ins1";
    srv1.mPort          = 11111;
    srv1.mPriority      = 1;
    srv1.mWeight        = 1;
    srv1.mSubTypeLabels = kSubtypes1;
    SuccessOrQuit(srv1.Init());
    SuccessOrQuit(client1.Get<Srp::Client>().AddService(srv1));

    addrs2[0] = client2.Get<Mle::Mle>().GetMeshLocalEid();
    addrs2[1] = client2.Get<Mle::Mle>().GetMeshLocalRloc();
    SuccessOrQuit(client2.Get<Srp::Client>().SetHostName("host2"));
    SuccessOrQuit(client2.Get<Srp::Client>().SetHostAddresses(addrs2, 2));
    ClearAllBytes(srv2);
    srv2.mName         = kService;
    srv2.mInstanceName = "ins2";
    srv2.mPort         = 22222;
    srv2.mPriority     = 2;
    srv2.mWeight       = 2;
    SuccessOrQuit(srv2.Init());
    SuccessOrQuit(client2.Get<Srp::Client>().AddService(srv2));

    addrs3[0] = client3.Get<Mle::Mle>().GetMeshLocalEid();
    addrs3[1] = client3.Get<Mle::Mle>().GetMeshLocalRloc();
    SuccessOrQuit(client3.Get<Srp::Client>().SetHostName("host3"));
    SuccessOrQuit(client3.Get<Srp::Client>().SetHostAddresses(addrs3, 2));
    ClearAllBytes(srv3);
    srv3.mName          = kService;
    srv3.mInstanceName  = "ins3";
    srv3.mPort          = 33333;
    srv3.mPriority      = 3;
    srv3.mWeight        = 3;
    srv3.mSubTypeLabels = kSubtypes3;
    SuccessOrQuit(srv3.Init());
    SuccessOrQuit(client3.Get<Srp::Client>().AddService(srv3));

    nexus.AdvanceTime(kStabilizationTime);

    //-----------------------------------------------------------------------------------------
    // Step 3: Resolve address (AAAA records)
    Log("Step 3: Resolve address (AAAA records)");

    AddressContext addressContext;
    addressContext.Reset();

    Dns::Client::QueryConfig queryConfig;
    queryConfig.Clear();
    AsCoreType(&queryConfig.mServerSockAddr).SetAddress(server.Get<Mle::Mle>().GetMeshLocalEid());
    queryConfig.mServerSockAddr.mPort = 53;
    client1.Get<Dns::Client>().SetDefaultConfig(queryConfig);

    SuccessOrQuit(client1.Get<Dns::Client>().ResolveAddress("host1.default.service.arpa.", HandleAddressResolveResponse,
                                                            &addressContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(addressContext.mError);
    VerifyOrQuit(addressContext.mCount == 2);
    VerifyOrQuit(addressContext.mAddresses[0] == client1.Get<Mle::Mle>().GetMeshLocalEid() ||
                 addressContext.mAddresses[0] == client1.Get<Mle::Mle>().GetMeshLocalRloc());
    VerifyOrQuit(addressContext.mAddresses[1] == client1.Get<Mle::Mle>().GetMeshLocalEid() ||
                 addressContext.mAddresses[1] == client1.Get<Mle::Mle>().GetMeshLocalRloc());

    addressContext.Reset();
    SuccessOrQuit(client1.Get<Dns::Client>().ResolveAddress("host2.default.service.arpa.", HandleAddressResolveResponse,
                                                            &addressContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(addressContext.mError);
    VerifyOrQuit(addressContext.mCount == 2);
    VerifyOrQuit(addressContext.mAddresses[0] == client2.Get<Mle::Mle>().GetMeshLocalEid() ||
                 addressContext.mAddresses[0] == client2.Get<Mle::Mle>().GetMeshLocalRloc());
    VerifyOrQuit(addressContext.mAddresses[1] == client2.Get<Mle::Mle>().GetMeshLocalEid() ||
                 addressContext.mAddresses[1] == client2.Get<Mle::Mle>().GetMeshLocalRloc());

    //-----------------------------------------------------------------------------------------
    // Step 4: Browsing for services
    Log("Step 4: Browsing for services");

    BrowseContext browseContext;
    browseContext.Reset();
    SuccessOrQuit(client1.Get<Dns::Client>().Browse(kFullServiceType, HandleBrowseResponse, &browseContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(browseContext.mError);
    VerifyOrQuit(browseContext.mCount == 3);
    VerifyOrQuit(browseContext.ContainsInstance("ins1"));
    VerifyOrQuit(browseContext.ContainsInstance("ins2"));
    VerifyOrQuit(browseContext.ContainsInstance("ins3"));

    // Browse for subtype _s1
    browseContext.Reset();
    SuccessOrQuit(client1.Get<Dns::Client>().Browse(kSubtype1ServiceType, HandleBrowseResponse, &browseContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(browseContext.mError);
    VerifyOrQuit(browseContext.mCount == 2);
    VerifyOrQuit(browseContext.ContainsInstance("ins1"));
    VerifyOrQuit(browseContext.ContainsInstance("ins3"));

    // Browse for subtype _s2
    browseContext.Reset();
    SuccessOrQuit(client1.Get<Dns::Client>().Browse(kSubtype2ServiceType, HandleBrowseResponse, &browseContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(browseContext.mError);
    VerifyOrQuit(browseContext.mCount == 1);
    VerifyOrQuit(browseContext.ContainsInstance("ins1"));

    //-----------------------------------------------------------------------------------------
    // Step 5: Resolve service
    Log("Step 5: Resolve service");

    ResolveContext resolveContext;

    resolveContext.Reset();
    SuccessOrQuit(
        client1.Get<Dns::Client>().ResolveService("ins1", kFullServiceType, HandleResolveResponse, &resolveContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(resolveContext.mError);
    VerifyOrQuit(StringMatch(resolveContext.mInstanceLabel, "ins1", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch(resolveContext.mFullServiceName, kFullServiceType, kStringCaseInsensitiveMatch));
    VerifyOrQuit(resolveContext.mServiceInfo.mPort == 11111);
    VerifyOrQuit(resolveContext.mServiceInfo.mPriority == 1);
    VerifyOrQuit(resolveContext.mServiceInfo.mWeight == 1);
    VerifyOrQuit(StringMatch(resolveContext.mHostName, "host1.default.service.arpa.", kStringCaseInsensitiveMatch));
    VerifyOrQuit(AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client1.Get<Mle::Mle>().GetMeshLocalEid() ||
                 AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client1.Get<Mle::Mle>().GetMeshLocalRloc());
    VerifyOrQuit(resolveContext.mServiceInfo.mTtl > 0);
    VerifyOrQuit(resolveContext.mServiceInfo.mHostAddressTtl > 0);

    resolveContext.Reset();
    SuccessOrQuit(
        client1.Get<Dns::Client>().ResolveService("ins2", kFullServiceType, HandleResolveResponse, &resolveContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(resolveContext.mError);
    VerifyOrQuit(StringMatch(resolveContext.mInstanceLabel, "ins2", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch(resolveContext.mFullServiceName, kFullServiceType, kStringCaseInsensitiveMatch));
    VerifyOrQuit(resolveContext.mServiceInfo.mPort == 22222);
    VerifyOrQuit(resolveContext.mServiceInfo.mPriority == 2);
    VerifyOrQuit(resolveContext.mServiceInfo.mWeight == 2);
    VerifyOrQuit(StringMatch(resolveContext.mHostName, "host2.default.service.arpa.", kStringCaseInsensitiveMatch));
    VerifyOrQuit(AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client2.Get<Mle::Mle>().GetMeshLocalEid() ||
                 AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client2.Get<Mle::Mle>().GetMeshLocalRloc());

    resolveContext.Reset();
    SuccessOrQuit(
        client1.Get<Dns::Client>().ResolveService("ins3", kFullServiceType, HandleResolveResponse, &resolveContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(resolveContext.mError);
    VerifyOrQuit(StringMatch(resolveContext.mInstanceLabel, "ins3", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch(resolveContext.mFullServiceName, kFullServiceType, kStringCaseInsensitiveMatch));
    VerifyOrQuit(resolveContext.mServiceInfo.mPort == 33333);
    VerifyOrQuit(resolveContext.mServiceInfo.mPriority == 3);
    VerifyOrQuit(resolveContext.mServiceInfo.mWeight == 3);
    VerifyOrQuit(StringMatch(resolveContext.mHostName, "host3.default.service.arpa.", kStringCaseInsensitiveMatch));
    VerifyOrQuit(AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client3.Get<Mle::Mle>().GetMeshLocalEid() ||
                 AsCoreType(&resolveContext.mServiceInfo.mHostAddress) == client3.Get<Mle::Mle>().GetMeshLocalRloc());

    //-----------------------------------------------------------------------------------------
    // Step 6: Add another service with TXT entries
    Log("Step 6: Add another service with TXT entries");

    Srp::Client::Service srv4;
    {
        static const Dns::TxtEntry kTxtEntries[] = {{"KEY", reinterpret_cast<const uint8_t *>("ABC"), 3}};

        ClearAllBytes(srv4);
        srv4.mName          = kService;
        srv4.mInstanceName  = "ins4";
        srv4.mPort          = 44444;
        srv4.mPriority      = 4;
        srv4.mWeight        = 4;
        srv4.mTxtEntries    = kTxtEntries;
        srv4.mNumTxtEntries = 1;
        srv4.mSubTypeLabels = kSubtypes3;

        SuccessOrQuit(srv4.Init());
        SuccessOrQuit(client3.Get<Srp::Client>().AddService(srv4));
    }
    nexus.AdvanceTime(kStabilizationTime);

    browseContext.Reset();
    SuccessOrQuit(client1.Get<Dns::Client>().Browse(kFullServiceType, HandleBrowseResponse, &browseContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(browseContext.mError);
    VerifyOrQuit(browseContext.mCount == 4);
    VerifyOrQuit(browseContext.ContainsInstance("ins1"));
    VerifyOrQuit(browseContext.ContainsInstance("ins2"));
    VerifyOrQuit(browseContext.ContainsInstance("ins3"));
    VerifyOrQuit(browseContext.ContainsInstance("ins4"));

    resolveContext.Reset();
    SuccessOrQuit(
        client1.Get<Dns::Client>().ResolveService("ins4", kFullServiceType, HandleResolveResponse, &resolveContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(resolveContext.mError);
    VerifyOrQuit(StringMatch(resolveContext.mInstanceLabel, "ins4", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch(resolveContext.mFullServiceName, kFullServiceType, kStringCaseInsensitiveMatch));
    VerifyOrQuit(resolveContext.mServiceInfo.mPort == 44444);
    VerifyOrQuit(resolveContext.mServiceInfo.mPriority == 4);
    VerifyOrQuit(resolveContext.mServiceInfo.mWeight == 4);
    VerifyOrQuit(resolveContext.mServiceInfo.mTxtDataSize == 8);
    VerifyOrQuit(memcmp(resolveContext.mServiceInfo.mTxtData, "\x07KEY=ABC", 8) == 0);

    //-----------------------------------------------------------------------------------------
    // Step 7: Query for KEY record
    Log("Step 7: Query for KEY record");

    RecordContext recordContext;
    recordContext.Reset();
    recordContext.mType = 25; // KEY
    SuccessOrQuit(client1.Get<Dns::Client>().QueryRecord(25, nullptr, "ins1._ipps._tcp.default.service.arpa.",
                                                         HandleRecordQueryResponse, &recordContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(recordContext.mError);
    VerifyOrQuit(recordContext.mCount == 1);
    VerifyOrQuit(recordContext.mLength == 78);
    VerifyOrQuit(recordContext.mTtl > 0);
    VerifyOrQuit(
        StringMatch(recordContext.mName, "ins1._ipps._tcp.default.service.arpa.", kStringCaseInsensitiveMatch));

    //-----------------------------------------------------------------------------------------
    // Step 8: Query for SRV record
    Log("Step 8: Query for SRV record");

    recordContext.Reset();
    recordContext.mType = 33; // SRV
    SuccessOrQuit(client1.Get<Dns::Client>().QueryRecord(33, nullptr, "ins1._ipps._tcp.default.service.arpa.",
                                                         HandleRecordQueryResponse, &recordContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(recordContext.mError);
    VerifyOrQuit(recordContext.mCount == 1);
    VerifyOrQuit(recordContext.mLength > 0);
    VerifyOrQuit(recordContext.mTtl > 0);
    VerifyOrQuit(
        StringMatch(recordContext.mName, "ins1._ipps._tcp.default.service.arpa.", kStringCaseInsensitiveMatch));

    //-----------------------------------------------------------------------------------------
    // Step 9: Query for non-existing A record
    Log("Step 9: Query for non-existing A record");

    recordContext.Reset();
    recordContext.mType = 1; // A
    SuccessOrQuit(client1.Get<Dns::Client>().QueryRecord(1, nullptr, "ins1._ipps._tcp.default.service.arpa.",
                                                         HandleRecordQueryResponse, &recordContext));
    nexus.AdvanceTime(kDnsQueryTime);
    SuccessOrQuit(recordContext.mError);
    VerifyOrQuit(recordContext.mCount == 0);

    nexus.SaveTestInfo(aJsonFileName);
}

} // namespace Nexus
} // namespace ot

int main(int argc, char *argv[])
{
    ot::Nexus::TestDnssd((argc > 2) ? argv[2] : "test_dnssd.json");
    printf("All tests passed\n");
    return 0;
}
