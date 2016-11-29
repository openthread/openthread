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
 *   This module implements code to manage the NETADAPTER object for the
 *   network adapter.
 */

#include "pch.hpp"
#include "adapter.tmh"

PAGED
_IRQL_requires_max_(PASSIVE_LEVEL)
NDIS_STATUS
AdapterInitialize(
    _In_ NDIS_HANDLE                MiniportAdapterHandle,
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
/*++ 
Routine Description:

    AdapterInitialize function is called to initialize the Network Adapter
    at the time of Pnp Add device. 
 
    This routine initializes the context of the adapter object
 
Arguments:
 
    MiniportAdapterHandle - Handle to the NDIS Miniport Adapter object.
 
    AdapterContext - The context associated with the adapter

Return Value:

    NTSTATUS - A failure here will indicate a fatal error in the driver.

--*/
{
    NDIS_STATUS Status;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();
    
    do
    {
        NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES AdapterRegistration = { 0 };
        NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES AdapterGeneral = { 0 };
        NDIS_PM_CAPABILITIES PmCapabilities = { 0 };
        
        //
        // First, set the registration attributes.
        //
        AdapterRegistration.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        AdapterRegistration.Header.Size = sizeof(AdapterRegistration);
        AdapterRegistration.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_2;

        AdapterRegistration.MiniportAdapterContext = AdapterContext;
        AdapterRegistration.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_SURPRISE_REMOVE_OK | NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM | NDIS_MINIPORT_ATTRIBUTES_NO_PAUSE_ON_SUSPEND;
        AdapterRegistration.InterfaceType = NdisInterfacePNPBus;

        NDIS_DECLARE_MINIPORT_ADAPTER_CONTEXT(_OTTMP_ADAPTER_CONTEXT);
        Status = NdisMSetMiniportAttributes(
            MiniportAdapterHandle,
            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterRegistration);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            LogError(DRIVER_DEFAULT, "[%p] NdisSetOptionalHandlers Status %!NDIS_STATUS!", AdapterContext, Status);
            break;
        }

        //
        // Next, set the general attributes.
        //

        AdapterGeneral.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        AdapterGeneral.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        AdapterGeneral.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;

        //
        // Specify the medium type that the NIC can support but not
        // necessarily the medium type that the NIC currently uses.
        //
        AdapterGeneral.MediaType = NIC_MEDIUM_TYPE;

        //
        // Specifiy medium type that the NIC currently uses.
        //
        AdapterGeneral.PhysicalMediumType = NdisPhysicalMediumNative802_15_4;

        //
        // We have to lie about the MTU, so that TCPIP will bind to us.
        // Specifically, we have to rely that the ThreadLwf will fragment
        // the packets appropriately.
        //
        AdapterGeneral.MtuSize = HW_MAX_FRAME_SIZE;
        AdapterGeneral.MaxXmitLinkSpeed = NIC_RECV_XMIT_SPEED;
        AdapterGeneral.XmitLinkSpeed = NIC_RECV_XMIT_SPEED;
        AdapterGeneral.MaxRcvLinkSpeed = NIC_RECV_XMIT_SPEED;
        AdapterGeneral.RcvLinkSpeed = NIC_RECV_XMIT_SPEED;
        AdapterGeneral.MediaConnectState = MediaConnectStateConnected;
        AdapterGeneral.MediaDuplexState = MediaDuplexStateFull;

        //
        // The maximum number of bytes the NIC can provide as lookahead data.
        // If that value is different from the size of the lookahead buffer
        // supported by bound protocols, NDIS will call MiniportOidRequest to
        // set the size of the lookahead buffer provided by the miniport driver
        // to the minimum of the miniport driver and protocol(s) values. If the
        // driver always indicates up full packets with
        // NdisMIndicateReceiveNetBufferLists, it should set this value to the
        // maximum total frame size, which excludes the header.
        //
        // Upper-layer drivers examine lookahead data to determine whether a
        // packet that is associated with the lookahead data is intended for
        // one or more of their clients. If the underlying driver supports
        // multipacket receive indications, bound protocols are given full net
        // packets on every indication. Consequently, this value is identical
        // to that returned for OID_GEN_RECEIVE_BLOCK_SIZE.
        //
        AdapterGeneral.LookaheadSize = HW_MAX_FRAME_SIZE;
        AdapterGeneral.PowerManagementCapabilities = NULL;
        AdapterGeneral.MacOptions = NIC_MAC_OPTIONS;
        AdapterGeneral.SupportedPacketFilters = NIC_SUPPORTED_FILTERS;

        //
        // The maximum number of multicast addresses the NIC driver can manage.
        // This list is global for all protocols bound to (or above) the NIC.
        // Consequently, a protocol can receive NDIS_STATUS_MULTICAST_FULL from
        // the NIC driver when attempting to set the multicast address list,
        // even if the number of elements in the given list is less than the
        // number originally returned for this query.
        //
        AdapterGeneral.MaxMulticastListSize = NIC_MAX_MCAST_LIST;
        AdapterGeneral.MacAddressLength = NIC_MACADDR_SIZE;

        //
        // Return the MAC address of the NIC burnt in the hardware.
        //
        memcpy(AdapterGeneral.PermanentMacAddress, &AdapterContext->ExtendedAddress, sizeof(AdapterContext->ExtendedAddress));
        memcpy(AdapterGeneral.CurrentMacAddress, &AdapterContext->ExtendedAddress, sizeof(AdapterContext->ExtendedAddress));

        AdapterGeneral.RecvScaleCapabilities = NULL;
        AdapterGeneral.AccessType = NET_IF_ACCESS_BROADCAST;
        AdapterGeneral.DirectionType = NET_IF_DIRECTION_SENDRECEIVE;
        AdapterGeneral.ConnectionType = NET_IF_CONNECTION_DEDICATED;
        AdapterGeneral.IfType = IF_TYPE_IEEE802154;
        AdapterGeneral.IfConnectorPresent = TRUE;
        AdapterGeneral.SupportedStatistics = NIC_SUPPORTED_STATISTICS;
        AdapterGeneral.SupportedPauseFunctions = NdisPauseFunctionsUnsupported;
        AdapterGeneral.DataBackFillSize = 0;
        AdapterGeneral.ContextBackFillSize = 0;

        //
        // The SupportedOidList is an array of OIDs for objects that the
        // underlying driver or its NIC supports.  Objects include general,
        // media-specific, and implementation-specific objects. NDIS forwards a
        // subset of the returned list to protocols that make this query. That
        // is, NDIS filters any supported statistics OIDs out of the list
        // because protocols never make statistics queries.
        //
        AdapterGeneral.SupportedOidList = NICSupportedOids;
        AdapterGeneral.SupportedOidListLength = SizeOfNICSupportedOids;
        AdapterGeneral.AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;

        //
        // Set the power management capabilities. All 0 basically means we don't
        // support Dx for anything.
        //

        PmCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PmCapabilities.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
        PmCapabilities.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;

        AdapterGeneral.PowerManagementCapabilitiesEx = &PmCapabilities;

        Status = NdisMSetMiniportAttributes(
            MiniportAdapterHandle,
            (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterGeneral);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            LogError(DRIVER_DEFAULT, "[%p] NdisSetOptionalHandlers failed %!NDIS_STATUS!", AdapterContext, Status);
            break;
        }

    } while (FALSE);

    LogFuncExitNDIS(DRIVER_DEFAULT, Status);

    return Status;
}

PAGED
_IRQL_requires_(PASSIVE_LEVEL)
VOID
AdapterUninitialize(
    _In_ POTTMP_ADAPTER_CONTEXT     AdapterContext
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    SerialUninitialize(AdapterContext);

    if (AdapterContext->Device)
    {
        WdfObjectDelete(AdapterContext->Device);
        AdapterContext->Device = nullptr;
    }

    NdisFreeMemory(AdapterContext, 0, 0);

    LogFuncExit(DRIVER_DEFAULT);
}

PAGED
_IRQL_requires_( PASSIVE_LEVEL )
_Function_class_( MINIPORT_RESTART )
NDIS_STATUS
MPRestart(
    _In_  NDIS_HANDLE                             MiniportAdapterContext,
    _In_  PNDIS_MINIPORT_RESTART_PARAMETERS       /* RestartParameters */
    )
/*++

Routine Description:

    When a miniport receives a restart request, it enters into a Restarting
    state.  The miniport may begin indicating received data (e.g., using
    NdisMIndicateReceiveNetBufferLists), handling status indications, and
    processing OID requests in the Restarting state.  However, no sends will be
    requested while the miniport is in the Restarting state.

    Once the miniport is ready to send data, it has entered the Running state.
    The miniport informs NDIS that it is in the Running state by returning
    NDIS_STATUS_SUCCESS from this MiniportRestart function; or if this function
    has already returned NDIS_STATUS_PENDING, by calling NdisMRestartComplete.


    MiniportRestart runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    RestartParameters  Additional information about the restart operation

Return Value:

    If the miniport is able to immediately enter the Running state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport is still in the Restarting state, it should return
    NDIS_STATUS_PENDING now, and call NdisMRestartComplete when the miniport
    has entered the Running state.

    Other NDIS_STATUS codes indicate errors.  If an error is encountered, the
    miniport must return to the Paused state (i.e., stop indicating receives).

--*/

{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)MiniportAdapterContext;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    // Set the running flag
    AdapterContext->IsRunning = true;

    LogFuncExitNDIS(DRIVER_DEFAULT, status);

    return status;
}

PAGED
_IRQL_requires_( PASSIVE_LEVEL )
_Function_class_( MINIPORT_PAUSE )
NDIS_STATUS
MPPause(
    _In_  NDIS_HANDLE                       MiniportAdapterContext,
    _In_  PNDIS_MINIPORT_PAUSE_PARAMETERS   /* MiniportPauseParameters */
    )
/*++

Routine Description:

    When a miniport receives a pause request, it enters into a Pausing state.
    The miniport should not indicate up any more network data.  Any pending
    send requests must be completed, and new requests must be rejected with
    NDIS_STATUS_PAUSED.

    Once all sends have been completed and all recieve NBLs have returned to
    the miniport, the miniport enters the Paused state.

    While paused, the miniport can still service interrupts from the hardware
    (to, for example, continue to indicate NDIS_STATUS_MEDIA_CONNECT
    notifications).

    The miniport must continue to be able to handle status indications and OID
    requests.  MiniportPause is different from MiniportHalt because, in
    general, the MiniportPause operation won't release any resources.
    MiniportPause must not attempt to acquire any resources where allocation
    can fail, since MiniportPause itself must not fail.


    MiniportPause runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    MiniportPauseParameters  Additional information about the pause operation

Return Value:

    If the miniport is able to immediately enter the Paused state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport must wait for send completions or pending receive NBLs, it
    should return NDIS_STATUS_PENDING now, and call NDISMPauseComplete when the
    miniport has entered the Paused state.

    No other return value is permitted.  The pause operation must not fail.

--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)MiniportAdapterContext;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();
    
    // Clear the flag to indicate we are no longer running
    AdapterContext->IsRunning = false;

    LogFuncExitNDIS(DRIVER_DEFAULT, status);

    return status;
}

VOID
MPSendNetBufferLists(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST        NetBufferLists,
    _In_  NDIS_PORT_NUMBER        /* PortNumber */,
    _In_  ULONG                   SendFlags
    )
/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our miniport sends one or more packets.

    The input packet descriptor pointers have been ordered according
    to the order in which the packets should be sent over the network
    by the protocol driver that set up the packet array. The NDIS
    library preserves the protocol-determined ordering when it submits
    each packet array to MiniportSendPackets

    As a deserialized driver, we are responsible for holding incoming send
    packets in our internal queue until they can be transmitted over the
    network and for preserving the protocol-determined ordering of packet
    descriptors incoming to its MiniportSendPackets function.
    A deserialized miniport driver must complete each incoming send packet
    with NdisMSendComplete, and it cannot call NdisMSendResourcesAvailable.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              Head of a list of NBLs to send
    PortNumber                  A miniport adapter port.  Default is 0.
    SendFlags                   Additional flags for the send operation

Return Value:

    None.  Write status directly into each NBL with the NET_BUFFER_LIST_STATUS
    macro.

--*/
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)MiniportAdapterContext;
    PNET_BUFFER_LIST FailedNbls = NULL;

    LogFuncEntryMsg(DRIVER_DEFAULT, "NetBufferList: %p", NetBufferLists);

    PNET_BUFFER_LIST CurrNbl = NetBufferLists;
    while (CurrNbl)
    {
        PNET_BUFFER_LIST NextNbl = CurrNbl->Next;
        CurrNbl->Next = NULL;

        // Only allow one NB per NBL
        if (CurrNbl->FirstNetBuffer == NULL ||
            CurrNbl->FirstNetBuffer->Next != NULL)
        {
            CurrNbl->Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            // Try to queue up for send
            NTSTATUS status = SerialSendData(AdapterContext, CurrNbl);

            if (!NT_SUCCESS(status)) {
                CurrNbl->Status = status;
            }
            else {
                CurrNbl = NULL;
            }
        }

        // If we still have the CurrNbl, it failed, so move it to the failure list
        if (CurrNbl) {
            CurrNbl->Next = FailedNbls;
            FailedNbls = CurrNbl;
        }

        CurrNbl = NextNbl;
    }
    
    // Complete any failures
    if (FailedNbls) {
        NdisMSendNetBufferListsComplete(AdapterContext->Adapter, FailedNbls, (SendFlags & NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL));
    }

    LogFuncExit(DRIVER_DEFAULT);
}

VOID
MPCancelSend(
    _In_  NDIS_HANDLE     /* MiniportAdapterContext */,
    _In_  PVOID           /* CancelId */
    )
/*++

Routine Description:

    MiniportCancelSend cancels the transmission of all NET_BUFFER_LISTs that
    are marked with a specified cancellation identifier. Miniport drivers
    that queue send packets for more than one second should export this
    handler. When a protocol driver or intermediate driver calls the
    NdisCancelSendNetBufferLists function, NDIS calls the MiniportCancelSend
    function of the appropriate lower-level driver (miniport driver or
    intermediate driver) on the binding.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the packets with this Id should be cancelled

Return Value:

    None.

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    LogFuncExit(DRIVER_DEFAULT);
}

VOID
MPReturnNetBufferLists(
    _In_  NDIS_HANDLE       /* MiniportAdapterContext */,
    _In_  PNET_BUFFER_LIST  NetBufferLists,
    _In_  ULONG             /* ReturnFlags */
    )
/*++

Routine Description:

    NDIS Miniport entry point called whenever protocols are done with one or
    NBLs that we indicated up with NdisMIndicateReceiveNetBufferLists.

    Note that the list of NBLs may be chained together from multiple separate
    lists that were indicated up individually.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              NBLs being returned
    ReturnFlags                 May contain the NDIS_RETURN_FLAGS_DISPATCH_LEVEL
                                flag, which if is set, indicates we can get a
                                small perf win by not checking or raising the
                                IRQL

Return Value:

    None.

--*/
{
    LogFuncEntry(DRIVER_DEFAULT);

    // Iterate through all the NetBufferLists
    for (PNET_BUFFER_LIST pNblNext, pNbl = NetBufferLists; pNbl; pNbl = pNblNext)
    {
        // Save next to temporary and clear member variable
        pNblNext = NET_BUFFER_LIST_NEXT_NBL(pNbl);
        NET_BUFFER_LIST_NEXT_NBL(pNbl) = NULL;

        // Iterate through all the Netbuffers
        for (PNET_BUFFER pNbNext, pNb = NET_BUFFER_LIST_FIRST_NB(pNbl); pNb; pNb = pNbNext)
        {
            pNbNext = NET_BUFFER_NEXT_NB(pNb);
            NdisFreeNetBuffer(pNb);
        }

        NdisFreeNetBufferList(pNbl);
    }

    LogFuncExit(DRIVER_DEFAULT);
}
