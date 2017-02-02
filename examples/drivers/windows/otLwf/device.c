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
                                  sizeof(OTLWF_DEVICE_EXTENSION),         // DeviceExtension
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
            InitializeListHead(&FilterDeviceExtension->ClientList);

            #pragma push
            #pragma warning(disable:28168) // The function 'otLwfDispatch' does not have a _Dispatch_type_ annotation matching dispatch table position *
            FilterDriverObject->MajorFunction[IRP_MJ_CREATE] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_CLEANUP] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_CLOSE] = otLwfDispatch;
            FilterDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = otLwfDeviceIoControl;
            #pragma pop

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

PIRP
otLwfDeviceClientCleanup(
    POTLWF_DEVICE_CLIENT DeviceClient
    )
{
    PIRP IrpToCancel = NULL;

    // Clean the FileObject context
    DeviceClient->FileObject->FsContext2 = NULL;

    // Release pending IRP
    if (DeviceClient->PendingNotificationIRP)
    {
        IrpToCancel = DeviceClient->PendingNotificationIRP;
        DeviceClient->PendingNotificationIRP = NULL;
    }

    // Free all pending notifications
    NT_ASSERT(DeviceClient->NotificationSize <= OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT);
    for (UCHAR i = 0; i < DeviceClient->NotificationSize; i++)
    {
        UCHAR index = (DeviceClient->NotificationOffset + i) % OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT;
        otLwfReleaseNotification(DeviceClient->PendingNotifications[index]);
    }

    return IrpToCancel;
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
        
        // Clean up all pending clients
        PLIST_ENTRY Link = FilterDeviceExtension->ClientList.Flink;
        while (Link != &FilterDeviceExtension->ClientList)
        {
            POTLWF_DEVICE_CLIENT DeviceClient = CONTAINING_RECORD(Link, OTLWF_DEVICE_CLIENT, Link);
            PIRP IrpToCancel = NULL;

            // Set next link
            Link = Link->Flink;
            
            // Make sure to clean up any left overs from the device client
            IrpToCancel = otLwfDeviceClientCleanup(DeviceClient);

            // Complete the pending IRP since we are shutting down
            if (IrpToCancel)
            {
                // Before we are allowed to complete the pending IRP, we must remove the cancel routine
                KIRQL irql;
                IoAcquireCancelSpinLock(&irql);
                IoSetCancelRoutine(IrpToCancel, NULL);
                IoReleaseCancelSpinLock(irql);

                IrpToCancel->IoStatus.Status = STATUS_CANCELLED;
                IrpToCancel->IoStatus.Information = 0;
                IoCompleteRequest(IrpToCancel, IO_NO_INCREMENT);
            }

            // Remove the device client from the list
            RemoveEntryList(&DeviceClient->Link);

            // Delete the device client
            NdisFreeMemory(DeviceClient, 0, 0);
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
    POTLWF_DEVICE_CLIENT    DeviceClient = NULL;

    UNREFERENCED_PARAMETER(DeviceObject);

    LogFuncEntry(DRIVER_IOCTL);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    switch (IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            LogInfo(DRIVER_IOCTL, "Client %p attached.", IrpStack->FileObject);

            if (FilterDeviceExtension->ClientListSize >= OTLWF_MAX_CLIENTS)
            {
                LogError(DRIVER_IOCTL, "Already have max clients!");
                Status = STATUS_TOO_MANY_SESSIONS;
                break;
            }

            DeviceClient = FILTER_ALLOC_DEVICE_CLIENT();
            if (DeviceClient)
            {
                RtlZeroMemory(DeviceClient, sizeof(OTLWF_DEVICE_CLIENT));
                DeviceClient->FileObject = IrpStack->FileObject;

                NT_ASSERT(IrpStack->FileObject->FsContext2 == NULL);
                IrpStack->FileObject->FsContext2 = DeviceClient;
                
                // Insert into the client list
                InsertTailList(&FilterDeviceExtension->ClientList, &DeviceClient->Link);
                FilterDeviceExtension->ClientListSize++;
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;

        case IRP_MJ_CLEANUP:
            LogInfo(DRIVER_IOCTL, "Client %p cleaning up.", IrpStack->FileObject);

            DeviceClient = (POTLWF_DEVICE_CLIENT)IrpStack->FileObject->FsContext2;

            // Make sure to clean up any left overs from the device client
            IrpToCancel = otLwfDeviceClientCleanup(DeviceClient);

            // Remove the device client from the list
            RemoveEntryList(&DeviceClient->Link);
            FilterDeviceExtension->ClientListSize--;

            // Delete the device client
            NdisFreeMemory(DeviceClient, 0, 0);
            break;

        case IRP_MJ_CLOSE:
            LogInfo(DRIVER_IOCTL, "Client %p detatched.", IrpStack->FileObject);
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

    PVOID               IoBuffer = Irp->AssociatedIrp.SystemBuffer;

    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG               InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG               OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG               IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    ULONG               FuncCode = (IoControlCode >> 2) & 0xFFF;

    LogFuncEntryMsg(DRIVER_IOCTL, "%p", IrpSp->FileObject);

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

    if (FuncCode >= MIN_OTLWF_IOCTL_FUNC_CODE && FuncCode <= MAX_OTLWF_IOCTL_FUNC_CODE)
    {
        CompleteIRP = FALSE;
        status = otLwfIoCtlOpenThreadControl(Irp);
        goto error;
    }

    // Check the IoControlCode to determine which IOCTL we are processing
    switch (IoControlCode)
    {
        case IOCTL_OTLWF_QUERY_NOTIFICATION:
            CompleteIRP = FALSE;
            status = otLwfQueryNextNotification(Irp);
            break;

        case IOCTL_OTLWF_ENUMERATE_DEVICES:
            status =
                otLwfIoCtlEnumerateInterfaces(
                    IoBuffer, InputBufferLength,
                    IoBuffer, &OutputBufferLength
                    );
            break;

        case IOCTL_OTLWF_QUERY_DEVICE:
            status =
                otLwfIoCtlQueryInterface(
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
            if (ExAcquireRundownProtection(&pFilter->ExternalRefs))
            {
                pOutput = pFilter;
            }
            break;
        }
    }

    NdisReleaseSpinLock(&FilterListLock);

    return pOutput;
}

//
// Notification Functions
//

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfReleaseNotification(
    _In_ PFILTER_NOTIFICATION_ENTRY NotifEntry
    )
{
    if (RtlDecrementReferenceCount(&NotifEntry->RefCount))
    {
        NdisFreeMemory(NotifEntry, 0, 0);
    }
}

// Indicates a new notification
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfIndicateNotification(
    _In_ PFILTER_NOTIFICATION_ENTRY NotifEntry
    )
{
    PIRP IrpsToComplete[OTLWF_MAX_CLIENTS] = {0};
    UCHAR IrpOffset = 0;

    LogFuncEntry(DRIVER_IOCTL);

    // Initialize with a local ref
    NotifEntry->RefCount = 1;

    if (FilterDeviceExtension == NULL) goto error;

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    // Pend the notification for each client
    PLIST_ENTRY Link = FilterDeviceExtension->ClientList.Flink;
    while (Link != &FilterDeviceExtension->ClientList)
    {
        POTLWF_DEVICE_CLIENT DeviceClient = CONTAINING_RECORD(Link, OTLWF_DEVICE_CLIENT, Link);

        // Set next link
        Link = Link->Flink;
        
        KIRQL irql;
        IoAcquireCancelSpinLock(&irql);

        // If there are other pending notifications or we don't have a pending IRP saved
        // then just go ahead and add the notification to the list
        NT_ASSERT(DeviceClient->NotificationSize <= OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT);
        if (DeviceClient->NotificationSize != 0 ||
            DeviceClient->PendingNotificationIRP == NULL
            )
        {
            // Calculate the next index
            UCHAR Index = (DeviceClient->NotificationOffset + DeviceClient->NotificationSize) % OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT;

            // Add additional ref to the notif
            RtlIncrementReferenceCount(&NotifEntry->RefCount);

            // If we are at the max already, release the oldest
            if (DeviceClient->NotificationSize == OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT)
            {
                LogWarning(DRIVER_IOCTL, "Dropping old notification!");
                otLwfReleaseNotification(DeviceClient->PendingNotifications[DeviceClient->NotificationOffset]);
                DeviceClient->NotificationOffset = (DeviceClient->NotificationOffset + 1) % OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT;
            }
            else
            {
                DeviceClient->NotificationSize++;
            }

            // Copy the notification to the next space
            DeviceClient->PendingNotifications[Index] = NotifEntry;
        }
        else
        {
            // Before we are allowed to complete the pending IRP, we must remove the cancel routine
            IoSetCancelRoutine(DeviceClient->PendingNotificationIRP, NULL);

            IrpsToComplete[IrpOffset] = DeviceClient->PendingNotificationIRP;
            IrpOffset++;

            DeviceClient->PendingNotificationIRP = NULL;
        }
        
        // Release the cancel spin lock
        IoReleaseCancelSpinLock(irql);
    }

    NdisReleaseSpinLock(&FilterDeviceExtension->Lock);

    // Complete any IRPs now, outside the lock
    for (UCHAR i = 0; i < IrpOffset; i++)
    {
        PIRP IrpToComplete = IrpsToComplete[i];

        // Copy the notification payload
        PVOID IoBuffer = IrpToComplete->AssociatedIrp.SystemBuffer;
        memcpy(IoBuffer, &NotifEntry->Notif, sizeof(OTLWF_NOTIFICATION));
        IrpToComplete->IoStatus.Information = sizeof(OTLWF_NOTIFICATION);

        // Complete the IRP
        IrpToComplete->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(IrpToComplete, IO_NO_INCREMENT);
    }

error:
    
    // Release local ref on the notification
    otLwfReleaseNotification(NotifEntry);

    LogFuncExit(DRIVER_IOCTL);
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

    LogFuncEntry(DRIVER_IOCTL);
    
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POTLWF_DEVICE_CLIENT DeviceClient = (POTLWF_DEVICE_CLIENT)IrpSp->FileObject->FsContext2;

    if (DeviceClient)
    {
        DeviceClient->PendingNotificationIRP = NULL;
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    LogFuncExit(DRIVER_IOCTL);
}

// Queries the next notification
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
otLwfQueryNextNotification(
    _In_ PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    POTLWF_DEVICE_CLIENT DeviceClient = NULL;
    PFILTER_NOTIFICATION_ENTRY NotifEntry = NULL;

    LogFuncEntry(DRIVER_IOCTL);

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    // Validate we have a big enough buffer
    if (OutputBufferLength < sizeof(OTLWF_NOTIFICATION))
    {
        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, OutputBufferLength);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }
    
    DeviceClient = (POTLWF_DEVICE_CLIENT)IrpSp->FileObject->FsContext2;
    if (DeviceClient == NULL)
    {
        status = STATUS_DEVICE_NOT_READY;
        goto error;
    }

    NdisAcquireSpinLock(&FilterDeviceExtension->Lock);

    // Check to see if there are any notifications available
    if (DeviceClient->NotificationSize == 0)
    {
        // Set the cancel routine
        IoSetCancelRoutine(Irp, otLwfQueryNotificationCancelled);

        // Mark the Irp as pending
        IoMarkIrpPending(Irp);

        // Save the IRP to complete later, when we have a notification
        DeviceClient->PendingNotificationIRP = Irp;
    }
    else
    {
        // Get the notification
        NotifEntry = DeviceClient->PendingNotifications[DeviceClient->NotificationOffset];
        DeviceClient->PendingNotifications[DeviceClient->NotificationOffset] = NULL;

        // Increment the offset and decrement the size
        DeviceClient->NotificationOffset = (DeviceClient->NotificationOffset + 1) % OTLWF_MAX_PENDING_NOTIFICATIONS_PER_CLIENT;
        DeviceClient->NotificationSize--;
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
        otLwfReleaseNotification(NotifEntry);
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

    LogFuncExitNT(DRIVER_IOCTL, status);

    return status;
}
