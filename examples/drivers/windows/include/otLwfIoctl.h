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
 *   This file defines the IOCTL interface for otLwf.sys.
 */

#ifndef __OTLWFIOCTL_H__
#define __OTLWFIOCTL_H__

#include <in6addr.h>

// User-mode IOCTL path for CreateFile
#define OTLWF_IOCLT_PATH      TEXT("\\\\.\\\\otlwf")

//
// IOCLTs and Data Types
//

#define OTLWF_CTL_CODE(request, method, access) \
    CTL_CODE(FILE_DEVICE_NETWORK, request, method, access)

#pragma pack(push)
#pragma pack(1)

// Different possible notification types
typedef enum _OTLWF_NOTIF_TYPE
{
    OTLWF_NOTIF_UNSPECIFIED,
    OTLWF_NOTIF_INTERFACE_ARRIVAL,
    OTLWF_NOTIF_INTERFACE_REMOVAL,
    OTLWF_NOTIF_INTERFACE_STATE,
    OTLWF_NOTIF_ROLE_STATE,
    OTLWF_NOTIF_CHILDREN_STATE,
    OTLWF_NOTIF_NEIGHBOR_STATE

} OTLWF_NOTIF_TYPE;

// Different possible states for the interface
typedef enum _OTLWF_INTERFACE_STATE
{
    OTLWF_INTERFACE_STATE_UNSPECIFIED,
    OTLWF_INTERFACE_STATE_DISCONNECTED,
    OTLWF_INTERFACE_STATE_DISCONNECTING,
    OTLWF_INTERFACE_STATE_CREATING_NEW_NETWORK,
    OTLWF_INTERFACE_STATE_REQUESTING_PARENT,
    OTLWF_INTERFACE_STATE_REQUESTING_CHILD_ID,
    OTLWF_INTERFACE_STATE_JOINED

} OTLWF_INTERFACE_STATE;

// Different possible role states for the interface
typedef enum _OTLWF_ROLE_STATE
{
    OTLWF_ROLE_STATE_UNSPECIFIED,
    OTLWF_ROLE_STATE_CHILD,
    OTLWF_ROLE_STATE_ROUTER

} OTLWF_ROLE_STATE;

//
// Queries (async) the next notification in the queue
//
#define IOCTL_OTLWF_QUERY_NOTIFICATION \
    OTLWF_CTL_CODE(0, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_NOTIFICATION {
        GUID                InterfaceGuid;
        OTLWF_NOTIF_TYPE   NotifType;
        union
        {
            // Payload for OTLWF_NOTIF_INTERFACE_STATE
            struct
            {
                OTLWF_INTERFACE_STATE  NewState;
            } InterfaceStatePayload;

            // Payload for OTLWF_NOTIF_ROLE_STATE
            struct
            {
                OTLWF_ROLE_STATE       NewState;
            } RoleStatePayload;

            // Payload for OTLWF_NOTIF_CHILDREN_STATE
            struct
            {
                ULONG                   CountOfChildren;
            } ChildrenStatePayload;

            // Payload for OTLWF_NOTIF_NEIGHBOR_STATE
            struct
            {
                ULONG                   CountOfNeighbors;
            } NeighborStatePayload;
        };
    } OTLWF_NOTIFICATION, *POTLWF_NOTIFICATION;

//
// Enumerates all the Thread interfaces
//
#define IOCTL_OTLWF_ENUMERATE_INTERFACES \
    OTLWF_CTL_CODE(1, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_INTERFACE {
        GUID  InterfaceGuid;
        WCHAR MiniportFriendlyName[128];
        OTLWF_INTERFACE_STATE InterfaceState;
        ULONG CompartmentID;
    } OTLWF_INTERFACE, *POTLWF_INTERFACE;
    typedef struct _OTLWF_INTERFACE_LIST
    {
        USHORT cInterfaces;
        OTLWF_INTERFACE pInterfaces[1];
    } OTLWF_INTERFACE_LIST, *POTLWF_INTERFACE_LIST;

// Flags indicating which optional parameters in OTLWF_NETWORK_PARAMS are set
#define OTLWF_NETWORK_PARAM_EXTENDED_MAC_ADDRESS   0x0001
#define OTLWF_NETWORK_PARAM_MESH_LOCAL_IID         0x0002
#define OTLWF_NETWORK_PARAM_MAX_CHILDREN           0x0004

//
// Starts a new Thread network
//
#define IOCTL_OTLWF_CREATE_NETWORK \
    OTLWF_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_DATA)
    typedef struct _OTLWF_NETWORK_PARAMS {
        USHORT  Channel;
        USHORT  PANID;
        UCHAR   ExtendedPANID[8];
        CHAR    NetworkName[16];
        UCHAR   MasterKey[16];
        BOOLEAN RouterEligibleEndDevice;

        // Optional parameters
        ULONG   OptionalFlags;
        UCHAR   ExtendedMACAddress[8];
        UCHAR   MeshLocalIID[8];
        UCHAR   MaxChildren;
    } OTLWF_NETWORK_PARAMS, *POTLWF_NETWORK_PARAMS;
    // GUID - InterfaceGuid
    // OTLWF_NETWORK_PARAMS

//
// Joins an existing Thread network
//
#define IOCTL_OTLWF_JOIN_NETWORK \
    OTLWF_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid
    // OTLWF_NETWORK_PARAMS

//
// Request to be a router in the Thread network
//
#define IOCTL_OTLWF_SEND_ROUTER_ID_REQUEST \
    OTLWF_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_DATA)
// GUID - InterfaceGuid

//
// Disconnects from the Thread network
//
#define IOCTL_OTLWF_DISCONNECT_NETWORK \
    OTLWF_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_DATA)
    // GUID - InterfaceGuid

// Helper class for decoding an RLOC16, also known as a Short Address
typedef struct _OTLWF_RLOC16
{
    USHORT RouterID : 5; // In network byte order
    USHORT Reserved : 1;
    USHORT ChildID  : 9; // In network byte order

#ifdef __cplusplus
    bool IsRouter() const { return ChildID == 0; }
#endif

} OTLWF_RLOC16, *POTLWF_RLOC16;

const size_t ThreadRLCO16Size = sizeof(OTLWF_RLOC16);

//
// Queries the Network (L2 & L3) Addresses of an Interface
//
#define IOCTL_OTLWF_QUERY_NETWORK_ADDRESSES \
    OTLWF_CTL_CODE(6, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_NETWORK_ADDRESSES {
        UCHAR    ExtendedMACAddress[8];
        IN6_ADDR LL_EID;
        IN6_ADDR RLOC;
        IN6_ADDR ML_EID;
#ifdef __cplusplus
        POTLWF_RLOC16 RLOC16() const { return (POTLWF_RLOC16)(RLOC.u.Byte + 14); }
#endif
    } OTLWF_NETWORK_ADDRESSES, *POTLWF_NETWORK_ADDRESSES;
    // GUID - InterfaceGuid
    // OTLWF_NETWORK_STATE

//
// Queries the Mesh state of an Interface
//
#define IOCTL_OTLWF_QUERY_MESH_STATE \
    OTLWF_CTL_CODE(7, METHOD_BUFFERED, FILE_READ_DATA)
    typedef struct _OTLWF_CHILD_LINK {
        USHORT  ChildID;
        UCHAR   ExtendedMacAddress[8];
    } OTLWF_CHILD_LINK, *POTLWF_CHILD_LINK;
    typedef struct _OTLWF_NEIGBHOR_LINK {
        UCHAR   RouterID;
        UCHAR   ExtendedMacAddress[8];
        UCHAR   IncomingLinkQuality;
        UCHAR   OutgoingLinkQuality;
    } OTLWF_NEIGBHOR_LINK, *POTLWF_NEIGBHOR_LINK;
    typedef struct _OTLWF_MESH_STATE {
        UCHAR                   cChildren;
        UCHAR                   cNeighbors;
        POTLWF_CHILD_LINK      Children;
        POTLWF_NEIGBHOR_LINK   Neighbors;
    } OTLWF_MESH_STATE, *POTLWF_MESH_STATE;
    // GUID - InterfaceGuid
    // OTLWF_MESH_STATE
    
    #define OTLWF_MESH_STATE_SIZE(cChildren, cNeighbors) \
        sizeof(OTLWF_MESH_STATE) \
        + (cChildren * sizeof(OTLWF_CHILD_LINK)) \
        + (cNeighbors * sizeof(OTLWF_NEIGBHOR_LINK))

#pragma pack(pop)

#endif //__OTLWFIOCTL_H__
