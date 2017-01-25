/*
 *    Copyright (c) 2016, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pch.hpp"
#include "serial.tmh"

PAGED
_No_competing_thread_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialInitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
/*++
Routine Description:

    SerialInitialize attempts to find and open the first COM port available, with
    the assumption that it should be for the Thread device.

Arguments:

    AdapterContext - handle to a OTTMP Adapter

Return Value:

    NTSTATUS    - A failure here will indicate the serial COM port was not able
                  to be opened.
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PWSTR SymbolicLinkList = NULL;

    LogFuncEntry(DRIVER_DEFAULT);
    
    PAGED_CODE();
    
    do
    {
        WDF_OBJECT_ATTRIBUTES attr = { 0 };
        WDF_WORKITEM_CONFIG config = { 0 };

        //
        // Send Queue Variables
        //

        InitializeListHead(&AdapterContext->SendQueue);
        AdapterContext->SendQueueRunning = false;

        WDF_OBJECT_ATTRIBUTES_INIT(&attr);
        attr.ParentObject = AdapterContext->Device;
        status = WdfSpinLockCreate(&attr, &AdapterContext->SendLock);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfSpinLockCreate(lockSend) failed %!STATUS!", status);
            break;
        }

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, WDF_DEVICE_INFO);
        attr.ParentObject = AdapterContext->Device;
        WDF_WORKITEM_CONFIG_INIT(&config, SerialSendLoop);

        status = WdfWorkItemCreate(&config, &attr, &AdapterContext->SendWorkItem);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfWorkItemCreate(SerialSendLoop) failed %!STATUS!", status);
            break;
        }
        GetWdfDeviceInfo(AdapterContext->SendWorkItem)->AdapterContext = AdapterContext;

        //
        // Receive Variables
        //

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, WDF_DEVICE_INFO);
        attr.ParentObject = AdapterContext->Device;
        WDF_WORKITEM_CONFIG_INIT(&config, SerialRecvLoop);

        status = WdfWorkItemCreate(&config, &attr, &AdapterContext->RecvWorkItem);
        
        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfWorkItemCreate(SerialRecvLoop) failed %!STATUS!", status);
            break;
        }

        GetWdfDeviceInfo(AdapterContext->RecvWorkItem)->AdapterContext = AdapterContext;

        // Query the system for device with SERIAL interface
        status =
            IoGetDeviceInterfaces(
                &GUID_DEVINTERFACE_COMPORT,
                NULL,
                0,
                &SymbolicLinkList   // List of symbolic names; separate by NULL, EOL with NULL+NULL.
            );

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IoGetDeviceInterfaces failed %!STATUS!", status);
            break;
        }

        // Make sure there is a COM port found
        NT_ASSERT(SymbolicLinkList);
        if (*SymbolicLinkList == NULL) {
            status = STATUS_DEVICE_NOT_CONNECTED;
            LogError(DRIVER_DEFAULT, "No COM ports found!");
            break;
        }

#if DBG
        for (PCWSTR sym = SymbolicLinkList; *sym != NULL; sym += wcslen(sym) + 1)
        {
            LogVerbose(DRIVER_DEFAULT, "Symbolic Name found: %ws", sym);
        }
#endif

        // Try to open each serial port until we get that one works or we exhaust them all
        for (PCWSTR sym = SymbolicLinkList; *sym != NULL; sym += wcslen(sym) + 1)
        {
            // Initialize the target
            status = SerialInitializeTarget(AdapterContext, sym);

            // Break on success
            if (NT_SUCCESS(status)) {
                break;
            }
        }

    } while (false);

    // Clean up on failure
    if (!NT_SUCCESS(status)) {
        SerialUninitialize(AdapterContext);
    }

    if (SymbolicLinkList) {
        ExFreePool(SymbolicLinkList);
        SymbolicLinkList = NULL;
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialUninitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
/*++
Routine Description:

    SerialUninitialize cleans up any cached Wdf IoTarget created from SerialInitialize.

Arguments:

    AdapterContext - handle to a OTTMP Adapter
--*/
{
    LogFuncEntry(DRIVER_DEFAULT);
    
    PAGED_CODE();

    // TODO - Clean up work item

    // TODO - Clean up send queue

    SerialUninitializeTarget(AdapterContext);

    if (AdapterContext->RecvWorkItem) {
        WdfWorkItemFlush(AdapterContext->RecvWorkItem);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

PAGED
_No_competing_thread_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialInitializeTarget(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ PCWSTR                     TargetName
)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WDFIOTARGET tempTarget = WDF_NO_HANDLE;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    do
    {
        DECLARE_UNICODE_STRING_SIZE(PortName, 64); // Maximum name length of the device path to a serial port
        WDF_IO_TARGET_OPEN_PARAMS openParams = { 0 };
        WDF_OBJECT_ATTRIBUTES attr = { 0 };

        // Create the Wdf IoTarget
        status = WdfIoTargetCreate(AdapterContext->Device, WDF_NO_OBJECT_ATTRIBUTES, &tempTarget);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfIoTargetCreate failed %!STATUS!", status);
            break;
        }

        // Try the COM port
        LogInfo(DRIVER_DEFAULT, "Opening device: %ws", TargetName);
        RtlInitUnicodeString(&PortName, TargetName);
        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
            &openParams,
            &PortName,
            GENERIC_READ | GENERIC_WRITE);

        // Open the port on the target
        status = WdfIoTargetOpen(tempTarget, &openParams);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfIoTargetOpen(%wZ) failed %!STATUS!", &PortName, status);
            break;
        }

        AdapterContext->WdfIoTarget = tempTarget;
        tempTarget = WDF_NO_HANDLE;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attr, WDF_DEVICE_INFO);
        attr.ParentObject = AdapterContext->Device;

        status = WdfRequestCreate(&attr, AdapterContext->WdfIoTarget, &AdapterContext->RecvReadRequest);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "WdfRequestCreate failed %!STATUS!", status);
            break;
        }

        // Try to configure the target
        status = SerialConfigure(AdapterContext);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "SerialConfigure failed %!STATUS!", status);
            break;
        }

    } while (false);

    // Clean up on failure
    if (!NT_SUCCESS(status)) {
        SerialUninitializeTarget(AdapterContext);
    }

    if (tempTarget) {
        WdfIoTargetClose(tempTarget);
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialUninitializeTarget(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
)
/*++
Routine Description:

    SerialUninitializeTarget cleans up any cached Wdf IoTarget created from SerialInitializeTarget.

Arguments:

    AdapterContext - handle to a OTTMP Adapter
--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    if (AdapterContext->WdfIoTarget) {
        //
        // WdfIoTargetStop will cancel all the outstanding I/O and wait
        // for them to complete before returning. WdfIoTargetStop  with the
        // action type WdfIoTargetCancelSentIo can be called at IRQL PASSIVE_LEVEL only.
        //
        WdfIoTargetStop(AdapterContext->WdfIoTarget, WdfIoTargetCancelSentIo);
        WdfIoTargetClose(AdapterContext->WdfIoTarget);
        AdapterContext->WdfIoTarget = NULL;
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
SerialSendIoctl(
    _In_     POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_     ULONG                      IoctlCode,
    _In_opt_ PWDF_REQUEST_SEND_OPTIONS  RequestOptions = NULL,
    _In_opt_ PWDF_MEMORY_DESCRIPTOR     InputBuffer = WDF_NO_HANDLE,
    _In_opt_ PWDF_MEMORY_DESCRIPTOR     OutputBuffer = WDF_NO_HANDLE,
    _Out_opt_ PULONG_PTR                BytesReturned = NULL
    )
/*++
Routine Description:

    Helper/Wrapper function for WdfIoTargetSendIoctlSynchronously.

--*/
{
    return WdfIoTargetSendIoctlSynchronously(
            AdapterContext->WdfIoTarget, 
            WDF_NO_HANDLE, 
            IoctlCode, 
            InputBuffer, 
            OutputBuffer, 
            RequestOptions, 
            BytesReturned);
}

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SerialConfigure(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
/*++
Routine Description:

    SerialInitialize attempts to find and open the first COM port available, with
    the assumption that it should be for the Thread device.

Arguments:

    AdapterContext - handle to a OTTMP Adapter

Return Value:

    NTSTATUS    - A failure here will indicate the serial COM port was not able
                  to be configured as desired.
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WDF_MEMORY_DESCRIPTOR inputDesc;
    WDF_REQUEST_SEND_OPTIONS wrso = {
        sizeof(WDF_REQUEST_SEND_OPTIONS),
        WDF_REQUEST_SEND_OPTION_TIMEOUT | WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
        WDF_REL_TIMEOUT_IN_SEC(1) // Nothing should take more than a second to complete
    };

    LogFuncEntry(DRIVER_DEFAULT);
    
    PAGED_CODE();
    
    do
    {
        // Initial reset of the device
        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_RESET_DEVICE, &wrso);
        
        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_RESET_DEVICE failed %!STATUS!", status);
            break;
        }

        // 8 bits, no parity, 1 stop bit
        {
            const SERIAL_LINE_CONTROL slc = { STOP_BIT_1, NO_PARITY, 8 };
            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&slc, sizeof(slc));

            status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_LINE_CONTROL, &wrso, &inputDesc);
            
            if (!NT_SUCCESS(status)) {
                LogError(DRIVER_DEFAULT,  "IOCTL_SERIAL_SET_LINE_CONTROL failed %!STATUS!", status );
                break;
            }
        }

        // Xon and Xoff characters
        {
            const SERIAL_CHARS sc = { 0, 0, 0, 0, 0x11, 0x13 };
            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&sc, sizeof(sc));

            status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_CHARS, &wrso, &inputDesc);
            
            if (!NT_SUCCESS(status)) {
                LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_CHARS failed %!STATUS!", status);
                break;
            }
        }

        // Baud rate
        {
            const SERIAL_BAUD_RATE sbr = { 115200 };
            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&sbr, sizeof(sbr));

            status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_BAUD_RATE, &wrso, &inputDesc);
            
            if (!NT_SUCCESS(status)) {
                LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_BAUD_RATE failed %!STATUS!", status);
                break;
            }
        }
        
        /*{
            // Only send if CTS is set, Set RTS before sending
            const SERIAL_HANDFLOW shf = 
            {
                SERIAL_CTS_HANDSHAKE, SERIAL_RTS_CONTROL,
                MAX_SPINEL_COMMAND_LENGTH, MAX_SPINEL_COMMAND_LENGTH
            };
            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&shf, sizeof(shf));
            
            status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_HANDFLOW, &wrso, &inputDesc);
            
            if (!NT_SUCCESS(status)) {
                LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_HANDFLOW failed %!STATUS!", status);
                // break; Ignore for now
            }
        }

        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_XON, &wrso);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_XON failed %!STATUS!", status);
            break;
        }

        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_RTS, &wrso);
        
        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_RTS failed %!STATUS!", status);
            break;
        }

        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_DTR, &wrso);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_DTR failed %!STATUS!", status);
            break;
        }*/

        {
            const SERIAL_TIMEOUTS sto = {
                1, 0, 0,      // On read, only timeout if more than 1ms *between* bytes (wait forever for first byte)
                1, 10         // Write times out after (1ms * n-bytes) + (10ms)
            };
            WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&sto, sizeof(sto));

            status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_SET_TIMEOUTS, &wrso, &inputDesc);
        
            if (!NT_SUCCESS(status)) {
                LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_SET_TIMEOUTS failed %!STATUS!", status);
                break;
            }
        }

        status = SerialFlushAndCheckStatus(AdapterContext);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "SerialFlushAndCheckStatus failed %!STATUS!", status);
            break;
        }

    } while (false);

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SerialCheckStatus(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ bool                       DataExpected
    )
/*++
Routine Description:

    SerialCheckStatus validates the current status of the serial COM port.

Arguments:

    AdapterContext - handle to a OTTMP Adapter

Return Value:

    NTSTATUS    - A failure here will indicate the serial COM port is not in an
                  expected state.
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WDF_MEMORY_DESCRIPTOR outputDesc;
    WDF_REQUEST_SEND_OPTIONS wrso = {
        sizeof(WDF_REQUEST_SEND_OPTIONS),
        WDF_REQUEST_SEND_OPTION_TIMEOUT | WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
        WDF_REL_TIMEOUT_IN_SEC(1) // Nothing should take more than a second to complete
    };
    ULONG_PTR bytesReturned = 0;
    SERIAL_STATUS ss = { 0 };
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDesc, (PVOID)&ss, sizeof(ss));

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    // Check to ensure we are ready to send
    status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_GET_COMMSTATUS, &wrso, WDF_NO_HANDLE, &outputDesc, &bytesReturned);

    if (!NT_SUCCESS(status)) 
    {
        LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_GET_COMMSTATUS failed %!STATUS!", status);
    }
    else if (bytesReturned >= sizeof(ss)) 
    {
        if (ss.HoldReasons)
        {
            if (ss.HoldReasons != SERIAL_TX_WAITING_FOR_CTS)
            {
                LogError(DRIVER_DEFAULT, "HoldReasons is wrong (should only be CTS, but is %x)", ss.HoldReasons );
                status = STATUS_INVALID_DEVICE_STATE;
            }
            else if (!DataExpected)
            {
                LogError(DRIVER_DEFAULT, "Adapter already has data on init!?!?!");
                status = STATUS_INVALID_STATE_TRANSITION;
            }
        }
        if (ss.Errors)
        {
            LogWarning(DRIVER_DEFAULT, "Unexpected Error %x", ss.Errors);
            status = STATUS_UNSUCCESSFUL;
        }
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SerialFlushAndCheckStatus(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
/*++
Routine Description:

    SerialFlushAndCheckStatus flushed and validates the current status of the serial COM port.

Arguments:

    AdapterContext - handle to a OTTMP Adapter

Return Value:

    NTSTATUS    - A failure here will indicate the serial COM port is not in an
                  expected state.
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    do
    {
        WDF_MEMORY_DESCRIPTOR inputDesc;
        WDF_REQUEST_SEND_OPTIONS wrso = {
            sizeof(WDF_REQUEST_SEND_OPTIONS),
            WDF_REQUEST_SEND_OPTION_TIMEOUT | WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
            WDF_REL_TIMEOUT_IN_SEC(1) // Nothing should take more than a second to complete
        };
        const ULONG flags = SERIAL_PURGE_RXABORT | SERIAL_PURGE_RXCLEAR | SERIAL_PURGE_TXABORT | SERIAL_PURGE_TXCLEAR;

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, (PVOID)&flags, sizeof(flags));
        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_PURGE, &wrso, &inputDesc);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_PURGE failed %!STATUS!", status);
            break;
        }

        status = SerialSendIoctl(AdapterContext, IOCTL_SERIAL_CLEAR_STATS, &wrso);

        if (!NT_SUCCESS(status)) {
            LogError(DRIVER_DEFAULT, "IOCTL_SERIAL_CLEAR_STATS failed %!STATUS!", status);
            break;
        }

        status = SerialCheckStatus(AdapterContext, false);
        for (int i = 0; !NT_SUCCESS(status) && i < 20; i++)
        {
            NdisMSleep(1); // just sleep enough to give up our quantum
            status = SerialCheckStatus(AdapterContext, false);
        }

    } while (false);

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
bool
SerialPushSend(
    _In_ POTTMP_ADAPTER_CONTEXT AdapterContext,
    _In_ PSERIAL_SEND_ITEM      SendItem
    )
{
    WdfSpinLockAcquire(AdapterContext->SendLock);
    
    // Start the work item up if it's not already running
    if (!AdapterContext->SendQueueRunning)
    {
        LogVerbose(DRIVER_DEFAULT, "Starting Send Work Item");
        AdapterContext->SendQueueRunning = true;
        WdfWorkItemEnqueue(AdapterContext->SendWorkItem);
    }

    // Insert the new item at the end of the list
    InsertTailList(&AdapterContext->SendQueue, &SendItem->Link);

    WdfSpinLockRelease(AdapterContext->SendLock);

    return true;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
PSERIAL_SEND_ITEM
SerialPopSend(
    _In_ POTTMP_ADAPTER_CONTEXT AdapterContext
    )
{
    PLIST_ENTRY current = NULL;

    // Grab the head of the list
    // Careful, this might have gotten aborted, leaving the list empty
    WdfSpinLockAcquire(AdapterContext->SendLock);

    if (!IsListEmpty(&AdapterContext->SendQueue))
    {
        current = RemoveHeadList(&AdapterContext->SendQueue);
    }
    if (current == NULL)
    {
        // Do this under the lock, but the state is consumed outside the lock
        AdapterContext->SendQueueRunning = false;
        LogVerbose(DRIVER_DEFAULT, "Send Work Item Complete");
    }

    WdfSpinLockRelease(AdapterContext->SendLock);

    return current ? CONTAINING_RECORD(current, SERIAL_SEND_ITEM, Link) : NULL;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
SerialSendData(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ PNET_BUFFER_LIST           NetBufferList
    )
/*++
Routine Description:

    SerialSendData encodes and queues up the data to be sent over the serial COM port.

Arguments:

    AdapterContext - handle to a OTTMP Adapter

    NetBufferLists - a single NET_BUFFER_LIST object, containing a signle NET_BUFFER for 
                     Spinel tunnel commands.

    DispatchLevel  - flag indicating if we are running at dispatch or not

Return Value:

    NTSTATUS    - A failure here will indicate we either failed to encode or queue the data.
--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY WdfMemBuffer = NULL;
    PSERIAL_SEND_ITEM SendItem = NULL;
    PUCHAR DecodedBuffer = NULL;
    ULONG DecodedBufferLength = NetBufferList->FirstNetBuffer->DataLength;
    ULONG EncodedBufferLength;

    LogFuncEntry(DRIVER_DEFAULT);

    do
    {
        // Get the decoded buffer from the NBL/NB. We required
        // the use of contiguous buffers.
        DecodedBuffer = (PUCHAR)NdisGetDataBuffer(NetBufferList->FirstNetBuffer, DecodedBufferLength, NULL, 1, 0);
        if (DecodedBuffer == NULL) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        LogVerbose(DRIVER_DEFAULT, "Sending %u decoded bytes", DecodedBufferLength);
        DumpBuffer(DecodedBuffer, DecodedBufferLength);

        // Calculate the buffer size required
        EncodedBufferLength = HdlcComputeEncodedLength(DecodedBuffer, DecodedBufferLength);
        
        // Allocate the memory
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = AdapterContext->Device;
#pragma warning(push)
#pragma warning(suppress: 28160) // Param 3 could be 0
        status = WdfMemoryCreate(
                    &attributes,
                    NonPagedPoolNx,
                    0,
                    SERIAL_SEND_ITEM_SIZE + EncodedBufferLength,
                    &WdfMemBuffer,
                    (PVOID*)&SendItem
                    );
#pragma warning(pop)

        if (!NT_SUCCESS(status)) {
            LogWarning(DRIVER_DEFAULT, "WdfMemoryCreate (%u bytes) failed %!STATUS!", (SERIAL_SEND_ITEM_SIZE + EncodedBufferLength), status);
            break;
        }
        
        SendItem->NetBufferList = NetBufferList;
        SendItem->WdfMemory = WdfMemBuffer;
        SendItem->EncodedBufferLength = EncodedBufferLength;

        // Encode data
        if (!HdlcEncodeBuffer(DecodedBuffer, DecodedBufferLength, SendItem->EncodedBuffer, EncodedBufferLength)) {
            NT_ASSERT(FALSE); // Should never fail, unless we have a bug in the length computation
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Queue data to be sent out
        if (!SerialPushSend(AdapterContext, SendItem)) {
            status = STATUS_DEVICE_NOT_READY;
            break;
        }

    } while (false);

    if (!NT_SUCCESS(status)) {
        if (WdfMemBuffer) {
            WdfObjectDelete(WdfMemBuffer);
        }
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialSendLoop(
    _In_ WDFWORKITEM WorkItem
    )
/*++
Routine Description:

    SerialSendLoop handles the actual sending of data over the serial COM port.

Arguments:

    WorkItem - handle to a Wdf Device Info object for the Adapter context

--*/
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = GetWdfDeviceInfo(WorkItem)->AdapterContext;
    WDF_REQUEST_SEND_OPTIONS wrso = {
        sizeof(WDF_REQUEST_SEND_OPTIONS),
        WDF_REQUEST_SEND_OPTION_TIMEOUT | WDF_REQUEST_SEND_OPTION_SYNCHRONOUS,
        WDF_REL_TIMEOUT_IN_SEC(1) // Nothing should take more than a second to complete
    };
    WDFMEMORY_OFFSET offset = { 0 };
    PSERIAL_SEND_ITEM SendItem = NULL;
    WDF_MEMORY_DESCRIPTOR wmd;
    NTSTATUS status;

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = AdapterContext->Device;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

#pragma warning(push)
#pragma warning(suppress: 6387) // Param 2 is NULL
    WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&wmd, NULL, &offset);
#pragma warning(pop)

    while (NULL != (SendItem = SerialPopSend(AdapterContext)))
    {
        LogVerbose(DRIVER_DEFAULT, "Sending %u encoded bytes", SendItem->EncodedBufferLength);
        DumpBuffer(SendItem->EncodedBuffer, SendItem->EncodedBufferLength);

        if (SendItem->EncodedBufferLength > 0)
        {
            offset.BufferLength = SendItem->EncodedBufferLength;
            status = 
                WdfMemoryCreatePreallocated(
                    &Attributes, 
                    SendItem->EncodedBuffer, 
                    SendItem->EncodedBufferLength, 
                    &wmd.u.HandleType.Memory);

            if (!NT_SUCCESS( status )) {
                LogError(DRIVER_DEFAULT, "WdfIoTargetSendWriteSynchronously (%u bytes) failed %!STATUS!", SendItem->EncodedBufferLength, status);
            }
            else
            {
                // Send the buffer out
                status = WdfIoTargetSendWriteSynchronously(AdapterContext->WdfIoTarget, NULL, &wmd, NULL, &wrso, NULL);

                if (!NT_SUCCESS( status )) {
                    LogError(DRIVER_DEFAULT, "WdfIoTargetSendWriteSynchronously (%u bytes) failed %!STATUS!", SendItem->EncodedBufferLength, status);
                }
            
                WdfObjectDelete(wmd.u.HandleType.Memory);
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }

        // Complete the NetBufferList
        SendItem->NetBufferList->Status = status;
#ifdef OTTMP_LEGACY
        NdisMSendNetBufferListsComplete(AdapterContext->Adapter, SendItem->NetBufferList, 0);
#else
        NetBufferListsCompleteSend(SendItem->NetBufferList);
#endif

        // Hack to sleep 1 ms per 5 bytes sent
        NdisMSleep(1000 * (1 + SendItem->EncodedBufferLength / 5));

        // Cleanup
        WdfObjectDelete(SendItem->WdfMemory);
    };

    LogFuncExit(DRIVER_DEFAULT);
}

PAGED
_Function_class_(EVT_WDF_WORKITEM)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
SerialRecvLoop(
    _In_ WDFWORKITEM WorkItem
    )
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = GetWdfDeviceInfo(WorkItem)->AdapterContext;
    WDFMEMORY mem = { 0 };
    NTSTATUS status;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    do
    {
        WDFREQUEST & request = AdapterContext->RecvReadRequest;

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = AdapterContext->Device;
        status =
            WdfMemoryCreatePreallocated(
                &attributes, 
                AdapterContext->RecvBuffer + AdapterContext->RecvBufferLength, 
                MAX_SPINEL_COMMAND_LENGTH, 
                &mem);

        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "WdfMemoryCreateFromLookaside failed %!STATUS!", status);
            break;
        }

        status = WdfIoTargetFormatRequestForRead(AdapterContext->WdfIoTarget, request, mem, NULL, NULL);

        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "WdfIoTargetFormatRequestForRead failed %!STATUS!", status);
            break;
        }
        else
        {
            WdfRequestSetCompletionRoutine(request, SerialRecvComplete, AdapterContext);
            if (WdfRequestSend(request, AdapterContext->WdfIoTarget, WDF_NO_SEND_OPTIONS))
            {
                // Send succeeded, no cleanup
                mem = NULL;
                break;
            }

            status = WdfRequestGetStatus(request);
            if (!NT_SUCCESS(status))
            {
                LogError(DRIVER_DEFAULT, "WdfRequestSend failed %!STATUS!", status);
            }

            WDF_REQUEST_REUSE_PARAMS reuseParams = { 0 };
            WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

            // refresh the request so it is ready to reuse
            status = WdfRequestReuse(request, &reuseParams);
            if (!NT_SUCCESS(status))
            {
                NT_ASSERT(NT_SUCCESS(status));
                LogError(DRIVER_DEFAULT, "WdfRequestReuse failed %!STATUS!", status);
            }
        }

    } while (false);

    if (mem) {
        WdfObjectDelete(mem);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(return==0,_At_(*pNetBufferList, __drv_allocatesMem(mem)))
_When_(return==0,_At_((*pNetBufferList)->FirstNetBuffer, __drv_allocatesMem(mem)))
NTSTATUS
SerialAllocateNetBufferList(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext,
    _In_ ULONG                      BufferLength,
    _Out_ PNET_BUFFER_LIST         *pNetBufferList
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNET_BUFFER_LIST NetBufferList = NULL;
    PNET_BUFFER NetBuffer = NULL;

    do
    {

#ifdef OTTMP_LEGACY
        // Allocate the NetBufferList
        NetBufferList = NdisAllocateNetBufferList(AdapterContext->pGlobals->hNblPool, 0, 0);

        if (NetBufferList == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Allocate the NetBuffer
        NetBufferList->FirstNetBuffer = NdisAllocateNetBufferMdlAndData(AdapterContext->pGlobals->hNbPool);

        if (NetBufferList->FirstNetBuffer == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
#else
        // Grab a NetBufferList from the collection
        status = NetBufferListCollectionRetrieveNbls(AdapterContext->ReceiveCollection, 1, &NetBufferList);

        if (!NT_SUCCESS(status))
        {
            break;
        }
#endif

        NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
        NetBuffer = NetBufferList->FirstNetBuffer;

        // If there is no buffer allocated yet, go ahead and allocate the max
        if (NET_BUFFER_DATA_LENGTH(NetBuffer) == 0)
        {
            // Allocate the max buffer size
            ndisStatus = NdisRetreatNetBufferDataStart(NetBuffer, MAX_SPINEL_COMMAND_LENGTH, 0, NULL);
            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                LogError(DRIVER_DEFAULT, "NdisRetreatNetBufferDataStart failed %!NDIS_STATUS!", ndisStatus);
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

        // By this point, we should have a NetBuffer with a contiguous memory block of MAX_SPINEL_COMMAND_LENGTH bytes,
        // though it's offset could be anywhere in the buffer, from when it was previously used.

        // Adjust buffer length to fit the requested length
        if (NET_BUFFER_DATA_LENGTH(NetBuffer) > BufferLength)
        {
            NdisAdvanceNetBufferDataStart(NetBuffer, NET_BUFFER_DATA_LENGTH(NetBuffer) - BufferLength, FALSE, NULL);
        }
        else if (NET_BUFFER_DATA_LENGTH(NetBuffer) < BufferLength)
        {
            ndisStatus = NdisRetreatNetBufferDataStart(NetBuffer, BufferLength - NET_BUFFER_DATA_LENGTH(NetBuffer), 0, NULL);
            NT_ASSERT(ndisStatus == NDIS_STATUS_SUCCESS);
            if (ndisStatus != NDIS_STATUS_SUCCESS)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

        // Set the output
        *pNetBufferList = NetBufferList;

    } while (FALSE);

    if (!NT_SUCCESS(status))
    {
#ifdef OTTMP_LEGACY
        if (NetBuffer) NdisFreeNetBuffer(NetBuffer);
        if (NetBufferList) NdisFreeNetBufferList(NetBufferList);
#else
        if (NetBufferList) NetBufferListsDiscardReceive(NetBufferList);
#endif
    }

    return status;
}

_Function_class_(EVT_WDF_REQUEST_COMPLETION_ROUTINE)
_IRQL_requires_same_
VOID
SerialRecvComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context
    )
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)Context;
    NTSTATUS status;

    LogFuncEntry(DRIVER_DEFAULT);

    UNREFERENCED_PARAMETER(Target); // Except for an assert
    NT_ASSERT((Target == AdapterContext->WdfIoTarget) || (WDF_NO_HANDLE == AdapterContext->WdfIoTarget));
    NT_ASSERT(Request == AdapterContext->RecvReadRequest);

    WDFMEMORY mem = Params->Parameters.Read.Buffer;
    NT_ASSERT(mem);

    status = WdfRequestGetStatus(Request);
    if (NT_SUCCESS(status))
    {
        NT_ASSERT(Params->Type == WdfRequestTypeRead);
        NT_ASSERT(Params->Parameters.Read.Offset == 0);

        size_t MemoryLength = 0;
        NT_ASSERT(AdapterContext->RecvBuffer + AdapterContext->RecvBufferLength == (PUCHAR)WdfMemoryGetBuffer(mem, &MemoryLength));
        UNREFERENCED_PARAMETER(MemoryLength);

        LogVerbose(DRIVER_DEFAULT, "Received %u encoded bytes", (ULONG)Params->IoStatus.Information);
        DumpBuffer(AdapterContext->RecvBuffer + AdapterContext->RecvBufferLength, (ULONG)Params->IoStatus.Information);

        AdapterContext->RecvBufferLength += (ULONG)Params->IoStatus.Information;
        
        // Decode and receive
        ULONG ReadOffset = 0;
        while (AdapterContext->RecvBufferLength > ReadOffset)
        {
            // Parse, validate and compute the decoded buffer size requirements
            ULONG UsedEncodedBufferLength = AdapterContext->RecvBufferLength - ReadOffset;
            ULONG DecodedBufferLength = 0;
            bool HasGoodBuffer = false;
            bool HasCompleteBuffer = 
                HdlcDecodeBuffer(
                    AdapterContext->RecvBuffer + ReadOffset,
                    &UsedEncodedBufferLength,
                    &DecodedBufferLength,
                    NULL,
                    &HasGoodBuffer);

            // We should never have used more buffer than available
            NT_ASSERT(UsedEncodedBufferLength <= AdapterContext->RecvBufferLength - ReadOffset);

            // Did we have a complete (start and end sequence chars) buffer?
            if (!HasCompleteBuffer)
            {
                AdapterContext->RecvBufferLength -= ReadOffset;

                LogWarning(DRIVER_DEFAULT, "Buffering %u incomplete bytes", AdapterContext->RecvBufferLength);
                NT_ASSERT(AdapterContext->RecvBufferLength < MAX_SPINEL_COMMAND_LENGTH);

                memmove_s(AdapterContext->RecvBuffer, sizeof(AdapterContext->RecvBuffer), 
                          AdapterContext->RecvBuffer + ReadOffset, AdapterContext->RecvBufferLength);
                break;
            }
            else
            {
                // Was the buffer too short or did it's FCS not match?
                if (HasGoodBuffer)
                {
                    NT_ASSERT(UsedEncodedBufferLength <= MAX_SPINEL_COMMAND_LENGTH);

                    // Allocate the NetBufferList & NetBuffer to decode the data to
                    PNET_BUFFER_LIST NetBufferList = NULL;
                    status = SerialAllocateNetBufferList(AdapterContext, DecodedBufferLength, &NetBufferList);

                    if (NT_SUCCESS(status))
                    {
                        PNET_BUFFER NetBuffer = NetBufferList->FirstNetBuffer;
                        NT_ASSERT(DecodedBufferLength == NET_BUFFER_DATA_LENGTH(NetBuffer));

                        // Get pointer to contiguous buffer
                        PUCHAR DecodedBuffer = (PUCHAR)NdisGetDataBuffer(NetBuffer, DecodedBufferLength, NULL, 1, 0);
                        NT_ASSERT(DecodedBuffer);
                        if (DecodedBuffer)
                        {
                            HasCompleteBuffer = 
                                HdlcDecodeBuffer(
                                    AdapterContext->RecvBuffer + ReadOffset,
                                    &UsedEncodedBufferLength,
                                    &DecodedBufferLength,
                                    DecodedBuffer,
                                    &HasGoodBuffer);

                            NT_ASSERT(HasCompleteBuffer);
                            NT_ASSERT(HasGoodBuffer);
                            NT_ASSERT(DecodedBufferLength == NET_BUFFER_DATA_LENGTH(NetBuffer));
                                
                            LogVerbose(DRIVER_DEFAULT, "Received %u decoded bytes", DecodedBufferLength);
                            DumpBuffer(DecodedBuffer, DecodedBufferLength);
                        }
                        else
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }

                        if (NT_SUCCESS(status))
                        {
                            // Indicate up the new NBL we just created
#ifdef OTTMP_LEGACY
                            NdisMIndicateReceiveNetBufferLists(
                                AdapterContext->Adapter,
                                NetBufferList,
                                NDIS_DEFAULT_PORT_NUMBER,
                                1,
                                0
                                );
#else
                            NetBufferListsCompleteReceive(
                                NetBufferList,
                                NDIS_DEFAULT_PORT_NUMBER,
                                0
                                );
#endif
                        }
                        else
                        {
#ifdef OTTMP_LEGACY
                            NdisFreeNetBuffer(NetBuffer);
                            NdisFreeNetBufferList(NetBufferList);
#else
                            NetBufferListsDiscardReceive(NetBufferList);
#endif
                        }
                    }
                }
                else
                {
                    LogWarning(DRIVER_DEFAULT, "Dropping %u bad bytes", UsedEncodedBufferLength);
                    DumpBuffer(AdapterContext->RecvBuffer + ReadOffset, UsedEncodedBufferLength);
                }

                // Skip over used data
                ReadOffset += UsedEncodedBufferLength;
            }
        }

        // We read all the buffer, so reset the length
        if (AdapterContext->RecvBufferLength == ReadOffset)
        {
            AdapterContext->RecvBufferLength = 0;
        }
    }
    else
    {
        LogError(DRIVER_DEFAULT, "Read request failed %!STATUS!", status);
    }

    WdfObjectDelete(mem);

    if (status != STATUS_DELETE_PENDING)
    {
        WDF_REQUEST_REUSE_PARAMS reuseParams = { 0 };
        WDF_REQUEST_REUSE_PARAMS_INIT(&reuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
        status = WdfRequestReuse(Request, &reuseParams);
        if (!NT_SUCCESS(status))
        {
            NT_ASSERT(NT_SUCCESS(status));
            LogError(DRIVER_DEFAULT, "WdfRequestReuse failed %!STATUS!", status);
        }
        
        LogVerbose(DRIVER_DEFAULT, "Starting recv worker");
        WdfWorkItemEnqueue(AdapterContext->RecvWorkItem);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

VOID 
DumpLine(
    _In_reads_bytes_(aLength) PCUCHAR aBuf, 
    _In_ size_t aLength
    )
{
    char buf[80] = {0};
    char *cur = buf;

    sprintf_s(cur, sizeof(buf) - (cur - buf), "|");
    cur += 1;

    for (size_t i = 0; i < 16; i++)
    {
        if (i < aLength)
        {
            sprintf_s(cur, sizeof(buf) - (cur - buf), " %02X", aBuf[i]);
            cur += 3;
        }
        else
        {
            sprintf_s(cur, sizeof(buf) - (cur - buf), " ..");
            cur += 3;
        }

        if (!((i + 1) % 8))
        {
            sprintf_s(cur, sizeof(buf) - (cur - buf), " |");
            cur += 2;
        }
    }

    sprintf_s(cur, sizeof(buf) - (cur - buf), " ");
    cur += 1;

    for (size_t i = 0; i < 16; i++)
    {
        if (i < aLength && isprint(0x7f & aBuf[i]))
        {
            char c = 0x7f & aBuf[i];
            sprintf_s(cur, sizeof(buf) - (cur - buf), "%c", c);
            cur += 1;
        }
        else
        {
            sprintf_s(cur, sizeof(buf) - (cur - buf), ".");
            cur += 1;
        }
    }

    LogVerbose(DRIVER_DEFAULT, "%s", buf);
}

VOID 
DumpBuffer(
    _In_reads_bytes_(aLength) PCUCHAR aBuf, 
    _In_ size_t aLength
    )
{
    for (size_t i = 0; i < aLength; i += 16)
    {
        DumpLine(aBuf + i, (aLength - i) < 16 ? (aLength - i) : 16);
    }
}
