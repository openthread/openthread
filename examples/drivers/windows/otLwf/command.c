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
 *  This file implements the functions for sending/receiving Spinel commands to the miniport.
 */

#include "precomp.h"
#include "command.tmh"

typedef struct _SPINEL_CMD_HANDLER_ENTRY
{
    LIST_ENTRY          Link;
    volatile LONG       RefCount;
    SPINEL_CMD_HANDLER *Handler;
    PVOID               Context;
    spinel_tid_t        TransactionId;
} SPINEL_CMD_HANDLER_ENTRY;

void AddEntryRef(SPINEL_CMD_HANDLER_ENTRY *pEntry) { InterlockedIncrement(&pEntry->RefCount); }
void ReleaseEntryRef(SPINEL_CMD_HANDLER_ENTRY *pEntry) 
{ 
    if (InterlockedDecrement(&pEntry->RefCount) == 0)
    {
        FILTER_FREE_MEM(pEntry);
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfCmdInitialize(
    _In_ PMS_FILTER pFilter
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    uint32_t MajorVersion = 0;
    uint32_t MinorVersion = 0;
    uint32_t InterfaceType = 0;

    NET_BUFFER_LIST_POOL_PARAMETERS PoolParams =
    {
        { NDIS_OBJECT_TYPE_DEFAULT, NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1, NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1 },
        NDIS_PROTOCOL_ID_DEFAULT,
        TRUE,
        0,
        'lbNC', // CNbl
        0
    };

    LogFuncEntry(DRIVER_DEFAULT);

    do
    {
        pFilter->cmdTIDsInUse = 0;
        pFilter->cmdNextTID = 1;
        pFilter->cmdResetReason = OT_PLAT_RESET_REASON_POWER_ON;

        NdisAllocateSpinLock(&pFilter->cmdLock);
        InitializeListHead(&pFilter->cmdHandlers);

        KeInitializeEvent(
            &pFilter->cmdResetCompleteEvent,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );

        // Enable rundown protection
        ExReInitializeRundownProtection(&pFilter->cmdRundown);

        // Create the NDIS pool for creating the SendNetBufferList
        pFilter->cmdNblPool = NdisAllocateNetBufferListPool(pFilter->FilterHandle, &PoolParams);
        if (pFilter->cmdNblPool == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            LogWarning(DRIVER_DEFAULT, "Failed to create NetBufferList pool for Spinel commands");
            break;
        }

        // Query the interface type to make sure it is a Thread device
#ifdef COMMAND_INIT_RETRY
        pFilter->cmdInitTryCount = 0;
        while (pFilter->cmdInitTryCount < 10)
        {
            NtStatus = otLwfCmdGetProp(pFilter, NULL, SPINEL_PROP_PROTOCOL_VERSION, "ii", &MajorVersion, &MinorVersion);
            if (!NT_SUCCESS(NtStatus))
            {
                pFilter->cmdInitTryCount++;
                NdisMSleep(100);
                continue;
            }
            break;
        }
        if (pFilter->cmdInitTryCount >= 10)
        {
#else
        NtStatus = otLwfCmdGetProp(pFilter, NULL, SPINEL_PROP_PROTOCOL_VERSION, "ii", &MajorVersion, &MinorVersion);
        if (!NT_SUCCESS(NtStatus))
        {
#endif
            Status = NDIS_STATUS_NOT_SUPPORTED;
            LogError(DRIVER_DEFAULT, "Failed to query SPINEL_PROP_PROTOCOL_VERSION, %!STATUS!", NtStatus);
            break;
        }
        if (MajorVersion != SPINEL_PROTOCOL_VERSION_THREAD_MAJOR ||
            MinorVersion < 3) // TODO - Remove this minor version check with the next major version update
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            LogError(DRIVER_DEFAULT, "Protocol Version Mismatch! OsVer: %d.%d DeviceVer: %d.%d",
                     SPINEL_PROTOCOL_VERSION_THREAD_MAJOR, SPINEL_PROTOCOL_VERSION_THREAD_MINOR,
                     MajorVersion, MinorVersion);
            break;
        }

        NtStatus = otLwfCmdGetProp(pFilter, NULL, SPINEL_PROP_INTERFACE_TYPE, SPINEL_DATATYPE_UINT_PACKED_S, &InterfaceType);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            LogError(DRIVER_DEFAULT, "Failed to query SPINEL_PROP_INTERFACE_TYPE, %!STATUS!", NtStatus);
            break;
        }
        if (InterfaceType != SPINEL_PROTOCOL_TYPE_THREAD)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            LogError(DRIVER_DEFAULT, "SPINEL_PROP_INTERFACE_TYPE is invalid, %d", InterfaceType);
            break;
        }

        NtStatus = otLwfCmdResetDevice(pFilter, FALSE);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

    } while (FALSE);

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    // Clean up on failure
    if (Status != NDIS_STATUS_SUCCESS)
    {
        otLwfCmdUninitialize(pFilter);
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfCmdUninitialize(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    // Release and wait for run down. This will block waiting for any pending sends to complete
    ExWaitForRundownProtectionRelease(&pFilter->cmdRundown);

    // Use the NBL Pool variable as a flag for initialization
    if (pFilter->cmdNblPool)
    {
        // Clean up any pending handlers
        PLIST_ENTRY Link = pFilter->cmdHandlers.Flink;
        while (Link != &pFilter->cmdHandlers)
        {
            SPINEL_CMD_HANDLER_ENTRY* pEntry = CONTAINING_RECORD(Link, SPINEL_CMD_HANDLER_ENTRY, Link);
            Link = Link->Flink;

            if (pEntry->Handler)
            {
                pEntry->Handler(pFilter, pEntry->Context, 0, 0, NULL, 0);
            }

            ReleaseEntryRef(pEntry);
        }
        InitializeListHead(&pFilter->cmdHandlers);

        // Free NBL Pool
        NdisFreeNetBufferPool(pFilter->cmdNblPool);
        pFilter->cmdNblPool = NULL;
    }

    LogFuncExit(DRIVER_DEFAULT);
}

//
// Receive Spinel Encoded Command
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfCmdProcess(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ UINT command,
    _In_reads_bytes_(cmd_data_len) const uint8_t* cmd_data_ptr,
    _In_ spinel_size_t cmd_data_len
    )
{
    uint8_t Header;
    spinel_prop_key_t key;
    uint8_t* value_data_ptr = NULL;
    spinel_size_t value_data_len = 0;

    // Make sure it's an expected command
    if (command < SPINEL_CMD_PROP_VALUE_IS || command > SPINEL_CMD_PROP_VALUE_REMOVED)
    {
        LogVerbose(DRIVER_DEFAULT, "Recieved unhandled command, %u", command);
        return;
    }

    // Decode the key and data
    if (spinel_datatype_unpack(cmd_data_ptr, cmd_data_len, "CiiD", &Header, NULL, &key, &value_data_ptr, &value_data_len) == -1)
    {
        LogVerbose(DRIVER_DEFAULT, "Failed to unpack command key & data");
        return;
    }

    // Get the transaction ID
    if (SPINEL_HEADER_GET_TID(Header) == 0)
    {
        // Handle out of band last status locally
        if (command == SPINEL_CMD_PROP_VALUE_IS && key == SPINEL_PROP_LAST_STATUS)
        {
            // Check if this is a reset
            spinel_status_t status = SPINEL_STATUS_OK;
            spinel_datatype_unpack(value_data_ptr, value_data_len, "i", &status);

            if ((status >= SPINEL_STATUS_RESET__BEGIN) && (status <= SPINEL_STATUS_RESET__END))
            {
                LogInfo(DRIVER_DEFAULT, "Interface %!GUID! was reset (status %d).", &pFilter->InterfaceGuid, status);
                pFilter->cmdResetReason = status - SPINEL_STATUS_RESET__BEGIN;
                KeSetEvent(&pFilter->cmdResetCompleteEvent, IO_NO_INCREMENT, FALSE);

                // TODO - Should this be passed on to Thread or Tunnel logic?
            }
        }
        else if (ExAcquireRundownProtection(&pFilter->ExternalRefs))
        {
            // If this is a 'Value Is' command, process it for notification of state changes.
            if (command == SPINEL_CMD_PROP_VALUE_IS)
            {
                if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
                {
                    otLwfThreadValueIs(pFilter, DispatchLevel, key, value_data_ptr, value_data_len);
                }
                else if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_THREAD_MODE)
                {
                    otLwfTunValueIs(pFilter, DispatchLevel, key, value_data_ptr, value_data_len);
                }
            }
            else if (command == SPINEL_CMD_PROP_VALUE_INSERTED)
            {
                if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
                {
                    otLwfThreadValueInserted(pFilter, DispatchLevel, key, value_data_ptr, value_data_len);
                }
                else if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_THREAD_MODE)
                {
                    otLwfTunValueInserted(pFilter, DispatchLevel, key, value_data_ptr, value_data_len);
                }
            }

            ExReleaseRundownProtection(&pFilter->ExternalRefs);
        }
    }
    // If there was a transaction ID, then look for the corresponding command handler
    else
    {
        PLIST_ENTRY Link;
        SPINEL_CMD_HANDLER_ENTRY* Handler = NULL;

        FILTER_ACQUIRE_LOCK(&pFilter->cmdLock, DispatchLevel);

        // Search for matching handlers for this command
        Link = pFilter->cmdHandlers.Flink;
        while (Link != &pFilter->cmdHandlers)
        {
            SPINEL_CMD_HANDLER_ENTRY* pEntry = CONTAINING_RECORD(Link, SPINEL_CMD_HANDLER_ENTRY, Link);
            Link = Link->Flink;

            if (SPINEL_HEADER_GET_TID(Header) == pEntry->TransactionId)
            {
                // Remove from the main list
                RemoveEntryList(&pEntry->Link);

                // Cache the handler
                Handler = pEntry;

                // Remove the transaction ID from the 'in use' bit field
                pFilter->cmdTIDsInUse &= ~(1 << pEntry->TransactionId);

                break;
            }
        }

        FILTER_RELEASE_LOCK(&pFilter->cmdLock, DispatchLevel);

        // TODO - Set event

        // Process the handler we found, outside the lock
        if (Handler)
        {
            // Call the handler function
            Handler->Handler(pFilter, Handler->Context, command, key, value_data_ptr, value_data_len);

            // Free the entry
            ReleaseEntryRef(Handler);
        }
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfCmdRecveive(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_reads_bytes_(BufferLength) const PUCHAR Buffer,
    _In_ ULONG BufferLength
    )
{
    uint8_t Header;
    UINT Command;

    // Unpack the header from the buffer
    if (spinel_datatype_unpack(Buffer, BufferLength, "Ci", &Header, &Command) <= 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Failed to unpack header and command");
        return;
    }

    // Validate the header
    if ((Header & SPINEL_HEADER_FLAG) != SPINEL_HEADER_FLAG)
    {
        LogVerbose(DRIVER_DEFAULT, "Recieved unrecognized frame, header=0x%x", Header);
        return;
    }

    // We only support IID zero for now
    if (SPINEL_HEADER_GET_IID(Header) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Recieved unsupported IID, %u", SPINEL_HEADER_GET_IID(Header));
        return;
    }

    // Process the received command
    otLwfCmdProcess(pFilter, DispatchLevel, Command, Buffer, BufferLength);
}

//
// Send Async Spinel Encoded Command
//

_IRQL_requires_max_(PASSIVE_LEVEL)
spinel_tid_t
otLwfCmdGetNextTID(
    _In_ PMS_FILTER pFilter
    )
{
    spinel_tid_t TID = 0;
    while (TID == 0)
    {
        NdisAcquireSpinLock(&pFilter->cmdLock);

        if (((1 << pFilter->cmdNextTID) & pFilter->cmdTIDsInUse) == 0)
        {
            TID = pFilter->cmdNextTID;
            pFilter->cmdNextTID = SPINEL_GET_NEXT_TID(pFilter->cmdNextTID);
            pFilter->cmdTIDsInUse |= (1 << TID);
        }

        NdisReleaseSpinLock(&pFilter->cmdLock);

        if (TID == 0)
        {
            // TODO - Wait for event
        }
    }
    return TID;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
otLwfCmdAddHandler(
    _In_ PMS_FILTER pFilter,
    _In_ SPINEL_CMD_HANDLER_ENTRY *pEntry
    )
{
    // Get the next transaction ID. This call will block if there are
    // none currently available.
    pEntry->TransactionId = otLwfCmdGetNextTID(pFilter);

    LogFuncEntryMsg(DRIVER_DEFAULT, "tid=%u", (ULONG)pEntry->TransactionId);
    
    NdisAcquireSpinLock(&pFilter->cmdLock);
    
    // Add to the handlers list
    AddEntryRef(pEntry);
    InsertTailList(&pFilter->cmdHandlers, &pEntry->Link);
    
    NdisReleaseSpinLock(&pFilter->cmdLock);

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdEncodeAndSendAsync(
    _In_ PMS_FILTER pFilter,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ spinel_tid_t tid,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    _In_opt_ va_list args
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNET_BUFFER_LIST NetBufferList = NULL;
    PNET_BUFFER NetBuffer = NULL;
    ULONG NetBufferLength = 0;
    PUCHAR DataBuffer = NULL;
    spinel_ssize_t PackedLength;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Cmd=%u Key=%u tid=%u", (ULONG)Command, (ULONG)Key, (ULONG)tid);

    NetBufferList =
        NdisAllocateNetBufferAndNetBufferList(
            pFilter->cmdNblPool,     // PoolHandle
            0,                              // ContextSize
            0,                              // ContextBackFill
            NULL,                           // MdlChain
            0,                              // DataOffset
            0                               // DataLength
            );
    if (NetBufferList == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogWarning(DRIVER_DEFAULT, "Failed to create command NetBufferList");
        goto exit;
    }
        
    // Initialize NetBuffer fields
    NetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
    NET_BUFFER_CURRENT_MDL(NetBuffer) = NULL;
    NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer) = 0;
    NET_BUFFER_DATA_LENGTH(NetBuffer) = 0;
    NET_BUFFER_DATA_OFFSET(NetBuffer) = 0;
    NET_BUFFER_FIRST_MDL(NetBuffer) = NULL;

    // Calculate length of NetBuffer
    NetBufferLength = 16 + MaxDataLength;
    if (NetBufferLength < 64) NetBufferLength = 64;
    
    // Allocate the NetBuffer for NetBufferList
    if (NdisRetreatNetBufferDataStart(NetBuffer, NetBufferLength, 0, NULL) != NDIS_STATUS_SUCCESS)
    {
        NetBuffer = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(DRIVER_DEFAULT, "Failed to allocate NB for command NetBufferList, %u bytes", NetBufferLength);
        goto exit;
    }

    // Get the pointer to the data buffer
    DataBuffer = (PUCHAR)NdisGetDataBuffer(NetBuffer, NetBufferLength, NULL, 1, 0);
    NT_ASSERT(DataBuffer);
    
    // Save the true NetBuffer length in the protocol reserved
    NetBuffer->ProtocolReserved[0] = (PVOID)NetBufferLength;
    NetBuffer->DataLength = 0;
    
    // Save the transaction ID in the protocol reserved
    NetBuffer->ProtocolReserved[1] = (PVOID)tid;

    // Pack the header, command and key
    PackedLength = 
        spinel_datatype_pack(
            DataBuffer, 
            NetBufferLength, 
            "Cii", 
            SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0 | tid, 
            Command, 
            Key);
    if (PackedLength < 0 || PackedLength + NetBuffer->DataLength > NetBufferLength)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    NetBuffer->DataLength += (ULONG)PackedLength;

    // Pack the data (if any)
    if (pack_format)
    {
        PackedLength = 
            spinel_datatype_vpack(
                DataBuffer + NetBuffer->DataLength, 
                NetBufferLength - NetBuffer->DataLength, 
                pack_format, 
                args);
        if (PackedLength < 0 || PackedLength + NetBuffer->DataLength > NetBufferLength)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        NetBuffer->DataLength += (ULONG)PackedLength;
    }

    // Grab a ref for rundown protection
    if (!ExAcquireRundownProtection(&pFilter->cmdRundown))
    {
        status = STATUS_DEVICE_NOT_READY;
        LogWarning(DRIVER_DEFAULT, "Failed to acquire rundown protection");
        goto exit;
    }

    // Send the NBL down
    NdisFSendNetBufferLists(
        pFilter->FilterHandle, 
        NetBufferList, 
        NDIS_DEFAULT_PORT_NUMBER, 
        0);

    // Clear local variable because we don't own the NBL any more
    NetBufferList = NULL;

exit:

    if (NetBufferList)
    {
        if (NetBuffer)
        {
            NetBuffer->DataLength = (ULONG)(ULONG_PTR)NetBuffer->ProtocolReserved[0];
            NdisAdvanceNetBufferDataStart(NetBuffer, NetBuffer->DataLength, TRUE, NULL);
        }
        NdisFreeNetBufferList(NetBufferList);
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdResetDevice(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN fAsync
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    KeResetEvent(&pFilter->cmdResetCompleteEvent);

    NTSTATUS status = otLwfCmdEncodeAndSendAsync(pFilter, SPINEL_CMD_RESET, 0, 0, 0, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Failed to send SPINEL_CMD_RESET, %!STATUS!", status);
    }
    else if (!fAsync)
    {
        // Create the relative (negative) time to wait for 5 seconds
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = -5000 * 10000;

        status = KeWaitForSingleObject(&pFilter->cmdResetCompleteEvent, Executive, KernelMode, FALSE, &Timeout);
        if (status != STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Failed waiting for reset complete, %!STATUS!", status);
            status = STATUS_DEVICE_BUSY;
        }
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdSendAsyncV(
    _In_ PMS_FILTER pFilter,
    _In_opt_ SPINEL_CMD_HANDLER *Handler,
    _In_opt_ PVOID HandlerContext,
    _Out_opt_ spinel_tid_t *pTid,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    _In_opt_ va_list args
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SPINEL_CMD_HANDLER_ENTRY *pEntry = NULL;

    if (pTid) *pTid = 0;

    // Create the handler entry and add it to the list
    if (Handler)
    {
        pEntry = FILTER_ALLOC_MEM(pFilter->FilterHandle, sizeof(SPINEL_CMD_HANDLER_ENTRY));
        if (pEntry == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            LogWarning(DRIVER_DEFAULT, "Failed to allocate handler entry");
            goto exit;
        }

        pEntry->RefCount = 1;
        pEntry->Handler = Handler;
        pEntry->Context = HandlerContext;

        otLwfCmdAddHandler(pFilter, pEntry);

        if (pTid) *pTid = pEntry->TransactionId;
    }
    
    status = otLwfCmdEncodeAndSendAsync(pFilter, Command, Key, pEntry ? pEntry->TransactionId : 0, MaxDataLength, pack_format, args);

    // Remove the handler entry from the list
    if (!NT_SUCCESS(status) && pEntry)
    {
        NdisAcquireSpinLock(&pFilter->cmdLock);
    
        // Remove from the main list
        RemoveEntryList(&pEntry->Link);

        // Remove the transaction ID from the 'in use' bit field
        pFilter->cmdTIDsInUse &= ~(1 << pEntry->TransactionId);

        NdisReleaseSpinLock(&pFilter->cmdLock);

        // TODO - Set event

        ReleaseEntryRef(pEntry);
    }

exit:

    if (pEntry) ReleaseEntryRef(pEntry);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdSendAsync(
    _In_ PMS_FILTER pFilter,
    _In_opt_ SPINEL_CMD_HANDLER *Handler,
    _In_opt_ PVOID HandlerContext,
    _Out_opt_ spinel_tid_t *pTid,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    ...
    )
{
    va_list args;
    va_start(args, pack_format);
    NTSTATUS status = 
        otLwfCmdSendAsyncV(pFilter, Handler, HandlerContext, pTid, Command, Key, MaxDataLength, pack_format, args);
    va_end(args);
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
otLwfCmdCancel(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_tid_t tid
    )
{
    PLIST_ENTRY Link;
    SPINEL_CMD_HANDLER_ENTRY* Handler = NULL;
    BOOLEAN Found = FALSE;

    LogFuncEntryMsg(DRIVER_DEFAULT, "tid=%u", (ULONG)tid);

    FILTER_ACQUIRE_LOCK(&pFilter->cmdLock, DispatchLevel);
    
    // Search for matching handlers for this transaction ID
    Link = pFilter->cmdHandlers.Flink;
    while (Link != &pFilter->cmdHandlers)
    {
        SPINEL_CMD_HANDLER_ENTRY* pEntry = CONTAINING_RECORD(Link, SPINEL_CMD_HANDLER_ENTRY, Link);
        Link = Link->Flink;

        if (tid == pEntry->TransactionId)
        {
            // Remove from the main list
            RemoveEntryList(&pEntry->Link);

            // Save handler to cancel outside lock
            Handler = pEntry;
            Found = TRUE;

            // Remove the transaction ID from the 'in use' bit field
            pFilter->cmdTIDsInUse &= ~(1 << pEntry->TransactionId);

            break;
        }
    }
    
    FILTER_RELEASE_LOCK(&pFilter->cmdLock, DispatchLevel);

    if (Handler)
    {
        // Call the handler function
        Handler->Handler(pFilter, Handler->Context, 0, 0, NULL, 0);

        // Free the entry
        ReleaseEntryRef(Handler);
    }

    LogFuncExitMsg(DRIVER_DEFAULT, "Found=%u", Found);

    return Found;
}

//
// Send Packet/Frame
//

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfCmdSendIp6PacketAsync(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ PNET_BUFFER IpNetBuffer,
    _In_ BOOLEAN Secured
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PNET_BUFFER_LIST NetBufferList = NULL;
    PNET_BUFFER NetBuffer = NULL;
    ULONG NetBufferLength = 0;
    PUCHAR DataBuffer = NULL;
    PUCHAR IpDataBuffer = NULL;
    spinel_ssize_t PackedLength;
    IPV6_HEADER* v6Header;

    NetBufferList =
        NdisAllocateNetBufferAndNetBufferList(
            pFilter->cmdNblPool,            // PoolHandle
            0,                              // ContextSize
            0,                              // ContextBackFill
            NULL,                           // MdlChain
            0,                              // DataOffset
            0                               // DataLength
            );
    if (NetBufferList == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogWarning(DRIVER_DEFAULT, "Failed to create command NetBufferList");
        goto exit;
    }
        
    // Initialize NetBuffer fields
    NetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
    NET_BUFFER_CURRENT_MDL(NetBuffer) = NULL;
    NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer) = 0;
    NET_BUFFER_DATA_LENGTH(NetBuffer) = 0;
    NET_BUFFER_DATA_OFFSET(NetBuffer) = 0;
    NET_BUFFER_FIRST_MDL(NetBuffer) = NULL;

    // Calculate length of NetBuffer
    NetBufferLength = 20 + IpNetBuffer->DataLength;
    if (NetBufferLength < 64) NetBufferLength = 64;
    
    // Allocate the NetBuffer for NetBufferList
    if (NdisRetreatNetBufferDataStart(NetBuffer, NetBufferLength, 0, NULL) != NDIS_STATUS_SUCCESS)
    {
        NetBuffer = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(DRIVER_DEFAULT, "Failed to allocate NB for command NetBufferList, %u bytes", NetBufferLength);
        goto exit;
    }

    // Get the pointer to the data buffer for the header data
    DataBuffer = (PUCHAR)NdisGetDataBuffer(NetBuffer, NetBufferLength, NULL, 1, 0);
    NT_ASSERT(DataBuffer);
    
    // Save the true NetBuffer length in the protocol reserved
    NetBuffer->ProtocolReserved[0] = (PVOID)NetBufferLength;
    NetBuffer->DataLength = 0;

    // Pack the header, command and key
    PackedLength = 
        spinel_datatype_pack(
            DataBuffer, 
            NetBufferLength, 
            "Cii", 
            (spinel_tid_t)(SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0), 
            (UINT)SPINEL_CMD_PROP_VALUE_SET, 
            (Secured ? SPINEL_PROP_STREAM_NET : SPINEL_PROP_STREAM_NET_INSECURE));
    if (PackedLength < 0 || PackedLength + NetBuffer->DataLength > NetBufferLength)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    NT_ASSERT(PackedLength >= 3);
    NetBuffer->DataLength += (ULONG)PackedLength;
    
    // Copy over the data length
    DataBuffer[NetBuffer->DataLength+1] = (((USHORT)IpNetBuffer->DataLength) >> 8) & 0xff;
    DataBuffer[NetBuffer->DataLength]   = (((USHORT)IpNetBuffer->DataLength) >> 0) & 0xff;
    NetBuffer->DataLength += 2;
    
    v6Header = (IPV6_HEADER*)(DataBuffer + NetBuffer->DataLength);

    // Copy the IP packet data
    IpDataBuffer = (PUCHAR)NdisGetDataBuffer(IpNetBuffer, IpNetBuffer->DataLength, v6Header, 1, 0);
    if (IpDataBuffer != (PUCHAR)v6Header)
    {
        RtlCopyMemory(v6Header, IpDataBuffer, IpNetBuffer->DataLength);
    }

    NetBuffer->DataLength += IpNetBuffer->DataLength;

    // Grab a ref for rundown protection
    if (!ExAcquireRundownProtection(&pFilter->cmdRundown))
    {
        status = STATUS_DEVICE_NOT_READY;
        LogWarning(DRIVER_DEFAULT, "Failed to acquire rundown protection");
        goto exit;
    }
                                            
    LogVerbose(DRIVER_DATA_PATH, "Filter: %p, IP6_SEND: %p : %!IPV6ADDR! => %!IPV6ADDR! (%u bytes)", 
                pFilter, NetBufferList, &v6Header->SourceAddress, &v6Header->DestinationAddress, 
                NET_BUFFER_DATA_LENGTH(IpNetBuffer));

    // Send the NBL down
    NdisFSendNetBufferLists(
        pFilter->FilterHandle, 
        NetBufferList, 
        NDIS_DEFAULT_PORT_NUMBER, 
        DispatchLevel ? NDIS_SEND_FLAGS_DISPATCH_LEVEL : 0);

    // Clear local variable because we don't own the NBL any more
    NetBufferList = NULL;

exit:

    if (NetBufferList)
    {
        if (NetBuffer)
        {
            NetBuffer->DataLength = (ULONG)(ULONG_PTR)NetBuffer->ProtocolReserved[0];
            NdisAdvanceNetBufferDataStart(NetBuffer, NetBuffer->DataLength, TRUE, NULL);
        }
        NdisFreeNetBufferList(NetBufferList);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfCmdSendMacFrameComplete(
    _In_ PMS_FILTER pFilter,
    _In_ PVOID Context,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength
    )
{
    UNREFERENCED_PARAMETER(Context);

    pFilter->otLastTransmitError = OT_ERROR_ABORT;

    if (Data && Command == SPINEL_CMD_PROP_VALUE_IS)
    {
        if (Key == SPINEL_PROP_LAST_STATUS)
        {
            spinel_status_t spinel_status = SPINEL_STATUS_OK;
            spinel_ssize_t packed_len = spinel_datatype_unpack(Data, DataLength, "i", &spinel_status);
            if (packed_len > 0)
            {
                if (spinel_status == SPINEL_STATUS_OK)
                {
                    pFilter->otLastTransmitError = OT_ERROR_NONE;
                    (void)spinel_datatype_unpack(
                        Data + packed_len,
                        DataLength - (spinel_size_t)packed_len,
                        "b",
                        &pFilter->otLastTransmitFramePending);
                }
                else
                {
                    pFilter->otLastTransmitError = SpinelStatusToThreadError(spinel_status);
                }
            }
        }
    }

    // Set the completion event
    KeSetEvent(&pFilter->SendNetBufferListComplete, IO_NO_INCREMENT, FALSE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfCmdSendMacFrameAsync(
    _In_ PMS_FILTER     pFilter,
    _In_ otRadioFrame*  Packet
    )
{
    // Reset the completion event
    KeResetEvent(&pFilter->SendNetBufferListComplete);
    pFilter->SendPending = TRUE;

    NTSTATUS status =
        otLwfCmdSendAsync(
            pFilter,
            otLwfCmdSendMacFrameComplete,
            NULL,
            NULL,
            SPINEL_CMD_PROP_VALUE_SET,
            SPINEL_PROP_STREAM_RAW,
            Packet->mLength + 20,
            SPINEL_DATATYPE_DATA_WLEN_S
            SPINEL_DATATYPE_UINT8_S
            SPINEL_DATATYPE_INT8_S,
            Packet->mPsdu,
            (uint32_t)Packet->mLength,
            Packet->mChannel,
            Packet->mInfo.mRxInfo.mRssi
            );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_STREAM_RAW failed, %!STATUS!", status);
        pFilter->otLastTransmitError = OT_ERROR_ABORT;
        KeSetEvent(&pFilter->SendNetBufferListComplete, IO_NO_INCREMENT, FALSE);
    }
}

//
// Send Synchronous Spinel Encoded Command
//

typedef struct _SPINEL_GET_PROP_CONTEXT
{
    KEVENT              CompletionEvent;
    spinel_prop_key_t   Key;
    PVOID              *DataBuffer;
    const char*         Format;
    va_list             Args;
    NTSTATUS            Status;
} SPINEL_GET_PROP_CONTEXT;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfGetPropHandler(
    _In_ PMS_FILTER pFilter,
    _In_ PVOID Context,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength
    )
{
    SPINEL_GET_PROP_CONTEXT* CmdContext = (SPINEL_GET_PROP_CONTEXT*)Context;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Key=%u", (ULONG)Key);

    UNREFERENCED_PARAMETER(pFilter);
    
    if (Data == NULL)
    {
        CmdContext->Status = STATUS_CANCELLED;
    }
    else if (Command != SPINEL_CMD_PROP_VALUE_IS)
    {
        CmdContext->Status = STATUS_INVALID_PARAMETER;
    }
    else if (Key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t spinel_status = SPINEL_STATUS_OK;
        spinel_ssize_t packed_len = spinel_datatype_unpack(Data, DataLength, "i", &spinel_status);
        if (packed_len < 0 || (ULONG)packed_len > DataLength)
        {
            CmdContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            otError errorCode = SpinelStatusToThreadError(spinel_status);
            LogVerbose(DRIVER_DEFAULT, "Get key=%u failed with %!otError!", CmdContext->Key, errorCode);
            CmdContext->Status = ThreadErrorToNtstatus(errorCode);
        }
    }
    else if (Key == CmdContext->Key)
    {
        if (CmdContext->DataBuffer)
        {
            *CmdContext->DataBuffer = FILTER_ALLOC_MEM(pFilter->FilterHandle, DataLength);
            if (*CmdContext->DataBuffer == NULL)
            {
                CmdContext->Status = STATUS_INSUFFICIENT_RESOURCES;
                DataLength = 0;
            }
            else
            {
                memcpy(*CmdContext->DataBuffer, Data, DataLength);
                Data = (uint8_t*)*CmdContext->DataBuffer;
            }
        }

        spinel_ssize_t packed_len = spinel_datatype_vunpack(Data, DataLength, CmdContext->Format, CmdContext->Args);
        if (packed_len < 0 || (ULONG)packed_len > DataLength)
        {
            CmdContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            CmdContext->Status = STATUS_SUCCESS;
        }
    }
    else
    {
        CmdContext->Status = STATUS_INVALID_PARAMETER;
    }

    // Set the completion event
    KeSetEvent(&CmdContext->CompletionEvent, 0, FALSE);

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdGetProp(
    _In_ PMS_FILTER pFilter,
    _Out_opt_ PVOID *DataBuffer,
    _In_ spinel_prop_key_t Key,
    _In_ const char *pack_format, 
    ...
    )
{
    NTSTATUS status;
    LARGE_INTEGER WaitTimeout;
    spinel_tid_t tid;

    // Create the context structure
    SPINEL_GET_PROP_CONTEXT Context;
    KeInitializeEvent(&Context.CompletionEvent, SynchronizationEvent, FALSE);
    Context.Key = Key;
    Context.DataBuffer = DataBuffer;
    Context.Format = pack_format;
    Context.Status = STATUS_SUCCESS;
    va_start(Context.Args, pack_format);

    LogFuncEntryMsg(DRIVER_DEFAULT, "Key=%u", (ULONG)Key);

    // Send the request transaction
    status = 
        otLwfCmdSendAsyncV(
            pFilter, 
            otLwfGetPropHandler, 
            &Context,
            &tid,
            SPINEL_CMD_PROP_VALUE_GET, 
            Key, 
            0, 
            NULL,
            NULL);
    if (NT_SUCCESS(status))
    {
        // Set a 1 second wait timeout
        WaitTimeout.QuadPart = -1000 * 10000;

        // Wait for the response
        if (KeWaitForSingleObject(
                &Context.CompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                &WaitTimeout) != STATUS_SUCCESS)
        {
            if (!otLwfCmdCancel(pFilter, FALSE, tid))
            {
                KeWaitForSingleObject(
                    &Context.CompletionEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);
            }
        }
    }
    else
    {
        Context.Status = status;
    }
    
    va_end(Context.Args);

    LogFuncExitNT(DRIVER_DEFAULT, Context.Status);

    return Context.Status;
}

typedef struct _SPINEL_SET_PROP_CONTEXT
{
    KEVENT              CompletionEvent;
    UINT                ExpectedResultCommand;
    spinel_prop_key_t   Key;
    NTSTATUS            Status;
} SPINEL_SET_PROP_CONTEXT;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfSetPropHandler(
    _In_ PMS_FILTER pFilter,
    _In_ PVOID Context,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength
    )
{
    SPINEL_SET_PROP_CONTEXT* CmdContext = (SPINEL_SET_PROP_CONTEXT*)Context;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Key=%u", (ULONG)Key);

    UNREFERENCED_PARAMETER(pFilter);
    
    if (Data == NULL)
    {
        CmdContext->Status = STATUS_CANCELLED;
    }
    else if (Command == SPINEL_CMD_PROP_VALUE_IS && Key == SPINEL_PROP_LAST_STATUS)
    {
        spinel_status_t spinel_status = SPINEL_STATUS_OK;
        spinel_ssize_t packed_len = spinel_datatype_unpack(Data, DataLength, "i", &spinel_status);
        if (packed_len < 0 || (ULONG)packed_len > DataLength)
        {
            CmdContext->Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            otError errorCode = SpinelStatusToThreadError(spinel_status);
            LogVerbose(DRIVER_DEFAULT, "Set key=%u failed with %!otError!", CmdContext->Key, errorCode);
            CmdContext->Status = ThreadErrorToNtstatus(errorCode);
        }
    }
    else if (Command != CmdContext->ExpectedResultCommand)
    {
        NT_ASSERT(FALSE);
        CmdContext->Status = STATUS_INVALID_PARAMETER;
    }
    else if (Key == CmdContext->Key)
    {
        CmdContext->Status = STATUS_SUCCESS;
    }
    else
    {
        NT_ASSERT(FALSE);
        CmdContext->Status = STATUS_INVALID_PARAMETER;
    }

    // Set the completion event
    KeSetEvent(&CmdContext->CompletionEvent, 0, FALSE);

    LogFuncExit(DRIVER_DEFAULT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdSetPropV(
    _In_ PMS_FILTER pFilter,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_opt_ const char *pack_format,
    _In_opt_ va_list args
    )
{
    NTSTATUS status;
    LARGE_INTEGER WaitTimeout;
    spinel_tid_t tid;

    // Create the context structure
    SPINEL_SET_PROP_CONTEXT Context;
    KeInitializeEvent(&Context.CompletionEvent, SynchronizationEvent, FALSE);
    Context.Key = Key;
    Context.Status = STATUS_SUCCESS;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Cmd=%u Key=%u", Command, (ULONG)Key);

    if (Command == SPINEL_CMD_PROP_VALUE_SET)
    {
        Context.ExpectedResultCommand = SPINEL_CMD_PROP_VALUE_IS;
    }
    else if (Command == SPINEL_CMD_PROP_VALUE_INSERT)
    {
        Context.ExpectedResultCommand = SPINEL_CMD_PROP_VALUE_INSERTED;
    }
    else if (Command == SPINEL_CMD_PROP_VALUE_REMOVE)
    {
        Context.ExpectedResultCommand = SPINEL_CMD_PROP_VALUE_REMOVED;
    }
    else
    {
        ASSERT(FALSE);
    }

    // Send the request transaction
    status = 
        otLwfCmdSendAsyncV(
            pFilter, 
            otLwfSetPropHandler, 
            &Context, 
            &tid,
            Command,
            Key, 
            8, 
            pack_format,
            args);
    if (NT_SUCCESS(status))
    {
        // Set a 1 second wait timeout
        WaitTimeout.QuadPart = -1000 * 10000;

        // Wait for the response
        if (KeWaitForSingleObject(
                &Context.CompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                &WaitTimeout) != STATUS_SUCCESS)
        {
            if (!otLwfCmdCancel(pFilter, FALSE, tid))
            {
                KeWaitForSingleObject(
                    &Context.CompletionEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);
            }
        }
    }
    else
    {
        Context.Status = status;
    }

    LogFuncExitNT(DRIVER_DEFAULT, Context.Status);

    return Context.Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdSetProp(
    _In_ PMS_FILTER pFilter,
    _In_ spinel_prop_key_t Key,
    _In_opt_ const char *pack_format,
    ...
    )
{
    va_list args;
    va_start(args, pack_format);
    NTSTATUS status = otLwfCmdSetPropV(pFilter, SPINEL_CMD_PROP_VALUE_SET, Key, pack_format, args);
    va_end(args);
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdInsertProp(
    _In_ PMS_FILTER pFilter,
    _In_ spinel_prop_key_t Key,
    _In_opt_ const char *pack_format,
    ...
    )
{
    va_list args;
    va_start(args, pack_format);
    NTSTATUS status = otLwfCmdSetPropV(pFilter, SPINEL_CMD_PROP_VALUE_INSERT, Key, pack_format, args);
    va_end(args);
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfCmdRemoveProp(
    _In_ PMS_FILTER pFilter,
    _In_ spinel_prop_key_t Key,
    _In_opt_ const char *pack_format,
    ...
    )
{
    va_list args;
    va_start(args, pack_format);
    NTSTATUS status = otLwfCmdSetPropV(pFilter, SPINEL_CMD_PROP_VALUE_REMOVE, Key, pack_format, args);
    va_end(args);
    return status;
}

//
// General Spinel Helpers
//

otError
SpinelStatusToThreadError(
    spinel_status_t error
    )
{
    otError ret;

    switch (error)
    {
    case SPINEL_STATUS_OK:
        ret = OT_ERROR_NONE;
        break;

    case SPINEL_STATUS_FAILURE:
        ret = OT_ERROR_FAILED;
        break;

    case SPINEL_STATUS_DROPPED:
        ret = OT_ERROR_DROP;
        break;

    case SPINEL_STATUS_NOMEM:
        ret = OT_ERROR_NO_BUFS;
        break;

    case SPINEL_STATUS_BUSY:
        ret = OT_ERROR_BUSY;
        break;

    case SPINEL_STATUS_PARSE_ERROR:
        ret = OT_ERROR_PARSE;
        break;

    case SPINEL_STATUS_INVALID_ARGUMENT:
        ret = OT_ERROR_INVALID_ARGS;
        break;

    case SPINEL_STATUS_UNIMPLEMENTED:
        ret = OT_ERROR_NOT_IMPLEMENTED;
        break;

    case SPINEL_STATUS_INVALID_STATE:
        ret = OT_ERROR_INVALID_STATE;
        break;

    case SPINEL_STATUS_NO_ACK:
        ret = OT_ERROR_NO_ACK;
        break;

    case SPINEL_STATUS_CCA_FAILURE:
        ret = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        break;

    case SPINEL_STATUS_ALREADY:
        ret = OT_ERROR_ALREADY;
        break;

    case SPINEL_STATUS_ITEM_NOT_FOUND:
        ret = OT_ERROR_NOT_FOUND;
        break;

    default:
        if (error >= SPINEL_STATUS_STACK_NATIVE__BEGIN && error <= SPINEL_STATUS_STACK_NATIVE__END)
        {
            ret = (otError)(error - SPINEL_STATUS_STACK_NATIVE__BEGIN);
        }
        else
        {
            ret = OT_ERROR_FAILED;
        }
        break;
    }

    return ret;
}

BOOLEAN
try_spinel_datatype_unpack(
    const uint8_t *data_in,
    spinel_size_t data_len,
    const char *pack_format,
    ...
    )
{
    va_list args;
    va_start(args, pack_format);
    spinel_ssize_t packed_len = spinel_datatype_vunpack(data_in, data_len, pack_format, args);
    va_end(args);

    return !(packed_len < 0 || (spinel_size_t)packed_len > data_len);
}
