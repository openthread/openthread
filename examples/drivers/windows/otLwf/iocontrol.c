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

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl(
    _In_ PMS_FILTER     pFilter,
    _In_ PIRP           Irp
    );

typedef struct _OTLWF_IOCTL_HANDLER
{
    const char*             Name;
    OTLWF_OT_IOCTL_FUNC*    otFunc;
    OTLWF_TUN_IOCTL_FUNC*   tunFunc;
} OTLWF_IOCTL_HANDLER;

OTLWF_IOCTL_HANDLER IoCtls[] =
{
    { "IOCTL_OTLWF_OT_ENABLED",                     NULL },
    { "IOCTL_OTLWF_OT_INTERFACE",                   REF_IOCTL_FUNC_WITH_TUN(otInterface) },
    { "IOCTL_OTLWF_OT_THREAD",                      REF_IOCTL_FUNC_WITH_TUN(otThread) },
    { "IOCTL_OTLWF_OT_ACTIVE_SCAN",                 REF_IOCTL_FUNC_WITH_TUN(otActiveScan) },
    { "IOCTL_OTLWF_OT_DISCOVER",                    REF_IOCTL_FUNC(otDiscover) },
    { "IOCTL_OTLWF_OT_CHANNEL",                     REF_IOCTL_FUNC_WITH_TUN(otChannel) },
    { "IOCTL_OTLWF_OT_CHILD_TIMEOUT",               REF_IOCTL_FUNC_WITH_TUN(otChildTimeout) },
    { "IOCTL_OTLWF_OT_EXTENDED_ADDRESS",            REF_IOCTL_FUNC_WITH_TUN(otExtendedAddress) },
    { "IOCTL_OTLWF_OT_EXTENDED_PANID",              REF_IOCTL_FUNC_WITH_TUN(otExtendedPanId) },
    { "IOCTL_OTLWF_OT_LEADER_RLOC",                 REF_IOCTL_FUNC_WITH_TUN(otLeaderRloc) },
    { "IOCTL_OTLWF_OT_LINK_MODE",                   REF_IOCTL_FUNC_WITH_TUN(otLinkMode) },
    { "IOCTL_OTLWF_OT_MASTER_KEY",                  REF_IOCTL_FUNC_WITH_TUN(otMasterKey) },
    { "IOCTL_OTLWF_OT_MESH_LOCAL_EID",              REF_IOCTL_FUNC_WITH_TUN(otMeshLocalEid) },
    { "IOCTL_OTLWF_OT_MESH_LOCAL_PREFIX",           REF_IOCTL_FUNC_WITH_TUN(otMeshLocalPrefix) },
    { "IOCTL_OTLWF_OT_NETWORK_DATA_LEADER",         NULL },
    { "IOCTL_OTLWF_OT_NETWORK_DATA_LOCAL",          NULL },
    { "IOCTL_OTLWF_OT_NETWORK_NAME",                REF_IOCTL_FUNC_WITH_TUN(otNetworkName) },
    { "IOCTL_OTLWF_OT_PAN_ID",                      REF_IOCTL_FUNC_WITH_TUN(otPanId) },
    { "IOCTL_OTLWF_OT_ROUTER_ROLL_ENABLED",         REF_IOCTL_FUNC_WITH_TUN(otRouterRollEnabled) },
    { "IOCTL_OTLWF_OT_SHORT_ADDRESS",               REF_IOCTL_FUNC_WITH_TUN(otShortAddress) },
    { "IOCTL_OTLWF_OT_UNICAST_ADDRESSES",           NULL },
    { "IOCTL_OTLWF_OT_ACTIVE_DATASET",              REF_IOCTL_FUNC(otActiveDataset) },
    { "IOCTL_OTLWF_OT_PENDING_DATASET",             REF_IOCTL_FUNC(otPendingDataset) },
    { "IOCTL_OTLWF_OT_LOCAL_LEADER_WEIGHT",         REF_IOCTL_FUNC_WITH_TUN(otLocalLeaderWeight) },
    { "IOCTL_OTLWF_OT_ADD_BORDER_ROUTER",           REF_IOCTL_FUNC_WITH_TUN(otAddBorderRouter) },
    { "IOCTL_OTLWF_OT_REMOVE_BORDER_ROUTER",        REF_IOCTL_FUNC_WITH_TUN(otRemoveBorderRouter) },
    { "IOCTL_OTLWF_OT_ADD_EXTERNAL_ROUTE",          REF_IOCTL_FUNC_WITH_TUN(otAddExternalRoute) },
    { "IOCTL_OTLWF_OT_REMOVE_EXTERNAL_ROUTE",       REF_IOCTL_FUNC_WITH_TUN(otRemoveExternalRoute) },
    { "IOCTL_OTLWF_OT_SEND_SERVER_DATA",            REF_IOCTL_FUNC(otSendServerData) },
    { "IOCTL_OTLWF_OT_CONTEXT_ID_REUSE_DELAY",      REF_IOCTL_FUNC_WITH_TUN(otContextIdReuseDelay) },
    { "IOCTL_OTLWF_OT_KEY_SEQUENCE_COUNTER",        REF_IOCTL_FUNC_WITH_TUN(otKeySequenceCounter) },
    { "IOCTL_OTLWF_OT_NETWORK_ID_TIMEOUT",          REF_IOCTL_FUNC_WITH_TUN(otNetworkIdTimeout) },
    { "IOCTL_OTLWF_OT_ROUTER_UPGRADE_THRESHOLD",    REF_IOCTL_FUNC_WITH_TUN(otRouterUpgradeThreshold) },
    { "IOCTL_OTLWF_OT_RELEASE_ROUTER_ID",           REF_IOCTL_FUNC_WITH_TUN(otReleaseRouterId) },
    { "IOCTL_OTLWF_OT_MAC_WHITELIST_ENABLED",       REF_IOCTL_FUNC_WITH_TUN(otMacWhitelistEnabled) },
    { "IOCTL_OTLWF_OT_ADD_MAC_WHITELIST",           REF_IOCTL_FUNC_WITH_TUN(otAddMacWhitelist) },
    { "IOCTL_OTLWF_OT_REMOVE_MAC_WHITELIST",        REF_IOCTL_FUNC_WITH_TUN(otRemoveMacWhitelist) },
    { "IOCTL_OTLWF_OT_NEXT_MAC_WHITELIST",          REF_IOCTL_FUNC(otNextMacWhitelist) },
    { "IOCTL_OTLWF_OT_CLEAR_MAC_WHITELIST",         REF_IOCTL_FUNC_WITH_TUN(otClearMacWhitelist) },
    { "IOCTL_OTLWF_OT_DEVICE_ROLE",                 REF_IOCTL_FUNC_WITH_TUN(otDeviceRole) },
    { "IOCTL_OTLWF_OT_CHILD_INFO_BY_ID",            REF_IOCTL_FUNC(otChildInfoById) },
    { "IOCTL_OTLWF_OT_CHILD_INFO_BY_INDEX",         REF_IOCTL_FUNC(otChildInfoByIndex) },
    { "IOCTL_OTLWF_OT_EID_CACHE_ENTRY",             REF_IOCTL_FUNC(otEidCacheEntry) },
    { "IOCTL_OTLWF_OT_LEADER_DATA",                 REF_IOCTL_FUNC(otLeaderData) },
    { "IOCTL_OTLWF_OT_LEADER_ROUTER_ID",            REF_IOCTL_FUNC_WITH_TUN(otLeaderRouterId) },
    { "IOCTL_OTLWF_OT_LEADER_WEIGHT",               REF_IOCTL_FUNC_WITH_TUN(otLeaderWeight) },
    { "IOCTL_OTLWF_OT_NETWORK_DATA_VERSION",        REF_IOCTL_FUNC_WITH_TUN(otNetworkDataVersion) },
    { "IOCTL_OTLWF_OT_PARTITION_ID",                REF_IOCTL_FUNC_WITH_TUN(otPartitionId) },
    { "IOCTL_OTLWF_OT_RLOC16",                      REF_IOCTL_FUNC_WITH_TUN(otRloc16) },
    { "IOCTL_OTLWF_OT_ROUTER_ID_SEQUENCE",          REF_IOCTL_FUNC(otRouterIdSequence) },
    { "IOCTL_OTLWF_OT_MAX_ROUTER_ID",               REF_IOCTL_FUNC(otMaxRouterId) },
    { "IOCTL_OTLWF_OT_ROUTER_INFO",                 REF_IOCTL_FUNC(otRouterInfo) },
    { "IOCTL_OTLWF_OT_STABLE_NETWORK_DATA_VERSION", REF_IOCTL_FUNC_WITH_TUN(otStableNetworkDataVersion) },
    { "IOCTL_OTLWF_OT_MAC_BLACKLIST_ENABLED",       REF_IOCTL_FUNC(otMacBlacklistEnabled) },
    { "IOCTL_OTLWF_OT_ADD_MAC_BLACKLIST",           REF_IOCTL_FUNC(otAddMacBlacklist) },
    { "IOCTL_OTLWF_OT_REMOVE_MAC_BLACKLIST",        REF_IOCTL_FUNC(otRemoveMacBlacklist) },
    { "IOCTL_OTLWF_OT_NEXT_MAC_BLACKLIST",          REF_IOCTL_FUNC(otNextMacBlacklist) },
    { "IOCTL_OTLWF_OT_CLEAR_MAC_BLACKLIST",         REF_IOCTL_FUNC(otClearMacBlacklist) },
    { "IOCTL_OTLWF_OT_TRANSMIT_POWER",              REF_IOCTL_FUNC(otTransmitPower) },
    { "IOCTL_OTLWF_OT_NEXT_ON_MESH_PREFIX",         REF_IOCTL_FUNC(otNextOnMeshPrefix) },
    { "IOCTL_OTLWF_OT_POLL_PERIOD",                 REF_IOCTL_FUNC(otPollPeriod) },
    { "IOCTL_OTLWF_OT_LOCAL_LEADER_PARTITION_ID",   REF_IOCTL_FUNC(otLocalLeaderPartitionId) },
    { "IOCTL_OTLWF_OT_PLATFORM_RESET",              REF_IOCTL_FUNC_WITH_TUN(otPlatformReset) },
    { "IOCTL_OTLWF_OT_PARENT_INFO",                 REF_IOCTL_FUNC_WITH_TUN(otParentInfo) },
    { "IOCTL_OTLWF_OT_SINGLETON",                   REF_IOCTL_FUNC(otSingleton) },
    { "IOCTL_OTLWF_OT_MAC_COUNTERS",                REF_IOCTL_FUNC(otMacCounters) },
    { "IOCTL_OTLWF_OT_MAX_CHILDREN",                REF_IOCTL_FUNC_WITH_TUN(otMaxChildren) },
    { "IOCTL_OTLWF_OT_COMMISIONER_START",           REF_IOCTL_FUNC(otCommissionerStart) },
    { "IOCTL_OTLWF_OT_COMMISIONER_STOP",            REF_IOCTL_FUNC(otCommissionerStop) },
    { "IOCTL_OTLWF_OT_JOINER_START",                REF_IOCTL_FUNC(otJoinerStart) },
    { "IOCTL_OTLWF_OT_JOINER_STOP",                 REF_IOCTL_FUNC(otJoinerStop) },
    { "IOCTL_OTLWF_OT_FACTORY_EUI64",               REF_IOCTL_FUNC(otFactoryAssignedIeeeEui64) },
    { "IOCTL_OTLWF_OT_JOINER_ID",                   REF_IOCTL_FUNC(otJoinerId) },
    { "IOCTL_OTLWF_OT_ROUTER_DOWNGRADE_THRESHOLD",  REF_IOCTL_FUNC_WITH_TUN(otRouterDowngradeThreshold) },
    { "IOCTL_OTLWF_OT_COMMISSIONER_PANID_QUERY",    REF_IOCTL_FUNC(otCommissionerPanIdQuery) },
    { "IOCTL_OTLWF_OT_COMMISSIONER_ENERGY_SCAN",    REF_IOCTL_FUNC(otCommissionerEnergyScan) },
    { "IOCTL_OTLWF_OT_ROUTER_SELECTION_JITTER",     REF_IOCTL_FUNC_WITH_TUN(otRouterSelectionJitter) },
    { "IOCTL_OTLWF_OT_JOINER_UDP_PORT",             REF_IOCTL_FUNC(otJoinerUdpPort) },
    { "IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_GET",         REF_IOCTL_FUNC(otSendDiagnosticGet) },
    { "IOCTL_OTLWF_OT_SEND_DIAGNOSTIC_RESET",       REF_IOCTL_FUNC(otSendDiagnosticReset) },
    { "IOCTL_OTLWF_OT_COMMISIONER_ADD_JOINER",      REF_IOCTL_FUNC(otCommissionerAddJoiner) },
    { "IOCTL_OTLWF_OT_COMMISIONER_REMOVE_JOINER",   REF_IOCTL_FUNC(otCommissionerRemoveJoiner) },
    { "IOCTL_OTLWF_OT_COMMISIONER_PROVISIONING_URL", REF_IOCTL_FUNC(otCommissionerProvisioningUrl) },
    { "IOCTL_OTLWF_OT_COMMISIONER_ANNOUNCE_BEGIN",  REF_IOCTL_FUNC(otCommissionerAnnounceBegin) },
    { "IOCTL_OTLWF_OT_ENERGY_SCAN",                 REF_IOCTL_FUNC_WITH_TUN(otEnergyScan) },
    { "IOCTL_OTLWF_OT_SEND_ACTIVE_GET",             REF_IOCTL_FUNC(otSendActiveGet) },
    { "IOCTL_OTLWF_OT_SEND_ACTIVE_SET",             REF_IOCTL_FUNC(otSendActiveSet) },
    { "IOCTL_OTLWF_OT_SEND_PENDING_GET",            REF_IOCTL_FUNC(otSendPendingGet) },
    { "IOCTL_OTLWF_OT_SEND_PENDING_SET",            REF_IOCTL_FUNC(otSendPendingSet) },
    { "IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_GET",  REF_IOCTL_FUNC(otSendMgmtCommissionerGet) },
    { "IOCTL_OTLWF_OT_SEND_MGMT_COMMISSIONER_SET",  REF_IOCTL_FUNC(otSendMgmtCommissionerSet) },
    { "IOCTL_OTLWF_OT_KEY_SWITCH_GUARDTIME",        REF_IOCTL_FUNC_WITH_TUN(otKeySwitchGuardtime) },
    { "IOCTL_OTLWF_OT_FACTORY_RESET",               REF_IOCTL_FUNC(otFactoryReset) },
    { "IOCTL_OTLWF_OT_THREAD_AUTO_START",           REF_IOCTL_FUNC(otThreadAutoStart) },
    { "IOCTL_OTLWF_OT_PREFERRED_ROUTER_ID",         REF_IOCTL_FUNC(otThreadPreferredRouterId) },
    { "IOCTL_OTLWF_OT_PSKC",                        REF_IOCTL_FUNC_WITH_TUN(otPSKc) },
    { "IOCTL_OTLWF_OT_PARENT_PRIORITY",             REF_IOCTL_FUNC(otParentPriority) },
    { "IOCTL_OTLWF_OT_ADD_MAC_FIXED_RSS",           REF_IOCTL_FUNC_WITH_TUN(otAddMacFixedRss) },
    { "IOCTL_OTLWF_OT_REMOVE_MAC_FIXED_RSS",        REF_IOCTL_FUNC_WITH_TUN(otRemoveMacFixedRss) },
    { "IOCTL_OTLWF_OT_NEXT_MAC_FIXED_RSS",          REF_IOCTL_FUNC(otNextMacFixedRss) },
    { "IOCTL_OTLWF_OT_CLEAR_MAC_FIXED_RSS",         REF_IOCTL_FUNC_WITH_TUN(otClearMacFixedRss) },
    { "IOCTL_OTLWF_OT_NEXT_ROUTE",                  REF_IOCTL_FUNC(otNextRoute) },
};

// intentionally -1 in the end due to that IOCTL_OTLWF_OT_ASSIGN_LINK_QUALITY (#161) is removed now.
static_assert(ARRAYSIZE(IoCtls) == (MAX_OTLWF_IOCTL_FUNC_CODE - MIN_OTLWF_IOCTL_FUNC_CODE) + 1 - 1,
              "The IoCtl strings should be up to date with the actual IoCtl list.");

const char*
IoCtlString(
    ULONG IoControlCode
)
{
    ULONG FuncCode = ((IoControlCode >> 2) & 0xFFF) - 100;
    return FuncCode < ARRAYSIZE(IoCtls) ? IoCtls[FuncCode].Name : "UNKNOWN IOCTL";
}

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
    ExReleaseRundownProtection(&pFilter->ExternalRefs);

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

    if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
    {
        // Pend the Irp for processing on the OpenThread event processing thread
        otLwfEventProcessingIndicateIrp(pFilter, Irp);
    }
    else
    {
        status = otLwfTunIoCtl(pFilter, Irp);
    }

    // Release our ref on the filter
    ExReleaseRundownProtection(&pFilter->ExternalRefs);

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

    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    ULONG FuncCode = ((IoControlCode >> 2) & 0xFFF) - 100;
    if (FuncCode < ARRAYSIZE(IoCtls))
    {
        LogVerbose(DRIVER_IOCTL, "Processing Irp=%p, for %s (In:%u,Out:%u)",
                    Irp, IoCtls[FuncCode].Name, InBufferLength, OutBufferLength);

        if (IoCtls[FuncCode].otFunc)
        {
            status = IoCtls[FuncCode].otFunc(pFilter, InBuffer, InBufferLength, OutBuffer, &OutBufferLength);
        }
        else
        {
            OutBufferLength = 0;
        }

        LogVerbose(DRIVER_IOCTL, "Completing Irp=%p, with %!STATUS! for %s (Out:%u)",
                    Irp, status, IoCtls[FuncCode].Name, OutBufferLength);
    }
    else
    {
        OutBufferLength = 0;
    }

    // Clear any leftover output buffer
    if (OutBufferLength < OrigOutBufferLength)
    {
        RtlZeroMemory((PUCHAR)OutBuffer + OutBufferLength, OrigOutBufferLength - OutBufferLength);
    }

    // Complete the IRP
    Irp->IoStatus.Information = OutBufferLength;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

// Handles Irp for IOTCLs for OpenThread control on the OpenThread thread
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl(
    _In_ PMS_FILTER     pFilter,
    _In_ PIRP           Irp
    )
{
    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PUCHAR InBuffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer + sizeof(GUID);
    ULONG InBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength - sizeof(GUID);
    ULONG OutBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    ULONG FuncCode = ((IoControlCode >> 2) & 0xFFF) - 100;
    if (FuncCode < ARRAYSIZE(IoCtls))
    {
        LogVerbose(DRIVER_IOCTL, "Processing Irp=%p, for %s (In:%u,Out:%u)",
                    Irp, IoCtls[FuncCode].Name, InBufferLength, OutBufferLength);

        if (IoCtls[FuncCode].tunFunc)
        {
            status = IoCtls[FuncCode].tunFunc(pFilter, Irp, InBuffer, InBufferLength, OutBufferLength);
        }

        if (!NT_SUCCESS(status))
        {
            LogVerbose(DRIVER_IOCTL, "Completing Irp=%p, with %!STATUS! for %s",
                        Irp, status, IoCtls[FuncCode].Name);
        }
    }

    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;

        // Mark the Irp as pending
        IoMarkIrpPending(Irp);
    }

    return status;
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
            otLwfRadioAddressesUpdated(pFilter);
	}

        status = ThreadErrorToNtstatus(otIp6SetEnabled(pFilter->otCtx, IsEnabled));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otIp6IsEnabled(pFilter->otCtx) ? TRUE : FALSE;
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
otLwfTunIoCtl_otInterface(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
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

            // Sync the current addresses
            KeSetEvent(&pFilter->TunWorkerThreadAddressChangedEvent, IO_NO_INCREMENT, FALSE);
        }

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_IF_UP,
                sizeof(BOOLEAN),
                SPINEL_DATATYPE_BOOL_S,
                IsEnabled);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otInterface_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_IF_UP,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otInterface_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_IF_UP)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_BOOL_S, (BOOLEAN*)OutBuffer))
        {
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
        }
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

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        BOOLEAN IsEnabled = *(BOOLEAN*)InBuffer;
        status = ThreadErrorToNtstatus(otThreadSetEnabled(pFilter->otCtx, IsEnabled));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = (otThreadGetDeviceRole(pFilter->otCtx) > OT_DEVICE_ROLE_DISABLED) ? TRUE : FALSE;
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
otLwfTunIoCtl_otThread(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_STACK_UP,
                sizeof(BOOLEAN),
                SPINEL_DATATYPE_BOOL_S,
                *(BOOLEAN*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otThread_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_STACK_UP,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otThread_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_STACK_UP)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_BOOL_S, (BOOLEAN*)OutBuffer))
        {
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
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
            otLinkActiveScan(
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
        *(BOOLEAN*)OutBuffer = otLinkIsActiveScanInProgress(pFilter->otCtx) ? TRUE : FALSE;
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
otLwfTunIoCtl_otActiveScan(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        uint8_t aScanState = SPINEL_SCAN_STATE_BEACON;

        // TODO - Send down scan channel & duration first
        UNREFERENCED_PARAMETER(aScanChannels);
        status = otLwfCmdSetProp(pFilter, SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_UINT16_S, aScanDuration);
        if (!NT_SUCCESS(status)) goto error;

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_MAC_SCAN_STATE,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                aScanState);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otActiveScan_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_MAC_SCAN_STATE,
                0,
                NULL);
    }

error:

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otActiveScan_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_MAC_SCAN_STATE)
    {
        uint8_t aScanState = 0;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, &aScanState))
        {
            *(BOOLEAN*)OutBuffer = (aScanState == SPINEL_SCAN_STATE_BEACON) ? TRUE : FALSE;
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
        }
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
            otLinkEnergyScan(
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
        *(BOOLEAN*)OutBuffer = otLinkIsEnergyScanInProgress(pFilter->otCtx) ? TRUE : FALSE;
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
otLwfTunIoCtl_otEnergyScan(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aScanDuration = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        uint8_t aScanState = SPINEL_SCAN_STATE_ENERGY;

        // TODO - Send down scan channel & duration first
        UNREFERENCED_PARAMETER(aScanChannels);
        status = otLwfCmdSetProp(pFilter, SPINEL_PROP_MAC_SCAN_MASK, SPINEL_DATATYPE_UINT16_S, aScanDuration);
        if (!NT_SUCCESS(status)) goto error;

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_MAC_SCAN_STATE,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                aScanState);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otEnergyScan_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_MAC_SCAN_STATE,
                0,
                NULL);
    }

error:

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otEnergyScan_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_MAC_SCAN_STATE)
    {
        uint8_t aScanState = 0;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, &aScanState))
        {
            *(BOOLEAN*)OutBuffer = (aScanState == SPINEL_SCAN_STATE_ENERGY) ? TRUE : FALSE;
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
        }
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

    if (InBufferLength >= sizeof(uint32_t) + sizeof(uint16_t) + sizeof(bool))
    {
        uint32_t aScanChannels = *(uint32_t*)InBuffer;
        uint16_t aPanid = *(uint16_t*)(InBuffer + sizeof(uint32_t));
        bool aJoiner = *(uint8_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint16_t));
        bool aEnableEui64Filtering = *(uint8_t*)(InBuffer + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t));
        status = ThreadErrorToNtstatus(
            otThreadDiscover(
                pFilter->otCtx,
                aScanChannels,
                aPanid,
                aJoiner,
                aEnableEui64Filtering,
                otLwfDiscoverCallback,
                pFilter)
            );
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otThreadIsDiscoverInProgress(pFilter->otCtx) ? TRUE : FALSE;
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
        status = ThreadErrorToNtstatus(otLinkSetChannel(pFilter->otCtx, *(uint8_t*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otLinkGetChannel(pFilter->otCtx);
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
otLwfTunIoCtl_otChannel(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_PHY_CHAN,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otChannel_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_PHY_CHAN,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otChannel_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_PHY_CHAN)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetChildTimeout(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otThreadGetChildTimeout(pFilter->otCtx);
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
otLwfTunIoCtl_otChildTimeout(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_CHILD_TIMEOUT,
                sizeof(uint32_t),
                SPINEL_DATATYPE_UINT32_S,
                *(uint32_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otChildTimeout_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_CHILD_TIMEOUT,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otChildTimeout_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_CHILD_TIMEOUT)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT32_S, (uint32_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint32_t);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otLinkSetExtendedAddress(pFilter->otCtx, (otExtAddress*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otExtAddress))
    {
        memcpy(OutBuffer, otLinkGetExtendedAddress(pFilter->otCtx), sizeof(otExtAddress));
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
otLwfTunIoCtl_otExtendedAddress(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otExtAddress))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_HWADDR,
                sizeof(otExtAddress),
                SPINEL_DATATYPE_EUI64_S,
                (otExtAddress*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(otExtAddress))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otExtendedAddress_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_HWADDR,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otExtendedAddress_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_HWADDR)
    {
        spinel_eui64_t *data = NULL;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_EUI64_S, &data) && data != NULL)
        {
            memcpy(OutBuffer, data, sizeof(otExtAddress));
            *OutBufferLength = sizeof(otExtAddress);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otThreadSetExtendedPanId(pFilter->otCtx, (otExtendedPanId*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otExtendedPanId))
    {
        const otExtendedPanId* aExtendedPanId = otThreadGetExtendedPanId(pFilter->otCtx);
        memcpy(OutBuffer, aExtendedPanId, sizeof(otExtendedPanId));
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
otLwfTunIoCtl_otExtendedPanId(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otExtendedPanId))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_XPANID,
                sizeof(otExtendedPanId) + sizeof(uint16_t),
                SPINEL_DATATYPE_DATA_S,
                (otExtendedPanId*)InBuffer,
                sizeof(otExtendedPanId));
    }
    else if (OutBufferLength >= sizeof(otExtendedPanId))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otExtendedPanId_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_XPANID,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otExtendedPanId_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_XPANID)
    {
        uint8_t *data = NULL;
        spinel_size_t aExtPanIdLen;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_DATA_S, &data, &aExtPanIdLen) && data != NULL &&
            aExtPanIdLen == sizeof(otExtendedPanId))
        {
            memcpy(OutBuffer, data, sizeof(otExtendedPanId));
            *OutBufferLength = sizeof(otExtendedPanId);
            status = STATUS_SUCCESS;
        }
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

    if (*OutBufferLength >= sizeof(otExtAddress))
    {
        otLinkGetFactoryAssignedIeeeEui64(pFilter->otCtx, (otExtAddress*)OutBuffer);
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
otLwfIoCtl_otJoinerId(
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
        status = ThreadErrorToNtstatus(otJoinerGetId(pFilter->otCtx, (otExtAddress*)OutBuffer));
        *OutBufferLength = sizeof(otExtAddress);
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
        status = ThreadErrorToNtstatus(otThreadGetLeaderRloc(pFilter->otCtx, (otIp6Address*)OutBuffer));
        *OutBufferLength = sizeof(otIp6Address);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLeaderRloc(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otIp6Address))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_LEADER_ADDR,
                sizeof(otIp6Address),
                SPINEL_DATATYPE_IPv6ADDR_S,
                (otIp6Address*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(otIp6Address))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otLeaderRloc_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_LEADER_ADDR,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLeaderRloc_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_LEADER_ADDR)
    {
        spinel_ipv6addr_t *data = NULL;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_IPv6ADDR_S, &data) && data != NULL)
        {
            memcpy(OutBuffer, data, sizeof(spinel_ipv6addr_t));
            *OutBufferLength = sizeof(otIp6Address);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otThreadSetLinkMode(pFilter->otCtx, *(otLinkModeConfig*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otLinkModeConfig))
    {
        *(otLinkModeConfig*)OutBuffer = otThreadGetLinkMode(pFilter->otCtx);
        *OutBufferLength = sizeof(otLinkModeConfig);
        status = STATUS_SUCCESS;
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

enum
{
    kThreadMode_RxOnWhenIdle        = (1 << 3),
    kThreadMode_SecureDataRequest   = (1 << 2),
    kThreadMode_FullThreadDevice  = (1 << 1),
    kThreadMode_FullNetworkData     = (1 << 0),
};

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLinkMode(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otLinkModeConfig))
    {
        const otLinkModeConfig* aLinkMode = (otLinkModeConfig*)InBuffer;
        uint8_t numeric_mode = 0;

        if (aLinkMode->mRxOnWhenIdle)       numeric_mode |= kThreadMode_RxOnWhenIdle;
        if (aLinkMode->mSecureDataRequests) numeric_mode |= kThreadMode_SecureDataRequest;
        if (aLinkMode->mDeviceType)         numeric_mode |= kThreadMode_FullThreadDevice;
        if (aLinkMode->mNetworkData)        numeric_mode |= kThreadMode_FullNetworkData;

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_MODE,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                numeric_mode);
    }
    else if (OutBufferLength >= sizeof(otLinkModeConfig))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otLinkMode_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_MODE,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLinkMode_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_MODE)
    {
		uint8_t numeric_mode = 0;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, &numeric_mode))
        {
            otLinkModeConfig* aLinkMode = (otLinkModeConfig*)OutBuffer;

            aLinkMode->mRxOnWhenIdle = ((numeric_mode & kThreadMode_RxOnWhenIdle) == kThreadMode_RxOnWhenIdle);
            aLinkMode->mSecureDataRequests = ((numeric_mode & kThreadMode_SecureDataRequest) == kThreadMode_SecureDataRequest);
            aLinkMode->mDeviceType = ((numeric_mode & kThreadMode_FullThreadDevice) == kThreadMode_FullThreadDevice);
            aLinkMode->mNetworkData = ((numeric_mode & kThreadMode_FullNetworkData) == kThreadMode_FullNetworkData);

            *OutBufferLength = sizeof(otLinkModeConfig);
            status = STATUS_SUCCESS;
        }
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

    if (InBufferLength >= sizeof(otMasterKey))
    {
        status = ThreadErrorToNtstatus(otThreadSetMasterKey(pFilter->otCtx, (otMasterKey*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otMasterKey))
    {
        const otMasterKey* aMasterKey = otThreadGetMasterKey(pFilter->otCtx);
        memcpy(OutBuffer, aMasterKey, sizeof(otMasterKey));
        *OutBufferLength = sizeof(otMasterKey);
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
otLwfTunIoCtl_otMasterKey(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otMasterKey))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_MASTER_KEY,
                sizeof(otMasterKey) + sizeof(uint16_t),
                SPINEL_DATATYPE_DATA_S,
                (otMasterKey*)InBuffer,
                sizeof(otMasterKey));
    }
    else if (OutBufferLength >= sizeof(otMasterKey))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otMasterKey_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_MASTER_KEY,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otMasterKey_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_MASTER_KEY)
    {
        uint8_t *data = NULL;
        spinel_size_t aKeyLength;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_DATA_S, &data, &aKeyLength) && data != NULL &&
            aKeyLength == sizeof(otMasterKey))
        {
            memcpy(OutBuffer, data, sizeof(otMasterKey));
            *OutBufferLength = sizeof(otMasterKey);
            status = STATUS_SUCCESS;
        }
    }
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otPSKc(
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

    if (InBufferLength >= sizeof(otPSKc))
    {
        status = ThreadErrorToNtstatus(otThreadSetPSKc(pFilter->otCtx, InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otPSKc))
    {
        const uint8_t* aPSKc = otThreadGetPSKc(pFilter->otCtx);
        memcpy(OutBuffer, aPSKc, sizeof(otPSKc));
        *OutBufferLength = sizeof(otPSKc);
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
otLwfTunIoCtl_otPSKc(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otPSKc))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_PSKC,
                sizeof(otPSKc) + sizeof(uint16_t),
                SPINEL_DATATYPE_DATA_S,
                (otPSKc*)InBuffer,
                OT_PSKC_MAX_SIZE);
    }
    else if (OutBufferLength >= sizeof(otPSKc))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otPSKc_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_PSKC,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otPSKc_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_PSKC)
    {
        uint8_t *data = NULL;
        spinel_size_t aPSKcLength;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_DATA_S, &data, &aPSKcLength) && data != NULL &&
            aPSKcLength == sizeof(otPSKc))
        {
            memcpy(OutBuffer, data, sizeof(otPSKc));
            *OutBufferLength = sizeof(otPSKc);
            status = STATUS_SUCCESS;
        }
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
        memcpy(OutBuffer,  otThreadGetMeshLocalEid(pFilter->otCtx), sizeof(otIp6Address));
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
otLwfTunIoCtl_otMeshLocalEid(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(otIp6Address))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otMeshLocalEid_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_IPV6_ML_ADDR,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otMeshLocalEid_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_IPV6_ML_ADDR)
    {
        spinel_ipv6addr_t *data = NULL;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_IPv6ADDR_S, &data) && data != NULL)
        {
            memcpy(OutBuffer, data, sizeof(spinel_ipv6addr_t));
            *OutBufferLength = sizeof(otIp6Address);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otThreadSetMeshLocalPrefix(pFilter->otCtx, (otMeshLocalPrefix*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otMeshLocalPrefix))
    {
        const otMeshLocalPrefix* aMeshLocalPrefix = otThreadGetMeshLocalPrefix(pFilter->otCtx);
        memcpy(OutBuffer, aMeshLocalPrefix, sizeof(otMeshLocalPrefix));
        *OutBufferLength = sizeof(otMeshLocalPrefix);
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
otLwfTunIoCtl_otMeshLocalPrefix(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otMeshLocalPrefix))
    {
        otIp6Address aAddress = {0};
        memcpy(&aAddress, InBuffer, sizeof(otMeshLocalPrefix));

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_IPV6_ML_PREFIX,
                sizeof(otIp6Address),
                SPINEL_DATATYPE_IPv6ADDR_S,
                &aAddress);
    }
    else if (OutBufferLength >= sizeof(otMeshLocalPrefix))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otMeshLocalPrefix_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_IPV6_ML_PREFIX,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otMeshLocalPrefix_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_IPV6_ML_PREFIX)
    {
        if (DataLength >= sizeof(otMeshLocalPrefix))
        {
            memcpy(OutBuffer, Data, sizeof(otMeshLocalPrefix));
            *OutBufferLength = sizeof(otMeshLocalPrefix);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otThreadSetNetworkName(pFilter->otCtx, (char*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otNetworkName))
    {
        strcpy_s((char*)OutBuffer, sizeof(otNetworkName), otThreadGetNetworkName(pFilter->otCtx));
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
otLwfTunIoCtl_otNetworkName(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otNetworkName))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_NETWORK_NAME,
                sizeof(otIp6Address),
                SPINEL_DATATYPE_UTF8_S,
                (otNetworkName*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(otNetworkName))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otNetworkName_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_NETWORK_NAME,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otNetworkName_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_NETWORK_NAME)
    {
        const char *data = NULL;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UTF8_S, &data) && data != NULL)
        {
            strcpy_s(OutBuffer, sizeof(otNetworkName), data);
            *OutBufferLength = sizeof(otNetworkName);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otLinkSetPanId(pFilter->otCtx, *(otPanId*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otPanId))
    {
        *(otPanId*)OutBuffer = otLinkGetPanId(pFilter->otCtx);
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
otLwfTunIoCtl_otPanId(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(otPanId))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_MAC_15_4_PANID,
                sizeof(otPanId),
                SPINEL_DATATYPE_UINT16_S,
                *(otPanId*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(otPanId))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otPanId_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_MAC_15_4_PANID,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otPanId_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_MAC_15_4_PANID)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT16_S, (otPanId*)OutBuffer))
        {
            *OutBufferLength = sizeof(otPanId);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetRouterRoleEnabled(pFilter->otCtx, *(BOOLEAN*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otThreadIsRouterRoleEnabled(pFilter->otCtx) ? TRUE : FALSE;
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
otLwfTunIoCtl_otRouterRollEnabled(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED,
                sizeof(BOOLEAN),
                SPINEL_DATATYPE_BOOL_S,
                *(BOOLEAN*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otRouterRollEnabled_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRouterRollEnabled_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_ROUTER_ROLE_ENABLED)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_BOOL_S, (BOOLEAN*)OutBuffer))
        {
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
        }
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
        *(otShortAddress*)OutBuffer = otLinkGetShortAddress(pFilter->otCtx);
        *OutBufferLength = sizeof(otShortAddress);
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
otLwfTunIoCtl_otShortAddress(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(otShortAddress))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otShortAddress_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_MAC_15_4_SADDR,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otShortAddress_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_MAC_15_4_SADDR)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT16_S, (otShortAddress*)OutBuffer))
        {
            *OutBufferLength = sizeof(otShortAddress);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otDatasetSetActive(pFilter->otCtx, (otOperationalDataset*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otDatasetGetActive(pFilter->otCtx, (otOperationalDataset*)OutBuffer));
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
        status = ThreadErrorToNtstatus(otDatasetSetPending(pFilter->otCtx, (otOperationalDataset*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(otOperationalDataset))
    {
        status = ThreadErrorToNtstatus(otDatasetGetPending(pFilter->otCtx, (otOperationalDataset*)OutBuffer));
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
        otThreadSetLocalLeaderWeight(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetLeaderWeight(pFilter->otCtx);
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
otLwfTunIoCtl_otLocalLeaderWeight(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otLocalLeaderWeight_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLocalLeaderWeight_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_LOCAL_LEADER_WEIGHT)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otBorderRouterAddOnMeshPrefix(pFilter->otCtx, (otBorderRouterConfig*)InBuffer));
    }

    return status;
}

const uint8_t kPreferenceOffset = 6;
//const uint8_t kPreferenceMask = 3 << kPreferenceOffset;
const uint8_t kPreferredFlag = 1 << 5;
const uint8_t kSlaacFlag = 1 << 4;
const uint8_t kDhcpFlag = 1 << 3;
const uint8_t kConfigureFlag = 1 << 2;
const uint8_t kDefaultRouteFlag = 1 << 1;
const uint8_t kOnMeshFlag = 1 << 0;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otAddBorderRouter(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otBorderRouterConfig))
    {
        const otBorderRouterConfig *aConfig = (otBorderRouterConfig*)InBuffer;

        otIp6Address prefix = {0};
        memcpy_s(&prefix, sizeof(prefix), &aConfig->mPrefix.mPrefix, aConfig->mPrefix.mLength);

        uint8_t flags = (uint8_t)(aConfig->mPreference << kPreferenceOffset);
        if (aConfig->mSlaac)        flags |= kSlaacFlag;
        if (aConfig->mDhcp)         flags |= kDhcpFlag;
        if (aConfig->mConfigure)    flags |= kConfigureFlag;
        if (aConfig->mDefaultRoute) flags |= kDefaultRouteFlag;
        if (aConfig->mOnMesh)       flags |= kOnMeshFlag;

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_INSERT,
                SPINEL_PROP_THREAD_ON_MESH_NETS,
                sizeof(otIp6Address) + 3 * sizeof(uint8_t),
                "6CbC",
                &prefix,
                aConfig->mPrefix.mLength,
                aConfig->mStable,
                flags);
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
        status = ThreadErrorToNtstatus(otBorderRouterRemoveOnMeshPrefix(pFilter->otCtx, (otIp6Prefix*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRemoveBorderRouter(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otIp6Prefix))
    {
        const otIp6Prefix *aPrefix = (otIp6Prefix*)InBuffer;

        otIp6Address prefix = {0};
        memcpy_s(&prefix, sizeof(prefix), &aPrefix->mPrefix, aPrefix->mLength);

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_REMOVE,
                SPINEL_PROP_THREAD_ON_MESH_NETS,
                sizeof(otIp6Address) + sizeof(uint8_t),
                "6C",
                &prefix,
                aPrefix->mLength);
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
        status = ThreadErrorToNtstatus(otBorderRouterAddRoute(pFilter->otCtx, (otExternalRouteConfig*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otAddExternalRoute(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otBorderRouterConfig))
    {
        const otBorderRouterConfig *aConfig = (otBorderRouterConfig*)InBuffer;

        otIp6Address prefix = {0};
        memcpy_s(&prefix, sizeof(prefix), &aConfig->mPrefix.mPrefix, aConfig->mPrefix.mLength);

        uint8_t flags = (uint8_t)(aConfig->mPreference << kPreferenceOffset);
        if (aConfig->mSlaac)        flags |= kSlaacFlag;
        if (aConfig->mDhcp)         flags |= kDhcpFlag;
        if (aConfig->mConfigure)    flags |= kConfigureFlag;
        if (aConfig->mDefaultRoute) flags |= kDefaultRouteFlag;
        if (aConfig->mOnMesh)       flags |= kOnMeshFlag;

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_INSERT,
                SPINEL_PROP_THREAD_OFF_MESH_ROUTES,
                sizeof(otIp6Address) + 3 * sizeof(uint8_t),
                "6CbC",
                &prefix,
                aConfig->mPrefix.mLength,
                aConfig->mStable,
                flags);
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
        status = ThreadErrorToNtstatus(otBorderRouterRemoveRoute(pFilter->otCtx, (otIp6Prefix*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRemoveExternalRoute(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otIp6Prefix))
    {
        const otIp6Prefix *aPrefix = (otIp6Prefix*)InBuffer;

        otIp6Address prefix = {0};
        memcpy_s(&prefix, sizeof(prefix), &aPrefix->mPrefix, aPrefix->mLength);

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_REMOVE,
                SPINEL_PROP_THREAD_OFF_MESH_ROUTES,
                sizeof(otIp6Address) + sizeof(uint8_t),
                "6C",
                &prefix,
                aPrefix->mLength);
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

    status = ThreadErrorToNtstatus(otBorderRouterRegister(pFilter->otCtx));

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
        otThreadSetContextIdReuseDelay(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otThreadGetContextIdReuseDelay(pFilter->otCtx);
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
otLwfTunIoCtl_otContextIdReuseDelay(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY,
                sizeof(uint32_t),
                SPINEL_DATATYPE_UINT32_S,
                *(uint32_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otContextIdReuseDelay_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otContextIdReuseDelay_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_CONTEXT_REUSE_DELAY)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT32_S, (uint32_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint32_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetKeySequenceCounter(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otThreadGetKeySequenceCounter(pFilter->otCtx);
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
otLwfTunIoCtl_otKeySequenceCounter(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER,
                sizeof(uint32_t),
                SPINEL_DATATYPE_UINT32_S,
                *(uint32_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otKeySequenceCounter_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otKeySequenceCounter_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_KEY_SEQUENCE_COUNTER)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT32_S, (uint32_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint32_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetNetworkIdTimeout(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetNetworkIdTimeout(pFilter->otCtx);
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
otLwfTunIoCtl_otNetworkIdTimeout(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otNetworkIdTimeout_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otNetworkIdTimeout_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_NETWORK_ID_TIMEOUT)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetRouterUpgradeThreshold(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetRouterUpgradeThreshold(pFilter->otCtx);
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
otLwfTunIoCtl_otRouterUpgradeThreshold(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otRouterUpgradeThreshold_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRouterUpgradeThreshold_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_ROUTER_UPGRADE_THRESHOLD)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetRouterDowngradeThreshold(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetRouterDowngradeThreshold(pFilter->otCtx);
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
otLwfTunIoCtl_otRouterDowngradeThreshold(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otRouterDowngradeThreshold_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRouterDowngradeThreshold_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_ROUTER_DOWNGRADE_THRESHOLD)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        status = ThreadErrorToNtstatus(otThreadReleaseRouterId(pFilter->otCtx, *(uint8_t*)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otReleaseRouterId(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_REMOVE,
                SPINEL_PROP_THREAD_ACTIVE_ROUTER_IDS,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
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
        otMacFilterAddressMode mode =
            aEnabled ? OT_MAC_FILTER_ADDRESS_MODE_WHITELIST : OT_MAC_FILTER_ADDRESS_MODE_DISABLED;
        status = ThreadErrorToNtstatus(otLinkFilterSetAddressMode(pFilter->otCtx, mode));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        otMacFilterAddressMode mode = otLinkFilterGetAddressMode(pFilter->otCtx);
        *(BOOLEAN*)OutBuffer = (mode == OT_MAC_FILTER_ADDRESS_MODE_WHITELIST) ? TRUE : FALSE;
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
otLwfTunIoCtl_otMacWhitelistEnabled(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_MAC_WHITELIST_ENABLED,
                sizeof(BOOLEAN),
                SPINEL_DATATYPE_BOOL_S,
                *(BOOLEAN*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(BOOLEAN))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otMacWhitelistEnabled_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_MAC_WHITELIST_ENABLED,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otMacWhitelistEnabled_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_MAC_WHITELIST_ENABLED)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_BOOL_S, (BOOLEAN*)OutBuffer))
        {
            *OutBufferLength = sizeof(BOOLEAN);
            status = STATUS_SUCCESS;
        }
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
    int8_t aRss = OT_MAC_FILTER_FIXED_RSS_DISABLED;

    if (InBufferLength >= sizeof(otExtAddress) + sizeof(int8_t))
    {
         aRss = *(int8_t*)(InBuffer + sizeof(otExtAddress));
    }

    otError error = otLinkFilterAddAddress(pFilter->otCtx, (otExtAddress *)InBuffer);

    if ((error == OT_ERROR_NONE || error == OT_ERROR_ALREADY) &&
        (aRss != OT_MAC_FILTER_FIXED_RSS_DISABLED))
    {

       error = otLinkFilterAddRssIn(pFilter->otCtx, (otExtAddress *)InBuffer, aRss);
    }

    status = ThreadErrorToNtstatus(error);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otAddMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otExtAddress))
    {
        int8_t aRss = OT_MAC_FILTER_FIXED_RSS_DISABLED;
        if (InBufferLength >= sizeof(otExtAddress) + sizeof(int8_t))
        {
            aRss = *(int8_t*)(InBuffer + sizeof(otExtAddress));
        }

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_INSERT,
                SPINEL_PROP_MAC_WHITELIST,
                sizeof(otExtAddress) + sizeof(int8_t),
                "Ec",
                (otExtAddress*)InBuffer,
                &aRss);
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
        status = ThreadErrorToNtstatus(otLinkFilterRemoveAddress(pFilter->otCtx,
                    (otExtAddress *)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRemoveMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otExtAddress))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_REMOVE,
                SPINEL_PROP_MAC_WHITELIST,
                sizeof(otExtAddress),
                "E",
                (otExtAddress*)InBuffer);
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNextMacWhitelist(
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
        *OutBufferLength >= sizeof(uint8_t) + sizeof(otMacFilterEntry))
    {
        uint8_t aIterator = *(uint8_t*)(InBuffer);
        otMacFilterEntry *aEntry = (otMacFilterEntry*)((PUCHAR)OutBuffer + sizeof(uint8_t));

        status = ThreadErrorToNtstatus(
            otLinkFilterGetNextAddress(
                pFilter->otCtx,
                &aIterator,
                aEntry
                )
            );

        *OutBufferLength = sizeof(otMacFilterEntry) + sizeof(uint8_t);

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

    otLinkFilterClearAddresses(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otClearMacWhitelist(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBufferLength);

    status =
        otLwfTunSendCommandForIrp(
            pFilter,
            pIrp,
            NULL,
            SPINEL_CMD_PROP_VALUE_SET,
            SPINEL_PROP_MAC_WHITELIST,
            0,
            NULL);

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

        if (role == OT_DEVICE_ROLE_LEADER)
        {
            status = ThreadErrorToNtstatus(
                        otThreadBecomeLeader(pFilter->otCtx)
                        );
        }
        else if (role == OT_DEVICE_ROLE_ROUTER)
        {
            status = ThreadErrorToNtstatus(
                        otThreadBecomeRouter(pFilter->otCtx)
                        );
        }
        else if (role == OT_DEVICE_ROLE_CHILD)
        {
            status = ThreadErrorToNtstatus(
                        otThreadBecomeChild(pFilter->otCtx)
                        );
        }
        else if (role == OT_DEVICE_ROLE_DETACHED)
        {
            status = ThreadErrorToNtstatus(
                        otThreadBecomeDetached(pFilter->otCtx)
                        );
        }
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = (uint8_t)otThreadGetDeviceRole(pFilter->otCtx);
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
otLwfTunIoCtl_otDeviceRole(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        otDeviceRole role = *(uint8_t*)InBuffer;
        uint8_t spinel_role = SPINEL_NET_ROLE_DETACHED;

        switch (role)
        {
        case OT_DEVICE_ROLE_CHILD:
            spinel_role = SPINEL_NET_ROLE_CHILD;
            break;
        case OT_DEVICE_ROLE_ROUTER:
            spinel_role = SPINEL_NET_ROLE_ROUTER;
            break;
        case OT_DEVICE_ROLE_LEADER:
            spinel_role = SPINEL_NET_ROLE_LEADER;
            break;
        }

        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_ROLE,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                spinel_role);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otDeviceRole_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_ROLE,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otDeviceRole_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_ROLE)
    {
		uint8_t spinel_role = 0;
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, &spinel_role))
        {
            switch (spinel_role)
            {
            default:
            case SPINEL_NET_ROLE_DETACHED:
                *(uint8_t*)OutBuffer = OT_DEVICE_ROLE_DETACHED;
                break;
            case SPINEL_NET_ROLE_CHILD:
                *(uint8_t*)OutBuffer = OT_DEVICE_ROLE_CHILD;
                break;
            case SPINEL_NET_ROLE_ROUTER:
                *(uint8_t*)OutBuffer = OT_DEVICE_ROLE_ROUTER;
                break;
            case SPINEL_NET_ROLE_LEADER:
                *(uint8_t*)OutBuffer = OT_DEVICE_ROLE_LEADER;
                break;
            }

            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
            otThreadGetChildInfoById(
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
            otThreadGetChildInfoByIndex(
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
            otThreadGetEidCacheEntry(
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
        status = ThreadErrorToNtstatus(otThreadGetLeaderData(pFilter->otCtx, (otLeaderData*)OutBuffer));
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
        *(uint8_t*)OutBuffer = otThreadGetLeaderRouterId(pFilter->otCtx);
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
otLwfTunIoCtl_otLeaderRouterId(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otLeaderRouterId_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_LEADER_RID,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLeaderRouterId_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_LEADER_RID)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        *(uint8_t*)OutBuffer = otThreadGetLeaderWeight(pFilter->otCtx);
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
otLwfTunIoCtl_otLeaderWeight(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otLeaderWeight_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_LEADER_WEIGHT,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otLeaderWeight_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_LEADER_WEIGHT)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        *(uint8_t*)OutBuffer = otNetDataGetVersion(pFilter->otCtx);
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
otLwfTunIoCtl_otNetworkDataVersion(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otNetworkDataVersion_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_NETWORK_DATA_VERSION,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otNetworkDataVersion_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_NETWORK_DATA_VERSION)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        *(uint32_t*)OutBuffer = otThreadGetPartitionId(pFilter->otCtx);
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
otLwfTunIoCtl_otPartitionId(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otPartitionId_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_PARTITION_ID,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otPartitionId_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_PARTITION_ID)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT32_S, (uint32_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint32_t);
            status = STATUS_SUCCESS;
        }
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
        *(uint16_t*)OutBuffer = otThreadGetRloc16(pFilter->otCtx);
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
otLwfTunIoCtl_otRloc16(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint16_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otRloc16_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_RLOC16,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRloc16_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_RLOC16)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT16_S, (uint16_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint16_t);
            status = STATUS_SUCCESS;
        }
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
        *(uint8_t*)OutBuffer = otThreadGetRouterIdSequence(pFilter->otCtx);
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
otLwfIoCtl_otMaxRouterId(
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
        *(uint8_t*)OutBuffer = otThreadGetMaxRouterId(pFilter->otCtx);
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
            otThreadGetRouterInfo(
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
        *(uint8_t*)OutBuffer = otNetDataGetStableVersion(pFilter->otCtx);
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
otLwfTunIoCtl_otStableNetworkDataVersion(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otStableNetworkDataVersion_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otStableNetworkDataVersion_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_STABLE_NETWORK_DATA_VERSION)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        otMacFilterAddressMode mode =
            aEnabled ? OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST : OT_MAC_FILTER_ADDRESS_MODE_DISABLED;
        status = ThreadErrorToNtstatus(otLinkFilterSetAddressMode(pFilter->otCtx, mode));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        otMacFilterAddressMode mode = otLinkFilterGetAddressMode(pFilter->otCtx);
        *(BOOLEAN*)OutBuffer = mode == (OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST) ? TRUE : FALSE;
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
        status = ThreadErrorToNtstatus(otLinkFilterAddAddress(pFilter->otCtx,
                    (otExtAddress *)InBuffer));
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
        status = ThreadErrorToNtstatus(otLinkFilterRemoveAddress(pFilter->otCtx,
                    (otExtAddress *)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNextMacBlacklist(
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
        *OutBufferLength >= sizeof(uint8_t) + sizeof(otMacFilterEntry))
    {
        uint8_t aIterator = *(uint8_t*)(InBuffer);
        otMacFilterEntry *aEntry = (otMacFilterEntry*)((PUCHAR)OutBuffer + sizeof(uint8_t));

        status = ThreadErrorToNtstatus(
            otLinkFilterGetNextAddress(
                pFilter->otCtx,
                &aIterator,
                aEntry
                )
            );

        *OutBufferLength = sizeof(otMacFilterEntry) + sizeof(uint8_t);

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

    otLinkFilterClearAddresses(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otTransmitPower(
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
        status = ThreadErrorToNtstatus(otPlatRadioSetTransmitPower(pFilter->otCtx, *(int8_t*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(int8_t))
    {
        status = ThreadErrorToNtstatus(otPlatRadioGetTransmitPower(pFilter->otCtx, (int8_t*)OutBuffer));
        *OutBufferLength = sizeof(int8_t);
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

    if (InBufferLength >= sizeof(BOOLEAN) + sizeof(uint32_t) &&
        *OutBufferLength >= sizeof(uint32_t) + sizeof(otBorderRouterConfig))
    {
        BOOLEAN aLocal = *(BOOLEAN*)InBuffer;
        uint32_t aIterator = *(uint32_t*)(InBuffer + sizeof(BOOLEAN));
        otBorderRouterConfig* aConfig = (otBorderRouterConfig*)((PUCHAR)OutBuffer + sizeof(uint32_t));
        if (aLocal)
        {
            status = ThreadErrorToNtstatus(
                otBorderRouterGetNextOnMeshPrefix(
                    pFilter->otCtx,
                    &aIterator,
                    aConfig)
                );
        }
        else
        {
            status = ThreadErrorToNtstatus(
                otNetDataGetNextOnMeshPrefix(
                    pFilter->otCtx,
                    &aIterator,
                    aConfig)
                );
        }
        *OutBufferLength = sizeof(uint8_t) + sizeof(otBorderRouterConfig);
        if (status == STATUS_SUCCESS)
        {
            *(uint32_t*)OutBuffer = aIterator;
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
otLwfIoCtl_otNextRoute(
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

    if (InBufferLength >= sizeof(BOOLEAN) + sizeof(uint32_t) &&
        *OutBufferLength >= sizeof(uint32_t) + sizeof(otExternalRouteConfig))
    {
        BOOLEAN aLocal = *(BOOLEAN*)InBuffer;
        uint32_t aIterator = *(uint32_t*)(InBuffer + sizeof(BOOLEAN));
        otExternalRouteConfig* aConfig = (otExternalRouteConfig*)((PUCHAR)OutBuffer + sizeof(uint32_t));
        if (aLocal)
        {
            status = ThreadErrorToNtstatus(
                otBorderRouterGetNextRoute(
                    pFilter->otCtx,
                    &aIterator,
                    aConfig)
                );
        }
        else
        {
            status = ThreadErrorToNtstatus(
                otNetDataGetNextRoute(
                    pFilter->otCtx,
                    &aIterator,
                    aConfig)
                );
        }
        *OutBufferLength = sizeof(uint8_t) + sizeof(otExternalRouteConfig);
        if (status == STATUS_SUCCESS)
        {
            *(uint32_t*)OutBuffer = aIterator;
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
        status = ThreadErrorToNtstatus(otLinkSetPollPeriod(pFilter->otCtx, *(uint32_t*)InBuffer));
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otLinkGetPollPeriod(pFilter->otCtx);
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
        otThreadSetLocalLeaderPartitionId(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otThreadGetLocalLeaderPartitionId(pFilter->otCtx);
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

    otInstanceReset(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otFactoryReset(
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

    otInstanceFactoryReset(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otPlatformReset(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBufferLength);

    status =
        otLwfTunSendCommandForIrp(
            pFilter,
            pIrp,
            NULL,
            SPINEL_CMD_RESET,
            0,
            0,
            NULL);

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
        status = ThreadErrorToNtstatus(otThreadGetParentInfo(pFilter->otCtx, (otRouterInfo*)OutBuffer));
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
otLwfTunIoCtl_otParentInfo(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);

    if (OutBufferLength >= sizeof(otRouterInfo))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otParentInfo_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_PARENT,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otParentInfo_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_PARENT)
    {
        otRouterInfo* aRouterInfo = (otRouterInfo*)OutBuffer;
        RtlZeroMemory(aRouterInfo, sizeof(otRouterInfo));
        if (try_spinel_datatype_unpack(Data, DataLength, "ES", &aRouterInfo->mExtAddress.m8, &aRouterInfo->mRloc16))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        *(BOOLEAN*)OutBuffer = otThreadIsSingleton(pFilter->otCtx) ? TRUE : FALSE;
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
        memcpy_s(OutBuffer, *OutBufferLength, otLinkGetCounters(pFilter->otCtx), sizeof(otMacCounters));
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
        otThreadSetMaxAllowedChildren(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetMaxAllowedChildren(pFilter->otCtx);
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
otLwfTunIoCtl_otMaxChildren(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_CHILD_COUNT_MAX,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otMaxChildren_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_CHILD_COUNT_MAX,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otMaxChildren_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_CHILD_COUNT_MAX)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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

#define IsNotNullTerminated(buf) (strnlen(buf, sizeof(buf)) == sizeof(buf))

        if (IsNotNullTerminated(aConfig->PSKd) ||
            IsNotNullTerminated(aConfig->ProvisioningUrl) ||
            IsNotNullTerminated(aConfig->VendorName) ||
            IsNotNullTerminated(aConfig->VendorModel) ||
            IsNotNullTerminated(aConfig->VendorSwVersion) ||
            IsNotNullTerminated(aConfig->VendorData))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            strcpy_s(pFilter->otVendorName, sizeof(pFilter->otVendorName), aConfig->VendorName);
            strcpy_s(pFilter->otVendorModel, sizeof(pFilter->otVendorModel), aConfig->VendorModel);
            strcpy_s(pFilter->otVendorSwVersion, sizeof(pFilter->otVendorSwVersion), aConfig->VendorSwVersion);
            strcpy_s(pFilter->otVendorData, sizeof(pFilter->otVendorData), aConfig->VendorData);

            status = ThreadErrorToNtstatus(
                otJoinerStart(
                    pFilter->otCtx,
                    aConfig->PSKd,
                    aConfig->ProvisioningUrl,
                    pFilter->otVendorName,
                    pFilter->otVendorModel,
                    pFilter->otVendorSwVersion,
                    pFilter->otVendorData[0] == '\0' ? NULL : pFilter->otVendorData,
                    otLwfJoinerCallback,
                    pFilter)
                );
        }
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
        otThreadSetRouterSelectionJitter(pFilter->otCtx, *(uint8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint8_t))
    {
        *(uint8_t*)OutBuffer = otThreadGetRouterSelectionJitter(pFilter->otCtx);
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
otLwfTunIoCtl_otRouterSelectionJitter(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER,
                sizeof(uint8_t),
                SPINEL_DATATYPE_UINT8_S,
                *(uint8_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint8_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otRouterSelectionJitter_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRouterSelectionJitter_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_THREAD_ROUTER_SELECTION_JITTER)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT8_S, (uint8_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint8_t);
            status = STATUS_SUCCESS;
        }
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
        otThreadSetJoinerUdpPort(pFilter->otCtx, *(uint16_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint16_t))
    {
        *(uint16_t*)OutBuffer = otThreadGetJoinerUdpPort(pFilter->otCtx);
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
                otThreadSendDiagnosticGet(
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
                otThreadSendDiagnosticReset(
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
        const ULONG aPSKdBufferLength = InBufferLength - sizeof(uint8_t) - sizeof(otExtAddress) - sizeof(uint32_t);

        if (aPSKdBufferLength <= OPENTHREAD_PSK_MAX_LENGTH + 1)
        {
            uint8_t aExtAddressValid = *(uint8_t*)InBuffer;
            const otExtAddress *aExtAddress = aExtAddressValid == 0 ? NULL : (otExtAddress*)(InBuffer + sizeof(uint8_t));
            char *aPSKd = (char*)(InBuffer + sizeof(uint8_t) + sizeof(otExtAddress));
            uint32_t aTimeout = *(uint32_t*)(InBuffer + sizeof(uint8_t) + sizeof(otExtAddress) + aPSKdBufferLength);

            // Ensure aPSKd is NULL terminated in the buffer
            if (strnlen(aPSKd, aPSKdBufferLength) < aPSKdBufferLength)
            {
                status = ThreadErrorToNtstatus(otCommissionerAddJoiner(
                    pFilter->otCtx, aExtAddress, aPSKd, aTimeout));
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

    if (InBufferLength >= sizeof(otOperationalDatasetComponents) + sizeof(uint8_t))
    {
        const otOperationalDatasetComponents *aDatasetComp = (otOperationalDatasetComponents*)InBuffer;
        uint8_t aLength = *(uint8_t*)(InBuffer + sizeof(otOperationalDatasetComponents));
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(otOperationalDatasetComponents) + sizeof(uint8_t);

        if (InBufferLength >= sizeof(otOperationalDatasetComponents) + sizeof(uint8_t) + aLength)
        {
            otIp6Address *aAddress = NULL;
            if (InBufferLength >= sizeof(otOperationalDatasetComponents) + sizeof(uint8_t) + aLength + sizeof(otIp6Address))
                aAddress = (otIp6Address*)(InBuffer + sizeof(otOperationalDatasetComponents) + sizeof(uint8_t) + aLength);

            status = ThreadErrorToNtstatus(
                otDatasetSendMgmtActiveGet(
                    pFilter->otCtx,
                    aDatasetComp,
                    aTlvTypes,
                    aLength,
                    aAddress)
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
                otDatasetSendMgmtActiveSet(
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

    if (InBufferLength >= sizeof(otOperationalDataset) + sizeof(uint8_t))
    {
        const otOperationalDatasetComponents *aDatasetComp = (otOperationalDatasetComponents*)InBuffer;
        uint8_t aLength = *(uint8_t*)(InBuffer + sizeof(otOperationalDataset));
        PUCHAR aTlvTypes = aLength == 0 ? NULL : InBuffer + sizeof(otOperationalDataset) +  sizeof(uint8_t);

        if (InBufferLength >= sizeof(otOperationalDatasetComponents) + sizeof(uint8_t) + aLength)
        {
            otIp6Address *aAddress = NULL;
            if (InBufferLength >= sizeof(otOperationalDatasetComponents) + sizeof(uint8_t) + aLength + sizeof(otIp6Address))
                aAddress = (otIp6Address*)(InBuffer + sizeof(otOperationalDataset) + sizeof(uint8_t) + aLength);

            status = ThreadErrorToNtstatus(
                otDatasetSendMgmtPendingGet(
                    pFilter->otCtx,
                    aDatasetComp,
                    aTlvTypes,
                    aLength,
                    aAddress)
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
                otDatasetSendMgmtPendingSet(
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
                otCommissionerSendMgmtGet(
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
                otCommissionerSendMgmtSet(
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
        otThreadSetKeySwitchGuardTime(pFilter->otCtx, *(uint32_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(uint32_t))
    {
        *(uint32_t*)OutBuffer = otThreadGetKeySwitchGuardTime(pFilter->otCtx);
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
otLwfTunIoCtl_otKeySwitchGuardtime(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    if (InBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_SET,
                SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME,
                sizeof(uint32_t),
                SPINEL_DATATYPE_UINT32_S,
                *(uint32_t*)InBuffer);
    }
    else if (OutBufferLength >= sizeof(uint32_t))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                otLwfTunIoCtl_otKeySwitchGuardtime_Handler,
                SPINEL_CMD_PROP_VALUE_GET,
                SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME,
                0,
                NULL);
    }

    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfTunIoCtl_otKeySwitchGuardtime_Handler(
    _In_ spinel_prop_key_t Key,
    _In_reads_bytes_(DataLength) const uint8_t* Data,
    _In_ spinel_size_t DataLength,
    _Out_writes_bytes_(*OutBufferLength)
            PVOID OutBuffer,
    _Inout_ PULONG OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    if (Key == SPINEL_PROP_NET_KEY_SWITCH_GUARDTIME)
    {
        if (try_spinel_datatype_unpack(Data, DataLength, SPINEL_DATATYPE_UINT32_S, (uint32_t*)OutBuffer))
        {
            *OutBufferLength = sizeof(uint32_t);
            status = STATUS_SUCCESS;
        }
    }
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otThreadAutoStart(
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
        status =
            ThreadErrorToNtstatus(
                otThreadSetAutoStart(
                    pFilter->otCtx,
                    *(BOOLEAN*)InBuffer != FALSE)
            );
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(BOOLEAN))
    {
        *(BOOLEAN*)OutBuffer = otThreadGetAutoStart(pFilter->otCtx) ? TRUE : FALSE;
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
otLwfIoCtl_otThreadPreferredRouterId(
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
        status =
            ThreadErrorToNtstatus(
                otThreadSetPreferredRouterId(
                    pFilter->otCtx,
                    *(uint8_t*)InBuffer != FALSE)
            );
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otParentPriority(
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
        otThreadSetParentPriority(pFilter->otCtx, *(int8_t*)InBuffer);
        status = STATUS_SUCCESS;
        *OutBufferLength = 0;
    }
    else if (*OutBufferLength >= sizeof(int8_t))
    {
        *(uint16_t*)OutBuffer = otThreadGetParentPriority(pFilter->otCtx);
        status = STATUS_SUCCESS;
        *OutBufferLength = sizeof(int8_t);
    }
    else
    {
        *OutBufferLength = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otAddMacFixedRss(
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
        int8_t aRss = *(int8_t*)(InBuffer + sizeof(otExtAddress));
        status = ThreadErrorToNtstatus(otLinkFilterAddRssIn(pFilter->otCtx,
                    (otExtAddress *)InBuffer, aRss));
    }
    else if (InBufferLength >= sizeof(int8_t))
    {
        status = ThreadErrorToNtstatus(otLinkFilterAddRssIn(pFilter->otCtx, NULL,
                    *(int8_t *)InBuffer));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otAddMacFixedRss(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otExtAddress) + sizeof(int8_t))
    {
        int8_t aRss = OT_MAC_FILTER_FIXED_RSS_DISABLED;
        aRss = *(int8_t*)(InBuffer + sizeof(otExtAddress));
        status =
            otLwfTunSendCommandForIrp(
                    pFilter,
                    pIrp,
                    NULL,
                    SPINEL_CMD_PROP_VALUE_INSERT,
                    SPINEL_PROP_MAC_FIXED_RSS,
                    sizeof(otExtAddress) + sizeof(int8_t),
                    "Ec",
                    (otExtAddress*)InBuffer,
                    &aRss);
    }
    else
    {
        status =
            otLwfTunSendCommandForIrp(
                    pFilter,
                    pIrp,
                    NULL,
                    SPINEL_CMD_PROP_VALUE_INSERT,
                    SPINEL_PROP_MAC_FIXED_RSS,
                    sizeof(int8_t),
                    "c",
                    (int8_t*)InBuffer);
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otRemoveMacFixedRss(
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
        status = ThreadErrorToNtstatus(otLinkFilterRemoveRssIn(pFilter->otCtx, (otExtAddress *)InBuffer));
    }
    else
    {
        status = ThreadErrorToNtstatus(otLinkFilterRemoveRssIn(pFilter->otCtx, NULL));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otRemoveMacFixedRss(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(OutBufferLength);

    if (InBufferLength >= sizeof(otExtAddress))
    {
        status =
            otLwfTunSendCommandForIrp(
                pFilter,
                pIrp,
                NULL,
                SPINEL_CMD_PROP_VALUE_REMOVE,
                SPINEL_PROP_MAC_FIXED_RSS,
                sizeof(otExtAddress),
                "E",
                (otExtAddress*)InBuffer);
    }
    else
    {
        status =
            otLwfTunSendCommandForIrp(
                    pFilter,
                    pIrp,
                    NULL,
                    SPINEL_CMD_PROP_VALUE_REMOVE,
                    SPINEL_PROP_MAC_FIXED_RSS,
                    0,
                    NULL);

    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otClearMacFixedRss(
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

    otLinkFilterClearRssIn(pFilter->otCtx);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfTunIoCtl_otClearMacFixedRss(
    _In_ PMS_FILTER         pFilter,
    _In_ PIRP               pIrp,
    _In_reads_bytes_(InBufferLength)
            PUCHAR          InBuffer,
    _In_    ULONG           InBufferLength,
    _In_    ULONG           OutBufferLength
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferLength);
    UNREFERENCED_PARAMETER(OutBufferLength);

    status =
        otLwfTunSendCommandForIrp(
            pFilter,
            pIrp,
            NULL,
            SPINEL_CMD_PROP_VALUE_SET,
            SPINEL_PROP_MAC_FIXED_RSS,
            0,
            NULL);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfIoCtl_otNextMacFixedRss(
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
        *OutBufferLength >= sizeof(uint8_t) + sizeof(otMacFilterEntry))
    {
        uint8_t aIterator = *(uint8_t*)(InBuffer);
        otMacFilterEntry *aEntry = (otMacFilterEntry*)((PUCHAR)OutBuffer + sizeof(uint8_t));

        status = ThreadErrorToNtstatus(
            otLinkFilterGetNextRssIn(
                pFilter->otCtx,
                &aIterator,
                aEntry
                )
            );

        *OutBufferLength = sizeof(otMacFilterEntry) + sizeof(uint8_t);

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
