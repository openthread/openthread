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

#include <openthread/openthread.h>
#include <openthread/platform/alarm.h>
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
#include "regmap.h"

#define FACTORY_TEST_TIMESTAMP      (0x7F8EA08) // Register holds a timestamp of facotry test of a chip
#define FACTORY_TESTER_ID           (0x7F8EA0C) // Register holds test machine ID used for factory test

#define DEFAULT_CHANNEL                 (11)

#define PRIVILEGED_DATA                 __attribute__((section("privileged_data_zi")))

#define RSSI_TABLE_SIZE                 8
#define EUI64_TABLE_SIZE                8

enum
{
    DA15000_RECEIVE_SENSITIVITY = -100,  // dBm
};

static void          da15000OtpRead(void);
static uint8_t       eui64[EUI64_TABLE_SIZE] = {0};

static otInstance   *sThreadInstance;
static otRadioState  sRadioState             = OT_RADIO_STATE_DISABLED;
static otRadioFrame  sReceiveFrame;
static otRadioFrame  sReceiveFrameAck;
static otRadioFrame  sTransmitFrame;
static otError       sTransmitStatus;
static bool          sRadioPromiscuous       = false;
static bool          sRssiInit               = true;
static uint8_t       sChannel                = DEFAULT_CHANNEL;
static uint8_t       sRssiCtr                = 0;
static uint32_t      sSleepInitDelay         = 0;

static uint8_t       sReceivePsdu[OT_RADIO_FRAME_MAX_SIZE];
static uint8_t       sReceivePsduAck[OT_RADIO_FRAME_MAX_SIZE];
static uint8_t       sRssiTable[RSSI_TABLE_SIZE];
static uint8_t       sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE];

PRIVILEGED_DATA static uint8_t sEnableRX;

static void da15000OtpRead(void)
{
    hw_otpc_init();         // Start clock.
    hw_otpc_disable();      // Make sure it is in standby mode.
    hw_otpc_init();         // Restart clock.
    hw_otpc_manual_read_on(false);

    __DMB();
    uint32_t *factoryTestTimeStamp  = (uint32_t *) FACTORY_TEST_TIMESTAMP;
    uint32_t *factoryTestId         = (uint32_t *) FACTORY_TESTER_ID;
    __DMB();

    eui64[0] = 0x80;    //80-EA-CA is for Dialog Semiconductor
    eui64[1] = 0xEA;
    eui64[2] = 0xCA;
    eui64[3] = (*factoryTestId        >>  8) & 0xff;
    eui64[4] = (*factoryTestTimeStamp >> 24) & 0xff;
    eui64[5] = (*factoryTestTimeStamp >> 16) & 0xff;
    eui64[6] = (*factoryTestTimeStamp >>  8) & 0xff;
    eui64[7] =  *factoryTestTimeStamp        & 0xff;

    hw_otpc_manual_read_off();
    hw_otpc_disable();
}

void da15000RadioInit(void)
{
    /* Wake up power domains */
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0);

    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP) == 0x0);

    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_ENABLE, 1);
    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_DIV, 0);

    hw_rf_poweron();
    hw_rf_system_init();

    ad_ftdf_init_phy_api();

    da15000OtpRead();

    sChannel               = DEFAULT_CHANNEL;
    sTransmitFrame.mPsdu   = sTransmitPsdu;
    sReceiveFrame.mPsdu    = sReceivePsdu;
    sReceiveFrameAck.mPsdu = sReceivePsduAck;

    otLogInfoPlat(sInstance, "Radio initialized", NULL);
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void)aInstance;
    memcpy(aIeeeEui64, eui64, EUI64_TABLE_SIZE);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanid)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "Set PanId: %X", aPanid);

    FTDF_setValue(FTDF_PIB_PAN_ID, &aPanid);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aAddress)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "Set Extended Address: %X%X%X%X%X%X%X%X",
                  aAddress[7], aAddress[6], aAddress[5], aAddress[4],
                  aAddress[3], aAddress[2], aAddress[1], aAddress[0]);

    FTDF_setValue(FTDF_PIB_EXTENDED_ADDRESS, aAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "Set Short Address: %X", aAddress);

    FTDF_setValue(FTDF_PIB_SHORT_ADDRESS, &aAddress);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    uint8_t maxRetries;
    uint8_t modeCCA;

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(sRadioState == OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    sThreadInstance = aInstance;

    sEnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &sChannel);
    maxRetries = 0;
    FTDF_setValue(FTDF_PIB_MAX_FRAME_RETRIES, &maxRetries);
    modeCCA = FTDF_CCA_MODE_2;
    FTDF_setValue(FTDF_PIB_CCA_MODE, &modeCCA);

    FTDF_Bitmap32 options =  FTDF_TRANSPARENT_ENABLE_FCS_GENERATION;
    options |= FTDF_TRANSPARENT_WAIT_FOR_ACK;
    options |= FTDF_TRANSPARENT_AUTO_ACK;

    FTDF_enableTransparentMode(FTDF_TRUE, options);
    otPlatRadioSetPromiscuous(aInstance, false);

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_SLEEP", NULL);
    sRadioState = OT_RADIO_STATE_SLEEP;

exit:
    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    (void)aInstance;
    sEnableRX = 0;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    ad_ftdf_sleep_when_possible(FTDF_TRUE);

    FTDF_fpprReset();

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_DISABLED", NULL);
    sRadioState = OT_RADIO_STATE_DISABLED;

    return OT_ERROR_NONE;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;

    return (sRadioState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    (void)aInstance;

    if (sRadioState == OT_RADIO_STATE_RECEIVE && sSleepInitDelay == 0)
    {
        sSleepInitDelay = otPlatAlarmGetNow();
        return OT_ERROR_NONE;
    }
    else if ((otPlatAlarmGetNow() - sSleepInitDelay) < dg_configINITIAL_SLEEP_DELAY_TIME)
    {
        return OT_ERROR_NONE;
    }

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(sRadioState == OT_RADIO_STATE_RECEIVE, error = OT_ERROR_INVALID_STATE);

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_SLEEP", NULL);
    sRadioState = OT_RADIO_STATE_SLEEP;

    sEnableRX = 0;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    ad_ftdf_sleep_when_possible(FTDF_TRUE);

exit:
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    (void)aInstance;
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sRadioState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    ad_ftdf_wake_up();

    sEnableRX = 0;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);

    sChannel = aChannel;
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &aChannel);

    sEnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_RECEIVE", NULL);
    sRadioState = OT_RADIO_STATE_RECEIVE;

exit:
    return error;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    (void)aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    otError error = OT_ERROR_NONE;
    uint8_t entry;
    uint8_t entryIdx;

    // check if address already stored

    otEXPECT(!FTDF_fpprLookupShortAddress(aShortAddress, &entry, &entryIdx));

    otEXPECT_ACTION(FTDF_fpprGetFreeShortAddress(&entry, &entryIdx), error = OT_ERROR_NO_BUFS);

    otLogDebgPlat(sInstance, "Add ShortAddress entry: %d", entry);

    FTDF_fpprSetShortAddress(entry, entryIdx, aShortAddress);
    FTDF_fpprSetShortAddressValid(entry, entryIdx, FTDF_TRUE);

exit:
    return error;

}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;

    uint8_t entry;
    FTDF_ExtAddress addr;
    uint32_t addrL;
    uint64_t addrH;

    addrL = (aExtAddress[3] << 24) | (aExtAddress[2] << 16) | (aExtAddress[1] << 8) | (aExtAddress[0] << 0);
    addrH = (aExtAddress[7] << 24) | (aExtAddress[6] << 16) | (aExtAddress[5] << 8) | (aExtAddress[4] << 0);
    addr = addrL | (addrH << 32);

    // check if address already stored
    otError error = OT_ERROR_NONE;
    otEXPECT(!FTDF_fpprLookupExtAddress(addr, &entry));

    otEXPECT_ACTION(FTDF_fpprGetFreeExtAddress(&entry), error = OT_ERROR_NO_BUFS);

    otLogDebgPlat(sInstance, "Add ExtAddress entry: %d", entry);

    FTDF_fpprSetExtAddress(entry, addr);
    FTDF_fpprSetExtAddressValid(entry, FTDF_TRUE);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    otError error = OT_ERROR_NONE;
    uint8_t entry;
    uint8_t entryIdx;

    otEXPECT_ACTION(FTDF_fpprLookupShortAddress(aShortAddress, &entry, &entryIdx), error = OT_ERROR_NO_ADDRESS);

    otLogDebgPlat(sInstance, "Clear ShortAddress entry: %d", entry);

    FTDF_fpprSetShortAddress(entry, entryIdx, 0);
    FTDF_fpprSetShortAddressValid(entry, entryIdx, FTDF_FALSE);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;

    uint8_t entry;
    FTDF_ExtAddress addr;
    uint32_t addrL;
    uint64_t addrH;

    addrL = (aExtAddress[3] << 24) | (aExtAddress[2] << 16) | (aExtAddress[1] << 8) | (aExtAddress[0] << 0);
    addrH = (aExtAddress[7] << 24) | (aExtAddress[6] << 16) | (aExtAddress[5] << 8) | (aExtAddress[4] << 0);
    addr = addrL | (addrH << 32);

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(FTDF_fpprLookupExtAddress(addr, &entry), error = OT_ERROR_NO_ADDRESS);

    otLogDebgPlat(sInstance, "Clear ExtAddress entry: %d", entry);

    FTDF_fpprSetExtAddress(entry, 0);
    FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;

    uint8_t i, j;

    otLogDebgPlat(sInstance, "Clear ShortAddress entries", NULL);

    for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (FTDF_fpprGetShortAddressValid(i, j))
            {
                FTDF_fpprSetShortAddressValid(i, j, FTDF_FALSE);
            }
        }
    }
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void)aInstance;

    uint8_t i;

    otLogDebgPlat(sInstance, "Clear ExtAddress entries", NULL);

    for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++)
    {
        if (FTDF_fpprGetExtAddressValid(i))
        {
            FTDF_fpprSetExtAddressValid(i, FTDF_FALSE);
        }
    }
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;

    return &sTransmitFrame;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    (void)aInstance;

    uint8_t csmaSuppress;

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(sRadioState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    otLogDebgPlat(sInstance, "Radio start transmit: %d bytes on channel: %d", aFrame->mLength, aFrame->mChannel);

    csmaSuppress = 0;
    ad_ftdf_send_frame_simple(aFrame->mLength, aFrame->mPsdu, aFrame->mChannel, 0, csmaSuppress); //Prio 0 for all.

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_TRANSMIT", NULL);
    sRadioState = OT_RADIO_STATE_TRANSMIT;

exit:
    return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;
    uint16_t result = 0;

    // fill table with same value for init
    if (sRssiInit)
    {
        for (uint8_t i = 0; i < RSSI_TABLE_SIZE; i++)
        {
            sRssiTable[i] = sReceiveFrame.mLqi;
        }

        sRssiInit = false;
    }
    else
    {
        ASSERT_ERROR(sRssiCtr < RSSI_TABLE_SIZE);
        sRssiTable[sRssiCtr] = sReceiveFrame.mLqi;
        sRssiCtr++;

        if (sRssiCtr >= RSSI_TABLE_SIZE)
        {
            sRssiCtr = 0;
        }
    }

    // average of 8 last lqi
    for (uint8_t i = 0; i < RSSI_TABLE_SIZE; i++)
    {
        result += sRssiTable[i];
    }

    result = result / RSSI_TABLE_SIZE;

    // Approximation to dB scale by divide by 9
    return result / 9;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;

    return OT_RADIO_CAPS_ACK_TIMEOUT;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;

    return sRadioPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "Set Promiscuous: %d", aEnable ? 1 : 0);

    FTDF_setValue(FTDF_PIB_PROMISCUOUS_MODE, &aEnable);
    sRadioPromiscuous = aEnable;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;

    return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    (void)aInstance;

    otLogInfoPlat(sInstance, "Set DefaultTxPower: %d", aPower);

    FTDF_setValue(FTDF_PIB_TX_POWER, &aPower);
}

void da15000RadioProcess(otInstance *aInstance)
{
    (void)aInstance;
    uint32_t ftdfCe = *FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CE);

    FTDF_confirmLmacInterrupt();

    if (ftdfCe & FTDF_MSK_RX_CE)
    {
        FTDF_processRxEvent();
    }

    if (ftdfCe & FTDF_MSK_TX_CE)
    {
        FTDF_processTxEvent();
    }

    volatile uint32_t *ftdfCm = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_FTDF_CM);
    *ftdfCm = FTDF_MSK_TX_CE | FTDF_MSK_RX_CE | FTDF_MSK_SYMBOL_TMR_CE;
}

void FTDF_sendFrameTransparentConfirm(void *handle, FTDF_Bitmap32 status)
{
    (void)handle;

    FTDF_FrameHeader frameHeader;

    switch (status)
    {
    case FTDF_TRANSPARENT_SEND_SUCCESSFUL:
        sTransmitStatus = OT_ERROR_NONE;
        otLogDebgPlat(sInstance, "Radio transmit SUCCESSFUL", NULL);
        break;

    case FTDF_TRANSPARENT_CSMACA_FAILURE:
        sTransmitStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        otLogDebgPlat(sInstance, "Radio transmit CHANNEL_ACCESS_FAILURE", NULL);
        break;

    case FTDF_TRANSPARENT_NO_ACK:
        sTransmitStatus = OT_ERROR_NO_ACK;
        otLogDebgPlat(sInstance, "Radio transmit NO_ACK", NULL);
        break;

    default:
        sTransmitStatus = OT_ERROR_ABORT;
        otLogDebgPlat(sInstance, "Radio transmit ABORT", NULL);
        break;
    }

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_RECEIVE", NULL);
    sRadioState = OT_RADIO_STATE_RECEIVE;

    FTDF_getFrameHeader(sTransmitFrame.mPsdu, &frameHeader);

    if ((frameHeader.options & FTDF_OPT_ACK_REQUESTED) == 0 || sTransmitStatus != OT_ERROR_NONE)
    {
        otPlatRadioTxDone(sThreadInstance, &sTransmitFrame, NULL, sTransmitStatus);
    }
    else
    {
        otPlatRadioTxDone(sThreadInstance, &sTransmitFrame, &sReceiveFrameAck, sTransmitStatus);
    }
}

void FTDF_rcvFrameTransparent(FTDF_DataLength frameLength,
                              FTDF_Octet     *frame,
                              FTDF_Bitmap32   status,
                              FTDF_LinkQuality lqi)
{
    FTDF_FrameHeader frameHeader;

    otEXPECT(frameLength <= OT_RADIO_FRAME_MAX_SIZE);

    otEXPECT(sRadioState != OT_RADIO_STATE_DISABLED)

    otLogDebgPlat(sInstance, "Radio state: OT_RADIO_STATE_RECEIVE", NULL);
    sRadioState = OT_RADIO_STATE_RECEIVE;

    if (status == FTDF_TRANSPARENT_RCV_SUCCESSFUL)
    {
        FTDF_getFrameHeader(frame, &frameHeader);

        otLogDebgPlat(sInstance, "Radio received: %d bytes", frameLength);

        if (frameHeader.frameType != FTDF_ACKNOWLEDGEMENT_FRAME)
        {
            sReceiveFrame.mChannel   = sChannel;
            sReceiveFrame.mLength    = frameLength;
            sReceiveFrame.mLqi       = lqi;
            sReceiveFrame.mPower     = otPlatRadioGetRssi(sThreadInstance);
            memcpy(sReceiveFrame.mPsdu, frame, frameLength);

            otPlatRadioReceiveDone(sThreadInstance, &sReceiveFrame, OT_ERROR_NONE);
        }
        else
        {
            sReceiveFrameAck.mChannel   = sChannel;
            sReceiveFrameAck.mLength    = frameLength;
            sReceiveFrameAck.mLqi       = lqi;
            sReceiveFrameAck.mPower     = otPlatRadioGetRssi(sThreadInstance);
            memcpy(sReceiveFrameAck.mPsdu, frame, frameLength);

            otPlatRadioReceiveDone(sThreadInstance, &sReceiveFrameAck, OT_ERROR_NONE);
        }
    }
    else
    {
        otPlatRadioReceiveDone(sThreadInstance, NULL, OT_ERROR_ABORT);
    }

exit:
    return;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    (void)aInstance;
    return DA15000_RECEIVE_SENSITIVITY;
}
