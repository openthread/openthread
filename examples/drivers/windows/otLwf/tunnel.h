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

//
// Initialization
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfTunInitialize(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfTunUninitialize(
    _In_ PMS_FILTER pFilter
    );

//
// Irp Commands
//

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
    );

//
// Value Callbacks
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfTunValueIs(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
void
otLwfTunValueInserted(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ spinel_prop_key_t key,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len
    );

// Called in response to receiving a Spinel Ip6 packet command
_IRQL_requires_max_(DISPATCH_LEVEL)
void 
otLwfTunReceiveIp6Packet(
    _In_ PMS_FILTER pFilter,
    _In_ BOOLEAN DispatchLevel,
    _In_ BOOLEAN Secure,
    _In_reads_bytes_(BufferLength) const uint8_t* Buffer,
    _In_ UINT BufferLength
    );

#endif  //_TUNNEL_H_
