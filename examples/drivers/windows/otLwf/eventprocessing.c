/*
 *  Copyright (c) 2016, Microsoft Corporation.
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

typedef struct _OTLWF_NBL_EVENT
{
    LIST_ENTRY          Link;
    BOOLEAN             Received;
    NDIS_PORT_NUMBER    PortNumber;
    PNET_BUFFER_LIST    NetBufferLists;

} OTLWF_NBL_EVENT, *POTLWF_NBL_EVENT;

KSTART_ROUTINE otLwfEventWorkerThread;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfCompleteNBLs(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ BOOLEAN                Received,
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

    NT_ASSERT(pFilter->EventWorkerThread == NULL);
    if (pFilter->EventWorkerThread != NULL)
    {
        status = STATUS_ALREADY_REGISTERED;
        goto error;
    }

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
    PLIST_ENTRY Link = pFilter->NBLsHead.Flink;
    while (Link != &pFilter->NBLsHead)
    {
        POTLWF_NBL_EVENT Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
        Link = Link->Flink;

        otLwfCompleteNBLs(pFilter, FALSE, Event->Received, Event->NetBufferLists, STATUS_CANCELLED);

        // Delete the event
        NdisFreeMemory(Event, 0, 0);
    }

    // Reinitialize the list head
    InitializeListHead(&pFilter->NBLsHead);

    LogFuncExit(DRIVER_DEFAULT);
}

// Updates the wait time for the alarm
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingUpdateWaitTime(
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

// Called to indicate that we have a NetBufferLists to process
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewNetBufferLists(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ BOOLEAN                Received,
    _In_ NDIS_PORT_NUMBER       PortNumber,
    _In_ PNET_BUFFER_LIST       NetBufferLists
    )
{
    POTLWF_NBL_EVENT Event = FILTER_ALLOC_MEM(pFilter->FilterHandle, sizeof(OTLWF_NBL_EVENT));
    if (Event == NULL)
    {
        LogWarning(DRIVER_DATA_PATH, "Failed to alloc new OTLWF_NBL_EVENT");
        otLwfCompleteNBLs(pFilter, DispatchLevel, Received, NetBufferLists, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Event->Received = Received;
    Event->PortNumber = PortNumber;
    Event->NetBufferLists = NetBufferLists;

    // Add the event to the queue
    FILTER_ACQUIRE_LOCK(&pFilter->EventsLock, DispatchLevel);
    if (Received) pFilter->CountPendingRecvNBLs++;
    InsertTailList(&pFilter->NBLsHead, &Event->Link);
    FILTER_RELEASE_LOCK(&pFilter->EventsLock, DispatchLevel);
    
    // Set the event to indicate we have a new NBL to process
    KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingCancelNetBufferLists(
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

        otLwfCompleteNBLs(pFilter, FALSE, Event->Received, Event->NetBufferLists, STATUS_CANCELLED);

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
    _In_ BOOLEAN                Received,
    _In_ PNET_BUFFER_LIST       NetBufferLists,
    _In_ NTSTATUS               Status
    )
{
    LogVerbose(DRIVER_DATA_PATH, "otLwfCompleteNBLs, Filter:%p, NBL:%p, Recv:%u", pFilter, NetBufferLists, Received);

    // Set the status for all the NBLs
    PNET_BUFFER_LIST CurrNbl = NetBufferLists;
    while (CurrNbl)
    {
        NET_BUFFER_LIST_STATUS(CurrNbl) = Status;
        CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
    }

    NT_ASSERT(NetBufferLists);

    // Indicate the completion
    if (Received)
    {
        NdisFReturnNetBufferLists(
            pFilter->FilterHandle,
            NetBufferLists,
            DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
            );
    }
    else
    {
        NdisFSendNetBufferListsComplete(
            pFilter->FilterHandle,
            NetBufferLists,
            DispatchLevel ? NDIS_RETURN_FLAGS_DISPATCH_LEVEL : 0
            );
    }
}


// Helper function to copy data out of a NET_BUFFER
__forceinline
NTSTATUS
CopyDataBuffer(
    _In_ PNET_BUFFER            NetBuffer,
    _In_ ULONG                  Size,
    _In_ PVOID                  Destination
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
        &pFilter->EventWorkerThreadWaitTimeUpdated,
        &pFilter->EventWorkerThreadProcessTasklets,
        &pFilter->SendNetBufferListComplete
    };

    KWAIT_BLOCK WaitBlocks[ARRAYSIZE(WaitEvents)] = { 0 };

    // Space to processing buffers
    UCHAR MessageBuffer[1500];

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
            (pFilter->EventTimerState == OT_EVENT_TIMER_FIRED && status == STATUS_WAIT_0 + 2))
        {
            // Reset the wait timeout
            pFilter->NextAlarmTickCount.QuadPart = 0;
            pFilter->EventTimerState = OT_EVENT_TIMER_NOT_RUNNING;

            // Indicate to OpenThread that the alarm has fired
            otPlatAlarmFired(pFilter->otCtx);
        }
        else if (status == STATUS_WAIT_0 + 1) // EventWorkerThreadProcessNBLs fired
        {
            // Go through the queue until there are no more items
            for (;;)
            {
                POTLWF_NBL_EVENT Event = NULL;
                NdisAcquireSpinLock(&pFilter->EventsLock);
                
                if (IsListeningForPackets(pFilter))
                {
                    // Just get the first item ('send' or 'recv'), if available
                    if (!IsListEmpty(&pFilter->NBLsHead))
                    {
                        PLIST_ENTRY Link = RemoveHeadList(&pFilter->NBLsHead);
                        Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
                        if (Event->Received) pFilter->CountPendingRecvNBLs--;
                    }
                }
                else
                {
                    // Get the next 'send' item to process
                    PLIST_ENTRY Link = pFilter->NBLsHead.Flink;
                    while (Link != &pFilter->NBLsHead)
                    {
                        POTLWF_NBL_EVENT _Event = CONTAINING_RECORD(Link, OTLWF_NBL_EVENT, Link);
                        Link = Link->Flink;

                        // Only return it if not 'recv'
                        if (_Event->Received == FALSE)
                        {
                            RemoveEntryList(&_Event->Link);
                            Event = _Event;
                            break;
                        }
                    }
                }

                NdisReleaseSpinLock(&pFilter->EventsLock);

                // Break out of the loop if we have emptied the queue
                if (Event == NULL) break;

                NT_ASSERT(Event->NetBufferLists);
                NTSTATUS NblStatus = STATUS_INSUFFICIENT_RESOURCES;

                // Process the event
                if (Event->Received) // Received a MAC frame
                {
                    PNET_BUFFER_LIST CurrNbl = Event->NetBufferLists;
                    NT_ASSERT(CurrNbl);
                    while (CurrNbl != NULL)
                    {
                        LogVerbose(DRIVER_DATA_PATH, "Passing NBL:%p to OpenThread to recv.", CurrNbl);

                        PNET_BUFFER CurrNb = NET_BUFFER_LIST_FIRST_NB(CurrNbl);
                        NT_ASSERT(CurrNb);
                        while (CurrNb != NULL)
                        {
                            NT_ASSERT(NET_BUFFER_DATA_LENGTH(CurrNb) <= sizeof(pFilter->otReceiveMessage));
                            if (NET_BUFFER_DATA_LENGTH(CurrNb) <= sizeof(pFilter->otReceiveMessage))
                            {
                                // Copy NB data into message
                                if (NT_SUCCESS(CopyDataBuffer(CurrNb, NET_BUFFER_DATA_LENGTH(CurrNb), &pFilter->otReceiveMessage)))
                                {
                                    POT_NBL_CONTEXT NblContext = GetNBLContext(CurrNbl);
                                    pFilter->otReceiveFrame.mChannel = NblContext->Channel;
                                    pFilter->otReceiveFrame.mPower = NblContext->Power;
                                    pFilter->otReceiveFrame.mLqi = NblContext->Lqi;
                                    pFilter->otReceiveFrame.mLength = (uint8_t)NET_BUFFER_DATA_LENGTH(CurrNb);
                                    otLwfRadioReceiveFrame(pFilter);
                                    NblStatus = STATUS_SUCCESS;
                                }
                            }

                            CurrNb = NET_BUFFER_NEXT_NB(CurrNb);
                        }

                        CurrNbl = NET_BUFFER_LIST_NEXT_NBL(CurrNbl);
                    }
                }
                else // Sending an IPv6 packet
                {
                    PNET_BUFFER_LIST CurrNbl = Event->NetBufferLists;
                    while (CurrNbl != NULL)
                    {
                        PNET_BUFFER CurrNb = NET_BUFFER_LIST_FIRST_NB(CurrNbl);
                        while (CurrNb != NULL)
                        {
                            NT_ASSERT(NET_BUFFER_DATA_LENGTH(CurrNb) <= sizeof(MessageBuffer));
                            if (NET_BUFFER_DATA_LENGTH(CurrNb) <= sizeof(MessageBuffer))
                            {
                                // Copy NB data into message
                                if (NT_SUCCESS(CopyDataBuffer(CurrNb, NET_BUFFER_DATA_LENGTH(CurrNb), MessageBuffer)))
                                {
                                    ThreadError error = kThreadError_None;

                                    // Create a new message
                                    otMessage message = otNewIPv6Message(pFilter->otCtx, (uint16_t)NET_BUFFER_DATA_LENGTH(CurrNb));
                                    if (message)
                                    {
                                        error = otSetMessageLength(message, (uint16_t)NET_BUFFER_DATA_LENGTH(CurrNb));
                                        if (error != kThreadError_None)
                                        {
                                            LogError(DRIVER_DATA_PATH, "otSetMessageLength failed with 0x%x", (ULONG)error);
                                            otFreeMessage(message);
                                        }
                                        else
                                        {
                                            // Write to the message
                                            int bytesWritten = otWriteMessage(message, 0, MessageBuffer, (uint16_t)NET_BUFFER_DATA_LENGTH(CurrNb));
                                            NT_ASSERT(bytesWritten == (int)NET_BUFFER_DATA_LENGTH(CurrNb));
                                            UNREFERENCED_PARAMETER(bytesWritten);

                                            IPV6_HEADER* v6Header = (IPV6_HEADER*)MessageBuffer;
                                            
                                            LogVerbose(DRIVER_DATA_PATH, "Filter: %p, SEND: %p : %!IPV6ADDR! => %!IPV6ADDR! (%u bytes)", 
                                                       pFilter, CurrNbl, &v6Header->SourceAddress, &v6Header->DestinationAddress, 
                                                       NET_BUFFER_DATA_LENGTH(CurrNb));
                                        
                                            #ifdef LOG_BUFFERS
                                            otLogBuffer(MessageBuffer, NET_BUFFER_DATA_LENGTH(CurrNb));
                                            #endif

                                            // Send message (it will free 'message')
                                            error = otSendIp6Datagram(pFilter->otCtx, message);
                                            if (error != kThreadError_None)
                                            {
                                                LogError(DRIVER_DATA_PATH, "otSendIp6Datagram failed with 0x%x", (ULONG)error);
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
                }

                // Complete the NBLs
                otLwfCompleteNBLs(pFilter, FALSE, Event->Received, Event->NetBufferLists, NblStatus);

                // Free the event
                NdisFreeMemory(Event, 0, 0);
            }
        }
        else if (status == STATUS_WAIT_0 + 2) // EventWorkerThreadWaitTimeUpdated fired
        {
            // Nothing to do, the next time we wait, we will be using the updated time
        }
        else if (status == STATUS_WAIT_0 + 3) // EventWorkerThreadProcessTasklets fired
        {
            do
            {
                // Process all tasklets that were indicated to us from OpenThread
                otProcessNextTasklet(pFilter->otCtx);

            } while (otAreTaskletsPending(pFilter->otCtx));
        }
        else if (status == STATUS_WAIT_0 + 4) // SendNetBufferListComplete fired
        {
            // Handle the completion of the NBL send
            otLwfRadioTransmitFrameDone(pFilter);
        }
        else
        {
            LogWarning(DRIVER_DEFAULT, "Unexpected wait result, %!STATUS!", status);
        }

        // If we have a frame ready to transmit, do it now if we are allowed to transmit
        if (pFilter->otPhyState == kStateTransmit && !pFilter->SendPending)
        {
            otLwfRadioTransmitFrame(pFilter);
        }
    }

    LogFuncExit(DRIVER_DEFAULT);

    PsTerminateSystemThread(STATUS_SUCCESS);
}
