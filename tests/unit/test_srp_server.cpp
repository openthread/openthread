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

#include <openthread/srp_client.h>
#include <openthread/srp_server.h>
#include <openthread/thread.h>

#include "common/arg_macros.hpp"
#include "common/array.hpp"
#include "common/instance.hpp"
#include "common/string.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE && OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE && \
    !OPENTHREAD_CONFIG_TIME_SYNC_ENABLE && !OPENTHREAD_PLATFORM_POSIX
#define ENABLE_SRP_TEST 1
#else
#define ENABLE_SRP_TEST 0
#endif

#if ENABLE_SRP_TEST

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

otError otPlatRadioTransmit(otInstance *, otRadioFrame *)
{
    sRadioTxOngoing = true;

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *)
{
    return &sRadioTxFrame;
}

//----------------------------------------------------------------------------------------------------------------------
// `otPlatAlaram`

void otPlatAlarmMilliStop(otInstance *)
{
    sAlarmOn = false;
}

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return sNow;
}

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

    while (sAlarmTime <= time)
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
    sInstance = static_cast<Instance *>(testInitInstance());

    memset(&sRadioTxFrame, 0, sizeof(sRadioTxFrame));
    sRadioTxFrame.mPsdu = sRadioTxFramePsdu;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Initialize Border Router and start Thread operation.

    SuccessOrQuit(otLinkSetPanId(sInstance, 0x1234));
    SuccessOrQuit(otIp6SetEnabled(sInstance, true));
    SuccessOrQuit(otThreadSetEnabled(sInstance, true));

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Ensure device starts as leader.

    AdvanceTime(10000);

    VerifyOrQuit(otThreadGetDeviceRole(sInstance) == OT_DEVICE_ROLE_LEADER);
}

//---------------------------------------------------------------------------------------------------------------------

enum UpdateHandlerMode
{
    kAccept, // Accept all updates.
    kReject, // Reject all updates.
    kIgnore  // Ignore all updates (do not call `otSrpServerHandleServiceUpdateResult()`).
};

static UpdateHandlerMode sUpdateHandlerMode       = kAccept;
static bool              sProcessedUpdateCallback = false;

void HandleSrpServerUpdate(otSrpServerServiceUpdateId aId,
                           const otSrpServerHost *    aHost,
                           uint32_t                   aTimeout,
                           void *                     aContext)
{
    Log("HandleSrpServerUpdate() called with %u, timeout:%u", aId, aTimeout);

    VerifyOrQuit(aHost != nullptr);
    VerifyOrQuit(aContext == sInstance);

    sProcessedUpdateCallback = true;

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
                             const otSrpClientService * aServices,
                             const otSrpClientService * aRemovedServices,
                             void *                     aContext)
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
    static const char          kInstanceLabel[] = "srv-instance";
    static const char          kSub1[]          = "_sub1";
    static const char          kSub2[]          = "_V1234567";
    static const char          kSub3[]          = "_XYZWS";
    static const char *        kSubLabels[]     = {kSub1, kSub2, kSub3, nullptr};
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
    const char *             name;

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
    Srp::Server *        srpServer;
    Srp::Client *        srpClient;
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
    testFreeInstance(sInstance);

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerBase");
}

void TestSrpServerReject(void)
{
    Srp::Server *        srpServer;
    Srp::Client *        srpClient;
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

    AdvanceTime(2000);
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
    testFreeInstance(sInstance);

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerReject");
}

void TestSrpServerIgnore(void)
{
    Srp::Server *        srpServer;
    Srp::Client *        srpClient;
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

    AdvanceTime(2000);
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
    testFreeInstance(sInstance);

    VerifyOrQuit(sHeapAllocatedPtrs.IsEmpty());

    Log("End of TestSrpServerIgnore");
}

#endif // ENABLE_SRP_TEST

int main(void)
{
#if ENABLE_SRP_TEST
    TestSrpServerBase();
    TestSrpServerReject();
    TestSrpServerIgnore();
    printf("All tests passed\n");
#else
    printf("SRP_SERVER or SRP_CLIENT feature is not enabled\n");
#endif

    return 0;
}
