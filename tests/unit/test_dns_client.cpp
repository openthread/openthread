/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#include <openthread/config.h>

#include "test_platform.h"
#include "test_util.hpp"

#include <openthread/dataset_ftd.h>
#include <openthread/dns_client.h>
#include <openthread/srp_client.h>
#include <openthread/srp_server.h>
#include <openthread/thread.h>

#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/string.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE &&                 \
    OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE && OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE && \
    OPENTHREAD_CONFIG_SRP_SERVER_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE &&                                   \
    !OPENTHREAD_CONFIG_TIME_SYNC_ENABLE && !OPENTHREAD_PLATFORM_POSIX
#define ENABLE_DNS_TEST 1
#else
#define ENABLE_DNS_TEST 0
#endif

#if ENABLE_DNS_TEST

using namespace ot;

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                          \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 36000000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint16_t kMaxRaSize = 800;

static ot::Instance *sInstance;

static uint32_t sNow = 0;
static uint32_t sAlarmTime;
static bool     sAlarmOn = false;

static otRadioFrame sRadioTxFrame;
static uint8_t      sRadioTxFramePsdu[OT_RADIO_FRAME_MAX_SIZE];
static bool         sRadioTxOngoing = false;

//----------------------------------------------------------------------------------------------------------------------
// Function prototypes

void ProcessRadioTxAndTasklets(void);
void AdvanceTime(uint32_t aDuration);

//----------------------------------------------------------------------------------------------------------------------
// `otPlatRadio`

extern "C" {

otRadioCaps otPlatRadioGetCaps(otInstance *) { return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF; }

otError otPlatRadioTransmit(otInstance *, otRadioFrame *)
{
    sRadioTxOngoing = true;

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *) { return &sRadioTxFrame; }

//----------------------------------------------------------------------------------------------------------------------
// `otPlatAlaram`

void otPlatAlarmMilliStop(otInstance *) { sAlarmOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

//----------------------------------------------------------------------------------------------------------------------

Array<void *, 500> sHeapAllocatedPtrs;

#if OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    void *ptr = calloc(aNum, aSize);

    SuccessOrQuit(sHeapAllocatedPtrs.PushBack(ptr));

    return ptr;
}

void otPlatFree(void *aPtr)
{
    if (aPtr != nullptr)
    {
        void **entry = sHeapAllocatedPtrs.Find(aPtr);

        VerifyOrQuit(entry != nullptr, "A heap allocated item is freed twice");
        sHeapAllocatedPtrs.Remove(*entry);
    }

    free(aPtr);
}
#endif

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    va_list args;

    printf("   ");
    va_start(args, aFormat);
    vprintf(aFormat, args);
    va_end(args);
    printf("\n");
}
#endif

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------

void ProcessRadioTxAndTasklets(void)
{
    do
    {
        if (sRadioTxOngoing)
        {
            sRadioTxOngoing = false;
            otPlatRadioTxStarted(sInstance, &sRadioTxFrame);
            otPlatRadioTxDone(sInstance, &sRadioTxFrame, nullptr, OT_ERROR_NONE);
        }

        otTaskletsProcess(sInstance);
    } while (otTaskletsArePending(sInstance));
}

void AdvanceTime(uint32_t aDuration)
{
    uint32_t time = sNow + aDuration;

    Log("AdvanceTime for %u.%03u", aDuration / 1000, aDuration % 1000);

    while (TimeMilli(sAlarmTime) <= TimeMilli(time))
    {
        ProcessRadioTxAndTasklets();
        sNow = sAlarmTime;
        otPlatAlarmMilliFired(sInstance);
    }

    ProcessRadioTxAndTasklets();
    sNow = time;
}

void InitTest(void)
{
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize OT instance.

    sNow      = 0;
    sAlarmOn  = false;
    sInstance = static_cast<Instance *>(testInitInstance());

    memset(&sRadioTxFrame, 0, sizeof(sRadioTxFrame));
    sRadioTxFrame.mPsdu = sRadioTxFramePsdu;
    sRadioTxOngoing     = false;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize Border Router and start Thread operation.

    otOperationalDataset     dataset;
    otOperationalDatasetTlvs datasetTlvs;

    SuccessOrQuit(otDatasetCreateNewNetwork(sInstance, &dataset));
    SuccessOrQuit(otDatasetConvertToTlvs(&dataset, &datasetTlvs));
    SuccessOrQuit(otDatasetSetActiveTlvs(sInstance, &datasetTlvs));

    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure device starts as leader.

    AdvanceTime(10000);

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
}

void FinalizeTest(void)
{
    SuccessOrQuit(otIp6SetEnabled(sInstance, false));
    SuccessOrQuit(otThreadSetEnabled(sInstance, false));
    // Make sure there is no message/buffer leak
    VerifyOrQuit(sInstance->Get<MessagePool>().GetFreeBufferCount() ==
                 sInstance->Get<MessagePool>().GetTotalBufferCount());
    SuccessOrQuit(otInstanceErasePersistentInfo(sInstance));
    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

static const char kHostName[]     = "elden";
static const char kHostFullName[] = "elden.default.service.arpa.";

static const char kService1Name[]      = "_srv._udp";
static const char kService1FullName[]  = "_srv._udp.default.service.arpa.";
static const char kInstance1Label[]    = "srv-instance";
static const char kInstance1FullName[] = "srv-instance._srv._udp.default.service.arpa.";

static const char kService2Name[]            = "_game._udp";
static const char kService2FullName[]        = "_game._udp.default.service.arpa.";
static const char kService2SubTypeFullName[] = "_best._sub._game._udp.default.service.arpa.";
static const char kInstance2Label[]          = "last-ninja";
static const char kInstance2FullName[]       = "last-ninja._game._udp.default.service.arpa.";

void PrepareService1(Srp::Client::Service &aService)
{
    static const char          kSub1[]       = "_sub1";
    static const char          kSub2[]       = "_V1234567";
    static const char          kSub3[]       = "_XYZWS";
    static const char         *kSubLabels[]  = {kSub1, kSub2, kSub3, nullptr};
    static const char          kTxtKey1[]    = "ABCD";
    static const uint8_t       kTxtValue1[]  = {'a', '0'};
    static const char          kTxtKey2[]    = "Z0";
    static const uint8_t       kTxtValue2[]  = {'1', '2', '3'};
    static const char          kTxtKey3[]    = "D";
    static const uint8_t       kTxtValue3[]  = {0};
    static const otDnsTxtEntry kTxtEntries[] = {
        {kTxtKey1, kTxtValue1, sizeof(kTxtValue1)},
        {kTxtKey2, kTxtValue2, sizeof(kTxtValue2)},
        {kTxtKey3, kTxtValue3, sizeof(kTxtValue3)},
    };

    memset(&aService, 0, sizeof(aService));
    aService.mName          = kService1Name;
    aService.mInstanceName  = kInstance1Label;
    aService.mSubTypeLabels = kSubLabels;
    aService.mTxtEntries    = kTxtEntries;
    aService.mNumTxtEntries = 3;
    aService.mPort          = 777;
    aService.mWeight        = 1;
    aService.mPriority      = 2;
}

void PrepareService2(Srp::Client::Service &aService)
{
    static const char  kSub4[]       = "_best";
    static const char *kSubLabels2[] = {kSub4, nullptr};

    memset(&aService, 0, sizeof(aService));
    aService.mName          = kService2Name;
    aService.mInstanceName  = kInstance2Label;
    aService.mSubTypeLabels = kSubLabels2;
    aService.mTxtEntries    = nullptr;
    aService.mNumTxtEntries = 0;
    aService.mPort          = 555;
    aService.mWeight        = 0;
    aService.mPriority      = 3;
}

void ValidateHost(Srp::Server &aServer, const char *aHostName)
{
    // Validate that only a host with `aHostName` is
    // registered on SRP server.

    const Srp::Server::Host *host;
    const char              *name;

    Log("ValidateHost()");

    host = aServer.GetNextHost(nullptr);
    VerifyOrQuit(host != nullptr);

    name = host->GetFullName();
    Log("Hostname: %s", name);

    VerifyOrQuit(StringStartsWith(name, aHostName, kStringCaseInsensitiveMatch));
    VerifyOrQuit(name[strlen(aHostName)] == '.');

    // Only one host on server
    VerifyOrQuit(aServer.GetNextHost(host) == nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void LogServiceInfo(const Dns::Client::ServiceInfo &aInfo)
{
    Log("   TTL: %u", aInfo.mTtl);
    Log("   Port: %u", aInfo.mPort);
    Log("   Weight: %u", aInfo.mWeight);
    Log("   HostName: %s", aInfo.mHostNameBuffer);
    Log("   HostAddr: %s", AsCoreType(&aInfo.mHostAddress).ToString().AsCString());
    Log("   TxtDataLength: %u", aInfo.mTxtDataSize);
    Log("   TxtDataTTL: %u", aInfo.mTxtDataTtl);
}

const char *ServiceModeToString(Dns::Client::QueryConfig::ServiceMode aMode)
{
    static const char *const kServiceModeStrings[] = {
        "unspec",      // kServiceModeUnspecified     (0)
        "srv",         // kServiceModeSrv             (1)
        "txt",         // kServiceModeTxt             (2)
        "srv_txt",     // kServiceModeSrvTxt          (3)
        "srv_txt_sep", // kServiceModeSrvTxtSeparate  (4)
        "srv_txt_opt", // kServiceModeSrvTxtOptimize  (5)
    };

    static_assert(Dns::Client::QueryConfig::kServiceModeUnspecified == 0, "Unspecified value is incorrect");
    static_assert(Dns::Client::QueryConfig::kServiceModeSrv == 1, "Srv value is incorrect");
    static_assert(Dns::Client::QueryConfig::kServiceModeTxt == 2, "Txt value is incorrect");
    static_assert(Dns::Client::QueryConfig::kServiceModeSrvTxt == 3, "SrvTxt value is incorrect");
    static_assert(Dns::Client::QueryConfig::kServiceModeSrvTxtSeparate == 4, "SrvTxtSeparate value is incorrect");
    static_assert(Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize == 5, "SrvTxtOptimize value is incorrect");

    return kServiceModeStrings[aMode];
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct BrowseInfo
{
    void Reset(void) { mCallbackCount = 0; }

    uint16_t          mCallbackCount;
    Error             mError;
    Dns::Name::Buffer mServiceName;
    uint16_t          mNumInstances;
};

static BrowseInfo sBrowseInfo;

void BrowseCallback(otError aError, const otDnsBrowseResponse *aResponse, void *aContext)
{
    const Dns::Client::BrowseResponse &response = AsCoreType(aResponse);

    Log("BrowseCallback");
    Log("   Error: %s", ErrorToString(aError));

    VerifyOrQuit(aContext == sInstance);

    sBrowseInfo.mCallbackCount++;
    sBrowseInfo.mError = aError;

    SuccessOrExit(aError);

    SuccessOrQuit(response.GetServiceName(sBrowseInfo.mServiceName, sizeof(sBrowseInfo.mServiceName)));
    Log("   ServiceName: %s", sBrowseInfo.mServiceName);

    for (uint16_t index = 0;; index++)
    {
        Dns::Name::LabelBuffer instLabel;
        Error                  error;

        error = response.GetServiceInstance(index, instLabel, sizeof(instLabel));

        if (error == kErrorNotFound)
        {
            sBrowseInfo.mNumInstances = index;
            break;
        }

        SuccessOrQuit(error);

        Log("  %2u) %s", index + 1, instLabel);
    }

exit:
    return;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static constexpr uint8_t  kMaxHostAddresses = 10;
static constexpr uint16_t kMaxTxtBuffer     = 256;

struct ResolveServiceInfo
{
    void Reset(void)
    {
        memset(this, 0, sizeof(*this));
        mInfo.mHostNameBuffer     = mNameBuffer;
        mInfo.mHostNameBufferSize = sizeof(mNameBuffer);
        mInfo.mTxtData            = mTxtBuffer;
        mInfo.mTxtDataSize        = sizeof(mTxtBuffer);
    };

    uint16_t                 mCallbackCount;
    Error                    mError;
    Dns::Client::ServiceInfo mInfo;
    Dns::Name::Buffer        mNameBuffer;
    uint8_t                  mTxtBuffer[kMaxTxtBuffer];
    Ip6::Address             mHostAddresses[kMaxHostAddresses];
    uint8_t                  mNumHostAddresses;
};

static ResolveServiceInfo sResolveServiceInfo;

void ServiceCallback(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    const Dns::Client::ServiceResponse &response = AsCoreType(aResponse);
    Dns::Name::LabelBuffer              instLabel;
    Dns::Name::Buffer                   serviceName;

    Log("ServiceCallback");
    Log("   Error: %s", ErrorToString(aError));

    VerifyOrQuit(aContext == sInstance);

    SuccessOrQuit(response.GetServiceName(instLabel, sizeof(instLabel), serviceName, sizeof(serviceName)));
    Log("   InstLabel: %s", instLabel);
    Log("   ServiceName: %s", serviceName);

    sResolveServiceInfo.mCallbackCount++;
    sResolveServiceInfo.mError = aError;

    SuccessOrExit(aError);
    SuccessOrQuit(response.GetServiceInfo(sResolveServiceInfo.mInfo));

    for (uint8_t index = 0; index < kMaxHostAddresses; index++)
    {
        Error    error;
        uint32_t ttl;

        error = response.GetHostAddress(sResolveServiceInfo.mInfo.mHostNameBuffer, index,
                                        sResolveServiceInfo.mHostAddresses[index], ttl);

        if (error == kErrorNotFound)
        {
            sResolveServiceInfo.mNumHostAddresses = index;
            break;
        }

        SuccessOrQuit(error);
    }

    LogServiceInfo(sResolveServiceInfo.mInfo);
    Log("   NumHostAddresses: %u", sResolveServiceInfo.mNumHostAddresses);

    for (uint8_t index = 0; index < sResolveServiceInfo.mNumHostAddresses; index++)
    {
        Log("      %s", sResolveServiceInfo.mHostAddresses[index].ToString().AsCString());
    }

exit:
    return;
}

//----------------------------------------------------------------------------------------------------------------------

void TestDnsClient(void)
{
    static constexpr uint8_t kNumAddresses = 2;

    static const char *const kAddresses[kNumAddresses] = {"2001::beef:cafe", "fd00:1234:5678:9abc::1"};

    const Dns::Client::QueryConfig::ServiceMode kServiceModes[] = {
        Dns::Client::QueryConfig::kServiceModeSrv,
        Dns::Client::QueryConfig::kServiceModeTxt,
        Dns::Client::QueryConfig::kServiceModeSrvTxt,
        Dns::Client::QueryConfig::kServiceModeSrvTxtSeparate,
        Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize,
    };

    Array<Ip6::Address, kNumAddresses> addresses;
    Srp::Server                       *srpServer;
    Srp::Client                       *srpClient;
    Srp::Client::Service               service1;
    Srp::Client::Service               service2;
    Dns::Client                       *dnsClient;
    Dns::Client::QueryConfig           queryConfig;
    Dns::ServiceDiscovery::Server     *dnsServer;
    uint16_t                           heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDnsClient");

    InitTest();

    for (const char *addrString : kAddresses)
    {
        otNetifAddress netifAddr;

        memset(&netifAddr, 0, sizeof(netifAddr));
        SuccessOrQuit(AsCoreType(&netifAddr.mAddress).FromString(addrString));
        netifAddr.mPrefixLength  = 64;
        netifAddr.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;
        netifAddr.mPreferred     = true;
        netifAddr.mValid         = true;
        SuccessOrQuit(otIp6AddUnicastAddress(sInstance, &netifAddr));

        SuccessOrQuit(addresses.PushBack(AsCoreType(&netifAddr.mAddress)));
    }

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register two services on SRP.

    SuccessOrQuit(srpClient->AddService(service1));
    SuccessOrQuit(srpClient->AddService(service2));

    AdvanceTime(2 * 1000);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);
    ValidateHost(*srpServer, kHostName);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check DNS Client's default config

    VerifyOrQuit(dnsClient->GetDefaultConfig().GetServiceMode() ==
                 Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate DNS Client `Browse()`

    sBrowseInfo.Reset();
    Log("Browse(%s)", kService1FullName);
    SuccessOrQuit(dnsClient->Browse(kService1FullName, BrowseCallback, sInstance));
    AdvanceTime(100);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    sBrowseInfo.Reset();

    Log("Browse(%s)", kService2FullName);
    SuccessOrQuit(dnsClient->Browse(kService2FullName, BrowseCallback, sInstance));
    AdvanceTime(100);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    sBrowseInfo.Reset();

    Log("Browse(%s)", kService2SubTypeFullName);
    SuccessOrQuit(dnsClient->Browse(kService2SubTypeFullName, BrowseCallback, sInstance));
    AdvanceTime(100);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    sBrowseInfo.Reset();
    Log("Browse() for unknown service");
    SuccessOrQuit(dnsClient->Browse("_unknown._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(100);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mError == kErrorNotFound);

    Log("Issue four parallel `Browse()` at the same time");
    sBrowseInfo.Reset();
    SuccessOrQuit(dnsClient->Browse(kService1FullName, BrowseCallback, sInstance));
    SuccessOrQuit(dnsClient->Browse(kService2FullName, BrowseCallback, sInstance));
    SuccessOrQuit(dnsClient->Browse("_unknown._udp.default.service.arpa.", BrowseCallback, sInstance));
    SuccessOrQuit(dnsClient->Browse("_unknown2._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(100);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 4);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate DNS Client `ResolveService()` using all service modes

    for (Dns::Client::QueryConfig::ServiceMode mode : kServiceModes)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("ResolveService(%s,%s) with ServiceMode: %s", kInstance1Label, kService1FullName,
            ServiceModeToString(mode));

        queryConfig.Clear();
        queryConfig.mServiceMode = static_cast<otDnsServiceMode>(mode);

        sResolveServiceInfo.Reset();
        SuccessOrQuit(
            dnsClient->ResolveService(kInstance1Label, kService1FullName, ServiceCallback, sInstance, &queryConfig));
        AdvanceTime(100);

        VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
        SuccessOrQuit(sResolveServiceInfo.mError);

        if (mode != Dns::Client::QueryConfig::kServiceModeTxt)
        {
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl != 0);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == service1.mPort);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mWeight == service1.mWeight);
            VerifyOrQuit(strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, kHostFullName) == 0);

            VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == kNumAddresses);
            VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress) == sResolveServiceInfo.mHostAddresses[0]);

            for (uint8_t index = 0; index < kNumAddresses; index++)
            {
                VerifyOrQuit(addresses.Contains(sResolveServiceInfo.mHostAddresses[index]));
            }
        }

        if (mode != Dns::Client::QueryConfig::kServiceModeSrv)
        {
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl != 0);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize != 0);
        }
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    Log("Set TestMode on server to only accept single question");
    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeSingleQuestionOnly);

    Log("ResolveService(%s,%s) with ServiceMode %s", kInstance1Label, kService1FullName,
        ServiceModeToString(Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize));

    queryConfig.Clear();
    queryConfig.mServiceMode = static_cast<otDnsServiceMode>(Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize);

    sResolveServiceInfo.Reset();
    SuccessOrQuit(
        dnsClient->ResolveService(kInstance1Label, kService1FullName, ServiceCallback, sInstance, &queryConfig));
    AdvanceTime(200);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveServiceInfo.mError);

    // Use `kServiceModeSrvTxt` and check that server does reject two questions.

    Log("ResolveService(%s,%s) with ServiceMode %s", kInstance1Label, kService1FullName,
        ServiceModeToString(Dns::Client::QueryConfig::kServiceModeSrvTxt));

    queryConfig.Clear();
    queryConfig.mServiceMode = static_cast<otDnsServiceMode>(Dns::Client::QueryConfig::kServiceModeSrvTxt);

    sResolveServiceInfo.Reset();
    SuccessOrQuit(
        dnsClient->ResolveService(kInstance1Label, kService1FullName, ServiceCallback, sInstance, &queryConfig));
    AdvanceTime(200);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveServiceInfo.mError != kErrorNone);

    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeDisabled);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate DNS Client `ResolveService()` using all service modes
    // when sever does not provide any RR in the addition data section.

    for (Dns::Client::QueryConfig::ServiceMode mode : kServiceModes)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
        Log("Set TestMode on server to not include any RR in additional section");
        dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeEmptyAdditionalSection);
        Log("ResolveService(%s,%s) with ServiceMode: %s", kInstance1Label, kService1FullName,
            ServiceModeToString(mode));

        queryConfig.Clear();
        queryConfig.mServiceMode = static_cast<otDnsServiceMode>(mode);

        sResolveServiceInfo.Reset();
        SuccessOrQuit(
            dnsClient->ResolveService(kInstance1Label, kService1FullName, ServiceCallback, sInstance, &queryConfig));
        AdvanceTime(100);

        VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
        SuccessOrQuit(sResolveServiceInfo.mError);

        if (mode != Dns::Client::QueryConfig::kServiceModeTxt)
        {
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl != 0);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == service1.mPort);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mWeight == service1.mWeight);
            VerifyOrQuit(strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, kHostFullName) == 0);
        }

        if (mode != Dns::Client::QueryConfig::kServiceModeSrv)
        {
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl != 0);
            VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize != 0);
        }

        // Since server is using `kTestModeEmptyAdditionalSection`, there
        // should be no AAAA records for host address.

        VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress).IsUnspecified());
        VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == 0);
    }

    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeDisabled);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate DNS Client `ResolveServiceAndHostAddress()` using all service modes
    // with different TestMode configs on server:
    // - Normal behavior when server provides AAAA records for host in
    //   additional section.
    // - Server provides no records in additional section. We validate that
    //   client will send separate query to resolve host address.

    for (Dns::Client::QueryConfig::ServiceMode mode : kServiceModes)
    {
        for (uint8_t testIter = 0; testIter <= 1; testIter++)
        {
            Error error;

            Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

            if (testIter == 1)
            {
                Log("Set TestMode on server to not include any RR in additional section");
                dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeEmptyAdditionalSection);
            }
            else
            {
                dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeDisabled);
            }

            Log("ResolveServiceAndHostAddress(%s,%s) with ServiceMode: %s", kInstance1Label, kService1FullName,
                ServiceModeToString(mode));

            queryConfig.Clear();
            queryConfig.mServiceMode = static_cast<otDnsServiceMode>(mode);

            sResolveServiceInfo.Reset();
            error = dnsClient->ResolveServiceAndHostAddress(kInstance1Label, kService1FullName, ServiceCallback,
                                                            sInstance, &queryConfig);

            if (mode == Dns::Client::QueryConfig::kServiceModeTxt)
            {
                Log("ResolveServiceAndHostAddress() with ServiceMode: %s failed correctly", ServiceModeToString(mode));
                VerifyOrQuit(error == kErrorInvalidArgs);
                continue;
            }

            SuccessOrQuit(error);

            AdvanceTime(100);

            VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
            SuccessOrQuit(sResolveServiceInfo.mError);

            if (mode != Dns::Client::QueryConfig::kServiceModeTxt)
            {
                VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl != 0);
                VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == service1.mPort);
                VerifyOrQuit(sResolveServiceInfo.mInfo.mWeight == service1.mWeight);
                VerifyOrQuit(strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, kHostFullName) == 0);

                VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == kNumAddresses);
                VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress) ==
                             sResolveServiceInfo.mHostAddresses[0]);

                for (uint8_t index = 0; index < kNumAddresses; index++)
                {
                    VerifyOrQuit(addresses.Contains(sResolveServiceInfo.mHostAddresses[index]));
                }
            }

            if (mode != Dns::Client::QueryConfig::kServiceModeSrv)
            {
                VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl != 0);
                VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize != 0);
            }
        }
    }

    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeDisabled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Set TestMode on server to not include any RR in additional section AND to only accept single question");
    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeEmptyAdditionalSection +
                           Dns::ServiceDiscovery::Server::kTestModeSingleQuestionOnly);

    Log("ResolveServiceAndHostAddress(%s,%s) with ServiceMode: %s", kInstance1Label, kService1FullName,
        ServiceModeToString(Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize));

    queryConfig.Clear();
    queryConfig.mServiceMode = static_cast<otDnsServiceMode>(Dns::Client::QueryConfig::kServiceModeSrvTxtOptimize);

    sResolveServiceInfo.Reset();
    SuccessOrQuit(dnsClient->ResolveServiceAndHostAddress(kInstance1Label, kService1FullName, ServiceCallback,
                                                          sInstance, &queryConfig));

    AdvanceTime(100);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveServiceInfo.mError);

    VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl != 0);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == service1.mPort);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mWeight == service1.mWeight);
    VerifyOrQuit(strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, kHostFullName) == 0);

    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl != 0);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize != 0);

    VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == kNumAddresses);
    VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress) == sResolveServiceInfo.mHostAddresses[0]);

    for (uint8_t index = 0; index < kNumAddresses; index++)
    {
        VerifyOrQuit(addresses.Contains(sResolveServiceInfo.mHostAddresses[index]));
    }

    dnsServer->SetTestMode(Dns::ServiceDiscovery::Server::kTestModeDisabled);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    Log("Stop DNS-SD server");
    dnsServer->Stop();

    Log("ResolveService(%s,%s) with ServiceMode %s", kInstance1Label, kService1FullName,
        ServiceModeToString(Dns::Client::QueryConfig::kServiceModeSrvTxtSeparate));

    queryConfig.Clear();
    queryConfig.mServiceMode = static_cast<otDnsServiceMode>(Dns::Client::QueryConfig::kServiceModeSrvTxtSeparate);

    sResolveServiceInfo.Reset();
    SuccessOrQuit(
        dnsClient->ResolveService(kInstance1Label, kService1FullName, ServiceCallback, sInstance, &queryConfig));
    AdvanceTime(25 * 1000);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveServiceInfo.mError == kErrorResponseTimeout);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // and/or by DNS Client are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestDnsClient");
}

//----------------------------------------------------------------------------------------------------------------------

Dns::Name::Buffer sLastSubscribeName;
Dns::Name::Buffer sLastUnsubscribeName;

void QuerySubscribe(void *aContext, const char *aFullName)
{
    uint16_t length = StringLength(aFullName, Dns::Name::kMaxNameSize);

    Log("QuerySubscribe(%s)", aFullName);

    VerifyOrQuit(aContext == sInstance);
    VerifyOrQuit(length < Dns::Name::kMaxNameSize);
    strcpy(sLastSubscribeName, aFullName);
}

void QueryUnsubscribe(void *aContext, const char *aFullName)
{
    uint16_t length = StringLength(aFullName, Dns::Name::kMaxNameSize);

    Log("QueryUnsubscribe(%s)", aFullName);

    VerifyOrQuit(aContext == sInstance);
    VerifyOrQuit(length < Dns::Name::kMaxNameSize);
    strcpy(sLastUnsubscribeName, aFullName);
}

void TestDnssdServerProxyCallback(void)
{
    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    otDnssdServiceInstanceInfo     instanceInfo;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestDnssdServerProxyCallback");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Set the query subscribe/unsubscribe callbacks on server

    dnsServer->SetQueryCallbacks(QuerySubscribe, QueryUnsubscribe, sInstance);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sLastSubscribeName[0]   = '\0';
    sLastUnsubscribeName[0] = '\0';

    sBrowseInfo.Reset();
    Log("Browse(%s)", kService1FullName);
    SuccessOrQuit(dnsClient->Browse(kService1FullName, BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(strcmp(sLastSubscribeName, kService1FullName) == 0);
    VerifyOrQuit(strcmp(sLastUnsubscribeName, "") == 0);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("Invoke subscribe callback");

    memset(&instanceInfo, 0, sizeof(instanceInfo));
    instanceInfo.mFullName = kInstance1FullName;
    instanceInfo.mHostName = kHostFullName;
    instanceInfo.mPort     = 200;

    dnsServer->HandleDiscoveredServiceInstance(kService1FullName, instanceInfo);

    AdvanceTime(10);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(strcmp(sLastUnsubscribeName, kService1FullName) == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sLastSubscribeName[0]   = '\0';
    sLastUnsubscribeName[0] = '\0';

    sBrowseInfo.Reset();
    Log("Browse(%s)", kService2FullName);
    SuccessOrQuit(dnsClient->Browse(kService2FullName, BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(strcmp(sLastSubscribeName, kService2FullName) == 0);
    VerifyOrQuit(strcmp(sLastUnsubscribeName, "") == 0);

    Log("Invoke subscribe callback for wrong name");

    memset(&instanceInfo, 0, sizeof(instanceInfo));
    instanceInfo.mFullName = kInstance1FullName;
    instanceInfo.mHostName = kHostFullName;
    instanceInfo.mPort     = 200;

    dnsServer->HandleDiscoveredServiceInstance(kService1FullName, instanceInfo);

    AdvanceTime(10);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("Invoke subscribe callback for correct name");

    memset(&instanceInfo, 0, sizeof(instanceInfo));
    instanceInfo.mFullName = kInstance2FullName;
    instanceInfo.mHostName = kHostFullName;
    instanceInfo.mPort     = 200;

    dnsServer->HandleDiscoveredServiceInstance(kService2FullName, instanceInfo);

    AdvanceTime(10);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(strcmp(sLastUnsubscribeName, kService2FullName) == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sLastSubscribeName[0]   = '\0';
    sLastUnsubscribeName[0] = '\0';

    sBrowseInfo.Reset();
    Log("Browse(%s)", kService2FullName);
    SuccessOrQuit(dnsClient->Browse(kService2FullName, BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(strcmp(sLastSubscribeName, kService2FullName) == 0);
    VerifyOrQuit(strcmp(sLastUnsubscribeName, "") == 0);

    Log("Do not invoke subscribe callback and let query to timeout");

    // Query timeout is set to 6 seconds

    AdvanceTime(5000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    AdvanceTime(2000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 0);

    VerifyOrQuit(strcmp(sLastUnsubscribeName, kService2FullName) == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sLastSubscribeName[0]   = '\0';
    sLastUnsubscribeName[0] = '\0';

    sBrowseInfo.Reset();
    Log("Browse(%s)", kService2FullName);
    SuccessOrQuit(dnsClient->Browse(kService2FullName, BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(strcmp(sLastSubscribeName, kService2FullName) == 0);
    VerifyOrQuit(strcmp(sLastUnsubscribeName, "") == 0);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("Do not invoke subscribe callback and stop server");

    dnsServer->Stop();

    AdvanceTime(10);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mError != kErrorNone);

    VerifyOrQuit(strcmp(sLastUnsubscribeName, kService2FullName) == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestDnssdServerProxyCallback");
}

#endif // ENABLE_DNS_TEST

int main(void)
{
#if ENABLE_DNS_TEST
    TestDnsClient();
    TestDnssdServerProxyCallback();
    printf("All tests passed\n");
#else
    printf("DNS_CLIENT or DSNSSD_SERVER feature is not enabled\n");
#endif

    return 0;
}
