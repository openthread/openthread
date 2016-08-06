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
#include "driver.tmh"

//
// Global variables
//

// Global Driver Object from DriverEntry
PDRIVER_OBJECT      FilterDriverObject = NULL;

// NDIS Filter handle from NdisFRegisterFilterDriver
NDIS_HANDLE         FilterDriverHandle = NULL;

// Global list of THREAD_FILTER instances
NDIS_SPIN_LOCK      FilterListLock;
LIST_ENTRY          FilterModuleList;

// Cached performance frequency of the system
LARGE_INTEGER       FilterPerformanceFrequency;

INITCODE
_Use_decl_annotations_
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_ PUNICODE_STRING    RegistryPath
    )
/*++

Routine Description:

    First entry point to be called, when this driver is loaded.
    Register with NDIS as a filter driver and create a device
    for communication with user-mode.

Arguments:

    DriverObject - pointer to the system's driver object structure
    for this driver

    RegistryPath - system's registry path for this driver

Return Value:

    STATUS_SUCCESS if all initialization is successful, STATUS_XXX
    error code if not.

--*/
{
    NDIS_STATUS                         Status;
    NDIS_FILTER_DRIVER_CHARACTERISTICS  FChars;
    NDIS_STRING                         ServiceName = RTL_CONSTANT_STRING(FILTER_SERVICE_NAME);
    NDIS_STRING                         UniqueName = RTL_CONSTANT_STRING(FILTER_UNIQUE_NAME);
    NDIS_STRING                         FriendlyName = RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME);
    BOOLEAN                             bFalse = FALSE;

    //
    // Initialize WPP logging
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Save global DriverObject
    //
    FilterDriverObject = DriverObject;

    // Cache perf freq
    (VOID)KeQueryPerformanceCounter(&FilterPerformanceFrequency);

    LogFuncEntry(DRIVER_DEFAULT);

    do
    {
        NdisZeroMemory(&FChars, sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS));
        FChars.Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
        FChars.Header.Size = sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS);
#if NDIS_SUPPORT_NDIS61
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;
#else
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_1;
#endif

        FChars.MajorNdisVersion = NDIS_FILTER_MAJOR_VERSION;
        FChars.MinorNdisVersion = NDIS_FILTER_MINOR_VERSION;
        FChars.MajorDriverVersion = 1;
        FChars.MinorDriverVersion = 0;
        FChars.Flags = 0;

        FChars.FriendlyName = FriendlyName;
        FChars.UniqueName = UniqueName;
        FChars.ServiceName = ServiceName;

        DriverObject->DriverUnload =                DriverUnload;

        FChars.AttachHandler =                      FilterAttach;
        FChars.DetachHandler =                      FilterDetach;
        FChars.RestartHandler =                     FilterRestart;
        FChars.PauseHandler =                       FilterPause;
        FChars.StatusHandler =                      FilterStatus;

        FChars.OidRequestHandler =                  FilterOidRequest;
        FChars.OidRequestCompleteHandler =          FilterOidRequestComplete;

        FChars.DevicePnPEventNotifyHandler =        FilterDevicePnPEventNotify;
        FChars.NetPnPEventHandler =                 FilterNetPnPEvent;

        FChars.SendNetBufferListsHandler =          FilterSendNetBufferLists;
        FChars.ReturnNetBufferListsHandler =        FilterReturnNetBufferLists;
        FChars.SendNetBufferListsCompleteHandler =  FilterSendNetBufferListsComplete;
        FChars.ReceiveNetBufferListsHandler =       FilterReceiveNetBufferLists;
        FChars.CancelSendNetBufferListsHandler =    FilterCancelSendNetBufferLists;

        //
        // Initialize global variables
        //
        NdisAllocateSpinLock(&FilterListLock);
        InitializeListHead(&FilterModuleList);
        
        //
        // Register the filter with NDIS
        //
        FilterDriverHandle = NULL;
        Status = 
            NdisFRegisterFilterDriver(
                DriverObject,
                (NDIS_HANDLE)FilterDriverObject,
                &FChars,
                &FilterDriverHandle
                );
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFreeSpinLock(&FilterListLock);
            LogError(DRIVER_DEFAULT, "Register filter driver failed, %!NDIS_STATUS!", Status);
            break;
        }

        //
        // Register the device IOCTL interface
        //
        Status = otLwfRegisterDevice();
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisFDeregisterFilterDriver(FilterDriverHandle);
            NdisFreeSpinLock(&FilterListLock);
            LogError(DRIVER_DEFAULT, "Register device for the filter driver failed, %!NDIS_STATUS!", Status);
            break;
        }

    } while (bFalse);

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        WPP_CLEANUP(DriverObject);
    }

    return Status;
}

PAGEDX
_Use_decl_annotations_
VOID
DriverUnload(
    _In_ PDRIVER_OBJECT     DriverObject
    )
/*++

Routine Description:

    Filter driver's unload routine.
    Deregister the driver from NDIS.

Arguments:

    DriverObject - pointer to the system's driver object structure
                   for this driver

Return Value:

    NONE

--*/
{
    PAGED_CODE();
    
    LogFuncEntry(DRIVER_DEFAULT);

    //
    // Clean up the device IOCTL interface
    //
    otLwfDeregisterDevice();

    //
    // Deregister the NDIS filter
    //
    NdisFDeregisterFilterDriver(FilterDriverHandle);

#if DBG
    // Validate we have no outstanding filter instances
    FILTER_ACQUIRE_LOCK(&FilterListLock, FALSE);
    ASSERT(IsListEmpty(&FilterModuleList));
    FILTER_RELEASE_LOCK(&FilterListLock, FALSE);
#endif

    //
    // Clean up global variables
    //
    NdisFreeSpinLock(&FilterListLock);

    LogFuncExit(DRIVER_DEFAULT);

    //
    // Clean up WPP logging
    //
    WPP_CLEANUP(DriverObject);
}
