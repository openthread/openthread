/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
#include <openthread/link.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "common/logging.hpp"
#include "utils/code_utils.h"
#include "utils/mac_frame.h"

#include "utils/soft_source_match_table.h"

#include "antenna.h"
#include "board_config.h"
#include "em_core.h"
#include "em_system.h"
#include "ieee802154mac.h"
#include "openthread-core-efr32-config.h"
#include "pa_conversions_efr32.h"
#include "platform-band.h"
#include "rail.h"
#include "rail_config.h"
#include "rail_ieee802154.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
#include "sl_rail_util_ant_div.h"
#include "sl_rail_util_ant_div_config.h"
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
#include "coexistence-802154.h"
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
#include "sl_rail_util_ieee802154_stack_event.h"
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT
#include "sl_rail_util_ieee802154_phy_select.h"
#endif // #ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT
//------------------------------------------------------------------------------
// Enums, macros and static variables

#define LOW_BYTE(n) ((uint8_t)((n)&0xFF))
#define HIGH_BYTE(n) ((uint8_t)(LOW_BYTE((n) >> 8)))

#define EFR32_RECEIVE_SENSITIVITY -100   // dBm
#define EFR32_RSSI_AVERAGING_TIME 16     // us
#define EFR32_RSSI_AVERAGING_TIMEOUT 300 // us

// Internal flags
#define FLAG_RADIO_INIT_DONE 0x0001
#define FLAG_ONGOING_TX_DATA 0x0002
#define FLAG_ONGOING_TX_ACK 0x0004
#define FLAG_WAITING_FOR_ACK 0x0008
#define FLAG_SYMBOL_TIMER_RUNNING 0x0010 // Not used
#define FLAG_CURRENT_TX_USE_CSMA 0x0020
#define FLAG_DATA_POLL_FRAME_PENDING_SET 0x0040
#define FLAG_CALIBRATION_NEEDED 0x0080 // Not used
#define FLAG_IDLE_PENDING 0x0100       // Not used

#define TX_COMPLETE_RESULT_SUCCESS 0x00 // Not used
#define TX_COMPLETE_RESULT_CCA_FAIL 0x01
#define TX_COMPLETE_RESULT_OTHER_FAIL 0x02
#define TX_COMPLETE_RESULT_NONE 0xFF // Not used

#define TX_WAITING_FOR_ACK 0x00
#define TX_NO_ACK 0x01

#define ONGOING_TX_FLAGS (FLAG_ONGOING_TX_DATA | FLAG_ONGOING_TX_ACK)

#define QUARTER_DBM_IN_DBM 4
#define US_IN_MS 1000

// Energy Scan
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

static volatile energyScanStatus sEnergyScanStatus;
static volatile int8_t           sEnergyScanResultDbm;
static energyScanMode            sEnergyScanMode;

static bool sIsSrcMatchEnabled = false;

// Receive
static uint8_t      sReceivePsdu[IEEE802154_MAX_LENGTH];
static uint8_t      sReceiveAckPsdu[IEEE802154_ACK_LENGTH];
static otRadioFrame sReceiveFrame;
static otRadioFrame sReceiveAckFrame;
static otError      sReceiveError;

// Transmit
static otRadioFrame     sTransmitFrame;
static uint8_t          sTransmitPsdu[IEEE802154_MAX_LENGTH];
static volatile otError sTransmitError;
static volatile bool    sTransmitBusy = false;
static otRadioFrame *   sTxFrame      = NULL;

// Radio
#define CCA_THRESHOLD_UNINIT 127
#define CCA_THRESHOLD_DEFAULT -75 // dBm  - default for 2.4GHz 802.15.4

static bool         sPromiscuous = false;
static otRadioState sState       = OT_RADIO_STATE_DISABLED;

static efr32CommonConfig sCommonConfig;
static efr32BandConfig   sBandConfig;
static efr32BandConfig * sCurrentBandConfig = NULL;

static int8_t sCcaThresholdDbm = CCA_THRESHOLD_DEFAULT;

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
static efr32RadioCounters sRailDebugCounters;
#endif

// RAIL
RAIL_Handle_t emPhyRailHandle;
#define gRailHandle emPhyRailHandle

static const RAIL_IEEE802154_Config_t sRailIeee802154Config = {
    NULL, // addresses
    {
        // ackConfig
        true, // ackConfig.enable
        672,  // ackConfig.ackTimeout
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
    false,                                  // defaultFramePendingInOutgoingAcks
};

// Misc
static volatile uint16_t miscInternalFlags = 0;
static bool              emPendingData     = false;

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
enum
{
    RHO_INACTIVE = 0,
    RHO_EXT_ACTIVE,
    RHO_INT_ACTIVE, // Not used
    RHO_BOTH_ACTIVE,
};

static uint8_t rhoActive = RHO_INACTIVE;
static bool    ptaGntEventReported;
static bool    sRadioCoexEnabled = true;

#if SL_OPENTHREAD_COEX_COUNTER_ENABLE
static uint32_t sCoexCounters[SL_RAIL_UTIL_COEX_EVENT_COUNT] = {0};
#endif // SL_OPENTHREAD_COEX_COUNTER_ENABLE

#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

// Enhanced Acks and CSL
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
static otExtAddress  sExtAddress;
static otRadioIeInfo sTransmitIeInfo;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint8_t sAckIeData[OT_ACK_IE_MAX_SIZE];
static uint8_t sAckIeDataLength = 0;

static uint32_t      sCslPeriod;
static uint32_t      sCslSampleTime;
static const uint8_t sCslIeHeader[OT_IE_HEADER_SIZE] = {CSL_IE_HEADER_BYTES_LO, CSL_IE_HEADER_BYTES_HI};

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

static uint32_t        sMacFrameCounter;
static uint8_t         sKeyId;
static struct otMacKey sPrevKey;
static struct otMacKey sCurrKey;
static struct otMacKey sNextKey;
static bool            sAckedWithSecEnhAck;
static uint32_t        sAckFrameCounter;
static uint8_t         sAckKeyId;

static void processSecurityForEnhancedAck(uint8_t *aAckFrame)
{
    otRadioFrame     ackFrame;
    struct otMacKey *key = NULL;
    uint8_t          keyId;

    sAckedWithSecEnhAck = false;
    otEXPECT(aAckFrame[1] & IEEE802154_FRAME_FLAG_SECURITY_ENABLED);

    memset(&ackFrame, 0, sizeof(ackFrame));
    ackFrame.mPsdu   = &aAckFrame[1];
    ackFrame.mLength = aAckFrame[0];

    keyId = otMacFrameGetKeyId(&ackFrame);

    otEXPECT(otMacFrameIsKeyIdMode1(&ackFrame) && keyId != 0);

    if (keyId == sKeyId)
    {
        key = &sCurrKey;
    }
    else if (keyId == sKeyId - 1)
    {
        key = &sPrevKey;
    }
    else if (keyId == sKeyId + 1)
    {
        key = &sNextKey;
    }
    else
    {
        otEXPECT(false);
    }

    sAckFrameCounter    = sMacFrameCounter;
    sAckKeyId           = keyId;
    sAckedWithSecEnhAck = true;

    ackFrame.mInfo.mTxInfo.mAesKey = key;

    otMacFrameSetKeyId(&ackFrame, keyId);
    otMacFrameSetFrameCounter(&ackFrame, sMacFrameCounter++);

    // Perform AES-CCM encryption on the frame which is going to be sent.
    otMacFrameProcessTransmitAesCcm(&ackFrame, &sExtAddress);

exit:
    return;
}
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

//------------------------------------------------------------------------------
// Forward Declarations

static void RAILCb_Generic(RAIL_Handle_t aRailHandle, RAIL_Events_t aEvents);

static void efr32PhyStackInit(void);

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
static void efr32CoexInit(void);
// Try to transmit the current outgoing frame subject to MAC-level PTA
static void tryTxCurrentPacket(void);
#else
// Transmit the current outgoing frame.
void        txCurrentPacket(void);
#define tryTxCurrentPacket txCurrentPacket
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

static void txFailedCallback(bool isAck, uint8_t status);

static bool validatePacketDetails(RAIL_RxPacketHandle_t   packetHandle,
                                  RAIL_RxPacketDetails_t *pPacketDetails,
                                  RAIL_RxPacketInfo_t *   pPacketInfo,
                                  uint16_t *              packetLength);
static bool validatePacketTimestamp(RAIL_RxPacketDetails_t *pPacketDetails, uint16_t packetLength);
static void updateRxFrameDetails(RAIL_RxPacketDetails_t *pPacketDetails, bool framePendingSetInOutgoingAck);

//------------------------------------------------------------------------------
// Helper Functions

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

static bool phyStackEventIsEnabled(void)
{
    bool result = false;

#if (defined(SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT) && SL_RAIL_UTIL_ANT_DIV_RX_RUNTIME_PHY_SELECT)
    result = true;
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
    result |= (sl_rail_util_coex_is_enabled() && sRadioCoexEnabled);
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

    return result;
}

static RAIL_Events_t currentEventConfig = RAIL_EVENTS_NONE;
static void          updateEvents(RAIL_Events_t mask, RAIL_Events_t values)
{
    RAIL_Status_t status;
    RAIL_Events_t newEventConfig = (currentEventConfig & ~mask) | (values & mask);
    if (newEventConfig != currentEventConfig)
    {
        currentEventConfig = newEventConfig;
        status             = RAIL_ConfigEvents(gRailHandle, mask, values);
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

static sl_rail_util_ieee802154_stack_event_t handlePhyStackEvent(sl_rail_util_ieee802154_stack_event_t stackEvent,
                                                                 uint32_t                              supplement)
{
    return (phyStackEventIsEnabled() ? sl_rail_util_ieee802154_on_event(stackEvent, supplement) : 0);
}
#else
static void updateEvents(RAIL_Events_t mask, RAIL_Events_t values)
{
    RAIL_Status_t status;
    status = RAIL_ConfigEvents(gRailHandle, mask, values);
    assert(status == RAIL_STATUS_NO_ERROR);
}

#define handlePhyStackEvent(event, supplement) 0
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

// Set or clear the passed flag.
static inline void setInternalFlag(uint16_t flag, bool val)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    miscInternalFlags = (val ? (miscInternalFlags | flag) : (miscInternalFlags & ~flag));
    CORE_EXIT_ATOMIC();
}
// Returns true if the passed flag is set, false otherwise.
static inline bool getInternalFlag(uint16_t flag)
{
    return ((miscInternalFlags & flag) != 0);
}

static inline bool txWaitingForAck(void)
{
    return (sTransmitBusy == true && ((sTransmitFrame.mPsdu[0] & IEEE802154_FRAME_FLAG_ACK_REQUIRED) != 0));
}

static bool txIsDataRequest(void)
{
    uint16_t fcf = sTransmitFrame.mPsdu[IEEE802154_FCF_OFFSET] | (sTransmitFrame.mPsdu[IEEE802154_FCF_OFFSET + 1] << 8);

    return (sTransmitBusy == true && (fcf & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_COMMAND);
}

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
static inline bool isReceivingFrame(void)
{
    return (RAIL_GetRadioState(gRailHandle) & RAIL_RF_STATE_RX_ACTIVE) == RAIL_RF_STATE_RX_ACTIVE;
}
#endif

static void radioSetIdle(void)
{
    if (RAIL_GetRadioState(gRailHandle) != RAIL_RF_STATE_IDLE)
    {
        RAIL_Idle(gRailHandle, RAIL_IDLE, true);
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_IDLED, 0U);
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_IDLED, 0U);
    }
    RAIL_YieldRadio(gRailHandle);
    sState = OT_RADIO_STATE_SLEEP;
}

static otError radioSetRx(uint8_t aChannel)
{
    otError       error = OT_ERROR_NONE;
    RAIL_Status_t status;

    RAIL_SchedulerInfo_t bgRxSchedulerInfo = {
        .priority = RADIO_SCHEDULER_BACKGROUND_RX_PRIORITY,
        // sliptime/transaction time is not used for bg rx
    };

    status = RAIL_StartRx(gRailHandle, aChannel, &bgRxSchedulerInfo);
    otEXPECT_ACTION(status == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

    (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_LISTEN, 0U);
    sState = OT_RADIO_STATE_RECEIVE;

    otLogInfoPlat("State=OT_RADIO_STATE_RECEIVE", NULL);
exit:
    return error;
}

//------------------------------------------------------------------------------
// Radio Initialization

static RAIL_Handle_t efr32RailInit(efr32CommonConfig *aCommonConfig)
{
    RAIL_Status_t status;
    RAIL_Handle_t handle;

    handle = RAIL_Init(&aCommonConfig->mRailConfig, NULL);
    assert(handle != NULL);

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    status = RAIL_InitPowerManager();
    assert(status == RAIL_STATUS_NO_ERROR);
#endif // SL_CATALOG_POWER_MANAGER_PRESENT

    status = RAIL_ConfigCal(handle, RAIL_CAL_ALL);
    assert(status == RAIL_STATUS_NO_ERROR);

    status = RAIL_IEEE802154_Init(handle, &sRailIeee802154Config);
    assert(status == RAIL_STATUS_NO_ERROR);

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    // Enhanced Frame Pending
    // status = RAIL_IEEE802154_EnableEarlyFramePending(handle, true);
    // assert(status == RAIL_STATUS_NO_ERROR);

    // status = RAIL_IEEE802154_EnableDataFramePending(handle, true);
    // assert(status == RAIL_STATUS_NO_ERROR);

    // Enhanced ACKs (only on platforms that support it, so error checking is disabled)
    // RAIL_IEEE802154_ConfigEOptions(handle,
    //                                (RAIL_IEEE802154_E_OPTION_GB868 | RAIL_IEEE802154_E_OPTION_ENH_ACK),
    //                                (RAIL_IEEE802154_E_OPTION_GB868 | RAIL_IEEE802154_E_OPTION_ENH_ACK));
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

    uint16_t actualLenth = RAIL_SetTxFifo(handle, aCommonConfig->mRailTxFifo, 0, sizeof(aCommonConfig->mRailTxFifo));
    assert(actualLenth == sizeof(aCommonConfig->mRailTxFifo));

    return handle;
}

static void efr32RailConfigLoad(efr32BandConfig *aBandConfig)
{
    RAIL_Status_t        status;
    RAIL_TxPowerConfig_t txPowerConfig = {SL_RAIL_UTIL_PA_SELECTION_2P4GHZ, SL_RAIL_UTIL_PA_VOLTAGE_MV, 10};

    if (aBandConfig->mChannelConfig != NULL)
    {
        uint16_t firstChannel = RAIL_ConfigChannels(gRailHandle, aBandConfig->mChannelConfig, NULL);
        assert(firstChannel == aBandConfig->mChannelMin);

        // txPowerConfig.mode = RAIL_TX_POWER_MODE_SUBGIG; TO DO:Check this macro
    }
    else
    {
#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT
        status = sl_rail_util_plugin_config_2p4ghz_radio(gRailHandle);
#else
        status = RAIL_IEEE802154_Config2p4GHzRadio(gRailHandle);
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT
        assert(status == RAIL_STATUS_NO_ERROR);
    }

    status = RAIL_ConfigTxPower(gRailHandle, &txPowerConfig);
    assert(status == RAIL_STATUS_NO_ERROR);
}

static void efr32RadioSetTxPower(int8_t aPowerDbm)
{
    RAIL_Status_t status;
    sl_rail_util_pa_init();

    status = RAIL_SetTxPowerDbm(gRailHandle, ((RAIL_TxPower_t)aPowerDbm) * 10);
    assert(status == RAIL_STATUS_NO_ERROR);
}

static efr32BandConfig *efr32RadioGetBandConfig(uint8_t aChannel)
{
    efr32BandConfig *config = NULL;

    if ((sBandConfig.mChannelMin <= aChannel) && (aChannel <= sBandConfig.mChannelMax))
    {
        config = &sBandConfig;
    }

    return config;
}

static void efr32ConfigInit(void (*aEventCallback)(RAIL_Handle_t railHandle, RAIL_Events_t events))
{
    sCommonConfig.mRailConfig.eventsCallback = aEventCallback;
    sCommonConfig.mRailConfig.protocol       = NULL; // only used by Bluetooth stack
#if RADIO_CONFIG_DMP_SUPPORT
    sCommonConfig.mRailConfig.scheduler = &(sCommonConfig.mRailSchedState);
#else
    sCommonConfig.mRailConfig.scheduler = NULL; // only needed for DMP
#endif

#if RADIO_CONFIG_2P4GHZ_OQPSK_SUPPORT
    sBandConfig.mChannelConfig = NULL;
    sBandConfig.mChannelMin    = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN;
    sBandConfig.mChannelMax    = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX;

#endif

#if RADIO_CONFIG_915MHZ_OQPSK_SUPPORT
    sBandConfig.mChannelConfig = channelConfigs[0]; // TO DO: channel config??
    sBandConfig.mChannelMin    = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN;
    sBandConfig.mChannelMax    = OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX;
#endif

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    memset(&sRailDebugCounters, 0x00, sizeof(efr32RadioCounters));
#endif

    gRailHandle = efr32RailInit(&sCommonConfig);
    assert(gRailHandle != NULL);

    updateEvents(RAIL_EVENTS_ALL,
                 (0 | RAIL_EVENT_RX_ACK_TIMEOUT | RAIL_EVENT_RX_PACKET_RECEIVED | RAIL_EVENTS_TXACK_COMPLETION |
                  RAIL_EVENTS_TX_COMPLETION | RAIL_EVENT_RSSI_AVERAGE_DONE | RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT || RADIO_CONFIG_DMP_SUPPORT
                  | RAIL_EVENT_CONFIG_SCHEDULED | RAIL_EVENT_CONFIG_UNSCHEDULED | RAIL_EVENT_SCHEDULER_STATUS
#endif
                  | RAIL_EVENT_CAL_NEEDED));

    efr32RailConfigLoad(&(sBandConfig));
}

void efr32RadioInit(void)
{
    if (getInternalFlag(FLAG_RADIO_INIT_DONE))
    {
        return;
    }
    RAIL_Status_t status;

    // check if RAIL_TX_FIFO_SIZE is power of two..
    assert((RAIL_TX_FIFO_SIZE & (RAIL_TX_FIFO_SIZE - 1)) == 0);

    // check the limits of the RAIL_TX_FIFO_SIZE.
    assert((RAIL_TX_FIFO_SIZE >= 64) || (RAIL_TX_FIFO_SIZE <= 4096));

    efr32ConfigInit(RAILCb_Generic);
    setInternalFlag(FLAG_RADIO_INIT_DONE, true);

    status = RAIL_ConfigSleep(gRailHandle, RAIL_SLEEP_CONFIG_TIMERSYNC_ENABLED);
    assert(status == RAIL_STATUS_NO_ERROR);

    sReceiveFrame.mLength    = 0;
    sReceiveFrame.mPsdu      = sReceivePsdu;
    sReceiveAckFrame.mLength = 0;
    sReceiveAckFrame.mPsdu   = sReceiveAckPsdu;
    sTransmitFrame.mLength   = 0;
    sTransmitFrame.mPsdu     = sTransmitPsdu;

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = &sTransmitIeInfo;
#endif
#endif

    sCurrentBandConfig = efr32RadioGetBandConfig(OPENTHREAD_CONFIG_DEFAULT_CHANNEL);
    assert(sCurrentBandConfig != NULL);

    efr32RadioSetTxPower(OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER);

    assert(RAIL_ConfigRxOptions(gRailHandle, RAIL_RX_OPTION_TRACK_ABORTED_FRAMES,
                                RAIL_RX_OPTION_TRACK_ABORTED_FRAMES) == RAIL_STATUS_NO_ERROR);
    efr32PhyStackInit();

    sEnergyScanStatus = ENERGY_SCAN_STATUS_IDLE;
    sTransmitError    = OT_ERROR_NONE;
    sTransmitBusy     = false;

    otLogInfoPlat("Initialized", NULL);
}

void efr32RadioDeinit(void)
{
    RAIL_Status_t status;

    RAIL_Idle(gRailHandle, RAIL_IDLE_ABORT, true);
    status = RAIL_ConfigEvents(gRailHandle, RAIL_EVENTS_ALL, 0);
    assert(status == RAIL_STATUS_NO_ERROR);

    sCurrentBandConfig = NULL;
}

//------------------------------------------------------------------------------
// Energy Scan support

static void energyScanComplete(int8_t scanResultDbm)
{
    sEnergyScanResultDbm = scanResultDbm;
    sEnergyScanStatus    = ENERGY_SCAN_STATUS_COMPLETED;
}

static otError efr32StartEnergyScan(energyScanMode aMode, uint16_t aChannel, RAIL_Time_t aAveragingTimeUs)
{
    RAIL_Status_t    status = RAIL_STATUS_NO_ERROR;
    otError          error  = OT_ERROR_NONE;
    efr32BandConfig *config = NULL;

    otEXPECT_ACTION(sEnergyScanStatus == ENERGY_SCAN_STATUS_IDLE, error = OT_ERROR_BUSY);

    sEnergyScanStatus = ENERGY_SCAN_STATUS_IN_PROGRESS;
    sEnergyScanMode   = aMode;

    RAIL_Idle(gRailHandle, RAIL_IDLE, true);

    config = efr32RadioGetBandConfig(aChannel);
    otEXPECT_ACTION(config != NULL, error = OT_ERROR_INVALID_ARGS);

    if (sCurrentBandConfig != config)
    {
        efr32RailConfigLoad(config);
        sCurrentBandConfig = config;
    }

    RAIL_SchedulerInfo_t scanSchedulerInfo = {.priority        = RADIO_SCHEDULER_CHANNEL_SCAN_PRIORITY,
                                              .slipTime        = RADIO_SCHEDULER_CHANNEL_SLIP_TIME,
                                              .transactionTime = aAveragingTimeUs};

    status = RAIL_StartAverageRssi(gRailHandle, aChannel, aAveragingTimeUs, &scanSchedulerInfo);
    otEXPECT_ACTION(status == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

exit:
    if (status != RAIL_STATUS_NO_ERROR)
    {
        energyScanComplete(OT_RADIO_RSSI_INVALID);
    }
    return error;
}

//------------------------------------------------------------------------------
// Enhanced Acks and CSL support

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

static uint16_t getCslPhase()
{
    uint32_t curTime       = otPlatAlarmMicroGetNow();
    uint32_t cslPeriodInUs = sCslPeriod * OT_US_PER_TEN_SYMBOLS;
    uint32_t diff = ((sCslSampleTime % cslPeriodInUs) - (curTime % cslPeriodInUs) + cslPeriodInUs) % cslPeriodInUs;

    return (uint16_t)(diff / OT_US_PER_TEN_SYMBOLS);
}

static void updateIeData(void)
{
    // The CSL IE Content field:
    //  ___________________________________________________
    // |   Octets: 2  |   Octets: 2  |     Octets: 0/2     |
    // |______________|______________|_____________________|
    // |   CSL Phase  |   CSL Period |   Rendezvous time   |
    // |______________|______________|_____________________|
    //
    // Note: The rendezvous time is included right when sending the packet,
    // (in txCurrentPacket), before updating the 802.15.4 header with CSL IEs.
    // The tx frame is modified at the right offset (see mInfo.mTxInfo.mIeInfo->mTimeIeOffset)

    int8_t offset = 0;
    if (sCslPeriod > 0)
    {
        uint8_t *finger = sAckIeData;
        memcpy(finger, sCslIeHeader, OT_IE_HEADER_SIZE);
        finger += OT_IE_HEADER_SIZE;

        uint16_t cslPhase = getCslPhase();
        *finger++         = HIGH_BYTE(cslPhase);
        *finger++         = LOW_BYTE(cslPhase);

        *finger++ = HIGH_BYTE((uint16_t)sCslPeriod);
        *finger++ = LOW_BYTE((uint16_t)sCslPeriod);

        offset = finger - sAckIeData;
    }

    sAckIeDataLength = offset;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

//------------------------------------------------------------------------------
// Stack support

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

    status = RAIL_IEEE802154_SetPanId(gRailHandle, aPanId, 0);
    assert(status == RAIL_STATUS_NO_ERROR);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    for (size_t i = 0; i < sizeof(*aAddress); i++)
    {
        sExtAddress.m8[i] = aAddress->m8[sizeof(*aAddress) - 1 - i];
    }
#endif
#endif

    RAIL_Status_t status;

    otLogInfoPlat("ExtAddr=%X%X%X%X%X%X%X%X", aAddress->m8[7], aAddress->m8[6], aAddress->m8[5], aAddress->m8[4],
                  aAddress->m8[3], aAddress->m8[2], aAddress->m8[1], aAddress->m8[0]);

    status = RAIL_IEEE802154_SetLongAddress(gRailHandle, (uint8_t *)aAddress->m8, 0);
    assert(status == RAIL_STATUS_NO_ERROR);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    otLogInfoPlat("ShortAddr=%X", aAddress);

    status = RAIL_IEEE802154_SetShortAddress(gRailHandle, aAddress, 0);
    assert(status == RAIL_STATUS_NO_ERROR);
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
    radioSetIdle();
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

    if (sCurrentBandConfig != config)
    {
        RAIL_Idle(gRailHandle, RAIL_IDLE, true);
        efr32RailConfigLoad(config);
        sCurrentBandConfig = config;
    }

    status = radioSetRx(aChannel);
    otEXPECT_ACTION(status == RAIL_STATUS_NO_ERROR, error = OT_ERROR_FAILED);

    sReceiveFrame.mChannel    = aChannel;
    sReceiveAckFrame.mChannel = aChannel;

exit:
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError          error = OT_ERROR_NONE;
    efr32BandConfig *config;

    otEXPECT_ACTION((sState != OT_RADIO_STATE_DISABLED) && (sState != OT_RADIO_STATE_TRANSMIT),
                    error = OT_ERROR_INVALID_STATE);

    config = efr32RadioGetBandConfig(aFrame->mChannel);
    otEXPECT_ACTION(config != NULL, error = OT_ERROR_INVALID_ARGS);
    if (sCurrentBandConfig != config)
    {
        RAIL_Idle(gRailHandle, RAIL_IDLE, true);
        efr32RailConfigLoad(config);
        sCurrentBandConfig = config;
    }

    assert(sTransmitBusy == false);
    sState         = OT_RADIO_STATE_TRANSMIT;
    sTransmitError = OT_ERROR_NONE;
    sTransmitBusy  = true;
    sTxFrame       = aFrame;

    setInternalFlag(FLAG_CURRENT_TX_USE_CSMA, aFrame->mInfo.mTxInfo.mCsmaCaEnabled);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    setInternalFlag(FLAG_ONGOING_TX_DATA, true);
    tryTxCurrentPacket();
    CORE_EXIT_ATOMIC();

    if (sTransmitError == OT_ERROR_NONE)
    {
        otPlatRadioTxStarted(aInstance, aFrame);
    }
exit:
    return error;
}

void txCurrentPacket(void)
{
    assert(getInternalFlag(FLAG_ONGOING_TX_DATA));
    assert(sTxFrame != NULL);

    RAIL_CsmaConfig_t csmaConfig = RAIL_CSMA_CONFIG_802_15_4_2003_2p4_GHz_OQPSK_CSMA;
    RAIL_TxOptions_t  txOptions  = RAIL_TX_OPTIONS_DEFAULT;
    RAIL_Status_t     status;
    uint8_t           frameLength;
    bool              ackRequested;

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    sRailDebugCounters.mRailPlatTxTriggered++;
#endif

    // signalling this event earlier, as this event can assert REQ (expecially for a
    // non-CSMA transmit) giving the Coex master a little more time to grant or deny.
    if (getInternalFlag(FLAG_CURRENT_TX_USE_CSMA))
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_PENDED_PHY, (uint32_t) true);
    }
    else
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_PENDED_PHY, (uint32_t) false);
    }

    frameLength = (uint8_t)sTxFrame->mLength;
    RAIL_WriteTxFifo(gRailHandle, &frameLength, sizeof frameLength, true);
    RAIL_WriteTxFifo(gRailHandle, sTxFrame->mPsdu, frameLength - 2, false);

    RAIL_SchedulerInfo_t txSchedulerInfo = {
        .priority        = RADIO_SCHEDULER_TX_PRIORITY,
        .slipTime        = RADIO_SCHEDULER_CHANNEL_SLIP_TIME,
        .transactionTime = 0, // will be calculated later if DMP is used
    };

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // Update IE data in the 802.15.4 header with the newest CSL period / phase
    if (sCslPeriod > 0)
    {
        otMacFrameSetCslIe(sTxFrame, (uint16_t)sCslPeriod, getCslPhase());
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Seek the time sync offset and update the rendezvous time
    if (aFrame->mInfo.mTxInfo.mIeInfo->mTimeIeOffset != 0)
    {
        uint8_t *timeIe = aFrame->mPsdu + aFrame->mInfo.mTxInfo.mIeInfo->mTimeIeOffset;
        uint64_t time   = otPlatTimeGet() + aFrame->mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset;

        *timeIe = aFrame->mInfo.mTxInfo.mIeInfo->mTimeSyncSeq;

        *(++timeIe) = (uint8_t)(time & 0xff);
        for (uint8_t i = 1; i < sizeof(uint64_t); i++)
        {
            time        = time >> 8;
            *(++timeIe) = (uint8_t)(time & 0xff);
        }
    }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

    ackRequested = (sTxFrame->mPsdu[0] & IEEE802154_FRAME_FLAG_ACK_REQUIRED);
    if (ackRequested)
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

#ifdef SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
    // Update Tx options to use currently-selected antenna.
    // If antenna diverisity on Tx is disabled, leave both options 0
    // so Tx antenna tracks Rx antenna.
    if (sl_rail_util_ant_div_get_antenna_mode() != SL_RAIL_UTIL_ANT_DIV_DISABLED)
    {
        txOptions |= ((sl_rail_util_ant_div_get_antenna_selected() == SL_RAIL_UTIL_ANTENNA_SELECT_ANTENNA1)
                          ? RAIL_TX_OPTION_ANTENNA0
                          : RAIL_TX_OPTION_ANTENNA1);
    }
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT

#if RADIO_CONFIG_DMP_SUPPORT
    // time needed for the frame itself
    // 4B preamble, 1B SFD, 1B PHR is not counted in frameLength
    if (RAIL_GetBitRate(gRailHandle) > 0)
    {
        txSchedulerInfo.transactionTime += (frameLength + 4 + 1 + 1) * 8 * 1e6 / RAIL_GetBitRate(gRailHandle);
    }
    else
    { // assume 250kbps
        txSchedulerInfo.transactionTime += (frameLength + 4 + 1 + 1) * RADIO_TIMING_DEFAULT_BYTETIME_US;
    }
#endif

    if (getInternalFlag(FLAG_CURRENT_TX_USE_CSMA))
    {
#if RADIO_CONFIG_DMP_SUPPORT
        // time needed for CSMA/CA
        txSchedulerInfo.transactionTime += RADIO_TIMING_CSMA_OVERHEAD_US;
#endif
        csmaConfig.csmaTries    = sTxFrame->mInfo.mTxInfo.mMaxCsmaBackoffs;
        csmaConfig.ccaThreshold = sCcaThresholdDbm;
        status = RAIL_StartCcaCsmaTx(gRailHandle, sTxFrame->mChannel, txOptions, &csmaConfig, &txSchedulerInfo);
    }
    else
    {
        status = RAIL_StartTx(gRailHandle, sTxFrame->mChannel, txOptions, &txSchedulerInfo);
        if (status == RAIL_STATUS_NO_ERROR)
        {
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_STARTED, 0U);
        }
    }
    if (status == RAIL_STATUS_NO_ERROR)
    {
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailTxStarted++;
#endif
    }
    else
    {
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailTxStartFailed++;
#endif
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_BLOCKED, (uint32_t)ackRequested);
        txFailedCallback(false, TX_COMPLETE_RESULT_OTHER_FAIL);

        otSysEventSignalPending();
    }
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

    otRadioCaps capabilities = (OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_ENERGY_SCAN);

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
    capabilities |= OT_RADIO_CAPS_TRANSMIT_SEC;
#endif

#if OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
    capabilities |= OT_RADIO_CAPS_TRANSMIT_TIMING;
#endif

    return capabilities;
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

    status = RAIL_IEEE802154_SetPromiscuousMode(gRailHandle, aEnable);
    assert(status == RAIL_STATUS_NO_ERROR);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    // set Frame Pending bit for all outgoing ACKs if aEnable is false
    sIsSrcMatchEnabled = aEnable;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    // RAIL_GetTxPowerDbm() returns power in deci-dBm (0.1dBm)
    // Divide by 10 because aPower is supposed be in units dBm
    *aPower = RAIL_GetTxPowerDbm(gRailHandle) / 10;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    RAIL_Status_t status;

    // RAIL_SetTxPowerDbm() takes power in units of deci-dBm (0.1dBm)
    // Divide by 10 because aPower is supposed be in units dBm
    status = RAIL_SetTxPowerDbm(gRailHandle, ((RAIL_TxPower_t)aPower) * 10);
    assert(status == RAIL_STATUS_NO_ERROR);

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(aThreshold != NULL, error = OT_ERROR_INVALID_ARGS);

    *aThreshold = sCcaThresholdDbm;

exit:
    return error;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCcaThresholdDbm = aThreshold;

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return EFR32_RECEIVE_SENSITIVITY;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    return efr32StartEnergyScan(ENERGY_SCAN_MODE_ASYNC, aScanChannel, (RAIL_Time_t)aScanDuration * US_IN_MS);
}

//------------------------------------------------------------------------------
// Enhanced Acks and CSL support

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
void otPlatRadioSetMacKey(otInstance *    aInstance,
                          uint8_t         aKeyIdMode,
                          uint8_t         aKeyId,
                          const otMacKey *aPrevKey,
                          const otMacKey *aCurrKey,
                          const otMacKey *aNextKey)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);

    assert(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    sKeyId = aKeyId;
    memcpy(sPrevKey.m8, aPrevKey->m8, OT_MAC_KEY_SIZE);
    memcpy(sCurrKey.m8, aCurrKey->m8, OT_MAC_KEY_SIZE);
    memcpy(sNextKey.m8, aNextKey->m8, OT_MAC_KEY_SIZE);

    CORE_EXIT_ATOMIC();
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    sMacFrameCounter = aMacFrameCounter;

    CORE_EXIT_ATOMIC();
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddr);

    sCslPeriod = aCslPeriod;
    updateIeData();

    return OT_ERROR_NONE;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCslSampleTime = aCslSampleTime;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aEnabled && !sl_rail_util_coex_is_enabled())
    {
        otLogInfoPlat("Coexistence GPIO configurations not set");
        return OT_ERROR_FAILED;
    }
    sRadioCoexEnabled = aEnabled;
    return OT_ERROR_NONE;
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return (sRadioCoexEnabled && sl_rail_util_coex_is_enabled());
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aCoexMetrics != NULL, error = OT_ERROR_INVALID_ARGS);

    memset(aCoexMetrics, 0, sizeof(otRadioCoexMetrics));
    // TO DO:
    // Tracking coex metrics with detailed granularity currently
    // not implemented.
    // memcpy(aCoexMetrics, &sCoexMetrics, sizeof(otRadioCoexMetrics));
exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE

//------------------------------------------------------------------------------

// Return false if enhanced ACK is not required
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint8_t getKeySourceLength(uint8_t keyIdMode)
{
    uint8_t len = 0;

    switch (keyIdMode)
    {
    case IEEE802154_KEYID_MODE_0:
        len = IEEE802154_KEYID_MODE_0_SIZE;
        break;

    case IEEE802154_KEYID_MODE_1:
        len = IEEE802154_KEYID_MODE_1_SIZE;
        break;

    case IEEE802154_KEYID_MODE_2:
        len = IEEE802154_KEYID_MODE_2_SIZE;
        break;

    case IEEE802154_KEYID_MODE_3:
        len = IEEE802154_KEYID_MODE_3_SIZE;
        break;
    }

    return len;
}

static bool writeIeee802154EnhancedAck(RAIL_Handle_t aRailHandle, const uint8_t *aIeData, uint8_t aIeLength)
{
    // This table is derived from 802.15.4-2015 Section 7.2.1.5 PAN ID
    // Compression field and Table 7-2 for both 2003/2006 and 2015
    // frame versions.  It is indexed by 6 bits of the MacFCF:
    //   SrcAdrMode FrameVer<msbit> DstAdrMode PanIdCompression
    // and each address' length is encoded in a nibble:
    //    15:12  11:8     7:4     3:0
    //   SrcAdr  SrcPan  DstAdr  DstPan
    // Illegal combinations are indicated by 0xFFFFU.

#define ADDRSIZE_DST_PAN_SHIFT 0
#define ADDRSIZE_DST_PAN_MASK (0x0FU << ADDRSIZE_DST_PAN_SHIFT)
#define ADDRSIZE_DST_ADR_SHIFT 4
#define ADDRSIZE_DST_ADR_MASK (0x0FU << ADDRSIZE_DST_ADR_SHIFT)
#define ADDRSIZE_SRC_PAN_SHIFT 8
#define ADDRSIZE_SRC_PAN_MASK (0x0FU << ADDRSIZE_SRC_PAN_SHIFT)
#define ADDRSIZE_SRC_ADR_SHIFT 12
#define ADDRSIZE_SRC_ADR_MASK (0x0FU << ADDRSIZE_SRC_ADR_SHIFT)

    static const uint16_t ieee802154Table7p2[64] = {
        0x0000U, 0x0000U, 0xFFFFU, 0xFFFFU, 0x0022U, 0x0022U, 0x0082U, 0x0082U, 0x0000U, 0x0002U, 0xFFFFU,
        0xFFFFU, 0x0022U, 0x0020U, 0x0082U, 0x0080U, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU,
        0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0xFFFFU, 0x2200U,
        0x2200U, 0xFFFFU, 0xFFFFU, 0x2222U, 0x2022U, 0x2282U, 0x2082U, 0x2200U, 0x2000U, 0xFFFFU, 0xFFFFU,
        0x2222U, 0x2022U, 0x2282U, 0x2082U, 0x8200U, 0x8200U, 0xFFFFU, 0xFFFFU, 0x8222U, 0x8022U, 0x8282U,
        0x8082U, 0x8200U, 0x8000U, 0xFFFFU, 0xFFFFU, 0x8222U, 0x8022U, 0x8082U, 0x8080U,
    };

    // For an Enhanced ACK, we need to generate that ourselves;
    // RAIL will generate an Immediate ACK for us, though we can
    // tell it to go out with its FramePending bit set.
    // An 802.15.4 packet from RAIL should look like:
    // 1/2 |   1/2  | 0/1  |  0/2   | 0/2/8  |  0/2   | 0/2/8  |   14
    // PHR | MacFCF | Seq# | DstPan | DstAdr | SrcPan | SrcAdr | SecHdr

    // With RAIL_IEEE802154_EnableEarlyFramePending(), RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND
    // is triggered after receiving through the SrcAdr field of the packet,
    // not the SecHdr which hasn't been received yet.

#define EARLY_FRAME_PENDING_EXPECTED_BYTES (2U + 2U + 1U + 2U + 8U + 2U + 8U)
#define MAX_SECURED_EXPECTED_RECEIVED_BYTES (EARLY_FRAME_PENDING_EXPECTED_BYTES + 14U)
#define FINAL_PACKET_LENGTH_WITH_IE (MAX_SECURED_EXPECTED_RECEIVED_BYTES + aIeLength)

    RAIL_RxPacketInfo_t packetInfo;
    uint8_t             pkt[FINAL_PACKET_LENGTH_WITH_IE];

    // TODO, in the original prototype for this code, this check was made to
    // determine the PHY header length.
    // When we add Sub-Ghz support, and we call RAIL_IEEE802154_ConfigGOptions,
    // we should check for a 2-byte PHR.
    // #if     ("mac has channel pages" && IEEE802154_GB868_SUPPORTED)
    //     #define PHRLen ((emRadioChannelPageInUse == 0) ? 1 : 2)
    // #else
    //     #define PHRLen 1
    uint8_t PHRLen = 1;

    uint8_t pktOffset = PHRLen; // No need to parse the PHR byte(s)
    RAIL_GetRxIncomingPacketInfo(gRailHandle, &packetInfo);

    // Check if packetinfo.packetBytes includes the info we want,
    // and if not, spin-wait calling RAIL_GetRxIncomingPacketInfo()
    // until it has arrived
    uint32_t startMs = otPlatAlarmMilliGetNow();
    while (packetInfo.packetBytes < MAX_SECURED_EXPECTED_RECEIVED_BYTES)
    {
        RAIL_GetRxIncomingPacketInfo(gRailHandle, &packetInfo);
        if (otPlatAlarmMilliGetNow() - startMs > 100u)
        {
            // More than 100 ms. has elapsed.
            break;
        }
    }

    if (packetInfo.packetBytes < (pktOffset + 2U))
    {
        return false;
    }

    // Only extract what we care about
    if (packetInfo.packetBytes > MAX_SECURED_EXPECTED_RECEIVED_BYTES)
    {
        packetInfo.packetBytes = MAX_SECURED_EXPECTED_RECEIVED_BYTES;
        if (packetInfo.firstPortionBytes >= MAX_SECURED_EXPECTED_RECEIVED_BYTES)
        {
            packetInfo.firstPortionBytes = MAX_SECURED_EXPECTED_RECEIVED_BYTES;
            packetInfo.lastPortionData   = NULL;
        }
    }

    RAIL_CopyRxPacket(pkt, &packetInfo);
    uint16_t macFcf = pkt[pktOffset++];

    if ((macFcf & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_MULTIPURPOSE)
    {
        // Multipurpose frames have an arcane FCF structure
        if ((macFcf & IEEE802154_MP_FRAME_FLAG_LONG_FCF) != 0U)
        {
            macFcf |= (pkt[pktOffset++] << 8);
        }

        // Map Multipurpose FCF to a 'normal' Version FCF as best we can.
        macFcf =
            (IEEE802154_FRAME_TYPE_MULTIPURPOSE |
             ((macFcf & (IEEE802154_MP_FRAME_FLAG_SECURITY_ENABLED | IEEE802154_MP_FRAME_FLAG_IE_LIST_PRESENT)) >> 6) |
             ((macFcf & IEEE802154_MP_FRAME_FLAG_FRAME_PENDING) >> 7) |
             ((macFcf & IEEE802154_MP_FRAME_FLAG_ACK_REQUIRED) >> 9) |
             ((macFcf & (IEEE802154_MP_FRAME_FLAG_PANID_PRESENT | IEEE802154_MP_FRAME_FLAG_SEQ_SUPPRESSION)) >> 2) |
             ((macFcf & IEEE802154_MP_FRAME_DESTINATION_MODE_MASK) << 6) | IEEE802154_MP_FRAME_VERSION_2015 |
             ((macFcf & IEEE802154_MP_FRAME_SOURCE_MODE_MASK) << 8));

        // MultiPurpose's PANID_PRESENT is not equivalent to 2012/5's
        // PANID_COMPRESSION so we map it best we can by flipping it
        // in the following address-combination situations:
        uint16_t addrCombo = (macFcf & (IEEE802154_FRAME_SOURCE_MODE_MASK | IEEE802154_FRAME_DESTINATION_MODE_MASK));
        if ((addrCombo == (IEEE802154_FRAME_SOURCE_MODE_NONE | IEEE802154_FRAME_DESTINATION_MODE_NONE)) ||
            (addrCombo == (IEEE802154_FRAME_SOURCE_MODE_SHORT | IEEE802154_FRAME_DESTINATION_MODE_SHORT)) ||
            (addrCombo == (IEEE802154_FRAME_SOURCE_MODE_SHORT | IEEE802154_FRAME_DESTINATION_MODE_LONG)) ||
            (addrCombo == (IEEE802154_FRAME_SOURCE_MODE_LONG | IEEE802154_FRAME_DESTINATION_MODE_SHORT)))
        {
            // 802.15.4-2015 PANID_COMPRESSION = MP PANID_PRESENT
        }
        else
        {
            // 802.15.4-2015 PANID_COMPRESSION = !MP PANID_PRESENT
            macFcf ^= IEEE802154_FRAME_FLAG_PANID_COMPRESSION; // Flip it
        }
    }
    else
    {
        macFcf |= (pkt[pktOffset++] << 8);
    }

    bool enhAck = ((macFcf & IEEE802154_FRAME_VERSION_MASK) == IEEE802154_FRAME_VERSION_2015);
    if (!enhAck)
    {
        return false;
    }

    // Compress MAC FCF to index into 64-entry address-length table:
    // SrcAdrMode FrameVer<msbit> DstAdrMode PanIdCompression
    //
    // Note: Use IEEE802154_FRAME_VERSION_2012 rather than _MASK so the
    // low-order bit of the version field isn't used in deriving the index.
    uint16_t index = (((macFcf & (IEEE802154_FRAME_SOURCE_MODE_MASK | IEEE802154_FRAME_VERSION_2012)) >> 10) |
                      ((macFcf & IEEE802154_FRAME_DESTINATION_MODE_MASK) >> 9) |
                      ((macFcf & IEEE802154_FRAME_FLAG_PANID_COMPRESSION) >> 6));

    uint16_t addrSizes = ieee802154Table7p2[index];
    // Illegal combinations mean illegal packets which we ignore
    if (addrSizes == 0xFFFFU)
    {
        return false;
    }

    uint8_t seqNo =
        ((enhAck && ((macFcf & IEEE802154_FRAME_FLAG_SEQ_SUPPRESSION) != 0U)) ? 0U : pkt[pktOffset++]); // Seq#

    // Start writing the enhanced ACK -- we need to construct it since RAIL cannot.

    // First extract addresses from incoming packet since we may
    // need to reflect them in a different order in the outgoing ACK.
    // Use byte[0] to hold each one's length.
    uint8_t dstPan[3] = {
        0,
    }; // Initialized only to eliminate false gcc warning
    dstPan[0] = ((addrSizes & ADDRSIZE_DST_PAN_MASK) >> ADDRSIZE_DST_PAN_SHIFT);
    if ((dstPan[0] + pktOffset) > packetInfo.packetBytes)
    {
        return false;
    }

    if (dstPan[0] > 0U)
    {
        dstPan[1] = pkt[pktOffset++];
        dstPan[2] = pkt[pktOffset++];
    }

    uint8_t dstAdr[9];
    dstAdr[0] = ((addrSizes & ADDRSIZE_DST_ADR_MASK) >> ADDRSIZE_DST_ADR_SHIFT);
    if ((dstAdr[0] + pktOffset) > packetInfo.packetBytes)
    {
        return false;
    }

    for (uint8_t i = 1U; i <= dstAdr[0]; i++)
    {
        dstAdr[i] = pkt[pktOffset++];
    }

    uint8_t srcPan[3];
    srcPan[0] = ((addrSizes & ADDRSIZE_SRC_PAN_MASK) >> ADDRSIZE_SRC_PAN_SHIFT);
    if ((srcPan[0] + pktOffset) > packetInfo.packetBytes)
    {
        return false;
    }

    if (srcPan[0] > 0U)
    {
        srcPan[1] = pkt[pktOffset++];
        srcPan[2] = pkt[pktOffset++];
    }

    uint8_t srcAdr[9];
    srcAdr[0] = ((addrSizes & ADDRSIZE_SRC_ADR_MASK) >> ADDRSIZE_SRC_ADR_SHIFT);
    if ((srcAdr[0] + pktOffset) > packetInfo.packetBytes)
    {
        return false;
    }

    for (uint8_t i = 1U; i <= srcAdr[0]; i++)
    {
        srcAdr[i] = pkt[pktOffset++];
    }

    // Once done with address fields, pick the security control (if present)
    uint8_t securityHeader[1 + 4 + 8 + 1]; // max len: control + fc + key source + key ID
    uint8_t securityHeaderLength = 0;

    if (macFcf & IEEE802154_FRAME_FLAG_SECURITY_ENABLED)
    {
        uint8_t securityControl = pkt[pktOffset];
        // Then the key ID
        uint8_t keySourceLength = getKeySourceLength(securityControl & IEEE802154_KEYID_MODE_MASK);
        securityHeaderLength += (sizeof(uint8_t)                       /* security control */
                                 + sizeof(uint32_t)                    /* frame counter */
                                 + keySourceLength + sizeof(uint8_t)); /* key ID */
        memcpy(securityHeader, pkt + pktOffset, securityHeaderLength);
        pktOffset += securityHeaderLength;
    }

    // Reuse packet[] buffer for outgoing Enhanced ACK.
    // Phr1 Phr2 FcfL FcfH [Seq#] [DstPan] [DstAdr] [SrcPan] [SrcAdr]
    // Will fill in PHR later.
    // MAC Fcf:
    // - Frame Type = ACK
    // - Security Enabled, as appropriate
    // - Frame Pending = 0 or as appropriate
    // - ACK Request = 0
    // - PanId compression = incoming packet's
    // - Seq# suppression = incoming packet's
    // - IE Present = 0 in this implementation
    // - DstAdrMode = SrcAdrMode of incoming packet's
    // - Frame Version = 2 (154E)
    // - SrcAdrMode = DstAdrMode of incoming packet's (for convenience)
    uint16_t ackFcf =
        (IEEE802154_FRAME_TYPE_ACK | (macFcf & IEEE802154_FRAME_FLAG_PANID_COMPRESSION) |
         (macFcf & IEEE802154_FRAME_FLAG_SEQ_SUPPRESSION) | (macFcf & IEEE802154_FRAME_FLAG_SECURITY_ENABLED) |
         IEEE802154_FRAME_VERSION_2015 | ((macFcf & IEEE802154_FRAME_SOURCE_MODE_MASK) >> 4) |
         ((macFcf & IEEE802154_FRAME_DESTINATION_MODE_MASK) << 4));

    // Do frame-pending check now
    if (sIsSrcMatchEnabled)
    {
        bool setFramePending = true;
        if (srcAdr[0] > 0U)
        {
            if (srcAdr[0] == 8U)
            {
                setFramePending = (utilsSoftSrcMatchExtFindEntry((otExtAddress *)&srcAdr[1]) >= 0);
            }
            else
            {
                uint16_t srcAdrShort = srcAdr[1] | (srcAdr[2] << 8);
                setFramePending      = (utilsSoftSrcMatchShortFindEntry(srcAdrShort) >= 0);
            }
        }
        if (setFramePending)
        {
            ackFcf |= IEEE802154_FRAME_FLAG_FRAME_PENDING;
        }
    }

    pktOffset        = PHRLen;
    pkt[pktOffset++] = (uint8_t)ackFcf;
    pkt[pktOffset++] = (uint8_t)(ackFcf >> 8);

    if ((macFcf & IEEE802154_FRAME_FLAG_SEQ_SUPPRESSION) == 0U)
    {
        pkt[pktOffset++] = seqNo;
    }

    // Determine outgoing ACK's address field sizes
    index = (((ackFcf & (IEEE802154_FRAME_SOURCE_MODE_MASK | IEEE802154_FRAME_VERSION_2012)) >> 10) |
             ((ackFcf & IEEE802154_FRAME_DESTINATION_MODE_MASK) >> 9) |
             ((ackFcf & IEEE802154_FRAME_FLAG_PANID_COMPRESSION) >> 6));

    addrSizes = ieee802154Table7p2[index];
    if (addrSizes == 0xFFFFU)
    {
        // Uh-oh! Enh-ACK would be malformed?!  Something funky happened!
        // Possibly a latency-induced issue.
        return false;
    }

    // DstPan = SrcPan of incoming if avail otherwise DstPan of incoming
    if ((addrSizes & ADDRSIZE_DST_PAN_MASK) != 0U)
    {
        if (srcPan[0] > 0U)
        {
            pkt[pktOffset++] = srcPan[1];
            pkt[pktOffset++] = srcPan[2];
        }
        else if (dstPan[0] > 0U)
        {
            pkt[pktOffset++] = dstPan[1];
            pkt[pktOffset++] = dstPan[2];
        }
        else
        {
            // Uh-oh! Outgoing packet needs a DstPanId but incoming had neither!
            // Possibly a latency-induced issue.
            return false;
        }
    }

    // DstAdr = SrcAdr of incoming packet -- their sizes should match
    if ((addrSizes & ADDRSIZE_DST_ADR_MASK) != 0U)
    {
        for (uint8_t i = 1U; i <= srcAdr[0]; i++)
        {
            pkt[pktOffset++] = srcAdr[i];
        }
    }

    // SrcPan = DstPan of incoming if avail otherwise SrcPan of incoming
    if ((addrSizes & ADDRSIZE_SRC_PAN_MASK) != 0U)
    {
        if (dstPan[0] > 0U)
        {
            pkt[pktOffset++] = dstPan[1];
            pkt[pktOffset++] = dstPan[2];
        }
        else if (srcPan[0] > 0U)
        {
            pkt[pktOffset++] = srcPan[1];
            pkt[pktOffset++] = srcPan[2];
        }
        else
        {
            // Uh-oh! Outgoing packet needs a SrcPanId but incoming had neither!
            // Possibly a latency-induced issue.
            return false;
        }
    }

    // SrcAdr = DstAdr of incoming packet -- their sizes should match
    if ((addrSizes & ADDRSIZE_SRC_ADR_MASK) != 0U)
    {
        for (uint8_t i = 1U; i <= dstAdr[0]; i++)
        {
            pkt[pktOffset++] = dstAdr[i];
        }
    }

    if (ackFcf & IEEE802154_FRAME_FLAG_SECURITY_ENABLED)
    {
        memcpy(pkt + pktOffset, securityHeader, securityHeaderLength);
        pktOffset += securityHeaderLength;
    }

    // Now add the IE data
    memcpy(pkt + pktOffset, aIeData, aIeLength);
    pktOffset += aIeLength;

    // Fill in PHR now that we know Enh-ACK's length
    if (PHRLen == 2U) // Not true till SubGhz implementation is in place
    {
        pkt[0] = (0x08U /*FCS=2byte*/ | 0x10U /*Whiten=enabled*/);
        pkt[1] = (uint8_t)(__RBIT(pktOffset - 2U /*PHRLen*/ + 2U /*FCS*/) >> 24);
    }
    else
    {
        pkt[0] = (pktOffset - 1U /*PHRLen*/ + 2U /*FCS*/);
    }

    processSecurityForEnhancedAck(pkt);

    return (RAIL_IEEE802154_WriteEnhAck(aRailHandle, pkt, pktOffset) == RAIL_STATUS_NO_ERROR);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

//------------------------------------------------------------------------------
// RAIL callbacks

static void dataRequestCommandCallback(RAIL_Handle_t aRailHandle)
{
    // This callback occurs after the address fields of an incoming
    // ACK-requesting CMD or DATA frame have been received and we
    // can do a frame pending check.  We must also figure out what
    // kind of ACK is being requested -- Immediate or Enhanced.

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2 && OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE)
    // Check if we need to write an enhanced ACK.
    if (writeIeee802154EnhancedAck(aRailHandle, sAckIeData, sAckIeDataLength))
    {
        return;
    }
#endif

    // If not, RAIL will send an immediate ACK, but we need to do FP lookup.
    RAIL_Status_t status = RAIL_STATUS_NO_ERROR;

    bool framePendingSet = false;

    if (sIsSrcMatchEnabled)
    {
        RAIL_IEEE802154_Address_t sourceAddress;

        status = RAIL_IEEE802154_GetAddress(aRailHandle, &sourceAddress);
        otEXPECT(status == RAIL_STATUS_NO_ERROR);

        if ((sourceAddress.length == RAIL_IEEE802154_LongAddress &&
             utilsSoftSrcMatchExtFindEntry((otExtAddress *)sourceAddress.longAddress) >= 0) ||
            (sourceAddress.length == RAIL_IEEE802154_ShortAddress &&
             utilsSoftSrcMatchShortFindEntry(sourceAddress.shortAddress) >= 0))
        {
            status = RAIL_IEEE802154_SetFramePending(aRailHandle);
            otEXPECT(status == RAIL_STATUS_NO_ERROR);
            framePendingSet = true;
        }
    }
    else
    {
        status = RAIL_IEEE802154_SetFramePending(aRailHandle);
        otEXPECT(status == RAIL_STATUS_NO_ERROR);
        framePendingSet = true;
    }

    if (framePendingSet)
    {
        // Store whether frame pending was set in the outgoing ACK in a reserved
        // bit of the MAC header.
        RAIL_RxPacketInfo_t packetInfo;
        RAIL_GetRxIncomingPacketInfo(gRailHandle, &packetInfo);

        // skip length byte
        otEXPECT(packetInfo.firstPortionBytes > 0);
        packetInfo.firstPortionData++;
        packetInfo.firstPortionBytes--;
        packetInfo.packetBytes--;

        uint8_t *macFcfPointer = ((packetInfo.firstPortionBytes == 0) ? (uint8_t *)packetInfo.lastPortionData
                                                                      : (uint8_t *)packetInfo.firstPortionData);
        *macFcfPointer |= IEEE802154_FRAME_PENDING_SET_IN_OUTGOING_ACK;
    }

exit:
    if (status == RAIL_STATUS_INVALID_STATE)
    {
        otLogWarnPlat("Too late to modify outgoing FP");
    }
    else
    {
        assert(status == RAIL_STATUS_NO_ERROR);
    }
}

static void packetReceivedCallback(RAIL_RxPacketHandle_t packetHandle)
{
    RAIL_RxPacketInfo_t    packetInfo;
    RAIL_RxPacketDetails_t packetDetails;
    uint16_t               length;
    bool                   framePendingInAck = false;
    bool                   rxCorrupted       = false;

    packetHandle = RAIL_GetRxPacketInfo(gRailHandle, packetHandle, &packetInfo);
    otEXPECT_ACTION(
        (packetHandle != RAIL_RX_PACKET_HANDLE_INVALID && packetInfo.packetStatus == RAIL_RX_PACKET_READY_SUCCESS),
        rxCorrupted = true);

    otEXPECT_ACTION(validatePacketDetails(packetHandle, &packetDetails, &packetInfo, &length), rxCorrupted = true);

    // skip length byte
    otEXPECT_ACTION(packetInfo.firstPortionBytes > 0, rxCorrupted = true);
    packetInfo.firstPortionData++;
    packetInfo.firstPortionBytes--;
    packetInfo.packetBytes--;

    uint8_t macFcf =
        ((packetInfo.firstPortionBytes == 0) ? packetInfo.lastPortionData[0] : packetInfo.firstPortionData[0]);

    if (packetDetails.isAck)
    {
        otEXPECT_ACTION(
            (length == IEEE802154_ACK_LENGTH && (macFcf & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK),
            rxCorrupted = true);

        // read packet
        RAIL_CopyRxPacket(sReceiveAckFrame.mPsdu, &packetInfo);
        sReceiveAckFrame.mLength = length;

        // Releasing the ACK frames here, ensures that the main thread (processNextRxPacket)
        // is not wasting cycles, releasing the ACK frames from the Rx FIFO queue.
        RAIL_ReleaseRxPacket(gRailHandle, packetHandle);

        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ENDED, (uint32_t)isReceivingFrame());

        if (txWaitingForAck() &&
            (sReceiveAckFrame.mPsdu[IEEE802154_DSN_OFFSET] == sTransmitFrame.mPsdu[IEEE802154_DSN_OFFSET]))
        {
            otEXPECT_ACTION(validatePacketTimestamp(&packetDetails, length), rxCorrupted = true);
            updateRxFrameDetails(&packetDetails, false);

            // Processing the ACK frame in ISR context avoids the Tx state to be messed up,
            // in case the Rx FIFO queue gets wiped out in a DMP situation.
            sTransmitBusy  = false;
            sTransmitError = OT_ERROR_NONE;
            setInternalFlag(FLAG_WAITING_FOR_ACK, false);

            framePendingInAck = ((macFcf & IEEE802154_FRAME_FLAG_FRAME_PENDING) != 0);
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_ACK_RECEIVED, (uint32_t)framePendingInAck);

            if (txIsDataRequest() && framePendingInAck)
            {
                emPendingData = true;
            }
        }
        // Yield the radio upon receiving an ACK as long as it is not related to
        // a data request.
        if (!txIsDataRequest())
        {
            RAIL_YieldRadio(gRailHandle);
        }
    }
    else
    {
        otEXPECT_ACTION(sPromiscuous || (length != IEEE802154_ACK_LENGTH), rxCorrupted = true);

        if (macFcf & IEEE802154_FRAME_FLAG_ACK_REQUIRED)
        {
            (void)handlePhyStackEvent((RAIL_IsRxAutoAckPaused(gRailHandle)
                                           ? SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACK_BLOCKED
                                           : SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACKING),
                                      (uint32_t)isReceivingFrame());
            setInternalFlag(FLAG_ONGOING_TX_ACK, true);
        }
        else
        {
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ENDED, (uint32_t)isReceivingFrame());
            // We received a frame that does not require an ACK as result of a data
            // poll: we yield the radio here.
            if (emPendingData)
            {
                RAIL_YieldRadio(gRailHandle);
                emPendingData = false;
            }
        }
    }
exit:
    if (rxCorrupted)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_CORRUPTED, (uint32_t)isReceivingFrame());
    }
}

static void packetSentCallback(bool isAck)
{
    if (isAck)
    {
        // We successfully sent out an ACK.
        setInternalFlag(FLAG_ONGOING_TX_ACK, false);
        // We acked the packet we received after a poll: we can yield now.
        if (emPendingData)
        {
            RAIL_YieldRadio(gRailHandle);
            emPendingData = false;
        }
    }
    else if (getInternalFlag(FLAG_ONGOING_TX_DATA))
    {
        setInternalFlag(FLAG_ONGOING_TX_DATA, false);

        if (txWaitingForAck())
        {
            setInternalFlag(FLAG_WAITING_FOR_ACK, true);
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_ACK_WAITING, 0U);
        }
        else
        {
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_ENDED, 0U);
            RAIL_YieldRadio(gRailHandle);
            sTransmitError = OT_ERROR_NONE;
            sTransmitBusy  = false;
        }
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventPacketSent++;
#endif
    }
}

static void txFailedCallback(bool isAck, uint8_t status)
{
    if (isAck)
    {
        setInternalFlag(FLAG_ONGOING_TX_ACK, false);
    }
    else if (getInternalFlag(FLAG_ONGOING_TX_DATA))
    {
        if (status == TX_COMPLETE_RESULT_CCA_FAIL)
        {
            sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
            setInternalFlag(FLAG_CURRENT_TX_USE_CSMA, false);
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventChannelBusy++;
#endif
        }
        else
        {
            sTransmitError = OT_ERROR_ABORT;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailEventTxAbort++;
#endif
        }
        setInternalFlag(FLAG_ONGOING_TX_DATA, false);
        RAIL_YieldRadio(gRailHandle);
        sTransmitBusy = false;
    }
}

static void ackTimeoutCallback(void)
{
    assert(txWaitingForAck());
    assert(getInternalFlag(FLAG_WAITING_FOR_ACK));

    sTransmitError = OT_ERROR_NO_ACK;
    sTransmitBusy  = false;
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    sRailDebugCounters.mRailEventNoAck++;
#endif

#ifdef SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
    // If antenna diversity is enabled toggle the selected antenna.
    sl_rail_util_ant_div_toggle_antenna();
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
    // TO DO: Check if we have an OT function that
    // provides the number of mac retry attempts left
    (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_ACK_TIMEDOUT, 0);

    setInternalFlag(FLAG_WAITING_FOR_ACK, false);
    RAIL_YieldRadio(gRailHandle);
    emPendingData = false;
}

static void schedulerEventCallback(RAIL_Handle_t aRailHandle)
{
    RAIL_SchedulerStatus_t status = RAIL_GetSchedulerStatus(aRailHandle);
    assert(status != RAIL_SCHEDULER_STATUS_INTERNAL_ERROR);

    if (status == RAIL_SCHEDULER_STATUS_CCA_CSMA_TX_FAIL || status == RAIL_SCHEDULER_STATUS_SINGLE_TX_FAIL ||
        status == RAIL_SCHEDULER_STATUS_SCHEDULED_TX_FAIL ||
        (status == RAIL_SCHEDULER_STATUS_SCHEDULE_FAIL && sTransmitBusy) ||
        (status == RAIL_SCHEDULER_STATUS_EVENT_INTERRUPTED && sTransmitBusy))
    {
        if (getInternalFlag(FLAG_ONGOING_TX_ACK))
        {
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACK_ABORTED, (uint32_t)isReceivingFrame());
            txFailedCallback(true, 0xFF);
        }
        // We were in the process of TXing a data frame, treat it as a CCA_FAIL.
        if (getInternalFlag(FLAG_ONGOING_TX_DATA))
        {
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_BLOCKED, (uint32_t)txWaitingForAck());
            txFailedCallback(false, TX_COMPLETE_RESULT_CCA_FAIL);
        }

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventSchedulerStatusError++;
#endif
    }
    else if (status == RAIL_SCHEDULER_STATUS_AVERAGE_RSSI_FAIL ||
             (status == RAIL_SCHEDULER_STATUS_SCHEDULE_FAIL && sEnergyScanStatus == ENERGY_SCAN_STATUS_IN_PROGRESS))
    {
        energyScanComplete(OT_RADIO_RSSI_INVALID);
    }
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
    else if (sTransmitBusy)
    {
        sRailDebugCounters.mRailEventsSchedulerStatusLastStatus = status;
        sRailDebugCounters.mRailEventsSchedulerStatusTransmitBusy++;
    }
#endif
}

static void configUnscheduledCallback(void)
{
    // We are waiting for an ACK: we will never get the ACK we were waiting for.
    // We want to call ackTimeoutCallback() only if the PACKET_SENT event
    // already fired (which would clear the FLAG_ONGOING_TX_DATA flag).
    if (getInternalFlag(FLAG_WAITING_FOR_ACK))
    {
        ackTimeoutCallback();
    }

    // We are about to send an ACK, which it won't happen.
    if (getInternalFlag(FLAG_ONGOING_TX_ACK))
    {
        txFailedCallback(true, 0xFF);
    }
}

static void RAILCb_Generic(RAIL_Handle_t aRailHandle, RAIL_Events_t aEvents)
{
#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
    if (aEvents & (RAIL_EVENT_RX_SYNC1_DETECT | RAIL_EVENT_RX_SYNC2_DETECT))
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_STARTED, (uint32_t)isReceivingFrame());
    }
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

    if ((aEvents & RAIL_EVENT_IEEE802154_DATA_REQUEST_COMMAND)
#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
        && !RAIL_IsRxAutoAckPaused(aRailHandle)
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT
    )
    {
        dataRequestCommandCallback(aRailHandle);
    }

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
    if (aEvents & RAIL_EVENT_RX_FILTER_PASSED)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACCEPTED, (uint32_t)isReceivingFrame());
    }
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

    if (aEvents & RAIL_EVENT_TX_PACKET_SENT)
    {
        packetSentCallback(false);
    }
    else if (aEvents & RAIL_EVENT_TX_CHANNEL_BUSY)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_BLOCKED, (uint32_t)txWaitingForAck());
        txFailedCallback(false, TX_COMPLETE_RESULT_CCA_FAIL);
    }
    else if (aEvents & RAIL_EVENT_TX_BLOCKED)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_BLOCKED, (uint32_t)txWaitingForAck());
        txFailedCallback(false, TX_COMPLETE_RESULT_OTHER_FAIL);
    }
    else if (aEvents & (RAIL_EVENT_TX_UNDERFLOW | RAIL_EVENT_TX_ABORTED))
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_ABORTED, (uint32_t)txWaitingForAck());
        txFailedCallback(false, TX_COMPLETE_RESULT_OTHER_FAIL);
    }
    else
    {
        // Pre-completion aEvents are processed in their logical order:
#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
        if (aEvents & RAIL_EVENT_TX_START_CCA)
        {
            // We are starting RXWARM for a CCA check
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_CCA_SOON, 0U);
        }
        if (aEvents & RAIL_EVENT_TX_CCA_RETRY)
        {
            // We failed a CCA check and need to retry
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_CCA_BUSY, 0U);
        }
        if (aEvents & RAIL_EVENT_TX_CHANNEL_CLEAR)
        {
            // We're going on-air
            (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_STARTED, 0U);
        }
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
    }

    if (aEvents & RAIL_EVENT_RX_PACKET_RECEIVED)
    {
        packetReceivedCallback(RAIL_HoldRxPacket(aRailHandle));
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventPacketReceived++;
#endif
    }

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
    if (aEvents & RAIL_EVENT_RX_FRAME_ERROR)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_CORRUPTED, (uint32_t)isReceivingFrame());
    }
    // The following 3 events cause us to not receive a packet
    if (aEvents & (RAIL_EVENT_RX_PACKET_ABORTED | RAIL_EVENT_RX_ADDRESS_FILTERED | RAIL_EVENT_RX_FIFO_OVERFLOW))
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_FILTERED, (uint32_t)isReceivingFrame());
    }
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

    if (aEvents & RAIL_EVENT_TXACK_PACKET_SENT)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACK_SENT, (uint32_t)isReceivingFrame());
        packetSentCallback(true);
    }
    if (aEvents & (RAIL_EVENT_TXACK_ABORTED | RAIL_EVENT_TXACK_UNDERFLOW))
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACK_ABORTED, (uint32_t)isReceivingFrame());
        txFailedCallback(true, 0xFF);
    }
    if (aEvents & RAIL_EVENT_TXACK_BLOCKED)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_ACK_BLOCKED, (uint32_t)isReceivingFrame());
        txFailedCallback(true, 0xFF);
    }
    // Deal with ACK timeout after possible RX completion in case RAIL
    // notifies us of the ACK and the timeout simultaneously -- we want
    // the ACK to win over the timeout.
    if (aEvents & RAIL_EVENT_RX_ACK_TIMEOUT)
    {
        if (getInternalFlag(FLAG_WAITING_FOR_ACK))
        {
            ackTimeoutCallback();
        }
    }

    if (aEvents & RAIL_EVENT_CONFIG_UNSCHEDULED)
    {
        (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_RX_IDLED, 0U);
        configUnscheduledCallback();
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventConfigUnScheduled++;
#endif
    }

    if (aEvents & RAIL_EVENT_CONFIG_SCHEDULED)
    {
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventConfigScheduled++;
#endif
    }

    if (aEvents & RAIL_EVENT_SCHEDULER_STATUS)
    {
        schedulerEventCallback(aRailHandle);
    }

    if (aEvents & RAIL_EVENT_CAL_NEEDED)
    {
        RAIL_Status_t status;

        status = RAIL_Calibrate(aRailHandle, NULL, RAIL_CAL_ALL_PENDING);
        // TODO: Non-RTOS DMP case fails
#if (!defined(SL_CATALOG_BLUETOOTH_PRESENT) || defined(SL_CATALOG_KERNEL_PRESENT))
        assert(status == RAIL_STATUS_NO_ERROR);
#endif

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventCalNeeded++;
#endif
    }

    if (aEvents & RAIL_EVENT_RSSI_AVERAGE_DONE)
    {
        const int16_t energyScanResultQuarterDbm = RAIL_GetAverageRssi(aRailHandle);
        RAIL_YieldRadio(aRailHandle);

        energyScanComplete(energyScanResultQuarterDbm == RAIL_RSSI_INVALID
                               ? OT_RADIO_RSSI_INVALID
                               : (energyScanResultQuarterDbm / QUARTER_DBM_IN_DBM));
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailPlatRadioEnergyScanDoneCbCount++;
#endif
    }

    otSysEventSignalPending();
}

//------------------------------------------------------------------------------
// Main thread packet handling

static bool validatePacketDetails(RAIL_RxPacketHandle_t   packetHandle,
                                  RAIL_RxPacketDetails_t *pPacketDetails,
                                  RAIL_RxPacketInfo_t *   pPacketInfo,
                                  uint16_t *              packetLength)
{
    bool pktValid = true;

    otEXPECT_ACTION((RAIL_GetRxPacketDetailsAlt(gRailHandle, packetHandle, pPacketDetails) == RAIL_STATUS_NO_ERROR),
                    pktValid = false);

    // RAIL's packetBytes includes the 1-byte PHY header but not the 2-byte CRC
    // We want *packetLength to match the PHY header length so we add 2 for CRC
    // and subtract 1 for PHY header.
    *packetLength = pPacketInfo->packetBytes + 1;

    // check the length in recv packet info structure; RAIL should take care of this.
    otEXPECT_ACTION(*packetLength == pPacketInfo->firstPortionData[0], pktValid = false);

    // check the length validity of recv packet; RAIL should take care of this.
    otEXPECT_ACTION((*packetLength >= IEEE802154_MIN_LENGTH && *packetLength <= IEEE802154_MAX_LENGTH),
                    pktValid = false);
exit:
    return pktValid;
}

static bool validatePacketTimestamp(RAIL_RxPacketDetails_t *pPacketDetails, uint16_t packetLength)
{
    bool rxTimestampValid = true;

    // Get the timestamp when the SFD was received
    otEXPECT_ACTION(pPacketDetails->timeReceived.timePosition != RAIL_PACKET_TIME_INVALID, rxTimestampValid = false);

    // + 1 for the 1-byte PHY header
    pPacketDetails->timeReceived.totalPacketBytes = packetLength + 1;

    otEXPECT_ACTION((RAIL_GetRxTimeSyncWordEndAlt(gRailHandle, pPacketDetails) == RAIL_STATUS_NO_ERROR),
                    rxTimestampValid = false);
exit:
    return rxTimestampValid;
}

static void updateRxFrameDetails(RAIL_RxPacketDetails_t *pPacketDetails, bool framePendingSetInOutgoingAck)
{
    assert(pPacketDetails != NULL);

    if (pPacketDetails->isAck)
    {
        sReceiveAckFrame.mInfo.mRxInfo.mRssi      = pPacketDetails->rssi;
        sReceiveAckFrame.mInfo.mRxInfo.mLqi       = pPacketDetails->lqi;
        sReceiveAckFrame.mInfo.mRxInfo.mTimestamp = pPacketDetails->timeReceived.packetTime;
    }
    else
    {
        sReceiveFrame.mInfo.mRxInfo.mRssi      = pPacketDetails->rssi;
        sReceiveFrame.mInfo.mRxInfo.mLqi       = pPacketDetails->lqi;
        sReceiveFrame.mInfo.mRxInfo.mTimestamp = pPacketDetails->timeReceived.packetTime;
        // Set this flag only when the packet is really acknowledged with frame pending set.
        sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = framePendingSetInOutgoingAck;
    }
}

static void processNextRxPacket(otInstance *aInstance)
{
    RAIL_RxPacketHandle_t  packetHandle = RAIL_RX_PACKET_HANDLE_INVALID;
    RAIL_RxPacketInfo_t    packetInfo;
    RAIL_RxPacketDetails_t packetDetails;
    RAIL_Status_t          status;
    uint16_t               length;
    bool                   rxProcessDone = false;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    packetHandle = RAIL_GetRxPacketInfo(gRailHandle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &packetInfo);
    otEXPECT_ACTION(
        (packetHandle != RAIL_RX_PACKET_HANDLE_INVALID && packetInfo.packetStatus == RAIL_RX_PACKET_READY_SUCCESS),
        packetHandle = RAIL_RX_PACKET_HANDLE_INVALID);

    otEXPECT(validatePacketDetails(packetHandle, &packetDetails, &packetInfo, &length));

    // skip length byte
    otEXPECT(packetInfo.firstPortionBytes > 0);
    packetInfo.firstPortionData++;
    packetInfo.firstPortionBytes--;
    packetInfo.packetBytes--;

    // As received ACK frames are already processed in packetReceivedCallback,
    // we only need to read and process the non-ACK frames here.
    otEXPECT(sPromiscuous || (!packetDetails.isAck && (length != IEEE802154_ACK_LENGTH)));

    // read packet
    RAIL_CopyRxPacket(sReceiveFrame.mPsdu, &packetInfo);
    sReceiveFrame.mLength = length;

    uint8_t *macFcfPointer = sReceiveFrame.mPsdu;

    // Check the reserved bit in the MAC header to see whether the frame pending
    // bit was set in the outgoing ACK
    // Then, clear it.
    bool framePendingSetInOutgoingAck = ((*macFcfPointer & IEEE802154_FRAME_PENDING_SET_IN_OUTGOING_ACK) != 0);
    *macFcfPointer &= ~IEEE802154_FRAME_PENDING_SET_IN_OUTGOING_ACK;

    status = RAIL_ReleaseRxPacket(gRailHandle, packetHandle);
    if (status == RAIL_STATUS_NO_ERROR)
    {
        packetHandle = RAIL_RX_PACKET_HANDLE_INVALID;
    }

    otEXPECT(validatePacketTimestamp(&packetDetails, length));
    updateRxFrameDetails(&packetDetails, framePendingSetInOutgoingAck);
    rxProcessDone = true;

exit:
    if (packetHandle != RAIL_RX_PACKET_HANDLE_INVALID)
    {
        RAIL_ReleaseRxPacket(gRailHandle, packetHandle);
    }
    CORE_EXIT_ATOMIC();

    // signal MAC layer
    if (rxProcessDone)
    {
        sReceiveError = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_DIAG_ENABLE
        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
        }
        else
#endif
        {
            otLogInfoPlat("Received %d bytes", sReceiveFrame.mLength);
            otPlatRadioReceiveDone(aInstance, &sReceiveFrame, sReceiveError);
#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
            sRailDebugCounters.mRailPlatRadioReceiveDoneCbCount++;
#endif
        }
        otSysEventSignalPending();
    }
}

static void processTxComplete(otInstance *aInstance)
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
            if (((sTransmitFrame.mPsdu[0] & IEEE802154_FRAME_FLAG_ACK_REQUIRED) == 0) ||
                (sTransmitError != OT_ERROR_NONE))
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, sTransmitError);
        }
        else
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveAckFrame, sTransmitError);
        }

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailPlatRadioTxDoneCbCount++;
#endif
        otSysEventSignalPending();
    }
}

void efr32RadioProcess(otInstance *aInstance)
{
    (void)handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TICK, 0U);

    // We should process the received packet first. Adding it at the end of this function,
    // will delay the stack notification until the next call to efr32RadioProcess()
    processNextRxPacket(aInstance);
    processTxComplete(aInstance);

    if (sEnergyScanMode == ENERGY_SCAN_MODE_ASYNC && sEnergyScanStatus == ENERGY_SCAN_STATUS_COMPLETED)
    {
        sEnergyScanStatus = ENERGY_SCAN_STATUS_IDLE;
        otPlatRadioEnergyScanDone(aInstance, sEnergyScanResultDbm);
        otSysEventSignalPending();

#if RADIO_CONFIG_DEBUG_COUNTERS_SUPPORT
        sRailDebugCounters.mRailEventEnergyScanCompleted++;
#endif
    }
}

//------------------------------------------------------------------------------
// Antenn Diveristy, Wifi coexistence and Run time PHY select support

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT

otError setRadioState(otRadioState state)
{
    otError error = OT_ERROR_NONE;

    // Defer idling the radio if we have an ongoing TX task
    otEXPECT_ACTION((!getInternalFlag(ONGOING_TX_FLAGS)), error = OT_ERROR_FAILED);

    switch (state)
    {
    case OT_RADIO_STATE_RECEIVE:
        otEXPECT_ACTION(radioSetRx(sReceiveFrame.mChannel) == OT_ERROR_NONE, error = OT_ERROR_FAILED);
        break;
    case OT_RADIO_STATE_SLEEP:
        radioSetIdle();
        break;
    default:
        error = OT_ERROR_FAILED;
    }
exit:
    return error;
}

void sl_ot_update_active_radio_config(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    // Proceed with PHY selection only if 2.4 GHz band is used
    otEXPECT(sBandConfig.mChannelConfig == NULL);

    otRadioState currentState = sState;
    otEXPECT(setRadioState(OT_RADIO_STATE_SLEEP) == OT_ERROR_NONE);
    sl_rail_util_plugin_config_2p4ghz_radio(gRailHandle);
    otEXPECT(setRadioState(currentState) == OT_ERROR_NONE);

exit:
    CORE_EXIT_ATOMIC();
    return;
}
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_PHY_SELECT_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
void efr32AntennaConfigInit(void)
{
    RAIL_Status_t status;
    sl_rail_util_ant_div_init();
    status = sl_rail_util_ant_div_update_antenna_config();
    assert(status == RAIL_STATUS_NO_ERROR);
}
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

static void changeDynamicEvents(void)
{
    const RAIL_Events_t eventMask =
        RAIL_EVENTS_NONE | RAIL_EVENT_RX_SYNC1_DETECT | RAIL_EVENT_RX_SYNC2_DETECT | RAIL_EVENT_RX_FRAME_ERROR |
        RAIL_EVENT_RX_FIFO_OVERFLOW | RAIL_EVENT_RX_ADDRESS_FILTERED | RAIL_EVENT_RX_PACKET_ABORTED |
        RAIL_EVENT_RX_FILTER_PASSED | RAIL_EVENT_TX_CHANNEL_CLEAR | RAIL_EVENT_TX_CCA_RETRY | RAIL_EVENT_TX_START_CCA;
    RAIL_Events_t eventValues = RAIL_EVENTS_NONE;

    if (phyStackEventIsEnabled())
    {
        eventValues |= eventMask;
    }
    updateEvents(eventMask, eventValues);
}
#endif // SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT

static void efr32PhyStackInit(void)
{
#ifdef SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT
    efr32AntennaConfigInit();
#endif // SL_CATALOG_RAIL_UTIL_ANT_DIV_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT
    efr32CoexInit();
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

#ifdef SL_CATALOG_RAIL_UTIL_IEEE802154_STACK_EVENT_PRESENT
    changeDynamicEvents();
#endif
}

#ifdef SL_CATALOG_RAIL_UTIL_COEX_PRESENT

static void emRadioEnableAutoAck(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();

    if (getInternalFlag(FLAG_RADIO_INIT_DONE))
    {
        if ((rhoActive >= RHO_INT_ACTIVE) // Internal always holds ACKs
            || ((rhoActive > RHO_INACTIVE) && ((sl_rail_util_coex_get_options() & SL_RAIL_UTIL_COEX_OPT_ACK_HOLDOFF) !=
                                               SL_RAIL_UTIL_COEX_OPT_DISABLED)))
        {
            RAIL_PauseRxAutoAck(gRailHandle, true);
        }
        else
        {
            RAIL_PauseRxAutoAck(gRailHandle, false);
        }
    }
    CORE_EXIT_ATOMIC();
}

static void emRadioEnablePta(bool enable)
{
    halInternalInitPta();

    // When PTA is enabled, we want to negate PTA_REQ as soon as an incoming
    // frame is aborted, e.g. due to filtering.  To do that we must turn off
    // the TRACKABFRAME feature that's normally on to benefit sniffing on PTI.
    assert(RAIL_ConfigRxOptions(gRailHandle, RAIL_RX_OPTION_TRACK_ABORTED_FRAMES,
                                (enable ? RAIL_RX_OPTIONS_NONE : RAIL_RX_OPTION_TRACK_ABORTED_FRAMES)) ==
           RAIL_STATUS_NO_ERROR);
}

static void efr32CoexInit(void)
{
    sl_rail_util_coex_options_t coexOptions = sl_rail_util_coex_get_options();

#if SL_OPENTHREAD_COEX_MAC_HOLDOFF_ENABLE
    coexOptions |= SL_RAIL_UTIL_COEX_OPT_MAC_HOLDOFF;
#endif // SL_OPENTHREAD_COEX_MAC_HOLDOFF_ENABLE

    sl_rail_util_coex_set_options(coexOptions);

    emRadioEnableAutoAck(); // Might suspend AutoACK if RHO already in effect
    emRadioEnablePta(sl_rail_util_coex_is_enabled());
}

// Managing radio transmission
static void onPtaGrantTx(sl_rail_util_coex_req_t ptaStatus)
{
    // Only pay attention to first PTA Grant callback, ignore any further ones
    if (ptaGntEventReported)
    {
        return;
    }
    ptaGntEventReported = true;

    assert(ptaStatus == SL_RAIL_UTIL_COEX_REQCB_GRANTED);
    // PTA is telling us we've gotten GRANT and should send ASAP *without* CSMA
    setInternalFlag(FLAG_CURRENT_TX_USE_CSMA, false);
    txCurrentPacket();
}

static void tryTxCurrentPacket(void)
{
    assert(getInternalFlag(FLAG_ONGOING_TX_DATA));

    ptaGntEventReported = false;
    sl_rail_util_ieee802154_stack_event_t ptaStatus =
        handlePhyStackEvent(SL_RAIL_UTIL_IEEE802154_STACK_EVENT_TX_PENDED_MAC, (uint32_t)&onPtaGrantTx);
    if (ptaStatus == SL_RAIL_UTIL_IEEE802154_STACK_STATUS_SUCCESS)
    {
        // Normal case where PTA allows us to start the (CSMA) transmit below
        txCurrentPacket();
    }
    else if (ptaStatus == SL_RAIL_UTIL_IEEE802154_STACK_STATUS_CB_PENDING)
    {
        // onPtaGrantTx() callback will take over (and might already have)
    }
    else if (ptaStatus == SL_RAIL_UTIL_IEEE802154_STACK_STATUS_HOLDOFF)
    {
        txFailedCallback(false, TX_COMPLETE_RESULT_OTHER_FAIL);
    }
}

// Managing CCA Threshold
static void setCcaThreshold(void)
{
    if (sCcaThresholdDbm == CCA_THRESHOLD_UNINIT)
    {
        sCcaThresholdDbm = CCA_THRESHOLD_DEFAULT;
    }
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    int8_t thresholddBm = sCcaThresholdDbm;

    if (getInternalFlag(FLAG_RADIO_INIT_DONE))
    {
        if (rhoActive > RHO_INACTIVE)
        {
            thresholddBm = RAIL_RSSI_INVALID_DBM;
        }
        assert(RAIL_SetCcaThreshold(gRailHandle, thresholddBm) == RAIL_STATUS_NO_ERROR);
    }
    CORE_EXIT_ATOMIC();
}

static void emRadioHoldOffInternalIsr(uint8_t active)
{
    if (active != rhoActive)
    {
        rhoActive = active; // Update rhoActive early
        if (getInternalFlag(FLAG_RADIO_INIT_DONE))
        {
            setCcaThreshold();
            emRadioEnableAutoAck();
        }
    }
}

// External API used by Coex Component
void emRadioHoldOffIsr(bool active)
{
    emRadioHoldOffInternalIsr((uint8_t)active | (rhoActive & ~RHO_EXT_ACTIVE));
}

#if SL_OPENTHREAD_COEX_COUNTER_ENABLE

void sl_rail_util_coex_counter_on_event(sl_rail_util_coex_event_t event)
{
    otEXPECT(event < SL_RAIL_UTIL_COEX_EVENT_COUNT);
    sCoexCounters[event] += 1;
exit:
    return;
}

void efr32RadioGetCoexCounters(uint32_t (*aCoexCounters)[SL_RAIL_UTIL_COEX_EVENT_COUNT])
{
    memset((void *)aCoexCounters, 0, sizeof(*aCoexCounters));
    memcpy(aCoexCounters, sCoexCounters, sizeof(*aCoexCounters));
}

void efr32RadioClearCoexCounters(void)
{
    memset((void *)sCoexCounters, 0, sizeof(sCoexCounters));
}

#endif // SL_OPENTHREAD_COEX_COUNTER_ENABLE
#endif // SL_CATALOG_RAIL_UTIL_COEX_PRESENT

RAIL_AntennaConfig_t halAntennaConfig;

void initAntenna(void)
{
#if (HAL_ANTDIV_ENABLE && defined(BSP_ANTDIV_SEL_PORT) && defined(BSP_ANTDIV_SEL_PIN) && defined(BSP_ANTDIV_SEL_LOC))
    halAntennaConfig.ant0PinEn = true;
    halAntennaConfig.ant0Port  = (uint8_t)BSP_ANTDIV_SEL_PORT;
    halAntennaConfig.ant0Pin   = BSP_ANTDIV_SEL_PIN;
    halAntennaConfig.ant0Loc   = BSP_ANTDIV_SEL_LOC;
#endif
#ifdef _SILICON_LABS_32B_SERIES_2
    halAntennaConfig.defaultPath = BSP_ANTDIV_SEL_LOC;
#endif
#if (HAL_ANTDIV_ENABLE && defined(BSP_ANTDIV_NSEL_PORT) && defined(BSP_ANTDIV_NSEL_PIN) && defined(BSP_ANTDIV_NSEL_LOC))
    halAntennaConfig.ant1PinEn = true;
    halAntennaConfig.ant1Port  = (uint8_t)BSP_ANTDIV_NSEL_PORT;
    halAntennaConfig.ant1Pin   = BSP_ANTDIV_NSEL_PIN;
    halAntennaConfig.ant1Loc   = BSP_ANTDIV_NSEL_LOC;
#endif
#if (HAL_ANTDIV_ENABLE || defined(_SILICON_LABS_32B_SERIES_2))
    (void)RAIL_ConfigAntenna(RAIL_EFR32_HANDLE, &halAntennaConfig);
#endif
}
