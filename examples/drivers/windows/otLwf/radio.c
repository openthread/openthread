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
 *  This file implements the logging function required for the OpenThread library.
 */

#include "precomp.h"
#include "radio.tmh"

VOID 
otLwfRadioSendPacket(
    _In_ PMS_FILTER     pFilter,
    _In_ RadioPacket*   Packet
    );

VOID 
otLwfRadioInit(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    pFilter->otPhyState = kStateDisabled;
    pFilter->otCurrentListenChannel = 0xFF;
    pFilter->otPromiscuous = false;

    pFilter->otReceiveFrame.mPsdu = pFilter->otReceiveMessage;
    pFilter->otTransmitFrame.mPsdu = pFilter->otTransmitMessage;
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateDisabled.", pFilter);
    
    LogFuncExit(DRIVER_DEFAULT);
}

ThreadError otPlatRadioSetPanId(_In_ otContext *otCtx, uint16_t panid)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NDIS_STATUS status;
    ULONG bytesProcessed;
    OT_PAND_ID OidBuffer = { {NDIS_OBJECT_TYPE_DEFAULT, OT_PAND_ID_REVISION_1, SIZEOF_OT_PAND_ID_REVISION_1}, panid };
    
    NT_ASSERT(pFilter->otPhyState != kStateTransmit);
    if (pFilter->otPhyState == kStateTransmit) return kThreadError_Busy;

    LogInfo(DRIVER_DEFAULT, "Filter %p set PanID: %X", pFilter, panid);

    pFilter->otPanID = panid;

    // Indicate to the miniport
    status = 
        otLwfSendInternalRequest(
            pFilter,
            NdisRequestSetInformation,
            OID_OT_PAND_ID,
            &OidBuffer,
            sizeof(OidBuffer),
            &bytesProcessed
            );
    if (status != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "Set for OID_OT_PAND_ID failed, %!NDIS_STATUS!", status);
    }

    return kThreadError_None;
}

ThreadError otPlatRadioSetExtendedAddress(_In_ otContext *otCtx, uint8_t *address)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NDIS_STATUS status;
    ULONG bytesProcessed;
    OT_EXTENDED_ADDRESS OidBuffer = { {NDIS_OBJECT_TYPE_DEFAULT, OT_EXTENDED_ADDRESS_REVISION_1, SIZEOF_OT_EXTENDED_ADDRESS_REVISION_1} };
    
    NT_ASSERT(pFilter->otPhyState != kStateTransmit);
    if (pFilter->otPhyState == kStateTransmit) return kThreadError_Busy;

    LogInfo(DRIVER_DEFAULT, "Filter %p set Ext Addr: %llX", pFilter, *(ULONGLONG*)address);

    for (size_t i = 0; i < sizeof(pFilter->otExtendedAddress); i++)
    {
        pFilter->otExtendedAddress[i] = address[sizeof(pFilter->otExtendedAddress) - 1 - i];
        OidBuffer.ExtendedAddress[i] = address[sizeof(pFilter->otExtendedAddress) - 1 - i];
    }

    // Indicate to the miniport
    status = 
        otLwfSendInternalRequest(
            pFilter,
            NdisRequestSetInformation,
            OID_OT_EXTENDED_ADDRESS,
            &OidBuffer,
            sizeof(OidBuffer),
            &bytesProcessed
            );
    if (status != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "Set for OID_OT_EXTENDED_ADDRESS failed, %!NDIS_STATUS!", status);
    }

    return kThreadError_None;
}

ThreadError otPlatRadioSetShortAddress(_In_ otContext *otCtx, uint16_t address)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NDIS_STATUS status;
    ULONG bytesProcessed;
    OT_SHORT_ADDRESS OidBuffer = { {NDIS_OBJECT_TYPE_DEFAULT, OT_SHORT_ADDRESS_REVISION_1, SIZEOF_OT_SHORT_ADDRESS_REVISION_1}, address };
    
    NT_ASSERT(pFilter->otPhyState != kStateTransmit);
    if (pFilter->otPhyState == kStateTransmit) return kThreadError_Busy;

    LogInfo(DRIVER_DEFAULT, "Filter %p set Short Addr: %X", pFilter, address);

    pFilter->otShortAddress = address;

    // Indicate to the miniport
    status = 
        otLwfSendInternalRequest(
            pFilter,
            NdisRequestSetInformation,
            OID_OT_SHORT_ADDRESS,
            &OidBuffer,
            sizeof(OidBuffer),
            &bytesProcessed
            );
    if (status != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "Set for OID_OT_SHORT_ADDRESS failed, %!NDIS_STATUS!", status);
    }

    return kThreadError_None;
}

void otPlatRadioSetPromiscuous(_In_ otContext *otCtx, int aEnable)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NDIS_STATUS status;
    ULONG bytesProcessed;
    OT_PROMISCUOUS_MODE OidBuffer = { {NDIS_OBJECT_TYPE_DEFAULT, OT_PROMISCUOUS_MODE_REVISION_1, SIZEOF_OT_PROMISCUOUS_MODE_REVISION_1}, (BOOLEAN)aEnable };

    pFilter->otPromiscuous = (BOOLEAN)aEnable;

    // Indicate to the miniport
    status = 
        otLwfSendInternalRequest(
            pFilter,
            NdisRequestSetInformation,
            OID_OT_PROMISCUOUS_MODE,
            &OidBuffer,
            sizeof(OidBuffer),
            &bytesProcessed
            );
    if (status != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "Set for OID_OT_PROMISCUOUS_MODE failed, %!NDIS_STATUS!", status);
    }
}

ThreadError otPlatRadioEnable(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    if (pFilter->otPhyState > kStateSleep) return kThreadError_Busy;

    pFilter->otPhyState = kStateSleep;
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateSleep.", pFilter);

    return kThreadError_None;
}

ThreadError otPlatRadioDisable(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    if (pFilter->otPhyState > kStateSleep) return kThreadError_Busy;

    pFilter->otPhyState = kStateDisabled;
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateDisabled.", pFilter);

    return kThreadError_None;
}

ThreadError otPlatRadioSleep(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    if (pFilter->otPhyState != kStateSleep && pFilter->otPhyState != kStateReceive) 
        return kThreadError_Busy;

    pFilter->otPhyState = kStateSleep;
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateSleep.", pFilter);

    return kThreadError_None;
}

ThreadError otPlatRadioReceive(_In_ otContext *otCtx, uint8_t aChannel)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    if (pFilter->otPhyState == kStateDisabled) return kThreadError_Busy;
    
    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    pFilter->otPhyState = kStateReceive;

    // Update current channel if different
    if (pFilter->otCurrentListenChannel != aChannel)
    {
        NDIS_STATUS status;
        ULONG bytesProcessed;
        OT_CURRENT_CHANNEL OidBuffer = { {NDIS_OBJECT_TYPE_DEFAULT, OT_CURRENT_CHANNEL_REVISION_1, SIZEOF_OT_CURRENT_CHANNEL_REVISION_1}, aChannel };

        LogInfo(DRIVER_DEFAULT, "Filter %p new Listen Channel = %u.", pFilter, aChannel);
        pFilter->otCurrentListenChannel = aChannel;
        
        // Indicate to the miniport
        status = 
            otLwfSendInternalRequest(
                pFilter,
                NdisRequestSetInformation,
                OID_OT_CURRENT_CHANNEL,
                &OidBuffer,
                sizeof(OidBuffer),
                &bytesProcessed
                );
        if (status != NDIS_STATUS_SUCCESS)
        {
            LogError(DRIVER_DEFAULT, "Set for OID_OT_CURRENT_CHANNEL failed, %!NDIS_STATUS!", status);
        }
    }
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateReceive.", pFilter);
    
    // Set the event to indicate we can process NBLs
    KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);
    
    LogFuncExit(DRIVER_DATA_PATH);

    return kThreadError_None;
}

RadioPacket *otPlatRadioGetTransmitBuffer(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return &pFilter->otTransmitFrame;
}

int8_t otPlatRadioGetNoiseFloor(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    return 0;
}

otRadioCaps otPlatRadioGetCaps(_In_ otContext *otCtx)
{
    return 
        ((otCtxToFilter(otCtx)->MiniportCapabilities.RadioCapabilities & OT_RADIO_CAP_ACK_TIMEOUT) != 0) ? 
            kRadioCapsAckTimeout : 
            kRadioCapsNone;
}

int otPlatRadioGetPromiscuous(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return pFilter->otPromiscuous;
}

VOID 
otLwfRadioReceiveFrame(
    _In_ PMS_FILTER pFilter
    )
{    
    NT_ASSERT(pFilter->otReceiveFrame.mChannel >= 11 && pFilter->otReceiveFrame.mChannel <= 26);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    if (pFilter->otPhyState == kStateReceive)
    {
        LogVerbose(DRIVER_DATA_PATH, "---> otPlatRadioReceiveDone");
        otPlatRadioReceiveDone(pFilter->otCtx, &pFilter->otReceiveFrame, kThreadError_None);
        LogVerbose(DRIVER_DATA_PATH, "<--- otPlatRadioReceiveDone");
    }
    else
    {
        LogVerbose(DRIVER_DATA_PATH, "MAC Frame Dropped: Wrong State, %u", (ULONG)pFilter->otPhyState);
    }
    
    LogFuncExit(DRIVER_DATA_PATH);
}

ThreadError otPlatRadioTransmit(_In_ otContext *otCtx)
{
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    ThreadError error = kThreadError_Busy;
    
    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    if (pFilter->otPhyState >= kStateReceive)
    {
        error = kThreadError_None;
        pFilter->otPhyState = kStateTransmit;
    
        LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateTransmit.", pFilter);
    }

    LogFuncExitMsg(DRIVER_DATA_PATH, "%u", error);

    return error;
}

VOID otLwfRadioTransmitFrame(_In_ PMS_FILTER pFilter)
{
    NT_ASSERT(pFilter->otPhyState == kStateTransmit);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    otLwfRadioSendPacket(pFilter, &pFilter->otTransmitFrame);

    LogFuncExit(DRIVER_DATA_PATH);
}

VOID 
otLwfRadioSendPacket(
    _In_ PMS_FILTER     pFilter,
    _In_ RadioPacket*   Packet
    )
{
    SIZE_T BytesCopied = 0;
    PNET_BUFFER SendNetBuffer = NET_BUFFER_LIST_FIRST_NB(pFilter->SendNetBufferList);
    OT_NBL_CONTEXT NblContext = { 0 /* Flags */, Packet->mChannel, Packet->mPower, 0 /* Lqi */ };

    NT_ASSERT(NblContext.Channel >= 11 && NblContext.Channel <= 26);
    NT_ASSERT(!pFilter->SendPending);

    NT_ASSERT(Packet->mLength <= kMaxPHYPacketSize);

    // Copy to the NetBufferList
    RtlCopyBufferToMdl(
        Packet->mPsdu, 
        SendNetBuffer->CurrentMdl, 
        SendNetBuffer->CurrentMdlOffset, 
        Packet->mLength, 
        &BytesCopied
        );
    NT_ASSERT(BytesCopied == Packet->mLength);

    // Set the length field
    NET_BUFFER_DATA_LENGTH(SendNetBuffer) = Packet->mLength;

    // Set the context
    SetNBLContext(pFilter->SendNetBufferList, &NblContext);

    // Reset the completion event
    KeResetEvent(&pFilter->SendNetBufferListComplete);
    pFilter->SendPending = TRUE;

    // Send the NetBufferList
    NdisFSendNetBufferLists(
        pFilter->FilterHandle,
        pFilter->SendNetBufferList,
        NDIS_DEFAULT_PORT_NUMBER,
        0
        );
}

VOID 
otLwfRadioTransmitFrameDone(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    pFilter->SendPending = FALSE;

    if (STATUS_SUCCESS == pFilter->SendNetBufferList->Status)
    {
        POT_NBL_CONTEXT SendNblContext = GetNBLContext(pFilter->SendNetBufferList);
        BOOLEAN FramePending = (SendNblContext->Flags & OT_NBL_FLAG_ACK_FRAME_PENDING) != 0 || pFilter->CountPendingRecvNBLs != 0;

        otPlatRadioTransmitDone(pFilter->otCtx, FramePending, kThreadError_None);
    }
    else if (STATUS_TIMEOUT == pFilter->SendNetBufferList->Status)
    {
        otPlatRadioTransmitDone(pFilter->otCtx, false, kThreadError_NoAck);
    }
    else
    {
        otPlatRadioTransmitDone(pFilter->otCtx, false, kThreadError_Abort);
    }

    LogFuncExit(DRIVER_DATA_PATH);
}
