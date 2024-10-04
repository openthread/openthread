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
#include "common/string.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE && OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE &&                 \
    OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE && OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE && \
    OPENTHREAD_CONFIG_DNSSD_DISCOVERY_PROXY_ENABLE && OPENTHREAD_CONFIG_SRP_SERVER_ENABLE &&                        \
    OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION && OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE &&             \
    !OPENTHREAD_CONFIG_TIME_SYNC_ENABLE && !OPENTHREAD_PLATFORM_POSIX
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
    otDatasetConvertToTlvs(&dataset, &datasetTlvs);
    SuccessOrQuit(otDatasetSetActiveTlvs(sInstance, &datasetTlvs));

    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));

    // Configure the `Dnssd` module to use `otPlatDnssd` APIs.

    sInstance->Get<Dnssd>().SetUseNativeMdns(false);

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
    if (aString == nullptr)
    {
        memset(aStringBuffer, 0, kSize);
    }
    else
    {
        uint16_t length = StringLength(aString, kSize);

        VerifyOrQuit(length < kSize);
        memcpy(aStringBuffer, aString, length + 1);
    }
}

struct BrowserInfo : public Clearable<BrowserInfo>
{
    bool ServiceTypeMatches(const char *aType) const { return !strcmp(mServiceType, aType); }
    bool SubTypeMatches(const char *aSubType) const { return !strcmp(mSubTypeLabel, aSubType); }

    void UpdateFrom(const otPlatDnssdBrowser *aBrowser)
    {
        mCallCount++;
        CopyString(mServiceType, aBrowser->mServiceType);
        CopyString(mSubTypeLabel, aBrowser->mSubTypeLabel);
        mCallback = aBrowser->mCallback;
    }

    uint16_t                  mCallCount;
    char                      mServiceType[Dns::Name::kMaxNameSize];
    char                      mSubTypeLabel[Dns::Name::kMaxNameSize];
    otPlatDnssdBrowseCallback mCallback;
};

struct SrvResolverInfo : public Clearable<SrvResolverInfo>
{
    bool ServiceTypeMatches(const char *aType) const { return !strcmp(mServiceType, aType); }
    bool ServiceInstanceMatches(const char *aInstance) const { return !strcmp(mServiceInstance, aInstance); }

    void UpdateFrom(const otPlatDnssdSrvResolver *aResolver)
    {
        mCallCount++;
        CopyString(mServiceInstance, aResolver->mServiceInstance);
        CopyString(mServiceType, aResolver->mServiceType);
        mCallback = aResolver->mCallback;
    }

    uint16_t               mCallCount;
    char                   mServiceInstance[Dns::Name::kMaxLabelSize];
    char                   mServiceType[Dns::Name::kMaxNameSize];
    otPlatDnssdSrvCallback mCallback;
};

struct TxtResolverInfo : public Clearable<TxtResolverInfo>
{
    bool ServiceTypeMatches(const char *aType) const { return !strcmp(mServiceType, aType); }
    bool ServiceInstanceMatches(const char *aInstance) const { return !strcmp(mServiceInstance, aInstance); }

    void UpdateFrom(const otPlatDnssdTxtResolver *aResolver)
    {
        mCallCount++;
        CopyString(mServiceInstance, aResolver->mServiceInstance);
        CopyString(mServiceType, aResolver->mServiceType);
        mCallback = aResolver->mCallback;
    }

    uint16_t               mCallCount;
    char                   mServiceInstance[Dns::Name::kMaxLabelSize];
    char                   mServiceType[Dns::Name::kMaxNameSize];
    otPlatDnssdTxtCallback mCallback;
};

struct IpAddrResolverInfo : public Clearable<IpAddrResolverInfo>
{
    bool HostNameMatches(const char *aName) const { return !strcmp(mHostName, aName); }

    void UpdateFrom(const otPlatDnssdAddressResolver *aResolver)
    {
        mCallCount++;
        CopyString(mHostName, aResolver->mHostName);
        mCallback = aResolver->mCallback;
    }

    uint16_t                   mCallCount;
    char                       mHostName[Dns::Name::kMaxNameSize];
    otPlatDnssdAddressCallback mCallback;
};

struct InvokeOnStart : public Clearable<InvokeOnStart>
{
    // When not null, these entries are used to invoke callback
    // directly from `otPlatDnssdStart{Browser/Resolver}()` APIs.
    // This is used in `TestProxyInvokeCallbackFromStartApi()`.

    const otPlatDnssdBrowseResult  *mBrowseResult;
    const otPlatDnssdSrvResult     *mSrvResult;
    const otPlatDnssdTxtResult     *mTxtResult;
    const otPlatDnssdAddressResult *mIp6AddrResult;
    const otPlatDnssdAddressResult *mIp4AddrResult;
};

static BrowserInfo        sStartBrowserInfo;
static BrowserInfo        sStopBrowserInfo;
static SrvResolverInfo    sStartSrvResolverInfo;
static SrvResolverInfo    sStopSrvResolverInfo;
static TxtResolverInfo    sStartTxtResolverInfo;
static TxtResolverInfo    sStopTxtResolverInfo;
static IpAddrResolverInfo sStartIp6AddrResolverInfo;
static IpAddrResolverInfo sStopIp6AddrResolverInfo;
static IpAddrResolverInfo sStartIp4AddrResolverInfo;
static IpAddrResolverInfo sStopIp4AddrResolverInfo;

static InvokeOnStart sInvokeOnStart;

void ResetPlatDnssdApiInfo(void)
{
    sStartBrowserInfo.Clear();
    sStopBrowserInfo.Clear();
    sStartSrvResolverInfo.Clear();
    sStopSrvResolverInfo.Clear();
    sStartTxtResolverInfo.Clear();
    sStopTxtResolverInfo.Clear();
    sStartIp6AddrResolverInfo.Clear();
    sStopIp6AddrResolverInfo.Clear();
    sStartIp4AddrResolverInfo.Clear();
    sStopIp4AddrResolverInfo.Clear();
    sInvokeOnStart.Clear();
}

const char *StringNullCheck(const char *aString) { return (aString != nullptr) ? aString : "(null)"; }

void InvokeBrowserCallback(otPlatDnssdBrowseCallback aCallback, const otPlatDnssdBrowseResult &aResult)
{
    Log("Invoking browser callback");
    Log("    serviceType    : \"%s\"", StringNullCheck(aResult.mServiceType));
    Log("    subType        : \"%s\"", StringNullCheck(aResult.mSubTypeLabel));
    Log("    serviceInstance: \"%s\"", StringNullCheck(aResult.mServiceInstance));
    Log("    ttl            : %u", aResult.mTtl);
    Log("    if-index       : %u", aResult.mInfraIfIndex);

    aCallback(sInstance, &aResult);
}

void InvokeSrvResolverCallback(otPlatDnssdSrvCallback aCallback, const otPlatDnssdSrvResult &aResult)
{
    Log("Invoking SRV resolver callback");
    Log("    serviceInstance: %s", StringNullCheck(aResult.mServiceInstance));
    Log("    serviceType    : %s", StringNullCheck(aResult.mServiceType));
    Log("    hostName       : %s", StringNullCheck(aResult.mHostName));
    Log("    port           : %u", aResult.mPort);
    Log("    priority       : %u", aResult.mPriority);
    Log("    weight         : %u", aResult.mWeight);
    Log("    ttl            : %u", aResult.mTtl);
    Log("    if-index       : %u", aResult.mInfraIfIndex);

    aCallback(sInstance, &aResult);
}

void InvokeTxtResolverCallback(otPlatDnssdTxtCallback aCallback, const otPlatDnssdTxtResult &aResult)
{
    Log("Invoking TXT resolver callback");
    Log("    serviceInstance: %s", StringNullCheck(aResult.mServiceInstance));
    Log("    serviceType    : %s", StringNullCheck(aResult.mServiceType));
    Log("    txt-data-len   : %u", aResult.mTxtDataLength);
    Log("    ttl            : %u", aResult.mTtl);
    Log("    if-index       : %u", aResult.mInfraIfIndex);

    aCallback(sInstance, &aResult);
}

void InvokeIp6AddrResolverCallback(const otPlatDnssdAddressCallback aCallback, const otPlatDnssdAddressResult &aResult)
{
    Log("Invoking Ip6 resolver callback");
    Log("    hostName       : %s", aResult.mHostName);
    Log("    if-index       : %u", aResult.mInfraIfIndex);
    Log("    numAddresses   : %u", aResult.mAddressesLength);
    for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
    {
        Log("    address[%u]     : %s", index, AsCoreType(&aResult.mAddresses[index].mAddress).ToString().AsCString());
        Log("    ttl[%u]         : %u", index, aResult.mAddresses[index].mTtl);
    }

    aCallback(sInstance, &aResult);
}

void InvokeIp4AddrResolverCallback(const otPlatDnssdAddressCallback aCallback, const otPlatDnssdAddressResult &aResult)
{
    Log("Invoking Ip4 resolver callback");
    Log("    hostName       : %s", aResult.mHostName);
    Log("    if-index       : %u", aResult.mInfraIfIndex);
    Log("    numAddresses   : %u", aResult.mAddressesLength);
    for (uint16_t index = 0; index < aResult.mAddressesLength; index++)
    {
        Log("    address[%u]     : %s", index, AsCoreType(&aResult.mAddresses[index].mAddress).ToString().AsCString());
        Log("    ttl[%u]         : %u", index, aResult.mAddresses[index].mTtl);
    }

    aCallback(sInstance, &aResult);
}

otPlatDnssdState otPlatDnssdGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sDnssdState;
}

void otPlatDnssdStartBrowser(otInstance *aInstance, const otPlatDnssdBrowser *aBrowser)
{
    VerifyOrQuit(aBrowser != nullptr);

    Log("otPlatDnssdStartBrowser(\"%s\", sub-type:\"%s\")", aBrowser->mServiceType, aBrowser->mSubTypeLabel);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aBrowser->mInfraIfIndex == kInfraIfIndex);

    sStartBrowserInfo.UpdateFrom(aBrowser);

    if (sInvokeOnStart.mBrowseResult != nullptr)
    {
        InvokeBrowserCallback(aBrowser->mCallback, *sInvokeOnStart.mBrowseResult);
    }
}

void otPlatDnssdStopBrowser(otInstance *aInstance, const otPlatDnssdBrowser *aBrowser)
{
    VerifyOrQuit(aBrowser != nullptr);

    Log("otPlatDnssdStopBrowser(\"%s\", sub-type:\"%s\")", aBrowser->mServiceType, aBrowser->mSubTypeLabel);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aBrowser->mInfraIfIndex == kInfraIfIndex);

    sStopBrowserInfo.UpdateFrom(aBrowser);
}

void otPlatDnssdStartSrvResolver(otInstance *aInstance, const otPlatDnssdSrvResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStartSrvResolver(\"%s\", \"%s\")", aResolver->mServiceInstance, aResolver->mServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStartSrvResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mSrvResult != nullptr)
    {
        InvokeSrvResolverCallback(aResolver->mCallback, *sInvokeOnStart.mSrvResult);
    }
}

void otPlatDnssdStopSrvResolver(otInstance *aInstance, const otPlatDnssdSrvResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStopSrvResolver(\"%s\", \"%s\")", aResolver->mServiceInstance, aResolver->mServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStopSrvResolverInfo.UpdateFrom(aResolver);
}

void otPlatDnssdStartTxtResolver(otInstance *aInstance, const otPlatDnssdTxtResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStartTxtResolver(\"%s\", \"%s\")", aResolver->mServiceInstance, aResolver->mServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStartTxtResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mTxtResult != nullptr)
    {
        InvokeTxtResolverCallback(aResolver->mCallback, *sInvokeOnStart.mTxtResult);
    }
}

void otPlatDnssdStopTxtResolver(otInstance *aInstance, const otPlatDnssdTxtResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStopTxtResolver(\"%s\", \"%s\")", aResolver->mServiceInstance, aResolver->mServiceType);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStopTxtResolverInfo.UpdateFrom(aResolver);
}

void otPlatDnssdStartIp6AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStartIp6AddressResolver(\"%s\")", aResolver->mHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStartIp6AddrResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mIp6AddrResult != nullptr)
    {
        InvokeIp6AddrResolverCallback(aResolver->mCallback, *sInvokeOnStart.mIp6AddrResult);
    }
}

void otPlatDnssdStopIp6AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStopIp6AddressResolver(\"%s\")", aResolver->mHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStopIp6AddrResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mIp6AddrResult != nullptr)
    {
        InvokeIp6AddrResolverCallback(aResolver->mCallback, *sInvokeOnStart.mIp6AddrResult);
    }
}

void otPlatDnssdStartIp4AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStartIp4AddressResolver(\"%s\")", aResolver->mHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStartIp4AddrResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mIp4AddrResult != nullptr)
    {
        InvokeIp6AddrResolverCallback(aResolver->mCallback, *sInvokeOnStart.mIp4AddrResult);
    }
}

void otPlatDnssdStopIp4AddressResolver(otInstance *aInstance, const otPlatDnssdAddressResolver *aResolver)
{
    VerifyOrQuit(aResolver != nullptr);

    Log("otPlatDnssdStopIp4AddressResolver(\"%s\")", aResolver->mHostName);

    VerifyOrQuit(aInstance == sInstance);
    VerifyOrQuit(aResolver->mInfraIfIndex == kInfraIfIndex);

    sStopIp4AddrResolverInfo.UpdateFrom(aResolver);

    if (sInvokeOnStart.mIp6AddrResult != nullptr)
    {
        InvokeIp4AddrResolverCallback(aResolver->mCallback, *sInvokeOnStart.mIp4AddrResult);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void TestProxyBasic(void)
{
    static constexpr uint32_t kTtl = 300;

    const uint8_t kTxtData[] = {3, 'A', '=', '1', 0};

    Srp::Server                     *srpServer;
    Srp::Client                     *srpClient;
    Dns::Client                     *dnsClient;
    Dns::ServiceDiscovery::Server   *dnsServer;
    Dnssd::BrowseResult              browseResult;
    Dnssd::SrvResult                 srvResult;
    Dnssd::TxtResult                 txtResult;
    Dnssd::AddressResult             ip6AddrrResult;
    Dnssd::AddressResult             ip4AddrrResult;
    Dnssd::AddressAndTtl             addressAndTtl;
    NetworkData::ExternalRouteConfig routeConfig;
    Ip6::Address                     address;

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
    Log("Add a route prefix (with NAT64 flag) to network data");

    routeConfig.Clear();
    SuccessOrQuit(AsCoreType(&routeConfig.mPrefix.mPrefix).FromString("64:ff9b::"));
    routeConfig.mPrefix.mLength = 96;
    routeConfig.mPreference     = NetworkData::kRoutePreferenceMedium;
    routeConfig.mNat64          = true;
    routeConfig.mStable         = true;

    SuccessOrQuit(otBorderRouterAddRoute(sInstance, &routeConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    browseResult.mServiceType     = "_avenger._udp";
    browseResult.mSubTypeLabel    = nullptr;
    browseResult.mServiceInstance = "hulk";
    browseResult.mTtl             = kTtl;
    browseResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeBrowserCallback(sStartBrowserInfo.mCallback, browseResult);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopBrowserInfo.mCallback == sStartBrowserInfo.mCallback);

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("hulk"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke SRV resolver callback");

    srvResult.mServiceInstance = "hulk";
    srvResult.mServiceType     = "_avenger._udp";
    srvResult.mHostName        = "compound";
    srvResult.mPort            = 7777;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeSrvResolverCallback(sStartSrvResolverInfo.mCallback, srvResult);

    AdvanceTime(10);

    // Check that SRV resolver is stopped and a TXT resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("hulk"));
    VerifyOrQuit(sStopSrvResolverInfo.mCallback == sStartSrvResolverInfo.mCallback);

    VerifyOrQuit(sStartTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartTxtResolverInfo.ServiceInstanceMatches("hulk"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke TXT resolver callback");

    txtResult.mServiceInstance = "hulk";
    txtResult.mServiceType     = "_avenger._udp";
    txtResult.mTxtData         = kTxtData;
    txtResult.mTxtDataLength   = sizeof(kTxtData);
    txtResult.mTtl             = kTtl;
    txtResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeTxtResolverCallback(sStartTxtResolverInfo.mCallback, txtResult);

    AdvanceTime(10);

    // Check that TXT resolver is stopped and an address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("hulk"));
    VerifyOrQuit(sStopTxtResolverInfo.mCallback == sStartTxtResolverInfo.mCallback);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("compound"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(AsCoreType(&addressAndTtl.mAddress).FromString("fd00::1234"));
    addressAndTtl.mTtl = kTtl;

    ip6AddrrResult.mHostName        = "compound";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = &addressAndTtl;
    ip6AddrrResult.mAddressesLength = 1;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("compound"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallback == sStartIp6AddrResolverInfo.mCallback);

    // Check that response is sent to client and validate it

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_avenger._udp.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "hulk"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "compound.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 7777);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sBrowseInfo.mServiceInfo.mHostAddress) == AsCoreType(&addressAndTtl.mAddress));
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("iron.man"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke SRV Resolver callback for wrong name");

    srvResult.mServiceInstance = "hulk";
    srvResult.mServiceType     = "_avenger._udp";
    srvResult.mHostName        = "compound";
    srvResult.mPort            = 7777;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeSrvResolverCallback(sStartSrvResolverInfo.mCallback, srvResult);

    AdvanceTime(10);

    // Check that no changes to browsers/resolvers

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);

    Log("Invoke SRV Resolver callback for correct name");

    srvResult.mServiceInstance = "iron.man";
    srvResult.mServiceType     = "_avenger._udp";
    srvResult.mHostName        = "starktower";
    srvResult.mPort            = 1024;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeSrvResolverCallback(sStartSrvResolverInfo.mCallback, srvResult);

    AdvanceTime(10);

    // Check that SRV resolver is stopped and TXT resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);

    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("iron.man"));
    VerifyOrQuit(sStopSrvResolverInfo.mCallback == sStartSrvResolverInfo.mCallback);

    VerifyOrQuit(sStartTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartTxtResolverInfo.ServiceInstanceMatches("iron.man"));

    Log("Invoke TXT Resolver callback");

    txtResult.mServiceInstance = "iron.man";
    txtResult.mServiceType     = "_avenger._udp";
    txtResult.mTxtData         = kTxtData;
    txtResult.mTxtDataLength   = sizeof(kTxtData);
    txtResult.mTtl             = kTtl;
    txtResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeTxtResolverCallback(sStartTxtResolverInfo.mCallback, txtResult);
    AdvanceTime(10);

    // Check that TXT resolver is stopped and addr resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);

    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("iron.man"));
    VerifyOrQuit(sStopTxtResolverInfo.mCallback == sStartTxtResolverInfo.mCallback);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("starktower"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    ip6AddrrResult.mHostName = "starktower";

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);
    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("starktower"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallback == sStartIp6AddrResolverInfo.mCallback);

    // Check that response is sent to client and validate it

    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 1);
    VerifyOrQuit(sResolveServiceInfo.mError == kErrorNone);

    VerifyOrQuit(!strcmp(sResolveServiceInfo.mInfo.mHostNameBuffer, "starktower.default.service.arpa."));
    VerifyOrQuit(sResolveServiceInfo.mInfo.mPort == 1024);
    VerifyOrQuit(sResolveServiceInfo.mInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sResolveServiceInfo.mInfo.mHostAddress) == AsCoreType(&addressAndTtl.mAddress));
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("earth"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(AsCoreType(&addressAndTtl.mAddress).FromString("fd00::7777"));
    addressAndTtl.mTtl              = kTtl;
    ip6AddrrResult.mHostName        = "earth";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = &addressAndTtl;
    ip6AddrrResult.mAddressesLength = 1;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that the address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("earth"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallback == sStartIp6AddrResolverInfo.mCallback);

    // Check that response is sent to client and validate it

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "earth.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 1);
    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == AsCoreType(&addressAndTtl.mAddress));
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);

    Log("--------------------------------------------------------------------------------------------");

    ResetPlatDnssdApiInfo();
    sResolveAddressInfo.Reset();

    Log("ResolveIp4Address()");
    SuccessOrQuit(dnsClient->ResolveIp4Address("shield.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that an IPv4 address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp4AddrResolverInfo.HostNameMatches("shield"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke IPv4 Address Resolver callback");

    SuccessOrQuit(AsCoreType(&addressAndTtl.mAddress).FromString("::ffff:1.2.3.4"));
    addressAndTtl.mTtl              = kTtl;
    ip4AddrrResult.mHostName        = "shield";
    ip4AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip4AddrrResult.mAddresses       = &addressAndTtl;
    ip4AddrrResult.mAddressesLength = 1;

    InvokeIp4AddrResolverCallback(sStartIp4AddrResolverInfo.mCallback, ip4AddrrResult);

    AdvanceTime(10);

    // Check that the IPv4 address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp4AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp4AddrResolverInfo.HostNameMatches("shield"));
    VerifyOrQuit(sStopIp4AddrResolverInfo.mCallback == sStartIp4AddrResolverInfo.mCallback);

    // Check that response is sent to client and validate it

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "shield.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 1);

    // The 1.2.3.4 address with the NAT64 prefix
    SuccessOrQuit(address.FromString("64:ff9b:0:0:0:0:102:304"));
    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == address);
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "shield.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 1);
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);

    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == address);

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
    Dnssd::BrowseResult            browseResult;
    Dnssd::SrvResult               srvResult;
    Dnssd::TxtResult               txtResult;
    Dnssd::AddressResult           ip6AddrrResult;
    Dnssd::AddressAndTtl           addressAndTtl;

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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartBrowserInfo.SubTypeMatches("_god"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    browseResult.mServiceType     = "_avenger._udp";
    browseResult.mSubTypeLabel    = "_god";
    browseResult.mServiceInstance = "thor";
    browseResult.mTtl             = kTtl;
    browseResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeBrowserCallback(sStartBrowserInfo.mCallback, browseResult);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopBrowserInfo.SubTypeMatches("_god"));
    VerifyOrQuit(sStopBrowserInfo.mCallback == sStartBrowserInfo.mCallback);

    // Check that the SRV resolver is correctly using the base service type
    // and not the sub-type name

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("thor"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    srvResult.mServiceInstance = "thor";
    srvResult.mServiceType     = "_avenger._udp";
    srvResult.mHostName        = "asgard";
    srvResult.mPort            = 1234;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeSrvResolverCallback(sStartSrvResolverInfo.mCallback, srvResult);

    AdvanceTime(10);

    // Check that SRV resolver is stopped and a TXT resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("thor"));
    VerifyOrQuit(sStopSrvResolverInfo.mCallback == sStartSrvResolverInfo.mCallback);

    VerifyOrQuit(sStartTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartTxtResolverInfo.ServiceInstanceMatches("thor"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    txtResult.mServiceInstance = "thor";
    txtResult.mServiceType     = "_avenger._udp";
    txtResult.mTxtData         = kTxtData;
    txtResult.mTxtDataLength   = sizeof(kTxtData);
    txtResult.mTtl             = kTtl;
    txtResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeTxtResolverCallback(sStartTxtResolverInfo.mCallback, txtResult);
    AdvanceTime(10);

    // Check that TXT resolver is stopped and an address resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("thor"));
    VerifyOrQuit(sStopTxtResolverInfo.mCallback == sStartTxtResolverInfo.mCallback);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("asgard"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(AsCoreType(&addressAndTtl.mAddress).FromString("fd00::1234"));
    addressAndTtl.mTtl = kTtl;

    ip6AddrrResult.mHostName        = "asgard";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = &addressAndTtl;
    ip6AddrrResult.mAddressesLength = 1;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that address resolver is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopIp6AddrResolverInfo.HostNameMatches("asgard"));
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallback == sStartIp6AddrResolverInfo.mCallback);

    // Check that response is sent to client and validate it

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    SuccessOrQuit(sBrowseInfo.mError);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceName, "_god._sub._avenger._udp.default.service.arpa."));
    VerifyOrQuit(!strcmp(sBrowseInfo.mInstanceLabel, "thor"));
    VerifyOrQuit(!strcmp(sBrowseInfo.mServiceInfo.mHostNameBuffer, "asgard.default.service.arpa."));
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mPort == 1234);
    VerifyOrQuit(sBrowseInfo.mServiceInfo.mTtl == kTtl);
    VerifyOrQuit(AsCoreType(&sBrowseInfo.mServiceInfo.mHostAddress) == AsCoreType(&addressAndTtl.mAddress));
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
    Dns::Client::QueryConfig       config;
    Dnssd::BrowseResult            browseResult;

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
    config.mNat64Mode       = OT_DNS_NAT64_DISALLOW;
    dnsClient->SetDefaultConfig(config);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_game._ps5.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_game._ps5"));
    VerifyOrQuit(sStopBrowserInfo.mCallback == sStartBrowserInfo.mCallback);

    Log("--------------------------------------------------------------------------------------------");
    Log("Timeout during service resolution");

    sBrowseInfo.Reset();
    ResetPlatDnssdApiInfo();

    SuccessOrQuit(dnsClient->Browse("_avenger._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    browseResult.mServiceType     = "_avenger._udp";
    browseResult.mSubTypeLabel    = nullptr;
    browseResult.mServiceInstance = "spiderman";
    browseResult.mTtl             = kTtl;
    browseResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeBrowserCallback(sStartBrowserInfo.mCallback, browseResult);

    AdvanceTime(10);

    // Check that browser is stopped and a service resolver is started

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("spiderman"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Wait for timeout");

    AdvanceTime(10 * 1000);

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 1);
    VerifyOrQuit(sBrowseInfo.mNumInstances == 1);

    // Check that the browser is stopped

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_avenger._udp"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Another Browse()");
    SuccessOrQuit(dnsClient->Browse("_game._udp.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("wanda"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("earth.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 2);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 0);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("wanda"));
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
    Dnssd::BrowseResult            browseResult;
    Dnssd::SrvResult               srvResult;
    Dnssd::TxtResult               txtResult;
    Dnssd::AddressResult           ip6AddrrResult;
    Dnssd::AddressAndTtl           addressAndTtl[2];

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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("starlord"));

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("knowhere"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Browser callback");

    browseResult.mServiceType     = "_guardian._glaxy";
    browseResult.mSubTypeLabel    = nullptr;
    browseResult.mServiceInstance = "starlord";
    browseResult.mTtl             = kTtl;
    browseResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeBrowserCallback(sStartBrowserInfo.mCallback, browseResult);

    AdvanceTime(10);

    // Check that browser is stopped and since the service instance
    // name matches an existing resolver, we should not see any new
    // resolver starting.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sBrowseInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveServiceInfo.mCallbackCount == 0);
    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Service Resolver callback");

    srvResult.mServiceInstance = "starlord";
    srvResult.mServiceType     = "_guardian._glaxy";
    srvResult.mHostName        = "knowhere";
    srvResult.mPort            = 3333;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeSrvResolverCallback(sStartSrvResolverInfo.mCallback, srvResult);

    AdvanceTime(10);

    // Check that SRV resolver is now stopped and a TXT resolver
    // is started for same service.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("starlord"));

    txtResult.mServiceInstance = "starlord";
    txtResult.mServiceType     = "_guardian._glaxy";
    txtResult.mTxtData         = kTxtData;
    txtResult.mTxtDataLength   = sizeof(kTxtData);
    txtResult.mTtl             = kTtl;
    txtResult.mInfraIfIndex    = kInfraIfIndex;

    InvokeTxtResolverCallback(sStartTxtResolverInfo.mCallback, txtResult);

    AdvanceTime(10);

    // Check that TXT resolver is now stopped but again since the
    // host name matches an existing address resolver we should not
    // see any new address resolver.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("starlord"));

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback");

    SuccessOrQuit(AsCoreType(&addressAndTtl[0].mAddress).FromString("fd00::5555"));
    SuccessOrQuit(AsCoreType(&addressAndTtl[1].mAddress).FromString("fd00::1234"));
    addressAndTtl[0].mTtl = kTtl;
    addressAndTtl[1].mTtl = kTtl;

    ip6AddrrResult.mHostName        = "knowhere";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = addressAndTtl;
    ip6AddrrResult.mAddressesLength = 2;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that the address resolver is now stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
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
        VerifyOrQuit(sResolveServiceInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[0].mAddress) ||
                     sResolveServiceInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[1].mAddress));
    }

    // Check the address resolve response received on client

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 1);
    SuccessOrQuit(sResolveAddressInfo.mError);

    VerifyOrQuit(!strcmp(sResolveAddressInfo.mHostName, "knowhere.default.service.arpa."));
    VerifyOrQuit(sResolveAddressInfo.mTtl == kTtl);
    VerifyOrQuit(sResolveAddressInfo.mNumHostAddresses == 2);
    for (uint16_t index = 0; index < 2; index++)
    {
        VerifyOrQuit(sResolveAddressInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[0].mAddress) ||
                     sResolveAddressInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[1].mAddress));
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
    Dnssd::AddressResult           ip6AddrrResult;
    Dnssd::AddressAndTtl           addressAndTtl[10];

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

    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sStartIp6AddrResolverInfo.HostNameMatches("host"));

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback with invalid addresses");

    SuccessOrQuit(AsCoreType(&addressAndTtl[0].mAddress).FromString("::"));         // Unspecified
    SuccessOrQuit(AsCoreType(&addressAndTtl[1].mAddress).FromString("fe80::1234")); // Link local
    SuccessOrQuit(AsCoreType(&addressAndTtl[2].mAddress).FromString("ff00::1234")); // Multicast
    SuccessOrQuit(AsCoreType(&addressAndTtl[3].mAddress).FromString("::1"));        // Lookback

    for (uint16_t index = 0; index < 5; index++)
    {
        addressAndTtl[index].mTtl = kTtl;
    }

    ip6AddrrResult.mHostName        = "host";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = addressAndTtl;
    ip6AddrrResult.mAddressesLength = 4;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that the address resolver is not stopped, since all addresses where
    // invalid address.

    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 0);

    VerifyOrQuit(sResolveAddressInfo.mCallbackCount == 0);

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Invoke Address Resolver callback with invalid addresses with one valid");

    SuccessOrQuit(AsCoreType(&addressAndTtl[4].mAddress).FromString("f00::1234"));

    ip6AddrrResult.mAddressesLength = 5;

    InvokeIp6AddrResolverCallback(sStartIp6AddrResolverInfo.mCallback, ip6AddrrResult);

    AdvanceTime(10);

    // Check that address resolver is not stopped

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
    VerifyOrQuit(sResolveAddressInfo.mHostAddresses[0] == AsCoreType(&addressAndTtl[4].mAddress));

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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_avenger._udp"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("captain.america"));

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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 0);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 0);
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
    Dnssd::BrowseResult            browseResult;
    Dnssd::SrvResult               srvResult;
    Dnssd::TxtResult               txtResult;
    Dnssd::AddressResult           ip6AddrrResult;
    Dnssd::AddressAndTtl           addressAndTtl[2];

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

    sInvokeOnStart.mBrowseResult  = &browseResult;
    sInvokeOnStart.mSrvResult     = &srvResult;
    sInvokeOnStart.mTxtResult     = &txtResult;
    sInvokeOnStart.mIp6AddrResult = &ip6AddrrResult;

    browseResult.mServiceType     = "_guardian._glaxy";
    browseResult.mSubTypeLabel    = nullptr;
    browseResult.mServiceInstance = "mantis";
    browseResult.mTtl             = kTtl;
    browseResult.mInfraIfIndex    = kInfraIfIndex;

    srvResult.mServiceInstance = "mantis";
    srvResult.mServiceType     = "_guardian._glaxy";
    srvResult.mHostName        = "nova";
    srvResult.mPort            = 3333;
    srvResult.mTtl             = kTtl;
    srvResult.mInfraIfIndex    = kInfraIfIndex;

    txtResult.mServiceInstance = "mantis";
    txtResult.mServiceType     = "_guardian._glaxy";
    txtResult.mTxtData         = kTxtData;
    txtResult.mTxtDataLength   = sizeof(kTxtData);
    txtResult.mTtl             = kTtl;
    txtResult.mInfraIfIndex    = kInfraIfIndex;

    SuccessOrQuit(AsCoreType(&addressAndTtl[0].mAddress).FromString("fd00::5555"));
    SuccessOrQuit(AsCoreType(&addressAndTtl[1].mAddress).FromString("fd00::1234"));
    addressAndTtl[0].mTtl = kTtl;
    addressAndTtl[1].mTtl = kTtl;

    ip6AddrrResult.mHostName        = "nova";
    ip6AddrrResult.mInfraIfIndex    = kInfraIfIndex;
    ip6AddrrResult.mAddresses       = addressAndTtl;
    ip6AddrrResult.mAddressesLength = 2;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sBrowseInfo.Reset();

    Log("Browse()");
    SuccessOrQuit(dnsClient->Browse("_guardian._glaxy.default.service.arpa.", BrowseCallback, sInstance));
    AdvanceTime(10);

    // All browsers/resolvers should be started and stopped
    // (since the callbacks are invoked directly from API)

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 1);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 1);

    VerifyOrQuit(sStartBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopBrowserInfo.ServiceTypeMatches("_guardian._glaxy"));

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("mantis"));

    VerifyOrQuit(sStartTxtResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartTxtResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("mantis"));

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

    // Check that new SRV/TXT resolver and address resolvers are
    // started and stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStartIp6AddrResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopIp6AddrResolverInfo.mCallCount == 2);

    VerifyOrQuit(sStartSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartSrvResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopSrvResolverInfo.ServiceInstanceMatches("mantis"));

    VerifyOrQuit(sStartTxtResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStartTxtResolverInfo.ServiceInstanceMatches("mantis"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceTypeMatches("_guardian._glaxy"));
    VerifyOrQuit(sStopTxtResolverInfo.ServiceInstanceMatches("mantis"));

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
        VerifyOrQuit(sResolveServiceInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[0].mAddress) ||
                     sResolveServiceInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[1].mAddress));
    }

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ");
    sResolveAddressInfo.Reset();
    Log("ResolveAddress()");
    SuccessOrQuit(dnsClient->ResolveAddress("nova.default.service.arpa.", AddressCallback, sInstance));
    AdvanceTime(10);

    // Check that new address resolver is started and stopped.

    VerifyOrQuit(sStartBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStopBrowserInfo.mCallCount == 1);
    VerifyOrQuit(sStartSrvResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopSrvResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStartTxtResolverInfo.mCallCount == 2);
    VerifyOrQuit(sStopTxtResolverInfo.mCallCount == 2);
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
        VerifyOrQuit(sResolveAddressInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[0].mAddress) ||
                     sResolveAddressInfo.mHostAddresses[index] == AsCoreType(&addressAndTtl[1].mAddress));
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
    printf("DISCOVERY_PROXY feature or a related feature required by this unit test is not enabled\n");
#endif

    return 0;
}
