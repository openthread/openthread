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
#include "iocontrol.tmh"

// Handles queries for the current list of Thread interfaces
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlEnumerateInterfaces(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS        status = STATUS_SUCCESS;
    ULONG           NewOutBufferLength = 0;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure to zero out the output first
    RtlZeroMemory(OutBuffer, *OutBufferLength);

    NdisAcquireSpinLock(&FilterListLock);

    // Make sure there is enough space for the first USHORT
    if (*OutBufferLength < sizeof(USHORT))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    POTLWF_INTERFACE_LIST pInterfaceList = (POTLWF_INTERFACE_LIST)OutBuffer;
    pInterfaceList->cInterfaces = 0;

    for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
    {
        POTLWF_INTERFACE pInterface = &pInterfaceList->pInterfaces[pInterfaceList->cInterfaces];
        pInterfaceList->cInterfaces++;

        PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);
        if (pFilter->State != FilterRunning) continue;

        NewOutBufferLength =
            FIELD_OFFSET(OTLWF_INTERFACE_LIST, pInterfaces) +
            pInterfaceList->cInterfaces * sizeof(OTLWF_INTERFACE);

        if (NewOutBufferLength <= *OutBufferLength)
        {
            pInterface->InterfaceGuid = pFilter->InterfaceGuid;
            pInterface->InterfaceState = 
                IsAttached(pFilter->otCachedRole) ? 
                    OTLWF_INTERFACE_STATE_JOINED : 
                    OTLWF_INTERFACE_STATE_DISCONNECTED;
            pInterface->CompartmentID = pFilter->InterfaceCompartmentID;

            NdisMoveMemory(pInterface->MiniportFriendlyName,
                            (PUCHAR)(pFilter->MiniportFriendlyName.Buffer),
                            pFilter->MiniportFriendlyName.Length);
        }
    }

    if (NewOutBufferLength > *OutBufferLength)
    {
        NewOutBufferLength = sizeof(USHORT);
    }

error:

    NdisReleaseSpinLock(&FilterListLock);

    *OutBufferLength = NewOutBufferLength;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to start a new Thread network
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfNetworkConfigure(
    _In_ PMS_FILTER             pFilter,
    _In_ POTLWF_NETWORK_PARAMS  NetworkParams
    )
{
    ThreadError otError = kThreadError_None;
    otLinkModeConfig linkMode = {0};

    otError = otSetChannel(pFilter->otCtx, (uint8_t)(NetworkParams->Channel + 11));
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otSetChannel failed: %!otError!", otError);
        goto error;
    }

    otError = otSetPanId(pFilter->otCtx, NetworkParams->PANID);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otSetPanId failed: %!otError!", otError);
        goto error;
    }

    otSetExtendedPanId(pFilter->otCtx, NetworkParams->ExtendedPANID);

    otError = otSetNetworkName(pFilter->otCtx, NetworkParams->NetworkName);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otSetNetworkName failed: %!otError!", otError);
        goto error;
    }

    otError = otSetMasterKey(pFilter->otCtx, NetworkParams->MasterKey, 16);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otSetMasterKey failed: %!otError!", otError);
        goto error;
    }

    linkMode.mRxOnWhenIdle = TRUE;
    linkMode.mSecureDataRequests = TRUE;
    if (NetworkParams->RouterEligibleEndDevice)
    {
        linkMode.mDeviceType = TRUE;
        linkMode.mNetworkData = TRUE;
    }
    else
    {
        linkMode.mDeviceType = FALSE;
        linkMode.mNetworkData = FALSE;
    }

    otError = otSetLinkMode(pFilter->otCtx, linkMode);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otSetLinkMode failed: %!otError!", otError);
        goto error;
    }

    if ((NetworkParams->OptionalFlags & OTLWF_NETWORK_PARAM_EXTENDED_MAC_ADDRESS) != 0)
    {
        // TODO ...
    }

    if ((NetworkParams->OptionalFlags & OTLWF_NETWORK_PARAM_MESH_LOCAL_IID) != 0)
    {
        otError = otSetMeshLocalPrefix(pFilter->otCtx, NetworkParams->MeshLocalIID);
        if (otError != kThreadError_None)
        {
            LogError(DRIVER_IOCTL, "otSetMeshLocalPrefix failed: %!otError!", otError);
            goto error;
        }
    }

    if ((NetworkParams->OptionalFlags & OTLWF_NETWORK_PARAM_MAX_CHILDREN) != 0)
    {
        // TODO
    }

error:

    return otError == kThreadError_None ? STATUS_SUCCESS : STATUS_INVALID_DEVICE_REQUEST;
}

// Handles requests to start a new Thread network
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlCreateNetwork(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    POTLWF_NETWORK_PARAMS   NetworkParams = NULL;
    PMS_FILTER              pFilter = NULL;
    ThreadError             otError = kThreadError_None;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room for the OTLWF_NETWORK_PARAMS was passed in
    if (InBufferLength < sizeof(GUID) + sizeof(OTLWF_NETWORK_PARAMS))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }

    NetworkParams = (POTLWF_NETWORK_PARAMS)((PUCHAR)InBuffer + sizeof(GUID));

    // Set the new network configuration
    status = otLwfNetworkConfigure(pFilter, NetworkParams);
    if (!NT_SUCCESS(status))
    {
        goto error;
    }

    // Create the network
    otError = otInterfaceUp(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otInterfaceUp failed: %!otError!", otError);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }
    otError = otThreadStart(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otThreadStart failed: %!otError!", otError);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    // No output
    RtlZeroMemory(OutBuffer, *OutBufferLength);
    *OutBufferLength = 0;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to join an existing Thread network
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlJoinNetwork(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    POTLWF_NETWORK_PARAMS   NetworkParams = NULL;
    PMS_FILTER              pFilter = NULL;
    ThreadError             otError = kThreadError_None;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room for the OTLWF_NETWORK_PARAMS was passed in
    if (InBufferLength < sizeof(GUID) + sizeof(OTLWF_NETWORK_PARAMS))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }

    NetworkParams = (POTLWF_NETWORK_PARAMS)((PUCHAR)InBuffer + sizeof(GUID));

    // Set the new network configuration
    status = otLwfNetworkConfigure(pFilter, NetworkParams);
    if (!NT_SUCCESS(status))
    {
        goto error;
    }

    // Start the joining process
    otError = otInterfaceUp(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otInterfaceUp failed: %!otError!", otError);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }
    otError = otThreadStart(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otThreadStart failed: %!otError!", otError);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    // No output
    RtlZeroMemory(OutBuffer, *OutBufferLength);
    *OutBufferLength = 0;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to force a router ID request to the Thread network
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlSendRouterIDRequest(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    PMS_FILTER              pFilter = NULL;
    ThreadError             otError = kThreadError_None;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room for the Interface GUID was passed in
    if (InBufferLength < sizeof(GUID))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }

    otError = otBecomeRouter(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otBecomeRouter failed: %!otError!", otError);
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    // No output
    RtlZeroMemory(OutBuffer, *OutBufferLength);
    *OutBufferLength = 0;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to disconnect an existing Thread network
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlDisconnectNetwork(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    PMS_FILTER              pFilter = NULL;
    ThreadError             otError = kThreadError_None;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room for the Interface GUID was passed in
    if (InBufferLength < sizeof(GUID))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }
    
    otError = otThreadStop(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otThreadStop failed: %!otError!", otError);
    }
    otError = otInterfaceDown(pFilter->otCtx);
    if (otError != kThreadError_None)
    {
        LogError(DRIVER_IOCTL, "otInterfaceDown failed: %!otError!", otError);
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    // No output
    RtlZeroMemory(OutBuffer, *OutBufferLength);
    *OutBufferLength = 0;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to query the network addresses of an Interface
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlQueryNetworkAddresses(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS                  status = STATUS_SUCCESS;
    PMS_FILTER                pFilter = NULL;
    POTLWF_NETWORK_ADDRESSES  pNetworkAddresses = (POTLWF_NETWORK_ADDRESSES)OutBuffer;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room was passed in
    if (InBufferLength < sizeof(GUID) || *OutBufferLength < sizeof(OTLWF_NETWORK_ADDRESSES))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }

    if (IsAttached(otGetDeviceRole(pFilter->otCtx)))
    {
        // Copy out the parameters
        memcpy(pNetworkAddresses->ExtendedMACAddress, otGetExtendedAddress(pFilter->otCtx), 8);
        RtlZeroMemory(&pNetworkAddresses->LL_EID, sizeof(IN6_ADDR));
        RtlZeroMemory(&pNetworkAddresses->RLOC, sizeof(IN6_ADDR));
        memcpy(&pNetworkAddresses->ML_EID, otGetMeshLocalEid(pFilter->otCtx), sizeof(IN6_ADDR));
    }
    else
    {
        // Not in the joined state
        status = STATUS_DEVICE_NOT_READY;
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(OutBuffer, *OutBufferLength);
    }

    *OutBufferLength = sizeof(OTLWF_NETWORK_ADDRESSES);

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles requests to query the mesh state of an Interface
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlQueryMeshState(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_opt_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PMS_FILTER          pFilter = NULL;
    POTLWF_MESH_STATE   pMeshState = (POTLWF_MESH_STATE)OutBuffer;
    ULONG               NewOutBufferLength = 0;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure enough room was passed in
    if (InBufferLength < sizeof(GUID) || *OutBufferLength < sizeof(OTLWF_MESH_STATE))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Find the filter context by the interface guid
    pFilter = otLwfFindAndRefInterface((PGUID)InBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_NOT_FOUND;
        goto error;
    }
    
    pMeshState->cChildren = 0;
    pMeshState->cNeighbors = 0;
    
    // Only has children and neighbors if it's a router
    /*if (pFilter->Roles.IsRouter())
    {
        if (*OutBufferLength == sizeof(OTLWF_MESH_STATE))
        {
            pMeshState->cChildren = (UCHAR)pFilter->Roles.Router.Children.Size();
            pMeshState->cNeighbors = (UCHAR)pFilter->RoutingData.Neighbors.Size();
            NewOutBufferLength = sizeof(OTLWF_MESH_STATE);       
        }
        else
        {
            NewOutBufferLength = sizeof(OTLWF_MESH_STATE);
            PVOID ContextPtrs[] = { pMeshState, OutBufferLength, &NewOutBufferLength };
            
            // Iterate through the items
            pMeshState->Children = (POTLWF_CHILD_LINK)(pMeshState + 1);
            pFilter->Roles.Router.Children.ForEach(false, CopyChildrenCallback, ContextPtrs);
            
            pMeshState->Neighbors = (POTLWF_NEIGBHOR_LINK)(pMeshState->Children + pMeshState->cChildren);            
            pFilter->RoutingData.Neighbors.ForEach(false, CopyNeighborsCallback, ContextPtrs);
            
            // Set the output buffer size to just return the header if we 
            // didn't have enough room for the entire buffer
            if (NewOutBufferLength > *OutBufferLength)
            {
                NewOutBufferLength = sizeof(OTLWF_MESH_STATE);
            }
            else
            {
                NT_ASSERT(NewOutBufferLength == OTLWF_MESH_STATE_SIZE(pMeshState->cChildren, pMeshState->cNeighbors));
            }
        }
    }
    else*/
    {
        NewOutBufferLength = sizeof(OTLWF_MESH_STATE);
    }

error:

    if (pFilter)
    {
        // Release the filter
        otLwfReleaseInterface(pFilter);
    }

    // Zero out any left over buffer
    if (NewOutBufferLength < *OutBufferLength)
    {
        RtlZeroMemory(((PUCHAR)OutBuffer) + NewOutBufferLength, (*OutBufferLength - NewOutBufferLength));
    }

    *OutBufferLength = NewOutBufferLength;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}
