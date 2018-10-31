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

    LogFuncEntry(DRIVER_DEFAULT);

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! resetting...", &pFilter->InterfaceGuid);

    // Indicate to the miniport
    (void)otLwfCmdResetDevice(pFilter, TRUE);

    // Finalize previous OpenThread instance
    otLwfReleaseInstance(pFilter);

    // Reset radio layer
    pFilter->otRadioState = OT_RADIO_STATE_DISABLED;
    pFilter->otCurrentListenChannel = 0xFF;
    pFilter->otPromiscuous = false;
    pFilter->otPendingMacOffloadEnabled = FALSE;

    // Reinitialize the OpenThread library
    pFilter->otCachedRole = OT_DEVICE_ROLE_DISABLED;
    pFilter->otCtx = otInstanceInit(pFilter->otInstanceBuffer + sizeof(PMS_FILTER), &pFilter->otInstanceSize);
    ASSERT(pFilter->otCtx);

    // Make sure our helper function returns the right pointer for the filter, given the openthread instance
    NT_ASSERT(otCtxToFilter(pFilter->otCtx) == pFilter);

    // Disable Icmp (ping) handling
    otIcmp6SetEchoMode(pFilter->otCtx, OT_ICMP6_ECHO_HANDLER_DISABLED);

    // Register callbacks with OpenThread
    otSetStateChangedCallback(pFilter->otCtx, otLwfStateChangedCallback, pFilter);
    otIp6SetReceiveCallback(pFilter->otCtx, otLwfReceiveIp6DatagramCallback, pFilter);

    // Query the current addresses from TCPIP and cache them
    (void)otLwfInitializeAddresses(pFilter);

    // Initialze media connect state to disconnected
    otLwfIndicateLinkState(pFilter, MediaConnectStateDisconnected);

    LogFuncExit(DRIVER_DEFAULT);
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
    PVOID SpinelBuffer = NULL;
    uint8_t *hwAddress = NULL;

    RtlZeroMemory(&pFilter->otFactoryAddress, sizeof(pFilter->otFactoryAddress));

    // Query the MP for the address
    status =
        otLwfCmdGetProp(
            pFilter,
            &SpinelBuffer,
            SPINEL_PROP_HWADDR,
            SPINEL_DATATYPE_EUI64_S,
            &hwAddress
        );
    if (!NT_SUCCESS(status) || hwAddress == NULL)
    {
        LogError(DRIVER_DEFAULT, "Get SPINEL_PROP_HWADDR failed, %!STATUS!", status);
        return;
    }

    NT_ASSERT(SpinelBuffer);
    memcpy(&pFilter->otFactoryAddress, hwAddress, sizeof(pFilter->otFactoryAddress));
    FILTER_FREE_MEM(SpinelBuffer);

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
        pFilter->otRadioCapabilities |= OT_RADIO_CAPS_ACK_TIMEOUT;
    if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO_MAC_RETRY_AND_COLLISION_AVOIDANCE) != 0)
        pFilter->otRadioCapabilities |= OT_RADIO_CAPS_TRANSMIT_RETRIES;
    if ((pFilter->DeviceCapabilities & OTLWF_DEVICE_CAP_RADIO_ENERGY_SCAN) != 0)
        pFilter->otRadioCapabilities |= OT_RADIO_CAPS_ENERGY_SCAN;

    pFilter->otRadioState = OT_RADIO_STATE_DISABLED;
    pFilter->otCurrentListenChannel = 0xFF;
    pFilter->otPromiscuous = false;

    pFilter->otReceiveFrame.mPsdu = pFilter->otReceiveMessage;
    pFilter->otTransmitFrame.mPsdu = pFilter->otTransmitMessage;
    
    pFilter->otPendingMacOffloadEnabled = FALSE;

    // Cache the factory address
    otLwfRadioGetFactoryAddress(pFilter);
    
    LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_DISABLED.", pFilter);
    
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

    if (pFilter->otRadioState != OT_RADIO_STATE_DISABLED &&
        pFilter->otPanID != 0xFFFF)
    {
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
}

void otPlatRadioSetExtendedAddress(_In_ otInstance *otCtx, const otExtAddress *address)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;
    spinel_eui64_t extAddr;

    LogInfo(DRIVER_DEFAULT, "Interface %!GUID! set Extended Mac Address: %llX", &pFilter->InterfaceGuid, *(ULONGLONG*)address);

    pFilter->otExtendedAddress = *(ULONGLONG*)address;

    for (size_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        extAddr.bytes[i] = address->m8[7 - i];
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

    if (pFilter->otRadioState != OT_RADIO_STATE_DISABLED)
    {
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
    return pFilter->otRadioState != OT_RADIO_STATE_DISABLED;
}

otError otPlatRadioEnable(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    NT_ASSERT(pFilter->otRadioState <= OT_RADIO_STATE_SLEEP);
    if (pFilter->otRadioState > OT_RADIO_STATE_SLEEP) return OT_ERROR_BUSY;

    pFilter->otRadioState = OT_RADIO_STATE_SLEEP;

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

    LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_SLEEP.", pFilter);

    if (pFilter->otPanID != 0xFFFF)
    {
        // Indicate PANID to the miniport
        status =
            otLwfCmdSetProp(
                pFilter,
                SPINEL_PROP_MAC_15_4_PANID,
                SPINEL_DATATYPE_UINT16_S,
                pFilter->otPanID
            );
        if (!NT_SUCCESS(status))
        {
            LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_15_4_PANID failed, %!STATUS!", status);
        }
    }

    // Indicate Short address to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_15_4_SADDR,
            SPINEL_DATATYPE_UINT16_S,
            pFilter->otShortAddress
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_15_4_SADDR failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioDisable(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    // First make sure we are in the Sleep state if we weren't already
    if (pFilter->otRadioState > OT_RADIO_STATE_SLEEP)
    {
        (void)otPlatRadioSleep(otCtx);
    }

    pFilter->otRadioState = OT_RADIO_STATE_DISABLED;

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

    LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_DISABLED.", pFilter);

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioSleep(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // If we were in the transmit state, cancel the transmit
    if (pFilter->otRadioState == OT_RADIO_STATE_TRANSMIT)
    {
        pFilter->otLastTransmitError = OT_ERROR_ABORT;
        otLwfRadioTransmitFrameDone(pFilter);
    }

    if (pFilter->otRadioState != OT_RADIO_STATE_SLEEP)
    {
        pFilter->otRadioState = OT_RADIO_STATE_SLEEP;
        LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_SLEEP.", pFilter);

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

    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(_In_ otInstance *otCtx, uint8_t aChannel)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    
    NT_ASSERT(pFilter->otRadioState != OT_RADIO_STATE_DISABLED);
    if (pFilter->otRadioState == OT_RADIO_STATE_DISABLED) return OT_ERROR_BUSY;
    
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
    if (pFilter->otRadioState == OT_RADIO_STATE_SLEEP)
    {
        pFilter->otRadioState = OT_RADIO_STATE_RECEIVE;
        LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_RECEIVE.", pFilter);

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

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(_In_ otInstance *otCtx)
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
    _In_ otError errorCode
    )
{    
    NT_ASSERT(pFilter->otReceiveFrame.mChannel >= 11 && pFilter->otReceiveFrame.mChannel <= 26);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    LogMacRecv(pFilter, pFilter->otReceiveFrame.mLength, pFilter->otReceiveFrame.mPsdu);

    if (pFilter->otRadioState > OT_RADIO_STATE_DISABLED)
    {
        otPlatRadioReceiveDone(pFilter->otCtx, &pFilter->otReceiveFrame, errorCode);
    }
    else
    {
        LogVerbose(DRIVER_DATA_PATH, "Mac frame dropped.");
    }
    
    LogFuncExit(DRIVER_DATA_PATH);
}

otError otPlatRadioTransmit(_In_ otInstance *otCtx, _In_ otRadioFrame *aFrame)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    otError error = OT_ERROR_BUSY;

    UNREFERENCED_PARAMETER(aFrame);

    LogFuncEntryMsg(DRIVER_DATA_PATH, "Filter: %p", pFilter);

    NT_ASSERT(pFilter->otRadioState == OT_RADIO_STATE_RECEIVE);
    if (pFilter->otRadioState == OT_RADIO_STATE_RECEIVE)
    {
        error = OT_ERROR_NONE;
        pFilter->otRadioState = OT_RADIO_STATE_TRANSMIT;
    
        LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_TRANSMIT.", pFilter);
    }

    LogFuncExitMsg(DRIVER_DATA_PATH, "%u", error);

    return error;
}

VOID otLwfRadioTransmitFrame(_In_ PMS_FILTER pFilter)
{
    NT_ASSERT(pFilter->otRadioState == OT_RADIO_STATE_TRANSMIT);

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

    if (pFilter->otRadioState == OT_RADIO_STATE_TRANSMIT)
    {
        pFilter->SendPending = FALSE;

        // Now that we are completing a send, fall back to receive state and set the event
        pFilter->otRadioState = OT_RADIO_STATE_RECEIVE;
        LogInfo(DRIVER_DEFAULT, "Filter %p RadioState = OT_RADIO_STATE_RECEIVE.", pFilter);
        KeSetEvent(&pFilter->EventWorkerThreadProcessNBLs, 0, FALSE);

        if (pFilter->otLastTransmitError != OT_ERROR_NONE &&
            pFilter->otLastTransmitError != OT_ERROR_CHANNEL_ACCESS_FAILURE &&
            pFilter->otLastTransmitError != OT_ERROR_NO_ACK)
        {
            pFilter->otLastTransmitError = OT_ERROR_ABORT;
        }

        if (((pFilter->otTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0) ||
            pFilter->otLastTransmitError != OT_ERROR_NONE)
        {
            otPlatRadioTxDone(pFilter->otCtx, &pFilter->otTransmitFrame, NULL, pFilter->otLastTransmitError);
        }
        else
        {
            otPlatRadioTxDone(pFilter->otCtx, &pFilter->otTransmitFrame, &pFilter->otReceiveFrame, pFilter->otLastTransmitError);
        }
    }

    LogFuncExit(DRIVER_DATA_PATH);
}

void otPlatRadioEnableSrcMatch(_In_ otInstance *otCtx, bool aEnable)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Ignore if we are already in the correct state
    if (aEnable == pFilter->otPendingMacOffloadEnabled) return;

    // Cache the new value
    pFilter->otPendingMacOffloadEnabled = aEnable ? TRUE : FALSE;

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_ENABLED,
            SPINEL_DATATYPE_BOOL_S,
            (aEnable ? TRUE : FALSE)
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SRC_MATCH_ENABLED failed, %!STATUS!", status);
    }
}

otError otPlatRadioAddSrcMatchShortEntry(_In_ otInstance *otCtx, otShortAddress aShortAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdInsertProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES,
            SPINEL_DATATYPE_UINT16_S,
            aShortAddress
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Insert SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioAddSrcMatchExtEntry(_In_ otInstance *otCtx, const otExtAddress *aExtAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdInsertProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES,
            SPINEL_DATATYPE_EUI64_S,
            aExtAddress
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Insert SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioClearSrcMatchShortEntry(_In_ otInstance *otCtx, otShortAddress aShortAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdRemoveProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES,
            SPINEL_DATATYPE_UINT16_S,
            aShortAddress
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Remove SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioClearSrcMatchExtEntry(_In_ otInstance *otCtx, const otExtAddress *aExtAddress)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdRemoveProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES,
            SPINEL_DATATYPE_EUI64_S,
            aExtAddress
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Remove SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

void otPlatRadioClearSrcMatchShortEntries(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES,
            NULL
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES failed, %!STATUS!", status);
    }
}

void otPlatRadioClearSrcMatchExtEntries(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);

    // Indicate to the miniport
    NTSTATUS status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES,
            NULL
        );
    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES failed, %!STATUS!", status);
    }
}

otError otPlatRadioEnergyScan(_In_ otInstance *otCtx, uint8_t aScanChannel, uint16_t aScanDuration)
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

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioGetTransmitPower(_In_ otInstance *otCtx, int8_t *aPower)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    status =
        otLwfCmdGetProp(
            pFilter,
            NULL,
            SPINEL_PROP_PHY_TX_POWER,
            SPINEL_DATATYPE_INT8_S,
            aPower
        );

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Get SPINEL_PROP_PHY_TX_POWER, failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatRadioSetTransmitPower(_In_ otInstance *otCtx, int8_t aPower)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;

    // Indicate to the miniport
    status =
        otLwfCmdSetProp(
            pFilter,
            SPINEL_PROP_PHY_TX_POWER,
            SPINEL_DATATYPE_INT8_S,
            aPower
        );

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Set SPINEL_PROP_PHY_TX_POWER failed, %!STATUS!", status);
    }

    return NT_SUCCESS(status) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

int8_t otPlatRadioGetReceiveSensitivity(_In_ otInstance *otCtx)
{
    NT_ASSERT(otCtx);
    PMS_FILTER pFilter = otCtxToFilter(otCtx);
    NTSTATUS status;
    int8_t receiveSensitivity;

    status =
        otLwfCmdGetProp(
            pFilter,
            NULL,
            SPINEL_PROP_PHY_RX_SENSITIVITY,
            SPINEL_DATATYPE_INT8_S,
            &receiveSensitivity
        );

    if (!NT_SUCCESS(status))
    {
        LogError(DRIVER_DEFAULT, "Get SPINEL_PROP_PHY_RX_SENSITIVITY, failed, %!STATUS!", status);
        return -100;  // return default value -100dBm
    }

    return receiveSensitivity;
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
