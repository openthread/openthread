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

#include "pch.hpp"
#include "oid.tmh"

template<typename _Datum>
PAGED NDIS_STATUS RequestQuery( _In_ PNDIS_OID_REQUEST OidRequest, const _Datum & value );

#define VERIFY_NDIS_OBJECT_HEADER(_header, _type, _revision, _size) \
    (((_header).Type == _type) && \
     ((_header).Revision >= _revision) && \
     ((_header).Size >= _size))

#define VERIFY_NDIS_OBJECT_HEADER_PTR(_header, _type, _revision, _size) \
    (((_header)->Type == _type) && \
     ((_header)->Revision >= _revision) && \
     ((_header)->Size >= _size))

#define VERIFY_NDIS_REQUEST_OBJECT_HEADER(_buffer, _type, _revision, _size) \
    VERIFY_NDIS_OBJECT_HEADER_PTR(reinterpret_cast<PNDIS_OBJECT_HEADER>(_buffer), _type, _revision, _size)

#define ASSIGN_NDIS_OBJECT_HEADER(_header, _type, _revision, _size) \
    (_header).Type = _type; \
    (_header).Revision = _revision; \
    (_header).Size = _size; 

#define ASSIGN_NDIS_OBJECT_HEADER_PTR(_header, _type, _revision, _size) \
    (_header)->Type = _type; \
    (_header)->Revision = _revision; \
    (_header)->Size = _size; 

//
// List of Supported OIDs.
//
NDIS_OID NICSupportedOids[] =
{
    // General
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_INTERRUPT_MODERATION,
    OID_GEN_LINK_PARAMETERS,   // TODO: Mandatory Set
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_RCV_OK,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_STATISTICS,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_XMIT_OK,
    OID_GEN_LINK_PARAMETERS,    // TODO: Mandatory Set

    // 802.3 Specific
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

    // PnP Stuff
    OID_PNP_CAPABILITIES,                // Optional
    OID_PNP_QUERY_POWER,                 // Optional
    
    // OpenThread Stuff
    OID_OT_CAPABILITIES,
};

const ULONG SizeOfNICSupportedOids = sizeof(NICSupportedOids);

template<typename _Datum>
PAGED
NDIS_STATUS RequestQuery( _In_ PNDIS_OID_REQUEST OidRequest, const _Datum & value )
{
    PAGED_CODE();
    NT_ASSERT( (OidRequest->RequestType == NdisRequestQueryInformation) ||
        (OidRequest->RequestType == NdisRequestQueryStatistics) );

    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof( _Datum ))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( _Datum );
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( _Datum );
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof( _Datum );
        _Datum * pData = reinterpret_cast<_Datum *>(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer);
        *pData = value;
        return NDIS_STATUS_SUCCESS;
    }
}

PAGED
NDIS_STATUS RequestQuery32or64( _In_ PNDIS_OID_REQUEST OidRequest, const ULONG64 value )
{
    PAGED_CODE();
    NT_ASSERT( (OidRequest->RequestType == NdisRequestQueryInformation) ||
        (OidRequest->RequestType == NdisRequestQueryStatistics) );

    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength >= sizeof( ULONG64 ))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( ULONG64 );
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof( ULONG64 );
        PULONG64 pData = reinterpret_cast<PULONG64>(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer);
        *pData = value;
        return NDIS_STATUS_SUCCESS;
    }
    else if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength >= sizeof( ULONG ))
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( ULONG );
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof( ULONG );
        PULONG pData = reinterpret_cast<PULONG>(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer);
        *pData = (ULONG)value;
        return NDIS_STATUS_SUCCESS;
    }
    else
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( ULONG64 );
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_INVALID_LENGTH;
    }
}

PAGED
NDIS_STATUS RequestQueryThreadCapabilities(_In_ PNDIS_OID_REQUEST OidRequest)
{
    PAGED_CODE();

    OT_CAPABILITIES caps = { 0 };
    ASSIGN_NDIS_OBJECT_HEADER(caps.Header, NDIS_OBJECT_TYPE_DEFAULT, OT_CAPABILITIES_REVISION_1, SIZEOF_OT_CAPABILITIES_REVISION_1);
    caps.MiniportMode = OT_MP_MODE_THREAD;    // Thread Tunnel mode
    
    if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < SIZEOF_OT_CAPABILITIES_REVISION_1)
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = SIZEOF_OT_CAPABILITIES_REVISION_1;
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
        return NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = SIZEOF_OT_CAPABILITIES_REVISION_1;
        OidRequest->DATA.QUERY_INFORMATION.BytesWritten = SIZEOF_OT_CAPABILITIES_REVISION_1;
        memcpy(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer, &caps, SIZEOF_OT_CAPABILITIES_REVISION_1);
        return NDIS_STATUS_SUCCESS;
    }
}

PAGED
NDIS_STATUS RequestQueryGenStatistics(_In_ PNDIS_OID_REQUEST OidRequest, _In_ POTTMP_ADAPTER_CONTEXT AdapterContext)
{
    PAGED_CODE();

    NDIS_STATISTICS_INFO statistics = { 0 };
    ASSIGN_NDIS_OBJECT_HEADER(statistics.Header, NDIS_OBJECT_TYPE_DEFAULT, NDIS_SIZEOF_STATISTICS_INFO_REVISION_1, NDIS_STATISTICS_INFO_REVISION_1);

    statistics.SupportedStatistics = NIC_SUPPORTED_STATISTICS;

    /* Bytes in */
    statistics.ifHCInOctets = AdapterContext->BytesRxDirected +
                               AdapterContext->BytesRxMulticast +
                               AdapterContext->BytesRxBroadcast;

    statistics.ifHCInUcastOctets = AdapterContext->BytesRxDirected;            
    statistics.ifHCInMulticastOctets = AdapterContext->BytesRxMulticast;            
    statistics.ifHCInBroadcastOctets = AdapterContext->BytesRxBroadcast;

    /* Packets in */
    statistics.ifHCInUcastPkts = AdapterContext->FramesRxDirected;            
    statistics.ifHCInMulticastPkts = AdapterContext->FramesRxMulticast;            
    statistics.ifHCInBroadcastPkts = AdapterContext->FramesRxBroadcast;

    /* Errors in */
    statistics.ifInErrors = AdapterContext->RxCrcErrors +
                             AdapterContext->RxAlignmentErrors +
                             AdapterContext->RxDmaOverrunErrors +
                             AdapterContext->RxRuntErrors;

    statistics.ifInDiscards = AdapterContext->RxResourceErrors;            

    /* Bytes out */
    statistics.ifHCOutOctets = AdapterContext->BytesTxDirected +
                                AdapterContext->BytesTxMulticast +
                                AdapterContext->BytesTxBroadcast;

    statistics.ifHCOutUcastOctets = AdapterContext->BytesTxDirected;            
    statistics.ifHCOutMulticastOctets = AdapterContext->BytesTxMulticast;            
    statistics.ifHCOutBroadcastOctets = AdapterContext->BytesTxBroadcast;

    /* Packets out */
    statistics.ifHCOutUcastPkts = AdapterContext->FramesTxDirected;            
    statistics.ifHCOutMulticastPkts = AdapterContext->FramesTxMulticast;            
    statistics.ifHCOutBroadcastPkts = AdapterContext->FramesTxBroadcast;

    /* Errors out */
    statistics.ifOutErrors = AdapterContext->TxAbortExcessCollisions +
                              AdapterContext->TxDmaUnderrun +
                              AdapterContext->TxLostCRS +
                              AdapterContext->TxLateCollisions+
                              AdapterContext->TransmitFailuresOther;

    statistics.ifOutDiscards = 0ULL;
    
    return RequestQuery(OidRequest, statistics);
}

PAGED 
_Use_decl_annotations_
NDIS_STATUS
MPOidRequest(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNDIS_OID_REQUEST       OidRequest
    )
{
    POTTMP_ADAPTER_CONTEXT AdapterContext = (POTTMP_ADAPTER_CONTEXT)MiniportAdapterContext;
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    bool fFailExpected = false;

    LogFuncEntry(DRIVER_DEFAULT);

    PAGED_CODE();

    switch (OidRequest->RequestType)
    {
    case NdisRequestSetInformation:
        switch (OidRequest->DATA.QUERY_INFORMATION.Oid)
        {
        case OID_802_3_MULTICAST_LIST:
            // TODO: Check with Jeffrey how to better handle this
            status = NDIS_STATUS_MULTICAST_FULL;
            fFailExpected = true;
            break;

            //
            // Fake it until we make it :)
            // We can't bind unless we report success for these OIDs
            //
        case OID_GEN_CURRENT_PACKET_FILTER:
        case OID_PM_ADD_WOL_PATTERN:
        case OID_PM_REMOVE_WOL_PATTERN:
        case OID_GEN_CURRENT_LOOKAHEAD:
            // TODO: Implement this
            status = NDIS_STATUS_SUCCESS;
            break;

            // Explicitly not supported
        case OID_GEN_INTERRUPT_MODERATION:
            status = NDIS_STATUS_NOT_SUPPORTED;
            fFailExpected = true;
            break;

            // Unknown
        default:
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
        break;
    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:
        switch (OidRequest->DATA.QUERY_INFORMATION.Oid)
        {
        case OID_GEN_INTERRUPT_MODERATION:
            {
                static const NDIS_INTERRUPT_MODERATION_PARAMETERS nimp = {
                    { NDIS_OBJECT_TYPE_DEFAULT, NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1, NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1 },
                    0,
                    NdisInterruptModerationNotSupported };
                status = RequestQuery( OidRequest, nimp );
                break;
            }

        case OID_GEN_RCV_OK:
            status = RequestQuery32or64( OidRequest, AdapterContext->FramesRxBroadcast
                     + AdapterContext->FramesRxMulticast
                     + AdapterContext->FramesRxDirected );
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            status = RequestQuery<ULONG>( OidRequest, HW_MAX_FRAME_SIZE );
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            status = RequestQuery<ULONG>( OidRequest, HW_MAX_FRAME_SIZE * AdapterContext->ulMaxBusyRecvs );
            break;

        case OID_GEN_STATISTICS:
            status = RequestQueryGenStatistics(OidRequest, AdapterContext);
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            status = RequestQuery<ULONG>( OidRequest, HW_MAX_FRAME_SIZE * AdapterContext->ulMaxBusySends );
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
        case OID_GEN_VENDOR_DRIVER_VERSION:
        case OID_GEN_VENDOR_ID:
            if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof(NIC_VENDOR_DESC))
            {
                OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( NIC_VENDOR_DESC );
                OidRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
                status = NDIS_STATUS_INVALID_LENGTH;
            }
            else
            {
                OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof( NIC_VENDOR_DESC );
                OidRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof( NIC_VENDOR_DESC );
                memcpy(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer, NIC_VENDOR_DESC, sizeof(NIC_VENDOR_DESC));
                status = NDIS_STATUS_SUCCESS;
            }
            break;

        case OID_GEN_XMIT_OK:
            status = RequestQuery32or64( OidRequest, AdapterContext->FramesTxBroadcast
                     + AdapterContext->FramesTxMulticast
                     + AdapterContext->FramesTxDirected );
            break;

        case OID_802_3_CURRENT_ADDRESS:
            status = RequestQuery( OidRequest, AdapterContext->ExtendedAddress );
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            status = RequestQuery( OidRequest, AdapterContext->ExtendedAddress );
            break;

        case OID_PNP_CAPABILITIES:
            // We do not support low power
            status = NDIS_STATUS_NOT_SUPPORTED;
            fFailExpected = true;
            break;

        case OID_PNP_QUERY_POWER:
            status = NDIS_STATUS_NOT_ACCEPTED;
            fFailExpected = true;
            break;

        case OID_OT_CAPABILITIES:
            status = RequestQueryThreadCapabilities( OidRequest );
            break;

        default:
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
        break;
    case NdisRequestMethod:
        break;
    default:
        status = NDIS_STATUS_INVALID_OID;
        break;

    }
    UNREFERENCED_PARAMETER( OidRequest );

    if (!fFailExpected && (status != NDIS_STATUS_SUCCESS))
    {
        LogFuncExitMsg(DRIVER_DEFAULT, " Type:%u Oid:%u Status:%!NDIS_STATUS!", OidRequest->RequestType, OidRequest->DATA.QUERY_INFORMATION.Oid, status);
    }
    else
    {
        // By not using LogFuncExitNDIS, this won't get auto-promoted to a warning on non-0 statuses
        LogFuncExitMsg(DRIVER_DEFAULT, " Type:%u Oid:%u Status:%!NDIS_STATUS!", OidRequest->RequestType, OidRequest->DATA.QUERY_INFORMATION.Oid, status);
    }
    return status;
}

_Use_decl_annotations_
VOID
MPCancelOidRequest(
    _In_  NDIS_HANDLE             /* MiniportAdapterContext */,
    _In_  PVOID                   /* pRequestId */
)
{
    LogFuncEntry(DRIVER_DEFAULT);

    LogFuncExit(DRIVER_DEFAULT);
}
