/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
#include <openthread/srp_client.h>
#include <openthread/srp_server.h>
#include <openthread/thread.h>

#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/string.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE && \
    !OPENTHREAD_CONFIG_TIME_SYNC_ENABLE && !OPENTHREAD_PLATFORM_POSIX
#define ENABLE_SRP_TEST 1
#else
#define ENABLE_SRP_TEST 0
#endif

namespace ot {

#if ENABLE_SRP_TEST

// Logs a message and adds current time (sNow) as "<hours>:<min>:<secs>.<msec>"
#define Log(...)                                                                                          \
    printf("%02u:%02u:%02u.%03u " OT_FIRST_ARG(__VA_ARGS__) "\n", (sNow / 36000000), (sNow / 60000) % 60, \
           (sNow / 1000) % 60, sNow % 1000 OT_REST_ARGS(__VA_ARGS__))

static constexpr uint16_t kMaxRaSize = 800;

static Instance *sInstance;

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

void InitTest(bool aStartThread = true)
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

    if (aStartThread)
    {
        SuccessOrQuit(otIp6SetEnabled(sInstance, true));
        SuccessOrQuit(otThreadSetEnabled(sInstance, true));

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Ensure device starts as leader.

        AdvanceTime(10000);

        VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
    }
}

void FinalizeTest(void)
{
    SuccessOrQuit(otIp6SetEnabled(sInstance, false));
    SuccessOrQuit(otThreadSetEnabled(sInstance, false));
    SuccessOrQuit(otInstanceErasePersistentInfo(sInstance));
    testFreeInstance(sInstance);
}

//---------------------------------------------------------------------------------------------------------------------

enum UpdateHandlerMode
{
    kAccept, // Accept all updates.
    kReject, // Reject all updates.
    kIgnore  // Ignore all updates (do not call `otSrpServerHandleServiceUpdateResult()`).
};

static UpdateHandlerMode    sUpdateHandlerMode       = kAccept;
static bool                 sProcessedUpdateCallback = false;
static otSrpServerLeaseInfo sUpdateHostLeaseInfo;
static uint32_t             sUpdateHostKeyLease;

void HandleSrpServerUpdate(otSrpServerServiceUpdateId aId,
                           const otSrpServerHost     *aHost,
                           uint32_t                   aTimeout,
                           void                      *aContext)
{
    Log("HandleSrpServerUpdate() called with %u, timeout:%u", aId, aTimeout);

    VerifyOrQuit(aHost != nullptr);
    VerifyOrQuit(aContext == sInstance);

    sProcessedUpdateCallback = true;

    otSrpServerHostGetLeaseInfo(aHost, &sUpdateHostLeaseInfo);

    switch (sUpdateHandlerMode)
    {
    case kAccept:
        otSrpServerHandleServiceUpdateResult(sInstance, aId, kErrorNone);
        break;
    case kReject:
        otSrpServerHandleServiceUpdateResult(sInstance, aId, kErrorFailed);
        break;
    case kIgnore:
        break;
    }
}

static bool  sProcessedClientCallback = false;
static Error sLastClientCallbackError = kErrorNone;

void HandleSrpClientCallback(otError                    aError,
                             const otSrpClientHostInfo *aHostInfo,
                             const otSrpClientService  *aServices,
                             const otSrpClientService  *aRemovedServices,
                             void                      *aContext)
{
    Log("HandleSrpClientCallback() called with error %s", ErrorToString(aError));

    VerifyOrQuit(aContext == sInstance);

    sProcessedClientCallback = true;
    sLastClientCallbackError = aError;

    OT_UNUSED_VARIABLE(aHostInfo);
    OT_UNUSED_VARIABLE(aServices);
    OT_UNUSED_VARIABLE(aRemovedServices);
}

static const char kHostName[] = "myhost";

void PrepareService1(Srp::Client::Service &aService)
{
    static const char          kServiceName[]   = "_srv._udp";
    static const char          kInstanceLabel[] = "srv.instance";
    static const char          kSub1[]          = "_sub1";
    static const char          kSub2[]          = "_V1234567";
    static const char          kSub3[]          = "_XYZWS";
    static const char         *kSubLabels[]     = {kSub1, kSub2, kSub3, nullptr};
    static const char          kTxtKey1[]       = "ABCD";
    static const uint8_t       kTxtValue1[]     = {'a', '0'};
    static const char          kTxtKey2[]       = "Z0";
    static const uint8_t       kTxtValue2[]     = {'1', '2', '3'};
    static const char          kTxtKey3[]       = "D";
    static const uint8_t       kTxtValue3[]     = {0};
    static const otDnsTxtEntry kTxtEntries[]    = {
           {kTxtKey1, kTxtValue1, sizeof(kTxtValue1)},
           {kTxtKey2, kTxtValue2, sizeof(kTxtValue2)},
           {kTxtKey3, kTxtValue3, sizeof(kTxtValue3)},
    };

    memset(&aService, 0, sizeof(aService));
    aService.mName          = kServiceName;
    aService.mInstanceName  = kInstanceLabel;
    aService.mSubTypeLabels = kSubLabels;
    aService.mTxtEntries    = kTxtEntries;
    aService.mNumTxtEntries = 3;
    aService.mPort          = 777;
    aService.mWeight        = 1;
    aService.mPriority      = 2;
}

void PrepareService2(Srp::Client::Service &aService)
{
    static const char  kService2Name[]   = "_00112233667882554._matter._udp";
    static const char  kInstance2Label[] = "ABCDEFGHI";
    static const char  kSub4[]           = "_44444444";
    static const char *kSubLabels2[]     = {kSub4, nullptr};

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

//----------------------------------------------------------------------------------------------------------------------

enum SrpCoderMode : bool
{
    kDoNotUseSrpCoderOnClient = false,
    kUseSrpCoderOnClient      = true,
};

void ApplySrpCoderMode(Srp::Client &aSrpClient, SrpCoderMode aCoderMode)
{
#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE
    aSrpClient.SetMessageCoderEnabled(aCoderMode);
    VerifyOrQuit(aSrpClient.IsMessageCoderEnabled() == aCoderMode);
#else
    OT_UNUSED_VARIABLE(aSrpClient);
    OT_UNUSED_VARIABLE(aCoderMode);
#endif
}

const char *CoderModeToString(SrpCoderMode aCoderMode)
{
    return (aCoderMode == kUseSrpCoderOnClient) ? "UseCoderOnClient" : "DoNotUseCoderOnClient";
}

//----------------------------------------------------------------------------------------------------------------------

void TestSrpServerBase(SrpCoderMode aCoderMode)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerBase(%s)", CoderModeToString(aCoderMode));

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, aCoderMode);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service, validate that update handler is called.

    SuccessOrQuit(srpClient->AddService(service1));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a second service, validate that update handler is called.

    SuccessOrQuit(srpClient->AddService(service2));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unregister first service, validate that update handler is called.

    SuccessOrQuit(srpClient->RemoveService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerBase(%s)", CoderModeToString(aCoderMode));
}

void TestSrpServerReject(SrpCoderMode aCoderMode)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerReject(%s)", CoderModeToString(aCoderMode));

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, aCoderMode);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(15 * 1000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    sUpdateHandlerMode = kReject;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service, validate that update handler is called
    // and rejected and no service is registered.

    SuccessOrQuit(srpClient->AddService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError != kErrorNone);

    VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);

    VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a second service, validate that update handler is
    // again called and update is rejected.

    SuccessOrQuit(srpClient->AddService(service2));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError != kErrorNone);

    VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() != Srp::Client::kRegistered);

    VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerReject(%s)", CoderModeToString(aCoderMode));
}

void TestSrpServerIgnore(SrpCoderMode aCoderMode)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerIgnore(%s)", CoderModeToString(aCoderMode));

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, aCoderMode);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(15 * 1000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    sUpdateHandlerMode = kIgnore;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service, validate that update handler is called
    // and ignored the update and no service is registered.

    SuccessOrQuit(srpClient->AddService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError != kErrorNone);

    VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);

    VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a second service, validate that update handler is
    // again called and update is still ignored.

    SuccessOrQuit(srpClient->AddService(service2));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError != kErrorNone);

    VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() != Srp::Client::kRegistered);

    VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerIgnore(%s)", CoderModeToString(aCoderMode));
}

void TestSrpServerClientRemove(bool aShouldRemoveKeyLease, SrpCoderMode aCoderMode)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerClientRemove(aShouldRemoveKeyLease:%u, %s)", aShouldRemoveKeyLease,
        CoderModeToString(aCoderMode));

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, aCoderMode);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(15 * 1000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register two services, validate that update handler is called.

    SuccessOrQuit(srpClient->AddService(service1));
    SuccessOrQuit(srpClient->AddService(service2));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);
    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove two services and clear key-lease, validate that update handler is called.

    SuccessOrQuit(srpClient->RemoveHostAndServices(aShouldRemoveKeyLease));

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRemoved);

    if (aShouldRemoveKeyLease)
    {
        VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);
    }
    else
    {
        ValidateHost(*srpServer, kHostName);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerClientRemove(aShouldRemoveKeyLease:%u, %s)", aShouldRemoveKeyLease,
        CoderModeToString(aCoderMode));
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void TestUpdateLeaseShortVariant(SrpCoderMode aCoderMode)
{
    // Test behavior of SRP client and server when short variant of
    // Update Lease Option is used (which only include lease interval).
    // This test uses `SetUseShortLeaseOption()` method of `Srp::Client`
    // which changes the default behavior and is available under the
    // `REFERENCE_DEVICE` config.

    Srp::Server                *srpServer;
    Srp::Server::LeaseConfig    leaseConfig;
    const Srp::Server::Service *service;
    Srp::Client                *srpClient;
    Srp::Client::Service        service1;
    uint16_t                    heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestUpdateLeaseShortVariant(%s)", CoderModeToString(aCoderMode));

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the default Lease Config on SRP server.
    // Server to accept lease in [30 sec, 27 hours] and
    // key-lease in [30 sec, 189 hours].

    srpServer->GetLeaseConfig(leaseConfig);

    VerifyOrQuit(leaseConfig.mMinLease == 30);             // 30 seconds
    VerifyOrQuit(leaseConfig.mMaxLease == 27u * 3600);     // 27 hours
    VerifyOrQuit(leaseConfig.mMinKeyLease == 30);          // 30 seconds
    VerifyOrQuit(leaseConfig.mMaxKeyLease == 189u * 3600); // 189 hours

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, aCoderMode);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(15 * 1000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    sUpdateHandlerMode = kAccept;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Change default lease intervals on SRP client and enable
    // "use short Update Lease Option" mode.

    srpClient->SetLeaseInterval(15u * 3600);
    srpClient->SetKeyLeaseInterval(40u * 3600);

    srpClient->SetUseShortLeaseOption(true);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service, validate that update handler is called
    // and service is successfully registered.

    SuccessOrQuit(srpClient->AddService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);

    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the lease info for service on SRP server. The client
    // is set up to use "short Update Lease Option format, so it only
    // include the lease interval as 15 hours in its request
    // message. Server should then see 15 hours for both lease and
    // key lease

    VerifyOrQuit(sUpdateHostLeaseInfo.mLease == 15u * 3600 * 1000);
    VerifyOrQuit(sUpdateHostLeaseInfo.mKeyLease == 15u * 3600 * 1000);

    // Check that SRP server granted 15 hours for both lease and
    // key lease.

    service = srpServer->GetNextHost(nullptr)->GetServices().GetHead();
    VerifyOrQuit(service != nullptr);
    VerifyOrQuit(service->GetLease() == 15u * 3600);
    VerifyOrQuit(service->GetKeyLease() == 15u * 3600);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the service.

    SuccessOrQuit(srpClient->RemoveService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register the service again, but this time change it to request
    // a lease time that is larger than the `LeaseConfig.mMinLease` of
    // 27 hours. This ensures that server needs to include the Lease
    // Option in its response (since it need to grant a different
    // lease interval).

    service1.mLease    = 100u * 3600; // 100 hours >= 27 hours.
    service1.mKeyLease = 110u * 3600;

    SuccessOrQuit(srpClient->AddService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);

    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate the lease info for service on SRP server.

    // We should see the 100 hours in request from client
    VerifyOrQuit(sUpdateHostLeaseInfo.mLease == 100u * 3600 * 1000);
    VerifyOrQuit(sUpdateHostLeaseInfo.mKeyLease == 100u * 3600 * 1000);

    // Check that SRP server granted  27 hours for both lease and
    // key lease.

    service = srpServer->GetNextHost(nullptr)->GetServices().GetHead();
    VerifyOrQuit(service != nullptr);
    VerifyOrQuit(service->GetLease() == 27u * 3600);
    VerifyOrQuit(service->GetKeyLease() == 27u * 3600);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestUpdateLeaseShortVariant(%s)", CoderModeToString(aCoderMode));
}

static uint16_t         sServerRxCount;
static Ip6::MessageInfo sServerMsgInfo;
static uint16_t         sServerLastMsgId;

void HandleServerUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Dns::Header header;

    VerifyOrQuit(aContext == nullptr);
    VerifyOrQuit(aMessage != nullptr);
    VerifyOrQuit(aMessageInfo != nullptr);

    SuccessOrQuit(AsCoreType(aMessage).Read(0, header));

    sServerMsgInfo   = AsCoreType(aMessageInfo);
    sServerLastMsgId = header.GetMessageId();
    sServerRxCount++;

    Log("HandleServerUdpReceive(), message-id: 0x%x", header.GetMessageId());
}

void TestSrpClientDelayedResponse(SrpCoderMode aCoderMode)
{
    static constexpr uint16_t kServerPort = 53535;

    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpClientDelayedResponse(%s)", CoderModeToString(aCoderMode));

    InitTest();

    srpClient = &sInstance->Get<Srp::Client>();

    for (uint8_t testIter = 0; testIter < 3; testIter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("testIter = %u", testIter);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Prepare a socket to act as SRP server.

        Ip6::Udp::Socket  udpSocket(*sInstance, HandleServerUdpReceive, nullptr);
        Ip6::SockAddr     serverSockAddr;
        uint16_t          firstMsgId;
        Message          *response;
        Dns::UpdateHeader header;

        sServerRxCount = 0;

        SuccessOrQuit(udpSocket.Open(Ip6::kNetifThreadInternal));
        SuccessOrQuit(udpSocket.Bind(kServerPort));

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Manually start the client with a message ID based on `testIter`
        // We use zero in the first iteration, `0xffff` in the second
        // iteration to test wrapping of 16-bit message ID.

        switch (testIter)
        {
        case 0:
            srpClient->SetNextMessageId(0);
            break;
        case 1:
            srpClient->SetNextMessageId(0xffff);
            break;
        case 2:
            srpClient->SetNextMessageId(0xaaaa);
            break;
        }

        serverSockAddr.SetAddress(sInstance->Get<Mle::Mle>().GetMeshLocalRloc());
        serverSockAddr.SetPort(kServerPort);
        SuccessOrQuit(srpClient->Start(serverSockAddr));
        ApplySrpCoderMode(*srpClient, aCoderMode);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Register a service

        SuccessOrQuit(srpClient->SetHostName(kHostName));
        SuccessOrQuit(srpClient->EnableAutoHostAddress());

        PrepareService1(service1);
        SuccessOrQuit(srpClient->AddService(service1));

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for short time and make sure server receives an SRP
        // update message from client.

        AdvanceTime(1 * 1000);

        VerifyOrQuit(sServerRxCount == 1);
        firstMsgId = sServerLastMsgId;

        switch (testIter)
        {
        case 0:
            VerifyOrQuit(firstMsgId == 0);
            break;
        case 1:
            VerifyOrQuit(firstMsgId == 0xffff);
            break;
        case 2:
            VerifyOrQuit(firstMsgId == 0xaaaa);
            break;
        }

        if (testIter == 2)
        {
            AdvanceTime(2 * 1000);

            PrepareService2(service2);
            SuccessOrQuit(srpClient->AddService(service2));
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for longer to allow client to retry a bunch of times

        AdvanceTime(20 * 1000);
        VerifyOrQuit(sServerRxCount > 1);
        VerifyOrQuit(sServerLastMsgId != firstMsgId);

        VerifyOrQuit(srpClient->GetHostInfo().GetState() != Srp::Client::kRegistered);
        VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Now send a delayed response from server using the first
        // message ID.

        response = udpSocket.NewMessage();
        VerifyOrQuit(response != nullptr);

        Log("Sending response with msg-id: 0x%x", firstMsgId);

        header.SetMessageId(firstMsgId);
        header.SetType(Dns::UpdateHeader::kTypeResponse);
        header.SetResponseCode(Dns::UpdateHeader::kResponseSuccess);
        SuccessOrQuit(response->Append(header));
        SuccessOrQuit(udpSocket.SendTo(*response, sServerMsgInfo));

        AdvanceTime(10);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // In the first two iterations, we ensure that client
        // did successfully accept the response with older message ID.
        // This should not be the case in the third iteration due to
        // changes to client services after first UPdate message was
        // sent by client.

        switch (testIter)
        {
        case 0:
        case 1:
            VerifyOrQuit(srpClient->GetHostInfo().GetState() == Srp::Client::kRegistered);
            VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
            break;
        case 2:
            VerifyOrQuit(srpClient->GetHostInfo().GetState() != Srp::Client::kRegistered);
            VerifyOrQuit(service1.GetState() != Srp::Client::kRegistered);
            break;
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Remove service and close socket.

        srpClient->ClearHostAndServices();
        srpClient->Stop();

        SuccessOrQuit(udpSocket.Close());
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestSrpClientDelayedResponse(%s)", CoderModeToString(aCoderMode));
}

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

void TestSrpServerAddressModeForceAdd(void)
{
    Srp::Server *srpServer;
    Srp::Client *srpClient;
    uint16_t     heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerAddressModeForceAdd");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Set address mode to `kAddressModeUnicastForceAdd`.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicastForceAdd));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicastForceAdd);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server, ensure it starts quickly.

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(0);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client and validate that it discovers server.

    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server. Validate that the NetData entry is removed and
    // client detects this.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(1);

    VerifyOrQuit(!srpClient->IsRunning());

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerAddressModeForceAdd");
}

#if OPENTHREAD_CONFIG_SRP_SERVER_FAST_START_MODE_ENABLE

void TestSrpServerFastStartMode(void)
{
    Srp::Server          *srpServer;
    otExternalRouteConfig route;
    Ip6::Address          address;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerFastStartMode");

    InitTest(/* aStartThread */ false);

    srpServer = &sInstance->Get<Srp::Server>();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Configure SRP server to use the "Fast Start Mode"/

    SuccessOrQuit(srpServer->EnableFastStartMode());
    VerifyOrQuit(srpServer->IsFastStartModeEnabled());

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Bring the IPv6 interface up and start Thread operation.

    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure that as soon as device attaches, the SRP server is started.

    while (otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_DETACHED)
    {
        AdvanceTime(100);
    }

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicastForceAdd);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add new entries in Network Data to trigger an "NetDataChanged" event
    // and ensure that the SRP server continues to run.

    AdvanceTime(10 * 1000);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    ClearAllBytes(route);
    route.mStable = true;

    SuccessOrQuit(otBorderRouterAddRoute(sInstance, &route));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    AdvanceTime(1 * 1000);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Publish an "DNS/SRP" entry in Network Data and ensure that this is
    // correctly detected by the "Fast Start Mode" and triggers SRP server to be
    // disabled.

    SuccessOrQuit(address.FromString("fd00::1"));
    otNetDataPublishDnsSrpServiceUnicast(sInstance, &address, /* aPort */ 1234, 0);

    AdvanceTime(10 * 1000);
    VerifyOrQuit(otNetDataIsDnsSrpServiceAdded(sInstance));

    VerifyOrQuit(srpServer->IsFastStartModeEnabled());
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    // Ensure the original AddressMode is restored on SRP server

    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unpublish the "DNS/SRP" entry in Network Data and check that
    // the "Fast Start Mode" causes the SRP server to start again.

    otNetDataUnpublishDnsSrpService(sInstance);

    AdvanceTime(25 * 1000);
    VerifyOrQuit(!otNetDataIsDnsSrpServiceAdded(sInstance));

    VerifyOrQuit(srpServer->IsFastStartModeEnabled());
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start auto-enable mode and ensure "fast start mode" is turned
    // off and the original AddressMode is restored on the SRP server.

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    srpServer->SetAutoEnableMode(true);

    VerifyOrQuit(!srpServer->IsFastStartModeEnabled());
    VerifyOrQuit(srpServer->IsAutoEnableMode());

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->EnableFastStartMode() == kErrorInvalidState);
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestSrpServerFastStartMode");
}

#endif // OPENTHREAD_CONFIG_SRP_SERVER_FAST_START_MODE_ENABLE

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE

namespace SrpCoder {

static const char    kHostName[]      = "DAAFF10F39B00F32";
static const uint8_t kHostNameCoded[] = {0xDA, 0xAF, 0xF1, 0x0F, 0x39, 0xB0, 0x0F, 0x32};

static const char kMatterServiceName[] = "_matter._tcp";
static const char kTestServiceName[]   = "_test._udp";

static const char kServiceInstance1[] = "8F097FD118441046-00000000B3B3D017";
static const char kServiceInstance2[] = "1FF04909193C16E2-000000006CC07561";

static const uint8_t kInstance1Coded[] = {0x8F, 0x09, 0x7F, 0xD1, 0x18, 0x44, 0x10, 0x46};
static const uint8_t kInstance2Coded[] = {0x1F, 0xF0, 0x49, 0x09, 0x19, 0x3C, 0x16, 0xE2};

static const char *kSubLabels1[] = {"_I8F097FD118441046", nullptr};
static const char *kSubLabels2[] = {"_I1FF04909193C16E2", nullptr};

static const char    kTxtKey1[]   = "SII";
static const uint8_t kTxtValue1[] = {'1', '0', '0', '0'};
static const char    kTxtKey2[]   = "SAI";
static const uint8_t kTxtValue2[] = {'1', '0', '0', '0'};
static const char    kTxtKey3[]   = "SAT";
static const uint8_t kTxtValue3[] = {'4', '0', '0', '0'};
static const char    kTxtKey4[]   = "T";
static const uint8_t kTxtValue4[] = {'0'};

static const otDnsTxtEntry kTxtEntries[] = {
    {kTxtKey1, kTxtValue1, sizeof(kTxtValue1)},
    {kTxtKey2, kTxtValue2, sizeof(kTxtValue2)},
    {kTxtKey3, kTxtValue3, sizeof(kTxtValue3)},
    {kTxtKey4, kTxtValue4, sizeof(kTxtValue4)},
};

static const uint8_t kTxtData[] = {
    8, 'S', 'I', 'I', '=', '1', '0', '0', '0', // SII=1000
    8, 'S', 'A', 'I', '=', '1', '0', '0', '0', // SAI=1000
    8, 'S', 'A', 'T', '=', '4', '0', '0', '0', // SAT=4000
    3, 'T', '=', '0',                          // T=0
};

static const uint8_t kOnMeshPrefix[] = {
    0xFD, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE,
};

static const uint8_t kExternalIp6Address[] = {
    0xFD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
};

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

uint16_t CountOccurrencesOfDataIn(const Message &aMessage, const void *aData, uint16_t aDataLength)
{
    uint16_t count = 0;

    for (uint16_t offset = 0; offset < aMessage.GetLength() - aDataLength; offset++)
    {
        if (aMessage.CompareBytes(offset, aData, aDataLength))
        {
            count++;
        }
    }

    return count;
}

template <uint16_t kArrayLength>
uint16_t CountOccurrencesOfDataIn(const Message &aMessage, const uint8_t (&aArray)[kArrayLength])
{
    return CountOccurrencesOfDataIn(aMessage, aArray, kArrayLength);
}

uint16_t CountOccurrencesOfStringIn(const Message &aMessage, const char *aString)
{
    return CountOccurrencesOfDataIn(aMessage, aString, strlen(aString));
}

void LogCodedMessage(const Message &aCodedMsg, const Message &aMessage, Error aError)
{
    uint8_t  buffer[2000];
    uint16_t len;

    SuccessOrQuit(aError);

    len = aCodedMsg.ReadBytes(/* aOffset */ 0, buffer, sizeof(buffer));
    DumpBuffer("CodedMsg", buffer, len);

    len = aMessage.ReadBytes(/* aOffset */ 0, buffer, sizeof(buffer));
    DumpBuffer("DecodedMsg", buffer, len);
}

#endif // OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void ValidateSingleService(const Message &aCodedMsg, const Message &aMessage, Error aError)
{
    // Host : "DAAFF10F39B00F32" (kHostName)
    // - Two addresses:
    // - An OMR address + explicitly added external
    //
    // One service:
    // - Instance : "DAAFF10F39B00F32" (kHostName)
    // - Type     : "_test._udp" (kTestServiceName)
    // - No sub-type

    Log("ValidateSingleService()");
    LogCodedMessage(aCodedMsg, aMessage, aError);

    // Make sure host name is encoded properly (only
    // one instance of the name is included).
    // kHostName is also used for service instance name

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kHostName) == 0);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kHostNameCoded) == 1);

    // Make sure service type "_test._udp" is encoded
    // properly

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "test") == 1);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "_test") == 0);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "udp") == 0);

    // Validate TXT data is present

    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kTxtData) == 1);

    // Make sure the OMR prefix is not included
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kOnMeshPrefix) == 0);

    // Make sure the explicitly added external address is
    // encoded directly

    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kExternalIp6Address) == 1);
}

#endif // OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void TestSingleService(void)
{
    NetworkData::OnMeshPrefixConfig prefixConfig;
    Ip6::Netif::UnicastAddress      unicastAddr;
    Srp::Server                    *srpServer;
    Srp::Client                    *srpClient;
    Srp::Client::Service            service;
    const Srp::Server::Host        *host;
    uint8_t                         numServices;
    uint8_t                         numAddresses;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSingleService()");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add an on-mesh prefix (with SLAAC) to network data

    prefixConfig.Clear();
    SuccessOrQuit(AsCoreType(&prefixConfig.mPrefix.mPrefix).FromString("fd00:dead:beef:cafe::"));
    prefixConfig.mPrefix.mLength = 64;
    prefixConfig.mStable         = true;
    prefixConfig.mSlaac          = true;
    prefixConfig.mPreferred      = true;
    prefixConfig.mOnMesh         = true;
    prefixConfig.mDefaultRoute   = false;
    prefixConfig.mPreference     = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Add a specific external address

    ClearAllBytes(unicastAddr);
    memcpy(&unicastAddr.GetAddress(), kExternalIp6Address, sizeof(kExternalIp6Address));
    unicastAddr.mAddressOrigin = Ip6::Netif::kOriginManual;
    unicastAddr.mValid         = true;
    unicastAddr.mPreferred     = true;
    SuccessOrQuit(sInstance->Get<ThreadNetif>().AddExternalUnicastAddress(unicastAddr));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare the service

    ClearAllBytes(service);
    service.mName          = kTestServiceName;
    service.mInstanceName  = kHostName;
    service.mSubTypeLabels = nullptr;
    service.mTxtEntries    = kTxtEntries;
    service.mNumTxtEntries = 4;
    service.mPort          = 0x3344;
    service.mWeight        = 0x1234;
    service.mPriority      = 0xabcd;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, kUseSrpCoderOnClient);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->SetTtl(4000);
    srpClient->SetLeaseInterval(5000);
    srpClient->SetKeyLeaseInterval(172800);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup SRP coder callback

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    sInstance->Get<Srp::Coder>().SetDecodeCallback(ValidateSingleService);
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register service, validate that coded message is properly
    // formatted.

    SuccessOrQuit(srpClient->AddService(service));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service.GetState() == Srp::Client::kRegistered);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the host and service information on server

    host = srpServer->GetNextHost(nullptr);
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(StringStartsWith(host->GetFullName(), kHostName, kStringCaseInsensitiveMatch));
    VerifyOrQuit(srpServer->GetNextHost(host) == nullptr);

    // Check host addresses
    host->GetAddresses(numAddresses);
    VerifyOrQuit(numAddresses == 2);

    for (const Ip6::Address *addr = host->GetAddresses(numAddresses); numAddresses > 0; addr++, numAddresses--)
    {
        VerifyOrQuit(addr->MatchesPrefix(prefixConfig.GetPrefix()) || (*addr == unicastAddr.GetAddress()));
    }

    VerifyOrQuit(host->GetLease() == 5000);
    VerifyOrQuit(host->GetKeyLease() == 172800);
    VerifyOrQuit(host->GetTtl() == 4000);

    numServices = 0;

    for (const Srp::Server::Service &serverService : host->GetServices())
    {
        numServices++;

        VerifyOrQuit(StringStartsWith(serverService.GetServiceName(), kTestServiceName));
        VerifyOrQuit(serverService.GetPort() == 0x3344);
        VerifyOrQuit(serverService.GetWeight() == 0x1234);
        VerifyOrQuit(serverService.GetPriority() == 0xabcd);
        VerifyOrQuit(serverService.GetNumberOfSubTypes() == 0);
        VerifyOrQuit(StringMatch(serverService.GetInstanceLabel(), kHostName));
        VerifyOrQuit(serverService.GetTxtDataLength() == sizeof(kTxtData));
        VerifyOrQuit(memcmp(kTxtData, serverService.GetTxtData(), sizeof(kTxtData)) == 0);
    }

    VerifyOrQuit(numServices == 1);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of SrpCoder::TestSingleService()");
}

//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void ValidateTwoServicesWithSubtypeSameTxtData(const Message &aCodedMsg, const Message &aMessage, Error aError)
{
    // Host : "DAAFF10F39B00F32" (kHostName)
    // - One address - OMR address
    //
    // Service 1:
    // - Instance : "8F097FD118441046-00000000B3B3D017" (kServiceInstance1)
    // - Type     : "_matter._tcp"
    // - Subtype  : "_I8F097FD118441046"
    //
    // Service 2:
    // - Instance : "1FF04909193C16E2-000000006CC07561" (kServiceInstance2)
    // - Type     : "_matter._tcp"
    // - Subtype  : "_I1FF04909193C16E2"

    Log("ValidateTwoServicesWithSubtypeSameTxtData()");
    LogCodedMessage(aCodedMsg, aMessage, aError);

    // Make sure host name is encoded properly (only
    // one instance of the name is included).

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kHostName) == 0);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kHostNameCoded) == 1);

    // Make sure service type "_matter._tcp" is encoded
    // properly. Both "matter" are "tcp" should be encoded
    // as a "commonly used" constant labels.

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "matter") == 0);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "tcp") == 0);

    // Make sure service instance labels are encoded properly
    // and reused for the sub-type labels which uses same hex
    // value.

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kServiceInstance1) == 0);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kServiceInstance2) == 0);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kInstance1Coded) == 1);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kInstance2Coded) == 1);

    // Same TXT data is used by both services, make sure
    // the data is used only once and other service
    // refers to previously encoded TXT data.

    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kTxtData) == 1);

    // Make sure the OMR prefix is not included
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kOnMeshPrefix) == 0);
}

#endif // OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void TestTwoServicesWithSubtypeSameTxtData(void)
{
    NetworkData::OnMeshPrefixConfig prefixConfig;
    Srp::Server                    *srpServer;
    Srp::Client                    *srpClient;
    Srp::Client::Service            service1;
    Srp::Client::Service            service2;
    const Srp::Server::Host        *host;
    uint8_t                         numServices;
    uint8_t                         numAddresses;
    uint16_t                        heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestTwoServicesWithSubtypeSameTxtData()");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // "Add an on-mesh prefix (with SLAAC) to network data

    prefixConfig.Clear();
    SuccessOrQuit(AsCoreType(&prefixConfig.mPrefix.mPrefix).FromString("fd00:dead:beef:cafe::"));
    prefixConfig.mPrefix.mLength = 64;
    prefixConfig.mStable         = true;
    prefixConfig.mSlaac          = true;
    prefixConfig.mPreferred      = true;
    prefixConfig.mOnMesh         = true;
    prefixConfig.mDefaultRoute   = false;
    prefixConfig.mPreference     = NetworkData::kRoutePreferenceMedium;

    SuccessOrQuit(otBorderRouterAddOnMeshPrefix(sInstance, &prefixConfig));
    SuccessOrQuit(otBorderRouterRegister(sInstance));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare two services

    memset(&service1, 0, sizeof(service1));
    service1.mName          = kMatterServiceName;
    service1.mInstanceName  = kServiceInstance1;
    service1.mSubTypeLabels = kSubLabels1;
    service1.mTxtEntries    = kTxtEntries;
    service1.mNumTxtEntries = 4;
    service1.mPort          = 5540;
    service1.mWeight        = 0;
    service1.mPriority      = 0;

    memset(&service2, 0, sizeof(service2));
    service2.mName          = kMatterServiceName;
    service2.mInstanceName  = kServiceInstance2;
    service2.mSubTypeLabels = kSubLabels2;
    service2.mTxtEntries    = kTxtEntries;
    service2.mNumTxtEntries = 4;
    service2.mPort          = 5540;
    service2.mWeight        = 0;
    service2.mPriority      = 0;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, kUseSrpCoderOnClient);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup SRP coder callback

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    sInstance->Get<Srp::Coder>().SetDecodeCallback(ValidateTwoServicesWithSubtypeSameTxtData);
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register two services, validate that coded message is properly
    // formatted.

    SuccessOrQuit(srpClient->AddService(service1));
    SuccessOrQuit(srpClient->AddService(service2));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the host and service information on server

    host = srpServer->GetNextHost(nullptr);
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(StringStartsWith(host->GetFullName(), kHostName, kStringCaseInsensitiveMatch));

    // Only one host on server
    VerifyOrQuit(srpServer->GetNextHost(host) == nullptr);

    // Check host addresses
    VerifyOrQuit(host->GetAddresses(numAddresses)->MatchesPrefix(prefixConfig.GetPrefix()));
    VerifyOrQuit(numAddresses == 1);

    numServices = 0;

    for (const Srp::Server::Service &serverService : host->GetServices())
    {
        numServices++;

        VerifyOrQuit(StringStartsWith(serverService.GetServiceName(), kMatterServiceName));
        VerifyOrQuit(serverService.GetPort() == 5540);
        VerifyOrQuit(serverService.GetWeight() == 0);
        VerifyOrQuit(serverService.GetPriority() == 0);
        VerifyOrQuit(serverService.GetNumberOfSubTypes() == 1);

        if (StringMatch(serverService.GetInstanceLabel(), kServiceInstance1))
        {
            VerifyOrQuit(StringStartsWith(serverService.GetSubTypeServiceNameAt(0), kSubLabels1[0]));
        }
        else if (StringMatch(serverService.GetInstanceLabel(), kServiceInstance2))
        {
            VerifyOrQuit(StringStartsWith(serverService.GetSubTypeServiceNameAt(0), kSubLabels2[0]));
        }
        else
        {
            VerifyOrQuit(false);
        }

        VerifyOrQuit(serverService.GetTxtDataLength() == sizeof(kTxtData));
        VerifyOrQuit(memcmp(kTxtData, serverService.GetTxtData(), sizeof(kTxtData)) == 0);
    }

    VerifyOrQuit(numServices == 2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove the first service.

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    sInstance->Get<Srp::Coder>().SetDecodeCallback(nullptr);
#endif

    SuccessOrQuit(srpClient->RemoveService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove host and clear key-lease

    SuccessOrQuit(srpClient->RemoveHostAndServices(/* aShouldRemoveKeyLease */ true));

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRemoved);

    VerifyOrQuit(srpServer->GetNextHost(nullptr) == nullptr);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of SrpCoder::TestTwoServicesWithSubtypeSameTxtData()");
}

//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void ValidateTwoServicesDifferentTxtData(const Message &aCodedMsg, const Message &aMessage, Error aError)
{
    // Host : "DAAFF10F39B00F32" (kHostName)
    // - Three address - set directly on client
    //
    // Service 1:
    // - Instance : "8F097FD118441046-00000000B3B3D017" (kServiceInstance1)
    // - Type     : "_matter._tcp"
    //
    // Service 2:
    // - Instance : "1FF04909193C16E2-000000006CC07561" (kServiceInstance2)
    // - Type     : "_matter._tcp"

    Ip6::Address addresses[3];

    Log("ValidateTwoServicesDifferentTxtData()");
    LogCodedMessage(aCodedMsg, aMessage, aError);

    // Make sure host name is encoded properly (only
    // one instance of the name is included).

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kHostName) == 0);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kHostNameCoded) == 1);

    // Make sure service type "_matter._tcp" is encoded
    // properly. Both "matter" are "tcp" should be encoded
    // as a "commonly used" constant labels.

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "matter") == 0);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, "tcp") == 0);

    // Make sure service instance labels are encoded properly
    // and reused for the sub-type labels which uses same hex
    // value.

    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kServiceInstance1) == 0);
    VerifyOrQuit(CountOccurrencesOfStringIn(aCodedMsg, kServiceInstance2) == 0);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kInstance1Coded) == 1);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kInstance2Coded) == 1);

    // Different TXT data is used by the two services, make sure
    // they are encoded properly.
    //
    // The TXT data entries are intentionally set to be similar:
    //
    // - First service uses 4 `TxtEntry` (SII=1000, SAI=1000, SAT=4000, T=0)
    // - Second service uses 3 `TxtEntry` ((SII=1000, SAI=1000, SAT=4000)
    //
    // Second TXT data will not have [3, 'T', '=', '0'] (4 bytes)

    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kTxtData) == 1);
    VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, kTxtData, sizeof(kTxtData) - 4) == 2);

    // Make sure the three explicitly specified host addresses
    // are fully encoded in the message.

    SuccessOrQuit(addresses[0].FromString("fd01::"));
    SuccessOrQuit(addresses[1].FromString("fd02::"));
    SuccessOrQuit(addresses[2].FromString("fd03::"));

    for (const Ip6::Address &addr : addresses)
    {
        VerifyOrQuit(CountOccurrencesOfDataIn(aCodedMsg, &addr, sizeof(Ip6::Address)) == 1);
    }
}

#endif // OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void TestTwoServicesDifferentTxtData(void)
{
    Srp::Server             *srpServer;
    Srp::Client             *srpClient;
    Srp::Client::Service     service1;
    Srp::Client::Service     service2;
    Ip6::Address             addresses[3];
    const Srp::Server::Host *host;
    uint8_t                  numServices;
    uint8_t                  numAddresses;
    uint16_t                 heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestTwoServicesDifferentTxtData()");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare two services

    memset(&service1, 0, sizeof(service1));
    service1.mName          = kMatterServiceName;
    service1.mInstanceName  = kServiceInstance1;
    service1.mSubTypeLabels = nullptr;
    service1.mTxtEntries    = kTxtEntries;
    service1.mNumTxtEntries = 4;
    service1.mPort          = 1234;
    service1.mWeight        = 0;
    service1.mPriority      = 0;

    memset(&service2, 0, sizeof(service2));
    service2.mName          = kMatterServiceName;
    service2.mInstanceName  = kServiceInstance2;
    service2.mSubTypeLabels = nullptr;
    service2.mTxtEntries    = kTxtEntries;
    service2.mNumTxtEntries = 3;
    service2.mPort          = 5678;
    service2.mWeight        = 0;
    service2.mPriority      = 0;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, kUseSrpCoderOnClient);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));

    // Use explicitly set addresses for host

    SuccessOrQuit(addresses[0].FromString("fd01::"));
    SuccessOrQuit(addresses[1].FromString("fd02::"));
    SuccessOrQuit(addresses[2].FromString("fd03::"));
    SuccessOrQuit(srpClient->SetHostAddresses(addresses, 3));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup SRP coder callback

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    sInstance->Get<Srp::Coder>().SetDecodeCallback(ValidateTwoServicesDifferentTxtData);
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register two services, validate that coded message is properly
    // formatted.

    SuccessOrQuit(srpClient->AddService(service1));
    SuccessOrQuit(srpClient->AddService(service2));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check the host and service information on server

    host = srpServer->GetNextHost(nullptr);
    VerifyOrQuit(host != nullptr);
    VerifyOrQuit(StringStartsWith(host->GetFullName(), kHostName, kStringCaseInsensitiveMatch));

    // Only one host on server
    VerifyOrQuit(srpServer->GetNextHost(host) == nullptr);

    // Check host addresses
    host->GetAddresses(numAddresses);
    VerifyOrQuit(numAddresses == 3);

    numServices = 0;

    for (const Srp::Server::Service &serverService : host->GetServices())
    {
        numServices++;

        VerifyOrQuit(StringStartsWith(serverService.GetServiceName(), kMatterServiceName));
        VerifyOrQuit(serverService.GetWeight() == 0);
        VerifyOrQuit(serverService.GetPriority() == 0);

        //       VerifyOrQuit(serverService.GetTxtDataLength() == sizeof(kTxtData));
        //     VerifyOrQuit(memcmp(kTxtData, serverService.GetTxtData(), sizeof(kTxtData)) == 0);
    }

    VerifyOrQuit(numServices == 2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of SrpCoder::TestTwoServicesDifferentTxtData()");
}

#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

void TestSrpClientSkipCoderOnRepeatedFailures(void)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpClientSkipCoderOnRepeatedFailures()");

    InitTest();

    srpServer = &sInstance->Get<Srp::Server>();
    srpClient = &sInstance->Get<Srp::Client>();

    heapAllocations = sHeapAllocatedPtrs.GetLength();

    PrepareService1(service1);
    PrepareService2(service2);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP server.

    SuccessOrQuit(srpServer->SetAddressMode(Srp::Server::kAddressModeUnicast));
    VerifyOrQuit(srpServer->GetAddressMode() == Srp::Server::kAddressModeUnicast);

    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateDisabled);

    srpServer->SetServiceHandler(HandleSrpServerUpdate, sInstance);

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start SRP client.

    ApplySrpCoderMode(*srpClient, kUseSrpCoderOnClient);
    srpClient->SetCallback(HandleSrpClientCallback, sInstance);

    srpClient->EnableAutoStartMode(nullptr, nullptr);
    VerifyOrQuit(srpClient->IsAutoStartModeEnabled());

    AdvanceTime(2000);
    VerifyOrQuit(srpClient->IsRunning());

    SuccessOrQuit(srpClient->SetHostName(kHostName));
    SuccessOrQuit(srpClient->EnableAutoHostAddress());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Configure SRP server in `kTestModeRejectCodedMessage` so
    // it rejects a received SRP coded message with format-error

    srpServer->SetTestMode(Srp::Server::kTestModeRejectCodedMessage);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service on client

    SuccessOrQuit(srpClient->AddService(service1));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that due to `TestMode` config the server rejects the
    // received coded message.

    AdvanceTime(1000);

    VerifyOrQuit(!sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorParse);

    VerifyOrQuit(!srpClient->IsSkippingCoderDueToRepeatedFailures());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that upon repeated failures, the client skips
    // using the coder and sends an uncompressed SRP message
    // which is then accepted by the server.

    AdvanceTime(6 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(srpClient->IsSkippingCoderDueToRepeatedFailures());

    VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Unregister the service, validate that server is notified.

    SuccessOrQuit(srpClient->RemoveService(service1));

    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    AdvanceTime(2 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(srpClient->IsSkippingCoderDueToRepeatedFailures());

    VerifyOrQuit(service1.GetState() == Srp::Client::kRemoved);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server. Verify that client detects this and it is stopped.

    srpServer->SetEnabled(false);
    AdvanceTime(1000);

    VerifyOrQuit(!srpClient->IsRunning());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Re-enable SRP server.

    srpServer->SetEnabled(true);
    VerifyOrQuit(srpServer->GetState() != Srp::Server::kStateDisabled);

    AdvanceTime(10000);
    VerifyOrQuit(srpServer->GetState() == Srp::Server::kStateRunning);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that client is also re-started and
    // `SkippingCoderDueToRepeatedFailures` is cleared again.

    VerifyOrQuit(srpClient->IsRunning());
    VerifyOrQuit(!srpClient->IsSkippingCoderDueToRepeatedFailures());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Configure SRP server in `kTestModeIgnoreCodedMessage` so
    // it ignores a received SRP coded message and cause client
    // registration to timeout.

    srpServer->SetTestMode(Srp::Server::kTestModeIgnoreCodedMessage);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Register a service on client

    SuccessOrQuit(srpClient->AddService(service2));

    sUpdateHandlerMode       = kAccept;
    sProcessedUpdateCallback = false;
    sProcessedClientCallback = false;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that due to `TestMode` config the server ignores the
    // received coded message and we see timeout error on client.

    AdvanceTime(6 * 1000);

    VerifyOrQuit(!sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorResponseTimeout);

    VerifyOrQuit(!srpClient->IsSkippingCoderDueToRepeatedFailures());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that upon repeated failures, the client skips
    // using the coder and sends an uncompressed SRP message
    // which is then accepted by the server.

    AdvanceTime(45 * 1000);

    VerifyOrQuit(sProcessedUpdateCallback);
    VerifyOrQuit(sProcessedClientCallback);
    VerifyOrQuit(sLastClientCallbackError == kErrorNone);

    VerifyOrQuit(srpClient->IsSkippingCoderDueToRepeatedFailures());

    VerifyOrQuit(service2.GetState() == Srp::Client::kRegistered);
    ValidateHost(*srpServer, kHostName);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Disable SRP server, verify that all heap allocations by SRP server
    // are freed.

    Log("Disabling SRP server");

    srpServer->SetEnabled(false);
    AdvanceTime(100);

    VerifyOrQuit(heapAllocations == sHeapAllocatedPtrs.GetLength());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance and validate all heap allocations are freed.

    Log("Finalizing OT instance");
    FinalizeTest();

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpClientSkipCoderOnRepeatedFailures()");
}

#endif // OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE

} // namespace SrpCoder

#endif // OPENTHREAD_CONFIG_SRP_CODER_ENABLE

#endif // ENABLE_SRP_TEST

} // namespace ot

int main(void)
{
#if ENABLE_SRP_TEST
    ot::TestSrpServerBase(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerReject(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerIgnore(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ true, ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ false, ot::kDoNotUseSrpCoderOnClient);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    ot::TestUpdateLeaseShortVariant(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpClientDelayedResponse(ot::kDoNotUseSrpCoderOnClient);
#endif
    ot::TestSrpServerAddressModeForceAdd();
#if OPENTHREAD_CONFIG_SRP_SERVER_FAST_START_MODE_ENABLE
    ot::TestSrpServerFastStartMode();
#endif

#if OPENTHREAD_CONFIG_SRP_CODER_ENABLE
    ot::TestSrpServerBase(ot::kUseSrpCoderOnClient);
    ot::TestSrpServerReject(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerIgnore(ot::kDoNotUseSrpCoderOnClient);
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ true, ot::kUseSrpCoderOnClient);
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ false, ot::kUseSrpCoderOnClient);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    ot::TestSrpClientDelayedResponse(ot::kUseSrpCoderOnClient);
#endif
    ot::SrpCoder::TestSingleService();
    ot::SrpCoder::TestTwoServicesWithSubtypeSameTxtData();
    ot::SrpCoder::TestTwoServicesDifferentTxtData();
#if OPENTHREAD_CONFIG_SRP_CODER_TEST_API_ENABLE
    ot::SrpCoder::TestSrpClientSkipCoderOnRepeatedFailures();
#endif
#endif // OPENTHREAD_CONFIG_SRP_CODER_ENABLE

    printf("All tests passed\n");
#else
    printf("SRP_SERVER or SRP_CLIENT feature is not enabled\n");
#endif // ENABLE_SRP_TESTq

    return 0;
}
