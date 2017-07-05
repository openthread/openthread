/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */

#include <assert.h>

#include <openthread/types.h>
#include <openthread/config.h>
#include <openthread/platform/platform.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>

#include "common/logging.hpp"
#include "utils/code_utils.h"

#include "em_core.h"
#include "em_system.h"
#include "rail.h"
#include "rail_ieee802154.h"
#include "openthread-core-efr32-config.h"

enum
{
    IEEE802154_MIN_LENGTH                = 5,
    IEEE802154_MAX_LENGTH                = 127,
    IEEE802154_ACK_LENGTH                = 5,
    IEEE802154_FRAME_TYPE_MASK           = 0x7,
    IEEE802154_FRAME_TYPE_ACK            = 0x2,
    IEEE802154_FRAME_PENDING             = 1 << 4,
    IEEE802154_ACK_REQUEST               = 1 << 5,
    IEEE802154_DSN_OFFSET                = 2,
};

enum
{
    EFR32_RECEIVE_SENSITIVITY = -100,  // dBm
};

static uint16_t       sPanId             = 0;
static uint8_t        sChannel           = 0;
static bool           sTransmitBusy      = false;
static bool           sPromiscuous       = false;
static bool           sIsReceiverEnabled = false;
static bool           sIsSrcMatchEnabled = false;
static otRadioState   sState             = OT_RADIO_STATE_DISABLED;

static uint8_t        sReceiveBuffer[IEEE802154_MAX_LENGTH + 1 + sizeof(RAIL_RxPacketInfo_t)];
static uint8_t        sReceivePsdu[IEEE802154_MAX_LENGTH];
static otRadioFrame   sReceiveFrame;
static otError        sReceiveError;

static otRadioFrame   sTransmitFrame;
static uint8_t        sTransmitPsdu[IEEE802154_MAX_LENGTH];
static otError        sTransmitError;

typedef struct        srcMatchEntry
{
    uint16_t checksum;
    bool     allocated;
} sSrcMatchEntry;

static sSrcMatchEntry srcMatchShortEntry[RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM];
static sSrcMatchEntry srcMatchExtEntry[RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM];

void efr32RadioInit(void)
{
    // Data Management
    RAIL_DataConfig_t railDataConfig =
    {
        TX_PACKET_DATA,
        RX_PACKET_DATA,
        PACKET_MODE,
        PACKET_MODE,
    };

    RAIL_DataConfig(&railDataConfig);

    // 802.15.4 configuration
    RAIL_IEEE802154_Config_t config =
    {
        false,                                   // promiscuousMode
        false,                                   // isPanCoordinator
        RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,  // framesMask
        RAIL_RF_STATE_RX,                        // defaultState
        100,                                     // idleTime
        192,                                     // turnaroundTime
        894,                                     // ackTimeout
        NULL                                     // addresses
    };

    sReceiveFrame.mLength  = 0;
    sReceiveFrame.mPsdu    = sReceivePsdu;
    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu   = sTransmitPsdu;

    if (RAIL_IEEE802154_2p4GHzRadioConfig())
    {
        assert(false);
    }

    if (RAIL_IEEE802154_Init(&config))
    {
        assert(false);
    }

    RAIL_TxPowerSet(OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER);

    otLogInfoPlat(sInstance, "Initialized", NULL);
}

void efr32RadioDeinit(void)
{
    RAIL_RfIdle();
    RAIL_IEEE802154_Deinit();
}

void setChannel(uint8_t aChannel)
{
    bool enabled = false;

    otEXPECT(sChannel != aChannel);

    if (sIsReceiverEnabled)
    {
        RAIL_RfIdle();
        enabled = true;
        sIsReceiverEnabled = false;
    }

    otLogInfoPlat(sInstance, "Channel=%d", aChannel);

    sChannel = aChannel;

    if (enabled)
    {
        if (RAIL_RxStart(aChannel))
        {
            assert(false);
        }

        sIsReceiverEnabled = true;
    }

exit:
    return;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    uint64_t eui64;
    uint8_t *eui64Ptr = NULL;
    (void)aInstance;

    eui64 = SYSTEM_GetUnique();
    eui64Ptr = (uint8_t *)&eui64;

    for (uint8_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        aIeeeEui64[i] = eui64Ptr[(OT_EXT_ADDRESS_SIZE - 1) - i];
    }
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "PANID=%X", aPanId);

    sPanId = aPanId;
    RAIL_IEEE802154_SetPanId(aPanId);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aAddress)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "ExtAddr=%X%X%X%X%X%X%X%X",
                  aAddress[7], aAddress[6], aAddress[5], aAddress[4],
                  aAddress[3], aAddress[2], aAddress[1], aAddress[0]);

    RAIL_IEEE802154_SetLongAddress(aAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "ShortAddr=%X", aAddress);

    RAIL_IEEE802154_SetShortAddress(aAddress);
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;
    return (sState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    otEXPECT(!otPlatRadioIsEnabled(aInstance));

    otLogInfoPlat(sInstance, "State=OT_RADIO_STATE_SLEEP", NULL);
    sState = OT_RADIO_STATE_SLEEP;

exit:
    CORE_EXIT_CRITICAL();
    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    otEXPECT(otPlatRadioIsEnabled(aInstance));

    otLogInfoPlat(sInstance, "State=OT_RADIO_STATE_DISABLED", NULL);
    sState = OT_RADIO_STATE_DISABLED;

exit:
    CORE_EXIT_CRITICAL();
    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    otEXPECT_ACTION((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED),
                    error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(sInstance, "State=OT_RADIO_STATE_SLEEP", NULL);
    sState = OT_RADIO_STATE_SLEEP;

    if (sIsReceiverEnabled)
    {
        RAIL_RfIdleExt(RAIL_IDLE, true);
        sIsReceiverEnabled = false;
    }

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    otEXPECT_ACTION(sState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(sInstance, "State=OT_RADIO_STATE_RECEIVE", NULL);
    sState = OT_RADIO_STATE_RECEIVE;
    setChannel(aChannel);
    sReceiveFrame.mChannel = aChannel;

    if (!sIsReceiverEnabled)
    {
        otEXPECT_ACTION(RAIL_RxStart(aChannel) == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

        sIsReceiverEnabled = true;
    }

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;
    RAIL_CsmaConfig_t csmaConfig = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;
    RAIL_TxData_t txData;
    RAIL_TxOptions_t txOption;
    uint8_t frame[IEEE802154_MAX_LENGTH + 1];
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    otEXPECT_ACTION((sState != OT_RADIO_STATE_DISABLED) && (sState != OT_RADIO_STATE_TRANSMIT),
                    error = OT_ERROR_INVALID_STATE);

    sState = OT_RADIO_STATE_TRANSMIT;
    sTransmitError = OT_ERROR_NONE;
    sTransmitBusy = true;

    frame[0] = aFrame->mLength;
    memcpy(frame + 1, aFrame->mPsdu, aFrame->mLength);
    txData.dataPtr = frame;
    txData.dataLength = aFrame->mLength - 1;

    txOption.waitForAck = (aFrame->mPsdu[0] & IEEE802154_ACK_REQUEST) ? true : false;
    txOption.removeCrc  = false;
    txOption.syncWordId = 0;

    RAIL_TxPowerSet(aFrame->mPower);
    setChannel(aFrame->mChannel);
    RAIL_RfIdleExt(RAIL_IDLE, true);

    otEXPECT_ACTION(RAIL_TxDataLoad(&txData) == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

    otEXPECT_ACTION(RAIL_TxStartWithOptions(aFrame->mChannel, &txOption, RAIL_CcaCsma, &csmaConfig) == RAIL_STATUS_NO_ERROR,
                    error = OT_ERROR_FAILED);

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;
    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;
    return (uint8_t)(RAIL_RxGetRSSI() >> 2);
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;
    return OT_RADIO_CAPS_ACK_TIMEOUT;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;
    return sPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    sPromiscuous = aEnable;
    RAIL_IEEE802154_SetPromiscuousMode(aEnable);
}

int8_t findSrcMatchAvailEntry(bool aShortAddress)
{
    int8_t entry = -1;

    if (aShortAddress)
    {
        for (uint8_t i = 0; i < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM; i++)
        {
            if (!srcMatchShortEntry[i].allocated)
            {
                entry = i;
                break;
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM; i++)
        {
            if (!srcMatchExtEntry[i].allocated)
            {
                entry = i;
                break;
            }
        }
    }

    return entry;
}

int8_t findSrcMatchShortEntry(const uint16_t aShortAddress)
{
    int8_t entry = -1;
    uint16_t checksum = aShortAddress + sPanId;

    for (uint8_t i = 0; i < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM; i++)
    {
        if (checksum == srcMatchShortEntry[i].checksum &&
            srcMatchShortEntry[i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

int8_t findSrcMatchExtEntry(const uint8_t *aExtAddress)
{
    int8_t entry = -1;
    uint16_t checksum = sPanId;

    checksum += (uint16_t)aExtAddress[0] | (uint16_t)(aExtAddress[1] << 8);
    checksum += (uint16_t)aExtAddress[2] | (uint16_t)(aExtAddress[3] << 8);
    checksum += (uint16_t)aExtAddress[4] | (uint16_t)(aExtAddress[5] << 8);
    checksum += (uint16_t)aExtAddress[6] | (uint16_t)(aExtAddress[7] << 8);

    for (uint8_t i = 0; i < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM; i++)
    {
        if (checksum == srcMatchExtEntry[i].checksum &&
            srcMatchExtEntry[i].allocated)
        {
            entry = i;
            break;
        }
    }

    return entry;
}

void addToSrcMatchShortIndirect(uint8_t entry, const uint16_t aShortAddress)
{
    uint16_t checksum = aShortAddress + sPanId;

    srcMatchShortEntry[entry].checksum = checksum;
    srcMatchShortEntry[entry].allocated = true;
}

void addToSrcMatchExtIndirect(uint8_t entry, const uint8_t *aExtAddress)
{
    uint16_t checksum = sPanId;

    checksum += (uint16_t)aExtAddress[0] | (uint16_t)(aExtAddress[1] << 8);
    checksum += (uint16_t)aExtAddress[2] | (uint16_t)(aExtAddress[3] << 8);
    checksum += (uint16_t)aExtAddress[4] | (uint16_t)(aExtAddress[5] << 8);
    checksum += (uint16_t)aExtAddress[6] | (uint16_t)(aExtAddress[7] << 8);

    srcMatchExtEntry[entry].checksum = checksum;
    srcMatchExtEntry[entry].allocated = true;
}

void removeFromSrcMatchShortIndirect(uint8_t entry)
{
    srcMatchShortEntry[entry].allocated = false;
    memset(&srcMatchShortEntry[entry].checksum, 0, sizeof(uint16_t));
}

void removeFromSrcMatchExtIndirect(uint8_t entry)
{
    srcMatchExtEntry[entry].allocated = false;
    memset(&srcMatchExtEntry[entry].checksum, 0, sizeof(uint16_t));
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    // set Frame Pending bit for all outgoing ACKs if aEnable is false
    sIsSrcMatchEnabled = aEnable;

    CORE_EXIT_CRITICAL();
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    otError error = OT_ERROR_NONE;
    int8_t entry = -1;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    entry = findSrcMatchAvailEntry(true);
    otLogDebgPlat(sInstance, "Add ShortAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM,
                    error = OT_ERROR_NO_BUFS);

    addToSrcMatchShortIndirect(entry, aShortAddress);

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    otError error = OT_ERROR_NONE;
    int8_t entry = -1;
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    entry = findSrcMatchAvailEntry(false);
    otLogDebgPlat(sInstance, "Add ExtAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM,
                    error = OT_ERROR_NO_BUFS);

    addToSrcMatchExtIndirect(entry, aExtAddress);

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;
    int8_t entry = -1;
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    entry = findSrcMatchShortEntry(aShortAddress);
    otLogDebgPlat(sInstance, "Clear ShortAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM,
                    error = OT_ERROR_NO_ADDRESS);

    removeFromSrcMatchShortIndirect(entry);

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    otError error = OT_ERROR_NONE;
    int8_t entry = -1;
    (void)aInstance;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    entry = findSrcMatchExtEntry(aExtAddress);
    otLogDebgPlat(sInstance, "Clear ExtAddr entry: %d", entry);

    otEXPECT_ACTION(entry >= 0 && entry < RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM,
                    error = OT_ERROR_NO_ADDRESS);

    removeFromSrcMatchExtIndirect(entry);

exit:
    CORE_EXIT_CRITICAL();
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;

    otLogDebgPlat(sInstance, "Clear ShortAddr entries", NULL);

    memset(srcMatchShortEntry, 0, sizeof(srcMatchShortEntry));
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void)aInstance;

    otLogDebgPlat(sInstance, "Clear ExtAddr entries", NULL);

    memset(srcMatchExtEntry, 0, sizeof(srcMatchExtEntry));
}

void RAILCb_IEEE802154_DataRequestCommand(RAIL_IEEE802154_Address_t *aAddress)
{
    if (sIsSrcMatchEnabled)
    {
        if ((aAddress->length == RAIL_IEEE802154_LongAddress &&
             findSrcMatchExtEntry(aAddress->longAddress) >= 0) ||
            (aAddress->length == RAIL_IEEE802154_ShortAddress &&
             findSrcMatchShortEntry(aAddress->shortAddress) >= 0))
        {
            RAIL_IEEE802154_SetFramePending();
        }
    }
    else
    {
        (void)aAddress;
        RAIL_IEEE802154_SetFramePending();
    }
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;
    return OT_ERROR_NOT_IMPLEMENTED;
}

void RAILCb_TxRadioStatus(uint8_t aStatus)
{
    switch (aStatus)
    {
    case RAIL_TX_CONFIG_CHANNEL_BUSY:
        sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        sTransmitBusy = false;
        break;

    case RAIL_TX_CONFIG_TX_ABORTED:
        sTransmitError = OT_ERROR_ABORT;
        sTransmitBusy = false;
        break;

    case RAIL_TX_CONFIG_BUFFER_UNDERFLOW:
        sTransmitError = OT_ERROR_ABORT;
        sTransmitBusy = false;
        break;

    default:
        break;
    }
}

void RAILCb_TxPacketSent(RAIL_TxPacketInfo_t *aTxPacketInfo)
{
    (void)aTxPacketInfo;
    sTransmitError = OT_ERROR_NONE;
    sTransmitBusy = false;
}

void RAILCb_RxPacketReceived(void *aRxPacketHandle)
{
    RAIL_RxPacketInfo_t *rxPacketInfo;
    uint8_t length;

    rxPacketInfo = (RAIL_RxPacketInfo_t *)aRxPacketHandle;

    // check recv packet appended info
    otEXPECT(rxPacketInfo != NULL &&
             rxPacketInfo->appendedInfo.crcStatus &&
             rxPacketInfo->appendedInfo.frameCodingStatus);

    length = rxPacketInfo->dataLength + 1;

    // check the length in recv packet info structure
    otEXPECT(length == rxPacketInfo->dataPtr[0]);

    // check the lenght validity of recv packet
    otEXPECT(length >= IEEE802154_MIN_LENGTH && length <= IEEE802154_MAX_LENGTH);

    otLogInfoPlat(sInstance, "Received data:%d", rxPacketInfo->dataLength);

    memcpy(sReceiveFrame.mPsdu, rxPacketInfo->dataPtr + 1, rxPacketInfo->dataLength);
    sReceiveFrame.mPower = rxPacketInfo->appendedInfo.rssiLatch;
    sReceiveFrame.mLqi = rxPacketInfo->appendedInfo.lqi;
    sReceiveFrame.mLength = length;
    sReceiveError = OT_ERROR_NONE;

exit:
    return;
}

void efr32RadioProcess(otInstance *aInstance)
{
    // ensure process would not be broken by
    // the incoming interrupt with higher priority
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    if ((sState == OT_RADIO_STATE_RECEIVE && sReceiveFrame.mLength > 0) ||
        (sState == OT_RADIO_STATE_TRANSMIT && sReceiveFrame.mLength > IEEE802154_ACK_LENGTH))
    {
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
        }
        else
#endif
        {
            // signal MAC layer for each received frame if promiscous is enabled
            // otherwise only signal MAC layer for non-ACK frame
            if (sPromiscuous || sReceiveFrame.mLength > IEEE802154_ACK_LENGTH)
            {
                otLogInfoPlat(sInstance, "Received %d bytes", sReceiveFrame.mLength);
                otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
            }
        }
    }

    if (sState == OT_RADIO_STATE_TRANSMIT && sTransmitBusy == false)
    {
        if (sTransmitError != OT_ERROR_NONE || (sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
        {
            if (sTransmitError != OT_ERROR_NONE)
            {
                otLogDebgPlat(sInstance, "Transmit failed ErrorCode=%d", sTransmitError);
            }

            sState = OT_RADIO_STATE_RECEIVE;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, sTransmitError);
            }
            else
#endif
            {
                otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
            }
        }
        else if (sReceiveFrame.mLength == IEEE802154_ACK_LENGTH &&
                 (sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK &&
                 (sReceiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET]))
        {
            sState = OT_RADIO_STATE_RECEIVE;

            otLogInfoPlat(sInstance, "Received ACK:%d", sReceiveFrame.mLength);
            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveFrame, sTransmitError);
        }
    }

    sReceiveFrame.mLength = 0;
    CORE_EXIT_CRITICAL();
}

void RAILCb_RxAckTimeout(void)
{
    sTransmitError = OT_ERROR_NO_ACK;
}

void RAILCb_RfReady(void)
{
}

void RAILCb_CalNeeded(void)
{
}

void RAILCb_RxRadioStatus(uint8_t aStatus)
{
    (void)aStatus;
}

void RAILCb_RadioStateChanged(uint8_t aState)
{
    (void)aState;
}

void RAILCb_RssiAverageDone(int16_t avgRssi)
{
    (void)avgRssi;
}

void RAILCb_RxFifoAlmostFull(uint16_t aBytesAvailable)
{
    (void)aBytesAvailable;
}

void RAILCb_TxFifoAlmostEmpty(uint16_t aSpaceAvailable)
{
    (void)aSpaceAvailable;
}

void *RAILCb_AllocateMemory(uint32_t aSize)
{
    uint8_t *pointer = NULL;
    CORE_DECLARE_IRQ_STATE;

    CORE_ENTER_CRITICAL();

    otEXPECT(aSize <= (IEEE802154_MAX_LENGTH + 1 + sizeof(RAIL_RxPacketInfo_t)));

    pointer = sReceiveBuffer;

exit:
    CORE_EXIT_CRITICAL();
    return pointer;
}

void *RAILCb_BeginWriteMemory(void *aHandle, uint32_t aOffset,
                              uint32_t *available)
{
    (void)available;
    return ((uint8_t *)aHandle) + aOffset;
}

void RAILCb_EndWriteMemory(void *aHandle, uint32_t aOffset, uint32_t aSize)
{
    (void)aHandle;
    (void)aOffset;
    (void)aSize;
}

void RAILCb_FreeMemory(void *aHandle)
{
    (void)aHandle;
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    (void)aInstance;
    RAIL_TxPowerSet(aPower);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    (void)aInstance;
    return EFR32_RECEIVE_SENSITIVITY;
}
