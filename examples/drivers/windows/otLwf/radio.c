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
 *  This file implements the logging function required for the OpenThread library.
 */

#include "precomp.h"
#include "radio.tmh"

void 
LogMac(
    _In_ PCSTR szDir,
    _In_ PMS_FILTER pFilter,
    ULONG frameLength,
    _In_reads_bytes_(frameLength) PUCHAR frame
    );

const char MacSend[] = "MAC_SEND";
const char MacRecv[] = "MAC_RECV";

#define LogMacSend(pFilter, frameLength, frame) LogMac(MacSend, pFilter, frameLength, frame)
#define LogMacRecv(pFilter, frameLength, frame) LogMac(MacRecv, pFilter, frameLength, frame)

void 
otPlatReset(
    _In_ otInstance *otCtx
    )
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! resetting...", &pFilter->InterfaceGuid);

    // Indicate to the miniport
    (void)otLwfCmdResetDevice(pFilter, TRUE);

    // Finalize previous OpenThread instance
    otInstanceFinalize(pFilter->otCtx);
    pFilter->otCtx = NULL;

    // Reinitialize the OpenThread library
    pFilter->otCachedRole = kDeviceRoleDisabled;
    pFilter->otCtx = otInstanceInit(pFilter->otInstanceBuffer + sizeof(PMS_FILTER), &pFilter->otInstanceSize);
    ASSERT(pFilter->otCtx);

    // Make sure our helper function returns the right pointer for the filter, given the openthread instance
    NT_ASSERT(otCtxToFilter(pFilter->otCtx) == pFilter);

    // Disable Icmp (ping) handling
    otSetIcmpEchoEnabled(pFilter->otCtx, FALSE);

    // Register callbacks with OpenThread
    otSetStateChangedCallback(pFilter->otCtx, otLwfStateChangedCallback, pFilter);
    otSetReceiveIp6DatagramCallback(pFilter->otCtx, otLwfReceiveIp6DatagramCallback, pFilter);

    // Query the current addresses from TCPIP and cache them
    (void)otLwfInitializeAddresses(pFilter);

    // Initialze media connect state to disconnected
    otLwfIndicateLinkState(pFilter, MediaConnectStateDisconnected);
}

otPlatResetReason 
otPlatGetResetReason(
    _In_ otInstance *otCtx
    )
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return pFilter->cmdResetReason;
}

VOID 
otLwfRadioGetFactoryAddress(
    _In_ PMS_FILTER pFilter
    )
{
    NTSTATUS status;
    uint8_t *hwAddress = NULL;

    RtlZeroMemory(&pFilter->otFactoryAddress, sizeof(pFilter->otFactoryAddress));

    // Query the MP for the address
    status =
        otLwfCmdGetProp(
            pFilter,
            NULL,
            SPINEL_PROP_HWADDR,
            SPINEL_DATATYPE_EUI64_S,
            &hwAddress
        );
    if (!NT_SUCCESS(status) || hwAddress == NULL)
    {
        LogError(DRIVER_DEFAULT, "Get SPINEL_PROP_HWADDR failed, %!STATUS!", status);
        return;
    }

    memcpy(&pFilter->otFactoryAddress, hwAddress, sizeof(pFilter->otFactoryAddress));

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! cached factory Extended Mac Address: %llX", &pFilter->InterfaceGuid, pFilter->otFactoryAddress);
}

VOID 
otLwfRadioInit(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntry(DRIVER_DEFAULT);

    NT_ASSERT(pFilter->DeviceStatus == OTLWF_DEVICE_STATUS_RADIO_MODE);

    // Initialize the OpenThread radio capability flags
    pFilter->otRadioCapabilities = 0;
    if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO_ACK_TIMEOUT) != 0)
        pFilter->otRadioCapabilities |= kRadioCapsAckTimeout;
    if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO_MAC_RETRY_AND_COLLISION_AVOIDANCE) != 0)
        pFilter->otRadioCapabilities |= kRadioCapsTransmitRetries;
    if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO_ENERGY_SCAN) != 0)
        pFilter->otRadioCapabilities |= kRadioCapsEnergyScan;

    pFilter->otPhyState = kStateDisabled;
    pFilter->otCurrentListenChannel = 0xFF;
    pFilter->otPromiscuous = false;

    pFilter->otReceiveFrame.mPsdu = pFilter->otReceiveMessage;
    pFilter->otTransmitFrame.mPsdu = pFilter->otTransmitMessage;
    
    pFilter->otPendingMacOffloadEnabled = FALSE;
    pFilter->otPendingShortAddressCount = 0;
    pFilter->otPendingExtendedAddressCount = 0;

    // Cache the factory address
    otLwfRadioGetFactoryAddress(pFilter);
    
    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateDisabled.", pFilter);
    
    LogFuncExit(DRIVER_DEFAULT);
}

void otPlatRadioGetIeeeEui64(otInstance *otCtx, uint8_t *aIeeeEui64)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    memcpy(aIeeeEui64, &pFilter->otFactoryAddress, sizeof(ULONGLONG));
}

void otPlatRadioSetPanId(_In_ otInstance *otCtx, uint16_t panid)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! set PanID: %X", &pFilter->InterfaceGuid, panid);

    pFilter->otPanID = panid;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_15_4_PANID,
            SPINEL_DATATYPE_UINT16_S,
            panid
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_15_4_PANID failed, %!STATUS!", status);
    }
}

void otPlatRadioSetExtendedAddress(_In_ otInstance *otCtx, uint8_t *address)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;
    spinel_eui64_t extAddr;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! set Extended Mac Address: %llX", &pFilter->InterfaceGuid, *(ULONGLONG*)address);

    pFilter->otExtendedAddress = *(ULONGLONG*)address;

    for (size_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        extAddr.bytes[i] = address[7 - i];
    }

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_15_4_LADDR,
            SPINEL_DATATYPE_EUI64_S,
            &extAddr
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_15_4_LADDR failed, %!STATUS!", status);
    }
}

void otPlatRadioSetShortAddress(_In_ otInstance *otCtx, uint16_t address)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! set Short Mac Address: %X", &pFilter->InterfaceGuid, address);

    pFilter->otShortAddress = address;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_15_4_SADDR,
            SPINEL_DATATYPE_UINT16_S,
            address
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_15_4_SADDR failed, %!STATUS!", status);
    }
}

void otPlatRadioSetPromiscuous(_In_ otInstance *otCtx, bool aEnable)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    pFilter->otPromiscuous = (BOOLEAN)aEnable;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_PROMISCUOUS_MODE,
            SPINEL_DATATYPE_UINT8_S,
            aEnable != 0 ? SPINEL_MAC_PROMISCUOUS_MODE_NETWORK : SPINEL_MAC_PROMISCUOUS_MODE_OFF
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_PROMISCUOUS_MODE failed, %!STATUS!", status);
    }
}

bool otPlatRadioIsEnabled(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return pFilter->otPhyState != kStateDisabled;
}

ThreadError otPlatRadioEnable(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    NT_ASSERT(pFilter->otPhyState <= kStateSleep);
    if (pFilter->otPhyState > kStateSleep) return kThreadError_Busy;

    pFilter->otPhyState = kStateSleep;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_PHY_ENABLED,
            SPINEL_DATATYPE_BOOL_S,
            TRUE
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_PHY_ENABLED (true) failed, %!STATUS!", status);
    }

    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateSleep.", pFilter);

    return NT_SUCCESS(status) ? kThreadError_None : kThreadError_Failed;
}

ThreadError otPlatRadioDisable(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    // First make sure we are in the Sleep state if we weren't already
    if (pFilter->otPhyState > kStateSleep)
    {
        (void)otPlatRadioSleep(otCtx);
    }

    pFilter->otPhyState = kStateDisabled;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_PHY_ENABLED,
            SPINEL_DATATYPE_BOOL_S,
            FALSE
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_PHY_ENABLED (false) failed, %!STATUS!", status);
    }

    LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateDisabled.", pFilter);

    return NT_SUCCESS(status) ? kThreadError_None : kThreadError_Failed;
}

ThreadError otPlatRadioSleep(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // If we were in the transmit state, cancel the transmit
    if (pFilter->otPhyState == kStateTransmit)
    {
        pFilter->otLastTransmitError = kThreadError_Abort;
        otLwfRadioTransmitFrameDone(pFilter);
    }

    if (pFilter->otPhyState != kStateSleep)
    {
        pFilter->otPhyState = kStateSleep;
        LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateSleep.", pFilter);

        // Indicate to the miniport
        NTSTATUS status =
            otLwfCmdSetProp(
                pFilter,
                SPINEL_PROP_MAC_RAW_STREAM_ENABLED,
                SPINEL_DATATYPE_BOOL_S,
                FALSE
            );
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_RAW_STREAM_ENABLED (false) failed, %!STATUS!", status);
        }
    }

    return kThreadError_None;
}

ThreadError otPlatRadioReceive(_In_ otInstance *otCtx, uint8_t aChannel)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    
    NT_ASSERT(pFilter->otPhyState != kStateDisabled);
    if (pFilter->otPhyState == kStateDisabled) return kThreadError_Busy;
    
    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    // Update current channel if different
    if (pFilter->otCurrentListenChannel != aChannel)
    {
        NTSTATUS status;

        NT_ASSERT(aChannel >= 11 && aChannel <= 26);

        LogInfo(DRIVER_DEFAULT, "Filter %p new Listen Channel = %u.", pFilter, aChannel);
        pFilter->otCurrentListenChannel = aChannel;

        // Indicate to the miniport
        status =
            otLwfCmdSetProp(
                pFilter,
                SPINEL_PROP_PHY_CHAN,
                SPINEL_DATATYPE_UINT8_S,
                aChannel
            );
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_PHY_CHAN failed, %!STATUS!", status);
        }
    }

    // Only transition to the receive state if we were sleeping; otherwise we
    // are already in receive or transmit state.
    if (pFilter->otPhyState == kStateSleep)
    {
        pFilter->otPhyState = kStateReceive;
        LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateReceive.", pFilter);

        NTSTATUS status =
            otLwfCmdSetProp(
                pFilter,
                SPINEL_PROP_MAC_RAW_STREAM_ENABLED,
                SPINEL_DATATYPE_BOOL_S,
                TRUE
            );
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_RAW_STREAM_ENABLED (true) failed, %!STATUS!", status);
        }

        // Set the event to indicate we can process NBLs
        KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);
    }
    
    LogFuncExit(DRIVER_DATA_PATH);

    return kThreadError_None;
}

RadioPacket *otPlatRadioGetTransmitBuffer(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return &pFilter->otTransmitFrame;
}

int8_t otPlatRadioGetRssi(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    UNREFERENCED_PARAMETER(pFilter);
    return 0;
}

otRadioCaps otPlatRadioGetCaps(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    return otCtxToFilter(otCtx)->otRadioCapabilities;
}

bool otPlatRadioGetPromiscuous(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    return pFilter->otPromiscuous;
}

VOID 
otLwfRadioReceiveFrame(
    _In_ PMS_FILTER pFilter,
    _In_ ThreadError errorCode
    )
{    
    NT_ASSERT(pFilter->otReceiveFrame.mChannel >= 11 && pFilter->otReceiveFrame.mChannel <= 26);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    LogMacRecv(pFilter, pFilter->otReceiveFrame.mLength, pFilter->otReceiveFrame.mPsdu);

    if (pFilter->otPhyState > kStateDisabled)
    {
        otPlatRadioReceiveDone(pFilter->otCtx, &pFilter->otReceiveFrame, errorCode);
    }
    else
    {
        LogVerbose(DRIVER_DATA_PATH, "Mac frame dropped.");
    }
    
    LogFuncExit(DRIVER_DATA_PATH);
}

ThreadError otPlatRadioTransmit(_In_ otInstance *otCtx, _In_ RadioPacket *aPacket)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    ThreadError error = kThreadError_Busy;

    UNREFERENCED_PARAMETER(aPacket);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    NT_ASSERT(pFilter->otPhyState == kStateReceive);
    if (pFilter->otPhyState == kStateReceive)
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

    LogMacSend(pFilter, pFilter->otTransmitFrame.mLength, pFilter->otTransmitFrame.mPsdu);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    otLwfCmdSendMacFrameAsync(pFilter, &pFilter->otTransmitFrame);

    LogFuncExit(DRIVER_DATA_PATH);
}

VOID 
otLwfRadioTransmitFrameDone(
    _In_ PMS_FILTER pFilter
    )
{
    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    if (pFilter->otPhyState == kStateTransmit)
    {
        pFilter->SendPending = FALSE;

        // Now that we are completing a send, fall back to receive state and set the event
        pFilter->otPhyState = kStateReceive;
        LogInfo(DRIVER_DEFAULT, "Filter %p PhyState = kStateReceive.", pFilter);
        KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);

        if (pFilter->otLastTransmitError != kThreadError_None &&
            pFilter->otLastTransmitError != kThreadError_ChannelAccessFailure &&
            pFilter->otLastTransmitError != kThreadError_NoAck)
        {
            pFilter->otLastTransmitError = kThreadError_Abort;
        }

        otPlatRadioTransmitDone(pFilter->otCtx, &pFilter->otTransmitFrame, pFilter->otLastTransmitFramePending, pFilter->otLastTransmitError);
    }

    LogFuncExit(DRIVER_DATA_PATH);
}

NDIS_STATUS 
otPlatRadioSendPendingMacOffload(
    _In_ PMS_FILTER pFilter
    )
{
    // TODO
    UNREFERENCED_PARAMETER(pFilter);
    return STATUS_NOT_SUPPORTED;
    /*NDIS_STATUS status;
    ULONG bytesProcessed;
    UCHAR ShortAddressCount = pFilter->otPendingMacOffloadEnabled == TRUE ? pFilter->otPendingShortAddressCount : 0;
    UCHAR ExtendedAddressCount = pFilter->otPendingMacOffloadEnabled == TRUE ? pFilter->otPendingExtendedAddressCount : 0;
    USHORT OidBufferSize = COMPLETE_SIZEOF_OT_PENDING_MAC_OFFLOAD_REVISION_1(ShortAddressCount, ExtendedAddressCount);
    POT_PENDING_MAC_OFFLOAD OidBuffer = (POT_PENDING_MAC_OFFLOAD)FILTER_ALLOC_MEM(pFilter->FilterHandle, OidBufferSize);
    PUCHAR Offset = (PUCHAR)(OidBuffer + 1);
    ULONG BufferSizeLeft = OidBufferSize - sizeof(OT_PENDING_MAC_OFFLOAD);

    OidBuffer->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    OidBuffer->Header.Revision = OT_PENDING_MAC_OFFLOAD_REVISION_1;
    OidBuffer->Header.Size = OidBufferSize;

    OidBuffer->ShortAddressCount = ShortAddressCount;
    OidBuffer->ExtendedAddressCount = ExtendedAddressCount;

    if (ShortAddressCount != 0)
    {
        memcpy_s(Offset, BufferSizeLeft, pFilter->otPendingShortAddresses, sizeof(USHORT) * ShortAddressCount);
        Offset += sizeof(USHORT) * ShortAddressCount;
        BufferSizeLeft -= sizeof(USHORT) * ShortAddressCount;
    }

    if (ExtendedAddressCount != 0)
    {
        memcpy_s(Offset, BufferSizeLeft, pFilter->otPendingExtendedAddresses, sizeof(ULONGLONG) * ExtendedAddressCount);
    }

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! indicating updated Pending Mac Offload", &pFilter->InterfaceGuid);
    
    // Indicate to the miniport
    status = 
        otLwfSendInternalRequest(
            pFilter,
            NdisRequestSetInformation,
            OID_OT_PENDING_MAC_OFFLOAD,
            OidBuffer,
            OidBufferSize,
            &bytesProcessed
            );
    if (status != NDIS_STATUS_SUCCESS)
    {
        LogError(DRIVER_DEFAULT, "Set for OID_OT_PENDING_MAC_OFFLOAD failed, %!NDIS_STATUS!", status);
    }

    FILTER_FREE_MEM(OidBuffer);

    return status;*/
}

void otPlatRadioEnableSrcMatch(_In_ otInstance *otCtx, bool aEnable)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Ignore if we are already in the correct state
    if (aEnable == pFilter->otPendingMacOffloadEnabled) return;

    // Set the new value and update the miniport
    pFilter->otPendingMacOffloadEnabled = aEnable ? TRUE : FALSE;
    otPlatRadioSendPendingMacOffload(pFilter);
}

ThreadError otPlatRadioAddSrcMatchShortEntry(_In_ otInstance *otCtx, const uint16_t aShortAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Check to see if it is in the list
    BOOLEAN Found = false;
    for (ULONG i = 0; i < pFilter->otPendingShortAddressCount; i++)
    {
        if (aShortAddress == pFilter->otPendingShortAddresses[i])
        {
            Found = TRUE;
            break;
        }
    }

    // Already in the list, return success
    if (Found) return kThreadError_None;

    // Make sure we have room
    if (pFilter->otPendingShortAddressCount == MAX_PENDING_MAC_SIZE) return kThreadError_NoBufs;
    
    // Copy to the list
    pFilter->otPendingShortAddresses[pFilter->otPendingShortAddressCount] = aShortAddress;
    pFilter->otPendingShortAddressCount++;

    // Update the miniport if enabled
    if (pFilter->otPendingMacOffloadEnabled)
    {
        otPlatRadioSendPendingMacOffload(pFilter);
    }

    return kThreadError_None;
}

ThreadError otPlatRadioAddSrcMatchExtEntry(_In_ otInstance *otCtx, const uint8_t *aExtAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Check to see if it is in the list
    BOOLEAN Found = false;
    for (ULONG i = 0; i < pFilter->otPendingExtendedAddressCount; i++)
    {
        if (memcmp(aExtAddress, &pFilter->otPendingExtendedAddresses[i], sizeof(ULONGLONG)) == 0)
        {
            Found = TRUE;
            break;
        }
    }

    // Already in the list, return success
    if (Found) return kThreadError_None;

    // Make sure we have room
    if (pFilter->otPendingExtendedAddressCount == MAX_PENDING_MAC_SIZE) return kThreadError_NoBufs;
    
    // Copy to the list
    memcpy(&pFilter->otPendingExtendedAddresses[pFilter->otPendingExtendedAddressCount], aExtAddress, sizeof(ULONGLONG));
    pFilter->otPendingExtendedAddressCount++;

    // Update the miniport if enabled
    if (pFilter->otPendingMacOffloadEnabled)
    {
        otPlatRadioSendPendingMacOffload(pFilter);
    }

    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchShortEntry(_In_ otInstance *otCtx, const uint16_t aShortAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Check to see if it is in the list
    BOOLEAN Found = false;
    for (ULONG i = 0; i < pFilter->otPendingShortAddressCount; i++)
    {
        if (aShortAddress == pFilter->otPendingShortAddresses[i])
        {
            // Remove it from the list
            if (i + 1 != pFilter->otPendingShortAddressCount)
                pFilter->otPendingShortAddresses[i] = pFilter->otPendingShortAddresses[pFilter->otPendingShortAddressCount - 1];
            pFilter->otPendingShortAddressCount--;
            Found = TRUE;
            break;
        }
    }

    // Wasn't in the list, return failure
    if (Found == FALSE) return kThreadError_NotFound;

    // Update the miniport if enabled
    if (pFilter->otPendingMacOffloadEnabled)
    {
        otPlatRadioSendPendingMacOffload(pFilter);
    }

    return kThreadError_None;
}

ThreadError otPlatRadioClearSrcMatchExtEntry(_In_ otInstance *otCtx, const uint8_t *aExtAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Check to see if it is in the list
    BOOLEAN Found = false;
    for (ULONG i = 0; i < pFilter->otPendingExtendedAddressCount; i++)
    {
        if (memcmp(aExtAddress, &pFilter->otPendingExtendedAddresses[i], sizeof(ULONGLONG)) == 0)
        {
            // Remove it from the list
            if (i + 1 != pFilter->otPendingExtendedAddressCount)
                pFilter->otPendingExtendedAddresses[i] = pFilter->otPendingExtendedAddresses[pFilter->otPendingExtendedAddressCount - 1];
            pFilter->otPendingExtendedAddressCount--;
            Found = TRUE;
            break;
        }
    }
    
    // Wasn't in the list, return failure
    if (Found == FALSE) return kThreadError_NotFound;

    // Update the miniport if enabled
    if (pFilter->otPendingMacOffloadEnabled)
    {
        otPlatRadioSendPendingMacOffload(pFilter);
    }

    return kThreadError_None;
}

void otPlatRadioClearSrcMatchShortEntries(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Ignore if we are already in the correct state
    if (pFilter->otPendingShortAddressCount == 0) return;

    // Set the new value and update the miniport
    pFilter->otPendingShortAddressCount = 0;
    otPlatRadioSendPendingMacOffload(pFilter);
}

void otPlatRadioClearSrcMatchExtEntries(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Ignore if we are already in the correct state
    if (pFilter->otPendingExtendedAddressCount == 0) return;

    // Set the new value and update the miniport
    pFilter->otPendingExtendedAddressCount = 0;
    otPlatRadioSendPendingMacOffload(pFilter);
}

ThreadError otPlatRadioEnergyScan(_In_ otInstance *otCtx, uint8_t aScanChannel, uint16_t aScanDuration)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    NTSTATUS status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SCAN_MASK,
            SPINEL_DATATYPE_UINT8_S,
            aScanChannel
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SCAN_MASK failed, %!STATUS!", status);
        goto error;
    }

    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SCAN_PERIOD,
            SPINEL_DATATYPE_UINT16_S,
            aScanDuration
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SCAN_PERIOD failed, %!STATUS!", status);
        goto error;
    }

    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SCAN_STATE,
            SPINEL_DATATYPE_UINT8_S,
            SPINEL_SCAN_STATE_ENERGY
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SCAN_STATE failed, %!STATUS!", status);
        goto error;
    }

error:

    return NT_SUCCESS(status) ? kThreadError_None : kThreadError_Failed;
}

inline USHORT getDstShortAddress(const UCHAR *frame)
{
    return (((USHORT)frame[IEEE802154_DSTADDR_OFFSET + 1]) << 8) | frame[IEEE802154_DSTADDR_OFFSET];
}

inline USHORT getSrcShortAddress(ULONG frameLength, _In_reads_bytes_(frameLength) PUCHAR frame, ULONG offset)
{
    return (offset + 1 < frameLength) ? ((((USHORT)frame[offset + 1]) << 8) | frame[offset]) : 0;
}

inline ULONGLONG getDstExtAddress(const UCHAR *frame)
{
    return *(ULONGLONG*)(frame + IEEE802154_DSTADDR_OFFSET);
}

inline ULONGLONG getSrcExtAddress(ULONG frameLength, _In_reads_bytes_(frameLength) PUCHAR frame, ULONG offset)
{
    return (offset + 7 < frameLength) ? (*(ULONGLONG*)(frame + offset)) : 0;
}

void 
LogMac(
    _In_ PCSTR szDir,
    _In_ PMS_FILTER pFilter,
    ULONG frameLength,
    _In_reads_bytes_(frameLength) PUCHAR frame
    )
{
    if (frameLength < 6) return;

    NT_ASSERT(frame);

    UCHAR AckRequested = (frame[0] & IEEE802154_ACK_REQUEST) != 0 ? 1 : 0;
    UCHAR FramePending = (frame[0] & IEEE802154_FRAME_PENDING) != 0 ? 1 : 0;

    switch (frame[1] & (IEEE802154_DST_ADDR_MASK | IEEE802154_SRC_ADDR_MASK))
    {
    case IEEE802154_DST_ADDR_NONE | IEEE802154_SRC_ADDR_NONE:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: null => null (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_NONE | IEEE802154_SRC_ADDR_SHORT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %X => null (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcShortAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET), frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_NONE | IEEE802154_SRC_ADDR_EXT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %llX => null (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcExtAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET), frameLength, AckRequested, FramePending);
        break;
        
    case IEEE802154_DST_ADDR_SHORT | IEEE802154_SRC_ADDR_NONE:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: null => %X (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getDstShortAddress(frame), frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_SHORT | IEEE802154_SRC_ADDR_SHORT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %X => %X (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcShortAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET+2), getDstShortAddress(frame), frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_SHORT | IEEE802154_SRC_ADDR_EXT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %llX => %X (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcExtAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET+2), getDstShortAddress(frame), frameLength, AckRequested, FramePending);
        break;
        
    case IEEE802154_DST_ADDR_EXT | IEEE802154_SRC_ADDR_NONE:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: null => %llX (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getDstExtAddress(frame), frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_EXT | IEEE802154_SRC_ADDR_SHORT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %X => %llX (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcShortAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET+8), getDstExtAddress(frame), frameLength, AckRequested, FramePending);
        break;
    case IEEE802154_DST_ADDR_EXT | IEEE802154_SRC_ADDR_EXT:
        LogVerbose(DRIVER_DATA_PATH, "Filter: %p, %s: %llX => %llX (%u bytes, AckReq=%u, FramePending=%u)", 
            pFilter, szDir, getSrcExtAddress(frameLength, frame, IEEE802154_DSTADDR_OFFSET+8), getDstExtAddress(frame), frameLength, AckRequested, FramePending);
        break;
    }
    
#ifdef LOG_BUFFERS
    otLogBuffer(frame, frameLength);
#endif
}
