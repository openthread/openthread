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

#include "precomp.h"
#include "oid.tmh"

_Use_decl_annotations_
NDIS_STATUS
FilterOidRequest(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request
    )
/*++

Routine Description:

    Request handler
    Handle requests from upper layers

Arguments:

    FilterModuleContext   - our filter
    Request               - the request passed down


Return Value:

     NDIS_STATUS_SUCCESS
     NDIS_STATUS_PENDING
     NDIS_STATUS_XXX

NOTE: Called at <= DISPATCH_LEVEL  (unlike a miniport's MiniportOidRequest)

--*/
{
    PMS_FILTER              pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS             Status;
    PNDIS_OID_REQUEST       ClonedRequest=NULL;
    BOOLEAN                 bSubmitted = FALSE;
    POTLWF_REQUEST_CONTEXT Context;
    BOOLEAN                 bFalse = FALSE;

    LogFuncEntryMsg(DRIVER_OID, "Request %p", Request);

    //
    // Most of the time, a filter will clone the OID request and pass down
    // the clone.  When the clone completes, the filter completes the original
    // OID request.
    //
    // If your filter needs to modify a specific request, it can modify the
    // request before or after sending down the cloned request.  Or, it can
    // complete the original request on its own without sending down any
    // clone at all.
    //
    // If your filter driver does not need to modify any OID requests, then
    // you may simply omit this routine entirely; NDIS will pass OID requests
    // down on your behalf.  This is more efficient than implementing a 
    // routine that does nothing but clone all requests, as in the sample here.
    //

    do
    {
        Status = NdisAllocateCloneOidRequest(pFilter->FilterHandle,
                                            Request,
                                            OTLWF_CLONED_OID_TAG,
                                            &ClonedRequest);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogWarning(DRIVER_OID, "Failed to Clone Request, %!NDIS_STATUS!", Status);
            break;
        }

        Context = (POTLWF_REQUEST_CONTEXT)(&ClonedRequest->SourceReserved[0]);
        *Context = Request;

        bSubmitted = TRUE;

        //
        // Use same request ID
        //
        ClonedRequest->RequestId = Request->RequestId;

        pFilter->PendingOidRequest = ClonedRequest;

        LogVerbose(DRIVER_OID, "Sending (cloned) Oid Request %p", ClonedRequest);

        Status = NdisFOidRequest(pFilter->FilterHandle, ClonedRequest);

        if (Status != NDIS_STATUS_PENDING)
        {
            FilterOidRequestComplete(pFilter, ClonedRequest, Status);
            Status = NDIS_STATUS_PENDING;
        }

    } while(bFalse);

    if (bSubmitted == FALSE)
    {
        switch(Request->RequestType)
        {
            case NdisRequestMethod:
                Request->DATA.METHOD_INFORMATION.BytesRead = 0;
                Request->DATA.METHOD_INFORMATION.BytesNeeded = 0;
                Request->DATA.METHOD_INFORMATION.BytesWritten = 0;
                break;

            case NdisRequestSetInformation:
                Request->DATA.SET_INFORMATION.BytesRead = 0;
                Request->DATA.SET_INFORMATION.BytesNeeded = 0;
                break;

            case NdisRequestQueryInformation:
            case NdisRequestQueryStatistics:
            default:
                Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
                Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
                break;
        }

    }

    LogFuncExitNDIS(DRIVER_OID, Status);

    return Status;
}

_Use_decl_annotations_
VOID
FilterCancelOidRequest(
    NDIS_HANDLE             FilterModuleContext,
    PVOID                   RequestId
    )
/*++

Routine Description:

    Cancels an OID request

    If your filter driver does not intercept and hold onto any OID requests,
    then you do not need to implement this routine.  You may simply omit it.
    Furthermore, if the filter only holds onto OID requests so it can pass
    down a clone (the most common case) the filter does not need to implement
    this routine; NDIS will then automatically request that the lower-level
    filter/miniport cancel your cloned OID.

    Most filters do not need to implement this routine.

Arguments:

    FilterModuleContext   - our filter
    RequestId             - identifies the request(s) to cancel

--*/
{
    PMS_FILTER                          pFilter = (PMS_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   Request = NULL;
    POTLWF_REQUEST_CONTEXT              Context;
    PNDIS_OID_REQUEST                   OriginalRequest = NULL;
    BOOLEAN                             bFalse = FALSE;
    
    LogFuncEntryMsg(DRIVER_OID, "Filter: %p, RequestId: %p", FilterModuleContext, RequestId);

    FILTER_ACQUIRE_LOCK(&pFilter->PendingOidRequestLock, bFalse);

    Request = pFilter->PendingOidRequest;

    if (Request != NULL)
    {
        Context = (POTLWF_REQUEST_CONTEXT)(&Request->SourceReserved[0]);

        OriginalRequest = (*Context);
    }

    if ((OriginalRequest != NULL) && (OriginalRequest->RequestId == RequestId))
    {
        FILTER_RELEASE_LOCK(&pFilter->PendingOidRequestLock, bFalse);

        NdisFCancelOidRequest(pFilter->FilterHandle, RequestId);
    }
    else
    {
        FILTER_RELEASE_LOCK(&pFilter->PendingOidRequestLock, bFalse);
    }
    
    LogFuncExit(DRIVER_OID);
}

_Use_decl_annotations_
VOID
FilterOidRequestComplete(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request,
    NDIS_STATUS         Status
    )
/*++

Routine Description:

    Notification that an OID request has been completed

    If this filter sends a request down to a lower layer, and the request is
    pended, the FilterOidRequestComplete routine is invoked when the request
    is complete.  Most requests we've sent are simply clones of requests
    received from a higher layer; all we need to do is complete the original
    higher request.

    However, if this filter driver sends original requests down, it must not
    attempt to complete a pending request to the higher layer.

Arguments:

    FilterModuleContext   - our filter context area
    NdisRequest           - the completed request
    Status                - completion status

--*/
{
    PMS_FILTER                          pFilter = (PMS_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   OriginalRequest;
    POTLWF_REQUEST_CONTEXT              Context;
    BOOLEAN                             bFalse = FALSE;

    LogFuncEntryMsg(DRIVER_OID, "Filter: %p, Request %p", FilterModuleContext, Request);

    Context = (POTLWF_REQUEST_CONTEXT)(&Request->SourceReserved[0]);
    OriginalRequest = (*Context);

    //
    // This is an internal request
    //
    if (OriginalRequest == NULL)
    {
        otLwfInternalRequestComplete(pFilter, Request, Status);
        LogFuncExit(DRIVER_OID);
        return;
    }

    FILTER_ACQUIRE_LOCK(&pFilter->PendingOidRequestLock, bFalse);

    ASSERT(pFilter->PendingOidRequest == Request);
    pFilter->PendingOidRequest = NULL;

    FILTER_RELEASE_LOCK(&pFilter->PendingOidRequestLock, bFalse);

    //
    // Copy the information from the returned request to the original request
    //
    switch(Request->RequestType)
    {
        case NdisRequestMethod:
            OriginalRequest->DATA.METHOD_INFORMATION.OutputBufferLength =  Request->DATA.METHOD_INFORMATION.OutputBufferLength;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesRead = Request->DATA.METHOD_INFORMATION.BytesRead;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesNeeded = Request->DATA.METHOD_INFORMATION.BytesNeeded;
            OriginalRequest->DATA.METHOD_INFORMATION.BytesWritten = Request->DATA.METHOD_INFORMATION.BytesWritten;
            break;

        case NdisRequestSetInformation:
            OriginalRequest->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.BytesRead;
            OriginalRequest->DATA.SET_INFORMATION.BytesNeeded = Request->DATA.SET_INFORMATION.BytesNeeded;
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
        default:
            OriginalRequest->DATA.QUERY_INFORMATION.BytesWritten = Request->DATA.QUERY_INFORMATION.BytesWritten;
            OriginalRequest->DATA.QUERY_INFORMATION.BytesNeeded = Request->DATA.QUERY_INFORMATION.BytesNeeded;
            break;
    }


    (*Context) = NULL;

    LogVerbose(DRIVER_OID, "Freeing (cloned) Oid Request %p", Request);

    NdisFreeCloneOidRequest(pFilter->FilterHandle, Request);

    LogVerbose(DRIVER_OID, "Completing (external) Oid Request %p", OriginalRequest);

    NdisFOidRequestComplete(pFilter->FilterHandle, OriginalRequest, Status);

    LogFuncExit(DRIVER_OID);
}

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
    )
/*++

Routine Description:

    Utility routine that forms and sends an NDIS_OID_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the filter handle until this returns.

Arguments:

    FilterModuleContext - pointer to our filter module context
    RequestType - NdisRequest[Set|Query]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    OutputBufferLength  - valid only for method request
    MethodId - valid only for method request
    pBytesProcessed - place to return bytes read/written

Return Value:

    Status of the set/query request

--*/
{
    LogFuncEntryMsg(DRIVER_OID, "Filter: %p, OID = 0x%u", FilterModuleContext, Oid);

    // Build the request structure
    OTLWF_REQUEST FilterRequest = { 0 };
    FilterRequest.Signature = OTLWF_REQUEST_TAG;
    FilterRequest.Request.Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
    FilterRequest.Request.Header.Revision = NDIS_OID_REQUEST_REVISION_1;
    FilterRequest.Request.Header.Size = sizeof(NDIS_OID_REQUEST);
    FilterRequest.Request.RequestType = RequestType;
    FilterRequest.Request.RequestId = (PVOID)OTLWF_REQUEST_ID;
    FilterRequest.FreeOnCompletion = FALSE;
    FilterRequest.Callback = otLwfInternalSyncRequestComplete;
    NdisInitializeEvent(&FilterRequest.ReqEvent);

    *pBytesProcessed = 0;

    switch (RequestType)
    {
        case NdisRequestQueryInformation:
            FilterRequest.Request.DATA.QUERY_INFORMATION.Oid = Oid;
            FilterRequest.Request.DATA.QUERY_INFORMATION.InformationBuffer = InformationBuffer;
            FilterRequest.Request.DATA.QUERY_INFORMATION.InformationBufferLength = InformationBufferLength;
            break;

        case NdisRequestSetInformation:
            FilterRequest.Request.DATA.SET_INFORMATION.Oid = Oid;
            FilterRequest.Request.DATA.SET_INFORMATION.InformationBuffer = InformationBuffer;
            FilterRequest.Request.DATA.SET_INFORMATION.InformationBufferLength = InformationBufferLength;
            break;

        default:
            NT_ASSERT(FALSE);
            break;
    }
    
    LogVerbose(DRIVER_OID, "Sending (internal, sync) Oid Request %p", &FilterRequest.Request);

    // Send off the OID request
    NT_ASSERT(FilterRequest.Signature == OTLWF_REQUEST_TAG);
    NDIS_STATUS Status = NdisFOidRequest(FilterModuleContext->FilterHandle, &FilterRequest.Request);

    // If pending, wait for completion
    if (Status == NDIS_STATUS_PENDING)
    {
        NdisWaitEvent(&FilterRequest.ReqEvent, 0);
        Status = FilterRequest.Status;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (RequestType == NdisRequestSetInformation)
        {
            *pBytesProcessed = FilterRequest.Request.DATA.SET_INFORMATION.BytesRead;
        }

        if (RequestType == NdisRequestQueryInformation)
        {
            *pBytesProcessed = FilterRequest.Request.DATA.QUERY_INFORMATION.BytesWritten;
        }
    }

    LogFuncExitNDIS(DRIVER_OID, Status);

    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS
otLwfSendInternalRequestAsync(
    _In_     PMS_FILTER                 FilterModuleContext,
    _In_     NDIS_REQUEST_TYPE              RequestType,
    _In_     NDIS_OID                       Oid,
    _Inout_updates_(InformationBufferLength)
             PVOID                          InformationBuffer,
    _In_     ULONG                          InformationBufferLength,
    _In_opt_ otLwfInternalRequestCallback  Callback
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    LogFuncEntryMsg(DRIVER_OID, "OID = 0x%u", Oid);

    // Build the request structure
    POTLWF_REQUEST_ASYNC FilterRequest = FILTER_ALLOC_MEM(FilterModuleContext->FilterHandle, sizeof(OTLWF_REQUEST_ASYNC));
    if (FilterRequest == NULL)
    {
        Status = NDIS_STATUS_RESOURCES;
        LogWarning(DRIVER_OID, "Failed to allocate async internal request structure");
        goto error;
    }
    RtlZeroMemory(FilterRequest, sizeof(OTLWF_REQUEST_ASYNC));
    FilterRequest->Signature = OTLWF_ASYNC_REQUEST_TAG;

    FilterRequest->Request.Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
    FilterRequest->Request.Header.Revision = NDIS_OID_REQUEST_REVISION_1;
    FilterRequest->Request.Header.Size = sizeof(NDIS_OID_REQUEST);
    FilterRequest->Request.RequestType = RequestType;
    FilterRequest->Request.RequestId = (PVOID)OTLWF_REQUEST_ID;
    FilterRequest->FreeOnCompletion = TRUE;
    FilterRequest->Callback = Callback;

    switch (RequestType)
    {
        case NdisRequestQueryInformation:
            FilterRequest->Request.DATA.QUERY_INFORMATION.Oid = Oid;
            FilterRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer = InformationBuffer;
            FilterRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength = InformationBufferLength;
            break;

        case NdisRequestSetInformation:
            FilterRequest->Request.DATA.SET_INFORMATION.Oid = Oid;
            FilterRequest->Request.DATA.SET_INFORMATION.InformationBuffer = InformationBuffer;
            FilterRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = InformationBufferLength;
            break;

        default:
            NT_ASSERT(FALSE);
            break;
    }

    LogVerbose(DRIVER_OID, "Sending (internal, async) Oid Request %p", &FilterRequest->Request);

    // Send the OID request off
    NT_ASSERT(FilterRequest->Signature == OTLWF_ASYNC_REQUEST_TAG);
    Status = NdisFOidRequest(FilterModuleContext->FilterHandle, &FilterRequest->Request);

    // Treat pending as success
    if (Status == NDIS_STATUS_PENDING)
    {
        Status = NDIS_STATUS_SUCCESS;
    }

error:

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisFreeMemory(FilterRequest, 0, 0);
    }

    LogFuncExitNDIS(DRIVER_OID, Status);

    return Status;
}

// Callback for synchronous OID requests
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfInternalSyncRequestComplete(
    _In_ PMS_FILTER                 FilterModuleContext,
    _In_ PNDIS_OID_REQUEST          NdisRequest,
    _In_ NDIS_STATUS                Status
    )
{
    //  Get at the request context.
    POTLWF_REQUEST FilterRequest = CONTAINING_RECORD(NdisRequest, OTLWF_REQUEST, Request);

    UNREFERENCED_PARAMETER(FilterModuleContext);

    //  Save away the completion status.
    FilterRequest->Status = Status;

    LogVerbose(DRIVER_OID, "Setting completion event for (internal, sync) Oid Request %p", NdisRequest);

    //  Wake up the thread blocked for this request to complete.
    NdisSetEvent(&FilterRequest->ReqEvent);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfInternalRequestComplete(
    _In_ NDIS_HANDLE                  FilterModuleContext,
    _In_ PNDIS_OID_REQUEST            NdisRequest,
    _In_ NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point indicating completion of a pended NDIS_OID_REQUEST.

Arguments:

    FilterModuleContext - pointer to filter module context
    NdisRequest - pointer to NDIS request
    Status - status of request completion

Return Value:

    None

--*/
{
    //  Get at the request context.
    POTLWF_REQUEST_ASYNC FilterRequest = CONTAINING_RECORD(NdisRequest, OTLWF_REQUEST_ASYNC, Request);

    NT_ASSERT(FilterRequest->Signature == OTLWF_REQUEST_TAG || FilterRequest->Signature == OTLWF_ASYNC_REQUEST_TAG);
    if (FilterRequest->Signature != OTLWF_REQUEST_TAG && FilterRequest->Signature != OTLWF_ASYNC_REQUEST_TAG) return;
    FilterRequest->Signature = (ULONG)(-1); // Prevent further calls into here

    BOOLEAN FreeOnCompletion = FilterRequest->FreeOnCompletion;

    // Invoke the callback or free the payload if there is no callback
    if (FilterRequest->Callback)
    {
        LogVerbose(DRIVER_OID, "Invoking callback for (internal) Oid Request %p", NdisRequest);

        FilterRequest->Callback((PMS_FILTER)FilterModuleContext, NdisRequest, Status);
    }
    else
    {
        switch (NdisRequest->RequestType)
        {
        case NdisRequestQueryInformation:
            NdisFreeMemory(NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer, 0, 0);
            break;

        case NdisRequestSetInformation:
            NdisFreeMemory(NdisRequest->DATA.SET_INFORMATION.InformationBuffer, 0, 0);
            break;

        default:
            NT_ASSERT(FALSE);
            break;
        }
    }

    // Free the request if necessary
    if (FreeOnCompletion)
    {
        LogVerbose(DRIVER_OID, "Freeing (internal) Oid Request %p", NdisRequest);

        NdisFreeMemory(FilterRequest, 0, 0);
    }
}
