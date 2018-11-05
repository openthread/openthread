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
 *  This file implements the functions for creating new notifications for IOCTL clients.
 */

#include "precomp.h"
#include "eventprocessing.tmh"

typedef struct _OTLWF_ADDR_EVENT
{
    LIST_ENTRY              Link;
    MIB_NOTIFICATION_TYPE   NotificationType;
    IN6_ADDR                Address;

} OTLWF_ADDR_EVENT, *POTLWF_ADDR_EVENT;

typedef struct _OTLWF_NBL_EVENT
{
    LIST_ENTRY          Link;
    PNET_BUFFER_LIST    NetBufferLists;

} OTLWF_NBL_EVENT, *POTLWF_NBL_EVENT;

typedef struct _OTLWF_MAC_FRAME_EVENT
{
    LIST_ENTRY          Link;
    uint8_t             BufferLength;
    uint8_t             Buffer[0];

} OTLWF_MAC_FRAME_EVENT, *POTLWF_MAC_FRAME_EVENT;


KSTART_ROUTINE otLwfEventWorkerThread;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfCompleteNBLs(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ PNET_BUFFER_LIST       NetBufferLists,
    _In_ NTSTATUS               Status
    );

// Starts the event queue processing
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfEventProcessingStart(
    _In_ PMS_FILTER             pFilter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE   threadHandle = NULL;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p, TimeIncrement = %u", pFilter, KeQueryTimeIncrement());
    
    pFilter->NextAlarmTickCount.QuadPart = 0;

    NT_ASSERT(pFilter->EventWorkerThread == NULL);
    if (pFilter->EventWorkerThread != NULL)
    {
        status = STATUS_ALREADY_REGISTERED;
        goto error;
    }

    // Make sure to reset the necessary events
    KeResetEvent(&pFilter->EventWorkerThreadStopEvent);
    KeResetEvent(&pFilter->SendNetBufferListComplete);
    KeResetEvent(&pFilter->EventWorkerThreadEnergyScanComplete);

    // Start the worker thread
    status = PsCreateSystemThread(
                &threadHandle,                  // ThreadHandle
                THREAD_ALL_ACCESS,              // DesiredAccess
                NULL,                           // ObjectAttributes
                NULL,                           // ProcessHandle
                NULL,                           // ClientId
                otLwfEventWorkerThread,         // StartRoutine
                pFilter                         // StartContext
                );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "PsCreateSystemThread failed, %!STATUS!", status);
        goto error;
    }

    // Grab the object reference to the worker thread
    status = ObReferenceObjectByHandle(
                threadHandle,
                THREAD_ALL_ACCESS,
                *PsThreadType,
                KernelMode,
                &pFilter->EventWorkerThread,
                NULL
                );
    if (!NT_VERIFYMSG("ObReferenceObjectByHandle can't fail with a valid kernel handle", NT_SUCCESS(status)))
    {
        LogError(DRIVER_DEFAULT, "ObReferenceObjectByHandle failed, %!STATUS!", status);
        KeSetEvent(&pFilter->EventWorkerThreadStopEvent, IO_NO_INCREMENT, FALSE);
    }

    ZwClose(threadHandle);

error:

    if (!NT_SUCCESS(status))
    {
        ExSetTimerResolution(0, FALSE);
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

// Stops the event queue processing
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingStop(
    _In_ PMS_FILTER             pFilter
    )
{
    PLIST_ENTRY Link = NULL;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p", pFilter);

    // By this point, we have disabled the Data Path, so no more 
    // NBLs should be queued up.

    // Clean up worker thread
    if (pFilter->EventWorkerThread)
    {
        LogInfo(DRIVER_DEFAULT, "Stopping event processing worker thread and waiting for it to complete.");

        // Send event to shutdown worker thread
        KeSetEvent(&pFilter->EventWorkerThreadStopEvent, 0, FALSE);

        // Wait for worker thread to finish
        KeWaitForSingleObject(
            pFilter->EventWorkerThread,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        // Free worker thread
        ObDereferenceObject(pFilter->EventWorkerThread);
        pFilter->EventWorkerThread = NULL;

        LogInfo(DRIVER_DEFAULT, "Event processing worker thread cleaned up.");
    }

    // Clean up any left over events
    if (pFilter->AddressChangesHead.Flink)
    {
        Link = pFilter->AddressChangesHead.Flink;
        while (Link != &pFilter->AddressChangesHead)
        {
            POTLWF_ADDR_EVENT Event = CONTAINING_RECORD(Link, OTLWF_ADDR_EVENT, Link);
            Link = Link->Flink;

            // Delete the event
            NdisFreeMemory(Event, 0, 0);
        }
    }

    // Clean up any left over events
    if (pFilter->NBLsHead.Flink)
    {
        Link = pFilter->NBLsHead.Flink;
        while (Link != &pFilter->NBLsHead)
        {
            POTLWF_NBL_EVENT Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
            Link = Link->Flink;

            otLwfCompleteNBLs(pFilter, FALSE, Event->NetBufferLists, STATUS_CANCELLED);

            // Delete the event
            NdisFreeMemory(Event, 0, 0);
        }
    }

    // Clean up any left over events
    if (pFilter->MacFramesHead.Flink)
    {
        Link = pFilter->MacFramesHead.Flink;
        while (Link != &pFilter->MacFramesHead)
        {
            POTLWF_MAC_FRAME_EVENT Event = CONTAINING_RECORD(Link, OTLWF_MAC_FRAME_EVENT, Link);
            Link = Link->Flink;

            // Delete the event
            NdisFreeMemory(Event, 0, 0);
        }
    }

    // Reinitialize the list head
    InitializeListHead(&pFilter->AddressChangesHead);
    InitializeListHead(&pFilter->NBLsHead);
    InitializeListHead(&pFilter->MacFramesHead);
    
    if (pFilter->EventIrpListHead.Flink)
    {
        FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, FALSE);

        // Clean up any left over IRPs
        Link = pFilter->EventIrpListHead.Flink;
        while (Link != &pFilter->EventIrpListHead)
        {
            PIRP Irp = CONTAINING_RECORD(Link, IRP, Tail.Overlay.ListEntry);
            Link = Link->Flink;
        
            // Before we are allowed to complete the pending IRP, we must remove the cancel routine
            KIRQL irql;
            IoAcquireCancelSpinLock(&irql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(irql);

            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    
        // Reinitialize the list head
        InitializeListHead(&pFilter->EventIrpListHead);
    
        FILTER_RELEASE_LOCK(&pFilter->EventsLock, FALSE);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

// Updates the wait time for the alarm
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateNewWaitTime(
    _In_ PMS_FILTER             pFilter,
    _In_ ULONG                  waitTime
    )
{
    BOOLEAN FireUpdateEvent = TRUE;
    
    // Cancel previous timer
    if (ExCancelTimer(pFilter->EventHighPrecisionTimer, NULL))
    {
        pFilter->EventTimerState = OT_EVENT_TIMER_NOT_RUNNING;
    }

    if (waitTime == (ULONG)(-1))
    {
        // Ignore if we are already stopped
        if (pFilter->NextAlarmTickCount.QuadPart == 0) return;
        pFilter->NextAlarmTickCount.QuadPart = 0;
    }
    else
    {
        if (waitTime == 0)
        {
#ifdef DEBUG_TIMING
            LogInfo(DRIVER_DEFAULT, "Event processing updating to fire timer immediately.");
#endif
            pFilter->EventTimerState = OT_EVENT_TIMER_FIRED;
            pFilter->NextAlarmTickCount.QuadPart = 0;
        }
        else if (waitTime * 10000ll < (KeQueryTimeIncrement() - 30000))
        {
#ifdef DEBUG_TIMING
            LogInfo(DRIVER_DEFAULT, "Event processing starting high precision timer for %u ms.", waitTime);
#endif
            pFilter->EventTimerState = OT_EVENT_TIMER_RUNNING;
            pFilter->NextAlarmTickCount.QuadPart = 0;
            FireUpdateEvent = FALSE;
            ExSetTimer(pFilter->EventHighPrecisionTimer, waitTime * -10000ll, 0, NULL);
        }
        else
        {

            ULONG TickWaitTime = (waitTime * 10000ll) / KeQueryTimeIncrement();
            if (TickWaitTime == 0) TickWaitTime = 1;
#ifdef DEBUG_TIMING
            LogInfo(DRIVER_DEFAULT, "Event processing updating wait ticks to %u.", TickWaitTime);
#endif

            // Update the time to be 'waitTime' ms from 'now', saved in TickCounts
            KeQueryTickCount(&pFilter->NextAlarmTickCount);
            pFilter->NextAlarmTickCount.QuadPart += TickWaitTime;
        }
    }
    
    // Indicate event to worker thread to update the wait time
    KeSetEvent(&pFilter->EventWorkerThreadWaitTimeUpdated, 0, FALSE);
}

// Indicates another tasklet needs to be processed
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateNewTasklet(
    _In_ PMS_FILTER             pFilter
    )
{
    KeSetEvent(&pFilter->EventWorkerThreadProcessTasklets, 0, FALSE);
}

// Called to indicate that we have an Address change to process
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateAddressChange(
    _In_ PMS_FILTER             pFilter,
    _In_ MIB_NOTIFICATION_TYPE  NotificationType,
    _In_ PIN6_ADDR              pAddr
    )
{
    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p", pFilter);

    NT_ASSERT(pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE);

    POTLWF_ADDR_EVENT Event = FILTER_ALLOC_MEM(pFilter->FilterHandle, sizeof(OTLWF_ADDR_EVENT));
    if (Event == NULL)
    {
        LogWarning(DRIVER_DEFAULT, "Failed to alloc new OTLWF_ADDR_EVENT");
    }
    else
    {
        Event->NotificationType = NotificationType;
        Event->Address = *pAddr;

        // Add the event to the queue
        NdisAcquireSpinLock(&pFilter->EventsLock);
        InsertTailList(&pFilter->AddressChangesHead, &Event->Link);
        NdisReleaseSpinLock(&pFilter->EventsLock);

        // Set the event to indicate we have a new address to process
        KeSetEvent(&pFilter->EventWorkerThreadProcessAddressChanges, 0, FALSE);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

// Called to indicate that we have a NetBufferLists to process
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewNetBufferLists(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ PNET_BUFFER_LIST       NetBufferLists
    )
{
    POTLWF_NBL_EVENT Event = FILTER_ALLOC_MEM(pFilter->FilterHandle, sizeof(OTLWF_NBL_EVENT));
    if (Event == NULL)
    {
        LogWarning(DRIVER_DATA_PATH, "Failed to alloc new OTLWF_NBL_EVENT");
        otLwfCompleteNBLs(pFilter, DispatchLevel, NetBufferLists, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Event->NetBufferLists = NetBufferLists;

    // Add the event to the queue
    FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, DispatchLevel);
    InsertTailList(&pFilter->NBLsHead, &Event->Link);
    FILTER_RELEASE_LOCK(&pFilter->EventsLock, DispatchLevel);
    
    // Set the event to indicate we have a new NBL to process
    KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewMacFrameCommand(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_reads_bytes_(BufferLength) 
         const uint8_t*         Buffer,
    _In_ uint8_t                BufferLength
    )
{
    POTLWF_MAC_FRAME_EVENT Event = FILTER_ALLOC_MEM(pFilter->FilterHandle, FIELD_OFFSET(OTLWF_MAC_FRAME_EVENT, Buffer) + BufferLength);
    if (Event == NULL)
    {
        LogWarning(DRIVER_DATA_PATH, "Failed to alloc new OTLWF_MAC_FRAME_EVENT");
        return;
    }

    Event->BufferLength = BufferLength;
    memcpy(Event->Buffer, Buffer, BufferLength);

    // Add the event to the queue
    FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, DispatchLevel);
    InsertTailList(&pFilter->MacFramesHead, &Event->Link);
    FILTER_RELEASE_LOCK(&pFilter->EventsLock, DispatchLevel);
    
    // Set the event to indicate we have a new Mac Frame to process
    KeSetEvent(&pFilter->EventWorkerThreadProcessMacFrames, 0, FALSE);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNetBufferListsCancelled(
    _In_ PMS_FILTER             pFilter,
    _In_ PVOID                  CancelId
    )
{
    PLIST_ENTRY Link = NULL;
    LIST_ENTRY CancelList = {0};
    InitializeListHead(&CancelList);
    
    // Build up a local list of all NBLs that need to be cancelled
    NdisAcquireSpinLock(&pFilter->EventsLock);
    Link = pFilter->NBLsHead.Flink;
    while (Link != &pFilter->NBLsHead)
    {
        POTLWF_NBL_EVENT Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
        Link = Link->Flink;

        if (NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(Event->NetBufferLists) == CancelId)
        {
            RemoveEntryList(&Event->Link);
            InsertTailList(&CancelList, &Event->Link);
        }
    }
    NdisReleaseSpinLock(&pFilter->EventsLock);
    
    // Cancel all the NBLs
    Link = CancelList.Flink;
    while (Link != &CancelList)
    {
        POTLWF_NBL_EVENT Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
        Link = Link->Flink;

        otLwfCompleteNBLs(pFilter, FALSE, Event->NetBufferLists, STATUS_CANCELLED);

        // Delete the event
        NdisFreeMemory(Event, 0, 0);
    }
}

// Completes the NetBufferLists in the event
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfCompleteNBLs(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ PNET_BUFFER_LIST       NetBufferLists,
    _In_ NTSTATUS               Status
    )
{
    LogVerbose(DRIVER_DATA_PATH, "otLwfCompleteNBLs, Filter:%p, NBL:%p, Status:%!STATUS!", pFilter, NetBufferLists, Status);

    // Set the status for all the NBLs
    PNET_BUFFER_LIST CurrNbl = NetBufferLists;
    while (CurrNbl)
    {
        NET_BUFFER_LIST_STATUS(CurrNbl) = Status;
        CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
    }

    NT_ASSERT(NetBufferLists);

    // Indicate the completion
    NdisFSendNetBufferListsComplete(
        pFilter->FilterHandle,
        NetBufferLists,
        DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
        );
}

_Function_class_(DRIVER_CANCEL)
_Requires_lock_held_(_Global_cancel_spin_lock_)
_Releases_lock_(_Global_cancel_spin_lock_)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingCancelIrp(
    _Inout_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
    )
{
    PIRP IrpToCancel = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);

    LogFuncEntryMsg(DRIVER_IOCTL, "Irp=%p", Irp);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Search for a queued up Irp and cancel it if we find it
    //

    NdisAcquireSpinLock(&FilterListLock);

    // Iterate through each filter instance
    for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
    {
        PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);
            
        FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, TRUE);
        
        // Iterate through all queued IRPs for the filter
        PLIST_ENTRY IrpLink = pFilter->EventIrpListHead.Flink;
        while (IrpLink != &pFilter->EventIrpListHead)
        {
            PIRP QueuedIrp = CONTAINING_RECORD(IrpLink, IRP, Tail.Overlay.ListEntry);
            IrpLink = IrpLink->Flink;

            // If we find it, remove from the and prepare to complete it
            if (QueuedIrp == Irp)
            {
                RemoveEntryList(&QueuedIrp->Tail.Overlay.ListEntry);
                IrpToCancel = QueuedIrp;
                break;
            }
        }
            
        FILTER_RELEASE_LOCK(&pFilter->EventsLock, TRUE);

        if (IrpToCancel) break;
    }

    NdisReleaseSpinLock(&FilterListLock);

    if (IrpToCancel)
    {
        IrpToCancel->IoStatus.Status = STATUS_CANCELLED;
        IrpToCancel->IoStatus.Information = 0;
        IoCompleteRequest(IrpToCancel, IO_NO_INCREMENT);
    }

    LogFuncExit(DRIVER_IOCTL);
}

// Queues an Irp for processing
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateIrp(
    _In_ PMS_FILTER pFilter,
    _In_ PIRP       Irp
    )
{
    LogFuncEntryMsg(DRIVER_IOCTL, "Irp=%p", Irp);

    // Mark the Irp as pending
    IoMarkIrpPending(Irp);
    
    FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, FALSE);

    // Set the cancel routine for the Irp
    IoSetCancelRoutine(Irp, otLwfEventProcessingCancelIrp);

    // Queue the Irp up for processing
    InsertTailList(&pFilter->EventIrpListHead, &Irp->Tail.Overlay.ListEntry);

    FILTER_RELEASE_LOCK(&pFilter->EventsLock, FALSE);
    
    // Set the event to indicate we have an Irp to process
    KeSetEvent(&pFilter->EventWorkerThreadProcessIrp, 0, FALSE);

    LogFuncExit(DRIVER_IOCTL);
}

// Processes the next OpenThread IoCtl Irp
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingNextIrp(
    _In_ PMS_FILTER pFilter
    )
{
    PIRP Irp = NULL;

    LogFuncEntry(DRIVER_IOCTL);

    do
    {
        // Reset pointer
        Irp = NULL;

        // Get the next Irp in the queue
        FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, FALSE);
        if (!IsListEmpty(&pFilter->EventIrpListHead))
        {
            PLIST_ENTRY Link = RemoveHeadList(&pFilter->EventIrpListHead);
            Irp = CONTAINING_RECORD(Link, IRP, Tail.Overlay.ListEntry);

            // Clear the cancel routine since we are processing this now
            KIRQL irql;
            IoAcquireCancelSpinLock(&irql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(irql);
        }
        FILTER_RELEASE_LOCK(&pFilter->EventsLock, FALSE);

        if (Irp)
        {    
            otLwfCompleteOpenThreadIrp(pFilter, Irp);
        }

    } while (Irp);

    LogFuncExit(DRIVER_IOCTL);
}

// Indicates a energy scan was completed
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateEnergyScanResult(
    _In_ PMS_FILTER pFilter,
    _In_ CHAR       MaxRssi
    )
{
    LogFuncEntry(DRIVER_IOCTL);

    // Cache the Rssi
    pFilter->otLastEnergyScanMaxRssi = MaxRssi;
    
    // Set the event to indicate we should indicate the state back to OpenThread
    KeSetEvent(&pFilter->EventWorkerThreadEnergyScanComplete, 0, FALSE);

    LogFuncExit(DRIVER_IOCTL);
}

// Helper function to copy data out of a NET_BUFFER
__forceinline
NTSTATUS
CopyDataBuffer(
    _In_ PNET_BUFFER            NetBuffer,
    _In_ ULONG                  Size,
    _Out_writes_bytes_all_(Size) 
         PVOID                  Destination
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    // Read the data out of the NetBuffer
    PVOID mem = NdisGetDataBuffer(NetBuffer, Size, Destination, 1, 0);
    if (mem == NULL)
    {
        NT_ASSERT(FALSE);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    // If we get a different output memory address, then copy that data to Destination;
    // otherwise, it was already copied there
    if (mem != Destination)
    {
        RtlCopyMemory(Destination, mem, Size);
    }

error:

    return status;
}

_Function_class_(EXT_CALLBACK)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
otLwfEventProcessingTimer(
    _In_ PEX_TIMER Timer,
    _In_opt_ PVOID Context
    )
{
    if (Context == NULL) return;

    PMS_FILTER pFilter = (PMS_FILTER)Context;
    UNREFERENCED_PARAMETER(Timer);
    
#ifdef DEBUG_TIMING
    LogInfo(DRIVER_DEFAULT, "Event processing high precision timer fired.");
#endif

    pFilter->EventTimerState = OT_EVENT_TIMER_FIRED;

    // Indicate event to worker thread to update the wait time
    KeSetEvent(&pFilter->EventWorkerThreadWaitTimeUpdated, 0, FALSE);
}

// Worker thread for processing all events
_Use_decl_annotations_
VOID
otLwfEventWorkerThread(
    PVOID   Context
    )
{
    PMS_FILTER pFilter = (PMS_FILTER)Context;
    NT_ASSERT(pFilter);

    LogFuncEntry(DRIVER_DEFAULT);

    PKEVENT WaitEvents[] = 
    { 
        &pFilter->EventWorkerThreadStopEvent,
        &pFilter->EventWorkerThreadProcessNBLs,
        &pFilter->EventWorkerThreadProcessMacFrames,
        &pFilter->EventWorkerThreadWaitTimeUpdated,
        &pFilter->EventWorkerThreadProcessTasklets,
        &pFilter->SendNetBufferListComplete,
        &pFilter->EventWorkerThreadProcessIrp,
        &pFilter->EventWorkerThreadProcessAddressChanges,
        &pFilter->EventWorkerThreadEnergyScanComplete
    };

    KWAIT_BLOCK WaitBlocks[ARRAYSIZE(WaitEvents)] = { 0 };

    // Space to processing buffers
    const ULONG MessageBufferSize = 1280;
    PUCHAR MessageBuffer = FILTER_ALLOC_MEM(pFilter->FilterHandle, MessageBufferSize);
    if (MessageBuffer == NULL)
    {
        LogError(DRIVER_DATA_PATH, "Failed to allocate 1280 bytes for MessageBuffer!");
        return;
    }

#if DEBUG_ALLOC
    // Initialize the list head for allocations
    InitializeListHead(&pFilter->otOutStandingAllocations);

    // Cache the Thread ID
    pFilter->otThreadId = PsGetCurrentThreadId();
#endif

    // Initialize the radio layer
    otLwfRadioInit(pFilter);

    // Calculate the size of the otInstance and allocate it
    pFilter->otInstanceSize = 0;
    (VOID)otInstanceInit(NULL, &pFilter->otInstanceSize);
    NT_ASSERT(pFilter->otInstanceSize != 0);

    // Add space for a pointer back to the filter
    pFilter->otInstanceSize += sizeof(PMS_FILTER);

    // Allocate the buffer
    pFilter->otInstanceBuffer = (PUCHAR)FILTER_ALLOC_MEM(pFilter->FilterHandle, (ULONG)pFilter->otInstanceSize);
    if (pFilter == NULL)
    {
        LogWarning(DRIVER_DEFAULT, "Failed to allocate otInstance buffer, 0x%x bytes", (ULONG)pFilter->otInstanceSize);
        goto exit;
    }
    RtlZeroMemory(pFilter->otInstanceBuffer, pFilter->otInstanceSize);

    // Store the pointer and decrement the size
    memcpy(pFilter->otInstanceBuffer, &pFilter, sizeof(PMS_FILTER));
    pFilter->otInstanceSize -= sizeof(PMS_FILTER);

    // Initialize the OpenThread library
    pFilter->otCachedRole = OT_DEVICE_ROLE_DISABLED;
    pFilter->otCtx = otInstanceInit(pFilter->otInstanceBuffer + sizeof(PMS_FILTER), &pFilter->otInstanceSize);
    NT_ASSERT(pFilter->otCtx);
    if (pFilter->otCtx == NULL)
    {
        LogError(DRIVER_DEFAULT, "otInstanceInit failed, otInstanceSize = %u bytes", (ULONG)pFilter->otInstanceSize);
        goto exit;
    }

    // Make sure our helper function returns the right pointer for the filter, given the openthread instance
    NT_ASSERT(otCtxToFilter(pFilter->otCtx) == pFilter);

    // Disable Icmp (ping) handling
    otIcmp6SetEchoMode(pFilter->otCtx, OT_ICMP6_ECHO_HANDLER_DISABLED);

    // Register callbacks with OpenThread
    otSetStateChangedCallback(pFilter->otCtx, otLwfStateChangedCallback, pFilter);
    otIp6SetReceiveCallback(pFilter->otCtx, otLwfReceiveIp6DatagramCallback, pFilter);

    // Query the current addresses from TCPIP and cache them
    (void)otLwfInitializeAddresses(pFilter);

    // Initialze media connect state to disconnected
    otLwfIndicateLinkState(pFilter, MediaConnectStateDisconnected);

    for (;;)
    {
        NTSTATUS status = STATUS_SUCCESS;

        if (pFilter->NextAlarmTickCount.QuadPart == 0)
        {
#ifdef DEBUG_TIMING
            LogVerbose(DRIVER_DEFAULT, "Event Processing waiting for next event.");
#endif

            // Wait for event to stop or process event to fire
            status = KeWaitForMultipleObjects(ARRAYSIZE(WaitEvents), (PVOID*)WaitEvents, WaitAny, Executive, KernelMode, FALSE, NULL, WaitBlocks);
        }
        else
        {
            LARGE_INTEGER SystemTickCount;
            KeQueryTickCount(&SystemTickCount);

            if (pFilter->NextAlarmTickCount.QuadPart > SystemTickCount.QuadPart)
            {
                // Create the relative (negative) time to wait on
                LARGE_INTEGER Timeout;
                Timeout.QuadPart = (SystemTickCount.QuadPart - pFilter->NextAlarmTickCount.QuadPart) * KeQueryTimeIncrement();
                
#ifdef DEBUG_TIMING
                LogVerbose(DRIVER_DEFAULT, "Event Processing waiting for next event, with timeout, %d ms.", (int)(Timeout.QuadPart / -10000));
#endif

                // Wait for event to stop or process event to fire or timeout
                status = KeWaitForMultipleObjects(ARRAYSIZE(WaitEvents), (PVOID*)WaitEvents, WaitAny, Executive, KernelMode, FALSE, &Timeout, WaitBlocks);
            }
            else
            {
#ifdef DEBUG_TIMING
                LogInfo(DRIVER_DEFAULT, "Event Processing running immediately.");
#endif

                // No need to wait
                status = STATUS_TIMEOUT;
            }
        }

        // If it is the first event, then we are shutting down. Exit loop and terminate thread
        if (status == STATUS_WAIT_0)
        {
            LogInfo(DRIVER_DEFAULT, "Received event worker thread shutdown event.");
            break;
        }
        
#ifdef DEBUG_TIMING
        LogVerbose(DRIVER_DEFAULT, "Event Processing status=0x%x", status);
#endif

        //
        // Event fired to process events
        //

        if (status == STATUS_TIMEOUT || 
            (pFilter->EventTimerState == OT_EVENT_TIMER_FIRED && status == STATUS_WAIT_0 + 3))
        {
            // Reset the wait timeout
            pFilter->NextAlarmTickCount.QuadPart = 0;
            pFilter->EventTimerState = OT_EVENT_TIMER_NOT_RUNNING;

            // Indicate to OpenThread that the alarm has fired
            otPlatAlarmMilliFired(pFilter->otCtx);
        }
        else if (status == STATUS_WAIT_0 + 1) // EventWorkerThreadProcessNBLs fired
        {
            // Go through the queue until there are no more items
            for (;;)
            {
                POTLWF_NBL_EVENT Event = NULL;
                NdisAcquireSpinLock(&pFilter->EventsLock);

                // Just get the first item, if available
                if (!IsListEmpty(&pFilter->NBLsHead))
                {
                    PLIST_ENTRY Link = RemoveHeadList(&pFilter->NBLsHead);
                    Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
                }

                NdisReleaseSpinLock(&pFilter->EventsLock);

                // Break out of the loop if we have emptied the queue
                if (Event == NULL) break;

                NT_ASSERT(Event->NetBufferLists);
                NTSTATUS NblStatus = STATUS_INSUFFICIENT_RESOURCES;

                // Process the event
                PNET_BUFFER_LIST CurrNbl = Event->NetBufferLists;
                while (CurrNbl != NULL)
                {
                    PNET_BUFFER CurrNb = NET_BUFFER_LIST_FIRST_NB(CurrNbl);
                    while (CurrNb != NULL)
                    {
                        NT_ASSERT(NET_BUFFER_DATA_LENGTH(CurrNb) <= MessageBufferSize);
                        if (NET_BUFFER_DATA_LENGTH(CurrNb) <= MessageBufferSize)
                        {
                            // Copy NB data into message
                            if (NT_SUCCESS(CopyDataBuffer(CurrNb, NET_BUFFER_DATA_LENGTH(CurrNb), MessageBuffer)))
                            {
                                otError error = OT_ERROR_NONE;

                                // Create a new message
                                otMessage *message = otIp6NewMessage(pFilter->otCtx, NULL);
                                if (message)
                                {
                                    // Write to the message
                                    error = otMessageAppend(message, MessageBuffer, (uint16_t)NET_BUFFER_DATA_LENGTH(CurrNb));
                                    if (error != OT_ERROR_NONE)
                                    {
                                        LogError(DRIVER_DATA_PATH, "otAppendMessage failed with %!otError!", error);
                                        otMessageFree(message);
                                    }
                                    else
                                    {
                                        IPV6_HEADER* v6Header = (IPV6_HEADER*)MessageBuffer;

                                        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, IP6_SEND: %p : %!IPV6ADDR! => %!IPV6ADDR! (%u bytes)",
                                            pFilter, CurrNbl, &v6Header->SourceAddress, &v6Header->DestinationAddress,
                                            NET_BUFFER_DATA_LENGTH(CurrNb));

#ifdef LOG_BUFFERS
                                        otLogBuffer(MessageBuffer, NET_BUFFER_DATA_LENGTH(CurrNb));
#endif

                                        // Send message (it will free 'message')
                                        error = otIp6Send(pFilter->otCtx, message);
                                        if (error != OT_ERROR_NONE)
                                        {
                                            LogError(DRIVER_DATA_PATH, "otSendIp6Datagram failed with %!otError!", error);
                                        }
                                        else
                                        {
                                            NblStatus = STATUS_SUCCESS;
                                        }
                                    }
                                }
                                else
                                {
                                    LogError(DRIVER_DATA_PATH, "otNewIPv6Message failed!");
                                }
                            }
                        }

                        CurrNb = NET_BUFFER_NEXT_NB(CurrNb);
                    }

                    CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
                }

                if (Event->NetBufferLists)
                {
                    // Complete the NBLs
                    otLwfCompleteNBLs(pFilter, FALSE, Event->NetBufferLists, NblStatus);
                }

                // Free the event
                NdisFreeMemory(Event, 0, 0);
            }
        }
        else if (status == STATUS_WAIT_0 + 2) // EventWorkerThreadProcessMacFrames fired
        {
            // Go through the queue until there are no more items
            for (;;)
            {
                POTLWF_MAC_FRAME_EVENT Event = NULL;
                NdisAcquireSpinLock(&pFilter->EventsLock);

                // Just get the first item, if available
                if (!IsListEmpty(&pFilter->MacFramesHead))
                {
                    PLIST_ENTRY Link = RemoveHeadList(&pFilter->MacFramesHead);
                    Event = CONTAINING_RECORD(Link, OTLWF_MAC_FRAME_EVENT, Link);
                }

                NdisReleaseSpinLock(&pFilter->EventsLock);

                // Break out of the loop if we have emptied the queue
                if (Event == NULL) break;

                // Read the initial length value and validate
                uint16_t packetLength = 0;
                if (try_spinel_datatype_unpack(
                        Event->Buffer,
                        Event->BufferLength,
                        SPINEL_DATATYPE_UINT16_S,
                        &packetLength) &&
                    packetLength <= sizeof(pFilter->otReceiveMessage) &&
                    Event->BufferLength > sizeof(uint16_t) + packetLength)
                {
                    pFilter->otReceiveFrame.mLength = (uint8_t)packetLength;

                    uint8_t offset = 2;
                    uint8_t length = Event->BufferLength - 2;

                    if (packetLength != 0)
                    {
                        memcpy(&pFilter->otReceiveMessage, Event->Buffer + offset, packetLength);
                        offset += pFilter->otReceiveFrame.mLength;
                        length -= pFilter->otReceiveFrame.mLength;
                    }

                    otError errorCode;
                    int8_t noiseFloor = -128;
                    uint16_t flags = 0;
                    if (try_spinel_datatype_unpack(
                            Event->Buffer + offset,
                            length,
                            SPINEL_DATATYPE_INT8_S
                            SPINEL_DATATYPE_INT8_S
                            SPINEL_DATATYPE_UINT16_S
                            SPINEL_DATATYPE_STRUCT_S( // PHY-data
                                SPINEL_DATATYPE_UINT8_S // 802.15.4 channel
                                SPINEL_DATATYPE_UINT8_S // 802.15.4 LQI
                            )
                            SPINEL_DATATYPE_STRUCT_S( // Vendor-data
                                SPINEL_DATATYPE_UINT_PACKED_S
                            ),
                            &pFilter->otReceiveFrame.mInfo.mRxInfo.mRssi,
                            &noiseFloor,
                            &flags,
                            &pFilter->otReceiveFrame.mChannel,
                            &pFilter->otReceiveFrame.mInfo.mRxInfo.mLqi,
                            &errorCode))
                    {
                        otLwfRadioReceiveFrame(pFilter, errorCode);
                    }
                }

                // Free the event
                NdisFreeMemory(Event, 0, 0);
            }
        }
        else if (status == STATUS_WAIT_0 + 3) // EventWorkerThreadWaitTimeUpdated fired
        {
            // Nothing to do, the next time we wait, we will be using the updated time
        }
        else if (status == STATUS_WAIT_0 + 4) // EventWorkerThreadProcessTasklets fired
        {
            // Process all tasklets that were indicated to us from OpenThread
            otTaskletsProcess(pFilter->otCtx);
        }
        else if (status == STATUS_WAIT_0 + 5) // SendNetBufferListComplete fired
        {
            // Handle the completion of the NBL send
            otLwfRadioTransmitFrameDone(pFilter);
        }
        else if (status == STATUS_WAIT_0 + 6) // EventWorkerThreadProcessIrp fired
        {
            // Process any IRPs that were pended
            otLwfEventProcessingNextIrp(pFilter);
        }
        else if (status == STATUS_WAIT_0 + 7) // EventWorkerThreadProcessAddressChanges fired
        {
            // Go through the queue until there are no more items
            for (;;)
            {
                POTLWF_ADDR_EVENT Event = NULL;
                NdisAcquireSpinLock(&pFilter->EventsLock);

                // Get the next item, if available
                if (!IsListEmpty(&pFilter->AddressChangesHead))
                {
                    PLIST_ENTRY Link = RemoveHeadList(&pFilter->AddressChangesHead);
                    Event = CONTAINING_RECORD(Link, OTLWF_ADDR_EVENT, Link);
                }

                NdisReleaseSpinLock(&pFilter->EventsLock);

                // Break out of the loop if we have emptied the queue
                if (Event == NULL) break;
                
                // Process the address change on the Openthread thread
                otLwfEventProcessingAddressChanged(pFilter, Event->NotificationType, &Event->Address);

                // Free the event
                NdisFreeMemory(Event, 0, 0);
            }
        }
        else if (status == STATUS_WAIT_0 + 8) // EventWorkerThreadEnergyScanComplete fired
        {
            // Indicate energy scan complete
            otPlatRadioEnergyScanDone(pFilter->otCtx, pFilter->otLastEnergyScanMaxRssi);
        }
        else
        {
            LogWarning(DRIVER_DEFAULT, "Unexpected wait result, %!STATUS!", status);
        }

        // If we have a frame ready to transmit, do it now if we are allowed to transmit
        if (pFilter->otRadioState == OT_RADIO_STATE_TRANSMIT && !pFilter->SendPending)
        {
            otLwfRadioTransmitFrame(pFilter);
        }
    }

exit:

    otLwfReleaseInstance(pFilter);

    if (pFilter->otInstanceBuffer != NULL)
    {
        NdisFreeMemory(pFilter->otInstanceBuffer, 0, 0);
    }

    LogFuncExit(DRIVER_DEFAULT);

    FILTER_FREE_MEM(MessageBuffer);

    PsTerminateSystemThread(STATUS_SUCCESS);
}
