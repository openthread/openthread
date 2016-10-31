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

#include "precomp.h"
#include "iocontrol.tmh"

// Handles queries for the current list of Thread interfaces
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlEnumerateInterfaces(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG NewOutBufferLength = 0;
    POTLWF_INTERFACE_LIST pInterfaceList = (POTLWF_INTERFACE_LIST)OutBuffer;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure to zero out the output first
    RtlZeroMemory(OutBuffer, *OutBufferLength);

    NdisAcquireSpinLock(&FilterListLock);

    // Make sure there is enough space for the first uint16_t
    if (*OutBufferLength < sizeof(uint16_t))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    // Iterate through each interface and build up the list of running interfaces
    for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
    {
        PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);
        if (pFilter->State != FilterRunning) continue;

        PGUID pInterfaceGuid = &pInterfaceList->InterfaceGuids[pInterfaceList->cInterfaceGuids];
        pInterfaceList->cInterfaceGuids++;

        NewOutBufferLength =
            FIELD_OFFSET(OTLWF_INTERFACE_LIST, InterfaceGuids) +
            pInterfaceList->cInterfaceGuids * sizeof(GUID);

        if (NewOutBufferLength <= *OutBufferLength)
        {
            *pInterfaceGuid = pFilter->InterfaceGuid;
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

// Handles queries for the details of a specific Thread interface
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlQueryInterface(
    _In_reads_bytes_(InBufferLength)
            PVOID           InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    NewOutBufferLength = 0;

    LogFuncEntry(DRIVER_IOCTL);

    // Make sure there is enough space for the first USHORT
    if (InBufferLength < sizeof(GUID) || *OutBufferLength < sizeof(OTLWF_DEVICE))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }
    
    PGUID pInterfaceGuid = (PGUID)InBuffer;
    POTLWF_DEVICE pDevice = (POTLWF_DEVICE)OutBuffer;

    // Look up the interface
    PMS_FILTER pFilter = otLwfFindAndRefInterface(pInterfaceGuid);
    if (pFilter == NULL)
    {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto error;
    }

    NewOutBufferLength = sizeof(OTLWF_DEVICE);
    pDevice->CompartmentID = pFilter->InterfaceCompartmentID;

    // Release the ref on the interface
    otLwfReleaseInterface(pFilter);

error:

    if (NewOutBufferLength < *OutBufferLength)
    {
        RtlZeroMemory((PUCHAR)OutBuffer + NewOutBufferLength, *OutBufferLength - NewOutBufferLength);
    }

    *OutBufferLength = NewOutBufferLength;

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

// Handles IOTCLs for OpenThread control
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtlOpenThreadControl(
    _In_ PIRP Irp
    )
{
    NTSTATUS   status = STATUS_PENDING;
    PMS_FILTER pFilter = NULL;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    LogFuncEntry(DRIVER_IOCTL);

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(GUID))
    {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    pFilter = otLwfFindAndRefInterface((PGUID)Irp->AssociatedIrp.SystemBuffer);
    if (pFilter == NULL)
    {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto error;
    }

    // Pend the Irp for processing on the OpenThread event processing thread
    otLwfEventProcessingIndicateIrp(pFilter, Irp);

    // Release our ref on the filter
    otLwfReleaseInterface(pFilter);

error:

    // Complete the IRP if we aren't pending (indicates we failed)
    if (status != STATUS_PENDING)
    {
        NT_ASSERT(status != STATUS_SUCCESS);
        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, IrpSp->Parameters.DeviceIoControl.OutputBufferLength);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

const char* IoCtlStrings[] = 
{
    "IOCTL_OTLWF_OT_ENABLED",
    "IOCTL_OTLWF_OT_INTERFACE",
    "IOCTL_OTLWF_OT_THREAD",
    "IOCTL_OTLWF_OT_ACTIVE_SCAN",
    "IOCTL_OTLWF_OT_DISCOVER",
    "IOCTL_OTLWF_OT_CHANNEL",
    "IOCTL_OTLWF_OT_CHILD_TIMEOUT",
    "IOCTL_OTLWF_OT_EXTENDED_ADDRESS",
    "IOCTL_OTLWF_OT_EXTENDED_PANID",
    "IOCTL_OTLWF_OT_LEADER_RLOC",
    "IOCTL_OTLWF_OT_LINK_MODE",
    "IOCTL_OTLWF_OT_MASTER_KEY",
    "IOCTL_OTLWF_OT_MESH_LOCAL_EID",
    "IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX",
    "IOCTL_OTLWF_OT_NETWORK_DATA_LEADER",
    "IOCTL_OTLWF_OT_NETWORK_DATA_LOCAL",
    "IOCTL_OTLWF_OT_NETWORK_NAME",
    "IOCTL_OTLWF_OT_PAN_ID",
    "IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED",
    "IOCTL_OTLWF_OT_SHORT_ADDRESS",
    "IOCTL_OTLWF_OT_UNICAST_ADDRESSES",
    "IOCTL_OTLWF_OT_ACTIVE_DATASET",
    "IOCTL_OTLWF_OT_PENDING_DATASET",
    "IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT",
    "IOCTL_OTLWF_OT_ADD_BORDER_ROUTER",
    "IOCTL_OTLWF_OT_REMOVE_BORDER_ROUTER",
    "IOCTL_OTLWF_OT_ADD_EXTERNAL_ROUTE",
    "IOCTL_OTLWF_OT_REMOVE_EXTERNAL_ROUTE",
    "IOCTL_OTLWF_OT_SEND_SERVER_DATA",
    "IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY",
    "IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER",
    "IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT",
    "IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD",
    "IOCTL_OTLWF_OT_RELEASE_ROUTER_ID",
    "IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED",
    "IOCTL_OTLWF_OT_ADD_MAC_WHITELIST",
    "IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST",
    "IOCTL_OTLWF_OT_MAC_WHITELIST_ENTRY",
    "IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST",
    "IOCTL_OTLWF_OT_DEVICE_ROLE",
    "IOCTL_OTLWF_OT_CHILD_INFO_BY_ID",
    "IOCTL_OTLWF_OT_CHILD_INFO_BY_INDEX",
    "IOCTL_OTLWF_OT_EID_CACHE_ENTRY",
    "IOCTL_OTLWF_OT_LEADER_DATA",
    "IOCTL_OTLWF_OT_LEADER_ROUTER_ID",
    "IOCTL_OTLWF_OT_LEADER_WEIGHT",
    "IOCTL_OTLWF_OT_NETWORK_DATA_VERSION",
    "IOCTL_OTLWF_OT_PARTITION_ID",
    "IOCTL_OTLWF_OT_RLOC16",
    "IOCTL_OTLWF_OT_ROUTER_ID_SEQUENCE",
    "IOCTL_OTLWF_OT_ROUTER_INFO",
    "IOCTL_OTLWF_OT_STABLE_NETWORK_DATA_VERSION",
    "IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED",
    "IOCTL_OTLWF_OT_ADD_MAC_BLACKLIST",
    "IOCTL_OTLWF_OT_REMOVE_MAC_BLACKLIST",
    "IOCTL_OTLWF_OT_MAC_BLACKLIST_ENTRY",
    "IOCTL_OTLWF_OT_CLEAR_MAC_BLACKLIST",
    "IOCTL_OTLWF_OT_MAX_TRANSMIT_POWER",
    "IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX",
    "IOCTL_OTLWF_OT_POLL_PERIOD",
    "IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID",
    "IOCTL_OTLWF_OT_ASSIGN_LINK_QUALITY",
    "IOCTL_OTLWF_OT_PLATFORM_RESET",
    "IOCTL_OTLWF_OT_PARENT_INFO",
    "IOCTL_OTLWF_OT_SINGLETON"
    "IOCTL_OTLWF_OT_MAC_COUNTERS",
    "IOCTL_OTLWF_OT_MAX_CHILDREN",
    "IOCTL_OTLWF_OT_COMMISIONER_START",
    "IOCTL_OTLWF_OT_COMMISIONER_STOP",
    "IOCTL_OTLWF_OT_JOINER_START",
    "IOCTL_OTLWF_OT_JOINER_STOP",
    "IOCTL_OTLWF_OT_FACTORY_EUI64",
    "IOCTL_OTLWF_OT_HASH_MAC_ADDRESS",
    "IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD",
    "IOCTL_OTLWF_OT_COMMISSIONER_PANID_QUERY",
    "IOCTL_OTLWF_OT_COMMISSIONER_ENERGY_SCAN",
    "IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER",
    "IOCTL_OTLWF_OT_JOINER_UDP_PORT",
    "IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_GET",
    "IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_RESET",
    "IOCTL_OTLWF_OT_COMMISIONER_ADD_JOINER",
    "IOCTL_OTLWF_OT_COMMISIONER_REMOVE_JOINER",
    "IOCTL_OTLWF_OT_COMMISIONER_PROVISIONING_URL",
    "IOCTL_OTLWF_OT_COMMISIONER_ANNOUNCE_BEGIN",
    "IOCTL_OTLWF_OT_ENERGY_SCAN",
    "IOCTL_OTLWF_OT_SEND_ACTIVE_GET",
    "IOCTL_OTLWF_OT_SEND_ACTIVE_SET",
    "IOCTL_OTLWF_OT_SEND_PENDING_GET",
    "IOCTL_OTLWF_OT_SEND_PENDING_SET",
    "IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_GET",
    "IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_SET",
    "IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME",
};

static_assert(ARRAYSIZE(IoCtlStrings) == (MAX_OTLWF_IOCTL_FUNC_CODE - MIN_OTLWF_IOCTL_FUNC_CODE),
              "The IoCtl strings should be up to date with the actual IoCtl list.");

const char*
IoCtlString(
    ULONG IoControlCode
)
{
    ULONG FuncCode = ((IoControlCode >> 2) & 0xFFF) - 100;
    return FuncCode < ARRAYSIZE(IoCtlStrings) ? IoCtlStrings[FuncCode] : "UNKNOWN IOCTL";
}

// Handles Irp for IOTCLs for OpenThread control on the OpenThread thread
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfCompleteOpenThreadIrp(
    _In_ PMS_FILTER     pFilter,
    _In_ PIRP           Irp
    )
{
    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PUCHAR InBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer + sizeof(GUID);
    PVOID OutBuffer = Irp->AssociatedIrp.SystemBuffer;

    ULONG InBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength - sizeof(GUID);
    ULONG OutBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    ULONG OrigOutBufferLength = OutBufferLength;
        
    NTSTATUS status = STATUS_SUCCESS;
        
    LogVerbose(DRIVER_IOCTL, "Processing Irp=%p, for %s (In:%u,Out:%u)", 
                Irp, IoCtlString(IoControlCode), InBufferLength, OutBufferLength);

    switch (IoControlCode)
    {
    case IOCTL_OTLWF_OT_INTERFACE:
        status = otLwfIoCtl_otInterface(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_THREAD:
        status = otLwfIoCtl_otThread(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ACTIVE_SCAN:
        status = otLwfIoCtl_otActiveScan(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_DISCOVER:
        status = otLwfIoCtl_otDiscover(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_CHANNEL:
        status = otLwfIoCtl_otChannel(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_CHILD_TIMEOUT:
        status = otLwfIoCtl_otChildTimeout(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_EXTENDED_ADDRESS:
        status = otLwfIoCtl_otExtendedAddress(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_EXTENDED_PANID:
        status = otLwfIoCtl_otExtendedPanId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_LEADER_RLOC:
        status = otLwfIoCtl_otLeaderRloc(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_LINK_MODE:
        status = otLwfIoCtl_otLinkMode(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MASTER_KEY:
        status = otLwfIoCtl_otMasterKey(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;            
    case IOCTL_OTLWF_OT_MESH_LOCAL_EID:
        status = otLwfIoCtl_otMeshLocalEid(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX:
        status = otLwfIoCtl_otMeshLocalPrefix(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_NETWORK_DATA_LEADER:
        status = STATUS_NOT_IMPLEMENTED;
        break;    
    case IOCTL_OTLWF_OT_NETWORK_DATA_LOCAL:
        status = STATUS_NOT_IMPLEMENTED;
        break;    
    case IOCTL_OTLWF_OT_NETWORK_NAME:
        status = otLwfIoCtl_otNetworkName(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_PAN_ID:
        status = otLwfIoCtl_otPanId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED:
        status = otLwfIoCtl_otRouterRollEnabled(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_SHORT_ADDRESS:
        status = otLwfIoCtl_otShortAddress(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;    
    case IOCTL_OTLWF_OT_UNICAST_ADDRESSES:
        status = STATUS_NOT_IMPLEMENTED;
        break;    
    case IOCTL_OTLWF_OT_ACTIVE_DATASET:
        status = otLwfIoCtl_otActiveDataset(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_PENDING_DATASET:
        status = otLwfIoCtl_otPendingDataset(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT:
        status = otLwfIoCtl_otLocalLeaderWeight(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ADD_BORDER_ROUTER:
        status = otLwfIoCtl_otAddBorderRouter(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_REMOVE_BORDER_ROUTER:
        status = otLwfIoCtl_otRemoveBorderRouter(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ADD_EXTERNAL_ROUTE:
        status = otLwfIoCtl_otAddExternalRoute(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_REMOVE_EXTERNAL_ROUTE:
        status = otLwfIoCtl_otRemoveExternalRoute(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_SERVER_DATA:
        status = otLwfIoCtl_otSendServerData(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY:
        status = otLwfIoCtl_otContextIdReuseDelay(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER:
        status = otLwfIoCtl_otKeySequenceCounter(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT:
        status = otLwfIoCtl_otNetworkIdTimeout(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD:
        status = otLwfIoCtl_otRouterUpgradeThreshold(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_RELEASE_ROUTER_ID:
        status = otLwfIoCtl_otReleaseRouterId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED:
        status = otLwfIoCtl_otMacWhitelistEnabled(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ADD_MAC_WHITELIST:
        status = otLwfIoCtl_otAddMacWhitelist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST:
        status = otLwfIoCtl_otRemoveMacWhitelist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAC_WHITELIST_ENTRY:
        status = otLwfIoCtl_otMacWhitelistEntry(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST:
        status = otLwfIoCtl_otClearMacWhitelist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;            
    case IOCTL_OTLWF_OT_DEVICE_ROLE:
        status = otLwfIoCtl_otDeviceRole(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_CHILD_INFO_BY_ID:
        status = otLwfIoCtl_otChildInfoById(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_CHILD_INFO_BY_INDEX:
        status = otLwfIoCtl_otChildInfoByIndex(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_EID_CACHE_ENTRY:
        status = otLwfIoCtl_otEidCacheEntry(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_LEADER_DATA:
        status = otLwfIoCtl_otLeaderData(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_LEADER_ROUTER_ID:
        status = otLwfIoCtl_otLeaderRouterId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_LEADER_WEIGHT:
        status = otLwfIoCtl_otLeaderWeight(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_NETWORK_DATA_VERSION:
        status = otLwfIoCtl_otNetworkDataVersion(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_PARTITION_ID:
        status = otLwfIoCtl_otPartitionId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_RLOC16:
        status = otLwfIoCtl_otRloc16(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_ROUTER_ID_SEQUENCE:
        status = otLwfIoCtl_otRouterIdSequence(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_ROUTER_INFO:
        status = otLwfIoCtl_otRouterInfo(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;        
    case IOCTL_OTLWF_OT_STABLE_NETWORK_DATA_VERSION:
        status = otLwfIoCtl_otStableNetworkDataVersion(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED:
        status = otLwfIoCtl_otMacBlacklistEnabled(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ADD_MAC_BLACKLIST:
        status = otLwfIoCtl_otAddMacBlacklist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_REMOVE_MAC_BLACKLIST:
        status = otLwfIoCtl_otRemoveMacBlacklist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAC_BLACKLIST_ENTRY:
        status = otLwfIoCtl_otMacBlacklistEntry(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_CLEAR_MAC_BLACKLIST:
        status = otLwfIoCtl_otClearMacBlacklist(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAX_TRANSMIT_POWER:
        status = otLwfIoCtl_otMaxTransmitPower(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX:
        status = otLwfIoCtl_otNextOnMeshPrefix(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_POLL_PERIOD:
        status = otLwfIoCtl_otPollPeriod(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID:
        status = otLwfIoCtl_otLocalLeaderPartitionId(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ASSIGN_LINK_QUALITY:
        status = otLwfIoCtl_otAssignLinkQuality(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_PLATFORM_RESET:
        status = otLwfIoCtl_otPlatformReset(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_PARENT_INFO:
        status = otLwfIoCtl_otParentInfo(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SINGLETON:
        status = otLwfIoCtl_otSingleton(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAC_COUNTERS:
        status = otLwfIoCtl_otMacCounters(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_MAX_CHILDREN:
        status = otLwfIoCtl_otMaxChildren(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_START:
        status = otLwfIoCtl_otCommissionerStart(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_STOP:
        status = otLwfIoCtl_otCommissionerStop(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_JOINER_START:
        status = otLwfIoCtl_otJoinerStart(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_JOINER_STOP:
        status = otLwfIoCtl_otJoinerStop(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_FACTORY_EUI64:
        status = otLwfIoCtl_otFactoryAssignedIeeeEui64(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_HASH_MAC_ADDRESS:
        status = otLwfIoCtl_otHashMacAddress(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD:
        status = otLwfIoCtl_otRouterDowngradeThreshold(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISSIONER_PANID_QUERY:
        status = otLwfIoCtl_otCommissionerPanIdQuery(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISSIONER_ENERGY_SCAN:
        status = otLwfIoCtl_otCommissionerEnergyScan(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER:
        status = otLwfIoCtl_otRouterSelectionJitter(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_JOINER_UDP_PORT:
        status = otLwfIoCtl_otJoinerUdpPort(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_GET:
        status = otLwfIoCtl_otSendDiagnosticGet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_RESET:
        status = otLwfIoCtl_otSendDiagnosticReset(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_ADD_JOINER:
        status = otLwfIoCtl_otCommissionerAddJoiner(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_REMOVE_JOINER:
        status = otLwfIoCtl_otCommissionerRemoveJoiner(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_PROVISIONING_URL:
        status = otLwfIoCtl_otCommissionerProvisioningUrl(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_COMMISIONER_ANNOUNCE_BEGIN:
        status = otLwfIoCtl_otCommissionerAnnounceBegin(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_ENERGY_SCAN:
        status = otLwfIoCtl_otEnergyScan(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_ACTIVE_GET:
        status = otLwfIoCtl_otSendActiveGet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_ACTIVE_SET:
        status = otLwfIoCtl_otSendActiveSet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_PENDING_GET:
        status = otLwfIoCtl_otSendPendingGet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_PENDING_SET:
        status = otLwfIoCtl_otSendPendingSet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_GET:
        status = otLwfIoCtl_otSendMgmtCommissionerGet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_SET:
        status = otLwfIoCtl_otSendMgmtCommissionerSet(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    case IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME:
        status = otLwfIoCtl_otKeySwitchGuardtime(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        OutBufferLength = 0;
    }

    // Clear any leftover output buffer
    if (OutBufferLength < OrigOutBufferLength)
    {
        RtlZeroMemory((PUCHAR)OutBuffer + OutBufferLength, OrigOutBufferLength - OutBufferLength);
    }

    LogVerbose(DRIVER_IOCTL, "Completing Irp=%p, with %!STATUS! for %s (Out:%u)", 
                Irp, status, IoCtlString(IoControlCode), OutBufferLength);

    // Complete the IRP
    Irp->IoStatus.Information = OutBufferLength;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otInterface(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        BOOLEAN IsEnabled = *(BOOLEAN*)InBuffer;
        if (IsEnabled)
        {
            // Make sure our addresses are in sync
            (void)otLwfInitializeAddresses(pFilter);
            otLwfAddressesUpdated(pFilter);

            status = ThreadErrorToNtstatus(otInterfaceUp(pFilter->otCtx));
        }
        else
        {
            status = ThreadErrorToNtstatus(otInterfaceDown(pFilter->otCtx));
        }
        
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsInterfaceUp(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otThread(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        BOOLEAN IsEnabled = *(BOOLEAN*)InBuffer;
        if (IsEnabled)
        {
            status = ThreadErrorToNtstatus(otThreadStart(pFilter->otCtx));
        }
        else
        {
            status = ThreadErrorToNtstatus(otThreadStop(pFilter->otCtx));
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otActiveScan(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        status = ThreadErrorToNtstatus(
            otActiveScan(
                pFilter->otCtx, 
                aScanChannels, 
                aScanDuration, 
                otLwfActiveScanCallback,
                pFilter)
            );
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsActiveScanInProgress(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otEnergyScan(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        status = ThreadErrorToNtstatus(
            otEnergyScan(
                pFilter->otCtx, 
                aScanChannels, 
                aScanDuration, 
                otLwfEnergyScanCallback,
                pFilter)
            );
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsEnergyScanInProgress(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otDiscover(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        uint16_t aPanid = *(uint16_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint16_t));
        status = ThreadErrorToNtstatus(
            otDiscover(
                pFilter->otCtx, 
                aScanChannels, 
                aScanDuration, 
                aPanid,
                otLwfDiscoverCallback,
                pFilter)
            );
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsDiscoverInProgress(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otChannel(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        uint8_t aChannel = *(uint8_t*)InBuffer;
        status = ThreadErrorToNtstatus(otSetChannel(pFilter->otCtx, aChannel));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetChannel(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otChildTimeout(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        uint32_t aTimeout = *(uint32_t*)InBuffer;
        otSetChildTimeout(pFilter->otCtx, aTimeout);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetChildTimeout(pFilter->otCtx);
        *OutBufferLength = sizeof(uint32_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otExtendedAddress(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otExtAddress))
    {
        status = ThreadErrorToNtstatus(otSetExtendedAddress(pFilter->otCtx, (otExtAddress*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otExtAddress))
    {
        memcpy(OutBuffer, otGetExtendedAddress(pFilter->otCtx), sizeof(otExtAddress));
        *OutBufferLength = sizeof(otExtAddress);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otExtendedPanId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otExtendedPanId))
    {
        otSetExtendedPanId(pFilter->otCtx, (uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otExtendedPanId))
    {
        memcpy(OutBuffer, otGetExtendedPanId(pFilter->otCtx), sizeof(otExtendedPanId));
        *OutBufferLength = sizeof(otExtendedPanId);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otFactoryAssignedIeeeEui64(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otExtendedPanId))
    {
        otGetFactoryAssignedIeeeEui64(pFilter->otCtx, (otExtAddress*)OutBuffer);
        *OutBufferLength = sizeof(otExtendedPanId);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otHashMacAddress(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otExtAddress))
    {
        otGetHashMacAddress(pFilter->otCtx, (otExtAddress*)OutBuffer);
        *OutBufferLength = sizeof(otExtAddress);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLeaderRloc(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otIp6Address))
    {
        status = ThreadErrorToNtstatus(otGetLeaderRloc(pFilter->otCtx, (otIp6Address*)OutBuffer));
        *OutBufferLength = sizeof(otExtendedPanId);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLinkMode(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    static_assert(sizeof(otLinkModeConfig) == 4, "The size of otLinkModeConfig should be 4 bytes");
    if (InBufferLength >= sizeof(otLinkModeConfig))
    {
        status = ThreadErrorToNtstatus(otSetLinkMode(pFilter->otCtx, *(otLinkModeConfig*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otLinkModeConfig))
    {
        *(otLinkModeConfig*)OutBuffer = otGetLinkMode(pFilter->otCtx);
        *OutBufferLength = sizeof(otLinkModeConfig);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMasterKey(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otMasterKey) + sizeof(uint8_t))
    {
        uint8_t aKeyLength = *(uint8_t*)(InBuffer + sizeof(otMasterKey));
        status = ThreadErrorToNtstatus(otSetMasterKey(pFilter->otCtx, InBuffer, aKeyLength));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otMasterKey) + sizeof(uint8_t))
    {
        uint8_t aKeyLength = 0;
        const uint8_t* aMasterKey = otGetMasterKey(pFilter->otCtx, &aKeyLength);
        memcpy(OutBuffer, aMasterKey, aKeyLength);
        memcpy((PUCHAR)OutBuffer + sizeof(otMasterKey), &aKeyLength, sizeof(uint8_t));
        *OutBufferLength = sizeof(otMasterKey) + sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMeshLocalEid(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otIp6Address))
    {
        memcpy(OutBuffer,  otGetMeshLocalEid(pFilter->otCtx), sizeof(otIp6Address));
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMeshLocalPrefix(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otMeshLocalPrefix))
    {
        status = ThreadErrorToNtstatus(otSetMeshLocalPrefix(pFilter->otCtx, InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otMeshLocalPrefix))
    {
        memcpy(OutBuffer, otGetMeshLocalPrefix(pFilter->otCtx), sizeof(otMeshLocalPrefix));
        *OutBufferLength = sizeof(otMeshLocalPrefix);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

// otLwfIoCtl_otNetworkDataLeader

// otLwfIoCtl_otNetworkDataLocal

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNetworkName(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otNetworkName))
    {
        status = ThreadErrorToNtstatus(otSetNetworkName(pFilter->otCtx, (char*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otNetworkName))
    {
        strcpy_s((char*)OutBuffer, sizeof(otNetworkName), otGetNetworkName(pFilter->otCtx));
        *OutBufferLength = sizeof(otNetworkName);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPanId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otPanId))
    {
        status = ThreadErrorToNtstatus(otSetPanId(pFilter->otCtx, *(otPanId*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otPanId))
    {
        *(otPanId*)OutBuffer = otGetPanId(pFilter->otCtx);
        *OutBufferLength = sizeof(otPanId);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterRollEnabled(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        otSetRouterRoleEnabled(pFilter->otCtx, *(BOOLEAN*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsRouterRoleEnabled(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otShortAddress(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otShortAddress))
    {
        *(otShortAddress*)OutBuffer = otGetShortAddress(pFilter->otCtx);
        *OutBufferLength = sizeof(otShortAddress);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

// otLwfIoCtl_otUnicastAddresses

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otActiveDataset(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otSetActiveDataset(pFilter->otCtx, (otOperationalDataset*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otGetActiveDataset(pFilter->otCtx, (otOperationalDataset*)OutBuffer));
        *OutBufferLength = sizeof(otOperationalDataset);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPendingDataset(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otSetPendingDataset(pFilter->otCtx, (otOperationalDataset*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otGetPendingDataset(pFilter->otCtx, (otOperationalDataset*)OutBuffer));
        *OutBufferLength = sizeof(otOperationalDataset);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLocalLeaderWeight(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetLocalLeaderWeight(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetLeaderWeight(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAddBorderRouter(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otBorderRouterConfig))
    {
        status = ThreadErrorToNtstatus(otAddBorderRouter(pFilter->otCtx, (otBorderRouterConfig*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRemoveBorderRouter(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otIp6Prefix))
    {
        status = ThreadErrorToNtstatus(otRemoveBorderRouter(pFilter->otCtx, (otIp6Prefix*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAddExternalRoute(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otExternalRouteConfig))
    {
        status = ThreadErrorToNtstatus(otAddExternalRoute(pFilter->otCtx, (otExternalRouteConfig*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRemoveExternalRoute(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otIp6Prefix))
    {
        status = ThreadErrorToNtstatus(otRemoveExternalRoute(pFilter->otCtx, (otIp6Prefix*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendServerData(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    status = ThreadErrorToNtstatus(otSendServerData(pFilter->otCtx));

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otContextIdReuseDelay(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        otSetContextIdReuseDelay(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetContextIdReuseDelay(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint32_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otKeySequenceCounter(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        otSetKeySequenceCounter(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetKeySequenceCounter(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint32_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNetworkIdTimeout(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetNetworkIdTimeout(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetNetworkIdTimeout(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint8_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterUpgradeThreshold(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetRouterUpgradeThreshold(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetRouterUpgradeThreshold(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint8_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterDowngradeThreshold(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetRouterDowngradeThreshold(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetRouterDowngradeThreshold(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint8_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otReleaseRouterId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(uint8_t))
    {
        status = ThreadErrorToNtstatus(otReleaseRouterId(pFilter->otCtx, *(uint8_t*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMacWhitelistEnabled(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        BOOLEAN aEnabled = *(BOOLEAN*)InBuffer;
        if (aEnabled)
        {
            otEnableMacWhitelist(pFilter->otCtx);
        }
        else
        {
            otDisableMacWhitelist(pFilter->otCtx);
        }
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsMacWhitelistEnabled(pFilter->otCtx) ? TRUE : FALSE;
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(BOOLEAN);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAddMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otExtAddress) + sizeof(int8_t))
    {
        int8_t aRssi = *(int8_t*)(InBuffer + sizeof(otExtAddress));
        status = ThreadErrorToNtstatus(otAddMacWhitelistRssi(pFilter->otCtx, (uint8_t*)InBuffer, aRssi));
    }
    else if (InBufferLength >= sizeof(otExtAddress))
    {
        status = ThreadErrorToNtstatus(otAddMacWhitelist(pFilter->otCtx, (uint8_t*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRemoveMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otExtAddress))
    {
        otRemoveMacWhitelist(pFilter->otCtx, (uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMacWhitelistEntry(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint8_t) && 
        *OutBufferLength >= sizeof(otMacWhitelistEntry))
    {
        status = ThreadErrorToNtstatus(
            otGetMacWhitelistEntry(
                pFilter->otCtx, 
                *(uint8_t*)InBuffer, 
                (otMacWhitelistEntry*)OutBuffer)
            );
        *OutBufferLength = sizeof(otMacWhitelistEntry);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otClearMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    otClearMacWhitelist(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otDeviceRole(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint8_t))
    {
        otDeviceRole role = *(uint8_t*)InBuffer;

        InBufferLength -= sizeof(uint8_t);
        InBuffer = InBuffer + sizeof(uint8_t);

        if (role == kDeviceRoleLeader)
        {
            status = ThreadErrorToNtstatus(
                        otBecomeLeader(pFilter->otCtx)
                        );
        }
        else if (role == kDeviceRoleRouter)
        {
            status = ThreadErrorToNtstatus(
                        otBecomeRouter(pFilter->otCtx)
                        );
        }
        else if (role == kDeviceRoleChild)
        {
            if (InBufferLength >= sizeof(uint8_t))
            {
                status = ThreadErrorToNtstatus(
                            otBecomeChild(pFilter->otCtx, *(uint8_t*)InBuffer)
                            );
            }
        }
        else if (role == kDeviceRoleDetached)
        {
            status = ThreadErrorToNtstatus(
                        otBecomeDetached(pFilter->otCtx)
                        );
        }
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = (uint8_t)otGetDeviceRole(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otChildInfoById(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint16_t) && 
        *OutBufferLength >= sizeof(otChildInfo))
    {
        status = ThreadErrorToNtstatus(
            otGetChildInfoById(
                pFilter->otCtx, 
                *(uint16_t*)InBuffer, 
                (otChildInfo*)OutBuffer)
            );
        *OutBufferLength = sizeof(otChildInfo);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otChildInfoByIndex(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint8_t) && 
        *OutBufferLength >= sizeof(otChildInfo))
    {
        status = ThreadErrorToNtstatus(
            otGetChildInfoByIndex(
                pFilter->otCtx, 
                *(uint8_t*)InBuffer, 
                (otChildInfo*)OutBuffer)
            );
        *OutBufferLength = sizeof(otChildInfo);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otEidCacheEntry(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint8_t) && 
        *OutBufferLength >= sizeof(otEidCacheEntry))
    {
        status = ThreadErrorToNtstatus(
            otGetEidCacheEntry(
                pFilter->otCtx, 
                *(uint8_t*)InBuffer, 
                (otEidCacheEntry*)OutBuffer)
            );
        *OutBufferLength = sizeof(otEidCacheEntry);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLeaderData(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otLeaderData))
    {
        status = ThreadErrorToNtstatus(otGetLeaderData(pFilter->otCtx, (otLeaderData*)OutBuffer));
        *OutBufferLength = sizeof(otLeaderData);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLeaderRouterId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetLeaderRouterId(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLeaderWeight(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetLeaderWeight(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNetworkDataVersion(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetNetworkDataVersion(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPartitionId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetPartitionId(pFilter->otCtx);
        *OutBufferLength = sizeof(uint32_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRloc16(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint16_t))
    {
        *(uint16_t*)OutBuffer = otGetRloc16(pFilter->otCtx);
        *OutBufferLength = sizeof(uint16_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterIdSequence(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetRouterIdSequence(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterInfo(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint16_t) && 
        *OutBufferLength >= sizeof(otRouterInfo))
    {
        status = ThreadErrorToNtstatus(
            otGetRouterInfo(
                pFilter->otCtx, 
                *(uint16_t*)InBuffer, 
                (otRouterInfo*)OutBuffer)
            );
        *OutBufferLength = sizeof(otRouterInfo);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otStableNetworkDataVersion(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetStableNetworkDataVersion(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMacBlacklistEnabled(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        BOOLEAN aEnabled = *(BOOLEAN*)InBuffer;
        if (aEnabled)
        {
            otEnableMacBlacklist(pFilter->otCtx);
        }
        else
        {
            otDisableMacBlacklist(pFilter->otCtx);
        }
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsMacBlacklistEnabled(pFilter->otCtx) ? TRUE : FALSE;
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(BOOLEAN);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAddMacBlacklist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otExtAddress))
    {
        status = ThreadErrorToNtstatus(otAddMacBlacklist(pFilter->otCtx, (uint8_t*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRemoveMacBlacklist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otExtAddress))
    {
        otRemoveMacBlacklist(pFilter->otCtx, (uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMacBlacklistEntry(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(uint8_t) && 
        *OutBufferLength >= sizeof(otMacBlacklistEntry))
    {
        status = ThreadErrorToNtstatus(
            otGetMacBlacklistEntry(
                pFilter->otCtx, 
                *(uint8_t*)InBuffer, 
                (otMacBlacklistEntry*)OutBuffer)
            );
        *OutBufferLength = sizeof(otMacBlacklistEntry);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otClearMacBlacklist(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    otClearMacBlacklist(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMaxTransmitPower(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(int8_t))
    {
        otSetMaxTransmitPower(pFilter->otCtx, *(int8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(int8_t))
    {
        *(int8_t*)OutBuffer = otGetMaxTransmitPower(pFilter->otCtx);
        *OutBufferLength = sizeof(int8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNextOnMeshPrefix(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    if (InBufferLength >= sizeof(BOOLEAN) + sizeof(uint8_t) && 
        *OutBufferLength >= sizeof(uint8_t) + sizeof(otBorderRouterConfig))
    {
        BOOLEAN aLocal = *(BOOLEAN*)InBuffer;
        uint8_t aIterator = *(uint8_t*)(InBuffer + sizeof(BOOLEAN));
        otBorderRouterConfig* aConfig = (otBorderRouterConfig*)((PUCHAR)OutBuffer + sizeof(uint8_t));
        status = ThreadErrorToNtstatus(
            otGetNextOnMeshPrefix(
                pFilter->otCtx, 
                aLocal, 
                &aIterator,
                aConfig)
            );
        *OutBufferLength = sizeof(uint8_t) + sizeof(otBorderRouterConfig);
        if (status == STATUS_SUCCESS)
        {
            *(uint8_t*)OutBuffer = aIterator;
        }
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPollPeriod(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        otSetPollPeriod(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetPollPeriod(pFilter->otCtx);
        *OutBufferLength = sizeof(uint32_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otLocalLeaderPartitionId(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        otSetLocalLeaderPartitionId(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetLocalLeaderPartitionId(pFilter->otCtx);
        *OutBufferLength = sizeof(uint32_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAssignLinkQuality(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otExtAddress) + sizeof(uint8_t))
    {
        otSetAssignLinkQuality(
            pFilter->otCtx, 
            (uint8_t*)InBuffer, 
            *(uint8_t*)(InBuffer + sizeof(otExtAddress)));
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (InBufferLength >= sizeof(otExtAddress) &&
            *OutBufferLength >= sizeof(uint8_t))
    {
        status = ThreadErrorToNtstatus(
            otGetAssignLinkQuality(
                pFilter->otCtx, 
                (uint8_t*)InBuffer, 
                (uint8_t*)OutBuffer)
            );
        *OutBufferLength = sizeof(uint32_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPlatformReset(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    otPlatformReset(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otParentInfo(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    
    static_assert(sizeof(otRouterInfo) == 20, "The size of otRouterInfo should be 20 bytes");
    if (*OutBufferLength >= sizeof(otRouterInfo))
    {
        status = ThreadErrorToNtstatus(otGetParentInfo(pFilter->otCtx, (otRouterInfo*)OutBuffer));
        *OutBufferLength = sizeof(otRouterInfo);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSingleton(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIsSingleton(pFilter->otCtx) ? TRUE : FALSE;
        *OutBufferLength = sizeof(BOOLEAN);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMacCounters(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (*OutBufferLength >= sizeof(otMacCounters))
    {
        memcpy_s(OutBuffer, *OutBufferLength, otGetMacCounters(pFilter->otCtx), sizeof(otMacCounters));
        *OutBufferLength = sizeof(otMacCounters);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}    

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otMaxChildren(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetMaxAllowedChildren(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetMaxAllowedChildren(pFilter->otCtx);
        *OutBufferLength = sizeof(uint8_t);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerStart(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{    
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    return ThreadErrorToNtstatus(otCommissionerStart(pFilter->otCtx));
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerStop(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    status = ThreadErrorToNtstatus(otCommissionerStop(pFilter->otCtx));

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otJoinerStart(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(otCommissionConfig))
    {
        otCommissionConfig *aConfig = (otCommissionConfig*)InBuffer;
        status = ThreadErrorToNtstatus(otJoinerStart(
            pFilter->otCtx, (const char*)aConfig->PSKd, (const char*)aConfig->ProvisioningUrl));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otJoinerStop(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    status = ThreadErrorToNtstatus(otJoinerStop(pFilter->otCtx));

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerPanIdQuery(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint16_t) + sizeof(uint32_t) + sizeof(otIp6Address))
    {
        uint16_t aPanId = *(uint16_t*)InBuffer;
        uint32_t aChannelMask = *(uint32_t*)(InBuffer + sizeof(uint16_t));
        const otIp6Address *aAddress = (otIp6Address*)(InBuffer + sizeof(uint16_t) + sizeof(uint32_t));

        status = ThreadErrorToNtstatus(
            otCommissionerPanIdQuery(
                pFilter->otCtx, 
                aPanId, 
                aChannelMask,
                aAddress,
                otLwfCommissionerPanIdConflictCallback,
                pFilter)
            );
    }

    return status;
}    

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerEnergyScan(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(otIp6Address))
    {
        uint32_t aChannelMask = *(uint32_t*)InBuffer;
        uint8_t aCount = *(uint8_t*)(InBuffer + sizeof(uint32_t));
        uint16_t aPeriod = *(uint16_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint8_t));
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t));
        const otIp6Address *aAddress = (otIp6Address*)(InBuffer + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t));

        status = ThreadErrorToNtstatus(
            otCommissionerEnergyScan(
                pFilter->otCtx,
                aChannelMask,
                aCount,
                aPeriod,
                aScanDuration,
                aAddress,
                otLwfCommissionerEnergyReportCallback,
                pFilter)
            );
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRouterSelectionJitter(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otSetRouterSelectionJitter(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otGetRouterSelectionJitter(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint8_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otJoinerUdpPort(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint16_t))
    {
        otSetJoinerUdpPort(pFilter->otCtx, *(uint16_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint16_t))
    {
        *(uint16_t*)OutBuffer = otGetJoinerUdpPort(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint16_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}    

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendDiagnosticGet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(otIp6Address) + sizeof(uint8_t))
    {
        const otIp6Address *aAddress = (otIp6Address*)InBuffer;
        uint8_t aCount = *(uint8_t*)(InBuffer + sizeof(otIp6Address));
        PUCHAR aTlvTypes = InBuffer + sizeof(otIp6Address) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otIp6Address) + sizeof(uint8_t) + aCount)
        {
            status = ThreadErrorToNtstatus(
                otSendDiagnosticGet(
                    pFilter->otCtx,
                    aAddress,
                    aTlvTypes,
                    aCount)
                );
        }
    }

    return status;
}   

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendDiagnosticReset(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(otIp6Address) + sizeof(uint8_t))
    {
        const otIp6Address *aAddress = (otIp6Address*)InBuffer;
        uint8_t aCount = *(uint8_t*)(InBuffer + sizeof(otIp6Address));
        PUCHAR aTlvTypes = InBuffer + sizeof(otIp6Address) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otIp6Address) + sizeof(uint8_t) + aCount)
        {
            status = ThreadErrorToNtstatus(
                otSendDiagnosticGet(
                    pFilter->otCtx,
                    aAddress,
                    aTlvTypes,
                    aCount)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerAddJoiner(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(uint8_t) + sizeof(otExtAddress))
    {
        const ULONG aPSKdBufferLength = InBufferLength - sizeof(uint8_t) - sizeof(otExtAddress);

        if (aPSKdBufferLength <= OPENTHREAD_PSK_MAX_LENGTH + 1)
        {
            uint8_t aExtAddressValid = *(uint8_t*)InBuffer;
            const otExtAddress *aExtAddress = aExtAddressValid == 0 ? NULL : (otExtAddress*)(InBuffer + sizeof(uint8_t));
            char *aPSKd = (char*)(InBuffer + sizeof(uint8_t) + sizeof(otExtAddress));

            // Ensure aPSKd is NULL terminated in the buffer
            if (strnlen(aPSKd, aPSKdBufferLength) < aPSKdBufferLength)
            {
                status = ThreadErrorToNtstatus(otCommissionerAddJoiner(
                    pFilter->otCtx, aExtAddress, aPSKd));
            }
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerRemoveJoiner(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength >= sizeof(uint8_t) + sizeof(otExtAddress))
    {
        uint8_t aExtAddressValid = *(uint8_t*)InBuffer;
        const otExtAddress *aExtAddress = aExtAddressValid == 0 ? NULL : (otExtAddress*)(InBuffer + sizeof(uint8_t));
        status = ThreadErrorToNtstatus(otCommissionerRemoveJoiner(
            pFilter->otCtx, aExtAddress));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerProvisioningUrl(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(OutBuffer);
    *OutBufferLength = 0;
    
    if (InBufferLength <= OPENTHREAD_PROV_URL_MAX_LENGTH + 1)
    {
        char *aProvisioningUrl = InBufferLength > 1 ? (char*)InBuffer : NULL;

        // Ensure aProvisioningUrl is empty or NULL terminated in the buffer
        if (aProvisioningUrl == NULL ||
            strnlen(aProvisioningUrl, InBufferLength) < InBufferLength)
        {
            status = ThreadErrorToNtstatus(otCommissionerSetProvisioningUrl(
                pFilter->otCtx, aProvisioningUrl));
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otCommissionerAnnounceBegin(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(otIp6Address))
    {
        uint32_t aChannelMask = *(uint32_t*)InBuffer;
        uint8_t aCount = *(uint8_t*)(InBuffer + sizeof(uint32_t));
        uint16_t aPeriod = *(uint16_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint8_t));
        const otIp6Address *aAddress = (otIp6Address*)(InBuffer + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t));

        if (InBufferLength >= sizeof(otIp6Address) + sizeof(uint8_t) + aCount)
        {
            status = ThreadErrorToNtstatus(
                otCommissionerAnnounceBegin(
                    pFilter->otCtx,
                    aChannelMask,
                    aCount,
                    aPeriod,
                    aAddress)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendActiveGet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint8_t))
    {
        uint8_t aLength = *(uint8_t*)InBuffer;
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(uint8_t);

        if (InBufferLength >= sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendActiveGet(
                    pFilter->otCtx,
                    aTlvTypes,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendActiveSet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(otOperationalDataset) + sizeof(uint8_t))
    {
        const otOperationalDataset *aDataset = (otOperationalDataset*)InBuffer;
        uint8_t aLength = *(uint8_t*)(InBuffer + sizeof(otOperationalDataset));
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(otOperationalDataset) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otOperationalDataset) + sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendActiveSet(
                    pFilter->otCtx,
                    aDataset,
                    aTlvTypes,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendPendingGet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint8_t))
    {
        uint8_t aLength = *(uint8_t*)InBuffer;
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(uint8_t);

        if (InBufferLength >= sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendPendingGet(
                    pFilter->otCtx,
                    aTlvTypes,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendPendingSet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(otOperationalDataset) + sizeof(uint8_t))
    {
        const otOperationalDataset *aDataset = (otOperationalDataset*)InBuffer;
        uint8_t aLength = *(uint8_t*)(InBuffer + sizeof(otOperationalDataset));
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(otOperationalDataset) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otOperationalDataset) + sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendPendingSet(
                    pFilter->otCtx,
                    aDataset,
                    aTlvTypes,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendMgmtCommissionerGet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(uint8_t))
    {
        uint8_t aLength = *(uint8_t*)InBuffer;
        PUCHAR aTlvs = aLength == 0 ? NULL : InBuffer + sizeof(uint8_t);

        if (InBufferLength >= sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendMgmtCommissionerGet(
                    pFilter->otCtx,
                    aTlvs,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otSendMgmtCommissionerSet(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    
    *OutBufferLength = 0;
    UNREFERENCED_PARAMETER(OutBuffer);

    if (InBufferLength >= sizeof(otCommissioningDataset) + sizeof(uint8_t))
    {
        const otCommissioningDataset *aDataset = (otCommissioningDataset*)InBuffer;
        uint8_t aLength = *(uint8_t*)(InBuffer + sizeof(otCommissioningDataset));
        PUCHAR aTlvs = aLength == 0 ? NULL : InBuffer + sizeof(otCommissioningDataset) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otCommissioningDataset) + sizeof(uint8_t) + aLength)
        {
            status = ThreadErrorToNtstatus(
                otSendMgmtCommissionerSet(
                    pFilter->otCtx,
                    aDataset,
                    aTlvs,
                    aLength)
                );
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otKeySwitchGuardtime(
    _In_ PMS_FILTER         pFilter,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID           OutBuffer,
    _Inout_ PULONG          OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        otSetKeySwitchGuardTime(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otGetKeySwitchGuardTime(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(uint32_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}
