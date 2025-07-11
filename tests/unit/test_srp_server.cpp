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

void TestSrpServerBase(void)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerBase");

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

    Log("End of TestSrpServerBase");
}

void TestSrpServerReject(void)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerReject");

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

    Log("End of TestSrpServerReject");
}

void TestSrpServerIgnore(void)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerIgnore");

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

    Log("End of TestSrpServerIgnore");
}

void TestSrpServerClientRemove(bool aShouldRemoveKeyLease)
{
    Srp::Server         *srpServer;
    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;
    uint16_t             heapAllocations;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpServerClientRemove(aShouldRemoveKeyLease:%u)", aShouldRemoveKeyLease);

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

    Log("End of TestSrpServerClientRemove");
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void TestUpdateLeaseShortVariant(void)
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
    Log("TestUpdateLeaseShortVariant");

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

    Log("End of TestUpdateLeaseShortVariant");
}

static uint16_t         sServerRxCount;
static Ip6::MessageInfo sServerMsgInfo;
static uint16_t         sServerLastMsgId;
static uint16_t         sServerLastMsgLength;

void HandleServerUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Dns::Header header;

    VerifyOrQuit(aContext == nullptr);
    VerifyOrQuit(aMessage != nullptr);
    VerifyOrQuit(aMessageInfo != nullptr);

    SuccessOrQuit(AsCoreType(aMessage).Read(0, header));

    sServerMsgInfo       = AsCoreType(aMessageInfo);
    sServerLastMsgId     = header.GetMessageId();
    sServerLastMsgLength = AsCoreType(aMessage).GetLength();
    sServerRxCount++;

    Log("HandleServerUdpReceive(), message-id:0x%x, message-len:%u", sServerLastMsgId, sServerLastMsgLength);
}

void TestSrpClientDelayedResponse(void)
{
    static constexpr uint16_t kServerPort = 53535;

    Srp::Client         *srpClient;
    Srp::Client::Service service1;
    Srp::Client::Service service2;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpClientDelayedResponse");

    InitTest();

    srpClient = &sInstance->Get<Srp::Client>();

    for (uint8_t testIter = 0; testIter < 2; testIter++)
    {
        Log("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
        Log("testIter = %u", testIter);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Prepare a socket to act as SRP server.

        Ip6::Udp::Socket  udpSocket(*sInstance, HandleServerUdpReceive, nullptr);
        Ip6::SockAddr     serverSockAddr;
        uint16_t          firstMsgId;
        uint16_t          secondMsgId;
        Message          *response;
        Dns::UpdateHeader header;

        sServerRxCount = 0;

        SuccessOrQuit(udpSocket.Open(Ip6::kNetifThreadInternal));
        SuccessOrQuit(udpSocket.Bind(kServerPort));

        serverSockAddr.SetAddress(sInstance->Get<Mle::Mle>().GetMeshLocalRloc());
        serverSockAddr.SetPort(kServerPort);
        SuccessOrQuit(srpClient->Start(serverSockAddr));

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
        firstMsgId     = sServerLastMsgId;
        sServerRxCount = 0;

        if (testIter == 1)
        {
            // In the second test iteration, register a second
            // service. Ensure that client uses a new ID for new
            // updated SRP message (containing both services).

            AdvanceTime(5 * 1000);

            PrepareService2(service2);
            SuccessOrQuit(srpClient->AddService(service2));

            AdvanceTime(20 * 1000);
            VerifyOrQuit(sServerRxCount > 1);
            VerifyOrQuit(sServerLastMsgId != firstMsgId);
            secondMsgId    = sServerLastMsgId;
            sServerRxCount = 0;
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for longer to allow client to retry a bunch of times.
        // Ensure the same ID is used for retries.

        AdvanceTime(60 * 1000);
        VerifyOrQuit(sServerRxCount > 1);

        switch (testIter)
        {
        case 0:
            VerifyOrQuit(sServerLastMsgId == firstMsgId);
            break;
        case 1:
            VerifyOrQuit(sServerLastMsgId == secondMsgId);
            break;
        }

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
        // In the first test iteration, we ensure that client did
        // successfully accept the response with first message ID.
        // This should not be the case in the second iteration due to
        // changes to client services after the first Update message
        // was sent by client.

        switch (testIter)
        {
        case 0:
            VerifyOrQuit(srpClient->GetHostInfo().GetState() == Srp::Client::kRegistered);
            VerifyOrQuit(service1.GetState() == Srp::Client::kRegistered);
            break;
        case 1:
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

    Log("End of TestSrpClientDelayedResponse");
}

void TestSrpClientSingleServiceMode(void)
{
    static constexpr uint16_t kNumServices = 5;
    static constexpr uint16_t kServerPort  = 53535;

    static const char *kSubLabels[] = {"_longsubtypelebel11111", "_longsubtypelebel2222222", nullptr};

    Srp::Client           *srpClient;
    Srp::Client::Service   services[kNumServices];
    Dns::Name::LabelBuffer serviceInstnaces[kNumServices];

    Log("--------------------------------------------------------------------------------------------");
    Log("TestSrpClientSingleServiceMode");

    InitTest();

    srpClient = &sInstance->Get<Srp::Client>();

    {
        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Prepare a socket to act as SRP server.

        Ip6::Udp::Socket  udpSocket(*sInstance, HandleServerUdpReceive, nullptr);
        Ip6::SockAddr     serverSockAddr;
        uint16_t          firstMsgId;
        uint16_t          secondMsgId;
        uint16_t          firstMsgLength;
        uint16_t          numServices;
        Message          *response;
        Dns::UpdateHeader header;

        sServerRxCount = 0;

        SuccessOrQuit(udpSocket.Open(Ip6::kNetifThreadInternal));
        SuccessOrQuit(udpSocket.Bind(kServerPort));

        serverSockAddr.SetAddress(sInstance->Get<Mle::Mle>().GetMeshLocalRloc());
        serverSockAddr.SetPort(kServerPort);
        SuccessOrQuit(srpClient->Start(serverSockAddr));

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Prepare five services with long service names and multiple sub-types.

        for (uint16_t i = 0; i < GetArrayLength(services); i++)
        {
            StringWriter writer(serviceInstnaces[i], sizeof(Dns::Name::LabelBuffer));

            writer.Append("IncrediblyLongServiceInstanceName-001122334455667788-%02X", i);

            ClearAllBytes(services[i]);
            services[i].mName          = "_longsrvname._udp";
            services[i].mInstanceName  = serviceInstnaces[i];
            services[i].mSubTypeLabels = kSubLabels;
            services[i].mTxtEntries    = nullptr;
            services[i].mNumTxtEntries = 0;
            services[i].mPort          = 5536 + i;
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Register four services (they should still fit in an IPv6 MTU).

        SuccessOrQuit(srpClient->SetHostName("SuperLongHostNameAABBCCDDEEFF001122334455667788990123457889"));
        SuccessOrQuit(srpClient->EnableAutoHostAddress());

        SuccessOrQuit(srpClient->AddService(services[0]));
        SuccessOrQuit(srpClient->AddService(services[1]));
        SuccessOrQuit(srpClient->AddService(services[2]));
        SuccessOrQuit(srpClient->AddService(services[3]));

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for a short time for the server to receive the SRP
        // update from the client. Verify that the message is smaller
        // than the IPv6 MTU and includes all services.

        AdvanceTime(1 * 1000);

        VerifyOrQuit(sServerRxCount == 1);
        firstMsgId     = sServerLastMsgId;
        firstMsgLength = sServerLastMsgLength;
        sServerRxCount = 0;

        VerifyOrQuit(services[0].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[1].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[2].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[3].GetState() == Srp::Client::kAdding);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait longer to allow the client to retry multiple times, and
        // ensure the same ID is used for all retries.

        AdvanceTime(60 * 1000);
        VerifyOrQuit(sServerRxCount > 1);
        VerifyOrQuit(sServerLastMsgId == firstMsgId);

        VerifyOrQuit(services[0].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[1].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[2].GetState() == Srp::Client::kAdding);
        VerifyOrQuit(services[3].GetState() == Srp::Client::kAdding);

        sServerRxCount = 0;

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Register a fifth service, causing the SRP update to exceed
        // the MTU limit. The client should now enter "single service
        // mode" and register services one by one.

        SuccessOrQuit(srpClient->AddService(services[4]));

        AdvanceTime(60 * 1000);
        VerifyOrQuit(sServerRxCount > 1);
        VerifyOrQuit(sServerLastMsgId != firstMsgId);
        VerifyOrQuit(sServerLastMsgLength < firstMsgLength);

        secondMsgId = sServerLastMsgId;

        // Check that only one service is included in the message.

        numServices = 0;

        for (const Srp::Client::Service &service : services)
        {
            switch (service.GetState())
            {
            case Srp::Client::kToAdd:
            case Srp::Client::kToRefresh:
                break;

            case Srp::Client::kAdding:
            case Srp::Client::kRefreshing:
                numServices++;
                break;

            default:
                VerifyOrQuit(false);
            }
        }

        VerifyOrQuit(numServices == 1);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Now send a response from server accepting the registration.

        response = udpSocket.NewMessage();
        VerifyOrQuit(response != nullptr);

        Log("Sending response with msg-id: 0x%x", secondMsgId);

        header.SetMessageId(secondMsgId);
        header.SetType(Dns::UpdateHeader::kTypeResponse);
        header.SetResponseCode(Dns::UpdateHeader::kResponseSuccess);
        SuccessOrQuit(response->Append(header));
        SuccessOrQuit(udpSocket.SendTo(*response, sServerMsgInfo));

        sServerRxCount = 0;
        AdvanceTime(10);

        // Check that exactly one service is successfully
        // registered.

        numServices = 0;

        for (const Srp::Client::Service &service : services)
        {
            switch (service.GetState())
            {
            case Srp::Client::kToAdd:
            case Srp::Client::kToRefresh:
            case Srp::Client::kAdding:
            case Srp::Client::kRefreshing:
                break;
            case Srp::Client::kRegistered:
                numServices++;
                break;

            default:
                VerifyOrQuit(false);
            }
        }

        VerifyOrQuit(numServices == 1);

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for the client to register the remaining services and
        // validate that it used a new message ID.

        AdvanceTime(60 * 1000);

        VerifyOrQuit(sServerRxCount > 1);
        VerifyOrQuit(sServerLastMsgId != secondMsgId);

        // Check that all remaining services are included in
        // the message.

        numServices = 0;

        for (const Srp::Client::Service &service : services)
        {
            switch (service.GetState())
            {
            case Srp::Client::kAdding:
            case Srp::Client::kRefreshing:
                break;
            case Srp::Client::kRegistered:
                numServices++;
                break;

            default:
                VerifyOrQuit(false);
            }
        }

        VerifyOrQuit(numServices == 1);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Finalize OT instance

    Log("Finalizing OT instance");
    FinalizeTest();

    Log("End of TestSrpClientSingleServiceMode");
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

#endif // ENABLE_SRP_TEST

} // namespace ot

int main(void)
{
#if ENABLE_SRP_TEST
    ot::TestSrpServerBase();
    ot::TestSrpServerReject();
    ot::TestSrpServerIgnore();
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ true);
    ot::TestSrpServerClientRemove(/* aShouldRemoveKeyLease */ false);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    ot::TestUpdateLeaseShortVariant();
    ot::TestSrpClientDelayedResponse();
    ot::TestSrpClientSingleServiceMode();
#endif
    ot::TestSrpServerAddressModeForceAdd();
#if OPENTHREAD_CONFIG_SRP_SERVER_FAST_START_MODE_ENABLE
    ot::TestSrpServerFastStartMode();
#endif

    printf("All tests passed\n");
#else
    printf("SRP_SERVER or SRP_CLIENT feature is not enabled\n");
#endif

    return 0;
}
