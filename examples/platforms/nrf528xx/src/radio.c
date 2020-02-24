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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils/code_utils.h"
#include "utils/mac_frame.h"

#include <platform-config.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "openthread-system.h"
#include "platform-nrf5.h"

#include <nrf.h>
#include <nrf_802154.h>

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/random_noncrypto.h>

// clang-format off

#define SHORT_ADDRESS_SIZE    2            ///< Size of MAC short address.
#define US_PER_MS             1000ULL      ///< Microseconds in millisecond.

#define ACK_REQUEST_OFFSET    1            ///< Byte containing Ack request bit (+1 for frame length byte).
#define ACK_REQUEST_BIT       (1 << 5)     ///< Ack request bit.
#define FRAME_PENDING_OFFSET  1            ///< Byte containing pending bit (+1 for frame length byte).
#define FRAME_PENDING_BIT     (1 << 4)     ///< Frame Pending bit.

#define RSSI_SETTLE_TIME_US   40           ///< RSSI settle time in microseconds.

#if defined(__ICCARM__)
_Pragma("diag_suppress=Pe167")
#endif

enum
{
    NRF528XX_RECEIVE_SENSITIVITY  = -100, // dBm
    NRF528XX_MIN_CCA_ED_THRESHOLD = -94,  // dBm
};

// clang-format on

static bool sDisabled;

static otError      sReceiveError = OT_ERROR_NONE;
static otRadioFrame sReceivedFrames[NRF_802154_RX_BUFFERS];
static otRadioFrame sTransmitFrame;
static uint8_t      sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE + 1];

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
static otExtAddress  sExtAddress;
static otRadioIeInfo sTransmitIeInfo;
static otInstance *  sInstance = NULL;
#endif

static otRadioFrame sAckFrame;
static bool         sAckedWithFramePending;

static int8_t sDefaultTxPower;

static uint32_t sEnergyDetectionTime;
static uint8_t  sEnergyDetectionChannel;
static int8_t   sEnergyDetected;

typedef enum
{
    kPendingEventSleep,                // Requested to enter Sleep state.
    kPendingEventFrameTransmitted,     // Transmitted frame and received ACK (if requested).
    kPendingEventChannelAccessFailure, // Failed to transmit frame (channel busy).
    kPendingEventInvalidOrNoAck,       // Failed to transmit frame (received invalid or no ACK).
    kPendingEventReceiveFailed,        // Failed to receive a valid frame.
    kPendingEventEnergyDetectionStart, // Requested to start Energy Detection procedure.
    kPendingEventEnergyDetected,       // Energy Detection finished.
} RadioPendingEvents;

static uint32_t sPendingEvents;

static void dataInit(void)
{
    sDisabled = true;

    sTransmitFrame.mPsdu = sTransmitPsdu + 1;
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = &sTransmitIeInfo;
#endif

    sReceiveError = OT_ERROR_NONE;

    for (uint32_t i = 0; i < NRF_802154_RX_BUFFERS; i++)
    {
        sReceivedFrames[i].mPsdu = NULL;
    }

    memset(&sAckFrame, 0, sizeof(sAckFrame));
}

static void convertShortAddress(uint8_t *aTo, uint16_t aFrom)
{
    aTo[0] = (uint8_t)aFrom;
    aTo[1] = (uint8_t)(aFrom >> 8);
}

static inline bool isPendingEventSet(RadioPendingEvents aEvent)
{
    return sPendingEvents & (1UL << aEvent);
}

static void setPendingEvent(RadioPendingEvents aEvent)
{
    volatile uint32_t pendingEvents;
    uint32_t          bitToSet = 1UL << aEvent;

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents |= bitToSet;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));

    otSysEventSignalPending();
}

static void resetPendingEvent(RadioPendingEvents aEvent)
{
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(1UL << aEvent);

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents &= bitsToRemain;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));
}

static inline void clearPendingEvents(void)
{
    // Clear pending events that could cause race in the MAC layer.
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(0UL);

    bitsToRemain &= ~(1UL << kPendingEventSleep);

    do
    {
        pendingEvents = __LDREXW((uint32_t *)&sPendingEvents);
        pendingEvents &= bitsToRemain;
    } while (__STREXW(pendingEvents, (uint32_t *)&sPendingEvents));
}

#if !OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint64_t factoryAddress;
    uint32_t index = 0;

    // Set the MAC Address Block Larger (MA-L) formerly called OUI.
    aIeeeEui64[index++] = (OPENTHREAD_CONFIG_STACK_VENDOR_OUI >> 16) & 0xff;
    aIeeeEui64[index++] = (OPENTHREAD_CONFIG_STACK_VENDOR_OUI >> 8) & 0xff;
    aIeeeEui64[index++] = OPENTHREAD_CONFIG_STACK_VENDOR_OUI & 0xff;

    // Use device identifier assigned during the production.
    factoryAddress = (uint64_t)NRF_FICR->DEVICEID[0] << 32;
    factoryAddress |= NRF_FICR->DEVICEID[1];
    memcpy(aIeeeEui64 + index, &factoryAddress, sizeof(factoryAddress) - index);
}
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t address[SHORT_ADDRESS_SIZE];
    convertShortAddress(address, aPanId);

    nrf_802154_pan_id_set(address);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    for (size_t i = 0; i < sizeof(*aExtAddress); i++)
    {
        sExtAddress.m8[i] = aExtAddress->m8[sizeof(*aExtAddress) - 1 - i];
    }
#endif
    nrf_802154_extended_address_set(aExtAddress->m8);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint8_t address[SHORT_ADDRESS_SIZE];
    convertShortAddress(address, aShortAddress);

    nrf_802154_short_address_set(address);
}

void nrf5RadioInit(void)
{
    dataInit();
    nrf_802154_init();
}

void nrf5RadioDeinit(void)
{
    nrf_802154_sleep();
    nrf_802154_deinit();
    sPendingEvents = 0;
}

void nrf5RadioClearPendingEvents(void)
{
    sPendingEvents = 0;

    for (uint32_t i = 0; i < NRF_802154_RX_BUFFERS; i++)
    {
        if (sReceivedFrames[i].mPsdu != NULL)
        {
            uint8_t *bufferAddress   = &sReceivedFrames[i].mPsdu[-1];
            sReceivedFrames[i].mPsdu = NULL;
            nrf_802154_buffer_free_raw(bufferAddress);
        }
    }
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sDisabled)
    {
        return OT_RADIO_STATE_DISABLED;
    }

    switch (nrf_802154_state_get())
    {
    case NRF_802154_STATE_SLEEP:
        return OT_RADIO_STATE_SLEEP;

    case NRF_802154_STATE_RECEIVE:
    case NRF_802154_STATE_ENERGY_DETECTION:
        return OT_RADIO_STATE_RECEIVE;

    case NRF_802154_STATE_TRANSMIT:
    case NRF_802154_STATE_CCA:
    case NRF_802154_STATE_CONTINUOUS_CARRIER:
        return OT_RADIO_STATE_TRANSMIT;

    default:
        assert(false); // Make sure driver returned valid state.
    }

    return OT_RADIO_STATE_RECEIVE; // It is the default state. Return it in case of unknown.
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return !sDisabled;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otError error;

#if !OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    OT_UNUSED_VARIABLE(aInstance);
#else
    sInstance = aInstance;
#endif

    if (sDisabled)
    {
        sDisabled = false;
        error     = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_INVALID_STATE;
    }

    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT(otPlatRadioIsEnabled(aInstance));
    otEXPECT_ACTION(otPlatRadioGetState(aInstance) == OT_RADIO_STATE_SLEEP || isPendingEventSet(kPendingEventSleep),
                    error = OT_ERROR_INVALID_STATE);

    sDisabled = true;

exit:
    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (nrf_802154_sleep())
    {
        clearPendingEvents();
    }
    else
    {
        clearPendingEvents();
        setPendingEvent(kPendingEventSleep);
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    bool result;

    nrf_802154_channel_set(aChannel);
    nrf_802154_tx_power_set(sDefaultTxPower);
    result = nrf_802154_receive();
    clearPendingEvents();

    return result ? OT_ERROR_NONE : OT_ERROR_INVALID_STATE;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    bool result = true;

    aFrame->mPsdu[-1] = aFrame->mLength;

    nrf_802154_channel_set(aFrame->mChannel);

    if (aFrame->mInfo.mTxInfo.mCsmaCaEnabled)
    {
        nrf_802154_transmit_csma_ca_raw(&aFrame->mPsdu[-1]);
    }
    else
    {
        result = nrf_802154_transmit_raw(&aFrame->mPsdu[-1], false);
    }

    clearPendingEvents();
    otPlatRadioTxStarted(aInstance, aFrame);

    if (!result)
    {
        setPendingEvent(kPendingEventChannelAccessFailure);
    }

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    // Ensure the RSSI measurement is done after RSSI settling time.
    // This is necessary for the Channel Monitor feature which quickly switches between channels.
    NRFX_DELAY_US(RSSI_SETTLE_TIME_US);

    nrf_802154_rssi_measure_begin();

    return nrf_802154_rssi_last_get();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (otRadioCaps)(OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF);
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return nrf_802154_promiscuous_get();
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    nrf_802154_promiscuous_set(aEnable);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    nrf_802154_auto_pending_bit_set(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    uint8_t shortAddress[SHORT_ADDRESS_SIZE];
    convertShortAddress(shortAddress, aShortAddress);

    if (nrf_802154_pending_bit_for_addr_set(shortAddress, false))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_BUFS;
    }

    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    if (nrf_802154_pending_bit_for_addr_set(aExtAddress->m8, true))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_BUFS;
    }

    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    uint8_t shortAddress[SHORT_ADDRESS_SIZE];
    convertShortAddress(shortAddress, aShortAddress);

    if (nrf_802154_pending_bit_for_addr_clear(shortAddress, false))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_ADDRESS;
    }

    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

    if (nrf_802154_pending_bit_for_addr_clear(aExtAddress->m8, true))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_ADDRESS;
    }

    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    nrf_802154_pending_bit_for_addr_reset(false);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    nrf_802154_pending_bit_for_addr_reset(true);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    sEnergyDetectionTime    = (uint32_t)aScanDuration * 1000UL;
    sEnergyDetectionChannel = aScanChannel;

    clearPendingEvents();

    nrf_802154_channel_set(aScanChannel);

    if (nrf_802154_energy_detection(sEnergyDetectionTime))
    {
        resetPendingEvent(kPendingEventEnergyDetectionStart);
    }
    else
    {
        setPendingEvent(kPendingEventEnergyDetectionStart);
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    if (aPower == NULL)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        *aPower = nrf_802154_tx_power_get();
    }

    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    sDefaultTxPower = aPower;
    nrf_802154_tx_power_set(aPower);

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError              error = OT_ERROR_NONE;
    nrf_802154_cca_cfg_t ccaConfig;

    if (aThreshold == NULL)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        nrf_802154_cca_cfg_get(&ccaConfig);
        // The radio driver has no function to convert ED threshold to dBm
        *aThreshold = (int8_t)ccaConfig.ed_threshold + NRF528XX_MIN_CCA_ED_THRESHOLD;
    }

    return error;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError              error = OT_ERROR_NONE;
    nrf_802154_cca_cfg_t ccaConfig;

    // The minimum value of ED threshold for radio driver is -94 dBm
    if (aThreshold < NRF528XX_MIN_CCA_ED_THRESHOLD)
    {
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        memset(&ccaConfig, 0, sizeof(ccaConfig));
        ccaConfig.mode         = NRF_RADIO_CCA_MODE_ED;
        ccaConfig.ed_threshold = nrf_802154_ccaedthres_from_dbm_calculate(aThreshold);

        nrf_802154_cca_cfg_set(&ccaConfig);
    }

    return error;
}

void nrf5RadioProcess(otInstance *aInstance)
{
    bool isEventPending = false;

    for (uint32_t i = 0; i < NRF_802154_RX_BUFFERS; i++)
    {
        if (sReceivedFrames[i].mPsdu != NULL)
        {
#if OPENTHREAD_CONFIG_DIAG_ENABLE

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioReceiveDone(aInstance, &sReceivedFrames[i], OT_ERROR_NONE);
            }
            else
#endif
            {
                otPlatRadioReceiveDone(aInstance, &sReceivedFrames[i], OT_ERROR_NONE);
            }

            uint8_t *bufferAddress   = &sReceivedFrames[i].mPsdu[-1];
            sReceivedFrames[i].mPsdu = NULL;
            nrf_802154_buffer_free_raw(bufferAddress);
        }
    }

    if (isPendingEventSet(kPendingEventFrameTransmitted))
    {
        resetPendingEvent(kPendingEventFrameTransmitted);

#if OPENTHREAD_CONFIG_DIAG_ENABLE

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otRadioFrame *ackPtr = (sAckFrame.mPsdu == NULL) ? NULL : &sAckFrame;
            otPlatRadioTxDone(aInstance, &sTransmitFrame, ackPtr, OT_ERROR_NONE);
        }

        if (sAckFrame.mPsdu != NULL)
        {
            nrf_802154_buffer_free_raw(sAckFrame.mPsdu - 1);
            sAckFrame.mPsdu = NULL;
        }
    }

    if (isPendingEventSet(kPendingEventChannelAccessFailure))
    {
        resetPendingEvent(kPendingEventChannelAccessFailure);

#if OPENTHREAD_CONFIG_DIAG_ENABLE

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_CHANNEL_ACCESS_FAILURE);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_CHANNEL_ACCESS_FAILURE);
        }
    }

    if (isPendingEventSet(kPendingEventInvalidOrNoAck))
    {
        resetPendingEvent(kPendingEventInvalidOrNoAck);

#if OPENTHREAD_CONFIG_DIAG_ENABLE

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NO_ACK);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NO_ACK);
        }
    }

    if (isPendingEventSet(kPendingEventReceiveFailed))
    {
        resetPendingEvent(kPendingEventReceiveFailed);

#if OPENTHREAD_CONFIG_DIAG_ENABLE

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(aInstance, NULL, sReceiveError);
        }
        else
#endif
        {
            otPlatRadioReceiveDone(aInstance, NULL, sReceiveError);
        }
    }

    if (isPendingEventSet(kPendingEventEnergyDetected))
    {
        resetPendingEvent(kPendingEventEnergyDetected);

        otPlatRadioEnergyScanDone(aInstance, sEnergyDetected);
    }

    if (isPendingEventSet(kPendingEventSleep))
    {
        if (nrf_802154_sleep())
        {
            resetPendingEvent(kPendingEventSleep);
        }
        else
        {
            isEventPending = true;
        }
    }

    if (isPendingEventSet(kPendingEventEnergyDetectionStart))
    {
        nrf_802154_channel_set(sEnergyDetectionChannel);

        if (nrf_802154_energy_detection(sEnergyDetectionTime))
        {
            resetPendingEvent(kPendingEventEnergyDetectionStart);
        }
        else
        {
            isEventPending = true;
        }
    }

    if (isEventPending)
    {
        otSysEventSignalPending();
    }
}

void nrf_802154_received_timestamp_raw(uint8_t *p_data, int8_t power, uint8_t lqi, uint32_t time)
{
    otRadioFrame *receivedFrame = NULL;

    for (uint32_t i = 0; i < NRF_802154_RX_BUFFERS; i++)
    {
        if (sReceivedFrames[i].mPsdu == NULL)
        {
            receivedFrame = &sReceivedFrames[i];

            memset(receivedFrame, 0, sizeof(*receivedFrame));
            break;
        }
    }

    assert(receivedFrame != NULL);

    receivedFrame->mPsdu               = &p_data[1];
    receivedFrame->mLength             = p_data[0];
    receivedFrame->mInfo.mRxInfo.mRssi = power;
    receivedFrame->mInfo.mRxInfo.mLqi  = lqi;
    receivedFrame->mChannel            = nrf_802154_channel_get();

    // Inform if this frame was acknowledged with frame pending set.
    if (p_data[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT)
    {
        receivedFrame->mInfo.mRxInfo.mAckedWithFramePending = sAckedWithFramePending;
    }
    else
    {
        receivedFrame->mInfo.mRxInfo.mAckedWithFramePending = false;
    }

    // Get the timestamp when the SFD was received.
#if !NRF_802154_TX_STARTED_NOTIFY_ENABLED
#error "NRF_802154_TX_STARTED_NOTIFY_ENABLED is required!"
#endif
    uint32_t offset =
        (int32_t)otPlatAlarmMicroGetNow() - (int32_t)nrf_802154_first_symbol_timestamp_get(time, p_data[0]);
    receivedFrame->mInfo.mRxInfo.mTimestamp = nrf5AlarmGetCurrentTime() - offset;

    sAckedWithFramePending = false;

    otSysEventSignalPending();
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error)
{
    switch (error)
    {
    case NRF_802154_RX_ERROR_INVALID_FRAME:
    case NRF_802154_RX_ERROR_DELAYED_TIMEOUT:
        sReceiveError = OT_ERROR_NO_FRAME_RECEIVED;
        break;

    case NRF_802154_RX_ERROR_INVALID_FCS:
        sReceiveError = OT_ERROR_FCS;
        break;

    case NRF_802154_RX_ERROR_INVALID_DEST_ADDR:
        sReceiveError = OT_ERROR_DESTINATION_ADDRESS_FILTERED;
        break;

    case NRF_802154_RX_ERROR_RUNTIME:
    case NRF_802154_RX_ERROR_TIMESLOT_ENDED:
    case NRF_802154_RX_ERROR_ABORTED:
    case NRF_802154_RX_ERROR_DELAYED_TIMESLOT_DENIED:
    case NRF_802154_RX_ERROR_INVALID_LENGTH:
        sReceiveError = OT_ERROR_FAILED;
        break;

    default:
        assert(false);
    }

    sAckedWithFramePending = false;

    setPendingEvent(kPendingEventReceiveFailed);
}

void nrf_802154_tx_ack_started(const uint8_t *p_data)
{
    // Check if the frame pending bit is set in ACK frame.
    sAckedWithFramePending = p_data[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT;
}

void nrf_802154_transmitted_raw(const uint8_t *aFrame, uint8_t *aAckPsdu, int8_t aPower, uint8_t aLqi)
{
    assert(aFrame == sTransmitPsdu);

    if (aAckPsdu == NULL)
    {
        sAckFrame.mPsdu = NULL;
    }
    else
    {
        sAckFrame.mPsdu               = &aAckPsdu[1];
        sAckFrame.mLength             = aAckPsdu[0];
        sAckFrame.mInfo.mRxInfo.mRssi = aPower;
        sAckFrame.mInfo.mRxInfo.mLqi  = aLqi;
        sAckFrame.mChannel            = nrf_802154_channel_get();
    }

    setPendingEvent(kPendingEventFrameTransmitted);
}

void nrf_802154_transmit_failed(const uint8_t *aFrame, nrf_802154_tx_error_t error)
{
    assert(aFrame == sTransmitPsdu);

    switch (error)
    {
    case NRF_802154_TX_ERROR_BUSY_CHANNEL:
    case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
    case NRF_802154_TX_ERROR_ABORTED:
    case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
        setPendingEvent(kPendingEventChannelAccessFailure);
        break;

    case NRF_802154_TX_ERROR_INVALID_ACK:
    case NRF_802154_TX_ERROR_NO_ACK:
    case NRF_802154_TX_ERROR_NO_MEM:
        setPendingEvent(kPendingEventInvalidOrNoAck);
        break;

    default:
        assert(false);
    }
}

void nrf_802154_energy_detected(uint8_t result)
{
    sEnergyDetected = nrf_802154_dbm_from_energy_level_calculate(result);

    setPendingEvent(kPendingEventEnergyDetected);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return NRF528XX_RECEIVE_SENSITIVITY;
}

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
void nrf_802154_tx_started(const uint8_t *aFrame)
{
    bool notifyFrameUpdated = false;
    assert(aFrame == sTransmitPsdu);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset != 0)
    {
        uint8_t *timeIe = sTransmitFrame.mPsdu + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset;
        uint64_t time   = otPlatTimeGet() + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset;

        *timeIe = sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeSyncSeq;

        *(++timeIe) = (uint8_t)(time & 0xff);
        for (uint8_t i = 1; i < sizeof(uint64_t); i++)
        {
            time        = time >> 8;
            *(++timeIe) = (uint8_t)(time & 0xff);
        }

        notifyFrameUpdated = true;
    }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    if (notifyFrameUpdated)
    {
        otMacFrameProcessTransmitAesCcm(&sTransmitFrame, &sExtAddress);
    }
}
#endif

void nrf_802154_random_init(void)
{
    // Intentionally empty
}

void nrf_802154_random_deinit(void)
{
    // Intentionally empty
}

uint32_t nrf_802154_random_get(void)
{
    return otRandomNonCryptoGetUint32();
}
