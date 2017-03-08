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
#include "openthread/openthread.h"
#include "openthread/platform/alarm.h"
#include "openthread/platform/radio.h"
#include "platform-da15000.h"

#include <ad_ftdf.h>
#include <ad_ftdf_phy_api.h>
#include <hw_rf.h>
#include <internal.h>
#include <regmap.h>

#define FACTORY_TEST_TIMESTAMP      (0x7F8EA08) // Register holds a timestamp of facotry test of a chip
#define FACTORY_TESTER_ID           (0x7F8EA0C) // Register holds test machine ID used for factory test

#define DEFAULT_CHANNEL                 (11)

#define PRIVILEGED_DATA                 __attribute__((section("privileged_data_zi")))

#define IEEE802154_ACK_LENGTH           5
#define IEEE802154_FRAME_TYPE_MASK      0x07
#define IEEE802154_FRAME_TYPE_ACK       0x02
#define IEEE802154_FRAME_PENDING        1 << 4
#define IEEE802154_ACK_REQUEST          1 << 5
#define IEEE802154_DSN_OFFSET           2

static otInstance *sThreadInstance;

static PhyState    sRadioState = kStateDisabled;
static uint8_t     sChannel = DEFAULT_CHANNEL;
static uint8_t     sTransmitPsdu[kMaxPHYPacketSize];
static uint8_t     sReceivePsdu[kMaxPHYPacketSize];
static RadioPacket sTransmitFrame;
static RadioPacket sReceiveFrame;
static ThreadError sTransmitStatus;

static bool sFramePending     = false;
static bool sSendFrameDone    = false;
static bool sRadioPromiscuous = false;
static bool sGoSleep          = false;

PRIVILEGED_DATA static uint8_t sEnableRX;
static uint32_t sSleepInitDelay = 0;

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
    uint32_t *factoryTestTimeStamp  = (uint32_t *) FACTORY_TEST_TIMESTAMP;
    uint32_t *factoryTestId         = (uint32_t *) FACTORY_TESTER_ID;

    aIeeeEui64[0] = 0x80;    //80-EA-CA is for Dialog Semiconductor
    aIeeeEui64[1] = 0xEA;
    aIeeeEui64[2] = 0xCA;
    aIeeeEui64[3] = (*factoryTestId        >>  8) & 0xff;
    aIeeeEui64[4] = (*factoryTestTimeStamp >> 24) & 0xff;
    aIeeeEui64[5] = (*factoryTestTimeStamp >> 16) & 0xff;
    aIeeeEui64[6] = (*factoryTestTimeStamp >>  8) & 0xff;
    aIeeeEui64[7] =  *factoryTestTimeStamp        & 0xff;
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
    uint8_t defaultChannel;
    uint8_t maxRetries;

    ThreadError error = kThreadError_None;
    VerifyOrExit(sRadioState == kStateDisabled, error = kThreadError_InvalidState);

    sThreadInstance = aInstance;
    sTransmitFrame.mPsdu = sTransmitPsdu;
    sReceiveFrame.mPsdu = sReceivePsdu;

    sEnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    defaultChannel = DEFAULT_CHANNEL;
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &defaultChannel);
    maxRetries = 0;
    FTDF_setValue(FTDF_PIB_MAX_FRAME_RETRIES, &maxRetries);

    FTDF_Bitmap32 options =  FTDF_TRANSPARENT_ENABLE_FCS_GENERATION;
    options |= FTDF_TRANSPARENT_WAIT_FOR_ACK;
    options |= FTDF_TRANSPARENT_AUTO_ACK;

    FTDF_enableTransparentMode(FTDF_TRUE, options);
    otPlatRadioSetPromiscuous(aInstance, false);
    sRadioState = kStateSleep;

exit:
    return error;
}

ThreadError otPlatRadioDisable(otInstance *aInstance)
{
    (void)aInstance;

    sRadioState = kStateDisabled;

    return kThreadError_None;
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    (void)aInstance;

    return (sRadioState != kStateDisabled);
}

ThreadError otPlatRadioSleep(otInstance *aInstance)
{
    (void)aInstance;

    if (sRadioState == kStateReceive && sSleepInitDelay == 0)
    {
        sSleepInitDelay = otPlatAlarmGetNow();
        return kThreadError_None;
    }
    else if ((otPlatAlarmGetNow() - sSleepInitDelay) < 3000)
    {
        return kThreadError_None;
    }

    ThreadError error = kThreadError_None;
    VerifyOrExit(((sRadioState == kStateReceive) || (sRadioState == kStateSleep)), error = kThreadError_InvalidState);

    sGoSleep = true;
    sRadioState = kStateSleep;

exit:
    return error;
}

ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    (void)aInstance;
    ThreadError error = kThreadError_None;

    VerifyOrExit(sRadioState != kStateDisabled, error = kThreadError_InvalidState);

    ad_ftdf_wake_up();
    sChannel = aChannel;
    FTDF_setValue(FTDF_PIB_CURRENT_CHANNEL, &aChannel);

    sEnableRX = 1;
    FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
    sRadioState = kStateReceive;

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

    uint8_t entry;
    uint8_t entryIdx;

    // check if address already stored
    ThreadError error = kThreadError_None;
    SuccessOrExit(FTDF_fpprLookupShortAddress(aShortAddress, &entry, &entryIdx));

    VerifyOrExit(FTDF_fpprGetFreeShortAddress(&entry, &entryIdx), error = kThreadError_NoBufs);

    FTDF_fpprSetShortAddress(entry, entryIdx, aShortAddress);
    FTDF_fpprSetShortAddressValid(entry, entryIdx, FTDF_TRUE);

exit:
    return error;

}

ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;

    uint8_t entry;
    FTDF_ExtAddress addr;
    uint32_t addrL;
    uint64_t addrH;

    addrL = (aExtAddress[4] << 24) | (aExtAddress[5] << 16) | (aExtAddress[6] << 8) | (aExtAddress[7] << 0);
    addrH = (aExtAddress[0] << 24) | (aExtAddress[1] << 16) | (aExtAddress[2] << 8) | (aExtAddress[3] << 0);
    addr = addrL | (addrH << 32);

    // check if address already stored
    ThreadError error = kThreadError_None;
    SuccessOrExit(FTDF_fpprLookupExtAddress(addr, &entry));

    VerifyOrExit(FTDF_fpprGetFreeExtAddress(&entry), error = kThreadError_NoBufs);

    FTDF_fpprSetExtAddress(entry, addr);
    FTDF_fpprSetExtAddressValid(entry, FTDF_TRUE);

exit:
    return error;
}

ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    (void)aInstance;

    uint8_t entry;
    uint8_t entryIdx;

    ThreadError error = kThreadError_None;
    VerifyOrExit(FTDF_fpprLookupShortAddress(aShortAddress, &entry, &entryIdx), error = kThreadError_NoAddress);

    FTDF_fpprSetShortAddress(entry, entryIdx, 0);
    FTDF_fpprSetShortAddressValid(entry, entryIdx, FTDF_FALSE);

exit:
    return error;
}

ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress)
{
    (void)aInstance;

    uint8_t entry;
    FTDF_ExtAddress addr;
    uint32_t addrL;
    uint64_t addrH;

    addrL = (aExtAddress[4] << 24) | (aExtAddress[5] << 16) | (aExtAddress[6] << 8) | (aExtAddress[7] << 0);
    addrH = (aExtAddress[0] << 24) | (aExtAddress[1] << 16) | (aExtAddress[2] << 8) | (aExtAddress[3] << 0);
    addr = addrL | (addrH << 32);

    ThreadError error = kThreadError_None;
    VerifyOrExit(FTDF_fpprLookupExtAddress(addr, &entry), error = kThreadError_NoAddress);

    FTDF_fpprSetExtAddress(entry, 0);
    FTDF_fpprSetExtAddressValid(entry, FTDF_FALSE);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    (void)aInstance;

    uint8_t i, j;

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

    for (i = 0; i < FTDF_FPPR_TABLE_ENTRIES; i++)
    {
        if (FTDF_fpprGetExtAddressValid(i))
        {
            FTDF_fpprSetExtAddressValid(i, FTDF_FALSE);
        }
    }
}

RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    (void)aInstance;

    return &sTransmitFrame;
}

ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket)
{
    (void)aInstance;

    uint8_t csmaSuppress;

    ThreadError error = kThreadError_None;
    VerifyOrExit(sRadioState != kStateDisabled, error = kThreadError_InvalidState);

    csmaSuppress = 0;
    ad_ftdf_send_frame_simple(aPacket->mLength, aPacket->mPsdu, aPacket->mChannel, 0, csmaSuppress); //Prio 0 for all.
    sRadioState = kStateTransmit;

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

    return sRadioPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    (void)aInstance;

    FTDF_setValue(FTDF_PIB_PROMISCUOUS_MODE, &aEnable);
    sRadioPromiscuous = aEnable;
}

ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    (void)aInstance;
    (void)aScanChannel;
    (void)aScanDuration;

    return kThreadError_NotImplemented;
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower)
{
    // TODO: Create a proper implementation for this driver.
    (void)aInstance;
    (void)aPower;
}

void da15000RadioProcess(otInstance *aInstance)
{
    if (sSendFrameDone)
    {
        // Check FP bit in ACK response
        if ((sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST)     != 0 &&
            (sReceiveFrame.mPsdu[0]  & IEEE802154_FRAME_TYPE_MASK) != 0)
        {
            sFramePending = ((sReceiveFrame.mPsdu[0] & IEEE802154_FRAME_PENDING) != 0);
        }

        sRadioState = kStateReceive;
        otPlatRadioTransmitDone(aInstance, &sTransmitFrame, sFramePending, sTransmitStatus);

        sFramePending = false;
        sSendFrameDone = false;
    }

    if (sRadioState == kStateSleep && sGoSleep)
    {
        sGoSleep = false;

        sEnableRX = 0;
        FTDF_setValue(FTDF_PIB_RX_ON_WHEN_IDLE, &sEnableRX);
        ad_ftdf_sleep_when_possible(true);
    }
}

void FTDF_sendFrameTransparentConfirm(void *handle, FTDF_Bitmap32 status)
{
    (void)handle;

    switch (status)
    {
    case FTDF_TRANSPARENT_SEND_SUCCESSFUL:
        sTransmitStatus = kThreadError_None;
        break;

    case FTDF_TRANSPARENT_CSMACA_FAILURE:
        sTransmitStatus = kThreadError_ChannelAccessFailure;
        break;

    case FTDF_TRANSPARENT_NO_ACK:
        sTransmitStatus = kThreadError_NoAck;
        sSleepInitDelay = 0;
        break;

    default:
        sTransmitStatus = kThreadError_Abort;
        break;
    }

    sSendFrameDone = true;
}

void FTDF_rcvFrameTransparent(FTDF_DataLength frameLength,
                              FTDF_Octet     *frame,
                              FTDF_Bitmap32   status,
                              FTDF_LinkQuality lqi)
{
    sReceiveFrame.mPower     = kPhyInvalidRssi;
    sReceiveFrame.mLqi       = lqi;

    sReceiveFrame.mChannel   = sChannel;
    sReceiveFrame.mLength    = frameLength;
    sReceiveFrame.mPsdu      = frame;

    if (status == FTDF_TRANSPARENT_RCV_SUCCESSFUL)
    {
        otPlatRadioReceiveDone(sThreadInstance, &sReceiveFrame, kThreadError_None);
    }
    else
    {
        otPlatRadioReceiveDone(sThreadInstance, &sReceiveFrame, kThreadError_Abort);
    }

    if (sRadioState != kStateDisabled)
    {
        sRadioState = kStateReceive;
    }
}

