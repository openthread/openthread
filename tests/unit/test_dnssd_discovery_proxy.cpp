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
#include "common/clearable.hpp"
#include "common/instance.hpp"
#include "common/string.hpp"
#include "common/time.hpp"

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE &&                 \
    OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE && OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE && \
    OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE && OPENTHREAD_CONFIG_SRP_SERVER_ENABLE &&                        \
    OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE && !OPENTHREAD_CONFIG_TIME_SYNC_ENABLE && !OPENTHREAD_PLATFORM_POSIX
#define ENABLE_DISCOVERY_PROXY_TEST 1
#else
#define ENABLE_DISCOVERY_PROXY_TEST 0
#endif

#if ENABLE_DISCOVERY_PROXY_TEST

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
// DNS Client callback

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

    uint16_t                 mCallbackCount;
    Error                    mError;
    char                     mServiceName[Dns::Name::kMaxNameSize];
    char                     mInstanceLabel[Dns::Name::kMaxLabelSize];
    uint16_t                 mNumInstances;
    Dns::Client::ServiceInfo mServiceInfo;
    char                     mHostNameBuffer[Dns::Name::kMaxNameSize];
    uint8_t                  mTxtBuffer[255];
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
        Error error;

        error = response.GetServiceInstance(index, sBrowseInfo.mInstanceLabel, sizeof(sBrowseInfo.mInstanceLabel));

        if (error == kErrorNotFound)
        {
            sBrowseInfo.mNumInstances = index;
            break;
        }

        SuccessOrQuit(error);

        Log("  %2u) %s", index + 1, sBrowseInfo.mInstanceLabel);
    }

    if (sBrowseInfo.mNumInstances == 1)
    {
        sBrowseInfo.mServiceInfo.mHostNameBuffer     = sBrowseInfo.mHostNameBuffer;
        sBrowseInfo.mServiceInfo.mHostNameBufferSize = sizeof(sBrowseInfo.mHostNameBuffer);
        sBrowseInfo.mServiceInfo.mTxtData            = sBrowseInfo.mTxtBuffer;
        sBrowseInfo.mServiceInfo.mTxtDataSize        = sizeof(sBrowseInfo.mTxtBuffer);

        SuccessOrExit(response.GetServiceInfo(sBrowseInfo.mInstanceLabel, sBrowseInfo.mServiceInfo));
        LogServiceInfo(sBrowseInfo.mServiceInfo);
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
    char                     mNameBuffer[Dns::Name::kMaxNameSize];
    uint8_t                  mTxtBuffer[kMaxTxtBuffer];
    Ip6::Address             mHostAddresses[kMaxHostAddresses];
    uint8_t                  mNumHostAddresses;
};

static ResolveServiceInfo sResolveServiceInfo;

void ServiceCallback(otError aError, const otDnsServiceResponse *aResponse, void *aContext)
{
    const Dns::Client::ServiceResponse &response = AsCoreType(aResponse);
    char                                instLabel[Dns::Name::kMaxLabelSize];
    char                                serviceName[Dns::Name::kMaxNameSize];

    Log("ServiceCallback");
    Log("   Error: %s", ErrorToString(aError));

    VerifyOrQuit(aContext == sInstance);

    SuccessOrQuit(response.GetServiceName(instLabel, sizeof(instLabel), serviceName, sizeof(serviceName)));
    Log("   InstLabel: %s", instLabel);
    Log("   ServiceName: %s", serviceName);

    sResolveServiceInfo.mCallbackCount++;
    sResolveServiceInfo.mError = aError;

    SuccessOrExit(aError);

    aError = response.GetServiceInfo(sResolveServiceInfo.mInfo);

    if (aError == kErrorNotFound)
    {
        sResolveServiceInfo.mError = aError;
        ExitNow();
    }

    SuccessOrQuit(aError);

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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct ResolveAddressInfo
{
    void Reset(void) { memset(this, 0, sizeof(*this)); };

    uint16_t     mCallbackCount;
    Error        mError;
    char         mHostName[Dns::Name::kMaxNameSize];
    Ip6::Address mHostAddresses[kMaxHostAddresses];
    uint8_t      mNumHostAddresses;
    uint32_t     mTtl;
};

static ResolveAddressInfo sResolveAddressInfo;

void AddressCallback(otError aError, const otDnsAddressResponse *aResponse, void *aContext)
{
    const Dns::Client::AddressResponse &response = AsCoreType(aResponse);

    Log("AddressCallback");
    Log("   Error: %s", ErrorToString(aError));

    VerifyOrQuit(aContext == sInstance);

    sResolveAddressInfo.mCallbackCount++;
    sResolveAddressInfo.mError = aError;

    SuccessOrExit(aError);

    SuccessOrQuit(response.GetHostName(sResolveAddressInfo.mHostName, sizeof(sResolveAddressInfo.mHostName)));
    Log("   HostName: %s", sResolveAddressInfo.mHostName);

    for (uint8_t index = 0; index < kMaxHostAddresses; index++)
    {
        Error    error;
        uint32_t ttl;

        error = response.GetAddress(index, sResolveAddressInfo.mHostAddresses[index], sResolveAddressInfo.mTtl);

        if (error == kErrorNotFound)
        {
            sResolveAddressInfo.mNumHostAddresses = index;
            break;
        }

        SuccessOrQuit(error);
    }

    Log("   NumHostAddresses: %u", sResolveAddressInfo.mNumHostAddresses);

    for (uint8_t index = 0; index < sResolveAddressInfo.mNumHostAddresses; index++)
    {
        Log("      %s", sResolveAddressInfo.mHostAddresses[index].ToString().AsCString());
    }

exit:
    return;
}

//----------------------------------------------------------------------------------------------------------------------
// otPlatDnssd APIs

constexpr uint32_t kInfraIfIndex = 1;

static otPlatDnssdState sDnssdState = OT_PLAT_DNSSD_READY;

template <uint16_t kSize> void CopyString(char (&aStringBuffer)[kSize], const char *aString)
{
    uint16_t length = StringLength(aString, kSize);

    VerifyOrQuit(length < kSize);
    memcpy(aStringBuffer, aString, length + 1);
}

struct BrowserInfo : public Clearable<BrowserInfo>
{
    void SetServiceType(const char *aType) { CopyString(mServiceType, aType); }
    bool ServiceTypeMatches(const char *aType) const { return !strcmp(mServiceType, aType); }

    uint16_t mCallCount;
    char     mServiceType[Dns::Name::kMaxNameSize];
};

struct ServiceResolverInfo : public Clearable<ServiceResolverInfo>
{
    void SetServiceType(const char *aType) { CopyString(mServiceType, aType); }
    void SetServiceInstance(const char *aInstance) { CopyString(mServiceInstance, aInstance); }

    bool ServiceTypeMatches(const char *aType) const { return !strcmp(mServiceType, aType); }
    bool ServiceInstanceMatches(const char *aInstance) const { return !strcmp(mServiceInstance, aInstance); }

    uint16_t mCallCount;
    char     mServiceInstance[Dns::Name::kMaxLabelSize];
    char     mServiceType[Dns::Name::kMaxNameSize];
};

struct Ip6AddrResolverInfo : public Clearable<Ip6AddrResolverInfo>
{
    void SetHostName(const char *aName) { CopyString(mHostName, aName); }
    bool HostNameMatches(const char *aName) const { return !strcmp(mHostName, aName); }

    uint16_t mCallCount;
    char     mHostName[Dns::Name::kMaxNameSize];
};

struct InvokeOnStart : public Clearable<InvokeOnStart>
{
    // When not null, these entries are used to invoke callback
    // directly from `otPlatDnssdStart{Browser/Resolver}()` APIs.
    // This is used in `TestProxyInvokeCallbackFromStartApi()`.

    const Dnssd::ServiceInstance *mServiceInstance;
    const Dnssd::Service         *mService;
    const Dnssd::Host            *mHost;
};

static BrowserInfo         sStartBrowserInfo;
static BrowserInfo         sStopBrowserInfo;
static ServiceResolverInfo sStartServiceResolverInfo;
static ServiceResolverInfo sStopServiceResolverInfo;
static Ip6AddrResolverInfo sStartIp6AddrResolverInfo;
static Ip6AddrResolverInfo sStopIp6AddrResolverInfo;

static InvokeOnStart sInvokeOnStart;

void ResetPlatDnssdApiInfo(void)
{
    sStartBrowserInfo.Clear();
    sStopBrowserInfo.Clear();
    sStartServiceResolverInfo.Clear();
    sStopServiceResolverInfo.Clear();
    sStartIp6AddrResolverInfo.Clear();
    sStopIp6AddrResolverInfo.Clear();
    sInvokeOnStart.Clear();
}

void InvokeBrowserCallback(const Dnssd::ServiceInstance &aServiceInstance, Dnssd::Event aEvent = Dnssd::kEventAdded)
{
    Log("Invoking otPlatDnssdHandleServiceBrowseResult()");
    Log("    event          : %s", aEvent == Dnssd::kEventAdded ? "added" : "removed");
    Log("    serviceType    : \"%s\"", aServiceInstance.mServiceType);
    Log("    serviceInstance: \"%s\"", aServiceInstance.mServiceInstance);
    Log("    ttl            : %u", aServiceInstance.mTtl);
    Log("    if-index       : %u", aServiceInstance.mInfraIfIndex);

    otPlatDnssdHandleServiceBrowseResult(sInstance, MapEnum(aEvent), &aServiceInstance);
}

void InvokeServiceResolverCallback(const Dnssd::Service &aService)
{
    Log("Invoking otPlatDnssdHandleServiceResolveResult()");
    Log("    serviceInstance: %s", aService.mServiceInstance);
    Log("    serviceType    : %s", aService.mServiceType);
    Log("    hostName       : %s", aService.mHostName);
    Log("    port           : %u", aService.mPort);
    Log("    priority       : %u", aService.mPriority);
    Log("    weight         : %u", aService.mWeight);
    Log("    txt data len   : %u", aService.mTxtDataLength);
    Log("    ttl            : %u", aService.mTtl);
    Log("    if-index       : %u", aService.mInfraIfIndex);

    otPlatDnssdHandleServiceResolveResult(sInstance, &aService);
}

void InvokeIp6AddrResolverCallback(const Dnssd::Host &aHost, Dnssd::Event aEvent = Dnssd::kEventAdded)
{
    Log("Invoking otPlatDnssdHandleIp6AddressResolveResult()");
    Log("    event          : %s", aEvent == Dnssd::kEventAdded ? "added" : "removed");
    Log("    hostName       : %s", aHost.mHostName);
    Log("    ttl            : %u", aHost.mTtl);
    Log("    if-index       : %u", aHost.mInfraIfIndex);
    Log("    numAddresses   : %u", aHost.mNumAddresses);
    for (uint16_t index = 0; index < aHost.mNumAddresses; index++)
    {
        Log("    address[%u]     : %s", index, AsCoreType(&aHost.mAddresses[index]).ToString().AsCString());
    }

    otPlatDnssdHandleIp6AddressResolveResult(sInstance, MapEnum(aEvent), &aHost);
}

otPlatDnssdState otPlatDnssdGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sDnssdState;
}

void otPlatDnssdStartServiceBrowser(otInstance *aInstance, const char *aServiceType, uint32_t aInfraIfIndex)
{
    Log("otPlatDnssdStartServiceBrowser(\"%s\")", aServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    sStartBrowserInfo.mCallCount++;
    sStartBrowserInfo.SetServiceType(aServiceType);

    if (sInvokeOnStart.mServiceInstance != nullptr)
    {
        InvokeBrowserCallback(*sInvokeOnStart.mServiceInstance);
    }
}

void otPlatDnssdStopServiceBrowser(otInstance *aInstance, const char *aServiceType, uint32_t aInfraIfIndex)
{
    uint16_t length;

    Log("otPlatDnssdStopServiceBrowser(\"%s\")", aServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    sStopBrowserInfo.mCallCount++;
    sStopBrowserInfo.SetServiceType(aServiceType);
}

void otPlatDnssdStartServiceResolver(otInstance *aInstance, const otPlatDnssdServiceInstance *aServiceInstance)
{
    uint16_t length;

    Log("otPlatDnssdStartServiceResolver(\"%s\", \"%s\")", aServiceInstance->mServiceInstance,
        aServiceInstance->mServiceType);

    VerifyOrQuit(aInstance == aInstance);
    VerifyOrQuit(aServiceInstance->mInfraIfIndex == kInfraIfIndex);

    sStartServiceResolverInfo.mCallCount++;
    sStartServiceResolverInfo.SetServiceType(aServiceInstance->mServiceType);
    sStartServiceResolverInfo.SetServiceInstance(aServiceInstance->mServiceInstance);

    if (sInvokeOnStart.mService != nullptr)
    {
        InvokeServiceResolverCallback(*sInvokeOnStart.mService);
    }
}

void otPlatDnssdStopServiceResolver(otInstance *aInstance, const otPlatDnssdServiceInstance *aServiceInstance)
{
    Log("otPlatDnssdStopServiceResolver(\"%s\", \"%s\")", aServiceInstance->mServiceInstance,
        aServiceInstance->mServiceType);

    VerifyOrQuit(aInstance == aInstance);
    VerifyOrQuit(aServiceInstance->mInfraIfIndex == kInfraIfIndex);

    sStopServiceResolverInfo.mCallCount++;
    sStopServiceResolverInfo.SetServiceType(aServiceInstance->mServiceType);
    sStopServiceResolverInfo.SetServiceInstance(aServiceInstance->mServiceInstance);
}

void otPlatDnssdStartIp6AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex)
{
    Log("otPlatDnssdStartIp6AddressResolver(\"%s\")", aHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    sStartIp6AddrResolverInfo.mCallCount++;
    sStartIp6AddrResolverInfo.SetHostName(aHostName);

    if (sInvokeOnStart.mHost != nullptr)
    {
        InvokeIp6AddrResolverCallback(*sInvokeOnStart.mHost);
    }
}

void otPlatDnssdStopIp6AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex)
{
    Log("otPlatDnssdStopIp6AddressResolver(\"%s\")", aHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aInfraIfIndex == kInfraIfIndex);

    sStopIp6AddrResolverInfo.mCallCount++;
    sStopIp6AddrResolverInfo.SetHostName(aHostName);
}

//----------------------------------------------------------------------------------------------------------------------

void TestProxyBasic(void)
{
    static constexpr uint32_t kTtl = 300;

    const uint8_t kTxtData[] = {3, 'A', '=', '1', 0};

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::ServiceInstance         serviceInstance;
    Dnssd::Service                 service;
    Dnssd::Host                    host;
    Ip6::Address                   address;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxyBasic");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    serviceInstance.mServiceType     = sStartBrowserInfo.mServiceType;
    serviceInstance.mServiceInstance = "hulk";
    serviceInstance.mTtl             = kTtl;
    serviceInstance.mInfraIfIndex    = kInfraIfIndex;
    InvokeBrowserCallback(serviceInstance);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("hulk"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    service.Clear();
    service.mHostName        = "compound";
    service.mServiceInstance = "hulk";
    service.mServiceType     = "_avenger._udp";
    service.mTxtData         = kTxtData;
    service.mTxtDataLength   = sizeof(kTxtData);
    service.mPort            = 7777;
    service.mTtl             = kTtl;
    service.mInfraIfIndex    = kInfraIfIndex;
    InvokeServiceResolverCallback(service);

    AdvanceTime(10);

    // Check that service resolver is stopped and an address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("hulk"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("compound"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(address.FromString("fd00::1234"));

    host.Clear();
    host.mHostName     = "compound";
    host.mAddresses    = &address;
    host.mNumAddresses = 1;
    host.mTtl          = kTtl;
    host.mInfraIfIndex = kInfraIfIndex;
    InvokeIp6AddrResolverCallback(host);

    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("compound"));

    // Check that response is sent to client and validate it

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_avenger._udp.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "hulk"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "compound.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 7777);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sBrowseInfo.mServiceInfo.mHostAddress) == address);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sBrowseInfo.mServiceInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sBrowseInfo.mServiceInfo.mTxtDataTruncated);

    Log("--------------------------------------------------------------------------------------------");

    ResetPlatDnssdApiInfo();
    sResolveServiceInfo.Reset();

    Log("ResolveService() with dot `.` character in service instance label");
    SuccessOrQuit(
        dnsClient->ResolveService("iron.man", "_avenger._udp.default.service.arpa.", ServiceCallback, sInstance));
    AdvanceTime(10);

    // Check that a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("iron.man"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback for wrong name");

    service.mServiceInstance = "hulk";
    service.mPort            = 7777;
    InvokeServiceResolverCallback(service);

    AdvanceTime(10);

    // Check that no changes to browsers/resolvers

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);

    Log("Invoke Service Resolver callback for correct name");

    service.mHostName        = "starktower";
    service.mServiceInstance = "iron.man";
    service.mPort            = 1024;
    InvokeServiceResolverCallback(service);

    AdvanceTime(10);

    // Check that service resolver is stopped and addr resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("iron.man"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("starktower"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    host.mHostName = "starktower";
    InvokeIp6AddrResolverCallback(host);
    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("starktower"));

    // Check that response is sent to client and validate it

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveServiceInfo.mError == kErrorNone);

    VerifyOrQuit(!strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, "starktower.default.service.arpa."));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == 1024);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress) == address);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sResolveServiceInfo.mInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sResolveServiceInfo.mInfo.mTxtDataTruncated);

    Log("--------------------------------------------------------------------------------------------");

    ResetPlatDnssdApiInfo();
    sResolveAddressInfo.Reset();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("earth.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that an address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("earth"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(address.FromString("fd00::7777"));

    host.mHostName = "earth";
    InvokeIp6AddrResolverCallback(host);
    AdvanceTime(10);

    // Check that the address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("earth"));

    // Check that response is sent to client and validate it

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "earth.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 1);
    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == address);
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxyBasic");
}

//----------------------------------------------------------------------------------------------------------------------

void TestProxySubtypeBrowse(void)
{
    static constexpr uint32_t kTtl = 300;

    const uint8_t kTxtData[] = {3, 'G', '=', '0', 0};

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::ServiceInstance         serviceInstance;
    Dnssd::Service                 service;
    Dnssd::Host                    host;
    Ip6::Address                   address;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxySubtypeBrowse");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse() for sub-type service");
    SuccessOrQuit(dnsClient->Browse("_god._sub._avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_god._sub._avenger._udp"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    serviceInstance.mServiceType     = sStartBrowserInfo.mServiceType;
    serviceInstance.mServiceInstance = "thor";
    serviceInstance.mTtl             = kTtl;
    serviceInstance.mInfraIfIndex    = kInfraIfIndex;
    InvokeBrowserCallback(serviceInstance);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_god._sub._avenger._udp"));

    // Check that the service resolver is correctly using the base service type
    // and not the sub-type name

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("thor"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    service.Clear();
    service.mHostName        = "asgard";
    service.mServiceInstance = "thor";
    service.mServiceType     = "_avenger._udp";
    service.mTxtData         = kTxtData;
    service.mTxtDataLength   = sizeof(kTxtData);
    service.mPort            = 1234;
    service.mTtl             = kTtl;
    service.mInfraIfIndex    = kInfraIfIndex;
    InvokeServiceResolverCallback(service);

    AdvanceTime(10);

    // Check that service resolver is stopped and an address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("thor"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("asgard"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(address.FromString("fd00::1234"));

    host.Clear();
    host.mHostName     = "asgard";
    host.mAddresses    = &address;
    host.mNumAddresses = 1;
    host.mTtl          = kTtl;
    host.mInfraIfIndex = kInfraIfIndex;
    InvokeIp6AddrResolverCallback(host);

    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("asgard"));

    // Check that response is sent to client and validate it

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_god._sub._avenger._udp.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "thor"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "asgard.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 1234);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sBrowseInfo.mServiceInfo.mHostAddress) == address);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sBrowseInfo.mServiceInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sBrowseInfo.mServiceInfo.mTxtDataTruncated);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxySubtypeBrowse");
}

//----------------------------------------------------------------------------------------------------------------------

void TestProxyTimeout(void)
{
    static constexpr uint32_t kTtl = 300;

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::ServiceInstance         serviceInstance;
    Dns::Client::QueryConfig       config;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxyTimeout");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Increase default response retry timeout on DNS client");

    config.Clear();
    config.mResponseTimeout = 120 * 1000; // 2 minutes (in msec)
    dnsClient->SetDefaultConfig(config);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_game._ps5.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_game._ps5"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for timeout and check empty response on client");

    AdvanceTime(10 * 1000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 0);

    // Check that the browser is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_game._ps5"));

    Log("--------------------------------------------------------------------------------------------");
    Log("Timeout during service resolution");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    SuccessOrQuit(dnsClient->Browse("_avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    serviceInstance.mServiceType     = sStartBrowserInfo.mServiceType;
    serviceInstance.mServiceInstance = "spiderman";
    serviceInstance.mTtl             = kTtl;
    serviceInstance.mInfraIfIndex    = kInfraIfIndex;
    InvokeBrowserCallback(serviceInstance);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("spiderman"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for timeout");

    AdvanceTime(10 * 1000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    // Check that the browser is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    // Validate the response received by client

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_avenger._udp.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "spiderman"));

    Log("--------------------------------------------------------------------------------------------");
    Log("Timeout during multiple requests");

    sBrowseInfo.Reset();
    sResolveServiceInfo.Reset();
    sResolveAddressInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Another Browse()");
    SuccessOrQuit(dnsClient->Browse("_game._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_game._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("ResolveService()");
    SuccessOrQuit(
        dnsClient->ResolveService("wanda", "_avenger._udp.default.service.arpa.", ServiceCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("wanda"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("earth.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("earth"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for timeout for all requests");

    AdvanceTime(10 * 1000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 2);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 0);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveServiceInfo.mError == kErrorNotFound);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 0);

    // Check that all browsers/resolvers are stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("wanda"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("earth"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxyTimeout");
}

void TestProxySharedResolver(void)
{
    static constexpr uint32_t kTtl = 300;

    const uint8_t kTxtData[] = {3, 'A', '=', '1', 0};

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::ServiceInstance         serviceInstance;
    Dnssd::Service                 service;
    Dnssd::Host                    host;
    Ip6::Address                   addresses[2];

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxySharedResolver");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    sResolveServiceInfo.Reset();
    sResolveAddressInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("knowhere.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    Log("ResolveService()");
    SuccessOrQuit(
        dnsClient->ResolveService("starlord", "_guardian._glaxy.default.service.arpa.", ServiceCallback, sInstance));
    AdvanceTime(10);

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_guardian._glaxy.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("starlord"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("knowhere"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    serviceInstance.mServiceType     = "_guardian._glaxy";
    serviceInstance.mServiceInstance = "starlord";
    serviceInstance.mTtl             = kTtl;
    serviceInstance.mInfraIfIndex    = kInfraIfIndex;
    InvokeBrowserCallback(serviceInstance);

    AdvanceTime(10);

    // Check that browser is stopped and since the service instance
    // name matches an existing resolver, we should not see any new
    // resolver starting.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    service.Clear();
    service.mHostName        = "knowhere";
    service.mServiceInstance = "starlord";
    service.mServiceType     = "_guardian._glaxy";
    service.mTxtData         = kTxtData;
    service.mTxtDataLength   = sizeof(kTxtData);
    service.mPort            = 3333;
    service.mTtl             = kTtl;
    service.mInfraIfIndex    = kInfraIfIndex;
    InvokeServiceResolverCallback(service);

    AdvanceTime(10);

    // Check that service resolver is now stopped but again since the
    // host name matches an existing address resolver we should not
    // see any new address resolver.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("starlord"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(addresses[0].FromString("fd00::5555"));
    SuccessOrQuit(addresses[1].FromString("fd00::1234"));

    host.Clear();
    host.mHostName     = "knowhere";
    host.mAddresses    = &addresses[0];
    host.mNumAddresses = 2;
    host.mTtl          = kTtl;
    host.mInfraIfIndex = kInfraIfIndex;
    InvokeIp6AddrResolverCallback(host);

    AdvanceTime(10);

    // Check that the address resolver is now stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("knowhere"));

    // Check the browse response received on client

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_guardian._glaxy.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "starlord"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "knowhere.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 3333);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sBrowseInfo.mServiceInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sBrowseInfo.mServiceInfo.mTxtDataTruncated);

    // Check the service resolve response received on client

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveServiceInfo.mError);

    VerifyOrQuit(!strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, "knowhere.default.service.arpa."));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == 3333);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sResolveServiceInfo.mInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sResolveServiceInfo.mInfo.mTxtDataTruncated);
    VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == 2);
    for (uint16_t index = 0; index < 2; index++)
    {
        VerifyOrQuit(sResolveServiceInfo.mHostAddresses[index] == addresses[0] ||
                     sResolveServiceInfo.mHostAddresses[index] == addresses[1]);
    }

    // Check the address resolve response received on client

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "knowhere.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 2);
    for (uint16_t index = 0; index < 2; index++)
    {
        VerifyOrQuit(sResolveAddressInfo.mHostAddresses[index] == addresses[0] ||
                     sResolveAddressInfo.mHostAddresses[index] == addresses[1]);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxySharedResolver");
}

void TestProxyFilterInvalidAddresses(void)
{
    static constexpr uint32_t kTtl = 300;

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::Host                    host;
    Ip6::Address                   addresses[10];

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxyFilterInvalidAddresses");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sResolveAddressInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("host.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("host"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback with invalid addresses");

    SuccessOrQuit(addresses[0].FromString("::"));         // Unspecified
    SuccessOrQuit(addresses[1].FromString("fe80::1234")); // Link local
    SuccessOrQuit(addresses[2].FromString("ff00::1234")); // Multicast
    SuccessOrQuit(addresses[3].FromString("::1"));        // Lookback

    host.Clear();
    host.mHostName     = "host";
    host.mAddresses    = &addresses[0];
    host.mNumAddresses = 4;
    host.mTtl          = kTtl;
    host.mInfraIfIndex = kInfraIfIndex;
    InvokeIp6AddrResolverCallback(host);

    AdvanceTime(10);

    // Check that the address resolver is not stopped, since all addresses where
    // invalid address.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback with invalid addresses with one valid");

    SuccessOrQuit(addresses[4].FromString("fd00::1234"));
    host.mNumAddresses = 5;

    InvokeIp6AddrResolverCallback(host);

    AdvanceTime(10);

    // Check that address resolver is not stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("host"));

    // Check that response received on client is valid and only contains
    // the valid two addresses and filters all others.

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "host.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 1);
    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == addresses[4]);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxyFilterInvalidAddresses");
}

void TestProxyStateChanges(void)
{
    static constexpr uint32_t kTtl = 300;

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxyStateChanges");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal DNS-SD platform state is stopped and not yet ready");

    sDnssdState = OT_PLAT_DNSSD_STOPPED;
    otPlatDnssdStateHandleStateChange(sInstance);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sResolveAddressInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("host.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that none of the DNS-SD resolver/browser APIs are called
    // since the platform is not yet ready

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal DNS-SD platform state is now ready");

    sDnssdState = OT_PLAT_DNSSD_READY;
    otPlatDnssdStateHandleStateChange(sInstance);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sResolveAddressInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("host.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("host"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_magic._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    // Check that browser is also started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_magic._udp"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal infra-if is not running");

    SuccessOrQuit(otPlatInfraIfStateChanged(sInstance, kInfraIfIndex, /* aIsRunning */ false));

    AdvanceTime(10);

    // Check that both address resolver and browser are stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("host"));
    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_magic._udp"));

    // And response is sent to client

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sResolveAddressInfo.Reset();

    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("earth.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that no resolver is started.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal that infra-if is running again ");

    SuccessOrQuit(otPlatInfraIfStateChanged(sInstance, kInfraIfIndex, /* aIsRunning */ true));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sResolveServiceInfo.Reset();
    Log("ResolveService()");
    SuccessOrQuit(dnsClient->ResolveService("captain.america", "_avenger._udp.default.service.arpa.", ServiceCallback,
                                            sInstance));
    AdvanceTime(10);

    // The proxy should be started again so check that a service resolver
    // is started for new request

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("captain.america"));

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal DNS-SD platform state is stopped");

    sDnssdState = OT_PLAT_DNSSD_STOPPED;
    otPlatDnssdStateHandleStateChange(sInstance);

    AdvanceTime(10);

    // This should stop proxy but since DNS-SD platform is stopped
    // we assume all browsers/resolvers are also stopped, so there
    // should be no explicit call to stop it.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    // Check that response is sent to client

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Signal DNS-SD platform state is ready again");

    sDnssdState = OT_PLAT_DNSSD_READY;
    otPlatDnssdStateHandleStateChange(sInstance);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_magical._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    // Proxy should be started again and we should see a new browser started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_magical._udp"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    // Check that the browser is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_magical._udp"));

    // And response is sent to client

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxyStateChanges");
}

void TestProxyInvokeCallbackFromStartApi(void)
{
    static constexpr uint32_t kTtl = 300;

    const uint8_t kTxtData[] = {3, 'A', '=', '1', 0};

    Srp::Server                   *srpServer;
    Srp::Client                   *srpClient;
    Dns::Client                   *dnsClient;
    Dns::ServiceDiscovery::Server *dnsServer;
    Dnssd::ServiceInstance         serviceInstance;
    Dnssd::Service                 service;
    Dnssd::Host                    host;
    Ip6::Address                   addresses[2];

    Log("--------------------------------------------------------------------------------------------");
    Log("TestProxyInvokeCallbackFromStartApi");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();
    dnsClient = &sInstance->Get<Dns::Client>();
    dnsServer = &sInstance->Get<Dns::ServiceDiscovery::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(otBorderRoutingInit(sInstance, /* aInfraIfIndex */ kInfraIfIndex, /* aInfraIfIsRunning */ true));

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

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Enable invoking of callback directly from otPlatDnssdStart{Browsers/Resolver} APIs");

    ResetPlatDnssdApiInfo();

    sInvokeOnStart.mServiceInstance = &serviceInstance;
    sInvokeOnStart.mService         = &service;
    sInvokeOnStart.mHost            = &host;

    serviceInstance.mServiceType     = "_guardian._glaxy";
    serviceInstance.mServiceInstance = "mantis";
    serviceInstance.mTtl             = kTtl;
    serviceInstance.mInfraIfIndex    = kInfraIfIndex;

    service.Clear();
    service.mHostName        = "nova";
    service.mServiceInstance = "mantis";
    service.mServiceType     = "_guardian._glaxy";
    service.mTxtData         = kTxtData;
    service.mTxtDataLength   = sizeof(kTxtData);
    service.mPort            = 3333;
    service.mTtl             = kTtl;
    service.mInfraIfIndex    = kInfraIfIndex;

    SuccessOrQuit(addresses[0].FromString("fd00::5555"));
    SuccessOrQuit(addresses[1].FromString("fd00::1234"));

    host.Clear();
    host.mHostName     = "nova";
    host.mAddresses    = &addresses[0];
    host.mNumAddresses = 2;
    host.mTtl          = kTtl;
    host.mInfraIfIndex = kInfraIfIndex;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sBrowseInfo.Reset();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_guardian._glaxy.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    // All browsers/resolvers should be started and stopped
    // (since the callbacks are invoked directly from API)

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("mantis"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("nova"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("nova"));

    // Check that response is revived by client and validate it

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_guardian._glaxy.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "mantis"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "nova.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 3333);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sBrowseInfo.mServiceInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sBrowseInfo.mServiceInfo.mTxtDataTruncated);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sResolveServiceInfo.Reset();
    Log("ResolveService()");
    SuccessOrQuit(
        dnsClient->ResolveService("mantis", "_guardian._glaxy.default.service.arpa.", ServiceCallback, sInstance));
    AdvanceTime(10);

    // Check that new service resolver and address resolvers are
    // started and stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 2);

    VerifyOrQuit(sStartServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartServiceResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopServiceResolverInfo.ServiceInstanceMatches("mantis"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("nova"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("nova"));

    // Check the service resolve response received on client

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveServiceInfo.mError);

    VerifyOrQuit(!strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, "nova.default.service.arpa."));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == 3333);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mHostAddressTtl == kTtl);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataSize == sizeof(kTxtData));
    VerifyOrQuit(!memcmp(sResolveServiceInfo.mInfo.mTxtData, kTxtData, sizeof(kTxtData)));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTxtDataTtl == kTtl);
    VerifyOrQuit(!sResolveServiceInfo.mInfo.mTxtDataTruncated);
    VerifyOrQuit(sResolveServiceInfo.mNumHostAddresses == 2);
    for (uint16_t index = 0; index < 2; index++)
    {
        VerifyOrQuit(sResolveServiceInfo.mHostAddresses[index] == addresses[0] ||
                     sResolveServiceInfo.mHostAddresses[index] == addresses[1]);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sResolveAddressInfo.Reset();
    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("nova.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that new address resolver is started and stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartServiceResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopServiceResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 3);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 3);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("nova"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("nova"));

    // Check the address resolve response received on client

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "nova.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 2);
    for (uint16_t index = 0; index < 2; index++)
    {
        VerifyOrQuit(sResolveAddressInfo.mHostAddresses[index] == addresses[0] ||
                     sResolveAddressInfo.mHostAddresses[index] == addresses[1]);
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Stop DNS-SD server");

    dnsServer->Stop();

    AdvanceTime(10);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestProxyInvokeCallbackFromStartApi");
}

#endif // ENABLE_DISCOVERY_PROXY_TEST

int main(void)
{
#if ENABLE_DISCOVERY_PROXY_TEST
    TestProxyBasic();
    TestProxySubtypeBrowse();
    TestProxyTimeout();
    TestProxySharedResolver();
    TestProxyFilterInvalidAddresses();
    TestProxyStateChanges();
    TestProxyInvokeCallbackFromStartApi();
    printf("All tests passed\n");
#else
    printf("DISCOVERY_PROXY feature is not enabled\n");
#endif

    return 0;
}
