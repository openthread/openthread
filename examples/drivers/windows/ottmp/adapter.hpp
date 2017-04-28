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
 *   Header file for the routines related to NETADAPTER Object for the 
 *   Network Adapter
 */

#define NIC_TAG_RECV_NBL ((ULONG)'rMVT')  // TVMr
#define OTTMP_ADAPTER_CONTEXT_SIGNATURE 'pdAt'

// The maximum size of one Spinel command / serial packet
#define MAX_SPINEL_COMMAND_LENGTH     1300

typedef struct _SERIAL_SEND_ITEM
{
    LIST_ENTRY          Link;
    PNET_BUFFER_LIST    NetBufferList;
    WDFMEMORY           WdfMemory;
    ULONG               EncodedBufferLength;
    _Field_size_bytes_(EncodedBufferLength)
    UCHAR               EncodedBuffer[0];

} SERIAL_SEND_ITEM, *PSERIAL_SEND_ITEM;

const ULONG SERIAL_SEND_ITEM_SIZE = FIELD_OFFSET(SERIAL_SEND_ITEM, EncodedBuffer);

typedef struct _OTTMP_ADAPTER_CONTEXT {
    ULONG                           Signature;

#ifdef OTTMP_LEGACY
    NDIS_HANDLE                     Adapter;
#else
    //
    // Handle to the NETADAPTER object for this context
    //
    NETADAPTER                      Adapter;
#endif

    //
    // Handle to the corresponding WDFDEVICE
    //
    WDFDEVICE                       Device;

    // Flag to indicate if the data path is enabled
    bool                            IsConnected;

    // Flag to indicate if the Adapter has been started or not
    bool                            IsRunning;
    
#ifdef OTTMP_LEGACY
    PGLOBALS                        pGlobals;
#else
    // Receive packet pool
    NETBUFFERLISTCOLLECTION         ReceiveCollection;
#endif
    
    ULONGLONG                       ExtendedAddress; // TODO - Cache

    //
    // Serial Device
    //

    WDFIOTARGET                     WdfIoTarget;

    WDFSPINLOCK                     SendLock;
    _Guarded_by_(SendLock)
    LIST_ENTRY                      SendQueue;
    bool                            SendQueueRunning;
    WDFWORKITEM                     SendWorkItem;
    
    WDFWORKITEM                     RecvWorkItem;
    WDFREQUEST                      RecvReadRequest;

    UCHAR                           RecvBuffer[MAX_SPINEL_COMMAND_LENGTH * 2];
    ULONG                           RecvBufferLength;

    //
    // NIC configuration - This information is queried by the protocol drivers.
    // Since this sample focuses on demonstrating the NDIS WDF model, 
    // it doesn't modify any these values during runtime.
    // Please look at the netvmini630 sample to see how these values can be set
    // and updated
    // -------------------------------------------------------------------------
    //
    ULONG                   PacketFilter;
    ULONG                   ulLookahead;
    ULONG64                 ulLinkSendSpeed;
    ULONG64                 ulLinkRecvSpeed;
    ULONG                   ulMaxBusySends;
    ULONG                   ulMaxBusyRecvs;

    //
    // Statistics - 
    // Since this sample focuses on demonstrating the NDIS WDF model, 
    // it doesn't modify any these values during runtime.
    // Please look at the netvmini630 sample to see how these values can be set
    // and updated
    // -------------------------------------------------------------------------
    //

    // Packet counts
    ULONG64                 FramesRxDirected;
    ULONG64                 FramesRxMulticast;
    ULONG64                 FramesRxBroadcast;
    ULONG64                 FramesTxDirected;
    ULONG64                 FramesTxMulticast;
    ULONG64                 FramesTxBroadcast;

    // Byte counts
    ULONG64                 BytesRxDirected;
    ULONG64                 BytesRxMulticast;
    ULONG64                 BytesRxBroadcast;
    ULONG64                 BytesTxDirected;
    ULONG64                 BytesTxMulticast;
    ULONG64                 BytesTxBroadcast;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;
    ULONG                   TransmitFailuresOther;

    // Count of receive errors
    ULONG                   RxCrcErrors;
    ULONG                   RxAlignmentErrors;
    ULONG                   RxResourceErrors;
    ULONG                   RxDmaOverrunErrors;
    ULONG                   RxCdtFrames;
    ULONG                   RxRuntErrors;

} OTTMP_ADAPTER_CONTEXT, *POTTMP_ADAPTER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(OTTMP_ADAPTER_CONTEXT, GetAdapterContext);

typedef struct _WDF_DEVICE_INFO
{
    POTTMP_ADAPTER_CONTEXT AdapterContext;

} WDF_DEVICE_INFO, *PWDF_DEVICE_INFO;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_DEVICE_INFO, GetWdfDeviceInfo);

#ifdef OTTMP_LEGACY

PAGED MINIPORT_PAUSE                    MPPause;
PAGED MINIPORT_RESTART                  MPRestart;

MINIPORT_SEND_NET_BUFFER_LISTS          MPSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS        MPReturnNetBufferLists;
MINIPORT_CANCEL_SEND                    MPCancelSend;

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS
AdapterInitialize(
    _In_ NDIS_HANDLE                MiniportAdapterHandle,
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
VOID
AdapterUninitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    );

#else

PAGED EVT_NET_ADAPTER_SET_CAPABILITIES  EvtAdapterSetCapabilities;
PAGED EVT_NET_ADAPTER_START             EvtAdapterStart;
PAGED EVT_NET_ADAPTER_PAUSE             EvtAdapterPause;

EVT_NET_ADAPTER_SEND_NET_BUFFER_LISTS   EvtAdapterSendNetBufferLists;

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
AdapterInitialize(
    _In_ NETADAPTER             Adapter,
    _In_ POTTMP_ADAPTER_CONTEXT AdapterContext,
    _In_ POTTMP_DEVICE_CONTEXT  DeviceContext
    );

#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
AdapterIndicateEnergyScanComplete(
    _In_ POTTMP_ADAPTER_CONTEXT AdapterContext,
    _In_ NDIS_STATUS            Status
    );

EXT_CALLBACK AdapterEnergyScanTimerCallback;
