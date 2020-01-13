/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "openthread-system.h"
#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "common/logging.hpp"
#include "utils/code_utils.h"

#include "utils/soft_source_match_table.h"

#include "board_config.h"
#include "em_cmu.h"
#include "em_core.h"
#include "em_system.h"
#include "openthread-core-efr32-config.h"
#include "pa_conversions_efr32.h"
#include "platform-band.h"
#include "rail.h"
#include "rail_config.h"
#include "rail_ieee802154.h"

enum
{
    IEEE802154_MIN_LENGTH      = 5,
    IEEE802154_MAX_LENGTH      = 127,
    IEEE802154_ACK_LENGTH      = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_ACK  = 0x2,
    IEEE802154_FRAME_PENDING   = 1 << 4,
    IEEE802154_ACK_REQUEST     = 1 << 5,
    IEEE802154_DSN_OFFSET      = 2,
};

enum
{
    EFR32_RECEIVE_SENSITIVITY    = -100, // dBm
    EFR32_RSSI_AVERAGING_TIME    = 16,   // us
    EFR32_RSSI_AVERAGING_TIMEOUT = 300,  // us
};

enum
{
    EFR32_SCHEDULER_SAMPLE_RSSI_PRIORITY = 10, // High priority
    EFR32_SCHEDULER_TX_PRIORITY          = 10, // High priority
    EFR32_SCHEDULER_RX_PRIORITY          = 20, // Low priority
};

enum
{
    EFR32_NUM_BAND_CONFIGS = 1,
};

typedef enum
{
    ENERGY_SCAN_STATUS_IDLE,
    ENERGY_SCAN_STATUS_IN_PROGRESS,
    ENERGY_SCAN_STATUS_COMPLETED
} energyScanStatus;

typedef enum
{
    ENERGY_SCAN_MODE_SYNC,
    ENERGY_SCAN_MODE_ASYNC
} energyScanMode;

RAIL_Handle_t gRailHandle;

static volatile bool sTransmitBusy      = false;
static bool          sPromiscuous       = false;
static bool          sIsSrcMatchEnabled = false;
static otRadioState  sState             = OT_RADIO_STATE_DISABLED;

static uint8_t      sReceivePsdu[IEEE802154_MAX_LENGTH];
static otRadioFrame sReceiveFrame;
static otError      sReceiveError;

static otRadioFrame     sTransmitFrame;
static uint8_t          sTransmitPsdu[IEEE802154_MAX_LENGTH];
static volatile otError sTransmitError;

static efr32BandConfig sBandConfigs[EFR32_NUM_BAND_CONFIGS];

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
static efr32RadioCounters sRailDebugCounters;
#endif

static volatile energyScanStatus sEnergyScanStatus;
static volatile int8_t           sEnergyScanResultDbm;
static energyScanMode            sEnergyScanMode;

#define QUARTER_DBM_IN_DBM 4
#define US_IN_MS 1000

static void RAILCb_Generic(RAIL_Handle_t aRailHandle, RAIL_Events_t aEvents);

static const RAIL_IEEE802154_Config_t sRailIeee802154Config = {
    NULL, // addresses
    {
        // ackConfig
        true, // ackConfig.enable
        894,  // ackConfig.ackTimeout
        {
            // ackConfig.rxTransitions
            RAIL_RF_STATE_RX, // ackConfig.rxTransitions.success
            RAIL_RF_STATE_RX, // ackConfig.rxTransitions.error
        },
        {
            // ackConfig.txTransitions
            RAIL_RF_STATE_RX, // ackConfig.txTransitions.success
            RAIL_RF_STATE_RX, // ackConfig.txTransitions.error
        },
    },
    {
        // timings
        100,      // timings.idleToRx
        192 - 10, // timings.txToRx
        100,      // timings.idleToTx
        192,      // timings.rxToTx
        0,        // timings.rxSearchTimeout
        0,        // timings.txToRxSearchTimeout
    },
    RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES, // framesMask
    false,                                  // promiscuousMode
    false,                                  // isPanCoordinator
};

RAIL_DECLARE_TX_POWER_VBAT_CURVES_ALT;

static int8_t sTxPowerDbm = OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER;

static efr32BandConfig *sTxBandConfig = NULL;
static efr32BandConfig *sRxBandConfig = NULL;

static RAIL_Handle_t efr32RailConfigInit(efr32BandConfig *aBandConfig)
{
    RAIL_Status_t     status;
    RAIL_Handle_t     handle;
    RAIL_DataConfig_t railDataConfig = {
        TX_PACKET_DATA,
        RX_PACKET_DATA,
        PACKET_MODE,
        PACKET_MODE,
    };

    handle = RAIL_Init(&aBandConfig->mRailConfig, NULL);
    assert(handle != NULL);

    status = RAIL_ConfigData(handle, &railDataConfig);
    assert(status == RAIL_STATUS_NO_ERROR);

    RAIL_Idle(handle, RAIL_IDLE, true);

    status = RAIL_ConfigCal(handle, RAIL_CAL_ALL);
    assert(status == RAIL_STATUS_NO_ERROR);

    if (aBandConfig->mChannelConfig != NULL)
    {
        RAIL_ConfigChannels(handle, aBandConfig->mChannelConfig, NULL);
    }
    else
    {
        status = RAIL_IEEE802154_Config2p4GHzRadio(handle);
        assert(status == RAIL_STATUS_NO_ERROR);
    }

    status = RAIL_IEEE802154_Init(handle, &sRailIeee802154Config);
    assert(status == RAIL_STATUS_NO_ERROR);

    status = RAIL_ConfigEvents(handle, RAIL_EVENTS_ALL,
                               RAIL_EVENT_RX_ACK_TIMEOUT |                      //
                                   RAIL_EVENTS_TX_COMPLETION |                  //
                                   RAIL_EVENT_RX_PACKET_RECEIVED |              //
                                   RAIL_EVENT_RSSI_AVERAGE_DONE |               //
                                   RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND | //
                                   RAIL_EVENT_CAL_NEEDED |                      //
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
                                   RAIL_EVENT_CONFIG_SCHEDULED |   //
                                   RAIL_EVENT_CONFIG_UNSCHEDULED | //
#endif
                                   RAIL_EVENT_SCHEDULER_STATUS //
    );
    assert(status == RAIL_STATUS_NO_ERROR);

    uint16_t actualLength = RAIL_SetTxFifo(handle, aBandConfig->mRailTxFifo, 0, sizeof(aBandConfig->mRailTxFifo));
    assert(actualLength == sizeof(aBandConfig->mRailTxFifo));

    return handle;
}

static void efr32RadioSetTxPower(RAIL_Handle_t               aRailHandle,
                                 const RAIL_ChannelConfig_t *aChannelConfig,
                                 int8_t                      aPowerDbm)
{
    RAIL_Status_t                       status;
    const RAIL_TxPowerCurvesConfigAlt_t txPowerCurvesConfig = RAIL_DECLARE_TX_POWER_CURVES_CONFIG_ALT;
    RAIL_TxPowerConfig_t                txPowerConfig       = {RAIL_TX_POWER_MODE_2P4_HP, 3300, 10};

    status = RAIL_InitTxPowerCurvesAlt(&txPowerCurvesConfig);
    assert(status == RAIL_STATUS_NO_ERROR);

    status = RAIL_ConfigTxPower(aRailHandle, &txPowerConfig);
    assert(status == RAIL_STATUS_NO_ERROR);

    status = RAIL_SetTxPowerDbm(aRailHandle, ((RAIL_TxPower_t)aPowerDbm) * 10);
    assert(status == RAIL_STATUS_NO_ERROR);
}

static efr32BandConfig *efr32RadioGetBandConfig(uint8_t aChannel)
{
    efr32BandConfig *config = NULL;

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        if ((sBandConfigs[i].mChannelMin <= aChannel) && (aChannel <= sBandConfigs[i].mChannelMax))
        {
            config = &sBandConfigs[i];
            break;
        }
    }

    return config;
}

static void efr32BandConfigInit(void (*aEventCallback)(RAIL_Handle_t railHandle, RAIL_Events_t events))
{
    uint8_t index = 0;

#if RADIO_CONFIG_2P4GHZ_OQPSK_SUPPORT
    sBandConfigs[index].mRailConfig.eventsCallback = aEventCallback;
    sBandConfigs[index].mRailConfig.protocol       = NULL;
    sBandConfigs[index].mRailConfig.scheduler      = &sBandConfigs[index].mRailSchedState;
    sBandConfigs[index].mChannelConfig             = NULL;
    sBandConfigs[index].mChannelMin                = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN;
    sBandConfigs[index].mChannelMax                = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX;

    assert((sBandConfigs[index].mRailHandle = efr32RailConfigInit(&sBandConfigs[index])) != NULL);
    index++;
#endif
}

void efr32RadioInit(void)
{
    efr32BandConfigInit(RAILCb_Generic);

    sReceiveFrame.mLength  = 0;
    sReceiveFrame.mPsdu    = sReceivePsdu;
    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu   = sTransmitPsdu;

    sRxBandConfig = efr32RadioGetBandConfig(OPENTHREAD_CONFIG_DEFAULT_CHANNEL);
    assert(sRxBandConfig != NULL);

    sTxBandConfig = sRxBandConfig;
    efr32RadioSetTxPower(sTxBandConfig->mRailHandle, sTxBandConfig->mChannelConfig, sTxPowerDbm);

    gRailHandle = sTxBandConfig->mRailHandle; // global handle for alarms.

    sEnergyScanStatus = ENERGY_SCAN_STATUS_IDLE;
    sTransmitError    = OT_ERROR_NONE;
    sTransmitBusy     = false;

    otLogInfoPlat("Initialized", NULL);
}

void efr32RadioDeinit(void)
{
    RAIL_Status_t status;

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        RAIL_Idle(sBandConfigs[i].mRailHandle, RAIL_IDLE_FORCE_SHUTDOWN_CLEAR_FLAGS, true);

        status = RAIL_IEEE802154_Deinit(sBandConfigs[i].mRailHandle);
        assert(status == RAIL_STATUS_NO_ERROR);

        sBandConfigs[i].mRailHandle = NULL;
    }

    sTxBandConfig = NULL;
    sRxBandConfig = NULL;
}

static otError efr32StartEnergyScan(energyScanMode aMode, uint16_t aChannel, RAIL_Time_t aAveragingTimeUs)
{
    RAIL_Status_t status;
    otError       error = OT_ERROR_NONE;

    otEXPECT_ACTION(sEnergyScanStatus == ENERGY_SCAN_STATUS_IDLE, error = OT_ERROR_BUSY);

    sEnergyScanStatus = ENERGY_SCAN_STATUS_IN_PROGRESS;
    sEnergyScanMode   = aMode;

    RAIL_Idle(sRxBandConfig->mRailHandle, RAIL_IDLE, true);

    RAIL_SchedulerInfo_t scanSchedulerInfo = {.priority        = RADIO_SCHEDULER_CHANNEL_SCAN_PRIORITY,
                                              .slipTime        = RADIO_SCHEDULER_CHANNEL_SLIP_TIME,
                                              .transactionTime = aAveragingTimeUs};

    status = RAIL_StartAverageRssi(sRxBandConfig->mRailHandle, aChannel, aAveragingTimeUs, &scanSchedulerInfo);
    otEXPECT_ACTION(status == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

exit:
    return error;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint64_t eui64;
    uint8_t *eui64Ptr = NULL;

    eui64    = SYSTEM_GetUnique();
    eui64Ptr = (uint8_t *)&eui64;

    for (uint8_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
    {
        aIeeeEui64[i] = eui64Ptr[(OT_EXT_ADDRESS_SIZE - 1) - i];
    }
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    otLogInfoPlat("PANID=%X", aPanId);

    utilsSoftSrcMatchSetPanId(aPanId);

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        status = RAIL_IEEE802154_SetPanId(sBandConfigs[i].mRailHandle, aPanId, 0);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    otLogInfoPlat("ExtAddr=%X%X%X%X%X%X%X%X", aAddress->m8[7], aAddress->m8[6], aAddress->m8[5], aAddress->m8[4],
                  aAddress->m8[3], aAddress->m8[2], aAddress->m8[1], aAddress->m8[0]);

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        status = RAIL_IEEE802154_SetLongAddress(sBandConfigs[i].mRailHandle, (uint8_t *)aAddress->m8, 0);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    otLogInfoPlat("ShortAddr=%X", aAddress);

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        status = RAIL_IEEE802154_SetShortAddress(sBandConfigs[i].mRailHandle, aAddress, 0);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otEXPECT(!otPlatRadioIsEnabled(aInstance));

    otLogInfoPlat("State=OT_RADIO_STATE_SLEEP", NULL);
    sState = OT_RADIO_STATE_SLEEP;

exit:
    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otEXPECT(otPlatRadioIsEnabled(aInstance));

    otLogInfoPlat("State=OT_RADIO_STATE_DISABLED", NULL);
    sState = OT_RADIO_STATE_DISABLED;

exit:
    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((sState != OT_RADIO_STATE_TRANSMIT) && (sState != OT_RADIO_STATE_DISABLED),
                    error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat("State=OT_RADIO_STATE_SLEEP", NULL);

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        RAIL_Idle(sBandConfigs[i].mRailHandle, RAIL_IDLE, true);
    }
    sState = OT_RADIO_STATE_SLEEP;

exit:
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    otError          error = OT_ERROR_NONE;
    RAIL_Status_t    status;
    efr32BandConfig *config;

    OT_UNUSED_VARIABLE(aInstance);
    otEXPECT_ACTION(sState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    config = efr32RadioGetBandConfig(aChannel);
    otEXPECT_ACTION(config != NULL, error = OT_ERROR_INVALID_ARGS);

    if (sRxBandConfig != config)
    {
        RAIL_Idle(sRxBandConfig->mRailHandle, RAIL_IDLE, false);
        sRxBandConfig = config;
    }

    RAIL_SchedulerInfo_t bgRxSchedulerInfo = {
        .priority = RADIO_SCHEDULER_BACKGROUND_RX_PRIORITY,
        // sliptime/transaction time is not used for bg rx
    };

    status = RAIL_StartRx(sRxBandConfig->mRailHandle, aChannel, &bgRxSchedulerInfo);
    otEXPECT_ACTION(status == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

    otLogInfoPlat("State=OT_RADIO_STATE_RECEIVE", NULL);
    sState                 = OT_RADIO_STATE_RECEIVE;
    sReceiveFrame.mChannel = aChannel;

exit:
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError           error      = OT_ERROR_NONE;
    RAIL_CsmaConfig_t csmaConfig = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;
    RAIL_TxOptions_t  txOptions  = RAIL_TX_OPTIONS_NONE;
    efr32BandConfig * config;
    RAIL_Status_t     status;
    uint8_t           frameLength;

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    sRailDebugCounters.mRailPlatTxTriggered++;
#endif

    assert(sTransmitBusy == false);

    otEXPECT_ACTION((sState != OT_RADIO_STATE_DISABLED) && (sState != OT_RADIO_STATE_TRANSMIT),
                    error = OT_ERROR_INVALID_STATE);

    config = efr32RadioGetBandConfig(aFrame->mChannel);
    otEXPECT_ACTION(config != NULL, error = OT_ERROR_INVALID_ARGS);

    sState         = OT_RADIO_STATE_TRANSMIT;
    sTransmitError = OT_ERROR_NONE;
    sTransmitBusy  = true;

    if (sTxBandConfig != config)
    {
        efr32RadioSetTxPower(config->mRailHandle, config->mChannelConfig, sTxPowerDbm);
        sTxBandConfig = config;
    }

    frameLength = (uint8_t)aFrame->mLength;
    RAIL_WriteTxFifo(sTxBandConfig->mRailHandle, &frameLength, sizeof frameLength, true);
    RAIL_WriteTxFifo(sTxBandConfig->mRailHandle, aFrame->mPsdu, frameLength - 2, false);

    RAIL_SchedulerInfo_t txSchedulerInfo = {
        .priority        = RADIO_SCHEDULER_TX_PRIORITY,
        .slipTime        = RADIO_SCHEDULER_CHANNEL_SLIP_TIME,
        .transactionTime = 0, // will be calculated later if DMP is used
    };

    if (aFrame->mPsdu[0] & IEEE802154_ACK_REQUEST)
    {
        txOptions |= RAIL_TX_OPTION_WAIT_FOR_ACK;

#if RADIO_CONFIG_DMP_SUPPORT
        // time we wait for ACK
        if (RAIL_GetSymbolRate(gRailHandle) > 0)
        {
            txSchedulerInfo.transactionTime += 12 * 1e6 / RAIL_GetSymbolRate(gRailHandle);
        }
        else
        {
            txSchedulerInfo.transactionTime += 12 * RADIO_TIMING_DEFAULT_SYMBOLTIME_US;
        }
#endif
    }

#if RADIO_CONFIG_DMP_SUPPORT
    // time needed for the frame itself
    // 4B preamble, 1B SFD, 1B PHR is not counted in frameLength
    if (RAIL_GetBitRate(gRailHandle) > 0)
    {
        txSchedulerInfo.transactionTime = (frameLength + 4 + 1 + 1) * 8 * 1e6 / RAIL_GetBitRate(gRailHandle);
    }
    else
    { // assume 250kbps
        txSchedulerInfo.transactionTime = (frameLength + 4 + 1 + 1) * RADIO_TIMING_DEFAULT_BYTETIME_US;
    }
#endif

    if (aFrame->mInfo.mTxInfo.mCsmaCaEnabled)
    {
#if RADIO_CONFIG_DMP_SUPPORT
        // time needed for CSMA/CA
        txSchedulerInfo.transactionTime += RADIO_TIMING_CSMA_OVERHEAD_US;
#endif
        status =
            RAIL_StartCcaCsmaTx(sTxBandConfig->mRailHandle, aFrame->mChannel, txOptions, &csmaConfig, &txSchedulerInfo);
    }
    else
    {
        status = RAIL_StartTx(sTxBandConfig->mRailHandle, aFrame->mChannel, txOptions, &txSchedulerInfo);
    }

    if (status == RAIL_STATUS_NO_ERROR)
    {
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailTxStarted++;
#endif
        otPlatRadioTxStarted(aInstance, aFrame);
    }
    else
    {
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailTxStartFailed++;
#endif
        sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        sTransmitBusy  = false;
    }

exit:
    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    otError  error;
    uint32_t start;
    int8_t   rssi = OT_RADIO_RSSI_INVALID;

    OT_UNUSED_VARIABLE(aInstance);

    error = efr32StartEnergyScan(ENERGY_SCAN_MODE_SYNC, sReceiveFrame.mChannel, EFR32_RSSI_AVERAGING_TIME);
    otEXPECT(error == OT_ERROR_NONE);

    start = RAIL_GetTime();

    // waiting for the event RAIL_EVENT_RSSI_AVERAGE_DONE
    while (sEnergyScanStatus == ENERGY_SCAN_STATUS_IN_PROGRESS &&
           ((RAIL_GetTime() - start) < EFR32_RSSI_AVERAGING_TIMEOUT))
        ;

    if (sEnergyScanStatus == ENERGY_SCAN_STATUS_COMPLETED)
    {
        rssi = sEnergyScanResultDbm;
    }

    sEnergyScanStatus = ENERGY_SCAN_STATUS_IDLE;
exit:
    return rssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_ENERGY_SCAN;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    sPromiscuous = aEnable;

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        status = RAIL_IEEE802154_SetPromiscuousMode(sBandConfigs[i].mRailHandle, aEnable);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    // set Frame Pending bit for all outgoing ACKs if aEnable is false
    sIsSrcMatchEnabled = aEnable;
}

static void processNextRxPacket(otInstance *aInstance, RAIL_Handle_t aRailHandle)
{
    RAIL_RxPacketHandle_t  packetHandle = RAIL_RX_PACKET_HANDLE_INVALID;
    RAIL_RxPacketInfo_t    packetInfo;
    RAIL_RxPacketDetails_t packetDetails;
    RAIL_Status_t          status;
    uint16_t               length;

    packetHandle = RAIL_GetRxPacketInfo(aRailHandle, RAIL_RX_PACKET_HANDLE_OLDEST, &packetInfo);

    otEXPECT_ACTION(packetHandle != RAIL_RX_PACKET_HANDLE_INVALID &&
                        packetInfo.packetStatus == RAIL_RX_PACKET_READY_SUCCESS,
                    packetHandle = RAIL_RX_PACKET_HANDLE_INVALID);

    packetDetails.timeReceived.timePosition     = RAIL_PACKET_TIME_INVALID;
    packetDetails.timeReceived.totalPacketBytes = 0;
    status                                      = RAIL_GetRxPacketDetails(aRailHandle, packetHandle, &packetDetails);
    otEXPECT(status == RAIL_STATUS_NO_ERROR);

    length = packetInfo.packetBytes + 1;

    // check the length in recv packet info structure
    otEXPECT(length == packetInfo.firstPortionData[0]);

    // check the length validity of recv packet
    otEXPECT(length >= IEEE802154_MIN_LENGTH && length <= IEEE802154_MAX_LENGTH);

    otLogInfoPlat("Received data:%d", length);

    // skip length byte
    assert(packetInfo.firstPortionBytes > 0);
    packetInfo.firstPortionData++;
    packetInfo.firstPortionBytes--;
    packetInfo.packetBytes--;

    // read packet
    memcpy(sReceiveFrame.mPsdu, packetInfo.firstPortionData, packetInfo.firstPortionBytes);
    memcpy(sReceiveFrame.mPsdu + packetInfo.firstPortionBytes, packetInfo.lastPortionData,
           packetInfo.packetBytes - packetInfo.firstPortionBytes);

    status = RAIL_ReleaseRxPacket(aRailHandle, packetHandle);
    if (status == RAIL_STATUS_NO_ERROR)
    {
        packetHandle = RAIL_RX_PACKET_HANDLE_INVALID;
    }

    sReceiveFrame.mLength = length;

    if (packetDetails.isAck)
    {
        assert((length == IEEE802154_ACK_LENGTH) &&
               (sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK);

        RAIL_YieldRadio(gRailHandle);
        sTransmitBusy = false;

        if (sReceiveFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET])
        {
            sTransmitError = OT_ERROR_NONE;
        }
        else
        {
            sTransmitError = OT_ERROR_NO_ACK;
        }
    }
    else
    {
        otEXPECT(length != IEEE802154_ACK_LENGTH);

        sReceiveError = OT_ERROR_NONE;

        sReceiveFrame.mInfo.mRxInfo.mRssi = packetDetails.rssi;
        sReceiveFrame.mInfo.mRxInfo.mLqi  = packetDetails.lqi;

        // TODO: grab timestamp and handle conversion to msec/usec and RAIL_GetRxTimeSyncWordEndAlt
        // sReceiveFrame.mInfo.mRxInfo.mMsec = packetDetails.packetTime;
        // sReceiveFrame.mInfo.mRxInfo.mUsec = packetDetails.packetTime;

        // TODO Set this flag only when the packet is really acknowledged with frame pending set.
        // See https://github.com/openthread/openthread/pull/3785
        sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = true;

#if OPENTHREAD_CONFIG_DIAG_ENABLE

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
                otLogInfoPlat("Received %d bytes", sReceiveFrame.mLength);
                otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
                sRailDebugCounters.mRailPlatRadioReceiveDoneCbCount++;
#endif
            }
        }
    }

    otSysEventSignalPending();

exit:

    if (packetHandle != RAIL_RX_PACKET_HANDLE_INVALID)
    {
        RAIL_ReleaseRxPacket(aRailHandle, packetHandle);
    }
}

static void ieee802154DataRequestCommand(RAIL_Handle_t aRailHandle)
{
    RAIL_Status_t status;

    if (sIsSrcMatchEnabled)
    {
        RAIL_IEEE802154_Address_t sourceAddress;

        status = RAIL_IEEE802154_GetAddress(aRailHandle, &sourceAddress);
        assert(status == RAIL_STATUS_NO_ERROR);

        if ((sourceAddress.length == RAIL_IEEE802154_LongAddress &&
             utilsSoftSrcMatchExtFindEntry((otExtAddress *)sourceAddress.longAddress) >= 0) ||
            (sourceAddress.length == RAIL_IEEE802154_ShortAddress &&
             utilsSoftSrcMatchShortFindEntry(sourceAddress.shortAddress) >= 0))
        {
            status = RAIL_IEEE802154_SetFramePending(aRailHandle);
            assert(status == RAIL_STATUS_NO_ERROR);
        }
    }
    else
    {
        status = RAIL_IEEE802154_SetFramePending(aRailHandle);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

static void RAILCb_Generic(RAIL_Handle_t aRailHandle, RAIL_Events_t aEvents)
{
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    if (aEvents & RAIL_EVENT_CONFIG_SCHEDULED)
    {
        sRailDebugCounters.mRailEventConfigScheduled++;
    }
    if (aEvents & RAIL_EVENT_CONFIG_UNSCHEDULED)
    {
        sRailDebugCounters.mRailEventConfigUnScheduled++;
    }
#endif
    if (aEvents & RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND)
    {
        ieee802154DataRequestCommand(aRailHandle);
    }
    if (aEvents & RAIL_EVENTS_TX_COMPLETION)
    {
        if (aEvents & RAIL_EVENT_TX_PACKET_SENT)
        {
            if ((sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0)
            {
                RAIL_YieldRadio(aRailHandle);
                sTransmitError = OT_ERROR_NONE;
                sTransmitBusy  = false;
            }
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventPacketSent++;
#endif
        }
        else if (aEvents & RAIL_EVENT_TX_CHANNEL_BUSY)
        {
            RAIL_YieldRadio(aRailHandle);
            sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            sTransmitBusy  = false;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventChannelBusy++;
#endif
        }
        else
        {
            RAIL_YieldRadio(aRailHandle);
            sTransmitError = OT_ERROR_ABORT;
            sTransmitBusy  = false;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventTxAbort++;
#endif
        }
    }

    if (aEvents & RAIL_EVENT_RX_ACK_TIMEOUT)
    {
        RAIL_YieldRadio(aRailHandle);
        sTransmitError = OT_ERROR_NO_ACK;
        sTransmitBusy  = false;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventNoAck++;
#endif
    }

    if (aEvents & RAIL_EVENT_RX_PACKET_RECEIVED)
    {
        RAIL_HoldRxPacket(aRailHandle);
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventPacketReceived++;
#endif
    }

    if (aEvents & RAIL_EVENT_CAL_NEEDED)
    {
        RAIL_Status_t status;

        status = RAIL_Calibrate(aRailHandle, NULL, RAIL_CAL_ALL_PENDING);
        assert(status == RAIL_STATUS_NO_ERROR);

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventCalNeeded++;
#endif
    }

    if (aEvents & RAIL_EVENT_RSSI_AVERAGE_DONE)
    {
        const int16_t energyScanResultQuarterDbm = RAIL_GetAverageRssi(aRailHandle);
        RAIL_YieldRadio(aRailHandle);

        sEnergyScanStatus = ENERGY_SCAN_STATUS_COMPLETED;

        if (energyScanResultQuarterDbm == RAIL_RSSI_INVALID)
        {
            sEnergyScanResultDbm = OT_RADIO_RSSI_INVALID;
        }
        else
        {
            sEnergyScanResultDbm = energyScanResultQuarterDbm / QUARTER_DBM_IN_DBM;
        }

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailPlatRadioEnergyScanDoneCbCount++;
#endif
    }
    if (aEvents & RAIL_EVENT_SCHEDULER_STATUS)
    {
        RAIL_SchedulerStatus_t status = RAIL_GetSchedulerStatus(aRailHandle);

        assert(status != RAIL_SCHEDULER_STATUS_INTERNAL_ERROR);

        if (status == RAIL_SCHEDULER_STATUS_CCA_CSMA_TX_FAIL || status == RAIL_SCHEDULER_STATUS_SINGLE_TX_FAIL ||
            status == RAIL_SCHEDULER_STATUS_SCHEDULED_TX_FAIL ||
            (status == RAIL_SCHEDULER_STATUS_SCHEDULE_FAIL && sTransmitBusy) ||
            (status == RAIL_SCHEDULER_STATUS_EVENT_INTERRUPTED && sTransmitBusy))
        {
            sTransmitError = OT_ERROR_ABORT;
            sTransmitBusy  = false;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventSchedulerStatusError++;
#endif
        }
        else if (status == RAIL_SCHEDULER_STATUS_AVERAGE_RSSI_FAIL)
        {
            sEnergyScanStatus    = ENERGY_SCAN_STATUS_COMPLETED;
            sEnergyScanResultDbm = OT_RADIO_RSSI_INVALID;
        }
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        else if (sTransmitBusy)
        {
            sRailDebugCounters.mRailEventsSchedulerStatusLastStatus = status;
            sRailDebugCounters.mRailEventsSchedulerStatusTransmitBusy++;
        }
#endif
    }

    otSysEventSignalPending();
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    return efr32StartEnergyScan(ENERGY_SCAN_MODE_ASYNC, aScanChannel, (RAIL_Time_t)aScanDuration * US_IN_MS);
}

void efr32RadioProcess(otInstance *aInstance)
{
    if (sState == OT_RADIO_STATE_TRANSMIT && sTransmitBusy == false)
    {
        if (sTransmitError != OT_ERROR_NONE)
        {
            otLogDebgPlat("Transmit failed ErrorCode=%d", sTransmitError);
        }

        sState = OT_RADIO_STATE_RECEIVE;
#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, sTransmitError);
        }
        else
#endif
            if (((sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST) == 0) || (sTransmitError != OT_ERROR_NONE))
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
        }
        else
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveFrame, sTransmitError);
        }

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailPlatRadioTxDoneCbCount++;
#endif

        otSysEventSignalPending();
    }
    else if (sEnergyScanMode == ENERGY_SCAN_MODE_ASYNC && sEnergyScanStatus == ENERGY_SCAN_STATUS_COMPLETED)
    {
        sEnergyScanStatus = ENERGY_SCAN_STATUS_IDLE;
        otPlatRadioEnergyScanDone(aInstance, sEnergyScanResultDbm);
        otSysEventSignalPending();

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventEnergyScanCompleted++;
#endif
    }

    processNextRxPacket(aInstance, sRxBandConfig->mRailHandle);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sTxPowerDbm;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    for (uint8_t i = 0; i < EFR32_NUM_BAND_CONFIGS; i++)
    {
        status = RAIL_SetTxPowerDbm(sBandConfigs[i].mRailHandle, ((RAIL_TxPower_t)aPower) * 10);
        assert(status == RAIL_STATUS_NO_ERROR);
    }

    sTxPowerDbm = aPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aThreshold);

    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return EFR32_RECEIVE_SENSITIVITY;
}
