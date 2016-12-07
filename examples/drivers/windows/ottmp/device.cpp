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
 *   This module implements code to manage the WDFDEVICE object for the
 *   network adapter.
 */

#include <initguid.h>
#include "pch.hpp"
#include "device.tmh"

PAGED
_IRQL_requires_( PASSIVE_LEVEL )
_Function_class_( MINIPORT_INITIALIZE )
NDIS_STATUS
MPInitializeEx(
    _In_  NDIS_HANDLE                       MiniportAdapterHandle,
    _In_  NDIS_HANDLE                       MiniportDriverContext,
    _In_  PNDIS_MINIPORT_INIT_PARAMETERS    /* MiniportInitParameters */
    )
/*++
Routine Description:

    The MiniportInitialize function is a required function that sets up a
    NIC (or virtual NIC) for network I/O operations, claims all hardware
    resources necessary to the NIC in the registry, and allocates resources
    the driver needs to carry out network I/O operations.

    MiniportInitialize runs at IRQL = PASSIVE_LEVEL.

Arguments:

Return Value:

    NDIS_STATUS_xxx code

--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    PGLOBALS                pGlobals = (PGLOBALS)MiniportDriverContext;
    PDEVICE_OBJECT          pPdo = nullptr;
    PDEVICE_OBJECT          pFdo = nullptr;
    PDEVICE_OBJECT          pNextDeviceObject = nullptr;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDEVICE               device = nullptr;
    POTTMP_DEVICE_CONTEXT   deviceContext = nullptr;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    do
    {
        NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES AdapterRegistration = { 0 };
        NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES AdapterGeneral = { 0 };
        NDIS_PM_CAPABILITIES PmCapabilities = { 0 };

        //
        // NdisMGetDeviceProperty function enables us to get the:
        // PDO - created by the bus driver to represent our device.
        // FDO - created by NDIS to represent our miniport as a function
        //              driver.
        // NextDeviceObject - deviceobject of another driver (filter)
        //                    attached to us at the bottom.
        // Since our driver is talking to NDISPROT, the NextDeviceObject
        // is not useful. But if we were to talk to a driver that we
        // are attached to as part of the devicestack then NextDeviceObject
        // would be our target DeviceObject for sending read/write Requests.
        //

        NdisMGetDeviceProperty(
            MiniportAdapterHandle,
            &pPdo,
            &pFdo,
            &pNextDeviceObject,
            nullptr,
            nullptr);

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, OTTMP_DEVICE_CONTEXT);

        NTSTATUS ntStatus = WdfDeviceMiniportCreate(
            pGlobals->WdfDriver,
            &attributes,
            pFdo,
            pNextDeviceObject,
            pPdo,
            &device);

        if (!NT_SUCCESS(ntStatus))
        {
            LogError(DRIVER_DEFAULT, "WdfDeviceMiniportCreate failed %!STATUS!", ntStatus);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Get WDF miniport device context.
        //
        deviceContext = GetDeviceContext(device);
        deviceContext->Signature = OTTMP_DEVICE_CONTEXT_SIGNATURE;
        deviceContext->Device = device;

        //
        // Allocate adapter context structure and initialize all the
        // memory resources for sending and receiving packets.
        //
        deviceContext->AdapterContext = (POTTMP_ADAPTER_CONTEXT)NdisAllocateMemoryWithTagPriority(
            pGlobals->hDriver, 
            sizeof(OTTMP_ADAPTER_CONTEXT), 
            OTTMP_ADAPTER_CONTEXT_SIGNATURE,
            NormalPoolPriority);

        if (deviceContext->AdapterContext == nullptr)
        {
            LogError(DRIVER_DEFAULT, "NdisAllocateMemoryWithTagPriority failed");
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        
        RtlZeroMemory(deviceContext->AdapterContext, sizeof(OTTMP_ADAPTER_CONTEXT));
        deviceContext->AdapterContext->Signature = OTTMP_ADAPTER_CONTEXT_SIGNATURE;
        deviceContext->AdapterContext->Adapter = MiniportAdapterHandle;
        deviceContext->AdapterContext->Device = deviceContext->Device;
        deviceContext->AdapterContext->pGlobals = pGlobals;

        Status = AdapterInitialize(
            MiniportAdapterHandle,
            deviceContext->AdapterContext);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            LogError(DRIVER_DEFAULT, "AdapterInitialize failed %!NDIS_STATUS!", Status);
            break;
        }

        ntStatus = SerialInitialize(deviceContext->AdapterContext);
        if (!NT_SUCCESS(ntStatus))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // Start the read loop
        LogVerbose(DRIVER_DEFAULT, "Starting recv worker");
        WdfWorkItemEnqueue(deviceContext->AdapterContext->RecvWorkItem);

    } while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (deviceContext && deviceContext->AdapterContext)
        {
            AdapterUninitialize(deviceContext->AdapterContext);
            deviceContext->AdapterContext = nullptr;
        }
    }

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    return Status;
}

PAGED
VOID
MPHaltEx(
    _In_  NDIS_HANDLE           MiniportAdapterContext,
    _In_  NDIS_HALT_ACTION      HaltAction
    )
/*++

Routine Description:

    Halt handler is called when NDIS receives IRP_MN_STOP_DEVICE,
    IRP_MN_SUPRISE_REMOVE or IRP_MN_REMOVE_DEVICE requests from the PNP
    manager. Here, the driver should free all the resources acquired in
    MiniportInitialize and stop access to the hardware. NDIS will not submit
    any further request once this handler is invoked.

    1) Free and unmap all I/O resources.
    2) Disable interrupt and deregister interrupt handler.
    3) Deregister shutdown handler regsitered by
        NdisMRegisterAdapterShutdownHandler .
    4) Cancel all queued up timer callbacks.
    5) Finally wait indefinitely for all the outstanding receive
        packets indicated to the protocol to return.

    MiniportHalt runs at IRQL = PASSIVE_LEVEL.


Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    HaltAction  The reason for halting the adapter

Return Value:

    None.

--*/
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)MiniportAdapterContext;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    //
    // Call Shutdown handler to disable interrupt and turn the hardware off
    // by issuing a full reset
    //
    if (HaltAction != NdisHaltDeviceSurpriseRemoved)
    {
        MPShutdownEx(MiniportAdapterContext, NdisShutdownPowerOff);
    }

    AdapterUninitialize(AdapterContext);

    LogFuncExit(DRIVER_DEFAULT);
}

VOID
MPShutdownEx(
    _In_  NDIS_HANDLE           /* MiniportAdapterContext */,
    _In_  NDIS_SHUTDOWN_ACTION  /* ShutdownAction */
    )
/*++

Routine Description:

    The MiniportShutdownEx handler restores hardware to its initial state when
    the system is shut down, whether by the user or because an unrecoverable
    system error occurred. This is to ensure that the NIC is in a known
    state and ready to be reinitialized when the machine is rebooted after
    a system shutdown occurs for any reason, including a crash dump.

    Here just disable the interrupt and stop the DMA engine.  Do not free
    memory resources or wait for any packet transfers to complete.  Do not call
    into NDIS at this time.

    This can be called at aribitrary IRQL, including in the context of a
    bugcheck.

Arguments:

    MiniportAdapterContext  Pointer to our adapter
    ShutdownAction  The reason why NDIS called the shutdown function

Return Value:

    None.

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    LogFuncExit(DRIVER_DEFAULT);
}

PAGED
VOID
MPDevicePnpEventNotify(
    _In_  NDIS_HANDLE             /* MiniportAdapterContext */,
    _In_  PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
    )
/*++

Routine Description:

    Runs at IRQL = PASSIVE_LEVEL in the context of system thread.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetDevicePnPEvent           Self-explanatory

Return Value:

    None.

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    switch (NetDevicePnPEvent->DevicePnPEvent)
    {
    case NdisDevicePnPEventQueryRemoved:
        //
        // Called when NDIS receives IRP_MN_QUERY_REMOVE_DEVICE.
        //
        LogInfo(DRIVER_DEFAULT, "MPPnPEventNotify: NdisDevicePnPEventQueryRemoved");
        break;

    case NdisDevicePnPEventRemoved:
        //
        // Called when NDIS receives IRP_MN_REMOVE_DEVICE.
        // NDIS calls MiniportHalt function after this call returns.
        //
        LogInfo(DRIVER_DEFAULT, "MPPnPEventNotify: NdisDevicePnPEventRemoved");
        break;

    case NdisDevicePnPEventSurpriseRemoved:
        //
        // Called when NDIS receives IRP_MN_SUPRISE_REMOVAL.
        // NDIS calls MiniportHalt function after this call returns.
        //
        LogInfo(DRIVER_DEFAULT, "MPDevicePnpEventNotify: NdisDevicePnPEventSurpriseRemoved");
        break;

    case NdisDevicePnPEventQueryStopped:
        //
        // Called when NDIS receives IRP_MN_QUERY_STOP_DEVICE. ??
        //
        LogInfo(DRIVER_DEFAULT, "MPPnPEventNotify: NdisDevicePnPEventQueryStopped");
        break;

    case NdisDevicePnPEventStopped:
        //
        // Called when NDIS receives IRP_MN_STOP_DEVICE.
        // NDIS calls MiniportHalt function after this call returns.
        //
        //
        LogInfo(DRIVER_DEFAULT, "MPPnPEventNotify: NdisDevicePnPEventStopped");
        break;

    case NdisDevicePnPEventPowerProfileChanged:
        //
        // After initializing a miniport driver and after miniport driver
        // receives an OID_PNP_SET_POWER notification that specifies
        // a device power state of NdisDeviceStateD0 (the powered-on state),
        // NDIS calls the miniport's MiniportPnPEventNotify function with
        // PnPEvent set to NdisDevicePnPEventPowerProfileChanged.
        //
        LogInfo(DRIVER_DEFAULT, "MPDevicePnpEventNotify: NdisDevicePnPEventPowerProfileChanged");

        if (NetDevicePnPEvent->InformationBufferLength == sizeof( ULONG ))
        {
            const ULONG NdisPowerProfile = *((const ULONG *)NetDevicePnPEvent->InformationBuffer);

            if (NdisPowerProfile == NdisPowerProfileBattery)
            {
                LogInfo(DRIVER_DEFAULT, "The host system is running on battery power");
            }
            else if (NdisPowerProfile == NdisPowerProfileAcOnLine)
            {
                LogInfo(DRIVER_DEFAULT, "The host system is running on AC power");
            }
        }
        break;

    default:
        LogError(DRIVER_DEFAULT, "MPDevicePnpEventNotify: unknown PnP event 0x%x\n", NetDevicePnPEvent->DevicePnPEvent);
    }

    LogFuncExit(DRIVER_DEFAULT);
}
