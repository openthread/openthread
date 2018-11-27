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
 * @file radio.cpp
 * Platform abstraction for radio communication.
 */

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include <string.h>

#include "platform-da15000.h"
#include "common/logging.hpp"
#include "utils/code_utils.h"

#include "ad_ftdf.h"
#include "ad_ftdf_phy_api.h"
#include "hw_otpc.h"
#include "hw_rf.h"
#include "internal.h"

// clang-format off
#define FACTORY_TEST_TIMESTAMP      (0x7F8EA08) // Register holds a timestamp of facotry test of a chip
#define FACTORY_TESTER_ID           (0x7F8EA0C) // Register holds test machine ID used for factory test

#define RADIO_DEFAULT_CHANNEL       (11)
#define RADIO_EUI64_TABLE_SIZE      (8)
#define RADIO_FRAMES_BUFFER_SIZE    (32)
// clang-format on

enum
{
    DA15000_RECEIVE_SENSITIVITY = -100, // dBm
};

static ftdf_dbm      sRssiReal = -100; // Initialize with the worst power
static otInstance *  sThreadInstance;
static otRadioState  sRadioState = OT_RADIO_STATE_DISABLED;
static otRadioFrame  sReceiveFrame[RADIO_FRAMES_BUFFER_SIZE];
static otRadioFrame *sReceiveFrameAck;
static otRadioFrame  sTransmitFrame;
static otError       sTransmitStatus;
static bool          sAckFrame          = false;
static bool          sDropFrame         = false;
static bool          sRadioPromiscuous  = false;
static bool          sTransmitDoneFrame = false;
static uint8_t       sChannel           = RADIO_DEFAULT_CHANNEL;
static uint8_t       sEnableRX          = 0;
static int8_t        sTxPower           = OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER;
static uint8_t       sReadFrame         = 0;
static uint8_t       sWriteFrame        = 0;
static uint32_t      sSleepInitDelay    = 0;

static uint8_t sEui64[RADIO_EUI64_TABLE_SIZE];
static uint8_t sReceivePsdu[RADIO_FRAMES_BUFFER_SIZE][OT_RADIO_FRAME_MAX_SIZE];
static uint8_t sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE];

static void da15000OtpRead(void)
{
    hw_otpc_init();    // Start clock.
    hw_otpc_disable(); // Make sure it is in standby mode.
    hw_otpc_init();    // Restart clock.
    hw_otpc_manual_read_on(false);

    __DMB();
    uint32_t *factoryTestTimeStamp = (uint32_t *)FACTORY_TEST_TIMESTAMP;
    uint32_t *factoryTestId        = (uint32_t *)FACTORY_TESTER_ID;
    __DMB();

    sEui64[0] = 0x80; // 80-EA-CA is for Dialog Semiconductor
    sEui64[1] = 0xEA;
    sEui64[2] = 0xCA;
    sEui64[3] = (*factoryTestId >> 8) & 0xff;
    sEui64[4] = (*factoryTestTimeStamp >> 24) & 0xff;
    sEui64[5] = (*factoryTestTimeStamp >> 16) & 0xff;
    sEui64[6] = (*factoryTestTimeStamp >> 8) & 0xff;
    sEui64[7] = *factoryTestTimeStamp & 0xff;

    hw_otpc_manual_read_off();
    hw_otpc_disable();
}

void da15000RadioInit(void)
{
    uint16_t ptr;

    /* Wake up power domains */
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0)
        ;

    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP) == 0x0)
        ;

    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_ENABLE, 1);
    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_DIV, 0);

    hw_rf_poweron();
    hw_rf_system_init();

    ad_ftdf_init_phy_api();

    da15000OtpRead();

    sChannel             = RADIO_DEFAULT_CHANNEL;
    sTransmitFrame.mPsdu = sTransmitPsdu;

    for (ptr = 0; ptr != RADIO_FRAMES_BUFFER_SIZE; ptr++)
    {
        sReceiveFrame[ptr].mPsdu = &sReceivePsdu[ptr][0];
    }

    otLogInfoPlat(sThreadInstance, "Radio initialized", NULL);
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aIeeeEui64, sEui64, RADIO_EUI64_TABLE_SIZE);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid)
{
    otLogInfoPlat("Set PanId: %X", aPanid);

    ftdf_set_value(FTDF_PIB_PAN_ID, &aPanid);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    otLogInfoPlat("Set Extended Address: %X%X%X%X%X%X%X%X", aAddress->m8[7], aAddress->m8[6], aAddress->m8[5],
                  aAddress->m8[4], aAddress->m8[3], aAddress->m8[2], aAddress->m8[1], aAddress->m8[0]);

    ftdf_set_value(FTDF_PIB_EXTENDED_ADDRESS, aAddress->m8);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    otLogInfoPlat("Set Short Address: %X", aAddress);

    ftdf_set_value(FTDF_PIB_SHORT_ADDRESS, &aAddress);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    ftdf_bitmap32_t options;
    otError         error = OT_ERROR_NONE;
    uint8_t         modeCCA;

    otEXPECT_ACTION(sRadioState == OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    sThreadInstance = aInstance;

    sEnableRX = 1;
    ftdf_set_value(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);

    ftdf_set_value(FTDF_PIB_CURRENT_CHANNEL, &sChannel);

    modeCCA = FTDF_CCA_MODE_2;
    ftdf_set_value(FTDF_PIB_CCA_MODE, &modeCCA);

    options = FTDF_TRANSPARENT_ENABLE_FCS_GENERATION;
    options |= FTDF_TRANSPARENT_WAIT_FOR_ACK;
    options |= FTDF_TRANSPARENT_AUTO_ACK;

    ftdf_enable_transparent_mode(FTDF_TRUE, options);
    otPlatRadioSetPromiscuous(aInstance, false);

    otLogDebgPlat("Radio state: OT_RADIO_STATE_SLEEP", NULL);
    sRadioState = OT_RADIO_STATE_SLEEP;

exit:
    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    sEnableRX = 0;
    ftdf_set_value(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    ad_ftdf_sleep_when_possible(FTDF_TRUE);

    ftdf_fppr_reset();

    otLogDebgPlat("Radio state: OT_RADIO_STATE_DISABLED", NULL);
    sRadioState = OT_RADIO_STATE_DISABLED;

    return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sRadioState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    if (sRadioState == OT_RADIO_STATE_RECEIVE && sSleepInitDelay == 0)
    {
        sSleepInitDelay = otPlatAlarmMilliGetNow();
        return OT_ERROR_NONE;
    }
    else if ((otPlatAlarmMilliGetNow() - sSleepInitDelay) < dg_configINITIAL_SLEEP_DELAY_TIME)
    {
        return OT_ERROR_NONE;
    }

    otEXPECT_ACTION(sRadioState == OT_RADIO_STATE_RECEIVE, error = OT_ERROR_INVALID_STATE);

    otLogDebgPlat("Radio state: OT_RADIO_STATE_SLEEP", NULL);
    sRadioState = OT_RADIO_STATE_SLEEP;

    sEnableRX = 0;
    ftdf_set_value(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    ad_ftdf_sleep_when_possible(FTDF_TRUE);

exit:
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sRadioState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    ad_ftdf_wake_up();

    sEnableRX = 0;
    ftdf_set_value(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);

    sChannel = aChannel;
    ftdf_set_value(FTDF_PIB_CURRENT_CHANNEL, &aChannel);

    sEnableRX = 1;
    ftdf_set_value(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);

    otLogDebgPlat("Radio state: OT_RADIO_STATE_RECEIVE", NULL);
    sRadioState = OT_RADIO_STATE_RECEIVE;

exit:
    return error;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;
    uint8_t entry;
    uint8_t entryIdx;

    // check if address already stored
    otEXPECT(!ftdf_fppr_lookup_short_address(aShortAddress, &entry, &entryIdx));

    otEXPECT_ACTION(ftdf_fppr_get_free_short_address(&entry, &entryIdx), error = OT_ERROR_NO_BUFS);

    otLogDebgPlat("Add ShortAddress entry: %d", entry);

    ftdf_fppr_set_short_address(entry, entryIdx, aShortAddress);
    ftdf_fppr_set_short_address_valid(entry, entryIdx, FTDF_TRUE);

exit:
    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError            error = OT_ERROR_NONE;
    uint8_t            entry;
    ftdf_ext_address_t addr;
    uint32_t           addrL;
    uint64_t           addrH;

    addrL =
        (aExtAddress->m8[3] << 24) | (aExtAddress->m8[2] << 16) | (aExtAddress->m8[1] << 8) | (aExtAddress->m8[0] << 0);
    addrH =
        (aExtAddress->m8[7] << 24) | (aExtAddress->m8[6] << 16) | (aExtAddress->m8[5] << 8) | (aExtAddress->m8[4] << 0);
    addr = addrL | (addrH << 32);

    // check if address already stored
    otEXPECT(!ftdf_fppr_lookup_ext_address(addr, &entry));

    otEXPECT_ACTION(ftdf_fppr_get_free_ext_address(&entry), error = OT_ERROR_NO_BUFS);

    otLogDebgPlat("Add ExtAddress entry: %d", entry);

    ftdf_fppr_set_ext_address(entry, addr);
    ftdf_fppr_set_ext_address_valid(entry, FTDF_TRUE);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;
    uint8_t entry;
    uint8_t entryIdx;

    otEXPECT_ACTION(ftdf_fppr_lookup_short_address(aShortAddress, &entry, &entryIdx), error = OT_ERROR_NO_ADDRESS);

    otLogDebgPlat("Clear ShortAddress entry: %d", entry);

    ftdf_fppr_set_short_address(entry, entryIdx, 0);
    ftdf_fppr_set_short_address_valid(entry, entryIdx, FTDF_FALSE);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otError            error = OT_ERROR_NONE;
    uint8_t            entry;
    ftdf_ext_address_t addr;
    uint32_t           addrL;
    uint64_t           addrH;

    addrL =
        (aExtAddress->m8[3] << 24) | (aExtAddress->m8[2] << 16) | (aExtAddress->m8[1] << 8) | (aExtAddress->m8[0] << 0);
    addrH =
        (aExtAddress->m8[7] << 24) | (aExtAddress->m8[6] << 16) | (aExtAddress->m8[5] << 8) | (aExtAddress->m8[4] << 0);
    addr = addrL | (addrH << 32);

    otEXPECT_ACTION(ftdf_fppr_lookup_ext_address(addr, &entry), error = OT_ERROR_NO_ADDRESS);

    otLogDebgPlat("Clear ExtAddress entry: %d", entry);

    ftdf_fppr_set_ext_address(entry, 0);
    ftdf_fppr_set_ext_address_valid(entry, FTDF_FALSE);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    uint8_t i, j;

    otLogDebgPlat("Clear ShortAddress entries", NULL);

    for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (ftdf_fppr_get_short_address_valid(i, j))
            {
                ftdf_fppr_set_short_address_valid(i, j, FTDF_FALSE);
            }
        }
    }
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    uint8_t i;

    otLogDebgPlat("Clear ExtAddress entries", NULL);

    for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++)
    {
        if (ftdf_fppr_get_ext_address_valid(i))
        {
            ftdf_fppr_set_ext_address_valid(i, FTDF_FALSE);
        }
    }
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sRadioState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    otLogDebgPlat("Radio start transmit: %d bytes on channel: %d", aFrame->mLength, aFrame->mChannel);

    ad_ftdf_send_frame_simple(aFrame->mLength, aFrame->mPsdu, aFrame->mChannel, 0, FTDF_TRUE);

    otLogDebgPlat("Radio state: OT_RADIO_STATE_TRANSMIT", NULL);
    sRadioState = OT_RADIO_STATE_TRANSMIT;

    otPlatRadioTxStarted(aInstance, aFrame);

exit:
    return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRssiReal;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sRadioPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    otLogInfoPlat("Set Promiscuous: %d", aEnable ? 1 : 0);

    ftdf_set_value(FTDF_PIB_PROMISCUOUS_MODE, &aEnable);
    sRadioPromiscuous = aEnable;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sTxPower;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    otLogInfoPlat("Set DefaultTxPower: %d", aPower);

    sTxPower = aPower;
    ftdf_set_value(FTDF_PIB_TX_POWER, &aPower);

    return OT_ERROR_NONE;
}

void da15000RadioProcess(otInstance *aInstance)
{
    ftdf_frame_header_t frameHeader;

    if (sReadFrame != sWriteFrame)
    {
        ftdf_get_frame_header(sReceiveFrame[sReadFrame].mPsdu, &frameHeader);

        otLogDebgPlat("Radio received: %d bytes", sReceiveFrame[sReadFrame].mLength);

        if (frameHeader.frame_type == FTDF_ACKNOWLEDGEMENT_FRAME)
        {
            sReceiveFrameAck = &sReceiveFrame[sReadFrame];
            sAckFrame        = true;
        }

        otPlatRadioReceiveDone(sThreadInstance, &sReceiveFrame[sReadFrame], OT_ERROR_NONE);

        sReadFrame = (sReadFrame + 1) % RADIO_FRAMES_BUFFER_SIZE;
        sDropFrame = false;
    }

    if (sTransmitDoneFrame)
    {
        otLogDebgPlat("Radio transmit status: %s", otThreadErrorToString(sTransmitStatus));

        ftdf_get_frame_header(sTransmitFrame.mPsdu, &frameHeader);

        if ((frameHeader.options & FTDF_OPT_ACK_REQUESTED) == 0 || sTransmitStatus != OT_ERROR_NONE)
        {
            sRadioState = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(sThreadInstance, &sTransmitFrame, NULL, sTransmitStatus);
        }
        else
        {
            otEXPECT(sAckFrame);
            sRadioState = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(sThreadInstance, &sTransmitFrame, sReceiveFrameAck, sTransmitStatus);
            sAckFrame = false;
        }

        sTransmitDoneFrame = false;

        otLogDebgPlat("Radio state: OT_RADIO_STATE_RECEIVE", NULL);
    }

exit:
    return;
}

void ftdf_send_frame_transparent_confirm(void *handle, ftdf_bitmap32_t status)
{
    OT_UNUSED_VARIABLE(handle);

    switch (status)
    {
    case FTDF_TRANSPARENT_SEND_SUCCESSFUL:
        sTransmitStatus = OT_ERROR_NONE;
        break;

    case FTDF_TRANSPARENT_CSMACA_FAILURE:
        sTransmitStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        break;

    case FTDF_TRANSPARENT_NO_ACK:
        sTransmitStatus = OT_ERROR_NO_ACK;
        break;

    default:
        sTransmitStatus = OT_ERROR_ABORT;
        break;
    }

    sTransmitDoneFrame = true;
}

static void radioRssiCalc(ftdf_link_quality_t link_quality)
{
    sRssiReal = (ftdf_dbm)((0.5239 * (float)link_quality) - 114.8604);
}

void ftdf_rcv_frame_transparent(ftdf_data_length_t  frame_length,
                                ftdf_octet_t *      frame,
                                ftdf_bitmap32_t     status,
                                ftdf_link_quality_t link_quality)
{
    otEXPECT(frame_length <= OT_RADIO_FRAME_MAX_SIZE);

    otEXPECT(sRadioState != OT_RADIO_STATE_DISABLED);

    otEXPECT(!sDropFrame);

    if (status == FTDF_TRANSPARENT_RCV_SUCCESSFUL)
    {
        radioRssiCalc(link_quality);

        if (otPlatRadioGetPromiscuous(sThreadInstance))
        {
            // Timestamp
            sReceiveFrame[sWriteFrame].mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
            sReceiveFrame[sWriteFrame].mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
        }

        sReceiveFrame[sWriteFrame].mChannel            = sChannel;
        sReceiveFrame[sWriteFrame].mLength             = frame_length;
        sReceiveFrame[sWriteFrame].mInfo.mRxInfo.mLqi  = OT_RADIO_LQI_NONE;
        sReceiveFrame[sWriteFrame].mInfo.mRxInfo.mRssi = otPlatRadioGetRssi(sThreadInstance);
        memcpy(sReceiveFrame[sWriteFrame].mPsdu, frame, frame_length);

        sWriteFrame = (sWriteFrame + 1) % RADIO_FRAMES_BUFFER_SIZE;

        if (sWriteFrame == sReadFrame)
        {
            sDropFrame = true;
        }
    }

exit:
    return;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return DA15000_RECEIVE_SENSITIVITY;
}
