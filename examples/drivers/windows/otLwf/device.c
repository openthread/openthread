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
#include "device.tmh"

// IoControl Device Object from IoCreateDeviceSecure
PDEVICE_OBJECT IoDeviceObject = NULL;

// Global context for device control callbacks
POTLWF_DEVICE_EXTENSION FilterDeviceExtension = NULL;

/*

Powershell script to generate security desciptors:

$sddl = "D:P(A;;GA;;;SY)(A;;GA;;;NS)(A;;GA;;;BA)(A;;GA;;;WD)(A;;GA;;;S-1-15-3-3)"
$blob = ([wmiclass]"Win32_SecurityDescriptorHelper").SDDLToBinarySD($sddl).BinarySD
$string = [BitConverter]::ToString($blob)
$string = $string -replace '-', ''
$string = $string -replace '(..)(..)(..)(..)', '0x$4$3$2$1, '
$string -replace '(.{10}, .{10}, .{10}, .{10},) ', "$&`n"

*/
const unsigned long c_sdThreadLwf[] =
{
    0x90040001, 0x00000000, 0x00000000, 0x00000000,
    0x00000014, 0x00740002, 0x00000005, 0x00140000,
    0x10000000, 0x00000101, 0x05000000, 0x00000012,
    0x00140000, 0x10000000, 0x00000101, 0x05000000,
    0x00000014, 0x00180000, 0x10000000, 0x00000201,
    0x05000000, 0x00000020, 0x00000220, 0x00140000,
    0x10000000, 0x00000101, 0x01000000, 0x00000000,
    0x00180000, 0x10000000, 0x00000201, 0x0F000000,
    0x00000003, 0x00000003
};

_No_competing_thread_
INITCODE
NDIS_STATUS
otLwfRegisterDevice(
    VOID
    )
{
    NTSTATUS                        Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING                  DeviceName;
    UNICODE_STRING                  DeviceLinkUnicodeString;
    PDEVICE_OBJECT                  DeviceObject;

    LogFuncEntry(DRIVER_DEFAULT);

    NT_ASSERT(FilterDeviceExtension == NULL);

    NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
    NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

    Status = IoCreateDeviceSecure(FilterDriverObject,                     // DriverObject
                                  sizeof(OTLWF_DEVICE_EXTENSION),        // DeviceExtension
                                  &DeviceName,                            // DeviceName
                                  FILE_DEVICE_NETWORK,                    // DeviceType
                                  FILE_DEVICE_SECURE_OPEN,                // DeviceCharacteristics
                                  FALSE,                                  // Exclusive
                                  &SDDL_DEVOBJ_KERNEL_ONLY,               // security attributes
                                  NULL,                                   // security override device class
                                  &DeviceObject);                         // DeviceObject

    if (NT_SUCCESS(Status))
    {
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        Status = IoCreateSymbolicLink(&DeviceLinkUnicodeString, &DeviceName);

        if (!NT_SUCCESS(Status))
        {
            LogError(DRIVER_DEFAULT, "IoCreateSymbolicLink failed, %!STATUS!", Status);
            IoDeleteDevice(DeviceObject);
        }
        else
        {
            FilterDeviceExtension = (POTLWF_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            RtlZeroMemory(FilterDeviceExtension, sizeof(OTLWF_DEVICE_EXTENSION));

            FilterDeviceExtension->Signature = 'FTDR';
            FilterDeviceExtension->Handle = FilterDriverHandle;

            NdisAllocateSpinLock(&FilterDeviceExtension->Lock);
            InitializeListHead(&FilterDeviceExtension->PendingNotificationList);

            FilterDriverObject->MajorFunction[IRP_MJ_CREATE] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_CLEANUP] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_CLOSE] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = otLwfDeviceIoControl;

            HANDLE fileHandle;
            Status = ObOpenObjectByPointer(DeviceObject,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           WRITE_DAC,
                                           0,
                                           KernelMode,
                                           &fileHandle);
            if (NT_SUCCESS(Status))
            {
                Status = ZwSetSecurityObject(fileHandle, 
                                             DACL_SECURITY_INFORMATION, 
                                             (PSECURITY_DESCRIPTOR)c_sdThreadLwf);

                if (!NT_SUCCESS(Status))
                {
                    LogError(DRIVER_DEFAULT, "ZwSetSecurityObject failed, %!STATUS!", Status);
                }

                ZwClose(fileHandle);
            }
            else
            {
                LogError(DRIVER_DEFAULT, "ObOpenObjectByPointer failed, %!STATUS!", Status);
            }

            IoDeviceObject = DeviceObject;
        }
    }
    else
    {
        LogError(DRIVER_DEFAULT, "IoCreateDeviceSecure failed, %!STATUS!", Status);
    }

    LogFuncExitNT(DRIVER_DEFAULT, Status);

    return (NDIS_STATUS)Status;
}

_No_competing_thread_
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfDeregisterDevice(
    VOID
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    if (IoDeviceObject != NULL)
    {
        NT_ASSERT(FilterDeviceExtension);
        NdisFreeSpinLock(&FilterDeviceExtension->Lock);

        // Complete the pending IRP since we are shutting down
        if (FilterDeviceExtension->PendingNotificationIRP)
        {
            // Before we are allowed to complete the pending IRP, we must remove the cancel routine
            KIRQL irql;
            IoAcquireCancelSpinLock(&irql);
            IoSetCancelRoutine(FilterDeviceExtension->PendingNotificationIRP, NULL);
            IoReleaseCancelSpinLock(irql);

            FilterDeviceExtension->PendingNotificationIRP->IoStatus.Status = STATUS_CANCELLED;
            FilterDeviceExtension->PendingNotificationIRP->IoStatus.Information = 0;
            IoCompleteRequest(FilterDeviceExtension->PendingNotificationIRP, IO_NO_INCREMENT);
        }

        // Free all pending notifications
        PLIST_ENTRY Link = FilterDeviceExtension->PendingNotificationList.Flink;
        while (Link != &FilterDeviceExtension->PendingNotificationList)
        {
            PFILTER_NOTIFICATION_ENTRY pNotification = CONTAINING_RECORD(Link, FILTER_NOTIFICATION_ENTRY, Link);

            // Set next link
            Link = Link->Flink;

            // Free the memory
            NdisFreeMemory(pNotification, 0, 0);
        }

        IoDeleteDevice(IoDeviceObject);
    }

    IoDeviceObject = NULL;

    LogFuncExit(DRIVER_DEFAULT);
}

_Use_decl_annotations_
NTSTATUS
otLwfDispatch(
    PDEVICE_OBJECT       DeviceObject,
    PIRP                 Irp
    )
{
    PIO_STACK_LOCATION      IrpStack;
    NTSTATUS                Status = STATUS_SUCCESS;
    PIRP                    IrpToCancel = NULL;
    PLIST_ENTRY             Link = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);

    LogFuncEntry(DRIVER_IOCTL);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    switch (IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            if (FilterDeviceExtension->HasClient)
            {
                LogInfo(DRIVER_IOCTL, "Failing Client attached since we already have 1 client.");
                Status = STATUS_DEVICE_ALREADY_ATTACHED;
            }
            else
            {
                LogInfo(DRIVER_IOCTL, "Client attached.");
                FilterDeviceExtension->HasClient = TRUE;
            }
            break;

        case IRP_MJ_CLEANUP:
            LogInfo(DRIVER_IOCTL, "Client cleaning up.");

            // Release pending IRP
            if (FilterDeviceExtension->PendingNotificationIRP)
            {
                IrpToCancel = FilterDeviceExtension->PendingNotificationIRP;
                FilterDeviceExtension->PendingNotificationIRP = NULL;
            }

            // Free all pending notifications
            Link = FilterDeviceExtension->PendingNotificationList.Flink;
            while (Link != &FilterDeviceExtension->PendingNotificationList)
            {
                PFILTER_NOTIFICATION_ENTRY pNotification = CONTAINING_RECORD(Link, FILTER_NOTIFICATION_ENTRY, Link);

                // Set next link
                Link = Link->Flink;

                // Free the memory
                NdisFreeMemory(pNotification, 0, 0);
            }
            InitializeListHead(&FilterDeviceExtension->PendingNotificationList);

            // Clear client flag so we don't pend more notifications.
            FilterDeviceExtension->HasClient = FALSE;
            break;

        case IRP_MJ_CLOSE:
            LogInfo(DRIVER_IOCTL, "Client detatched.");
            break;

        default:
            break;
    }

    NdisReleaseSpinLock(&FilterDeviceExtension->Lock);

    // Cancel the pending notification IRP if set
    if (IrpToCancel)
    {
        // Complete the pending IRP
        IrpToCancel->IoStatus.Status = STATUS_CANCELLED;
        IrpToCancel->IoStatus.Information = 0;
        IoCompleteRequest(IrpToCancel, IO_NO_INCREMENT);
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    LogFuncExitNT(DRIVER_IOCTL, Status);

    return Status;
}

_Use_decl_annotations_
NTSTATUS
otLwfDeviceIoControl(
    PDEVICE_OBJECT        DeviceObject,
    PIRP                  Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN CompleteIRP = TRUE;

    LogFuncEntry(DRIVER_IOCTL);

    PVOID               IoBuffer = Irp->AssociatedIrp.SystemBuffer;

    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG               InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG               OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG               IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

#if DBG
    ASSERT(((POTLWF_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Signature == 'FTDR');
#else
    UNREFERENCED_PARAMETER(DeviceObject);
#endif

    // We only allow PASSIVE_LEVEL calls
    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        LogWarning(DRIVER_IOCTL, "FilterDeviceIoControl called higher than PASSIVE.");
        status = STATUS_NOT_SUPPORTED;
        RtlZeroMemory(IoBuffer, OutputBufferLength);
        OutputBufferLength = 0;
        goto error;
    }

    // Check the IoControlCode to determine which IOCTL we are processing
    switch (IoControlCode)
    {
        case IOCTL_OTLWF_QUERY_NOTIFICATION:
            CompleteIRP = FALSE;
            status = otLwfQueryNextNotification(Irp);
            break;

        case IOCTL_OTLWF_ENUMERATE_INTERFACES:
            status =
                otLwfIoCtlEnumerateInterfaces(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_CREATE_NETWORK:
            status =
                otLwfIoCtlCreateNetwork(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_JOIN_NETWORK:
            status =
                otLwfIoCtlJoinNetwork(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_SEND_ROUTER_ID_REQUEST:
            status =
                otLwfIoCtlSendRouterIDRequest(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_DISCONNECT_NETWORK:
            status =
                otLwfIoCtlDisconnectNetwork(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_QUERY_NETWORK_ADDRESSES:
            status =
                otLwfIoCtlQueryNetworkAddresses(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_QUERY_MESH_STATE:
            status =
                otLwfIoCtlQueryMeshState(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        default:
            status = STATUS_NOT_IMPLEMENTED;
            RtlZeroMemory(IoBuffer, OutputBufferLength);
            OutputBufferLength = 0;
            break;
    }

error:

    if (CompleteIRP)
    {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = OutputBufferLength;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}

_Use_decl_annotations_
PMS_FILTER
otLwfFindAndRefInterface(
    _In_ PGUID  InterfaceGuid
    )
{
   PMS_FILTER pOutput = NULL;

   NdisAcquireSpinLock(&FilterListLock);

   for (PLIST_ENTRY Link = FilterModuleList.Flink; Link != &FilterModuleList; Link = Link->Flink)
   {
       PMS_FILTER pFilter = CONTAINING_RECORD(Link, MS_FILTER, FilterModuleLink);

       if (pFilter->State == FilterRunning &&
           memcmp(InterfaceGuid, &pFilter->InterfaceGuid, sizeof(GUID)) == 0)
       {
           // Increment ref count on interface
           RtlIncrementReferenceCount(&pFilter->IoControlReferences);
           pOutput = pFilter;
           break;
       }
   }

   NdisReleaseSpinLock(&FilterListLock);

   return pOutput;
}

_Use_decl_annotations_
VOID
otLwfReleaseInterface(
    _In_ PMS_FILTER pFilter
    )
{
    // Decrement ref count, and set shutdown event if it goes to 0
    if (RtlDecrementReferenceCount(&pFilter->IoControlReferences))
    {
        NdisSetEvent(&pFilter->IoControlShutdownComplete);
    }
}

//
// Notification Functions
//

// Indicates a new notification
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfIndicateNotification(
    _In_ PFILTER_NOTIFICATION_ENTRY NotifEntry
    )
{
    PIRP IrpToComplete = NULL;

    LogFuncEntry(DRIVER_DEFAULT);

    if (FilterDeviceExtension == NULL) goto error;

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    // Only do anything with notifications if we have a current client
    if (FilterDeviceExtension->HasClient)
    {
        // If there are other pending notifications or we don't have a pending IRP saved
        // then just go ahead and add the notification to the list
        if (!IsListEmpty(&FilterDeviceExtension->PendingNotificationList) ||
            FilterDeviceExtension->PendingNotificationIRP == NULL
            )
        {
            // Just insert into the list. It will eventually be drained
            InsertTailList(&FilterDeviceExtension->PendingNotificationList, &NotifEntry->Link);
            
            // Set the pointer to NULL so we don't free it at the end
            NotifEntry = NULL;
        }
        else
        {
            IrpToComplete = FilterDeviceExtension->PendingNotificationIRP;
            FilterDeviceExtension->PendingNotificationIRP = NULL;
        }
    }

    NdisReleaseSpinLock(&FilterDeviceExtension->Lock);

    // If we got the IRP, complete it with the notification now
    if (IrpToComplete)
    {
        // Before we are allowed to complete the pending IRP, we must remove the cancel routine
        KIRQL irql;
        IoAcquireCancelSpinLock(&irql);
        IoSetCancelRoutine(IrpToComplete, NULL);
        IoReleaseCancelSpinLock(irql);

        // Copy the notification payload
        PVOID IoBuffer = IrpToComplete->AssociatedIrp.SystemBuffer;
        memcpy(IoBuffer, &NotifEntry->Notif, sizeof(OTLWF_NOTIFICATION));
        IrpToComplete->IoStatus.Information = sizeof(OTLWF_NOTIFICATION);

        // Complete the IRP
        IrpToComplete->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(IrpToComplete, IO_NO_INCREMENT);
    }

error:

    // Free the notification if not cleared already
    if (NotifEntry)
    {
        NdisFreeMemory(NotifEntry, 0, 0);
    }

    LogFuncExit(DRIVER_DEFAULT);
}

DRIVER_CANCEL otLwfQueryNotificationCancelled;

_Use_decl_annotations_
VOID
otLwfQueryNotificationCancelled(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    LogFuncEntry(DRIVER_DEFAULT);

    FilterDeviceExtension->PendingNotificationIRP = NULL;
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    LogFuncExit(DRIVER_DEFAULT);
}

// Queries the next notification
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfQueryNextNotification(
    _In_ PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = NULL;

    LogFuncEntry(DRIVER_DEFAULT);

    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG  OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // Validate we have a big enough buffer
    if (OutputBufferLength < sizeof(OTLWF_NOTIFICATION))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    // Check to see if there are any notifications available
    if (IsListEmpty(&FilterDeviceExtension->PendingNotificationList))
    {
        // Set the cancel routine
        IoSetCancelRoutine(Irp, otLwfQueryNotificationCancelled);

        // Mark the Irp as pending
        IoMarkIrpPending(Irp);

        // Save the IRP to complete later, when we have a notification
        FilterDeviceExtension->PendingNotificationIRP = Irp;
    }
    else
    {
        // Grab the head of the list
        PLIST_ENTRY Link = RemoveHeadList(&FilterDeviceExtension->PendingNotificationList);

        // Get the notification
        NotifEntry = CONTAINING_RECORD(Link, FILTER_NOTIFICATION_ENTRY, Link);
    }

    NdisReleaseSpinLock(&FilterDeviceExtension->Lock);

    // If we found a notification, complete the IRP with it
    if (NotifEntry)
    {
        // Copy the notification payload
        PVOID IoBuffer = Irp->AssociatedIrp.SystemBuffer;
        memcpy(IoBuffer, &NotifEntry->Notif, sizeof(OTLWF_NOTIFICATION));
        Irp->IoStatus.Information = sizeof(OTLWF_NOTIFICATION);

        // Free the notification
        NdisFreeMemory(NotifEntry, 0, 0);
    }
    else
    {
        // Otherwise, set status to indicate we are pending the IRP
        status = STATUS_PENDING;
    }

error:

    // Complete the IRP if we aren't pending
    if (status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    LogFuncExitNT(DRIVER_DEFAULT, status);

    return status;
}
