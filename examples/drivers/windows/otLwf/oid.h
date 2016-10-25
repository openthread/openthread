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
 *  This file defines the structures and functions for sending and processing
 *  NDIS OIDs.
 */

#ifndef _OID_H
#define _OID_H

// Allocation tag for OID requests sent from the filter, 'TOID'
#define OTLWF_REQUEST_ID          'DIOT'

// Allocation tag for cloned OIDs, 'TCOD'
#define OTLWF_CLONED_OID_TAG      'DOCT'

// The context inside a cloned request
typedef struct _NDIS_OID_REQUEST *OTLWF_REQUEST_CONTEXT, **POTLWF_REQUEST_CONTEXT;

// Callback to set a new list of Keys to the MAC layer
_IRQL_requires_max_(DISPATCH_LEVEL)
typedef VOID
(*otLwfInternalRequestCallback)(
    _In_ PMS_FILTER                 FilterModuleContext,
    _In_ PNDIS_OID_REQUEST          Request,
    _In_ NDIS_STATUS                Status
    );

#define OTLWF_ASYNC_REQUEST_TAG    'aRIT'
#define OTLWF_REQUEST_TAG          'sRIT'

// Used for sending internal OID requests
typedef struct _OTLWF_REQUEST_ASYNC
{
    ULONG                           Signature; // equals OTLWF_ASYNC_REQUEST_TAG or OTLWF_REQUEST_TAG
    NDIS_OID_REQUEST                Request;
    BOOLEAN                         FreeOnCompletion;
    otLwfInternalRequestCallback   Callback;
} OTLWF_REQUEST_ASYNC, *POTLWF_REQUEST_ASYNC;

// Used for sending internal OID requests
typedef struct _OTLWF_REQUEST
{
    OTLWF_REQUEST_ASYNC;
    NDIS_EVENT          ReqEvent;
    NDIS_STATUS         Status;
} OTLWF_REQUEST, *POTLWF_REQUEST;

FILTER_OID_REQUEST FilterOidRequest;

FILTER_CANCEL_OID_REQUEST FilterCancelOidRequest;

FILTER_OID_REQUEST_COMPLETE FilterOidRequestComplete;

// Sends an internal OID request down and waits
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS
otLwfSendInternalRequest(
    _In_     PMS_FILTER                 FilterModuleContext,
    _In_     NDIS_REQUEST_TYPE          RequestType,
    _In_     NDIS_OID                   Oid,
    _Inout_updates_bytes_to_(InformationBufferLength, *pBytesProcessed)
             PVOID                      InformationBuffer,
    _In_     ULONG                      InformationBufferLength,
    _Out_    PULONG                     pBytesProcessed
    );

// Sends an internal OID request down. When complete it fires the callback,
// or if the callback isn't set, frees the InformationBuffer.
_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS
otLwfSendInternalRequestAsync(
    _In_     PMS_FILTER                     FilterModuleContext,
    _In_     NDIS_REQUEST_TYPE              RequestType,
    _In_     NDIS_OID                       Oid,
    _Inout_updates_(InformationBufferLength)
             PVOID                          InformationBuffer,
    _In_     ULONG                          InformationBufferLength,
    _In_opt_ otLwfInternalRequestCallback  Callback
    );

// Callback for synchronous OID requests
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfInternalSyncRequestComplete(
    _In_ PMS_FILTER                 FilterModuleContext,
    _In_ PNDIS_OID_REQUEST          NdisRequest,
    _In_ NDIS_STATUS                Status
    );

// Callback when the internal OID request completes
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfInternalRequestComplete(
    _In_ NDIS_HANDLE                  FilterModuleContext,
    _In_ PNDIS_OID_REQUEST            NdisRequest,
    _In_ NDIS_STATUS                  Status
    );

#endif  //_OID_H


