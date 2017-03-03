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
#include "filter.tmh"

// Helper function to query the CompartmentID of a Network Interface
COMPARTMENT_ID 
GetInterfaceCompartmentID(
    _In_ PIF_LUID pNetLuid
    )
{  
    COMPARTMENT_ID CompartmentID = UNSPECIFIED_COMPARTMENT_ID;  

    NTSTATUS Status =
        NsiGetParameter(
            NsiActive,
            &NPI_MS_NDIS_MODULEID,
            NdisNsiObjectInterfaceInformation,
            pNetLuid, sizeof(*pNetLuid),
            NsiStructRoDynamic,
            &CompartmentID, sizeof(CompartmentID),
            FIELD_OFFSET(NDIS_NSI_INTERFACE_INFORMATION_ROD, CompartmentId)
            );

    return (NT_SUCCESS(Status) ? CompartmentID : DEFAULT_COMPARTMENT_ID);
}

_Use_decl_annotations_
NDIS_STATUS
FilterAttach(
    NDIS_HANDLE                     NdisFilterHandle,
    NDIS_HANDLE                     FilterDriverContext,
    PNDIS_FILTER_ATTACH_PARAMETERS  AttachParameters
    )
/*++

Routine Description:

    Filter attach routine.
    Create filter's context, allocate NetBufferLists and NetBuffer pools and any
    other resources, and read configuration if needed.

Arguments:

    NdisFilterHandle - Specify a handle identifying this instance of the filter. FilterAttach
                       should save this handle. It is a required  parameter in subsequent calls
                       to NdisFxxx functions.
    FilterDriverContext - Filter driver context passed to NdisFRegisterFilterDriver.

    AttachParameters - attach parameters

Return Value:

    NDIS_STATUS_SUCCESS: FilterAttach successfully allocated and initialize data structures
                         for this filter instance.
    NDIS_STATUS_RESOURCES: FilterAttach failed due to insufficient resources.
    NDIS_STATUS_FAILURE: FilterAttach could not set up this instance of this filter and it has called
                         NdisWriteErrorLogEntry with parameters specifying the reason for failure.

N.B.:  FILTER can use NdisRegisterDeviceEx to create a device, so the upper 
    layer can send Irps to the filter.

--*/
{
    PMS_FILTER              pFilter = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    NTSTATUS                NtStatus;
    NDIS_FILTER_ATTRIBUTES  FilterAttributes;
    ULONG                   Size;
    COMPARTMENT_ID          OriginalCompartmentID;
    OBJECT_ATTRIBUTES       ObjectAttributes = {0};

    const ULONG RegKeyOffset = ARRAYSIZE(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\otlwf\\Parameters\\NdisAdapters\\") - 1;
    DECLARE_CONST_UNICODE_STRING(RegKeyPath, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\otlwf\\Parameters\\NdisAdapters\\{00000000-0000-0000-0000-000000000000}");
    RtlCopyMemory(RegKeyPath.Buffer + RegKeyOffset, AttachParameters->BaseMiniportName->Buffer + 8, sizeof(L"{00000000-0000-0000-0000-000000000000}"));

    LogFuncEntry(DRIVER_DEFAULT);

    do
    {
        ASSERT(FilterDriverContext == (NDIS_HANDLE)FilterDriverObject);
        if (FilterDriverContext != (NDIS_HANDLE)FilterDriverObject)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        // Verify the media type is supported.  This is a last resort; the
        // the filter should never have been bound to an unsupported miniport
        // to begin with.
        if (AttachParameters->MiniportMediaType != NdisMediumIP)
        {
            LogError(DRIVER_DEFAULT, "Unsupported media type, 0x%x.", (ULONG)AttachParameters->MiniportMediaType);
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        Size = sizeof(MS_FILTER) +  AttachParameters->BaseMiniportInstanceName->Length;

        pFilter = (PMS_FILTER)FILTER_ALLOC_MEM(NdisFilterHandle, Size);
        if (pFilter == NULL)
        {
            LogWarning(DRIVER_DEFAULT, "Failed to allocate context structure, 0x%x bytes", Size);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pFilter, sizeof(MS_FILTER));

        LogVerbose(DRIVER_DEFAULT, "Opening interface registry key %S", RegKeyPath.Buffer);

        InitializeObjectAttributes(
            &ObjectAttributes,
            (PUNICODE_STRING)&RegKeyPath,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL);

        // Open the registry key
        NtStatus = ZwOpenKey(&pFilter->InterfaceRegKey, KEY_ALL_ACCESS, &ObjectAttributes);
        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "ZwOpenKey failed to open %S, %!STATUS!", RegKeyPath.Buffer, NtStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Format of "\DEVICE\{5BA90C49-0D7E-455B-8D3B-614F6714A212}"
        AttachParameters->BaseMiniportName->Buffer += 8;
        AttachParameters->BaseMiniportName->Length -= 8 * sizeof(WCHAR);
        NtStatus = RtlGUIDFromString(AttachParameters->BaseMiniportName, &pFilter->InterfaceGuid);
        AttachParameters->BaseMiniportName->Buffer -= 8;
        AttachParameters->BaseMiniportName->Length += 8 * sizeof(WCHAR);
        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "Failed to convert FilterModuleGuidName to a GUID, %!STATUS!", NtStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        pFilter->InterfaceFriendlyName.Length = pFilter->InterfaceFriendlyName.MaximumLength = AttachParameters->BaseMiniportInstanceName->Length;
        pFilter->InterfaceFriendlyName.Buffer = (PWSTR)((PUCHAR)pFilter + sizeof(MS_FILTER));
        NdisMoveMemory(pFilter->InterfaceFriendlyName.Buffer,
                        AttachParameters->BaseMiniportInstanceName->Buffer,
                        pFilter->InterfaceFriendlyName.Length);

        pFilter->InterfaceIndex = AttachParameters->BaseMiniportIfIndex;
        pFilter->InterfaceLuid = AttachParameters->BaseMiniportNetLuid;
        pFilter->InterfaceCompartmentID = UNSPECIFIED_COMPARTMENT_ID;
        pFilter->FilterHandle = NdisFilterHandle;

        NdisZeroMemory(&FilterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
        FilterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
        FilterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
        FilterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
        FilterAttributes.Flags = 0;

        NDIS_DECLARE_FILTER_MODULE_CONTEXT(MS_FILTER);
        Status = NdisFSetAttributes(NdisFilterHandle, pFilter, &FilterAttributes);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Failed to set attributes, %!NDIS_STATUS!", Status);
            break;
        }

        // Filter initially in Paused state
        pFilter->State = FilterPaused;

        // Initialize rundowns to disabled with no active references
        pFilter->ExternalRefs.Count = EX_RUNDOWN_ACTIVE;
        pFilter->cmdRundown.Count = EX_RUNDOWN_ACTIVE;

        // Query the compartment ID for this interface to use for the IP stack
        pFilter->InterfaceCompartmentID = GetInterfaceCompartmentID(&pFilter->InterfaceLuid);
        LogVerbose(DRIVER_DEFAULT, "Interface %!GUID! is in Compartment %u", &pFilter->InterfaceGuid, (ULONG)pFilter->InterfaceCompartmentID);

        // Make sure we are in the right compartment
        (VOID)otLwfSetCompartment(pFilter, &OriginalCompartmentID);

        // Register for address changed notifications
        NtStatus = 
            NotifyUnicastIpAddressChange(
                AF_INET6,
                otLwfAddressChangeCallback,
                pFilter,
                FALSE,
                &pFilter->AddressChangeHandle
                );

        // Revert the compartment, now that we have the table
        otLwfRevertCompartment(OriginalCompartmentID);

        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "NotifyUnicastIpAddressChange failed, %!STATUS!", NtStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Add Filter to global list of Thread Filters
        NdisAcquireSpinLock(&FilterListLock);
        InsertTailList(&FilterModuleList, &pFilter->FilterModuleLink);
        NdisReleaseSpinLock(&FilterListLock);

        LogVerbose(DRIVER_DEFAULT, "Created Filter: %p", pFilter);

    } while (FALSE);

    // Clean up on failure
    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pFilter != NULL)
        {
            if (pFilter->AddressChangeHandle != NULL)
            {
                CancelMibChangeNotify2(pFilter->AddressChangeHandle);
                pFilter->AddressChangeHandle = NULL;
            }

            NdisFreeMemory(pFilter, 0, 0);
        }
    }

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    return Status;
}

_Use_decl_annotations_
VOID
FilterDetach(
    NDIS_HANDLE     FilterModuleContext
    )
/*++

Routine Description:

    Filter detach routine.
    This is a required function that will deallocate all the resources allocated during
    FilterAttach. NDIS calls FilterAttach to remove a filter instance from a filter stack.

Arguments:

    FilterModuleContext - pointer to the filter context area.

Return Value:
    None.

NOTE: Called at PASSIVE_LEVEL and the filter is in paused state

--*/
{
    PMS_FILTER  pFilter = (PMS_FILTER)FilterModuleContext;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p", FilterModuleContext);

    // Filter must be in paused state and pretty much inactive
    NT_ASSERT(pFilter->State == FilterPaused);
    NT_ASSERT(pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_UNINTIALIZED);

    //
    // Detach must not fail, so do not put any code here that can possibly fail.
    //

    // Remove this Filter from the global list
    NdisAcquireSpinLock(&FilterListLock);
    RemoveEntryList(&pFilter->FilterModuleLink);
    NdisReleaseSpinLock(&FilterListLock);

    // Unregister from address change notifications
    CancelMibChangeNotify2(pFilter->AddressChangeHandle);
    pFilter->AddressChangeHandle = NULL;

    // Close the registry key
    if (pFilter->InterfaceRegKey)
    {
        ZwClose(pFilter->InterfaceRegKey);
        pFilter->InterfaceRegKey = NULL;
    }

    // Free the memory allocated
    NdisFreeMemory(pFilter, 0, 0);

    LogFuncExit(DRIVER_DEFAULT);
}

// Indicates an interface state change has taken place (used for interface arrival/removal)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfNotifyDeviceAvailabilityChange(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                fAvailable
    )
{
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_DEVICE_AVAILABILITY;
        NotifEntry->Notif.DeviceAvailabilityPayload.Available = fAvailable;

        otLwfIndicateNotification(NotifEntry);
    }
}

PAGED
NTSTATUS
GetRegDWORDValue(
    _In_  PMS_FILTER        pFilter,
    _In_  PCWSTR            ValueName,
    _Out_ PULONG            ValueData
)
{
    NTSTATUS            status;
    ULONG               resultLength;
    UCHAR               keybuf[128] = {0};
    UNICODE_STRING      UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(
        pFilter->InterfaceRegKey,
        &UValueName,
        KeyValueFullInformation,
        keybuf,
        sizeof(keybuf),
        &resultLength);

    if (NT_SUCCESS(status)) 
    {
        PKEY_VALUE_FULL_INFORMATION keyInfo = (PKEY_VALUE_FULL_INFORMATION)keybuf;

        if (keyInfo->Type != REG_DWORD)
        {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
        else
        {
            *ValueData = *((ULONG UNALIGNED *)(keybuf + keyInfo->DataOffset));
        }
    }

    return status;
}

PAGED
NTSTATUS
SetRegDWORDValue(
    _In_ PMS_FILTER     pFilter,
    _In_ PCWSTR         ValueName,
    _In_ ULONG          ValueData
)
{
    NTSTATUS            status;
    UNICODE_STRING      UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwSetValueKey(
        pFilter->InterfaceRegKey,
        &UValueName,
        0,
        REG_DWORD,
        (PVOID)&ValueData,
        sizeof(ValueData));

    return status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterRestart(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_RESTART_PARAMETERS RestartParameters
    )
/*++

Routine Description:

    Filter restart routine.
    Start the datapath - begin sending and receiving NBLs.

Arguments:

    FilterModuleContext - pointer to the filter context stucture.
    RestartParameters   - additional information about the restart operation.

Return Value:

    NDIS_STATUS_SUCCESS: if filter restarts successfully
    NDIS_STATUS_XXX: Otherwise.

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_SUCCESS;
    PMS_FILTER          pFilter = (PMS_FILTER)FilterModuleContext;
    PVOID               SpinelCapsDataBuffer = NULL;
    const uint8_t*      SpinelCapsPtr = NULL;
    spinel_size_t       SpinelCapsLen = 0;
    NL_INTERFACE_KEY    key = {0};
    NL_INTERFACE_RW     interfaceRw;
    ULONG               ThreadOnHost = TRUE;

    PNDIS_RESTART_GENERAL_ATTRIBUTES NdisGeneralAttributes;
    PNDIS_RESTART_ATTRIBUTES         NdisRestartAttributes;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p", FilterModuleContext);

    NT_ASSERT(pFilter->State == FilterPaused);

    NdisRestartAttributes = RestartParameters->RestartAttributes;

    //
    // If NdisRestartAttributes is not NULL, then the filter can modify generic 
    // attributes and add new media specific info attributes at the end. 
    // Otherwise, if NdisRestartAttributes is NULL, the filter should not try to 
    // modify/add attributes.
    //
    if (NdisRestartAttributes != NULL)
    {
        ASSERT(NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES);

        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;

        //
        // Check to see if we need to change any attributes. For example, the
        // driver can change the current MAC address here. Or the driver can add
        // media specific info attributes.
        //
        NdisGeneralAttributes->LookaheadSize = 128;
    }

    // Initialize the Spinel command processing
    NdisStatus = otLwfCmdInitialize(pFilter);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "otLwfCmdInitialize failed, %!NDIS_STATUS!", NdisStatus);
        goto exit;
    }

    // Query the device capabilities
    NtStatus = otLwfCmdGetProp(pFilter, &SpinelCapsDataBuffer, SPINEL_PROP_CAPS, SPINEL_DATATYPE_DATA_S, &SpinelCapsPtr, &SpinelCapsLen);
    if (!NT_SUCCESS(NtStatus))
    {
        NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
        LogError(DRIVER_DEFAULT, "Failed to query SPINEL_PROP_CAPS, %!STATUS!", NtStatus);
        goto exit;
    }

    // Iterate and process returned capabilities
    NT_ASSERT(SpinelCapsDataBuffer);
    while (SpinelCapsLen > 0)
    {
        ULONG SpinelCap = 0;
        spinel_ssize_t len = spinel_datatype_unpack(SpinelCapsPtr, SpinelCapsLen, SPINEL_DATATYPE_UINT_PACKED_S, &SpinelCap);
        if (len < 1) break;
        SpinelCapsLen -= (spinel_size_t)len;
        SpinelCapsPtr += len;

        switch (SpinelCap)
        {
        case SPINEL_CAP_MAC_RAW:
            pFilter->DeviceCapabilities |= OTLWF_DEVICE_CAP_RADIO;
            pFilter->DeviceCapabilities |= OTLWF_DEVICE_CAP_RADIO_ACK_TIMEOUT;
            pFilter->DeviceCapabilities |= OTLWF_DEVICE_CAP_RADIO_MAC_RETRY_AND_COLLISION_AVOIDANCE;
            pFilter->DeviceCapabilities |= OTLWF_DEVICE_CAP_RADIO_ENERGY_SCAN;
            break;
        case SPINEL_CAP_NET_THREAD_1_0:
            pFilter->DeviceCapabilities |= OTLWF_DEVICE_CAP_THREAD_1_0;
            break;
        default:
            break;
        }
    }

    // Set the state indicating where we should be running the Thread logic (Host or Device).
    if (!NT_SUCCESS(GetRegDWORDValue(pFilter, L"RunOnHost", &ThreadOnHost)))
    {
        // Default to running on the host if the key isn't present
        ThreadOnHost = TRUE;
        SetRegDWORDValue(pFilter, L"RunOnHost", ThreadOnHost);
    }

    LogInfo(DRIVER_DEFAULT, "Filter: %p initializing ThreadOnHost=%d", FilterModuleContext, ThreadOnHost);

    // Initialize the processing logic
    if (ThreadOnHost)
    {
        // Ensure the device has the capabilities to support raw radio commands
        if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO) == 0)
        {
            LogError(DRIVER_DEFAULT, "Failed to start because device doesn't support raw radio commands");
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            goto exit;
        }

        pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_RADIO_MODE;
        NtStatus = otLwfInitializeThreadMode(pFilter);
        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "otLwfInitializeThreadMode failed, %!STATUS!", NtStatus);
            NdisStatus = NDIS_STATUS_FAILURE;
            pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_UNINTIALIZED;
            goto exit;
        }
    }
    else
    {
        // Ensure the device has the capabilities to support Thread commands
        if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_THREAD_1_0) == 0)
        {
            LogError(DRIVER_DEFAULT, "Failed to start because device doesn't support thread commands");
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            goto exit;
        }

        pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_THREAD_MODE;
        NtStatus = otLwfTunInitialize(pFilter);
        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "otLwfInitializeTunnelMode failed, %!STATUS!", NtStatus);
            NdisStatus = NDIS_STATUS_FAILURE;
            pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_UNINTIALIZED;
            goto exit;
        }
    }

    //
    // Disable DAD and Neighbor advertisements
    //
    key.Luid = pFilter->InterfaceLuid;
    NlInitializeInterfaceRw(&interfaceRw);
    interfaceRw.DadTransmits = 0;
    interfaceRw.SendUnsolicitedNeighborAdvertisementOnDad = FALSE;
  
    NtStatus =
        NsiSetAllParameters(
            NsiActive,
            NsiSetDefault,
            &NPI_MS_IPV6_MODULEID,
            NlInterfaceObject,
            &key,
            sizeof(key),
            &interfaceRw,
            sizeof(interfaceRw));
    if (!NT_SUCCESS(NtStatus))
    {
        LogError(DRIVER_DEFAULT, "NsiSetAllParameters (NlInterfaceObject) failed, %!STATUS!", NtStatus);
        NdisStatus = NDIS_STATUS_FAILURE;
        goto exit;
    }

    //
    // Enable the external references to the filter
    //
    ExReInitializeRundownProtection(&pFilter->ExternalRefs);

    //
    // If everything is OK, set the filter in running state.
    //
    pFilter->State = FilterRunning; // when successful
    otLwfNotifyDeviceAvailabilityChange(pFilter, TRUE);
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! arrival, Filter=%p", &pFilter->InterfaceGuid, pFilter);

exit:

    //
    // Ensure the state is Paused if restart failed.
    //
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        pFilter->State = FilterPaused;

        if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
        {
            otLwfUninitializeThreadMode(pFilter);
        }
        else if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_THREAD_MODE)
        {
            otLwfTunUninitialize(pFilter);
        }

        pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_UNINTIALIZED;

        // Clean up Spinel command processing
        otLwfCmdUninitialize(pFilter);
    }

    // Free the buffer for the capabilities we queried
    if (SpinelCapsDataBuffer != NULL)
    {
        FILTER_FREE_MEM(SpinelCapsDataBuffer);
    }

    LogFuncExitNDIS(DRIVER_DEFAULT, NdisStatus);
    return NdisStatus;
}

_Use_decl_annotations_
NDIS_STATUS
FilterPause(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_PAUSE_PARAMETERS   PauseParameters
    )
/*++

Routine Description:

    Filter pause routine.
    Complete all the outstanding sends and queued sends,
    wait for all the outstanding recvs to be returned
    and return all the queued receives.

Arguments:

    FilterModuleContext - pointer to the filter context stucture
    PauseParameters     - additional information about the pause

Return Value:

    NDIS_STATUS_SUCCESS if filter pauses successfully, NDIS_STATUS_PENDING
    if not.  No other return value is allowed (pause must succeed, eventually).

N.B.: When the filter is in Pausing state, it can still process OID requests, 
    complete sending, and returning packets to NDIS, and also indicate status.
    After this function completes, the filter must not attempt to send or 
    receive packets, but it may still process OID requests and status 
    indications.

--*/
{
    PMS_FILTER      pFilter = (PMS_FILTER)(FilterModuleContext);
    NDIS_STATUS     Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(PauseParameters);

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p", FilterModuleContext);

    //
    // Set the flag that the filter is going to pause
    //
    NT_ASSERT(pFilter->State == FilterRunning);
    NdisAcquireSpinLock(&FilterListLock);
    pFilter->State = FilterPausing;
    NdisReleaseSpinLock(&FilterListLock);

    //
    // Send final notification of interface removal
    //
    otLwfNotifyDeviceAvailabilityChange(pFilter, FALSE);
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removal.", &pFilter->InterfaceGuid);

    //
    // Disable external references and wait for existing calls to complete
    //
    LogInfo(DRIVER_DEFAULT, "Disabling and waiting for external references to release");
    ExWaitForRundownProtectionRelease(&pFilter->ExternalRefs);
    LogInfo(DRIVER_DEFAULT, "External references released.");

    //
    // Clean up based on the device mode
    //
    if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE)
    {
        otLwfUninitializeThreadMode(pFilter);
    }
    else if (pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_THREAD_MODE)
    {
        otLwfTunUninitialize(pFilter);
    }

    pFilter->DeviceStatus = OTLWF_DEVICE_STATUS_UNINTIALIZED;

    //
    // Clean up the Spinel command processing
    //
    otLwfCmdUninitialize(pFilter);

    //
    // Set the state back to Paused now that we are done
    //
    pFilter->State = FilterPaused;

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);
    return Status;
}

_Use_decl_annotations_
VOID
FilterStatus(
    NDIS_HANDLE             FilterModuleContext,
    PNDIS_STATUS_INDICATION StatusIndication
    )
/*++

Routine Description:

    Status indication handler

Arguments:

    FilterModuleContext     - our filter context
    StatusIndication        - the status being indicated

NOTE: called at <= DISPATCH_LEVEL

  FILTER driver may call NdisFIndicateStatus to generate a status indication to
  all higher layer modules.

--*/
{
    PMS_FILTER      pFilter = (PMS_FILTER)FilterModuleContext;

    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p, IndicateStatus: %8x", FilterModuleContext, StatusIndication->StatusCode);

    if (StatusIndication->StatusCode == NDIS_STATUS_LINK_STATE)
    {
        PNDIS_LINK_STATE LinkState = (PNDIS_LINK_STATE)StatusIndication->StatusBuffer;

        LogInfo(DRIVER_DEFAULT, "Filter: %p, MediaConnectState: %u", FilterModuleContext, LinkState->MediaConnectState);

        // Cache the link state from the miniport
        memcpy(&pFilter->MiniportLinkState, LinkState, sizeof(NDIS_LINK_STATE));
    }

    NdisFIndicateStatus(pFilter->FilterHandle, StatusIndication);
    
    LogFuncExit(DRIVER_DEFAULT);
}

// Indicate a change of the link state
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfIndicateLinkState(
    _In_ PMS_FILTER                 pFilter,
    _In_ NDIS_MEDIA_CONNECT_STATE   MediaState
    )
{
    // If we are already in the correct state, just return
    if (pFilter->MiniportLinkState.MediaConnectState == MediaState)
    {
        return;
    }

    NDIS_STATUS_INDICATION StatusIndication = {0};
  
    StatusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;  
    StatusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;  
    StatusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);  
    StatusIndication.SourceHandle = pFilter->FilterHandle;  
      
    StatusIndication.StatusCode = NDIS_STATUS_LINK_STATE;  
    StatusIndication.StatusBuffer = &pFilter->MiniportLinkState;  
    StatusIndication.StatusBufferSize = sizeof(pFilter->MiniportLinkState);  
      
    pFilter->MiniportLinkState.MediaConnectState = MediaState;
    
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! new media state: %u", &pFilter->InterfaceGuid, MediaState);
  
    NdisFIndicateStatus(pFilter->FilterHandle, &StatusIndication);  
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSetCompartment(
    _In_  PMS_FILTER                pFilter,
    _Out_ COMPARTMENT_ID*           pOriginalCompartment
    )
/*++

Routine Description:

    Sets the current thread's compartment ID to match the filter instance.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    // Make sure we are in the right compartment
    *pOriginalCompartment = NdisGetCurrentThreadCompartmentId();
    if (*pOriginalCompartment != pFilter->InterfaceCompartmentID)
    {
        status = NdisSetCurrentThreadCompartmentId(pFilter->InterfaceCompartmentID);
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "NdisSetCurrentThreadCompartmentId failed, %!STATUS!", status);
            *pOriginalCompartment = 0;
        }
    }
    else
    {
        *pOriginalCompartment = 0;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfRevertCompartment(
    _In_ COMPARTMENT_ID             OriginalCompartment
    )
/*++

Routine Description:

    Resets the current thread's compartment ID.

--*/
{
    // Revert the compartment if it is set
    if (OriginalCompartment != 0)
    {
        (VOID)NdisSetCurrentThreadCompartmentId(OriginalCompartment);
    }
}
