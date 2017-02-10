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
    NDIS_STATUS Status;

    // Initialize WPP logging
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    // Save global DriverObject
    FilterDriverObject = DriverObject;

    // Set the driver unload handler
    DriverObject->DriverUnload = DriverUnload;

    // Cache performance counter frequency
    (VOID)KeQueryPerformanceCounter(&FilterPerformanceFrequency);

    LogFuncEntryMsg(DRIVER_DEFAULT, "Registry: %S", RegistryPath->Buffer);

    do
    {
        NDIS_FILTER_DRIVER_CHARACTERISTICS  FChars = 
        {
            {
                NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS,
#if NDIS_SUPPORT_NDIS61
                NDIS_FILTER_CHARACTERISTICS_REVISION_2,
#else
                NDIS_FILTER_CHARACTERISTICS_REVISION_1,
#endif
                sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS)
            },
            NDIS_FILTER_MAJOR_VERSION,
            NDIS_FILTER_MINOR_VERSION,
            1,
            0,
            0,
            RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME),
            RTL_CONSTANT_STRING(FILTER_UNIQUE_NAME),
            RTL_CONSTANT_STRING(FILTER_SERVICE_NAME),

            NULL,
            NULL,
            FilterAttach,
            FilterDetach,
            FilterRestart,
            FilterPause,
            FilterSendNetBufferLists,
            FilterSendNetBufferListsComplete,
            FilterCancelSendNetBufferLists,
            FilterReceiveNetBufferLists,
            FilterReturnNetBufferLists,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            FilterStatus,
#if (NDIS_SUPPORT_NDIS61)
            NULL,
            NULL,
            NULL,
#endif
        };

        //
        // Initialize global variables
        //
        NdisAllocateSpinLock(&FilterListLock);
        InitializeListHead(&FilterModuleList);
        
        //
        // Register the filter with NDIS
        //
        Status = 
            NdisFRegisterFilterDriver(
                DriverObject,
                (NDIS_HANDLE)FilterDriverObject,
                &FChars,
                &FilterDriverHandle
                );
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Register filter driver failed, %!NDIS_STATUS!", Status);
            break;
        }

        //
        // Register the device IOCTL interface
        //
        Status = otLwfRegisterDevice();
        if (Status != NDIS_STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Register device for the filter driver failed, %!NDIS_STATUS!", Status);
            break;
        }

    } while (FALSE);

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (FilterDriverHandle)
        {
            NdisFDeregisterFilterDriver(FilterDriverHandle);
            FilterDriverHandle = NULL;
        }
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
    FilterDriverHandle = NULL;

    // Validate we have no outstanding filter instances
    NT_ASSERT(IsListEmpty(&FilterModuleList));

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
