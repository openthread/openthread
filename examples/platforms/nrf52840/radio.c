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
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */

#include <openthread/openthread_config.h>
// NRF tools use #define PACKAGE - for other purposes
// ie: the physical package the chip comes in
// This conflicts with the GNU Autoconf "PACAKGE" define
#undef PACKAGE

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <platform-config.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>

#include "device/nrf.h"
#include "drivers/nrf_drv_radio802154.h"

#include <openthread-core-config.h>
#include <openthread/openthread_config.h>
#include <openthread/types.h>

#define SHORT_ADDRESS_SIZE    2
#define EXTENDED_ADDRESS_SIZE 8
#define PENDING_BIT           0x10

enum
{
    NRF52840_RECEIVE_SENSITIVITY = -100,  // dBm
};

static bool sDisabled;

static otRadioFrame sReceivedFrames[RADIO_RX_BUFFERS];
static otRadioFrame sTransmitFrame;
static uint8_t      sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE + 1]
__attribute__((section("nrf_radio_buffer.sTransmiPsdu")));
static otRadioFrame sAckFrame;

static uint32_t     sEnergyDetectionTime;
static uint8_t      sEnergyDetectionChannel;
static int8_t       sEnergyDetected;

typedef enum
{
    kPendingEventSleep,                // Requested to enter Sleep state.
    kPendingEventFrameTransmitted,     // Transmitted frame and received ACK (if requested).
    kPendingEventChannelAccessFailure, // Failed to transmit frame (channel busy).
    kPendingEventEnergyDetectionStart, // Requested to start Energy Detection procedure.
    kPendingEventEnergyDetected,       // Energy Detection finished.
} RadioPendingEvents;

static uint32_t sPendingEvents;

static void dataInit(void)
{
    sDisabled = true;

    sTransmitFrame.mPsdu = sTransmitPsdu + 1;

    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        sReceivedFrames[i].mPsdu = NULL;
    }

    memset(&sAckFrame, 0, sizeof(sAckFrame));
}

static void convertShortAddress(uint8_t *aTo, uint16_t aFrom)
{
    aTo[0] = (uint8_t) aFrom;
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
        pendingEvents = __LDREXW(&sPendingEvents);
        pendingEvents |= bitToSet;
    }
    while (__STREXW(pendingEvents, &sPendingEvents));
}

static void resetPendingEvent(RadioPendingEvents aEvent)
{
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(1UL << aEvent);

    do
    {
        pendingEvents = __LDREXW(&sPendingEvents);
        pendingEvents &= bitsToRemain;
    }
    while (__STREXW(pendingEvents, &sPendingEvents));
}

static inline void clearPendingEvents(void)
{
    // Clear pending events that could cause race in the MAC layer.
    volatile uint32_t pendingEvents;
    uint32_t          bitsToRemain = ~(0UL);

    bitsToRemain &= ~(1UL << kPendingEventFrameTransmitted);
    bitsToRemain &= ~(1UL << kPendingEventChannelAccessFailure);

    bitsToRemain &= ~(1UL << kPendingEventSleep);

    do
    {
        pendingEvents = __LDREXW(&sPendingEvents);
        pendingEvents &= bitsToRemain;
    }
    while (__STREXW(pendingEvents, &sPendingEvents));
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void) aInstance;

    uint64_t factoryAddress;
    factoryAddress = (((uint64_t)NRF_FICR->DEVICEID[0] << 32) | NRF_FICR->DEVICEID[1]);

    memcpy(aIeeeEui64, &factoryAddress, sizeof(factoryAddress));
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    (void) aInstance;

    uint8_t address[SHORT_ADDRESS_SIZE];
    convertShortAddress(address, aPanId);

    nrf_drv_radio802154_pan_id_set(address);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aExtendedAddress)
{
    (void) aInstance;

    nrf_drv_radio802154_extended_address_set(aExtendedAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    (void) aInstance;

    uint8_t address[SHORT_ADDRESS_SIZE];
    convertShortAddress(address, aShortAddress);

    nrf_drv_radio802154_short_address_set(address);
}

void nrf5RadioInit(void)
{
    dataInit();
    nrf_drv_radio802154_init();
}

void nrf5RadioDeinit(void)
{
    nrf_drv_radio802154_deinit();
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    (void) aInstance;

    if (sDisabled)
    {
        return OT_RADIO_STATE_DISABLED;
    }

    switch (nrf_drv_radio802154_state_get())
    {
    case NRF_DRV_RADIO802154_STATE_SLEEP:
        return OT_RADIO_STATE_SLEEP;

    case NRF_DRV_RADIO802154_STATE_RECEIVE:
    case NRF_DRV_RADIO802154_STATE_ENERGY_DETECTION:
        return OT_RADIO_STATE_RECEIVE;

    case NRF_DRV_RADIO802154_STATE_TRANSMIT:
        return OT_RADIO_STATE_TRANSMIT;

    default:
        assert(false); // Make sure driver returned valid state.
    }

    return OT_RADIO_STATE_RECEIVE; // It is the default state. Return it in case of unknown.
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    (void) aInstance;

    otError error;

    if (sDisabled)
    {
        sDisabled = false;
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_INVALID_STATE;
    }

    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    (void) aInstance;

    otError error;

    if (!sDisabled)
    {
        sDisabled = true;
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_INVALID_STATE;
    }

    return error;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void) aInstance;

    return !sDisabled;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    (void) aInstance;

    if (nrf_drv_radio802154_sleep())
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
    (void) aInstance;

    nrf_drv_radio802154_receive(aChannel);
    clearPendingEvents();

    return OT_ERROR_NONE;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    (void) aInstance;

    aFrame->mPsdu[-1] = aFrame->mLength;

    if (nrf_drv_radio802154_transmit(&aFrame->mPsdu[-1], aFrame->mChannel, aFrame->mPower))
    {
        clearPendingEvents();
    }
    else
    {
        clearPendingEvents();
        setPendingEvent(kPendingEventChannelAccessFailure);
    }

    return OT_ERROR_NONE;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void) aInstance;

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void) aInstance;

    return nrf_drv_radio802154_rssi_last_get();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void) aInstance;

    return OT_RADIO_CAPS_ENERGY_SCAN;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void) aInstance;

    return nrf_drv_radio802154_promiscuous_get();
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void) aInstance;

    nrf_drv_radio802154_promiscuous_set(aEnable);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void) aInstance;

    nrf_drv_radio802154_auto_pending_bit_set(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void) aInstance;

    otError error;

    uint8_t shortAddress[SHORT_ADDRESS_SIZE];
    convertShortAddress(shortAddress, aShortAddress);

    if (nrf_drv_radio802154_pending_bit_for_addr_set(shortAddress, false))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_BUFS;
    }

    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void) aInstance;

    otError error;

    if (nrf_drv_radio802154_pending_bit_for_addr_set(aExtAddress, true))
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
    (void) aInstance;

    otError error;

    uint8_t shortAddress[SHORT_ADDRESS_SIZE];
    convertShortAddress(shortAddress, aShortAddress);

    if (nrf_drv_radio802154_pending_bit_for_addr_clear(shortAddress, false))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_ADDRESS;
    }

    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void) aInstance;

    otError error;

    if (nrf_drv_radio802154_pending_bit_for_addr_clear(aExtAddress, true))
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
    (void) aInstance;

    nrf_drv_radio802154_pending_bit_for_addr_reset(false);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void) aInstance;

    nrf_drv_radio802154_pending_bit_for_addr_reset(true);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void) aInstance;

    sEnergyDetectionTime    = (uint32_t) aScanDuration * 1000UL;
    sEnergyDetectionChannel = aScanChannel;

    clearPendingEvents();

    if (nrf_drv_radio802154_energy_detection(aScanChannel, sEnergyDetectionTime))
    {
        resetPendingEvent(kPendingEventEnergyDetectionStart);
    }
    else
    {
        setPendingEvent(kPendingEventEnergyDetectionStart);
    }

    return OT_ERROR_NONE;
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    (void)aInstance;

    nrf_drv_radio802154_ack_tx_power_set(aPower);
}

void nrf5RadioProcess(otInstance *aInstance)
{
    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        if (sReceivedFrames[i].mPsdu != NULL)
        {
#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagRadioReceiveDone(aInstance, &sReceivedFrames[i], OT_ERROR_NONE);
            }
            else
#endif
            {
                otPlatRadioReceiveDone(aInstance, &sReceivedFrames[i], OT_ERROR_NONE);
            }

            uint8_t *bufferAddress = &sReceivedFrames[i].mPsdu[-1];
            sReceivedFrames[i].mPsdu = NULL;
            nrf_drv_radio802154_buffer_free(bufferAddress);
        }
    }

    if (isPendingEventSet(kPendingEventFrameTransmitted))
    {
#if OPENTHREAD_ENABLE_DIAG

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
            nrf_drv_radio802154_buffer_free(sAckFrame.mPsdu - 1);
            sAckFrame.mPsdu = NULL;
        }

        resetPendingEvent(kPendingEventFrameTransmitted);
    }

    if (isPendingEventSet(kPendingEventChannelAccessFailure))
    {
#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_CHANNEL_ACCESS_FAILURE);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_CHANNEL_ACCESS_FAILURE);
        }

        resetPendingEvent(kPendingEventChannelAccessFailure);
    }

    if (isPendingEventSet(kPendingEventEnergyDetected))
    {
        otPlatRadioEnergyScanDone(aInstance, sEnergyDetected);

        resetPendingEvent(kPendingEventEnergyDetected);
    }

    if (isPendingEventSet(kPendingEventSleep))
    {
        if (nrf_drv_radio802154_sleep())
        {
            resetPendingEvent(kPendingEventSleep);
        }
    }

    if (isPendingEventSet(kPendingEventEnergyDetectionStart))
    {
        if (nrf_drv_radio802154_energy_detection(sEnergyDetectionChannel, sEnergyDetectionTime))
        {
            resetPendingEvent(kPendingEventEnergyDetectionStart);
        }
    }
}

void nrf_drv_radio802154_received(uint8_t *p_data, int8_t power, int8_t lqi)
{
    otRadioFrame *receivedFrame = NULL;

    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        if (sReceivedFrames[i].mPsdu == NULL)
        {
            receivedFrame = &sReceivedFrames[i];
            break;
        }
    }

    assert(receivedFrame != NULL);

    memset(receivedFrame, 0, sizeof(*receivedFrame));

    receivedFrame->mPsdu    = &p_data[1];
    receivedFrame->mLength  = p_data[0];
    receivedFrame->mPower   = power;
    receivedFrame->mLqi     = lqi;
    receivedFrame->mChannel = nrf_drv_radio802154_channel_get();
}

void nrf_drv_radio802154_transmitted(uint8_t *aAckPsdu, int8_t aPower, int8_t aLqi)
{
    if (aAckPsdu == NULL)
    {
        sAckFrame.mPsdu = NULL;
    }
    else
    {
        sAckFrame.mPsdu    = &aAckPsdu[1];
        sAckFrame.mLength  = aAckPsdu[0];
        sAckFrame.mPower   = aPower;
        sAckFrame.mLqi     = aLqi;
        sAckFrame.mChannel = nrf_drv_radio802154_channel_get();
    }

    setPendingEvent(kPendingEventFrameTransmitted);
}

void nrf_drv_radio802154_busy_channel(void)
{
    setPendingEvent(kPendingEventChannelAccessFailure);
}

void nrf_drv_radio802154_energy_detected(int8_t result)
{
    sEnergyDetected = result;

    setPendingEvent(kPendingEventEnergyDetected);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    (void)aInstance;
    return NRF52840_RECEIVE_SENSITIVITY;
}
