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

/**
 * @file
 * @brief
 *  This file defines the structures and functions for the otLwf Filter instance.
 */

#ifndef _FILT_H
#define _FILT_H

// The maximum allowed addresses an OpenThread interface
#if (OPENTHREAD_ENABLE_DHCP6_CLIENT && OPENTHREAD_ENABLE_DHCP6_SERVER)
#define OT_MAX_ADDRESSES (4 + OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES + 2 * OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES)
#elif (OPENTHREAD_ENABLE_DHCP6_CLIENT || OPENTHREAD_ENABLE_DHCP6_SERVER)
#define OT_MAX_ADDRESSES (4 + OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES + OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES)
#else
#define OT_MAX_ADDRESSES (4 + OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES)
#endif

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

// Flags for the different device capabilities
typedef enum OTLWF_DEVICE_CAPABILITY
{
    // Device supports raw Radio commands
    OTLWF_DEVICE_CAP_RADIO                                      = 1 << 0,

    // Device supports Ack timeouts internally
    OTLWF_DEVICE_CAP_RADIO_ACK_TIMEOUT                          = 1 << 1,

    // Device supports MAC retry logic and timers; as well as collision avoidance.
    OTLWF_DEVICE_CAP_RADIO_MAC_RETRY_AND_COLLISION_AVOIDANCE    = 1 << 2,

    // Device supports the energy scan command.
    OTLWF_DEVICE_CAP_RADIO_ENERGY_SCAN                          = 1 << 3,

    // Device support Net & Thread commands.
    OTLWF_DEVICE_CAP_THREAD_1_0                                 = 1 << 16,

} OTLWF_DEVICE_CAPABILITY;

// Flags for the different device capabilities
typedef enum OTLWF_DEVICE_STATUS
{
    OTLWF_DEVICE_STATUS_UNINTIALIZED,   // Not yet initialzied.
    OTLWF_DEVICE_STATUS_RADIO_MODE,     // The device is just operating as a simple radio.
    OTLWF_DEVICE_STATUS_THREAD_MODE     // The device is managing the Thread stack.

} OTLWF_DEVICE_STATUS;

#define OT_EVENT_TIMER_NOT_RUNNING  0
#define OT_EVENT_TIMER_RUNNING      1
#define OT_EVENT_TIMER_FIRED        2

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

typedef struct BufferPool
{
    struct BufferPool* Next;
    uint8_t Buffers[0];

} BufferPool;

enum
{
    kPageSize                = PAGE_SIZE,
    kPagesPerBufferPool      = 1,
    kMaxPagesForBufferPools  = 64,
    kMaxBytesForBufferPools  = kPageSize * kMaxPagesForBufferPools,

    kEstimatedBufferSize     = 128,         // sizeof(ot::Buffer)
    kEstimatedBufferPoolSize = ((kPageSize * kPagesPerBufferPool) - sizeof(BufferPool)) / kEstimatedBufferSize,
    kEstimatedMaxBuffers     = kMaxPagesForBufferPools * kEstimatedBufferPoolSize
};

#endif

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

    // Handle for unicast IP address notifications
    HANDLE                          AddressChangeHandle;

    //
    // Interface variables
    //
    GUID                            InterfaceGuid;
    NET_IFINDEX                     InterfaceIndex;
    NET_LUID                        InterfaceLuid;
    COMPARTMENT_ID                  InterfaceCompartmentID;
    NDIS_STRING                     InterfaceFriendlyName;
    HANDLE                          InterfaceRegKey;

    //
    // Miniport Link State
    //
    NDIS_LINK_STATE                 MiniportLinkState;

    //
    // External references management
    //   Used for IOCTLs, SendNBLs, and Address Changed callbacks
    //
    EX_RUNDOWN_REF                  ExternalRefs;

    //
    // Spinel Command State
    //
    EX_RUNDOWN_REF                  cmdRundown;
    NDIS_SPIN_LOCK                  cmdLock;
    LIST_ENTRY                      cmdHandlers;
    USHORT                          cmdTIDsInUse;
    spinel_tid_t                    cmdNextTID;
    NDIS_HANDLE                     cmdNblPool;
#ifdef COMMAND_INIT_RETRY
    ULONG                           cmdInitTryCount;
#endif
    otPlatResetReason               cmdResetReason;
    KEVENT                          cmdResetCompleteEvent;

    //
    // Device Capabilities / State
    //
    OTLWF_DEVICE_CAPABILITY         DeviceCapabilities;
    OTLWF_DEVICE_STATUS             DeviceStatus;

    //
    // OpenThread addresses
    //
    IN6_ADDR                    otCachedAddr[OT_MAX_ADDRESSES];
    ULONG                       otCachedAddrCount;
    IN6_ADDR                    otLinkLocalAddr;

    union
    {
    struct // Thread Mode Variables
    {
        //
        // OpenThread Event processing
        //
        PVOID                       EventWorkerThread;
        KEVENT                      EventWorkerThreadStopEvent;
        KEVENT                      EventWorkerThreadProcessAddressChanges;
        KEVENT                      EventWorkerThreadProcessNBLs;
        KEVENT                      EventWorkerThreadProcessMacFrames;
        NDIS_SPIN_LOCK              EventsLock;
        LIST_ENTRY                  AddressChangesHead;
        LIST_ENTRY                  NBLsHead;
        LIST_ENTRY                  MacFramesHead;
        LARGE_INTEGER               NextAlarmTickCount;
        KEVENT                      EventWorkerThreadWaitTimeUpdated;
        KEVENT                      EventWorkerThreadProcessTasklets;
        PEX_TIMER                   EventHighPrecisionTimer;
        UCHAR                       EventTimerState;
        LIST_ENTRY                  EventIrpListHead;
        KEVENT                      EventWorkerThreadProcessIrp;
        KEVENT                      EventWorkerThreadEnergyScanComplete;

        //
        // OpenThread Settings Management
        //
        HANDLE                      otSettingsRegKey;

        //
        // OpenThread state management
        //
        otDeviceRole                otCachedRole;

        //
        // OpenThread data path state
        //
        BOOLEAN                     SendPending;
        KEVENT                      SendNetBufferListComplete;
    
        //
        // OpenThread radio variables
        //
        otRadioCaps                 otRadioCapabilities;
        otRadioState                otRadioState;
        uint8_t                     otCurrentListenChannel;
        uint8_t                     otReceiveMessage[OT_RADIO_FRAME_MAX_SIZE];
        uint8_t                     otTransmitMessage[OT_RADIO_FRAME_MAX_SIZE];
        otRadioFrame                otReceiveFrame;
        otRadioFrame                otTransmitFrame;
        otError                     otLastTransmitError;
        BOOLEAN                     otLastTransmitFramePending;
        CHAR                        otLastEnergyScanMaxRssi;

        BOOLEAN                     otPromiscuous;
        uint16_t                    otPanID;
        uint64_t                    otFactoryAddress;
        uint64_t                    otExtendedAddress;
        uint16_t                    otShortAddress;

        BOOLEAN                     otPendingMacOffloadEnabled;

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        uint16_t                    otBufferSize;               // Bytes in a single buffer
        uint16_t                    otBufferPoolByteSize;       // Bytes in a buffer pool
        uint16_t                    otBufferPoolBufferCount;    // Number of buffers in a pool
        uint16_t                    otBuffersLeft;              // Number of buffers left to return
        BufferPool*                 otBufferPoolHead;           // List of buffer pools
        otMessage*                  otFreeBuffers;              // List of buffers to return
#endif

#if DEBUG_ALLOC
        // Used for tracking memory allocations
        HANDLE                      otThreadId;
        volatile LONG               otOutstandingAllocationCount;
        volatile LONG               otOutstandingMemoryAllocated;
        LIST_ENTRY                  otOutStandingAllocations;
        ULONG                       otAllocationID;
#endif

        //
        // OpenThread Joiner Vendor Info
        //
        char otVendorName[OPENTHREAD_VENDOR_NAME_MAX_LENGTH + 1];
        char otVendorModel[OPENTHREAD_VENDOR_MODEL_MAX_LENGTH + 1];
        char otVendorSwVersion[OPENTHREAD_VENDOR_SW_VERSION_MAX_LENGTH + 1];
        char otVendorData[OPENTHREAD_VENDOR_DATA_MAX_LENGTH + 1];

        //
        // OpenThread context buffer
        //
        otInstance*                 otCtx;
        size_t                      otInstanceSize;
        PUCHAR                      otInstanceBuffer;
    };
    struct // Tunnel Mode Variables
    {
        PVOID                       TunWorkerThread;
        KEVENT                      TunWorkerThreadStopEvent;
        KEVENT                      TunWorkerThreadAddressChangedEvent;
    };
    };

} MS_FILTER, * PMS_FILTER;

//
// NDIS Filter Functions
//

FILTER_ATTACH FilterAttach;
FILTER_DETACH FilterDetach;
FILTER_RESTART FilterRestart;
FILTER_PAUSE FilterPause;
FILTER_STATUS FilterStatus;
FILTER_SEND_NET_BUFFER_LISTS FilterSendNetBufferLists;
FILTER_RETURN_NET_BUFFER_LISTS FilterReturnNetBufferLists;
FILTER_SEND_NET_BUFFER_LISTS_COMPLETE FilterSendNetBufferListsComplete;
FILTER_RECEIVE_NET_BUFFER_LISTS FilterReceiveNetBufferLists;
FILTER_CANCEL_SEND_NET_BUFFER_LISTS FilterCancelSendNetBufferLists;

//
// Link State Functions
//

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
otLwfIndicateLinkState(
    _In_ PMS_FILTER                 pFilter,
    _In_ NDIS_MEDIA_CONNECT_STATE   MediaState
    );

//
// Compartment Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
otLwfSetCompartment(
    _In_  PMS_FILTER                pFilter,
    _Out_ COMPARTMENT_ID*           pOriginalCompartment
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfRevertCompartment(
    _In_ COMPARTMENT_ID             OriginalCompartment
    );

//
// Address Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NETIOAPI_API_ 
otLwfAddressChangeCallback(
    _In_ PVOID CallerContext,
    _In_opt_ PMIB_UNICASTIPADDRESS_ROW Row,
    _In_ MIB_NOTIFICATION_TYPE NotificationType
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
otLwfEventProcessingAddressChanged(
    _In_ PMS_FILTER             pFilter,
    _In_ MIB_NOTIFICATION_TYPE  NotificationType,
    _In_ PIN6_ADDR              pAddr
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
otLwfInitializeAddresses(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfRadioAddressesUpdated(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID 
otLwfTunAddressesUpdated(
    _In_ PMS_FILTER pFilter,
    _In_reads_bytes_(value_data_len) const uint8_t* value_data_ptr,
    _In_ spinel_size_t value_data_len,
    _Out_ uint32_t *aNotifFlags
    );

int 
otLwfFindCachedAddrIndex(
    _In_ PMS_FILTER pFilter, 
    _In_ PIN6_ADDR addr
    );

//
// Tunnel Logic Functions
//

_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS 
otLwfInitializeTunnelMode(
    _In_ PMS_FILTER pFilter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void 
otLwfUninitializeTunnelMode(
    _In_ PMS_FILTER pFilter
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

//
// Debug Helpers
//

#if DEBUG_ALLOC

typedef struct _OT_ALLOC
{
    LIST_ENTRY Link;
    LONG Length;
    ULONG ID;
} OT_ALLOC;

PMS_FILTER
otLwfFindFromCurrentThread();

#endif

#endif  //_FILT_H
