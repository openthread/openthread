/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "precomp.h"
#include "otApi.tmh"

//#define DEBUG_ASYNC_IO

using namespace std;

// The maximum time we will wait for an overlapped result. Essentially, the maximum
// amount of time each synchronous IOCTL should take.
const DWORD c_MaxOverlappedWaitTimeMS = 5 * 1000;

// Version string returned by the API
const char c_Version[] = "Windows"; // TODO - What should we really put here?

template<class CallbackT>
class otCallback
{
public:
    RTL_REFERENCE_COUNT CallbackRefCount;
    HANDLE              CallbackCompleteEvent;
    GUID                InterfaceGuid;
    CallbackT           Callback;
    PVOID               CallbackContext;

    otCallback(
        CallbackT _Callback,
        PVOID _CallbackContext
        ) : 
        CallbackRefCount(1),
        CallbackCompleteEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)),
        Callback(_Callback),
        CallbackContext(_CallbackContext)
    {
    }

    otCallback(
        const GUID& _InterfaceGuid,
        CallbackT _Callback,
        PVOID _CallbackContext
        ) : 
        CallbackRefCount(1),
        CallbackCompleteEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)),
        InterfaceGuid(_InterfaceGuid),
        Callback(_Callback),
        CallbackContext(_CallbackContext)
    {
    }

    ~otCallback()
    {
        if (CallbackCompleteEvent) CloseHandle(CallbackCompleteEvent);
    }

    void AddRef()
    {
        RtlIncrementReferenceCount(&CallbackRefCount);
    }

    void Release(bool waitForShutdown = false)
    {
        if (RtlDecrementReferenceCount(&CallbackRefCount))
        {
            // Set completion event if there are no more refs
            SetEvent(CallbackCompleteEvent);
        }

        if (waitForShutdown)
        {
            WaitForSingleObject(CallbackCompleteEvent, INFINITE);
        }
    }
};

typedef otCallback<otDeviceAvailabilityChangedCallback> otApiDeviceAvailabilityCallback;
typedef otCallback<otHandleActiveScanResult> otApiActiveScanCallback;
typedef otCallback<otHandleEnergyScanResult> otApiEnergyScanCallback;
typedef otCallback<otStateChangedCallback> otApiStateChangeCallback;
typedef otCallback<otCommissionerEnergyReportCallback> otApiCommissionerEnergyReportCallback;
typedef otCallback<otCommissionerPanIdConflictCallback> otApiCommissionerPanIdConflictCallback;
typedef otCallback<otJoinerCallback> otApiJoinerCallback;

typedef struct otApiInstance
{
    // Handle to the driver
    HANDLE                      DeviceHandle;

    // Async IO variables
    OVERLAPPED                  Overlapped;
    PTP_WAIT                    ThreadpoolWait;

    // Notification variables
    CRITICAL_SECTION            CallbackLock;
    OTLWF_NOTIFICATION          NotificationBuffer;

    // Callbacks
    otApiDeviceAvailabilityCallback*    DeviceAvailabilityCallbacks;
    vector<otApiActiveScanCallback*>    ActiveScanCallbacks;
    vector<otApiEnergyScanCallback*>    EnergyScanCallbacks;
    vector<otApiActiveScanCallback*>    DiscoverCallbacks;
    vector<otApiStateChangeCallback*>   StateChangedCallbacks;
    vector<otApiCommissionerEnergyReportCallback*>  CommissionerEnergyReportCallbacks;
    vector<otApiCommissionerPanIdConflictCallback*> CommissionerPanIdConflictCallbacks;
    vector<otApiJoinerCallback*>        JoinerCallbacks;

    // Constructor
    otApiInstance() : 
        DeviceHandle(INVALID_HANDLE_VALUE),
        Overlapped({0}),
        ThreadpoolWait(nullptr),
        DeviceAvailabilityCallbacks(nullptr)
    { 
        InitializeCriticalSection(&CallbackLock);
    }

    ~otApiInstance()
    {
        DeleteCriticalSection(&CallbackLock);
    }

    // Helper function to set a callback
    template<class CallbackT>
    bool 
    SetCallback(
        vector<otCallback<CallbackT>*> &Callbacks, 
        const GUID& InterfaceGuid,
        CallbackT Callback,
        PVOID CallbackContext
        )
    {
        bool alreadyExists = false;
        otCallback<CallbackT>* CallbackToRelease = nullptr;

        EnterCriticalSection(&CallbackLock);

        if (Callback == nullptr)
        {
            for (size_t i = 0; i < Callbacks.size(); i++)
            {
                if (Callbacks[i]->InterfaceGuid == InterfaceGuid)
                {
                    CallbackToRelease = Callbacks[i];
                    Callbacks.erase(Callbacks.begin() + i);
                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < Callbacks.size(); i++)
            {
                if (Callbacks[i]->InterfaceGuid == InterfaceGuid)
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
            {
                Callbacks.push_back(new otCallback<CallbackT>(InterfaceGuid, Callback, CallbackContext));
            }
        }

        LeaveCriticalSection(&CallbackLock);

        if (CallbackToRelease)
        {
            CallbackToRelease->Release(true);
            delete CallbackToRelease;
        }

        return !alreadyExists;
    }

} otApiInstance;

typedef struct otInstance
{
    otApiInstance   *ApiHandle;      // Pointer to the Api handle
    NET_IFINDEX      InterfaceIndex; // Interface Index
    NET_LUID         InterfaceLuid;  // Interface Luid
    GUID             InterfaceGuid;  // Interface guid
    ULONG            CompartmentID;  // Interface Compartment ID

} otInstance;

// otpool wait callback for async IO completion
VOID CALLBACK 
otIoComplete(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              Wait,
    _In_        TP_WAIT_RESULT        WaitResult
    );

OTAPI 
otApiInstance *
OTCALL
otApiInit(
    )
{
    DWORD dwError = ERROR_SUCCESS;
    otApiInstance *aApitInstance = nullptr;
    
    LogFuncEntry(API_DEFAULT);

    aApitInstance = new(std::nothrow)otApiInstance();
    if (aApitInstance == nullptr)
    {
        dwError = GetLastError();
        LogWarning(API_DEFAULT, "Failed to allocate otApiInstance");
        goto error;
    }

    // Open the pipe to the OpenThread driver
    aApitInstance->DeviceHandle = 
        CreateFile(
            OTLWF_IOCLT_PATH,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,                // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING,          // No special create flags
            FILE_FLAG_OVERLAPPED,   // Allow asynchronous requests
            nullptr
            );
    if (aApitInstance->DeviceHandle == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        LogError(API_DEFAULT, "CreateFile failed, %!WINERROR!", dwError);
        goto error;
    }

    // Create event for completion of async IO
    aApitInstance->Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (aApitInstance->Overlapped.hEvent == nullptr)
    {
        dwError = GetLastError();
        LogError(API_DEFAULT, "CreateEvent (Overlapped.hEvent) failed, %!WINERROR!", dwError);
        goto error;
    }

    // Create the otpool wait
    aApitInstance->ThreadpoolWait = 
        CreateThreadpoolWait(
            otIoComplete,
            aApitInstance,
            nullptr
            );
    if (aApitInstance->ThreadpoolWait == nullptr)
    {
        dwError = GetLastError();
        LogError(API_DEFAULT, "CreateThreadpoolWait failed, %!WINERROR!", dwError);
        goto error;
    }

    // Start the otpool waiting on the overlapped event
    SetThreadpoolWait(aApitInstance->ThreadpoolWait, aApitInstance->Overlapped.hEvent, nullptr);

#ifdef DEBUG_ASYNC_IO
    LogVerbose(API_DEFAULT, "Querying for 1st notification");
#endif

    // Request first notification asynchronously
    if (!DeviceIoControl(
            aApitInstance->DeviceHandle,
            IOCTL_OTLWF_QUERY_NOTIFICATION,
            nullptr, 0,
            &aApitInstance->NotificationBuffer, sizeof(OTLWF_NOTIFICATION),
            nullptr, 
            &aApitInstance->Overlapped))
    {
        dwError = GetLastError();
        if (dwError != ERROR_IO_PENDING)
        {
            LogError(API_DEFAULT, "DeviceIoControl for first notification failed, %!WINERROR!", dwError);
            goto error;
        }
        dwError = ERROR_SUCCESS;
    }

error:

    if (dwError != ERROR_SUCCESS)
    {
        otApiFinalize(aApitInstance);
        aApitInstance = nullptr;
    }
    
    LogFuncExit(API_DEFAULT);

    return aApitInstance;
}

OTAPI 
void 
OTCALL
otApiFinalize(
    _In_ otApiInstance *aApitInstance
)
{
    if (aApitInstance == nullptr) return;
    
    LogFuncEntry(API_DEFAULT);

    // If we never got the handle, nothing left to clean up
    if (aApitInstance->DeviceHandle != INVALID_HANDLE_VALUE)
    {
        //
        // Make sure we unregister callbacks
        //

        EnterCriticalSection(&aApitInstance->CallbackLock);

        otApiDeviceAvailabilityCallback* DeviceAvailabilityCallbacks = aApitInstance->DeviceAvailabilityCallbacks;
        aApitInstance->DeviceAvailabilityCallbacks = nullptr;

        vector<otApiActiveScanCallback*> ActiveScanCallbacks(aApitInstance->ActiveScanCallbacks);
        aApitInstance->ActiveScanCallbacks.clear();

        vector<otApiEnergyScanCallback*> EnergyScanCallbacks(aApitInstance->EnergyScanCallbacks);
        aApitInstance->EnergyScanCallbacks.clear();

        vector<otApiActiveScanCallback*> DiscoverCallbacks(aApitInstance->DiscoverCallbacks);
        aApitInstance->DiscoverCallbacks.clear();

        vector<otApiStateChangeCallback*> StateChangedCallbacks(aApitInstance->StateChangedCallbacks);
        aApitInstance->StateChangedCallbacks.clear();

        vector<otApiCommissionerEnergyReportCallback*> CommissionerEnergyReportCallbacks(aApitInstance->CommissionerEnergyReportCallbacks);
        aApitInstance->CommissionerEnergyReportCallbacks.clear();

        vector<otApiCommissionerPanIdConflictCallback*> CommissionerPanIdConflictCallbacks(aApitInstance->CommissionerPanIdConflictCallbacks);
        aApitInstance->CommissionerPanIdConflictCallbacks.clear();

        vector<otApiJoinerCallback*> JoinerCallbacks(aApitInstance->JoinerCallbacks);
        aApitInstance->JoinerCallbacks.clear();

        #ifdef DEBUG_ASYNC_IO
        LogVerbose(API_DEFAULT, "Clearing Threadpool Wait");
        #endif

        // Clear the threadpool wait to prevent further waits from being scheduled
        PTP_WAIT tpWait = aApitInstance->ThreadpoolWait;
        aApitInstance->ThreadpoolWait = nullptr;

        LeaveCriticalSection(&aApitInstance->CallbackLock);

        // Clear all callbacks
        if (DeviceAvailabilityCallbacks)
        {
            DeviceAvailabilityCallbacks->Release(true);
            delete DeviceAvailabilityCallbacks;
        }
        for (size_t i = 0; i < ActiveScanCallbacks.size(); i++)
        {
            ActiveScanCallbacks[i]->Release(true);
            delete ActiveScanCallbacks[i];
        }
        for (size_t i = 0; i < EnergyScanCallbacks.size(); i++)
        {
            EnergyScanCallbacks[i]->Release(true);
            delete EnergyScanCallbacks[i];
        }
        for (size_t i = 0; i < DiscoverCallbacks.size(); i++)
        {
            DiscoverCallbacks[i]->Release(true);
            delete DiscoverCallbacks[i];
        }
        for (size_t i = 0; i < StateChangedCallbacks.size(); i++)
        {
            StateChangedCallbacks[i]->Release(true);
            delete StateChangedCallbacks[i];
        }
        for (size_t i = 0; i < CommissionerEnergyReportCallbacks.size(); i++)
        {
            CommissionerEnergyReportCallbacks[i]->Release(true);
            delete CommissionerEnergyReportCallbacks[i];
        }
        for (size_t i = 0; i < CommissionerPanIdConflictCallbacks.size(); i++)
        {
            CommissionerPanIdConflictCallbacks[i]->Release(true);
            delete CommissionerPanIdConflictCallbacks[i];
        }
        for (size_t i = 0; i < JoinerCallbacks.size(); i++)
        {
            JoinerCallbacks[i]->Release(true);
            delete JoinerCallbacks[i];
        }
        
        // Clean up threadpool wait
        if (tpWait)
        {
            #ifdef DEBUG_ASYNC_IO
            LogVerbose(API_DEFAULT, "Waiting for outstanding threadpool callbacks to compelte");
            #endif

            // Cancel any queued waits and wait for any outstanding calls to compelte
            WaitForThreadpoolWaitCallbacks(tpWait, TRUE);
        
            #ifdef DEBUG_ASYNC_IO
            LogVerbose(API_DEFAULT, "Cancelling any pending IO");
            #endif

            // Cancel any async IO
            CancelIoEx(aApitInstance->DeviceHandle, &aApitInstance->Overlapped);

            // Free the threadpool wait
            CloseThreadpoolWait(tpWait);
        }

        // Clean up overlapped event
        if (aApitInstance->Overlapped.hEvent)
        {
            CloseHandle(aApitInstance->Overlapped.hEvent);
        }
    
        // Close the device handle
        CloseHandle(aApitInstance->DeviceHandle);
    }

    delete aApitInstance;
    
    LogFuncExit(API_DEFAULT);
}

OTAPI 
void 
OTCALL
otFreeMemory(
    _In_ const void *mem
    )
{
    free((void*)mem);
}

// Handles cleanly invoking the register callback
VOID
ProcessNotification(
    _In_ otApiInstance         *aApitInstance,
    _In_ POTLWF_NOTIFICATION    Notif
    )
{
    if (Notif->NotifType == OTLWF_NOTIF_DEVICE_AVAILABILITY)
    {
        otCallback<otDeviceAvailabilityChangedCallback>* Callback = nullptr;
        
        EnterCriticalSection(&aApitInstance->CallbackLock);

        if (aApitInstance->DeviceAvailabilityCallbacks != nullptr)
        {
            aApitInstance->DeviceAvailabilityCallbacks->AddRef();
            Callback = aApitInstance->DeviceAvailabilityCallbacks;
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);

        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->DeviceAvailabilityPayload.Available != FALSE, 
                &Notif->InterfaceGuid, 
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_STATE_CHANGE)
    {
        otCallback<otStateChangedCallback>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->StateChangedCallbacks.size(); i++)
        {
            if (aApitInstance->StateChangedCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->StateChangedCallbacks[i]->AddRef();
                Callback = aApitInstance->StateChangedCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->StateChangePayload.Flags, 
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_DISCOVER)
    {
        otCallback<otHandleActiveScanResult>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->DiscoverCallbacks.size(); i++)
        {
            if (aApitInstance->DiscoverCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->DiscoverCallbacks[i]->AddRef();
                Callback = aApitInstance->DiscoverCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->DiscoverPayload.Valid ? &Notif->DiscoverPayload.Results : nullptr, 
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_ACTIVE_SCAN)
    {
        otCallback<otHandleActiveScanResult>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->ActiveScanCallbacks.size(); i++)
        {
            if (aApitInstance->ActiveScanCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->ActiveScanCallbacks[i]->AddRef();
                Callback = aApitInstance->ActiveScanCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->ActiveScanPayload.Valid ? &Notif->ActiveScanPayload.Results : nullptr, 
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_ENERGY_SCAN)
    {
        otCallback<otHandleEnergyScanResult>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->EnergyScanCallbacks.size(); i++)
        {
            if (aApitInstance->EnergyScanCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->EnergyScanCallbacks[i]->AddRef();
                Callback = aApitInstance->EnergyScanCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->EnergyScanPayload.Valid ? &Notif->EnergyScanPayload.Results : nullptr, 
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_COMMISSIONER_ENERGY_REPORT)
    {
        otCallback<otCommissionerEnergyReportCallback>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->CommissionerEnergyReportCallbacks.size(); i++)
        {
            if (aApitInstance->CommissionerEnergyReportCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->CommissionerEnergyReportCallbacks[i]->AddRef();
                Callback = aApitInstance->CommissionerEnergyReportCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->CommissionerEnergyReportPayload.ChannelMask,
                Notif->CommissionerEnergyReportPayload.EnergyList,
                Notif->CommissionerEnergyReportPayload.EnergyListLength,
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_COMMISSIONER_PANID_QUERY)
    {
        otCallback<otCommissionerPanIdConflictCallback>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->CommissionerPanIdConflictCallbacks.size(); i++)
        {
            if (aApitInstance->CommissionerPanIdConflictCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->CommissionerPanIdConflictCallbacks[i]->AddRef();
                Callback = aApitInstance->CommissionerPanIdConflictCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            Callback->Callback(
                Notif->CommissionerPanIdQueryPayload.PanId,
                Notif->CommissionerPanIdQueryPayload.ChannelMask,
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else if (Notif->NotifType == OTLWF_NOTIF_JOINER_COMPLETE)
    {
        otCallback<otJoinerCallback>* Callback = nullptr;

        EnterCriticalSection(&aApitInstance->CallbackLock);

        for (size_t i = 0; i < aApitInstance->JoinerCallbacks.size(); i++)
        {
            if (aApitInstance->JoinerCallbacks[i]->InterfaceGuid == Notif->InterfaceGuid)
            {
                aApitInstance->JoinerCallbacks[i]->AddRef();
                Callback = aApitInstance->JoinerCallbacks[i];
                break;
            }
        }

        LeaveCriticalSection(&aApitInstance->CallbackLock);
        
        // Invoke the callback outside the lock and release ref when done
        if (Callback)
        {
            aApitInstance->SetCallback(
                aApitInstance->JoinerCallbacks,
                Notif->InterfaceGuid, (otJoinerCallback)nullptr, (PVOID)nullptr
                );

            Callback->Callback(
                Notif->JoinerCompletePayload.Error,
                Callback->CallbackContext);

            Callback->Release();
        }
    }
    else
    {
        // Unexpected notif type
    }
}

// Threadpool wait callback for async IO completion
VOID CALLBACK 
otIoComplete(
    _Inout_     PTP_CALLBACK_INSTANCE /* Instance */,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              /* Wait */,
    _In_        TP_WAIT_RESULT        /* WaitResult */
    )
{
#ifdef DEBUG_ASYNC_IO
    LogFuncEntry(API_DEFAULT);
#endif

    otApiInstance *aApitInstance = (otApiInstance*)Context;
    if (aApitInstance == nullptr) return;

    // Get the result of the IO operation
    DWORD dwBytesTransferred = 0;
    if (!GetOverlappedResult(
            aApitInstance->DeviceHandle,
            &aApitInstance->Overlapped,
            &dwBytesTransferred,
            FALSE))
    {
        DWORD dwError = GetLastError();
        LogError(API_DEFAULT, "GetOverlappedResult for notification failed, %!WINERROR!", dwError);
    }
    else
    {
        LogVerbose(API_DEFAULT, "Received successful callback for notification, type=%d", 
                     aApitInstance->NotificationBuffer.NotifType);

        // Invoke the callback if set
        ProcessNotification(aApitInstance, &aApitInstance->NotificationBuffer);
            
        // Try to get the threadpool wait to see if we are allowed to continue processing notifications
        EnterCriticalSection(&aApitInstance->CallbackLock);
        PTP_WAIT tpWait = aApitInstance->ThreadpoolWait;
        LeaveCriticalSection(&aApitInstance->CallbackLock);

        if (tpWait)
        {
            // Start waiting for next notification
            SetThreadpoolWait(tpWait, aApitInstance->Overlapped.hEvent, nullptr);
            
#ifdef DEBUG_ASYNC_IO
            LogVerbose(API_DEFAULT, "Querying for next notification");
#endif

            // Request next notification
            if (!DeviceIoControl(
                    aApitInstance->DeviceHandle,
                    IOCTL_OTLWF_QUERY_NOTIFICATION,
                    nullptr, 0,
                    &aApitInstance->NotificationBuffer, sizeof(OTLWF_NOTIFICATION),
                    nullptr, 
                    &aApitInstance->Overlapped))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_IO_PENDING)
                {
                    LogError(API_DEFAULT, "DeviceIoControl for new notification failed, %!WINERROR!", dwError);
                }
            }
        }
    }
    
#ifdef DEBUG_ASYNC_IO
    LogFuncExit(API_DEFAULT);
#endif
}

DWORD
SendIOCTL(
    _In_ otApiInstance *aApitInstance,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD nInBufferSize,
    _Out_writes_bytes_opt_(nOutBufferSize) LPVOID lpOutBuffer,
    _In_ DWORD nOutBufferSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    OVERLAPPED Overlapped = { 0 };
    DWORD dwBytesReturned = 0;
    
    Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (Overlapped.hEvent == nullptr)
    {
        dwError = GetLastError();
        LogError(API_DEFAULT, "CreateEvent (Overlapped.hEvent) failed, %!WINERROR!", dwError);
        goto error;
    }
    
    // Send the IOCTL the OpenThread driver
    if (!DeviceIoControl(
            aApitInstance->DeviceHandle,
            dwIoControlCode,
            lpInBuffer, nInBufferSize,
            lpOutBuffer, nOutBufferSize,
            nullptr, 
            &Overlapped))
    {
        dwError = GetLastError();
        if (dwError != ERROR_IO_PENDING)
        {
            LogError(API_DEFAULT, "DeviceIoControl(0x%x) failed, %!WINERROR!", dwIoControlCode, dwError);
            goto error;
        }
        dwError = ERROR_SUCCESS;
    }

    // Get the result of the IO operation
    if (!GetOverlappedResultEx(
            aApitInstance->DeviceHandle,
            &Overlapped,
            &dwBytesReturned,
            c_MaxOverlappedWaitTimeMS,
            FALSE
            ))
    {
        dwError = GetLastError();
        if (dwError == WAIT_TIMEOUT)
        {
            dwError = ERROR_TIMEOUT;
            CancelIoEx(aApitInstance->DeviceHandle, &Overlapped);
        }
        LogError(API_DEFAULT, "GetOverlappedResult failed, %!WINERROR!", dwError);
        goto error;
    }

    if (dwBytesReturned != nOutBufferSize)
    {
        dwError = ERROR_INVALID_DATA;
        LogError(API_DEFAULT, "GetOverlappedResult returned invalid output size, expected=%u actual=%u", 
                     nOutBufferSize, dwBytesReturned);
        goto error;
    }

error:

    if (Overlapped.hEvent)
    {
        CloseHandle(Overlapped.hEvent);
    }

    return dwError;
}

__pragma(pack(push,1))
template <class T1, class T2>
struct PackedBuffer2
{
    T1 data1; T2 data2;
    PackedBuffer2(const T1 &d1, const T2 &d2) : data1(d1), data2(d2) { }
};
template <class T1, class T2, class T3>
struct PackedBuffer3
{
    T1 data1; T2 data2; T3 data3;
    PackedBuffer3(const T1 &d1, const T2 &d2, const T3 &d3) : data1(d1), data2(d2), data3(d3) { }
};
template <class T1, class T2, class T3, class T4>
struct PackedBuffer4
{
    T1 data1; T2 data2; T3 data3; T4 data4;
    PackedBuffer4(const T1 &d1, const T2 &d2, const T3 &d3, const T4 &d4) : data1(d1), data2(d2), data3(d3), data4(d4) { }
};
template <class T1, class T2, class T3, class T4, class T5>
struct PackedBuffer5
{
    T1 data1; T2 data2; T3 data3; T4 data4; T5 data5;
    PackedBuffer5(const T1 &d1, const T2 &d2, const T3 &d3, const T4 &d4, const T5 &d5) : data1(d1), data2(d2), data3(d3), data4(d4), data5(d5) { }
};
template <class T1, class T2, class T3, class T4, class T5, class T6>
struct PackedBuffer6
{
    T1 data1; T2 data2; T3 data3; T4 data4; T5 data5; T6 data6;
    PackedBuffer6(const T1 &d1, const T2 &d2, const T3 &d3, const T4 &d4, const T5 &d5, const T6 &d6) : data1(d1), data2(d2), data3(d3), data4(d4), data5(d5), data6(d6) { }
};
 __pragma(pack(pop))

template <class in, class out>
DWORD
QueryIOCTL(
    _In_ otInstance *aInstance,
    _In_ DWORD dwIoControlCode,
    _In_ const in *input,
    _Out_ out* output
    )
{
    PackedBuffer2<GUID,in> Buffer(aInstance->InterfaceGuid, *input);
    return SendIOCTL(aInstance->ApiHandle, dwIoControlCode, &Buffer, sizeof(Buffer), output, sizeof(out));
}

template <class out>
DWORD
QueryIOCTL(
    _In_ otInstance *aInstance,
    _In_ DWORD dwIoControlCode,
    _Out_ out* output
    )
{
    return SendIOCTL(aInstance->ApiHandle, dwIoControlCode, &aInstance->InterfaceGuid, sizeof(GUID), output, sizeof(out));
}

template <class in>
DWORD
SetIOCTL(
    _In_ otInstance *aInstance,
    _In_ DWORD dwIoControlCode,
    _In_ const in* input
    )
{
    PackedBuffer2<GUID,in> Buffer(aInstance->InterfaceGuid, *input);
    return SendIOCTL(aInstance->ApiHandle, dwIoControlCode, &Buffer, sizeof(Buffer), nullptr, 0);
}

template <class in>
DWORD
SetIOCTL(
    _In_ otInstance *aInstance,
    _In_ DWORD dwIoControlCode,
    _In_ const in input
    )
{
    PackedBuffer2<GUID,in> Buffer(aInstance->InterfaceGuid, input);
    return SendIOCTL(aInstance->ApiHandle, dwIoControlCode, &Buffer, sizeof(Buffer), nullptr, 0);
}

DWORD
SetIOCTL(
    _In_ otInstance *aInstance,
    _In_ DWORD dwIoControlCode
    )
{
    return SendIOCTL(aInstance->ApiHandle, dwIoControlCode, &aInstance->InterfaceGuid, sizeof(GUID), nullptr, 0);
}

otError
DwordToThreadError(
    DWORD dwError
    )
{
    if (((int)dwError) > 0)
    {
        return OT_ERROR_GENERIC;
    }
    else
    {
        return (otError)(-(int)dwError);
    }
}

OTAPI 
void 
OTCALL
otSetDeviceAvailabilityChangedCallback(
    _In_ otApiInstance *aApitInstance,
    _In_ otDeviceAvailabilityChangedCallback aCallback,
    _In_ void *aCallbackContext
    )
{
    otApiDeviceAvailabilityCallback* CallbackToRelease = nullptr;

    EnterCriticalSection(&aApitInstance->CallbackLock);

    if (aApitInstance->DeviceAvailabilityCallbacks != nullptr)
    {
        CallbackToRelease = aApitInstance->DeviceAvailabilityCallbacks;
        aApitInstance->DeviceAvailabilityCallbacks = nullptr;
    }

    if (aCallback != nullptr)
    {
        aApitInstance->DeviceAvailabilityCallbacks = 
            new otApiDeviceAvailabilityCallback(aCallback, aCallbackContext);
    }
    
    LeaveCriticalSection(&aApitInstance->CallbackLock);

    if (CallbackToRelease)
    {
        CallbackToRelease->Release(true);
        delete CallbackToRelease;
    }
}

OTAPI 
otDeviceList* 
OTCALL
otEnumerateDevices(
    _In_ otApiInstance *aApitInstance
    )
{
    DWORD dwError = ERROR_SUCCESS;
    OVERLAPPED Overlapped = { 0 };
    DWORD dwBytesReturned = 0;
    otDeviceList* pDeviceList = nullptr;
    DWORD cbDeviceList = sizeof(otDeviceList);
    
    LogFuncEntry(API_DEFAULT);

    Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (Overlapped.hEvent == nullptr)
    {
        dwError = GetLastError();
        LogError(API_DEFAULT, "CreateEvent (Overlapped.hEvent) failed, %!WINERROR!", dwError);
        goto error;
    }
    
    pDeviceList = (otDeviceList*)malloc(cbDeviceList);
    if (pDeviceList == nullptr)
    {
        LogWarning(API_DEFAULT, "Failed to allocate otDeviceList of %u bytes.", cbDeviceList);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    RtlZeroMemory(pDeviceList, cbDeviceList);
    
    // Query in a loop to account for it changing between calls
    while (true)
    {
        // Send the IOCTL to query the interfaces
        if (!DeviceIoControl(
                aApitInstance->DeviceHandle,
                IOCTL_OTLWF_ENUMERATE_DEVICES,
                nullptr, 0,
                pDeviceList, cbDeviceList,
                nullptr, 
                &Overlapped))
        {
            dwError = GetLastError();
            if (dwError != ERROR_IO_PENDING)
            {
                LogError(API_DEFAULT, "DeviceIoControl(IOCTL_OTLWF_ENUMERATE_DEVICES) failed, %!WINERROR!", dwError);
                goto error;
            }
            dwError = ERROR_SUCCESS;
        }

        // Get the result of the IO operation
        if (!GetOverlappedResultEx(
                aApitInstance->DeviceHandle,
                &Overlapped,
                &dwBytesReturned,
                c_MaxOverlappedWaitTimeMS,
                TRUE))
        {
            dwError = GetLastError();
            if (dwError == WAIT_TIMEOUT)
            {
                dwError = ERROR_TIMEOUT;
                CancelIoEx(aApitInstance->DeviceHandle, &Overlapped);
            }
            LogError(API_DEFAULT, "GetOverlappedResult for notification failed, %!WINERROR!", dwError);
            goto error;
        }
        
        // Calculate the expected size of the full buffer
        cbDeviceList = 
            FIELD_OFFSET(otDeviceList, aDevices) +
            pDeviceList->aDevicesLength * sizeof(otDeviceList::aDevices);
        
        // Make sure they returned a complete buffer
        if (dwBytesReturned != sizeof(otDeviceList::aDevicesLength)) break;
        
        // If we get here that means we didn't have a big enough buffer
        // Reallocate a new buffer
        free(pDeviceList);
        pDeviceList = (otDeviceList*)malloc(cbDeviceList);
        if (pDeviceList == nullptr)
        {
            LogError(API_DEFAULT, "Failed to allocate otDeviceList of %u bytes.", cbDeviceList);
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }
        RtlZeroMemory(pDeviceList, cbDeviceList);
    }

error:

    if (dwError != ERROR_SUCCESS)
    {
        free(pDeviceList);
        pDeviceList = nullptr;
    }

    if (Overlapped.hEvent)
    {
        CloseHandle(Overlapped.hEvent);
    }
    
    LogFuncExitMsg(API_DEFAULT, "%d devices", pDeviceList == nullptr ? -1 : (int)pDeviceList->aDevicesLength);

    return pDeviceList;
}
    
OTAPI 
otInstance *
OTCALL
otInstanceInit(
    _In_ otApiInstance *aApitInstance, 
    _In_ const GUID *aDeviceGuid
    )
{
    otInstance *aInstance = nullptr;

    OTLWF_DEVICE Result = {0};
    if (aApitInstance &&
        SendIOCTL(
            aApitInstance, 
            IOCTL_OTLWF_QUERY_DEVICE, 
            (LPVOID)aDeviceGuid, 
            sizeof(GUID), 
            &Result, 
            sizeof(Result)
            ) == ERROR_SUCCESS)
    {
        aInstance = (otInstance*)malloc(sizeof(otInstance));
        if (aInstance)
        {
            aInstance->ApiHandle = aApitInstance;
            aInstance->InterfaceGuid = *aDeviceGuid;
            aInstance->CompartmentID = Result.CompartmentID;

            if (ConvertInterfaceGuidToLuid(aDeviceGuid, &aInstance->InterfaceLuid) != ERROR_SUCCESS ||
                ConvertInterfaceLuidToIndex(&aInstance->InterfaceLuid, &aInstance->InterfaceIndex) != ERROR_SUCCESS)
            {
                LogError(API_DEFAULT, "Failed to convert interface guid to index!");
                free(aInstance);
                aInstance = nullptr;
            }
        }
    }

    return aInstance;
}

OTAPI 
GUID 
OTCALL
otGetDeviceGuid(
    otInstance *aInstance
    )
{
    if (aInstance == nullptr) return {};
    return aInstance->InterfaceGuid;
}

OTAPI 
uint32_t 
OTCALL
otGetDeviceIfIndex(
    otInstance *aInstance
    )
{
    if (aInstance == nullptr) return (uint32_t)-1;
    return aInstance->InterfaceIndex;
}

OTAPI 
uint32_t 
OTCALL
otGetCompartmentId(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return (uint32_t)-1;
    return aInstance->CompartmentID;
}

OTAPI 
const char *
OTCALL
otGetVersionString()
{
    char* szVersion = (char*)malloc(sizeof(c_Version));
    if (szVersion)
    {
        memcpy_s(szVersion, sizeof(c_Version), c_Version, sizeof(c_Version));
    }
    return szVersion;
}

OTAPI 
otError 
OTCALL
otIp6SetEnabled(
    _In_ otInstance *aInstance,
    bool aEnabled
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_INTERFACE, (BOOLEAN)aEnabled));
}

OTAPI 
bool 
OTCALL
otIp6IsEnabled(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_INTERFACE, &Result);
    return Result != FALSE;
}

OTAPI 
otError 
OTCALL
otThreadSetEnabled(
    _In_ otInstance *aInstance,
    bool aEnabled
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_THREAD, (BOOLEAN)aEnabled));
}

OTAPI
otError
OTCALL
otThreadSetAutoStart(
    _In_ otInstance *aInstance,
    bool aStartAutomatically
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_THREAD_AUTO_START, (BOOLEAN)(aStartAutomatically ? TRUE : FALSE)));
}

OTAPI
bool
OTCALL
otThreadGetAutoStart(
    otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_THREAD_AUTO_START, &Result);
    return Result != FALSE;
}

OTAPI 
bool 
OTCALL
otThreadIsSingleton(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_SINGLETON, &Result);
    return Result != FALSE;
}

OTAPI 
otError 
OTCALL
otLinkActiveScan(
    _In_ otInstance *aInstance, 
    uint32_t aScanChannels, 
    uint16_t aScanDuration,
    otHandleActiveScanResult aCallback,
    _In_ void *aCallbackContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->ActiveScanCallbacks,
        aInstance->InterfaceGuid, aCallback, aCallbackContext
        );
    
    PackedBuffer3<GUID,uint32_t,uint16_t> Buffer(aInstance->InterfaceGuid, aScanChannels, aScanDuration);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_ACTIVE_SCAN, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
bool 
OTCALL
otLinkIsActiveScanInProgress(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ACTIVE_SCAN, &Result);
    return Result != FALSE;
}

OTAPI 
otError 
OTCALL 
otLinkEnergyScan(
    _In_ otInstance *aInstance, 
    uint32_t aScanChannels, 
    uint16_t aScanDuration,
    _In_ otHandleEnergyScanResult aCallback, 
    _In_ void *aCallbackContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->EnergyScanCallbacks,
        aInstance->InterfaceGuid, aCallback, aCallbackContext
        );
    
    PackedBuffer3<GUID,uint32_t,uint16_t> Buffer(aInstance->InterfaceGuid, aScanChannels, aScanDuration);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_ENERGY_SCAN, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
bool 
OTCALL 
otLinkIsEnergyScanInProgress(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ENERGY_SCAN, &Result);
    return Result != FALSE;
}

OTAPI 
otError 
OTCALL
otThreadDiscover(
    _In_ otInstance *aInstance, 
    uint32_t aScanChannels, 
    uint16_t aPanid,
    bool aJoiner,
    bool aEnableEui64Filtering,
    otHandleActiveScanResult aCallback,
    void *aCallbackContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->DiscoverCallbacks,
        aInstance->InterfaceGuid, aCallback, aCallbackContext
        );

    PackedBuffer5<GUID,uint32_t,uint16_t, uint8_t, uint8_t> Buffer(aInstance->InterfaceGuid, aScanChannels, aPanid, aJoiner, aEnableEui64Filtering);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_DISCOVER, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
bool 
OTCALL
otIsDiscoverInProgress(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = FALSE;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_DISCOVER, &Result);
    return Result != FALSE;
}

OTAPI 
otError
OTCALL
otLinkSendDataRequest(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    UNREFERENCED_PARAMETER(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED; // TODO
}

OTAPI 
uint8_t 
OTCALL
otLinkGetChannel(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_CHANNEL, &Result);
    return Result;
}

OTAPI 
otError 
OTCALL
otLinkSetChannel(
    _In_ otInstance *aInstance, 
    uint8_t aChannel
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_CHANNEL, aChannel));
}

OTAPI
otError
OTCALL
otDatasetSetDelayTimerMinimal(
    _In_ otInstance *aInstance,
    uint32_t aDelayTimerMinimal
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    // TODO
    UNREFERENCED_PARAMETER(aDelayTimerMinimal);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OTAPI
uint32_t
OTCALL
otDatasetGetDelayTimerMinimal(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return 0;
    // TODO
    return 0;
}

OTAPI 
uint8_t 
OTCALL
otThreadGetMaxAllowedChildren(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MAX_CHILDREN, &Result);
    return Result;
}

OTAPI 
otError 
OTCALL
otThreadSetMaxAllowedChildren(
    _In_ otInstance *aInstance, 
    uint8_t aMaxChildren
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MAX_CHILDREN, aMaxChildren));
}

OTAPI 
uint32_t 
OTCALL
otThreadGetChildTimeout(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_CHILD_TIMEOUT, &Result);
    return Result;
}

OTAPI 
void 
OTCALL
otThreadSetChildTimeout(
    _In_ otInstance *aInstance, 
    uint32_t aTimeout
    )
{
    if (aInstance == nullptr) return;
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_CHILD_TIMEOUT, aTimeout);
}

OTAPI 
const 
otExtAddress *
OTCALL
otLinkGetExtendedAddress(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otExtAddress *Result = (otExtAddress*)malloc(sizeof(otExtAddress));
    if (Result && QueryIOCTL(aInstance, IOCTL_OTLWF_OT_EXTENDED_ADDRESS, Result) != ERROR_SUCCESS)
    {
        free(Result);
        Result = nullptr;
    }
    return Result;
}

OTAPI 
otError 
OTCALL
otLinkSetExtendedAddress(
    _In_ otInstance *aInstance, 
    const otExtAddress *aExtAddress
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_EXTENDED_ADDRESS, aExtAddress));
}

OTAPI 
const otExtendedPanId *
OTCALL
otThreadGetExtendedPanId(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otExtendedPanId *Result = (otExtendedPanId*)malloc(sizeof(otExtendedPanId));
    if (Result == nullptr) return nullptr;
    if (QueryIOCTL(aInstance, IOCTL_OTLWF_OT_EXTENDED_PANID, Result) != ERROR_SUCCESS)
    {
        free(Result);
        Result = nullptr;
    }
    return Result;
}

OTAPI 
otError 
OTCALL
otThreadSetExtendedPanId(
    _In_ otInstance *aInstance, 
    const otExtendedPanId *aExtendedPanId
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_EXTENDED_PANID, aExtendedPanId));
}

OTAPI 
void 
OTCALL
otLinkGetFactoryAssignedIeeeEui64(
    _In_ otInstance *aInstance, 
    _Out_ otExtAddress *aEui64
)
{
    if (aInstance == nullptr) return;
    (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_FACTORY_EUI64, aEui64);
}

OTAPI 
otError 
OTCALL
otThreadGetLeaderRloc(
    _In_ otInstance *aInstance, 
    _Out_ otIp6Address *aLeaderRloc
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LEADER_RLOC, aLeaderRloc));
}

OTAPI 
otLinkModeConfig 
OTCALL
otThreadGetLinkMode(
    _In_ otInstance *aInstance
    )
{
    otLinkModeConfig Result = {0};
    static_assert(sizeof(otLinkModeConfig) == 4, "The size of otLinkModeConfig should be 4 bytes");
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LINK_MODE, &Result);
    return Result;
}

OTAPI 
otError 
OTCALL
otThreadSetLinkMode(
    _In_ otInstance *aInstance, 
    otLinkModeConfig aConfig
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    static_assert(sizeof(otLinkModeConfig) == 4, "The size of otLinkModeConfig should be 4 bytes");
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_LINK_MODE, aConfig));
}

OTAPI 
const otMasterKey *
OTCALL
otThreadGetMasterKey(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otMasterKey *Result = (otMasterKey*)malloc(sizeof(otMasterKey));
    if (Result == nullptr) return nullptr;
    if (QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MASTER_KEY, Result) != ERROR_SUCCESS)
    {
        free(Result);
        return nullptr;
    }
    return Result;
}

OTAPI
otError
OTCALL
otThreadSetMasterKey(
    _In_ otInstance *aInstance, 
    const otMasterKey *aKey
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MASTER_KEY, aKey));
}

OTAPI 
const uint8_t *
OTCALL
otThreadGetPSKc(
    _In_ otInstance *aInstance 
    )
{
    if (aInstance == nullptr) return nullptr;

    uint8_t *Result = (uint8_t*)malloc(sizeof(otPSKc));
    if (Result == nullptr) return nullptr;
    if (QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PSKC, Result) != ERROR_SUCCESS)
    {
        free(Result);
        return nullptr;
    }
    return Result;
}

OTAPI
otError
OTCALL
otThreadSetPSKc(
    _In_ otInstance *aInstance, 
    const uint8_t *aPSKc 
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    
    BYTE Buffer[sizeof(GUID) + sizeof(otPSKc)];
    memcpy_s(Buffer, sizeof(Buffer), &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), sizeof(Buffer) - sizeof(GUID), aPSKc, sizeof(otPSKc));
    
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_PSKC, Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI
const otIp6Address *
OTCALL
otThreadGetMeshLocalEid(
    _In_ otInstance *aInstance
    )
{
    otIp6Address *Result = (otIp6Address*)malloc(sizeof(otIp6Address));
    if (Result && QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MESH_LOCAL_EID, Result) != ERROR_SUCCESS)
    {
        free(Result);
        Result = nullptr;
    }
    return Result;
}

OTAPI
const otMeshLocalPrefix *
OTCALL
otThreadGetMeshLocalPrefix(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otMeshLocalPrefix *Result = (otMeshLocalPrefix*)malloc(sizeof(otMeshLocalPrefix));
    if (Result && QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX, Result) != ERROR_SUCCESS)
    {
        free(Result);
        Result = nullptr;
    }
    return Result;
}

OTAPI
otError
OTCALL
otThreadSetMeshLocalPrefix(
    _In_ otInstance *aInstance, 
    const otMeshLocalPrefix *aMeshLocalPrefix
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX, aMeshLocalPrefix));
}

OTAPI
otError
OTCALL
otThreadGetNetworkDataLeader(
    _In_ otInstance *aInstance, 
    bool aStable, 
    _Out_ uint8_t *aData, 
    _Out_ uint8_t *aDataLength
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    UNREFERENCED_PARAMETER(aInstance);
    UNREFERENCED_PARAMETER(aStable);
    UNREFERENCED_PARAMETER(aData);
    UNREFERENCED_PARAMETER(aDataLength);
    return OT_ERROR_NOT_IMPLEMENTED; // TODO
}

OTAPI
otError
OTCALL
otThreadGetNetworkDataLocal(
    _In_ otInstance *aInstance, 
    bool aStable, 
    _Out_ uint8_t *aData, 
    _Out_ uint8_t *aDataLength
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    UNREFERENCED_PARAMETER(aInstance);
    UNREFERENCED_PARAMETER(aStable);
    UNREFERENCED_PARAMETER(aData);
    UNREFERENCED_PARAMETER(aDataLength);
    return OT_ERROR_NOT_IMPLEMENTED; // TODO
}

OTAPI
const char *
OTCALL
otThreadGetNetworkName(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otNetworkName *Result = (otNetworkName*)malloc(sizeof(otNetworkName));
    if (Result && QueryIOCTL(aInstance, IOCTL_OTLWF_OT_NETWORK_NAME, Result) != ERROR_SUCCESS)
    {
        free(Result);
        Result = nullptr;
    }
    return (char*)Result;
}

OTAPI
otError
OTCALL
otThreadSetNetworkName(
    _In_ otInstance *aInstance, 
    _In_ const char *aNetworkName
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    otNetworkName Buffer = {0};
    strcpy_s(Buffer.m8, sizeof(Buffer), aNetworkName);
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_NETWORK_NAME, (const otNetworkName*)&Buffer));
}

OTAPI 
otError 
OTCALL
otNetDataGetNextOnMeshPrefix(
    _In_ otInstance *aInstance, 
    _Inout_ otNetworkDataIterator *aIterator,
    _Out_ otBorderRouterConfig *aConfig
    )
{
    if (aInstance == nullptr || aConfig == nullptr) return OT_ERROR_INVALID_ARGS;
    
    BOOLEAN aLocal = FALSE;
    PackedBuffer3<GUID,BOOLEAN,otNetworkDataIterator> InBuffer(aInstance->InterfaceGuid, aLocal, *aIterator);
    BYTE OutBuffer[sizeof(uint8_t) + sizeof(otBorderRouterConfig)];

    otError aError =
        DwordToThreadError(
            SendIOCTL(
                aInstance->ApiHandle,
                IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX,
                &InBuffer, sizeof(InBuffer),
                OutBuffer, sizeof(OutBuffer)));

    if (aError == OT_ERROR_NONE)
    {
        memcpy(aIterator, OutBuffer, sizeof(uint8_t));
        memcpy(aConfig, OutBuffer + sizeof(uint8_t), sizeof(otBorderRouterConfig));
    }
    else
    {
        ZeroMemory(aConfig, sizeof(otBorderRouterConfig));
    }

    return aError;
}

OTAPI
otError
OTCALL
otBorderRouterGetNextOnMeshPrefix(
    _In_ otInstance *aInstance,
    _Inout_ otNetworkDataIterator *aIterator,
    _Out_ otBorderRouterConfig *aConfig
    )
{
    if (aInstance == nullptr || aConfig == nullptr) return OT_ERROR_INVALID_ARGS;

    BOOLEAN aLocal = TRUE;
    PackedBuffer3<GUID,BOOLEAN,otNetworkDataIterator> InBuffer(aInstance->InterfaceGuid, aLocal, *aIterator);
    BYTE OutBuffer[sizeof(uint8_t) + sizeof(otBorderRouterConfig)];

    otError aError = 
        DwordToThreadError(
            SendIOCTL(
                aInstance->ApiHandle, 
                IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX, 
                &InBuffer, sizeof(InBuffer), 
                OutBuffer, sizeof(OutBuffer)));

    if (aError == OT_ERROR_NONE)
    {
        memcpy(aIterator, OutBuffer, sizeof(uint8_t));
        memcpy(aConfig, OutBuffer + sizeof(uint8_t), sizeof(otBorderRouterConfig));
    }
    else
    {
        ZeroMemory(aConfig, sizeof(otBorderRouterConfig));
    }

    return aError;
}

OTAPI
otError
OTCALL
otBorderRouterGetNextRoute(
    _In_ otInstance *aInstance,
    _Inout_ otNetworkDataIterator *aIterator,
    _Out_ otExternalRouteConfig *aConfig
    )
{
    if (aInstance == nullptr || aConfig == nullptr) return OT_ERROR_INVALID_ARGS;

    BOOLEAN aLocal = TRUE;
    PackedBuffer3<GUID,BOOLEAN,otNetworkDataIterator> InBuffer(aInstance->InterfaceGuid, aLocal, *aIterator);
    BYTE OutBuffer[sizeof(uint8_t) + sizeof(otExternalRouteConfig)];

    otError aError =
        DwordToThreadError(
            SendIOCTL(
                aInstance->ApiHandle,
                IOCTL_OTLWF_OT_NEXT_ROUTE,
                &InBuffer, sizeof(InBuffer),
                OutBuffer, sizeof(OutBuffer)));

    if (aError == OT_ERROR_NONE)
    {
        memcpy(aIterator, OutBuffer, sizeof(uint8_t));
        memcpy(aConfig, OutBuffer + sizeof(uint8_t), sizeof(otExternalRouteConfig));
    }
    else
    {
        ZeroMemory(aConfig, sizeof(otExternalRouteConfig));
    }

    return aError;
}

OTAPI
otPanId 
OTCALL
otLinkGetPanId(
    _In_ otInstance *aInstance
    )
{
    otPanId Result = {0};
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PAN_ID, &Result);
    return Result;
}

OTAPI
otError
OTCALL
otLinkSetPanId(
    _In_ otInstance *aInstance, 
    otPanId aPanId
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_PAN_ID, aPanId));
}

OTAPI
bool 
OTCALL
otThreadIsRouterRoleEnabled(
    _In_ otInstance *aInstance
    )
{
    BOOLEAN Result = {0};
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED, &Result);
    return Result != FALSE;
}

OTAPI
void 
OTCALL
otThreadSetRouterRoleEnabled(
    _In_ otInstance *aInstance, 
    bool aEnabled
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED, (BOOLEAN)aEnabled);
}

OTAPI
otError
OTCALL
otThreadSetPreferredRouterId(
    _In_ otInstance *aInstance,
    uint8_t aRouterId
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_PAN_ID, aRouterId));
}

OTAPI
otShortAddress 
OTCALL
otLinkGetShortAddress(
    _In_ otInstance *aInstance
    )
{
    otShortAddress Result = {0};
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_SHORT_ADDRESS, &Result);
    return Result;
}

BOOL
GetAdapterAddresses(
    PIP_ADAPTER_ADDRESSES * ppIAA
)
{
    PIP_ADAPTER_ADDRESSES pIAA = NULL;
    DWORD len = 0;
    DWORD flags;

    flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    if (GetAdaptersAddresses(AF_INET6, flags, NULL, NULL, &len) != ERROR_BUFFER_OVERFLOW)
        return FALSE;

    pIAA = (PIP_ADAPTER_ADDRESSES)malloc(len);
    if (pIAA) {
        GetAdaptersAddresses(AF_INET6, flags, NULL, pIAA, &len);
        *ppIAA = pIAA;
        return TRUE;
    }
    return FALSE;
}

OTAPI
const otNetifAddress *
OTCALL
otIp6GetUnicastAddresses(
    _In_ otInstance *aInstance
    )
{
    LogFuncEntry(API_DEFAULT);
    if (aInstance == nullptr)
    {
        LogFuncExit(API_DEFAULT);
        return nullptr;
    }

    // Put the current thead in the correct compartment
    bool RevertCompartmentOnExit = false;
    ULONG OriginalCompartmentID = GetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != aInstance->CompartmentID)
    {
        DWORD dwError = ERROR_SUCCESS;
        if ((dwError = SetCurrentThreadCompartmentId(aInstance->CompartmentID)) != ERROR_SUCCESS)
        {
            LogError(API_DEFAULT, "SetCurrentThreadCompartmentId failed, %!WINERROR!", dwError);
            return nullptr;
        }
        RevertCompartmentOnExit = true;
    }

    otNetifAddress *addrs = nullptr;
    ULONG AddrCount = 0;

    // Query the current adapter addresses and format them in the proper output format
    PIP_ADAPTER_ADDRESSES pIAAList;
    if (GetAdapterAddresses(&pIAAList))
    {
        // Loop through all the interfaces
        for (auto pIAA = pIAAList; pIAA != nullptr; pIAA = pIAA->Next) 
        {
            // Look for the right interface
            if (pIAA->Ipv6IfIndex != aInstance->InterfaceIndex) continue;

            // Look through all unicast addresses
            for (auto pUnicastAddr = pIAA->FirstUnicastAddress; 
                 pUnicastAddr != nullptr; 
                 pUnicastAddr = pUnicastAddr->Next)
            {
                AddrCount++;
            }

            break;
        }

        // If we didn't find any addresses, just break out
        if (AddrCount == 0) goto error;

        // Allocate the addresses
        addrs = (otNetifAddress*)malloc(AddrCount * sizeof(otNetifAddress));
        if (addrs == nullptr)
        {
            LogWarning(API_DEFAULT, "Not enough memory to alloc otNetifAddress array");
            goto error;
        }
        ZeroMemory(addrs, AddrCount * sizeof(otNetifAddress));

        // Initialize the next pointers
        for (ULONG i = 0; i < AddrCount; i++)
        {
            addrs[i].mNext = (i + 1 == AddrCount) ? nullptr : &addrs[i + 1];
        }

        AddrCount = 0;

        // Loop through all the interfaces
        for (auto pIAA = pIAAList; pIAA != nullptr; pIAA = pIAA->Next) 
        {
            // Look for the right interface
            if (pIAA->Ipv6IfIndex != aInstance->InterfaceIndex) continue;

            // Look through all unicast addresses
            for (auto pUnicastAddr = pIAA->FirstUnicastAddress; 
                 pUnicastAddr != nullptr; 
                 pUnicastAddr = pUnicastAddr->Next)
            {
                LPSOCKADDR_IN6 pAddr = (LPSOCKADDR_IN6)pUnicastAddr->Address.lpSockaddr;

                // Copy the necessary parameters
                memcpy(&addrs[AddrCount].mAddress, &pAddr->sin6_addr, sizeof(pAddr->sin6_addr));
                addrs[AddrCount].mPreferred = pUnicastAddr->PreferredLifetime != 0;
                addrs[AddrCount].mValid = pUnicastAddr->ValidLifetime != 0;
                addrs[AddrCount].mPrefixLength = pUnicastAddr->OnLinkPrefixLength;

                AddrCount++;
            }

            break;
        }

    error:
        free(pIAAList);
    }
    else
    {
        LogError(API_DEFAULT, "GetAdapterAddresses failed!");
    }

    // Revert the comparment if necessary
    if (RevertCompartmentOnExit)
    {
        (VOID)SetCurrentThreadCompartmentId(OriginalCompartmentID);
    }
    
    LogFuncExitMsg(API_DEFAULT, "%d addrs", AddrCount);
    return addrs;
}

OTAPI
otError
OTCALL
otIp6AddUnicastAddress(
    _In_ otInstance *aInstance, 
    const otNetifAddress *aAddress
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    // Put the current thead in the correct compartment
    bool RevertCompartmentOnExit = false;
    ULONG OriginalCompartmentID = GetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != aInstance->CompartmentID)
    {
        DWORD dwError = ERROR_SUCCESS;
        if ((dwError = SetCurrentThreadCompartmentId(aInstance->CompartmentID)) != ERROR_SUCCESS)
        {
            LogError(API_DEFAULT, "SetCurrentThreadCompartmentId failed, %!WINERROR!", dwError);
            return OT_ERROR_FAILED;
        }
        RevertCompartmentOnExit = true;
    }

    MIB_UNICASTIPADDRESS_ROW newRow;
    InitializeUnicastIpAddressEntry(&newRow);

    newRow.InterfaceIndex = aInstance->InterfaceIndex;
    newRow.InterfaceLuid = aInstance->InterfaceLuid;
    newRow.Address.si_family = AF_INET6;
    newRow.Address.Ipv6.sin6_family = AF_INET6;
        
    static_assert(sizeof(IN6_ADDR) == sizeof(otIp6Address), "Windows and OpenThread IPv6 Addr Structs must be same size");

    memcpy(&newRow.Address.Ipv6.sin6_addr, &aAddress->mAddress, sizeof(IN6_ADDR));
    newRow.OnLinkPrefixLength = aAddress->mPrefixLength;
    newRow.PreferredLifetime = aAddress->mPreferred ? 0xffffffff : 0;
    newRow.ValidLifetime = aAddress->mValid ? 0xffffffff : 0;
    newRow.PrefixOrigin = IpPrefixOriginOther;  // Derived from network XPANID
    newRow.SkipAsSource = FALSE;                // Allow automatic binding to this address (default)

    if (IN6_IS_ADDR_LINKLOCAL(&newRow.Address.Ipv6.sin6_addr))
    {
        newRow.SuffixOrigin = IpSuffixOriginLinkLayerAddress;   // Derived from Extended MAC address
    }
    else
    {
        newRow.SuffixOrigin = IpSuffixOriginRandom;             // Was created randomly
    }

    DWORD dwError = CreateUnicastIpAddressEntry(&newRow);

    // Revert the comparment if necessary
    if (RevertCompartmentOnExit)
    {
        (VOID)SetCurrentThreadCompartmentId(OriginalCompartmentID);
    }

    if (dwError != ERROR_SUCCESS)
    {
        LogError(API_DEFAULT, "CreateUnicastIpAddressEntry failed %!WINERROR!", dwError);
        return OT_ERROR_FAILED;
    }

    return OT_ERROR_NONE;
}

OTAPI
otError
OTCALL
otIp6RemoveUnicastAddress(
    _In_ otInstance *aInstance, 
    const otIp6Address *aAddress
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    // Put the current thead in the correct compartment
    bool RevertCompartmentOnExit = false;
    ULONG OriginalCompartmentID = GetCurrentThreadCompartmentId();
    if (OriginalCompartmentID != aInstance->CompartmentID)
    {
        DWORD dwError = ERROR_SUCCESS;
        if ((dwError = SetCurrentThreadCompartmentId(aInstance->CompartmentID)) != ERROR_SUCCESS)
        {
            LogError(API_DEFAULT, "SetCurrentThreadCompartmentId failed, %!WINERROR!", dwError);
            return OT_ERROR_FAILED;
        }
        RevertCompartmentOnExit = true;
    }

    MIB_UNICASTIPADDRESS_ROW deleteRow;
    InitializeUnicastIpAddressEntry(&deleteRow);

    deleteRow.InterfaceIndex = aInstance->InterfaceIndex;
    deleteRow.InterfaceLuid = aInstance->InterfaceLuid;
    deleteRow.Address.si_family = AF_INET6;

    memcpy(&deleteRow.Address.Ipv6.sin6_addr, aAddress, sizeof(IN6_ADDR));
    
    DWORD dwError = DeleteUnicastIpAddressEntry(&deleteRow);

    // Revert the comparment if necessary
    if (RevertCompartmentOnExit)
    {
        (VOID)SetCurrentThreadCompartmentId(OriginalCompartmentID);
    }

    if (dwError != ERROR_SUCCESS)
    {
        LogError(API_DEFAULT, "DeleteUnicastIpAddressEntry failed %!WINERROR!", dwError);
        return OT_ERROR_FAILED;
    }

    return OT_ERROR_NONE;
}

OTAPI
otError 
OTCALL
otSetStateChangedCallback(
    _In_ otInstance *aInstance, 
    _In_ otStateChangedCallback aCallback, 
    _In_ void *aContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    bool success = 
        aInstance->ApiHandle->SetCallback(
            aInstance->ApiHandle->StateChangedCallbacks,
            aInstance->InterfaceGuid, aCallback, aContext
            );
    return success ? OT_ERROR_NONE : OT_ERROR_ALREADY;
}

OTAPI
void
OTCALL
otRemoveStateChangeCallback(
    _In_ otInstance *aInstance,
    _In_ otStateChangedCallback /* aCallback */,
    _In_ void *aContext
    )
{
    if (aInstance == nullptr) return;
    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->StateChangedCallbacks,
        aInstance->InterfaceGuid, (otStateChangedCallback)nullptr, aContext
        );
}

OTAPI
otError
OTCALL
otDatasetGetActive(
    _In_ otInstance *aInstance, 
    _Out_ otOperationalDataset *aDataset
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ACTIVE_DATASET, aDataset));
}

OTAPI
otError
OTCALL
otDatasetSetActive(
    _In_ otInstance *aInstance, 
    const otOperationalDataset *aDataset
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_ACTIVE_DATASET, aDataset));
}

OTAPI
otError
OTCALL
otDatasetGetPending(
    _In_ otInstance *aInstance, 
    _Out_ otOperationalDataset *aDataset
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PENDING_DATASET, aDataset));
}

OTAPI
otError
OTCALL
otDatasetSetPending(
    _In_ otInstance *aInstance, 
    const otOperationalDataset *aDataset
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_PENDING_DATASET, aDataset));
}

OTAPI 
otError 
OTCALL
otDatasetSendMgmtActiveGet(
    _In_ otInstance *aInstance,
    const otOperationalDatasetComponents *aDatasetComponents,
    const uint8_t *aTlvTypes, 
    uint8_t aLength,
    _In_opt_ const otIp6Address *aAddress
    )
{
    if (aInstance == nullptr || aDatasetComponents == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvTypes == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(uint8_t) + aLength;
    if (aAddress) BufferSize += sizeof(otIp6Address);
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDatasetComponents, sizeof(otOperationalDatasetComponents));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(uint8_t), aTlvTypes, aLength);
    if (aAddress)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t) + aLength, BufferSize - sizeof(GUID) - sizeof(uint8_t) - aLength, aAddress, sizeof(otIp6Address));
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_ACTIVE_GET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL
otDatasetSendMgmtActiveSet(
    _In_ otInstance *aInstance, 
    const otOperationalDataset *aDataset,
    const uint8_t *aTlvs,
    uint8_t aLength
    )
{
    if (aInstance == nullptr || aDataset == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvs == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(otOperationalDataset) + sizeof(uint8_t) + aLength;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDataset, sizeof(otOperationalDataset));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otOperationalDataset), BufferSize - sizeof(GUID) - sizeof(otOperationalDataset), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(otOperationalDataset) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(otOperationalDataset) - sizeof(uint8_t), aTlvs, aLength);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_ACTIVE_SET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL
otDatasetSendMgmtPendingGet(
    _In_ otInstance *aInstance,
    const otOperationalDatasetComponents *aDatasetComponents,
    const uint8_t *aTlvTypes, 
    uint8_t aLength,
    _In_opt_ const otIp6Address *aAddress
    )
{
    if (aInstance == nullptr || aDatasetComponents == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvTypes == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(uint8_t) + aLength;
    if (aAddress) BufferSize += sizeof(otIp6Address);
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDatasetComponents, sizeof(otOperationalDatasetComponents));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(uint8_t), aTlvTypes, aLength);
    if (aAddress)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t) + aLength, BufferSize - sizeof(GUID) - sizeof(uint8_t) - aLength, aAddress, sizeof(otIp6Address));
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_PENDING_GET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL
otDatasetSendMgmtPendingSet(
    _In_ otInstance *aInstance, 
    const otOperationalDataset *aDataset, 
    const uint8_t *aTlvs,
    uint8_t aLength
    )
{
    if (aInstance == nullptr || aDataset == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvs == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(otOperationalDataset) + sizeof(uint8_t) + aLength;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDataset, sizeof(otOperationalDataset));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otOperationalDataset), BufferSize - sizeof(GUID) - sizeof(otOperationalDataset), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(otOperationalDataset) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(otOperationalDataset) - sizeof(uint8_t), aTlvs, aLength);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_PENDING_SET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
uint32_t 
OTCALL
otLinkGetPollPeriod(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_POLL_PERIOD, &Result);
    return Result;
}

OTAPI
otError
OTCALL
otLinkSetPollPeriod(
    _In_ otInstance *aInstance,
    uint32_t aPollPeriod
    )
{
    otError result = OT_ERROR_INVALID_ARGS;

    if (aInstance) result = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_POLL_PERIOD, aPollPeriod));

    return result;
}

OTAPI
uint8_t 
OTCALL
otThreadGetLocalLeaderWeight(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetLocalLeaderWeight(
    _In_ otInstance *aInstance, 
    uint8_t aWeight
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT, aWeight);
}

OTAPI 
uint32_t 
OTCALL
otThreadGetLocalLeaderPartitionId(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID, &Result);
    return Result;
}

OTAPI 
void 
OTCALL
otThreadSetLocalLeaderPartitionId(
    _In_ otInstance *aInstance, 
    uint32_t aPartitionId
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID, aPartitionId);
}

OTAPI 
uint16_t 
OTCALL 
otThreadGetJoinerUdpPort(
    _In_ otInstance *aInstance
)
{
    uint16_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_JOINER_UDP_PORT, &Result);
    return Result;
}

OTAPI 
otError 
OTCALL 
otThreadSetJoinerUdpPort(
    _In_ otInstance *aInstance, 
    uint16_t aJoinerUdpPort
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_JOINER_UDP_PORT, aJoinerUdpPort));
}

OTAPI
otError
OTCALL
otBorderRouterAddOnMeshPrefix(
    _In_ otInstance *aInstance, 
    const otBorderRouterConfig *aConfig
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_ADD_BORDER_ROUTER, aConfig));
}

OTAPI
otError
OTCALL
otBorderRouterRemoveOnMeshPrefix(
    _In_ otInstance *aInstance, 
    const otIp6Prefix *aPrefix
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_REMOVE_BORDER_ROUTER, aPrefix));
}

OTAPI
otError
OTCALL
otBorderRouterAddRoute(
    _In_ otInstance *aInstance, 
    const otExternalRouteConfig *aConfig
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_ADD_EXTERNAL_ROUTE, aConfig));
}

OTAPI
otError
OTCALL
otBorderRouterRemoveRoute(
    _In_ otInstance *aInstance, 
    const otIp6Prefix *aPrefix
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_REMOVE_EXTERNAL_ROUTE, aPrefix));
}

OTAPI
otError
OTCALL
otBorderRouterRegister(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_SEND_SERVER_DATA));
}

OTAPI
uint32_t 
OTCALL
otThreadGetContextIdReuseDelay(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetContextIdReuseDelay(
    _In_ otInstance *aInstance, 
    uint32_t aDelay
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY, aDelay);
}

OTAPI
uint32_t 
OTCALL
otThreadGetKeySequenceCounter(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetKeySequenceCounter(
    _In_ otInstance *aInstance, 
    uint32_t aKeySequenceCounter
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER, aKeySequenceCounter);
}

OTAPI
uint32_t 
OTCALL
otThreadGetKeySwitchGuardTime(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetKeySwitchGuardTime(
    _In_ otInstance *aInstance, 
    uint32_t aKeySwitchGuardTime
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME, aKeySwitchGuardTime);
}

OTAPI
uint8_t
OTCALL
otThreadGetNetworkIdTimeout(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetNetworkIdTimeout(
    _In_ otInstance *aInstance, 
    uint8_t aTimeout
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT, aTimeout);
}

OTAPI
uint8_t 
OTCALL
otThreadGetRouterUpgradeThreshold(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD, &Result);
    return Result;
}

OTAPI
void 
OTCALL
otThreadSetRouterUpgradeThreshold(
    _In_ otInstance *aInstance, 
    uint8_t aThreshold
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD, aThreshold);
}

OTAPI 
uint8_t 
OTCALL
otThreadGetRouterDowngradeThreshold(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD, &Result);
    return Result;
}

OTAPI 
void 
OTCALL
otThreadSetRouterDowngradeThreshold(
    _In_ otInstance *aInstance, 
    uint8_t aThreshold
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD, aThreshold);
}

OTAPI 
uint8_t 
OTCALL
otThreadGetRouterSelectionJitter(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER, &Result);
    return Result;
}

OTAPI 
void 
OTCALL
otThreadSetRouterSelectionJitter(
    _In_ otInstance *aInstance, 
    uint8_t aRouterJitter
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER, aRouterJitter);
}

OTAPI
otError
OTCALL
otThreadReleaseRouterId(
    _In_ otInstance *aInstance, 
    uint8_t aRouterId
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_RELEASE_ROUTER_ID, aRouterId));
}

OTAPI
otError
OTCALL
otThreadBecomeDetached(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_DEVICE_ROLE, (uint8_t)OT_DEVICE_ROLE_DETACHED));
}

OTAPI
otError
OTCALL
otThreadBecomeChild(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_DEVICE_ROLE, (uint8_t)OT_DEVICE_ROLE_CHILD));
}

OTAPI
otError
OTCALL
otThreadBecomeRouter(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_DEVICE_ROLE, (uint8_t)OT_DEVICE_ROLE_ROUTER));
}

OTAPI
otError
OTCALL
otThreadBecomeLeader(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_DEVICE_ROLE, (uint8_t)OT_DEVICE_ROLE_LEADER));
}

OTAPI
otMacFilterAddressMode
OTCALL
otLinkFilterGetAddressMode(
    _In_ otInstance *aInstance
    )
{
    otMacFilterAddressMode mode = OT_MAC_FILTER_ADDRESS_MODE_DISABLED;
    BOOLEAN enabled = 0;

    if (aInstance)
    {
        (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED, &enabled);

        if (enabled)
        {
            mode = OT_MAC_FILTER_ADDRESS_MODE_WHITELIST;
        }
        else
        {
            (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED, &enabled);

            if (enabled)
            {
                mode = OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST;
            }
        }
    }

    return mode;
}

OTAPI
otError
OTCALL
otLinkFilterSetAddressMode(
    _In_ otInstance *aInstance,
    otMacFilterAddressMode aMode
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        bool enabled = false;

        if (aMode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST)
        {
            enabled = true;
            error = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED, (BOOLEAN)enabled));
        }
        else if (aMode == OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST)
        {
            enabled = true;
            error = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED, (BOOLEAN)enabled));
        }
        else
        {
            error = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED, (BOOLEAN)enabled));
        }
    }
    return error;
}

OTAPI
otError
OTCALL
otLinkFilterAddAddress(
    _In_ otInstance *aInstance,
    const otExtAddress *aExtAddr
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr || aExtAddr == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        // same effect to add an address to the address filter with IOCTL_OTLWF_OT_ADD_MAC_WHITELIST or
        // IOCTL_OTLWF_OT_ADD_MAC_BLACKLIST.
        error = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_ADD_MAC_WHITELIST, aExtAddr));
    }

    return error;
}


OTAPI
otError
OTCALL
otLinkFilterRemoveAddress(
    _In_ otInstance *aInstance,
    const otExtAddress *aExtAddr
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr || aExtAddr == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        // same effect to remove an address from the address filter with IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST or
        // IOCTL_OTLWF_OT_REMOVE_MAC_BLACKLIST.
        error = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST, aExtAddr));
    }

    return error;
}

OTAPI
void
OTCALL
otLinkFilterClearAddresses(
    _In_ otInstance *aInstance
    )
{
    if (aInstance)
    {
        // same effect to clear the addresses from the address filter with IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST or
        // IOCTL_OTLWF_OT_CLEAR_MAC_BLACKLIST.
        (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST);
    }
}

OTAPI
otError
OTCALL
otLinkFilterGetNextAddress(
    _In_ otInstance *aInstance,
    _Inout_ otMacFilterIterator *aIterator,
    _Out_ otMacFilterEntry *aEntry
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr || aIterator == nullptr|| aEntry == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        // same effect to get an in-use address filter entry with IOCTL_OTLWF_OT_NEXT_MAC_WHITELIST or
        // IOCTL_OTLWF_OT_NEXT_MAC_BLACKLIST.
        return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_NEXT_MAC_WHITELIST, aIterator, aEntry));
    }
    return OT_ERROR_NOT_FOUND;
}


OTAPI
otError
OTCALL
otLinkFilterAddRssIn(
    _In_ otInstance *aInstance,
    const otExtAddress *aExtAddr,
    int8_t aRss
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        if (aExtAddr == nullptr)
        {
            PackedBuffer2<GUID,int8_t> Buffer(aInstance->InterfaceGuid, aRss);

            error = DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_ADD_MAC_FIXED_RSS,
                        &Buffer, sizeof(Buffer), nullptr, 0));
        }
        else
        {
            PackedBuffer3<GUID,otExtAddress,int8_t> Buffer(aInstance->InterfaceGuid, *aExtAddr, aRss);

            error = DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_ADD_MAC_FIXED_RSS,
                        &Buffer, sizeof(Buffer), nullptr, 0));
        }

    }
    return error;
}

OTAPI
otError
OTCALL
otLinkFilterRemoveRssIn(
    _In_ otInstance *aInstance,
    const otExtAddress *aExtAddr
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        if (aExtAddr == nullptr)
        {
            error = DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_REMOVE_MAC_FIXED_RSS,
                        nullptr, 0, nullptr, 0));
        }
        else
        {
            PackedBuffer2<GUID,otExtAddress> Buffer(aInstance->InterfaceGuid, *aExtAddr);
            error = DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_REMOVE_MAC_FIXED_RSS,
                        &Buffer, sizeof(Buffer), nullptr, 0));
        }
    }
    return error;
}

OTAPI
void
OTCALL
otLinkFilterClearRssIn(
    _In_ otInstance *aInstance
    )
{
    if (aInstance)
    {
        (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_CLEAR_MAC_FIXED_RSS);
    }
}

OTAPI
otError
OTCALL
otLinkFilterGetNextRssIn(
    _In_ otInstance *aInstance,
    _Inout_ otMacFilterIterator *aIterator,
    _Out_ otMacFilterEntry *aEntry
    )
{
    otError error = OT_ERROR_NONE;

    if (aInstance == nullptr || aIterator == nullptr|| aEntry == nullptr)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_NEXT_MAC_FIXED_RSS, aIterator, aEntry));
    }
    return OT_ERROR_NOT_FOUND;
}

OTAPI 
void 
OTCALL
otInstanceReset(
    _In_ otInstance *aInstance
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_PLATFORM_RESET);
}

OTAPI 
void 
OTCALL
otInstanceFactoryReset(
    _In_ otInstance *aInstance
    )
{
    if (aInstance) (void)SetIOCTL(aInstance, IOCTL_OTLWF_OT_FACTORY_RESET);
}

OTAPI
otError
OTCALL
otThreadGetChildInfoById(
    _In_ otInstance *aInstance, 
    uint16_t aChildId, 
    _Out_ otChildInfo *aChildInfo
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_CHILD_INFO_BY_ID, &aChildId, aChildInfo));
}

OTAPI
otError
OTCALL
otThreadGetChildInfoByIndex(
    _In_ otInstance *aInstance, 
    uint8_t aChildIndex, 
    _Out_ otChildInfo *aChildInfo
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_CHILD_INFO_BY_INDEX, &aChildIndex, aChildInfo));
}

OTAPI
otError
OTCALL
otThreadGetNextNeighborInfo(
    _In_ otInstance *aInstance,
    _Inout_ otNeighborInfoIterator *aIterator,
    _Out_ otNeighborInfo *aInfo
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    UNREFERENCED_PARAMETER(aIterator);
    UNREFERENCED_PARAMETER(aInfo);
    return OT_ERROR_NOT_IMPLEMENTED; // TODO
}

OTAPI
otDeviceRole 
OTCALL
otThreadGetDeviceRole(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = OT_DEVICE_ROLE_DISABLED;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_DEVICE_ROLE, &Result);
    return (otDeviceRole)Result;
}

OTAPI
otError
OTCALL
otThreadGetEidCacheEntry(
    _In_ otInstance *aInstance, 
    uint8_t aIndex, 
    _Out_ otEidCacheEntry *aEntry
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_EID_CACHE_ENTRY, &aIndex, aEntry));
}

OTAPI
otError
OTCALL
otThreadGetLeaderData(
    _In_ otInstance *aInstance, 
    _Out_ otLeaderData *aLeaderData
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LEADER_DATA, aLeaderData));
}

OTAPI
uint8_t 
OTCALL
otThreadGetLeaderRouterId(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LEADER_ROUTER_ID, &Result);
    return Result;
}

OTAPI
uint8_t 
OTCALL
otThreadGetLeaderWeight(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_LEADER_WEIGHT, &Result);
    return Result;
}

OTAPI
uint8_t 
OTCALL
otNetDataGetVersion(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_NETWORK_DATA_VERSION, &Result);
    return Result;
}

OTAPI
uint32_t 
OTCALL
otThreadGetPartitionId(
    _In_ otInstance *aInstance
    )
{
    uint32_t Result = 0xFFFFFFFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PARTITION_ID, &Result);
    return Result;
}

OTAPI
uint16_t 
OTCALL
otThreadGetRloc16(
    _In_ otInstance *aInstance
    )
{
    uint16_t Result = 0xFFFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_RLOC16, &Result);
    return Result;
}

OTAPI
uint8_t 
OTCALL
otThreadGetRouterIdSequence(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_ID_SEQUENCE, &Result);
    return Result;
}

OTAPI
uint8_t
OTCALL
otThreadGetMaxRouterId(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MAX_ROUTER_ID, &Result);
    return Result;
}

OTAPI
otError
OTCALL
otThreadGetRouterInfo(
    _In_ otInstance *aInstance, 
    uint16_t aRouterId, 
    _Out_ otRouterInfo *aRouterInfo
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_ROUTER_INFO, &aRouterId, aRouterInfo));
}

OTAPI 
otError 
OTCALL
otThreadGetParentInfo(
    _In_ otInstance *aInstance, 
    _Out_ otRouterInfo *aParentInfo
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    static_assert(sizeof(otRouterInfo) == 20, "The size of otRouterInfo should be 20 bytes");
    return DwordToThreadError(QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PARENT_INFO, aParentInfo));
}

OTAPI
uint8_t 
OTCALL
otNetDataGetStableVersion(
    _In_ otInstance *aInstance
    )
{
    uint8_t Result = 0xFF;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_STABLE_NETWORK_DATA_VERSION, &Result);
    return Result;
}

OTAPI
const otMacCounters*
OTCALL
otLinkGetCounters(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return nullptr;

    otMacCounters* aCounters = (otMacCounters*)malloc(sizeof(otMacCounters));
    if (aCounters)
    {
        if (ERROR_SUCCESS != QueryIOCTL(aInstance, IOCTL_OTLWF_OT_MAC_COUNTERS, aCounters))
        {
            free(aCounters);
            aCounters = nullptr;
        }
    }
    return aCounters;
}

OTAPI
void
OTCALL
otMessageGetBufferInfo(
    _In_ otInstance *,
    _Out_ otBufferInfo *aBufferInfo
    )
{
    // Not supported on Windows
    ZeroMemory(aBufferInfo, sizeof(otBufferInfo));
}

OTAPI
bool 
OTCALL
otIsIp6AddressEqual(
    const otIp6Address *a, 
    const otIp6Address *b
    )
{
    return memcmp(a->mFields.m8, b->mFields.m8, sizeof(otIp6Address)) == 0;
}

OTAPI
otError 
OTCALL
otIp6AddressFromString(
    const char *str, 
    otIp6Address *address
    )
{
    otError error = OT_ERROR_NONE;
    uint8_t *dst = reinterpret_cast<uint8_t *>(address->mFields.m8);
    uint8_t *endp = reinterpret_cast<uint8_t *>(address->mFields.m8 + 15);
    uint8_t *colonp = NULL;
    uint16_t val = 0;
    uint8_t count = 0;
    bool first = true;
    char ch;
    uint8_t d;

    memset(address->mFields.m8, 0, 16);

    dst--;

    for (;;)
    {
        ch = *str++;
        d = ch & 0xf;

        if (('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
        {
            d += 9;
        }
        else if (ch == ':' || ch == '\0' || ch == ' ')
        {
            if (count)
            {
                if (dst + 2 > endp)
                {
                    error = OT_ERROR_PARSE;
                    goto exit;
                }
                *(dst + 1) = static_cast<uint8_t>(val >> 8);
                *(dst + 2) = static_cast<uint8_t>(val);
                dst += 2;
                count = 0;
                val = 0;
            }
            else if (ch == ':')
            {
                if (!(colonp == nullptr || first))
                {
                    error = OT_ERROR_PARSE;
                    goto exit;
                }
                colonp = dst;
            }

            if (ch == '\0' || ch == ' ')
            {
                break;
            }

            continue;
        }
        else
        {
            if (!('0' <= ch && ch <= '9'))
            {
                error = OT_ERROR_PARSE;
                goto exit;
            }
        }

        first = false;
        val = static_cast<uint16_t>((val << 4) | d);
        if (!(++count <= 4))
        {
            error = OT_ERROR_PARSE;
            goto exit;
        }
    }

    while (colonp && dst > colonp)
    {
        *endp-- = *dst--;
    }

    while (endp > dst)
    {
        *endp-- = 0;
    }

exit:
    return error;
}

OTAPI 
uint8_t 
OTCALL
otIp6PrefixMatch(
    const otIp6Address *aFirst, 
    const otIp6Address *aSecond
    )
{
    uint8_t rval = 0;
    uint8_t diff;

    for (uint8_t i = 0; i < sizeof(otIp6Address); i++)
    {
        diff = aFirst->mFields.m8[i] ^ aSecond->mFields.m8[i];

        if (diff == 0)
        {
            rval += 8;
        }
        else
        {
            while ((diff & 0x80) == 0)
            {
                rval++;
                diff <<= 1;
            }

            break;
        }
    }

    return rval;
}

OTAPI
const char *
OTCALL
otThreadErrorToString(
    otError aError
    )
{
    const char *retval;

    switch (aError)
    {
    case OT_ERROR_NONE:
        retval = "OK";
        break;

    case OT_ERROR_FAILED:
        retval = "Failed";
        break;

    case OT_ERROR_DROP:
        retval = "Drop";
        break;

    case OT_ERROR_NO_BUFS:
        retval = "NoBufs";
        break;

    case OT_ERROR_NO_ROUTE:
        retval = "NoRoute";
        break;

    case OT_ERROR_BUSY:
        retval = "Busy";
        break;

    case OT_ERROR_PARSE:
        retval = "Parse";
        break;

    case OT_ERROR_INVALID_ARGS:
        retval = "InvalidArgs";
        break;

    case OT_ERROR_SECURITY:
        retval = "Security";
        break;

    case OT_ERROR_ADDRESS_QUERY:
        retval = "AddressQuery";
        break;

    case OT_ERROR_NO_ADDRESS:
        retval = "NoAddress";
        break;

    case OT_ERROR_ABORT:
        retval = "Abort";
        break;

    case OT_ERROR_NOT_IMPLEMENTED:
        retval = "NotImplemented";
        break;

    case OT_ERROR_INVALID_STATE:
        retval = "InvalidState";
        break;

    case OT_ERROR_NO_ACK:
        retval = "NoAck";
        break;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        retval = "ChannelAccessFailure";
        break;

    case OT_ERROR_DETACHED:
        retval = "Detached";
        break;

    case OT_ERROR_FCS:
        retval = "FcsErr";
        break;

    case OT_ERROR_NO_FRAME_RECEIVED:
        retval = "NoFrameReceived";
        break;

    case OT_ERROR_UNKNOWN_NEIGHBOR:
        retval = "UnknownNeighbor";
        break;

    case OT_ERROR_INVALID_SOURCE_ADDRESS:
        retval = "InvalidSourceAddress";
        break;

    case OT_ERROR_ADDRESS_FILTERED:
        retval = "AddressFiltered";
        break;

    case OT_ERROR_DESTINATION_ADDRESS_FILTERED:
        retval = "DestinationAddressFiltered";
        break;

    case OT_ERROR_NOT_FOUND:
        retval = "NotFound";
        break;

    case OT_ERROR_ALREADY:
        retval = "Already";
        break;

    case OT_ERROR_IP6_ADDRESS_CREATION_FAILURE:
        retval = "Ipv6AddressCreationFailure";
        break;

    case OT_ERROR_NOT_CAPABLE:
        retval = "NotCapable";
        break;

    case OT_ERROR_RESPONSE_TIMEOUT:
        retval = "ResponseTimeout";
        break;

    case OT_ERROR_DUPLICATED:
        retval = "Duplicated";
        break;

    case OT_ERROR_GENERIC:
        retval = "GenericError";
        break;

    default:
        retval = "UnknownErrorType";
        break;
    }

    return retval;
}

OTAPI 
otError 
OTCALL 
otThreadSendDiagnosticGet(
    _In_ otInstance *aInstance, 
    const otIp6Address *aDestination, 
    const uint8_t aTlvTypes[],
    uint8_t aCount
    )
{
    if (aInstance == nullptr || aDestination == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvTypes == nullptr && aCount != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(otIp6Address) + sizeof(uint8_t) + aCount;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDestination, sizeof(otIp6Address));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otIp6Address), BufferSize - sizeof(GUID) - sizeof(otIp6Address), &aCount, sizeof(aCount));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otIp6Address) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(otIp6Address) - sizeof(uint8_t), aTlvTypes, aCount);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_GET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL 
otThreadSendDiagnosticReset(
    _In_ otInstance *aInstance, 
    const otIp6Address *aDestination, 
    const uint8_t aTlvTypes[],
    uint8_t aCount
    )
{
    if (aInstance == nullptr || aDestination == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvTypes == nullptr && aCount != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(otIp6Address) + sizeof(uint8_t) + aCount;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDestination, sizeof(otIp6Address));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otIp6Address), BufferSize - sizeof(GUID) - sizeof(otIp6Address), &aCount, sizeof(aCount));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otIp6Address) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(otIp6Address) - sizeof(uint8_t), aTlvTypes, aCount);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_RESET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL
otCommissionerStart(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_COMMISIONER_START));
}

OTAPI 
otError 
OTCALL
otCommissionerAddJoiner(
    _In_ otInstance *aInstance, 
    const otExtAddress *aExtAddress, 
    const char *aPSKd,
    uint32_t aTimeout
    )
{
    if (aInstance == nullptr || aPSKd == nullptr) return OT_ERROR_INVALID_ARGS;

    size_t aPSKdLength = strnlen(aPSKd, OPENTHREAD_PSK_MAX_LENGTH + 1);
    if (aPSKdLength > OPENTHREAD_PSK_MAX_LENGTH)
    {
        return OT_ERROR_INVALID_ARGS;
    }

    uint8_t aExtAddressValid = aExtAddress ? 1 : 0;
    
    const ULONG BufferLength = sizeof(GUID) + sizeof(uint8_t) + sizeof(otExtAddress) + (ULONG)aPSKdLength + 1 + sizeof(aTimeout);
    BYTE Buffer[sizeof(GUID) + sizeof(uint8_t) + sizeof(otExtAddress) + OPENTHREAD_PSK_MAX_LENGTH + 1 + sizeof(aTimeout)] = {0};
    memcpy_s(Buffer, sizeof(Buffer), &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), sizeof(Buffer) - sizeof(GUID), &aExtAddressValid, sizeof(aExtAddressValid));
    if (aExtAddressValid)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t), sizeof(Buffer) - sizeof(GUID) - sizeof(uint8_t), aExtAddress, sizeof(otExtAddress));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t) + sizeof(otExtAddress), sizeof(Buffer) - sizeof(GUID) - sizeof(uint8_t) - sizeof(otExtAddress), aPSKd, aPSKdLength);
    memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t) + sizeof(otExtAddress) + aPSKdLength + 1, sizeof(Buffer) - sizeof(GUID) - sizeof(uint8_t) - sizeof(otExtAddress) - aPSKdLength - 1, &aTimeout, sizeof(aTimeout));
    
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISIONER_ADD_JOINER, Buffer, BufferLength, nullptr, 0));
}

OTAPI 
otError 
OTCALL
otCommissionerRemoveJoiner(
    _In_ otInstance *aInstance, 
    const otExtAddress *aExtAddress
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    uint8_t aExtAddressValid = aExtAddress ? 1 : 0;
    
    BYTE Buffer[sizeof(GUID) + sizeof(uint8_t) + sizeof(otExtAddress)] = {0};
    memcpy_s(Buffer, sizeof(Buffer), &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), sizeof(Buffer) - sizeof(GUID), &aExtAddressValid, sizeof(aExtAddressValid));
    if (aExtAddressValid)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t), sizeof(Buffer) - sizeof(GUID) - sizeof(uint8_t), aExtAddress, sizeof(otExtAddress));

    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISIONER_REMOVE_JOINER, Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
otError 
OTCALL
otCommissionerSetProvisioningUrl(
    _In_ otInstance *aInstance,
    const char *aProvisioningUrl
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    size_t aProvisioningUrlLength = aProvisioningUrl ? strnlen(aProvisioningUrl, OPENTHREAD_PROV_URL_MAX_LENGTH + 1) : 0;
    if (aProvisioningUrlLength > OPENTHREAD_PROV_URL_MAX_LENGTH)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    
    const ULONG BufferLength = sizeof(GUID) + (ULONG)aProvisioningUrlLength + 1;
    BYTE Buffer[sizeof(GUID) + OPENTHREAD_PROV_URL_MAX_LENGTH + 1] = {0};
    memcpy_s(Buffer, sizeof(Buffer), &aInstance->InterfaceGuid, sizeof(GUID));
    if (aProvisioningUrlLength > 0)
        memcpy_s(Buffer + sizeof(GUID), sizeof(Buffer) - sizeof(GUID), aProvisioningUrl, aProvisioningUrlLength);
    
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISIONER_PROVISIONING_URL, Buffer, BufferLength, nullptr, 0));
}

OTAPI
otError
OTCALL
otCommissionerAnnounceBegin(
    otInstance *aInstance,
    uint32_t aChannelMask,
    uint8_t aCount,
    uint16_t aPeriod,
    const otIp6Address *aAddress
    )
{
    if (aInstance == nullptr || aAddress == nullptr) return OT_ERROR_INVALID_ARGS;

    PackedBuffer5<GUID,uint32_t,uint8_t,uint16_t,otIp6Address> Buffer(aInstance->InterfaceGuid, aChannelMask, aCount, aPeriod, *aAddress);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISIONER_ANNOUNCE_BEGIN, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
otError 
OTCALL
otCommissionerStop(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_COMMISIONER_STOP));
}

OTAPI
otError 
OTCALL
otCommissionerEnergyScan(
    _In_ otInstance *aInstance, 
    uint32_t aChannelMask, 
    uint8_t aCount, 
    uint16_t aPeriod,
    uint16_t aScanDuration, 
    const otIp6Address *aAddress,
    _In_ otCommissionerEnergyReportCallback aCallback, 
    _In_ void *aContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->CommissionerEnergyReportCallbacks,
        aInstance->InterfaceGuid, aCallback, aContext
        );

    PackedBuffer6<GUID,uint32_t,uint8_t,uint16_t,uint16_t,otIp6Address> Buffer(aInstance->InterfaceGuid, aChannelMask, aCount, aPeriod, aScanDuration, *aAddress);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISSIONER_ENERGY_SCAN, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI
otError 
OTCALL
otCommissionerPanIdQuery(
    _In_ otInstance *aInstance, 
    uint16_t aPanId, 
    uint32_t aChannelMask,
    const otIp6Address *aAddress,
    _In_ otCommissionerPanIdConflictCallback aCallback, 
    _In_ void *aContext
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->CommissionerPanIdConflictCallbacks,
        aInstance->InterfaceGuid, aCallback, aContext
        );

    PackedBuffer4<GUID,uint16_t,uint32_t,otIp6Address> Buffer(aInstance->InterfaceGuid, aPanId, aChannelMask, *aAddress);
    return DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_COMMISSIONER_PANID_QUERY, &Buffer, sizeof(Buffer), nullptr, 0));
}

OTAPI 
otError 
OTCALL 
otCommissionerSendMgmtGet(
    _In_ otInstance *aInstance, 
    const uint8_t *aTlvs, 
    uint8_t aLength
)
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvs == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(uint8_t) + aLength;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(uint8_t), aTlvs, aLength);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_GET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI 
otError 
OTCALL 
otCommissionerSendMgmtSet(
    _In_ otInstance *aInstance,
    const otCommissioningDataset *aDataset,
    const uint8_t *aTlvs,
    uint8_t aLength
    )
{
    if (aInstance == nullptr || aDataset == nullptr) return OT_ERROR_INVALID_ARGS;
    if (aTlvs == nullptr && aLength != 0) return OT_ERROR_INVALID_ARGS;
    
    DWORD BufferSize = sizeof(GUID) + sizeof(otCommissioningDataset) + sizeof(uint8_t) + aLength;
    PBYTE Buffer = (PBYTE)malloc(BufferSize);
    if (Buffer == nullptr) return OT_ERROR_NO_BUFS;

    memcpy_s(Buffer, BufferSize, &aInstance->InterfaceGuid, sizeof(GUID));
    memcpy_s(Buffer + sizeof(GUID), BufferSize - sizeof(GUID), aDataset, sizeof(otCommissioningDataset));
    memcpy_s(Buffer + sizeof(GUID) + sizeof(otCommissioningDataset), BufferSize - sizeof(GUID) - sizeof(otCommissioningDataset), &aLength, sizeof(aLength));
    if (aLength > 0)
        memcpy_s(Buffer + sizeof(GUID) + sizeof(otCommissioningDataset) + sizeof(uint8_t), BufferSize - sizeof(GUID) - sizeof(otCommissioningDataset) - sizeof(uint8_t), aTlvs, aLength);
    
    otError result = 
        DwordToThreadError(SendIOCTL(aInstance->ApiHandle, IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_SET, Buffer, BufferSize, nullptr, 0));

    free(Buffer);
    return result;
}

OTAPI
uint16_t
OTCALL
otCommissionerGetSessionId(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return 0;
    // TODO
    return 0;
}

OTAPI 
otError 
OTCALL
otJoinerStart(
    _In_ otInstance *aInstance,
    _Null_terminated_ const char *aPSKd,
    _Null_terminated_ const char *aProvisioningUrl,
    _Null_terminated_ const char *aVendorName,
    _Null_terminated_ const char *aVendorModel,
    _Null_terminated_ const char *aVendorSwVersion,
    _Null_terminated_ const char *aVendorData,
    _In_ otJoinerCallback aCallback,
    _In_ void *aCallbackContext
    )
{
    if (aInstance == nullptr || aPSKd == nullptr) return OT_ERROR_INVALID_ARGS;

    otCommissionConfig config = {0};

    size_t aPSKdLength = strlen(aPSKd);
    size_t aProvisioningUrlLength = aProvisioningUrl == nullptr ? 0 : strlen(aProvisioningUrl);
    size_t aVendorNameLength = aVendorName == nullptr ? 0 : strlen(aVendorName);
    size_t aVendorModelLength = aVendorModel == nullptr ? 0 : strlen(aVendorModel);
    size_t aVendorSwVersionLength = aVendorSwVersion == nullptr ? 0 : strlen(aVendorSwVersion);
    size_t aVendorDataLength = aVendorData == nullptr ? 0 : strlen(aVendorData);

    if (aPSKdLength > OPENTHREAD_PSK_MAX_LENGTH ||
        aProvisioningUrlLength > OPENTHREAD_PROV_URL_MAX_LENGTH ||
        aVendorNameLength > OPENTHREAD_VENDOR_NAME_MAX_LENGTH ||
        aVendorModelLength > OPENTHREAD_VENDOR_MODEL_MAX_LENGTH ||
        aVendorSwVersionLength > OPENTHREAD_VENDOR_SW_VERSION_MAX_LENGTH ||
        aVendorDataLength > OPENTHREAD_VENDOR_DATA_MAX_LENGTH)
    {
        return OT_ERROR_INVALID_ARGS;
    }

    memcpy_s(config.PSKd, sizeof(config.PSKd), aPSKd, aPSKdLength);
    memcpy_s(config.ProvisioningUrl, sizeof(config.ProvisioningUrl), aProvisioningUrl, aProvisioningUrlLength);
    memcpy_s(config.VendorName, sizeof(config.VendorName), aVendorName, aVendorNameLength);
    memcpy_s(config.VendorModel, sizeof(config.VendorModel), aVendorModel, aVendorModelLength);
    memcpy_s(config.VendorSwVersion, sizeof(config.VendorSwVersion), aVendorSwVersion, aVendorSwVersionLength);
    memcpy_s(config.VendorData, sizeof(config.VendorData), aVendorData, aVendorDataLength);

    aInstance->ApiHandle->SetCallback(
        aInstance->ApiHandle->JoinerCallbacks,
        aInstance->InterfaceGuid, aCallback, aCallbackContext
        );

    auto ret = DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_JOINER_START, (const otCommissionConfig*)&config));

    if (ret != OT_ERROR_NONE)
    {
        aInstance->ApiHandle->SetCallback(
            aInstance->ApiHandle->JoinerCallbacks,
            aInstance->InterfaceGuid, (otJoinerCallback)nullptr, (PVOID)nullptr
            );
    }

    return ret;
}

OTAPI 
otError 
OTCALL
otJoinerStop(
    _In_ otInstance *aInstance
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_JOINER_STOP));
}

OTAPI 
otError 
OTCALL
otJoinerGetId(
    _In_ otInstance *aInstance, 
    _Out_ otExtAddress *aJoinerId
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    ZeroMemory(aJoinerId, sizeof(otExtAddress));
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_JOINER_ID, aJoinerId));
}

OTAPI
int8_t
OTCALL
otThreadGetParentPriority(
    _In_ otInstance *aInstance
)
{
    int8_t Result = 0;
    if (aInstance) (void)QueryIOCTL(aInstance, IOCTL_OTLWF_OT_PARENT_PRIORITY, &Result);
    return Result;
}

OTAPI
otError
OTCALL
otThreadSetParentPriority(
    _In_ otInstance *aInstance,
    int8_t aParentPriority
    )
{
    if (aInstance == nullptr) return OT_ERROR_INVALID_ARGS;
    return DwordToThreadError(SetIOCTL(aInstance, IOCTL_OTLWF_OT_PARENT_PRIORITY, aParentPriority));
}
