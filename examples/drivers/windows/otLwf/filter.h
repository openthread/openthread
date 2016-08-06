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
 *  This file defines the structures and functions for the otLwf Filter instance.
 */

#ifndef _FILT_H
#define _FILT_H

// The maximum allowed addresses an OpenThread interface
#define OT_MAX_ADDRESSES 10

#define OTLWF_ALLOC_TAG 'mFto' // otFm

#define FILTER_ALLOC_MEM(_NdisHandle, _Size) \
    NdisAllocateMemoryWithTagPriority(_NdisHandle, _Size, OTLWF_ALLOC_TAG, LowPoolPriority)
#define FILTER_FREE_MEM(_pMem)      NdisFreeMemory(_pMem, 0, 0)
#define FILTER_INIT_LOCK(_pLock)    NdisAllocateSpinLock(_pLock)
#define FILTER_FREE_LOCK(_pLock)    NdisFreeSpinLock(_pLock)

// Helper for locking an NDIS lock
#define FILTER_ACQUIRE_LOCK(_pLock, DispatchLevel)          \
{                                                           \
    if (DispatchLevel) { NdisDprAcquireSpinLock(_pLock); }  \
    else               { NdisAcquireSpinLock(_pLock);    }  \
}

// Helper for releasing an NDIS lock
#define FILTER_RELEASE_LOCK(_pLock, DispatchLevel)          \
{                                                           \
    if (DispatchLevel) { NdisDprReleaseSpinLock(_pLock); }  \
    else               { NdisReleaseSpinLock(_pLock);    }  \
}

//
// Enum of filter's states
// Filter can only be in one state at one time
//
typedef enum _FILTER_STATE
{
    FilterStateUnspecified,
    FilterPausing,
    FilterPaused,
    FilterRunning,
} FILTER_STATE;

#define OT_EVENT_TIMER_NOT_RUNNING  0
#define OT_EVENT_TIMER_RUNNING      1
#define OT_EVENT_TIMER_FIRED        2

//
// Define the filter struct
//
typedef struct _MS_FILTER
{
    // Entry in the global list of Filter instances
    LIST_ENTRY                      FilterModuleLink;

    // NDIS Handle for the Filter instance
    NDIS_HANDLE                     FilterHandle;

    // Current state (Running or not) of the Filter instance
    FILTER_STATE                    State;
    NDIS_EVENT                      FilterPauseComplete;
    
    //
    // Interface / Adapter variables
    //
    GUID                            InterfaceGuid;
    NET_IFINDEX                     InterfaceIndex;
    NET_LUID                        InterfaceLuid;
    COMPARTMENT_ID                  InterfaceCompartmentID;
    NDIS_STRING                     MiniportFriendlyName;
    volatile LONG                   PendingDisconnectTasks;

    //
    // Miniport Capabilities
    //
    OT_CAPABILITIES                 MiniportCapabilities;
    
    //
    // Pending OID Handling
    //
    NDIS_SPIN_LOCK                  PendingOidRequestLock;
    PNDIS_OID_REQUEST               PendingOidRequest;

    //
    // Io Control Path Synchronization
    //
    RTL_REFERENCE_COUNT             IoControlReferences;
    NDIS_EVENT                      IoControlShutdownComplete;

    //
    // Event processing
    //
    PVOID                           EventWorkerThread;
    KEVENT                          EventWorkerThreadStopEvent;
    KEVENT                          EventWorkerThreadProcessNBLs;
    NDIS_SPIN_LOCK                  EventsLock;
    _Guarded_by_(EventsLock)
    LIST_ENTRY                      NBLsHead;
    ULONG                           CountPendingRecvNBLs;
    LARGE_INTEGER                   NextAlarmTickCount;
    KEVENT                          EventWorkerThreadWaitTimeUpdated;
    KEVENT                          EventWorkerThreadProcessTasklets;
    PEX_TIMER                       EventHighPrecisionTimer;
    UCHAR                           EventTimerState;

    //
    // Data Path Synchronization
    //
    EX_RUNDOWN_REF                  DataPathRundown;
    NDIS_HANDLE                     NetBufferListPool;
    BOOLEAN                         SendPending;
    PNET_BUFFER_LIST                SendNetBufferList;
    KEVENT                          SendNetBufferListComplete;

    //
    // OpenThread context buffer
    //
    UCHAR                           otCtxBuffer[OT_CONTEXT_SIZE];
    otContext*                      otCtx;

    //
    // OpenThread state management
    //
    otDeviceRole                    otCachedRole;

    //
    // OpenThread addresses
    //
    IN6_ADDR                        otCachedAddr[OT_MAX_ADDRESSES];
    ULONG                           otCachedAddrCount;
    IN6_ADDR                        otLinkLocalAddr;
    
    //
    // OpenThread radio variables
    //
    PhyState                        otPhyState;
    uint8_t                         otCurrentListenChannel;
    uint8_t                         otReceiveMessage[kMaxPHYPacketSize];
    uint8_t                         otTransmitMessage[kMaxPHYPacketSize];
    RadioPacket                     otReceiveFrame;
    RadioPacket                     otTransmitFrame;

    BOOLEAN                         otPromiscuous;
    uint16_t                        otPanID;
    uint8_t                         otExtendedAddress[OT_EXT_ADDRESS_SIZE];
    uint16_t                        otShortAddress;


} MS_FILTER, * PMS_FILTER;

// Helper function that converts an otContext pointer to a MS_FILTER pointer
__inline PMS_FILTER otCtxToFilter(_In_ otContext* otCtx)
{
    return (PMS_FILTER)CONTAINING_RECORD(otCtx, MS_FILTER, otCtxBuffer);
}

// Helper function to indicate if a role means it is attached or not
_inline BOOLEAN IsAttached(_In_ otDeviceRole role)
{
    return role > kDeviceRoleDetached;
}

// Helper function to indicate OpenThread is ready to process a receive packet
_inline BOOLEAN IsListeningForPackets(_In_ PMS_FILTER pFilter)
{
    return pFilter->otPhyState == kStateReceive;
}

//
// function prototypes
//

FILTER_ATTACH FilterAttach;
FILTER_DETACH FilterDetach;
FILTER_RESTART FilterRestart;
FILTER_PAUSE FilterPause;
FILTER_STATUS FilterStatus;
FILTER_DEVICE_PNP_EVENT_NOTIFY FilterDevicePnPEventNotify;
FILTER_NET_PNP_EVENT FilterNetPnPEvent;
FILTER_SEND_NET_BUFFER_LISTS FilterSendNetBufferLists;
FILTER_RETURN_NET_BUFFER_LISTS FilterReturnNetBufferLists;
FILTER_SEND_NET_BUFFER_LISTS_COMPLETE FilterSendNetBufferListsComplete;
FILTER_RECEIVE_NET_BUFFER_LISTS FilterReceiveNetBufferLists;
FILTER_CANCEL_SEND_NET_BUFFER_LISTS FilterCancelSendNetBufferLists;

//
// Event Processing Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfEventProcessingStart(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingStop(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingUpdateWaitTime(
    _In_ PMS_FILTER             pFilter,
    _In_ ULONG                  waitTime
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingIndicateNewTasklet(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfEventProcessingIndicateNewNetBufferLists(
    _In_ PMS_FILTER             pFilter,
    _In_ BOOLEAN                DispatchLevel,
    _In_ BOOLEAN                Received,
    _In_ NDIS_PORT_NUMBER       PortNumber,
    _In_ PNET_BUFFER_LIST       NetBufferLists
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingCancelNetBufferLists(
    _In_ PMS_FILTER             pFilter,
    _In_ PVOID                  CancelId
    );

_Function_class_(EXT_CALLBACK)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
otLwfEventProcessingTimer(
    _In_ PEX_TIMER Timer,
    _In_opt_ PVOID Context
    );

//
// Data Path functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEnableDataPath(
    _In_ PMS_FILTER             pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfDisableDataPath(
    _In_ PMS_FILTER             pFilter
    );

//
// OpenThread callbacks
//

void otLwfStateChangedCallback(uint32_t aFlags, _In_ void *aContext);
void otLwfReceiveIp6DatagramCallback(_In_ otMessage aMessage, _In_ void *aContext);

//
// Notify Functions
//

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfNotifyRoleStateChange(
    _In_ PMS_FILTER             pFilter
    );

//
// Address Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
otLwfInitializeAddresses(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfAddressesUpdated(
    _In_ PMS_FILTER pFilter
    );

int 
otLwfFindCachedAddrIndex(
    _In_ PMS_FILTER pFilter, 
    _In_ PIN6_ADDR addr
    );

//
// Logging Helper
//

#ifdef LOG_BUFFERS
void
otLogBuffer(
    _In_reads_bytes_(BufferLength) PUCHAR Buffer,
    _In_                           ULONG  BufferLength
    );
#endif

#endif  //_FILT_H
