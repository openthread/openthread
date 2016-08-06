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

/**
 * @file
 * @brief
 *  This file implements the functions for creating new notifications for IOCTL clients.
 */

#include "precomp.h"
#include "notify.tmh"

// Helper function to convert OTLWF_INTERFACE_STATE to a string for logging purposes
_IRQL_requires_max_(DISPATCH_LEVEL)
PCSTR
IfStateToString(_In_ OTLWF_INTERFACE_STATE state)
{
    switch (state)
    {
    case OTLWF_INTERFACE_STATE_UNSPECIFIED:
        return "OTLWF_INTERFACE_STATE_UNSPECIFIED";
    case OTLWF_INTERFACE_STATE_DISCONNECTED:
        return "OTLWF_INTERFACE_STATE_DISCONNECTED";
    case OTLWF_INTERFACE_STATE_DISCONNECTING:
        return "OTLWF_INTERFACE_STATE_DISCONNECTING";
    case OTLWF_INTERFACE_STATE_CREATING_NEW_NETWORK:
        return "OTLWF_INTERFACE_STATE_CREATING_NEW_NETWORK";
    case OTLWF_INTERFACE_STATE_REQUESTING_PARENT:
        return "OTLWF_INTERFACE_STATE_REQUESTING_PARENT";
    case OTLWF_INTERFACE_STATE_REQUESTING_CHILD_ID:
        return "OTLWF_INTERFACE_STATE_REQUESTING_CHILD_ID";
    case OTLWF_INTERFACE_STATE_JOINED:
        return "OTLWF_INTERFACE_STATE_JOINED";
    }
    return "Invalid OTLWF_INTERFACE_STATE";
}

// Helper function to convert OTLWF_INTERFACE_STATE to a string for logging purposes
_IRQL_requires_max_(DISPATCH_LEVEL)
PCSTR
RoleToString(_In_ otDeviceRole role)
{
    switch (role)
    {
    case kDeviceRoleDisabled:
        return "Disabled";
    case kDeviceRoleDetached:
        return "Detached";
    case kDeviceRoleChild:
        return "Child";
    case kDeviceRoleRouter:
        return "Router";
    case kDeviceRoleLeader:
        return "Leader";
    }
    return "Invalid otDeviceRole";
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfNotifyRoleStateChange(
    _In_ PMS_FILTER             pFilter
    )
{
    otDeviceRole prevRole = pFilter->otCachedRole;
    pFilter->otCachedRole = otGetDeviceRole(pFilter->otCtx);
    if (prevRole == pFilter->otCachedRole) return;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! new role: %s", &pFilter->InterfaceGuid, RoleToString(pFilter->otCachedRole));

    if (IsAttached(prevRole) != IsAttached(pFilter->otCachedRole))
    {
        PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
        if (NotifEntry)
        {
            RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
            NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
            NotifEntry->Notif.NotifType = OTLWF_NOTIF_INTERFACE_STATE;
            NotifEntry->Notif.InterfaceStatePayload.NewState = 
                IsAttached(pFilter->otCachedRole) ? 
                    OTLWF_INTERFACE_STATE_JOINED : 
                    OTLWF_INTERFACE_STATE_DISCONNECTED;
            
            LogInfo(DRIVER_DEFAULT, 
                    "Interface %!GUID! new state: %s", 
                    &pFilter->InterfaceGuid, 
                    IfStateToString(NotifEntry->Notif.InterfaceStatePayload.NewState));

            otLwfIndicateNotification(NotifEntry);
        }
    }
    
    if (pFilter->otCachedRole == kDeviceRoleChild || 
        pFilter->otCachedRole == kDeviceRoleRouter)
    {
        if (pFilter->otCachedRole == kDeviceRoleChild)
        {
            //LogInfo("Interface %!GUID! now acting as Child %u of Router %u.",
            //        &pFilter->InterfaceGuid, pFilter->Roles.Child.ChildID, pFilter->Roles.Child.ParentID);
        }
        else if (pFilter->otCachedRole == kDeviceRoleRouter)
        {
            //LogInfo("Interface %!GUID! now acting as Router %u.",
            //        &pFilter->InterfaceGuid, pFilter->Roles.Router.RouterID);
        }

        PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
        if (NotifEntry)
        {
            RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
            NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
            NotifEntry->Notif.NotifType = OTLWF_NOTIF_ROLE_STATE;
            NotifEntry->Notif.RoleStatePayload.NewState = 
                pFilter->otCachedRole == kDeviceRoleChild ? 
                    OTLWF_ROLE_STATE_CHILD : 
                    OTLWF_ROLE_STATE_ROUTER;

            otLwfIndicateNotification(NotifEntry);
        }
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfNotifyChildrenStateChange(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                Added,
    _In_ USHORT                 ChildID,
    _In_reads_bytes_(ThreadExtendedMacAddressLength)
         PUCHAR                 ChildMACAddress
    )
/*++

Routine Description:

    Builds up a notification for the children state change and 
    queues it up for the clients.

Arguments:

    pFilter         - The Filter context

--*/
{
    UNREFERENCED_PARAMETER(ChildID);
    UNREFERENCED_PARAMETER(ChildMACAddress);

    if (Added)
    {
        //LogInfo(DRIVER_DEFAULT, "Interface %!GUID! added new child, ID = %d, MAC = %!MAC8ADDR!", 
        //        &pFilter->InterfaceGuid, ChildID, ChildMACAddress);
    }
    else
    {
        //LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removed child, ID = %d, MAC = %!MAC8ADDR!", 
        //        &pFilter->InterfaceGuid, ChildID, ChildMACAddress);
    }

    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_CHILDREN_STATE;
        //NotifEntry->Notif.ChildrenStatePayload.CountOfChildren = pFilter->Roles.Router.Children.Size();

        otLwfIndicateNotification(NotifEntry);
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfNotifyNeighborStateChange(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                Added,
    _In_ USHORT                 RouterID,
    _In_reads_bytes_(ThreadExtendedMacAddressLength)
         PUCHAR                 RouterMACAddress
    )
/*++

Routine Description:

    Builds up a notification for the neighbors state change and 
    queues it up for the clients.

Arguments:

    pFilter         - The Filter context

--*/
{
    UNREFERENCED_PARAMETER(RouterID);
    UNREFERENCED_PARAMETER(RouterMACAddress);

    if (Added)
    {
        //LogInfo(DRIVER_DEFAULT, "Interface %!GUID! added new neighbor, ID = %d, MAC = %!MAC8ADDR!",
        //        &pFilter->InterfaceGuid, RouterID, RouterMACAddress);
    }
    else
    {
        //LogInfo(DRIVER_DEFAULT, "Interface %!GUID! removed neighbor, ID = %d, MAC = %!MAC8ADDR!",
        //        &pFilter->InterfaceGuid, RouterID, RouterMACAddress);
    }

    PFILTER_NOTIFICATION_ENTRY NotifEntry = FILTER_ALLOC_NOTIF(pFilter);
    if (NotifEntry)
    {
        RtlZeroMemory(NotifEntry, sizeof(FILTER_NOTIFICATION_ENTRY));
        NotifEntry->Notif.InterfaceGuid = pFilter->InterfaceGuid;
        NotifEntry->Notif.NotifType = OTLWF_NOTIF_NEIGHBOR_STATE;
        //NotifEntry->Notif.NeighborStatePayload.CountOfNeighbors = pFilter->RoutingData.Neighbors.Size();

        otLwfIndicateNotification(NotifEntry);
    }
}
