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
 *  This file defines the functions for the otLwf Filter Tunnel mode.
 */

#ifndef _TUNNEL_H_
#define _TUNNEL_H_

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(SPINEL_CMD_HANDLER)(
    _In_ PMS_FILTER pFilter,
    _In_ PVOID Context,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength
    );

typedef struct _SPINEL_CMD_HANDLER_ENTRY
{
    LIST_ENTRY          Link;
    SPINEL_CMD_HANDLER *Handler;
    PVOID               Context;
    spinel_tid_t        TransactionId;
} SPINEL_CMD_HANDLER_ENTRY;

//
// Initialization Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfInitializeTunnelMode(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfUninitializeTunnelMode(
    _In_ PMS_FILTER pFilter
    );

//
// Spinel Packet Functions
//

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
otLwfCancelCommandHandler(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_tid_t tid
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSendTunnelCommandWithHandlerV(
    _In_ PMS_FILTER pFilter,
    _In_opt_ SPINEL_CMD_HANDLER *Handler,
    _In_opt_ PVOID HandlerContext,
    _Out_opt_ spinel_tid_t *pTid,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    _In_opt_ va_list args
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSendTunnelCommandWithHandler(
    _In_ PMS_FILTER pFilter,
    _In_opt_ SPINEL_CMD_HANDLER *Handler,
    _In_opt_ PVOID HandlerContext,
    _Out_opt_ spinel_tid_t *pTid,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    ...
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfSendTunnelPacket(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ PNET_BUFFER IpNetBuffer,
    _In_ BOOLEAN Secured
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void 
otLwfReceiveTunnelPacket(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_reads_bytes_(BufferLength) const PUCHAR Buffer,
    _In_ ULONG BufferLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void 
otLwfProcessSpinelIPv6Packet(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ BOOLEAN Secure,
    _In_reads_bytes_(BufferLength) const uint8_t* Buffer,
    _In_ UINT BufferLength
    );

//
// Irp Commands
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSendTunnelCommandForIrp(
    _In_ PMS_FILTER pFilter,
    _In_ PIRP Irp,
    _In_opt_ SPINEL_IRP_CMD_HANDLER *Handler,
    _In_ UINT Command,
    _In_ spinel_prop_key_t Key,
    _In_ ULONG MaxDataLength,
    _In_opt_ const char *pack_format, 
    ...
    );

//
// Spinel Helper Functions
//

ThreadError
SpinelStatusToThreadError(
    spinel_status_t error
    );

BOOLEAN
try_spinel_datatype_unpack(
    const uint8_t *data_in,
    spinel_size_t data_len,
    const char *pack_format,
    ...
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfGetTunProp(
    _In_ PMS_FILTER pFilter,
    _Out_opt_ PVOID *DataBuffer,
    _In_ spinel_prop_key_t Key,
    _In_ const char *pack_format, 
    ...
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSetTunProp(
    _In_ PMS_FILTER pFilter,
    _In_ spinel_prop_key_t Key,
    _In_ const char *pack_format, 
    ...
    );

#endif  //_TUNNEL_H_
