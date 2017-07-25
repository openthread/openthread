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
 *  This file implements the Tunnel mode (Thread Miniport) functions required for the OpenThread library.
 */

#include "precomp.h"
#include "tunnel.tmh"

KSTART_ROUTINE otLwfTunWorkerThread;

SPINEL_CMD_HANDLER otLwfIrpCommandHandler;

typedef struct _SPINEL_IRP_CMD_CONTEXT
{
    PMS_FILTER              pFilter;
    PIRP                    Irp;
    SPINEL_IRP_CMD_HANDLER *Handler;
    spinel_tid_t            tid;
} SPINEL_IRP_CMD_CONTEXT;

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfTunInitialize(
    _In_ PMS_FILTER pFilter
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    HANDLE threadHandle = NULL;

    LogFuncEntry(DRIVER_DEFAULT);

    NT_ASSERT(pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_THREAD_1_0);
    
    KeInitializeEvent(
        &pFilter->TunWorkerThreadStopEvent,
        SynchronizationEvent, // auto-clearing event
        FALSE                 // event initially non-signalled
        );
    KeInitializeEvent(
        &pFilter->TunWorkerThreadAddressChangedEvent,
        SynchronizationEvent, // auto-clearing event
        FALSE                 // event initially non-signalled
        );

    // Start the worker thread
    Status = PsCreateSystemThread(
                &threadHandle,                  // ThreadHandle
                THREAD_ALL_ACCESS,              // DesiredAccess
                NULL,                           // ObjectAttributes
                NULL,                           // ProcessHandle
                NULL,                           // ClientId
                otLwfTunWorkerThread,           // StartRoutine
                pFilter                         // StartContext
                );
    if (!NT_SUCCESS(Status))
    {
        LogError(DRIVER_DEFAULT, "PsCreateSystemThread failed, %!STATUS!", Status);
        goto error;
    }

    // Grab the object reference to the worker thread
    Status = ObReferenceObjectByHandle(
                threadHandle,
                THREAD_ALL_ACCESS,
                *PsThreadType,
                KernelMode,
                &pFilter->TunWorkerThread,
                NULL
                );
    if (!NT_VERIFYMSG("ObReferenceObjectByHandle can't fail with a valid kernel handle", NT_SUCCESS(Status)))
    {
        LogError(DRIVER_DEFAULT, "ObReferenceObjectByHandle failed, %!STATUS!", Status);
        KeSetEvent(&pFilter->TunWorkerThreadStopEvent, IO_NO_INCREMENT, FALSE);
    }

    // Make sure to enable RLOC passthrough
    Status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_THREAD_RLOC16_DEBUG_PASSTHRU,
            SPINEL_DATATYPE_BOOL_S,
            TRUE
        );
    if (!NT_SUCCESS(Status))
    {
        LogError(DRIVER_DEFAULT, "Enabling RLOC pass through failed, %!STATUS!", Status);
        goto error;
    }

    // TODO - Query other values and capabilities

error:

    if (!NT_SUCCESS(Status))
    {
        otLwfTunUninitialize(pFilter);
    }

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfTunUninitialize(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    // Clean up worker thread
    if (pFilter->TunWorkerThread)
    {
        LogInfo(DRIVER_DEFAULT, "Stopping tunnel worker thread and waiting for it to complete.");

        // Send event to shutdown worker thread
        KeSetEvent(&pFilter->TunWorkerThreadStopEvent, 0, FALSE);

        // Wait for worker thread to finish
        KeWaitForSingleObject(
            pFilter->TunWorkerThread,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        // Free worker thread
        ObDereferenceObject(pFilter->TunWorkerThread);
        pFilter->TunWorkerThread = NULL;

        LogInfo(DRIVER_DEFAULT, "Tunnel worker thread cleaned up.");
    }

    LogFuncExit(DRIVER_DEFAULT);
}

// Worker thread for processing all tunnel events
_Use_decl_annotations_
VOID
otLwfTunWorkerThread(
    PVOID   Context
    )
{
    PMS_FILTER pFilter = (PMS_FILTER)Context;
    NT_ASSERT(pFilter);

    LogFuncEntry(DRIVER_DEFAULT);

    PKEVENT WaitEvents[] = 
    { 
        &pFilter->TunWorkerThreadStopEvent,
        &pFilter->TunWorkerThreadAddressChangedEvent
    };

    LogFuncExit(DRIVER_DEFAULT);
    
    while (true)
    {
        // Wait for event to stop or process event to fire
        NTSTATUS status = 
            KeWaitForMultipleObjects(
                ARRAYSIZE(WaitEvents), 
                (PVOID*)WaitEvents, 
                WaitAny, 
                Executive, 
                KernelMode, 
                FALSE, 
                NULL, 
                NULL);

        // If it is the first event, then we are shutting down. Exit loop and terminate thread
        if (status == STATUS_WAIT_0)
        {
            LogInfo(DRIVER_DEFAULT, "Received tunnel worker thread shutdown event.");
            break;
        }
        else if (status == STATUS_WAIT_0 + 1) // TunWorkerThreadAddressChangedEvent fired
        {
            PVOID DataBuffer = NULL;
            const uint8_t* value_data_ptr = NULL;
            spinel_size_t value_data_len = 0;
            
            // Query the current addresses
            status = 
                otLwfCmdGetProp(
                    pFilter,
                    &DataBuffer,
                    SPINEL_PROP_IPV6_ADDRESS_TABLE,
                    SPINEL_DATATYPE_DATA_S,
                    &value_data_ptr,
                    &value_data_len);
            if (NT_SUCCESS(status))
            {
                uint32_t aNotifFlags = 0;
                otLwfTunAddressesUpdated(pFilter, value_data_ptr, value_data_len, &aNotifFlags);

                // Send notification
                if (aNotifFlags != 0)
                {
                    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
                    if (NotifEntry)
                    {
                        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
                        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
                        NotifEntry->Notif.NotifType = OTLWF_NOTIF_STATE_CHANGE;
                        NotifEntry->Notif.StateChangePayload.Flags = aNotifFlags;

                        otLwfIndicateNotification(NotifEntry);
                    }
                }
            }
            else
            {
                LogWarning(DRIVER_DEFAULT, "Failed to query addresses, %!STATUS!", status);
            }

            if (DataBuffer) FILTER_FREE_MEM(DataBuffer);
        }
        else
        {
            LogWarning(DRIVER_DEFAULT, "Unexpected wait result, %!STATUS!", status);
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfIrpCommandHandler(
    _In_ PMS_FILTER pFilter,
    _In_ PVOID Context,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength
    )
{
    SPINEL_IRP_CMD_CONTEXT* CmdContext = (SPINEL_IRP_CMD_CONTEXT*)Context;
    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(CmdContext->Irp);
    
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    PVOID OutBuffer = CmdContext->Irp->AssociatedIrp.SystemBuffer;
    ULONG OutBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG OrigOutBufferLength = OutBufferLength;

    NTSTATUS status;

    UNREFERENCED_PARAMETER(pFilter);

    // Clear the cancel routine
    IoSetCancelRoutine(CmdContext->Irp, NULL);
    
    if (Data == NULL)
    {
        status = STATUS_CANCELLED;
        OutBufferLength = 0;
    }
    else if (Command == SPINEL_CMD_PROP_VALUE_IS && Key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t spinel_status = SPINEL_STATUS_OK;
        spinel_ssize_t packed_len = spinel_datatype_unpack(Data, DataLength, "i", &spinel_status);
        if (packed_len < 0 || (ULONG)packed_len > DataLength)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            status = ThreadErrorToNtstatus(SpinelStatusToThreadError(spinel_status));
        }
    }
    else if (CmdContext->Handler)
    {
        status = CmdContext->Handler(Key, Data, DataLength, OutBuffer, &OutBufferLength);
    }
    else // No handler, so no output
    {
        status = STATUS_SUCCESS;
        OutBufferLength = 0;
    }

    // Clear any leftover output buffer
    if (OutBufferLength < OrigOutBufferLength)
    {
        RtlZeroMemory((PUCHAR)OutBuffer + OutBufferLength, OrigOutBufferLength - OutBufferLength);
    }

    LogVerbose(DRIVER_IOCTL, "Completing Irp=%p, with %!STATUS! for %s (Out:%u)", 
                CmdContext->Irp, status, IoCtlString(IoControlCode), OutBufferLength);

    // Complete the IRP
    CmdContext->Irp->IoStatus.Information = OutBufferLength;
    CmdContext->Irp->IoStatus.Status = status;
    IoCompleteRequest(CmdContext->Irp, IO_NO_INCREMENT);

    FILTER_FREE_MEM(Context);
}

_Function_class_(DRIVER_CANCEL)
_Requires_lock_held_(_Global_cancel_spin_lock_)
_Releases_lock_(_Global_cancel_spin_lock_)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
VOID
otLwfTunCancelIrp(
    _Inout_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
    )
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    SPINEL_IRP_CMD_CONTEXT* CmdContext = (SPINEL_IRP_CMD_CONTEXT*)IrpStack->Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    LogFuncEntryMsg(DRIVER_IOCTL, "Irp=%p", Irp);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Try to cancel pending command
    otLwfCmdCancel(
        CmdContext->pFilter, 
        (Irp->CancelIrql == DISPATCH_LEVEL) ? TRUE : FALSE, 
        CmdContext->tid);

    LogFuncExit(DRIVER_IOCTL);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunSendCommandForIrp(
    _In_ PMS_FILTER pFilter,
    _In_ PIRP Irp,
    _In_opt_ SPINEL_IRP_CMD_HANDLER *Handler,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    ...
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SPINEL_IRP_CMD_CONTEXT *pContext = NULL;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

    // Create the context structure
    pContext = FILTER_ALLOC_MEM(pFilter->FilterHandle, sizeof(SPINEL_IRP_CMD_CONTEXT));
    if (pContext == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogWarning(DRIVER_DEFAULT, "Failed to allocate irp cmd context");
        goto exit;
    }

    pContext->pFilter = pFilter;
    pContext->Irp = Irp;
    pContext->Handler = Handler;

    NT_ASSERT(IrpStack->Context == NULL);
    IrpStack->Context = pContext;

    // Set the cancel routine
    IoSetCancelRoutine(Irp, otLwfTunCancelIrp);
    
    va_list args;
    va_start(args, pack_format);
    status = 
        otLwfCmdSendAsyncV(
            pFilter, 
            otLwfIrpCommandHandler, 
            pContext, 
            &pContext->tid,
            Command, 
            Key, 
            MaxDataLength, 
            pack_format, 
            args);
    va_end(args);

    // Remove the handler entry from the list
    if (!NT_SUCCESS(status))
    {
        // Clear the cancel routine
        IoSetCancelRoutine(Irp, NULL);

        FILTER_FREE_MEM(pContext);
    }

exit:

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfTunValueIs(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    )
{
    uint32_t aNotifFlags = 0;

    LogFuncEntryMsg(DRIVER_DEFAULT, "[%p] received Value for %s", pFilter, spinel_prop_key_to_cstr(key));

    if (key == SPINEL_PROP_NET_ROLE)
    {
        uint8_t value;
        spinel_datatype_unpack(value_data_ptr, value_data_len, SPINEL_DATATYPE_UINT8_S, &value);

        LogInfo(DRIVER_DEFAULT, "Interface %!GUID! new spinel role: %u", &pFilter->InterfaceGuid, value);

        // Make sure we are in the correct media connect state
        otLwfIndicateLinkState(
            pFilter,
            value > SPINEL_NET_ROLE_DETACHED ?
            MediaConnectStateConnected :
            MediaConnectStateDisconnected);

        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_ROLE;
    }
    else if (key == SPINEL_PROP_IPV6_LL_ADDR)
    {
        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_LL_ADDR;
    }
    else if (key == SPINEL_PROP_IPV6_ML_ADDR)
    {
        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_ML_ADDR;
    }
    else if (key == SPINEL_PROP_NET_PARTITION_ID)
    {
        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_PARTITION_ID;
    }
    else if (key == SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER)
    {
        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER;
    }
    else if (key == SPINEL_PROP_IPV6_ADDRESS_TABLE)
    {
        KeSetEvent(&pFilter->TunWorkerThreadAddressChangedEvent, IO_NO_INCREMENT, FALSE);
    }
    else if (key == SPINEL_PROP_THREAD_CHILD_TABLE)
    {
        // TODO - Update cached children
        // TODO - Send notification
    }
    else if (key == SPINEL_PROP_THREAD_ON_MESH_NETS)
    {
        // TODO - Slaac

        // Set flag to indicate we should send a notification
        aNotifFlags = OT_CHANGED_THREAD_NETDATA;
    }
    else if ((key == SPINEL_PROP_STREAM_NET) || (key == SPINEL_PROP_STREAM_NET_INSECURE))
    {
        const uint8_t* frame_ptr = NULL;
        UINT frame_len = 0;
        spinel_ssize_t ret;

        ret = spinel_datatype_unpack(
            value_data_ptr,
            value_data_len,
            SPINEL_DATATYPE_DATA_WLEN_S SPINEL_DATATYPE_DATA_S,
            &frame_ptr,
            &frame_len,
            NULL,
            NULL);

        NT_ASSERT(ret > 0);
        if (ret > 0)
        {
            otLwfTunReceiveIp6Packet(
                pFilter,
                DispatchLevel,
                (SPINEL_PROP_STREAM_NET_INSECURE == key) ? FALSE : TRUE,
                frame_ptr,
                frame_len);
        }
    }
    else if (key == SPINEL_PROP_MAC_SCAN_STATE)
    {
        // TODO - If pending scan, send notification of completion
    }
    else if (key == SPINEL_PROP_STREAM_RAW)
    {
        // May be used in the future
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

    // Send notification
    if (aNotifFlags != 0)
    {
        PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
        if (NotifEntry)
        {
            RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
            NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
            NotifEntry->Notif.NotifType = OTLWF_NOTIF_STATE_CHANGE;
            NotifEntry->Notif.StateChangePayload.Flags = aNotifFlags;

            otLwfIndicateNotification(NotifEntry);
        }
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfTunValueInserted(
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
    UNREFERENCED_PARAMETER(value_data_ptr);
    UNREFERENCED_PARAMETER(value_data_len);

    if (key == SPINEL_PROP_MAC_SCAN_BEACON)
    {
        PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
        if (NotifEntry)
        {
            RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
            NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
            NotifEntry->Notif.NotifType = OTLWF_NOTIF_ACTIVE_SCAN;
            NotifEntry->Notif.ActiveScanPayload.Valid = TRUE;

            const otExtAddress *aExtAddr = NULL;
            const uint8_t *aExtPanId = NULL;
            const char *aNetworkName = NULL;
            unsigned int xpanid_len = 0;

            if (try_spinel_datatype_unpack(
                value_data_ptr,
                value_data_len,
                SPINEL_DATATYPE_MAC_SCAN_RESULT_S(
                    SPINEL_802_15_4_DATATYPE_MAC_SCAN_RESULT_V1_S,
                    SPINEL_NET_DATATYPE_MAC_SCAN_RESULT_V1_S
                ),
                &NotifEntry->Notif.ActiveScanPayload.Results.mChannel,
                &NotifEntry->Notif.ActiveScanPayload.Results.mRssi,
                &aExtAddr,
                NULL, // saddr (don't care)
                &NotifEntry->Notif.ActiveScanPayload.Results.mPanId,
                &NotifEntry->Notif.ActiveScanPayload.Results.mLqi,
                NULL, // proto (don't care)
                NULL, // flags (don't care)
                &aNetworkName,
                &aExtPanId,
                &xpanid_len
            ) &&
                aExtAddr != NULL && aExtPanId != NULL && aNetworkName != NULL &&
                xpanid_len == OT_EXT_PAN_ID_SIZE)
            {
                memcpy_s(NotifEntry->Notif.ActiveScanPayload.Results.mExtAddress.m8,
                    sizeof(NotifEntry->Notif.ActiveScanPayload.Results.mExtAddress.m8),
                    aExtAddr, sizeof(otExtAddress));
                memcpy_s(NotifEntry->Notif.ActiveScanPayload.Results.mExtendedPanId.m8,
                    sizeof(NotifEntry->Notif.ActiveScanPayload.Results.mExtendedPanId.m8),
                    aExtPanId, sizeof(otExtendedPanId));
                strcpy_s(NotifEntry->Notif.ActiveScanPayload.Results.mNetworkName.m8,
                    sizeof(NotifEntry->Notif.ActiveScanPayload.Results.mNetworkName.m8),
                    aNetworkName);
                otLwfIndicateNotification(NotifEntry);
            }
            else
            {
                FILTER_FREE_MEM(NotifEntry);
            }
        }
    }
    else if (key == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT)
    {
        PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
        if (NotifEntry)
        {
            RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
            NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
            NotifEntry->Notif.NotifType = OTLWF_NOTIF_ENERGY_SCAN;
            NotifEntry->Notif.EnergyScanPayload.Valid = TRUE;

            if (try_spinel_datatype_unpack(
                value_data_ptr,
                value_data_len,
                "Cc",
                &NotifEntry->Notif.EnergyScanPayload.Results.mChannel,
                &NotifEntry->Notif.EnergyScanPayload.Results.mMaxRssi
            ))
            {
                otLwfIndicateNotification(NotifEntry);
            }
            else
            {
                FILTER_FREE_MEM(NotifEntry);
            }
        }
    }

    LogFuncExit(DRIVER_DEFAULT);
}
