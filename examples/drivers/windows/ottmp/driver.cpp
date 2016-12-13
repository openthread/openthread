/*
 *    Copyright (c) 2016, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This module has code to deal with loading and unloading of the driver 
 */

#include "pch.hpp"
#include "driver.tmh"

#ifdef OTTMP_LEGACY
static GLOBALS GlobalData = { 0 };
#endif

INITCODE
extern "C"
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    A success status as determined by NT_SUCCESS macro, if successful. 
 
--*/
{
    NTSTATUS                             status;
    WDF_DRIVER_CONFIG                    config;
#ifdef OTTMP_LEGACY
    NDIS_STATUS                          ndisStatus = NDIS_STATUS_FAILURE;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS MPChar = { 0 };
    NET_BUFFER_LIST_POOL_PARAMETERS      NblParams = { 0 };
    NET_BUFFER_POOL_PARAMETERS           NbParams = { 0 };
#endif

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    LogFuncEntry(DRIVER_DEFAULT);

    //
    // Create the WdfDriver Object 
    //
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

#ifdef OTTMP_LEGACY
    //
    // Set WdfDriverInitNoDispatchOverride flag to tell the framework
    // not to provide dispatch routines for the driver. In other words,
    // the framework must not intercept IRPs that the I/O manager has
    // directed to the driver. In this case, it will be handled by NDIS
    // port driver.
    //
    config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;
#else
    config.EvtDriverDeviceAdd = EvtDriverDeviceAdd;
    config.EvtDriverUnload = EvtDriverUnload;
#endif

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
#ifdef OTTMP_LEGACY
                             &GlobalData.WdfDriver
#else
                             NULL
#endif
                             );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "WdfDriverCreate failed, %!STATUS!", status);
        goto error;
    }
    
#ifdef OTTMP_LEGACY
    MPChar.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
    MPChar.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
    MPChar.Header.Size = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;

    // Version Suff
    MPChar.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    MPChar.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    MPChar.MajorDriverVersion = NIC_VENDOR_DRIVER_VERSION_MAJOR;
    MPChar.MinorDriverVersion = NIC_VENDOR_DRIVER_VERSION_MINOR;

    MPChar.InitializeHandlerEx = MPInitializeEx;
    MPChar.HaltHandlerEx = MPHaltEx;
    MPChar.UnloadHandler = MPDriverUnload;
    MPChar.PauseHandler = MPPause;
    MPChar.RestartHandler = MPRestart;
    MPChar.OidRequestHandler = MPOidRequest;
    MPChar.SendNetBufferListsHandler = MPSendNetBufferLists;
    MPChar.ReturnNetBufferListsHandler = MPReturnNetBufferLists;
    MPChar.CancelSendHandler = MPCancelSend;
    MPChar.DevicePnPEventNotifyHandler = MPDevicePnpEventNotify;
    MPChar.ShutdownHandlerEx = MPShutdownEx;
    MPChar.CancelOidRequestHandler = MPCancelOidRequest;

    ndisStatus = NdisMRegisterMiniportDriver(DriverObject,
                                             RegistryPath,
                                             &GlobalData,
                                             &MPChar,
                                             &GlobalData.hDriver);
    if (ndisStatus != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "NdisMRegisterMiniportDriver failed %!NDIS_STATUS!", ndisStatus);
        status = STATUS_UNSUCCESSFUL;
        goto error;
    }

    NblParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    NblParams.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
    NblParams.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
    NblParams.PoolTag = NIC_TAG_RECV_NBL;
    GlobalData.hNblPool = NdisAllocateNetBufferListPool(GlobalData.hDriver, &NblParams);
    if (GlobalData.hNblPool == 0)
    {
        LogError(DRIVER_DEFAULT, "NdisAllocateNetBufferListPool failed");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    NbParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    NbParams.Header.Revision = NET_BUFFER_POOL_PARAMETERS_REVISION_1;
    NbParams.Header.Size = NDIS_SIZEOF_NET_BUFFER_POOL_PARAMETERS_REVISION_1;
    NbParams.PoolTag = NIC_TAG_RECV_NBL;
    NbParams.DataSize = MAX_SPINEL_COMMAND_LENGTH;
    GlobalData.hNbPool = NdisAllocateNetBufferPool(GlobalData.hDriver, &NbParams);
    if (GlobalData.hNbPool == 0)
    {
        LogError(DRIVER_DEFAULT, "NdisAllocateNetBufferListPool failed");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }
#endif

error:

    if (!NT_SUCCESS(status)) 
    {
#ifdef OTTMP_LEGACY
        MPDriverUnload(DriverObject);
#else
        WPP_CLEANUP(DriverObject);
#endif
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);
    return status;
}

#ifdef OTTMP_LEGACY

PAGED
_Use_decl_annotations_
VOID
MPDriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:

    MPDriverUnload will clean up the WPP resources that was allocated
    for this driver.

Arguments:

    DriverObject - Handle to a framework driver object created in DriverEntry

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    if (GlobalData.WdfDriver)
    {
        NT_ASSERT(GlobalData.WdfDriver == WdfGetDriver());
        WdfDriverMiniportUnload(GlobalData.WdfDriver);
        GlobalData.WdfDriver = nullptr;
    }

    if (GlobalData.hNblPool)
    {
        NdisFreeNetBufferListPool(GlobalData.hNblPool);
        GlobalData.hNblPool = nullptr;
    }

    if (GlobalData.hDriver)
    {
        NdisMDeregisterMiniportDriver(GlobalData.hDriver);
        GlobalData.hDriver = nullptr;
    }

    LogFuncExit(DRIVER_DEFAULT);

#pragma warning(suppress: 25024) // Dangerous cast
    WPP_CLEANUP(DriverObject);
}

#else

PAGED
VOID
EvtDriverUnload(
    WDFDRIVER Driver
    )
/*++
Routine Description:

    EvtDriverUnload will clean up the WPP resources that was allocated
    for this driver.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    LogFuncExit(DRIVER_DEFAULT);
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));
}

#endif
