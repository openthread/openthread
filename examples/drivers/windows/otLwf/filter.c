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
    PNET_BUFFER             SendNetBuffer;
    ULONG                   bytesProcessed = 0;
    COMPARTMENT_ID          OriginalCompartmentID;

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

        Size = sizeof(MS_FILTER) +
               AttachParameters->BaseMiniportInstanceName->Length;

        pFilter = (PMS_FILTER)FILTER_ALLOC_MEM(NdisFilterHandle, Size);
        if (pFilter == NULL)
        {
            LogWarning(DRIVER_DEFAULT, "Failed to allocate context structure, 0x%x bytes", Size);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pFilter, sizeof(MS_FILTER));

        // Format of "\DEVICE\{5BA90C49-0D7E-455B-8D3B-614F6714A212}"
        AttachParameters->BaseMiniportName->Buffer += 8;
        AttachParameters->BaseMiniportName->Length -= 8 * sizeof(WCHAR);
        NtStatus = RtlGUIDFromString(AttachParameters->BaseMiniportName,
                                     &pFilter->InterfaceGuid);
        AttachParameters->BaseMiniportName->Buffer -= 8;
        AttachParameters->BaseMiniportName->Length += 8 * sizeof(WCHAR);
        if (!NT_SUCCESS(NtStatus))
        {
            LogError(DRIVER_DEFAULT, "Failed to convert FilterModuleGuidName to a GUID, %!STATUS!", NtStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        
        pFilter->MiniportFriendlyName.Length = pFilter->MiniportFriendlyName.MaximumLength = AttachParameters->BaseMiniportInstanceName->Length;
        pFilter->MiniportFriendlyName.Buffer = (PWSTR)((PUCHAR)pFilter + sizeof(MS_FILTER));
        NdisMoveMemory(pFilter->MiniportFriendlyName.Buffer,
                        AttachParameters->BaseMiniportInstanceName->Buffer,
                        pFilter->MiniportFriendlyName.Length);

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
        Status = NdisFSetAttributes(NdisFilterHandle,
                                    pFilter,
                                    &FilterAttributes);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogWarning(DRIVER_DEFAULT, "Failed to set attributes, %!NDIS_STATUS!", Status);
            break;
        }

        // Create the NDIS pool for creating the SendNetBufferList
        NET_BUFFER_LIST_POOL_PARAMETERS PoolParams = 
        {
            { NDIS_OBJECT_TYPE_DEFAULT, NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1, NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1 },
            NDIS_PROTOCOL_ID_DEFAULT,
            TRUE,
            0,
            'BNto', // otNB
            0
        };
        pFilter->NetBufferListPool = NdisAllocateNetBufferListPool(pFilter->FilterHandle, &PoolParams);
        if (pFilter->NetBufferListPool == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            LogWarning(DRIVER_DEFAULT, "Failed to create NetBufferList pool");
            break;
        }

        // Create the SendNetBufferList
        pFilter->SendNetBufferList =
            NdisAllocateNetBufferAndNetBufferList(
                pFilter->NetBufferListPool,     // PoolHandle
                0,                              // ContextSize
                0,                              // ContextBackFill
                NULL,                           // MdlChain
                0,                              // DataOffset
                0                               // DataLength
                );
        if (pFilter->SendNetBufferList == NULL)
        {
            NdisFreeNetBufferListPool(pFilter->NetBufferListPool);
            Status = NDIS_STATUS_RESOURCES;
            LogWarning(DRIVER_DEFAULT, "Failed to create Send NetBufferList");
            break;
        }
        
        // Initialize NetBuffer fields
        SendNetBuffer = NET_BUFFER_LIST_FIRST_NB(pFilter->SendNetBufferList);
        NET_BUFFER_CURRENT_MDL(SendNetBuffer) = NULL;
        NET_BUFFER_CURRENT_MDL_OFFSET(SendNetBuffer) = 0;
        NET_BUFFER_DATA_LENGTH(SendNetBuffer) = 0;
        NET_BUFFER_DATA_OFFSET(SendNetBuffer) = 0;
        NET_BUFFER_FIRST_MDL(SendNetBuffer) = NULL;

        // Allocate the NetBuffer for SendNetBufferList
        Status = NdisRetreatNetBufferDataStart(SendNetBuffer, kMaxPHYPacketSize, 0, NULL);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFreeNetBufferList(pFilter->SendNetBufferList);
            NdisFreeNetBufferListPool(pFilter->NetBufferListPool);
            LogError(DRIVER_DEFAULT, "Failed to allocate NB for Send NetBufferList, %!NDIS_STATUS!", Status);
            break;
        }

        // Filter initially in Paused state
        pFilter->State = FilterPaused;
        NdisInitializeEvent(&pFilter->FilterPauseComplete);

        // Initialize initial ref count on IoControl to 1, which we release on shutdown
        RtlInitializeReferenceCount(&pFilter->IoControlReferences);
        NdisInitializeEvent(&pFilter->IoControlShutdownComplete);

        // Initialize PendingOidRequest lock
        NdisAllocateSpinLock(&pFilter->PendingOidRequestLock);

        // Initialize datapath to disabled with no active references
        pFilter->DataPathRundown.Count = EX_RUNDOWN_ACTIVE;
        KeInitializeEvent(
            &pFilter->SendNetBufferListComplete,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );

        // Initialize the event processing
        pFilter->EventWorkerThread = NULL;
        NdisAllocateSpinLock(&pFilter->EventsLock);
        InitializeListHead(&pFilter->AddressChangesHead);
        InitializeListHead(&pFilter->NBLsHead);
        InitializeListHead(&pFilter->EventIrpListHead);
        KeInitializeEvent(
            &pFilter->EventWorkerThreadStopEvent,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadWaitTimeUpdated,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessTasklets,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessAddressChanges,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessNBLs,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadProcessIrp,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        KeInitializeEvent(
            &pFilter->EventWorkerThreadEnergyScanComplete,
            SynchronizationEvent, // auto-clearing event
            FALSE                 // event initially non-signalled
            );
        pFilter->EventHighPrecisionTimer = 
            ExAllocateTimer(
                otLwfEventProcessingTimer, 
                pFilter, 
                EX_TIMER_HIGH_RESOLUTION
                );
        if (pFilter->EventHighPrecisionTimer == NULL)
        {
            LogError(DRIVER_DEFAULT, "Failed to allocate timer!");
            break;
        }

        // Little hack to pass the IfIndex to the VMP
        *((ULONG*)&pFilter->MiniportCapabilities) = AttachParameters->BaseMiniportIfIndex;

        // Query the MP capabilities
        Status = 
            otLwfSendInternalRequest(
                pFilter,
                NdisRequestQueryInformation,
                OID_OT_CAPABILITIES,
                &pFilter->MiniportCapabilities,
                sizeof(OT_CAPABILITIES),
                &bytesProcessed
                );
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Query for OID_OT_CAPABILITIES failed, %!NDIS_STATUS!", Status);
            break;
        }

        // Validate the return header
        if (bytesProcessed != SIZEOF_OT_CAPABILITIES_REVISION_1 ||
            pFilter->MiniportCapabilities.Header.Type != NDIS_OBJECT_TYPE_DEFAULT ||
            pFilter->MiniportCapabilities.Header.Revision != OT_CAPABILITIES_REVISION_1 ||
            pFilter->MiniportCapabilities.Header.Size != SIZEOF_OT_CAPABILITIES_REVISION_1)
        {
            Status = NDIS_STATUS_INVALID_DATA;
            LogError(DRIVER_DEFAULT, "Query for OID_OT_CAPABILITIES returned invalid data");
            break;
        }

        // Make sure we are in RADIO mode
        if (pFilter->MiniportCapabilities.MiniportMode != OT_MP_MODE_RADIO)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            LogError(DRIVER_DEFAULT, "Miniport indicated it isn't running in RADIO mode!");
            break;
        }

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

        // Initialize the event processing thread
        if (!NT_SUCCESS(otLwfEventProcessingStart(pFilter)))
        {
            NdisAcquireSpinLock(&FilterListLock);
            RemoveEntryList(&pFilter->FilterModuleLink);
            NdisReleaseSpinLock(&FilterListLock);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        LogVerbose(DRIVER_DEFAULT, "Created Filter: %p", pFilter);

    } while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pFilter != NULL)
        {
            if (pFilter->AddressChangeHandle != NULL)
            {
                CancelMibChangeNotify2(pFilter->AddressChangeHandle);
                pFilter->AddressChangeHandle = NULL;
            }
            
            // Stop event processing thread
            otLwfEventProcessingStop(pFilter);

            if (pFilter->EventHighPrecisionTimer) ExDeleteTimer(pFilter->EventHighPrecisionTimer, TRUE, FALSE, NULL);
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
    NT_ASSERT(pFilter->IoControlReferences > 0);

    //
    // Detach must not fail, so do not put any code here that can possibly fail.
    //

    // Release IoControl reference and wait for shutdown if there are active refernces
    NdisResetEvent(&pFilter->IoControlShutdownComplete);
    if (!RtlDecrementReferenceCount(&pFilter->IoControlReferences))
    {
        LogInfo(DRIVER_DEFAULT, "Waiting for Io Control shutdown...");
        NdisWaitEvent(&pFilter->IoControlShutdownComplete, 0);
        NT_ASSERT(pFilter->IoControlReferences == 0);
        LogInfo(DRIVER_DEFAULT, "Received Io Control shutdown event.");
    }
    
    // Unregister from address change notifications
    CancelMibChangeNotify2(pFilter->AddressChangeHandle);
    pFilter->AddressChangeHandle = NULL;

    // Stop event processing thread
    otLwfEventProcessingStop(pFilter);

    // Remove this Filter from the global list
    NdisAcquireSpinLock(&FilterListLock);
    RemoveEntryList(&pFilter->FilterModuleLink);
    NdisReleaseSpinLock(&FilterListLock);

    // Free NBL & Pools
    NdisAdvanceNetBufferDataStart(NET_BUFFER_LIST_FIRST_NB(pFilter->SendNetBufferList), kMaxPHYPacketSize, TRUE, NULL);
    NdisFreeNetBufferList(pFilter->SendNetBufferList);
    NdisFreeNetBufferPool(pFilter->NetBufferListPool);
    
    // Free timer
    ExDeleteTimer(pFilter->EventHighPrecisionTimer, TRUE, FALSE, NULL);

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
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    PMS_FILTER      pFilter = (PMS_FILTER)FilterModuleContext;

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

    //
    // Enable the data path
    //
    otLwfEnableDataPath(pFilter);

    //
    // If everything is OK, set the filter in running state.
    //
    pFilter->State = FilterRunning; // when successful
    otLwfNotifyDeviceAvailabilityChange(pFilter, TRUE);
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! arrival, Filter=%p", &pFilter->InterfaceGuid, pFilter);

    //
    // Ensure the state is Paused if restart failed.
    //
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        pFilter->State = FilterPaused;
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

    // Reset the pause complete event
    NdisResetEvent(&pFilter->FilterPauseComplete);

    //
    // Set the flag that the filter is going to pause
    //
    NT_ASSERT(pFilter->State == FilterRunning);
    pFilter->State = FilterPausing;

    //
    // Make sure we are disconnected from the network
    //
    // TODO ...
    //otBecomeDetached(pFilter->otCtx); // TODO - This should block or we wait for it to complete
    //otDisable(pFilter->otCtx);
    
    otLwfNotifyDeviceAvailabilityChange(pFilter, FALSE);
    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removal.", &pFilter->InterfaceGuid);

    //
    // Disable the data path
    //
    otLwfDisableDataPath(pFilter);

    // Set the state back to Paused now that we are done
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
    else if (StatusIndication->StatusCode == NDIS_STATUS_OT_ENERGY_SCAN_RESULT)
    {
        NT_ASSERT(StatusIndication->StatusBufferSize == sizeof(OT_ENERGY_SCAN_RESULT));
        if (StatusIndication->StatusBufferSize != sizeof(OT_ENERGY_SCAN_RESULT)) goto exit;

        POT_ENERGY_SCAN_RESULT ScanResult = (POT_ENERGY_SCAN_RESULT)StatusIndication->StatusBuffer;

        LogInfo(DRIVER_DEFAULT, "Filter: %p, completed energy scan: Rssi:%d, Status:%!NDIS_STATUS!", FilterModuleContext, ScanResult->MaxRssi, ScanResult->Status);

        // Marshal to OpenThread worker to indicate back
        otLwfEventProcessingIndicateEnergyScanResult(pFilter, ScanResult->MaxRssi);
    }

exit:

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

_Use_decl_annotations_
VOID
FilterDevicePnPEventNotify(
    NDIS_HANDLE             FilterModuleContext,
    PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
    )
/*++

Routine Description:

    Device PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetDevicePnPEvent           - a Device PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PMS_FILTER             pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_DEVICE_PNP_EVENT  DevicePnPEvent = NetDevicePnPEvent->DevicePnPEvent;
#if DBG
    BOOLEAN                bFalse = FALSE;
#endif
    
    LogFuncEntryMsg(DRIVER_DEFAULT, "Filter: %p, DevicePnPEvent: %u", FilterModuleContext, NetDevicePnPEvent->DevicePnPEvent);

    //
    // The filter may do processing on the event here, including intercepting
    // and dropping it entirely.  However, the sample does nothing with Device
    // PNP events, except pass them down to the next lower* layer.  It is more
    // efficient to omit the FilterDevicePnPEventNotify handler entirely if it
    // does nothing, but it is included in this sample for illustrative purposes.
    //
    // * Trivia: Device PNP events percolate DOWN the stack, instead of upwards
    // like status indications and Net PNP events.  So the next layer is the
    // LOWER layer.
    //

    switch (DevicePnPEvent)
    {
        case NdisDevicePnPEventQueryRemoved:
        case NdisDevicePnPEventRemoved:
        case NdisDevicePnPEventSurpriseRemoved:
        case NdisDevicePnPEventQueryStopped:
        case NdisDevicePnPEventStopped:
        case NdisDevicePnPEventPowerProfileChanged:
        case NdisDevicePnPEventFilterListChanged:
            break;

        default:
            LogError(DRIVER_DEFAULT, "FilterDevicePnPEventNotify: Invalid event.\n");
            NT_ASSERT(bFalse);
            break;
    }

    NdisFDevicePnPEventNotify(pFilter->FilterHandle, NetDevicePnPEvent);
    
    LogFuncExit(DRIVER_DEFAULT);
}

_Use_decl_annotations_
NDIS_STATUS
FilterNetPnPEvent(
    NDIS_HANDLE              FilterModuleContext,
    PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification
    )
/*++

Routine Description:

    Net PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetPnPEventNotification     - a Net PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PMS_FILTER                pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS               Status = NDIS_STATUS_SUCCESS;

    //
    // The filter may do processing on the event here, including intercepting
    // and dropping it entirely.  However, the sample does nothing with Net PNP
    // events, except pass them up to the next higher layer.  It is more
    // efficient to omit the FilterNetPnPEvent handler entirely if it does
    // nothing, but it is included in this sample for illustrative purposes.
    //

    Status = NdisFNetPnPEvent(pFilter->FilterHandle, NetPnPEventNotification);

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSetCompartment(
    _In_  PMS_FILTER                pFilter,
    _Out_ COMPARTMENT_ID*           pOriginalCompartment
    )
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
{
    // Revert the compartment if it is set
    if (OriginalCompartment != 0)
    {
        (VOID)NdisSetCurrentThreadCompartmentId(OriginalCompartment);
    }
}

#if DBG
PMS_FILTER
otLwfFindFromCurrentThread()
{
    PMS_FILTER pOutput = NULL;
    HANDLE CurThreadId = PsGetCurrentThreadId();

    NdisAcquireSpinLock(&FilterListLock);

    for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
    {
        PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);

        if (pFilter->otThreadId == CurThreadId)
        {
            pOutput = pFilter;
            break;
        }
    }

    NdisReleaseSpinLock(&FilterListLock);

    NT_ASSERT(pOutput);
    return pOutput;
}
#endif

void *otPlatCAlloc(size_t aNum, size_t aSize)
{
    size_t totalSize = aNum * aSize;
#if DBG
    totalSize += sizeof(OT_ALLOC);
#endif
    PVOID mem = ExAllocatePoolWithTag(NonPagedPoolNx, totalSize, 'OTDM');
    if (mem)
    {
        RtlZeroMemory(mem, totalSize);
#if DBG
        PMS_FILTER pFilter = otLwfFindFromCurrentThread();
        //LogVerbose(DRIVER_DEFAULT, "otPlatAlloc(%u) = ID:%u %p", (ULONG)totalSize, pFilter->otAllocationID, mem);

        OT_ALLOC* AllocHeader = (OT_ALLOC*)mem;
        AllocHeader->Length = (LONG)totalSize;
        AllocHeader->ID = pFilter->otAllocationID++;
        InsertTailList(&pFilter->otOutStandingAllocations, &AllocHeader->Link);

        InterlockedIncrement(&pFilter->otOutstandingAllocationCount);
        InterlockedAdd(&pFilter->otOutstandingMemoryAllocated, AllocHeader->Length);
        
        mem = (PUCHAR)(mem) + sizeof(OT_ALLOC);
#endif
    }
    return mem;
}

void otPlatFree(void *aPtr)
{
    if (aPtr == NULL) return;
#if DBG
    aPtr = (PUCHAR)(aPtr) - sizeof(OT_ALLOC);
    //LogVerbose(DRIVER_DEFAULT, "otPlatFree(%p)", aPtr);
    OT_ALLOC* AllocHeader = (OT_ALLOC*)aPtr;

    PMS_FILTER pFilter = otLwfFindFromCurrentThread();
    InterlockedDecrement(&pFilter->otOutstandingAllocationCount);
    InterlockedAdd(&pFilter->otOutstandingMemoryAllocated, -AllocHeader->Length);
    RemoveEntryList(&AllocHeader->Link);
#endif
    ExFreePoolWithTag(aPtr, 'OTDM');
}

uint32_t otPlatRandomGet()
{
    LARGE_INTEGER Counter = KeQueryPerformanceCounter(NULL);
    return (uint32_t)RtlRandomEx(&Counter.LowPart);
}

ThreadError otPlatRandomSecureGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
{
    // Just use the system-preferred random number generator algorithm
    NTSTATUS status = 
        BCryptGenRandom(
            NULL, 
            aOutput, 
            (ULONG)aInputLength, 
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
            );
    NT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "BCryptGenRandom failed, %!STATUS!", status);
        return kThreadError_Failed;
    }

    *aOutputLength = aInputLength;

    return kThreadError_None;
}

void otSignalTaskletPending(_In_ otInstance *otCtx)
{
    LogVerbose(DRIVER_DEFAULT, "otSignalTaskletPending");
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    otLwfEventProcessingIndicateNewTasklet(pFilter);
}

// Process a role state change
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfProcessRoleStateChange(
    _In_ PMS_FILTER             pFilter
    )
{
    otDeviceRole prevRole = pFilter->otCachedRole;
    pFilter->otCachedRole = otGetDeviceRole(pFilter->otCtx);
    if (prevRole == pFilter->otCachedRole) return;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! new role: %!otDeviceRole!", &pFilter->InterfaceGuid, pFilter->otCachedRole);

    // Make sure we are in the correct media connect state
    otLwfIndicateLinkState(
        pFilter, 
        IsAttached(pFilter->otCachedRole) ? 
            MediaConnectStateConnected : 
            MediaConnectStateDisconnected);
}

void otLwfStateChangedCallback(uint32_t aFlags, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);

    //
    // Process the notification internally
    //

    if ((aFlags & OT_IP6_ADDRESS_ADDED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ADDRESS_ADDED", pFilter);
        otLwfAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_IP6_ADDRESS_REMOVED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ADDRESS_REMOVED", pFilter);
        otLwfAddressesUpdated(pFilter);
    }

    if ((aFlags & OT_NET_ROLE) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_ROLE", pFilter);
        otLwfProcessRoleStateChange(pFilter);
    }

    if ((aFlags & OT_NET_PARTITION_ID) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_PARTITION_ID", pFilter);
    }

    if ((aFlags & OT_NET_KEY_SEQUENCE_COUNTER) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_NET_KEY_SEQUENCE_COUNTER", pFilter);
    }

    if ((aFlags & OT_THREAD_CHILD_ADDED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_CHILD_ADDED", pFilter);
    }

    if ((aFlags & OT_THREAD_CHILD_REMOVED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_CHILD_REMOVED", pFilter);
    }

    if ((aFlags & OT_THREAD_NETDATA_UPDATED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_THREAD_NETDATA_UPDATED", pFilter);
        otSlaacUpdate(pFilter->otCtx, pFilter->otAutoAddresses, ARRAYSIZE(pFilter->otAutoAddresses), otCreateRandomIid, NULL);
    }

    if ((aFlags & OT_IP6_ML_ADDR_CHANGED) != 0)
    {
        LogVerbose(DRIVER_DEFAULT, "Filter %p received OT_IP6_ML_ADDR_CHANGED", pFilter);
    }
    
    //
    // Queue the notification for clients
    //

    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_STATE_CHANGE;
        NotifEntry->Notif.StateChangePayload.Flags = aFlags;

        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfActiveScanCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_ACTIVE_SCAN;

        if (aResult)
        {
            NotifEntry->Notif.ActiveScanPayload.Valid = TRUE;
            NotifEntry->Notif.ActiveScanPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.ActiveScanPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfEnergyScanCallback(_In_ otEnergyScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_ENERGY_SCAN;

        if (aResult)
        {
            NotifEntry->Notif.EnergyScanPayload.Valid = TRUE;
            NotifEntry->Notif.EnergyScanPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.EnergyScanPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfDiscoverCallback(_In_ otActiveScanResult *aResult, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);

    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_DISCOVER;

        if (aResult)
        {
            NotifEntry->Notif.DiscoverPayload.Valid = TRUE;
            NotifEntry->Notif.DiscoverPayload.Results = *aResult;
        }
        else
        {
            NotifEntry->Notif.DiscoverPayload.Valid = FALSE;
        }
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfCommissionerEnergyReportCallback(uint32_t aChannelMask, const uint8_t *aEnergyList, uint8_t aEnergyListLength, void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);
    
    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_COMMISSIONER_ENERGY_REPORT;

        // Limit the number of reports if necessary
        if (aEnergyListLength > MAX_ENERGY_REPORT_LENGTH) aEnergyListLength = MAX_ENERGY_REPORT_LENGTH;
        
        NotifEntry->Notif.CommissionerEnergyReportPayload.ChannelMask = aChannelMask;
        NotifEntry->Notif.CommissionerEnergyReportPayload.EnergyListLength = aEnergyListLength;
        memcpy(NotifEntry->Notif.CommissionerEnergyReportPayload.EnergyList, aEnergyList, aEnergyListLength);
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

void otLwfCommissionerPanIdConflictCallback(uint16_t aPanId, uint32_t aChannelMask, _In_ void *aContext)
{
    LogFuncEntry(DRIVER_DEFAULT);
    
    PMS_FILTER pFilter = (PMS_FILTER)aContext;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_COMMISSIONER_PANID_QUERY;
        
        NotifEntry->Notif.CommissionerPanIdQueryPayload.PanId = aPanId;
        NotifEntry->Notif.CommissionerPanIdQueryPayload.ChannelMask = aChannelMask;
        
        otLwfIndicateNotification(NotifEntry);
    }

    LogFuncExit(DRIVER_DEFAULT);
}
