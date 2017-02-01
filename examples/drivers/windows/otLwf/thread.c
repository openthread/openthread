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

/**
 * @file
 * @brief
 *  This file implements the Thread mode (Radio Miniport) functions required for the OpenThread library.
 */

#include "precomp.h"
#include "thread.tmh"

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfInitializeThreadMode(
    _In_ PMS_FILTER pFilter
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    LogFuncEntry(DRIVER_DEFAULT);

    NT_ASSERT(pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO);

    do
    {
        KeInitializeEvent(
            &pFilter->SendNetBufferListComplete,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );

        // Initialize the event processing
        pFilter->EventWorkerThread = NULL;
        NdisAllocateSpinLock(&pFilter->EventsLock);
        InitializeListHead(&pFilter->AddressChangesHead);
        InitializeListHead(&pFilter->NBLsHead);
        InitializeListHead(&pFilter->MacFramesHead);
        InitializeListHead(&pFilter->EventIrpListHead);
        KeInitializeEvent(
            &pFilter->EventWorkerThreadStopEvent,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadWaitTimeUpdated,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessTasklets,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessAddressChanges,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessNBLs,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessMacFrames,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessIrp,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadEnergyScanComplete,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        pFilter->EventHighPrecisionTimer = 
            ExAllocateTimer(
                otLwfEventProcessingTimer, 
                pFilter, 
                EX_TIMER_HIGH_RESOLUTION
                );
        if (pFilter->EventHighPrecisionTimer == NULL)
        {
            LogError(DRIVER_DEFAULT, "Failed to allocate timer!");
            break;
        }

        // Query the interface state (best effort, since it might not be supported)
        BOOLEAN IfUp = FALSE;
        Status = otLwfCmdGetProp(pFilter, NULL, SPINEL_PROP_NET_IF_UP, SPINEL_DATATYPE_BOOL_S, &IfUp);
        if (!NT_SUCCESS(Status))
        {
            LogVerbose(DRIVER_DEFAULT, "Failed to query SPINEL_PROP_INTERFACE_TYPE, %!STATUS!", Status);
            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            NT_ASSERT(IfUp == FALSE);
        }

        // Initialize the event processing thread
        if (!NT_SUCCESS(otLwfEventProcessingStart(pFilter)))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

    } while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        // Stop event processing thread
        otLwfEventProcessingStop(pFilter);

        // Stop and free the timer
        if (pFilter->EventHighPrecisionTimer)
        {
            ExDeleteTimer(pFilter->EventHighPrecisionTimer, TRUE, FALSE, NULL);
        }
    }

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfUninitializeThreadMode(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    // Stop event processing thread
    otLwfEventProcessingStop(pFilter);
    
    // Free timer
    if (pFilter->EventHighPrecisionTimer)
    {
        ExDeleteTimer(pFilter->EventHighPrecisionTimer, TRUE, FALSE, NULL);
        pFilter->EventHighPrecisionTimer = NULL;
    }

    // Close handle to settings registry key
    if (pFilter->otSettingsRegKey)
    {
        ZwClose(pFilter->otSettingsRegKey);
        pFilter->otSettingsRegKey = NULL;
    }

    LogFuncExit(DRIVER_DEFAULT);
}

#if DEBUG_ALLOC
PMS_FILTER
otLwfFindFromCurrentThread()
{
    PMS_FILTER pOutput = NULL;
    HANDLE CurThreadId = PsGetCurrentThreadId();

    NdisAcquireSpinLock(&FilterListLock);

    for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
    {
        PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);

        if (pFilter->otThreadId == CurThreadId)
        {
            pOutput = pFilter;
            break;
        }
    }

    NdisReleaseSpinLock(&FilterListLock);

    NT_ASSERT(pOutput);
    return pOutput;
}
#endif

//
// OpenThread Platform functions
//

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    size_t totalSize = aNum * aSize;
#if DEBUG_ALLOC
    totalSize += sizeof(OT_ALLOC);
#endif
    PVOID mem = ExAllocatePoolWithTag(NonPagedPoolNx, totalSize, 'OTDM');
    if (mem)
    {
        RtlZeroMemory(mem, totalSize);
#if DEBUG_ALLOC
        PMS_FILTER pFilter = otLwfFindFromCurrentThread();
        //LogVerbose(DRIVER_DEFAULT, "otPlatAlloc(%u) = ID:%u %p", (ULONG)totalSize, pFilter->otAllocationID, mem);

        OT_ALLOC* AllocHeader = (OT_ALLOC*)mem;
        AllocHeader->Length = (LONG)totalSize;
        AllocHeader->ID = pFilter->otAllocationID++;
        InsertTailList(&pFilter->otOutStandingAllocations, &AllocHeader->Link);

        InterlockedIncrement(&pFilter->otOutstandingAllocationCount);
        InterlockedAdd(&pFilter->otOutstandingMemoryAllocated, AllocHeader->Length);
        
        mem = (PUCHAR)(mem) + sizeof(OT_ALLOC);
#endif
    }
    return mem;
}

void otPlatFree(void *aPtr)
{
    if (aPtr == NULL) return;
#if DEBUG_ALLOC
    aPtr = (PUCHAR)(aPtr) - sizeof(OT_ALLOC);
    //LogVerbose(DRIVER_DEFAULT, "otPlatFree(%p)", aPtr);
    OT_ALLOC* AllocHeader = (OT_ALLOC*)aPtr;

    PMS_FILTER pFilter = otLwfFindFromCurrentThread();
    InterlockedDecrement(&pFilter->otOutstandingAllocationCount);
    InterlockedAdd(&pFilter->otOutstandingMemoryAllocated, -AllocHeader->Length);
    RemoveEntryList(&AllocHeader->Link);
#endif
    ExFreePoolWithTag(aPtr, 'OTDM');
}

uint32_t otPlatRandomGet()
{
    LARGE_INTEGER Counter = KeQueryPerformanceCounter(NULL);
    return (uint32_t)RtlRandomEx(&Counter.LowPart);
}

ThreadError otPlatRandomSecureGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
{
    // Just use the system-preferred random number generator algorithm
    NTSTATUS status = 
        BCryptGenRandom(
            NULL, 
            aOutput, 
            (ULONG)aInputLength, 
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
            );
    NT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "BCryptGenRandom failed, %!STATUS!", status);
        return kThreadError_Failed;
    }

    *aOutputLength = aInputLength;

    return kThreadError_None;
}

void otSignalTaskletPending(_In_ otInstance *otCtx)
{
    LogVerbose(DRIVER_DEFAULT, "otSignalTaskletPending");
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    otLwfEventProcessingIndicateNewTasklet(pFilter);
}

// Process a role state change
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfProcessRoleStateChange(
    _In_ PMS_FILTER             pFilter
    )
{
    otDeviceRole prevRole = pFilter->otCachedRole;
    pFilter->otCachedRole = otGetDeviceRole(pFilter->otCtx);
    if (prevRole == pFilter->otCachedRole) return;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! new role: %!otDeviceRole!", &pFilter->InterfaceGuid, pFilter->otCachedRole);

    // Make sure we are in the correct media connect state
    otLwfIndicateLinkState(
        pFilter, 
        IsAttached(pFilter->otCachedRole) ? 
            MediaConnectStateConnected : 
            MediaConnectStateDisconnected);
}

void otLwfStateChangedCallback(uint32_t aFlags, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);

    //
    // Process the notification internally
    //

    if ((aFlags & OT_IP6_ADDRESS_ADDED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ADDRESS_ADDED", pFilter);
        otLwfRadioAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_IP6_ADDRESS_REMOVED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ADDRESS_REMOVED", pFilter);
        otLwfRadioAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_IP6_RLOC_ADDED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_RLOC_ADDED", pFilter);
        otLwfRadioAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_IP6_RLOC_REMOVED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_RLOC_REMOVED", pFilter);
        otLwfRadioAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_NET_ROLE) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_ROLE", pFilter);
        otLwfProcessRoleStateChange(pFilter);
    }

    if ((aFlags & OT_NET_PARTITION_ID) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_PARTITION_ID", pFilter);
    }

    if ((aFlags & OT_NET_KEY_SEQUENCE_COUNTER) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_KEY_SEQUENCE_COUNTER", pFilter);
    }

    if ((aFlags & OT_THREAD_CHILD_ADDED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_CHILD_ADDED", pFilter);
    }

    if ((aFlags & OT_THREAD_CHILD_REMOVED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_CHILD_REMOVED", pFilter);
    }

    if ((aFlags & OT_THREAD_NETDATA_UPDATED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_NETDATA_UPDATED", pFilter);
        otSlaacUpdate(pFilter->otCtx, pFilter->otAutoAddresses, ARRAYSIZE(pFilter->otAutoAddresses), otCreateRandomIid, NULL);

#if OPENTHREAD_ENABLE_DHCP6_SERVER
        otDhcp6ServerUpdate(pFilter->otCtx);
#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER

#if OPENTHREAD_ENABLE_DHCP6_CLIENT
        otDhcp6ClientUpdate(pFilter->otCtx, pFilter->otDhcpAddresses, ARRAYSIZE(pFilter->otDhcpAddresses), NULL);
#endif  // OPENTHREAD_ENABLE_DHCP6_CLIENT
    }

    if ((aFlags & OT_IP6_ML_ADDR_CHANGED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ML_ADDR_CHANGED", pFilter);
    }
    
    //
    // Queue the notification for clients
    //

    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_STATE_CHANGE;
        NotifEntry->Notif.StateChangePayload.Flags = aFlags;

        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfActiveScanCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_ACTIVE_SCAN;

        if (aResult)
        {
            NotifEntry->Notif.ActiveScanPayload.Valid = TRUE;
            NotifEntry->Notif.ActiveScanPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.ActiveScanPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfEnergyScanCallback(_In_ otEnergyScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_ENERGY_SCAN;

        if (aResult)
        {
            NotifEntry->Notif.EnergyScanPayload.Valid = TRUE;
            NotifEntry->Notif.EnergyScanPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.EnergyScanPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfDiscoverCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_DISCOVER;

        if (aResult)
        {
            NotifEntry->Notif.DiscoverPayload.Valid = TRUE;
            NotifEntry->Notif.DiscoverPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.DiscoverPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfCommissionerEnergyReportCallback(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength, void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);
    
    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_COMMISSIONER_ENERGY_REPORT;

        // Limit the number of reports if necessary
        if (aEnergyListLength > MAX_ENERGY_REPORT_LENGTH) aEnergyListLength = MAX_ENERGY_REPORT_LENGTH;
        
        NotifEntry->Notif.CommissionerEnergyReportPayload.ChannelMask = aChannelMask;
        NotifEntry->Notif.CommissionerEnergyReportPayload.EnergyListLength = aEnergyListLength;
        memcpy(NotifEntry->Notif.CommissionerEnergyReportPayload.EnergyList, aEnergyList, aEnergyListLength);
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfCommissionerPanIdConflictCallback(uint16_t aPanId, uint32_t aChannelMask, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);
    
    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_COMMISSIONER_PANID_QUERY;
        
        NotifEntry->Notif.CommissionerPanIdQueryPayload.PanId = aPanId;
        NotifEntry->Notif.CommissionerPanIdQueryPayload.ChannelMask = aChannelMask;
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfThreadValueIs(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    )
{
    LogFuncEntryMsg(DRIVER_DEFAULT, "[%p] received Value for %s", pFilter, spinel_prop_key_to_cstr(key));

    if (key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t status = SPINEL_STATUS_OK;
        spinel_datatype_unpack(value_data_ptr, value_data_len, "i", &status);

        if ((status >= SPINEL_STATUS_RESET__BEGIN) && (status <= SPINEL_STATUS_RESET__END))
        {
            LogInfo(DRIVER_DEFAULT, "Interface %!GUID! was reset (status %d).", &pFilter->InterfaceGuid, status);
            // TODO - Handle reset
        }
    }
    else if (key == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        uint8_t scanChannel;
        int8_t maxRssi;
        spinel_ssize_t ret;

        ret = spinel_datatype_unpack(
            value_data_ptr,
            value_data_len,
            "Cc",
            &scanChannel,
            &maxRssi);

        NT_ASSERT(ret > 0);
        if (ret > 0)
        {
            LogInfo(DRIVER_DEFAULT, "Filter: %p, completed energy scan: Rssi:%d", pFilter, maxRssi);
            otLwfEventProcessingIndicateEnergyScanResult(pFilter, maxRssi);
        }
    }
    else if (key == SPINEL_PROP_STREAM_RAW)
    {
        if (value_data_len < 256)
        {
            otLwfEventProcessingIndicateNewMacFrameCommand(
                pFilter,
                DispatchLevel,
                value_data_ptr,
                (uint8_t)value_data_len);
        }
    }
    else if (key == SPINEL_PROP_STREAM_DEBUG)
    {
        const uint8_t* output = NULL;
        UINT output_len = 0;
        spinel_ssize_t ret;

        ret = spinel_datatype_unpack(
            value_data_ptr,
            value_data_len,
            SPINEL_DATATYPE_DATA_S,
            &output,
            &output_len);

        NT_ASSERT(ret > 0);
        if (ret > 0 && output && output_len <= (UINT)ret)
        {
            if (strnlen((char*)output, output_len) != output_len)
            {
                LogInfo(DRIVER_DEFAULT, "DEVICE: %s", (char*)output);
            }
            else if (output_len < 128)
            {
                char strOutput[128] = { 0 };
                memcpy(strOutput, output, output_len);
                LogInfo(DRIVER_DEFAULT, "DEVICE: %s", strOutput);
            }
        }
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfThreadValueInserted(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    )
{
    LogFuncEntryMsg(DRIVER_DEFAULT, "[%p] received Value Inserted for %s", pFilter, spinel_prop_key_to_cstr(key));

    UNREFERENCED_PARAMETER(pFilter);
    UNREFERENCED_PARAMETER(DispatchLevel);
    UNREFERENCED_PARAMETER(key);
    UNREFERENCED_PARAMETER(value_data_ptr);
    UNREFERENCED_PARAMETER(value_data_len);

    LogFuncExit(DRIVER_DEFAULT);
}
