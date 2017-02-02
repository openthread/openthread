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

#include <common/code_utils.hpp>
#include <platform/alarm.h>
#include <platform/radio.h>
#include "platform-da15000.h"

#include <ad_ftdf.h>
#include <ad_ftdf_phy_api.h>
#include <hw_rf.h>
#include <internal.h>
#include <regmap.h>
#include <hw_uart.h>

#define DEFAULT_CHANNEL           (11)

static otInstance *ThreadInstance;

static PhyState RadioState = kStateDisabled;
static uint8_t Channel = DEFAULT_CHANNEL;
static uint8_t TransmitPsdu[kMaxPHYPacketSize];
static RadioPacket TransmitFrame;

static bool SendFrameDone = false;
static bool RadioPromiscuous = false;

static uint32_t SleepInitDelay = 0;
static uint32_t SleepConstDelay;
static bool SED = false;

uint32_t NODE_ID = 1;

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
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    (void)aInstance;

    aIeeeEui64[0] = 0x18;
    aIeeeEui64[1] = 0xb4;
    aIeeeEui64[2] = 0x30;
    aIeeeEui64[3] = 0x00;
    aIeeeEui64[4] = (NODE_ID >> 24) & 0xff;
    aIeeeEui64[5] = (NODE_ID >> 16) & 0xff;
    aIeeeEui64[6] = (NODE_ID >> 8) & 0xff;
    aIeeeEui64[7] = NODE_ID & 0xff;
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    (void)aInstance;

    FTDF_setValue(FTDF_PIB_PAN_ID, &panid);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *address)
{
    (void)aInstance;

    FTDF_setValue(FTDF_PIB_EXTENDED_ADDRESS, address);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    (void)aInstance;

    FTDF_setValue(FTDF_PIB_SHORT_ADDRESS, &address);
}

ThreadError otPlatRadioEnable(otInstance *aInstance)
{
    uint8_t DefaultChannel;
    uint8_t EnableRX;
    uint8_t MaxRetries;

    ThreadError error = kThreadError_None;
    VerifyOrExit(RadioState == kStateDisabled, error = kThreadError_InvalidState);

    ThreadInstance = aInstance;
    TransmitFrame.mPsdu = TransmitPsdu;

    EnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &EnableRX);
    DefaultChannel = DEFAULT_CHANNEL;
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &DefaultChannel);
    MaxRetries = 0;
    FTDF_setValue(FTDF_PIB_MAX_FRAME_RETRIES, &MaxRetries);

    FTDF_Bitmap32 options =  FTDF_TRANSPARENT_ENABLE_FCS_GENERATION;
    options |= FTDF_TRANSPARENT_WAIT_FOR_ACK;
    options |= FTDF_TRANSPARENT_AUTO_ACK;

    FTDF_enableTransparentMode(FTDF_TRUE, options);
    otPlatRadioSetPromiscuous(aInstance, false);
    RadioState = kStateSleep;

exit:
    return error;
}

ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    (void)aInstance;

    RadioState = kStateDisabled;

    return kThreadError_None;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;

    return (RadioState != kStateDisabled) ? true : false;
}

ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    (void)aInstance;

    if (RadioState == kStateReceive && SleepInitDelay == 0)
    {
        SleepInitDelay = otPlatAlarmGetNow();
        return kThreadError_None;
    }
    else if ((otPlatAlarmGetNow() - SleepInitDelay) < 3000)
    {
        return kThreadError_None;
    }

    if (RadioState == kStateSleep || RadioState == kStateReceive)
    {
        SED = true;
        SleepConstDelay = otPlatAlarmGetNow();
    }

    return kThreadError_None;
}

ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    (void)aInstance;

    uint8_t EnableRX;

    ThreadError error = kThreadError_None;
    VerifyOrExit(RadioState != kStateDisabled, error = kThreadError_InvalidState);

    ad_ftdf_wake_up();
    Channel = aChannel;
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &aChannel);
    EnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &EnableRX);
    RadioState = kStateReceive;

exit:
    return error;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;
    (void)aEnable;
}

ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;

    return kThreadError_NotImplemented;
}

ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;
    (void)aExtAddress;

    return kThreadError_NotImplemented;
}

ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;
    (void)aShortAddress;

    return kThreadError_NotImplemented;
}

ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aExtAddress;
    (void)aInstance;

    return kThreadError_NotImplemented;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    (void)aInstance;
}

RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;

    return &TransmitFrame;
}

ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket)
{
    (void)aInstance;

    uint8_t csmaSuppress;

    ThreadError error = kThreadError_None;
    VerifyOrExit(RadioState != kStateDisabled, error = kThreadError_InvalidState);

    csmaSuppress = 0;
    ad_ftdf_send_frame_simple(aPacket->mLength, aPacket->mPsdu, aPacket->mChannel, 0, csmaSuppress); //Prio 0 for all.
    RadioState = kStateTransmit;

exit:
    return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    (void)aInstance;

    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    (void)aInstance;

    return kRadioCapsNone;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    (void)aInstance;

    return RadioPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    FTDF_setValue(FTDF_PIB_PROMISCUOUS_MODE, &aEnable);
    RadioPromiscuous = aEnable;
}

ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;

    return kThreadError_NotImplemented;
}

void da15000RadioProcess(otInstance *aInstance)
{
    uint8_t EnableRX;

    if (SendFrameDone)
    {
        RadioState = kStateReceive;
        otPlatRadioTransmitDone(aInstance, &TransmitFrame, false, kThreadError_None);
        SendFrameDone = false;
    }

    if (SED)
    {
        if ((otPlatAlarmGetNow() - SleepConstDelay) > 100)
        {
            EnableRX = 0;
            FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &EnableRX);
            ad_ftdf_sleep_when_possible(1);
            RadioState = kStateSleep;
            SED = false;
        }
    }
}

void FTDF_sendFrameTransparentConfirm(void *handle, FTDF_Bitmap32 status)
{
    (void)handle;
    (void)status;

    SendFrameDone = true;
}

void FTDF_rcvFrameTransparent(FTDF_DataLength frameLength,
                              FTDF_Octet     *frame,
                              FTDF_Bitmap32   status,
                              FTDF_LinkQuality lqi)
{
    (void)status;

    RadioPacket otRadioPacket;

    otRadioPacket.mPower = kPhyInvalidRssi;
    otRadioPacket.mLqi = lqi;

    otRadioPacket.mChannel = Channel;
    otRadioPacket.mLength = frameLength;
    otRadioPacket.mPsdu = frame;

    otPlatRadioReceiveDone(ThreadInstance, &otRadioPacket, kThreadError_None);

    if (RadioState != kStateDisabled)
    {
        RadioState = kStateReceive;
    }
}

