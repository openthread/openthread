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

#include "asf.h"
#include "phy.h"

#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include "platform-samr21.h"

#include "common/logging.hpp"
#include "utils/code_utils.h"

enum
{
    IEEE802154_ACK_LENGTH = 5,
    IEEE802154_FCS_SIZE   = 2
};

enum
{
    SAMR21_RECEIVE_SENSITIVITY = -99 // dBm
};

static otRadioFrame sTransmitFrame;
static uint8_t      sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE];

static otRadioFrame sReceiveFrame;

static bool         sSleep       = false;
static bool         sRxEnable    = false;
static bool         sTxDone      = false;
static bool         sRxDone      = false;
static otError      sTxStatus    = OT_ERROR_NONE;
static int8_t       sPower       = OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER;
static otRadioState sState       = OT_RADIO_STATE_DISABLED;
static bool         sPromiscuous = false;
static uint8_t      sChannel     = 0xFF;

static int8_t   sMaxRssi;
static uint32_t sScanStartTime;
static uint16_t sScanDuration;
static bool     sStartScan = false;

static int8_t sTxPowerTable[] = {4, 4, 3, 3, 3, 2, 1, 0, -1, -2, -3, -4, -6, -8, -12, -17};

/*******************************************************************************
 * Static
 ******************************************************************************/

static void radioSleep()
{
    if (!sSleep)
    {
        PHY_SetRxState(false);
        PHY_Sleep();

        sSleep    = true;
        sRxEnable = false;
    }
}

static void radioWakeup()
{
    if (sSleep)
    {
        PHY_Wakeup();
    }
}

static void radioRxEnable()
{
    if (sSleep)
    {
        PHY_Wakeup();

        sSleep = false;
    }

    if (!sRxEnable)
    {
        PHY_SetRxState(true);

        sRxEnable = true;
    }
}

static void radioTrxOff()
{
    if (sSleep)
    {
        PHY_Wakeup();
    }
    else if (sRxEnable)
    {
        PHY_SetRxState(false);
    }
}

static void radioRestore()
{
    if (sSleep)
    {
        PHY_Sleep();
    }
    else if (sRxEnable)
    {
        PHY_SetRxState(true);
    }
}

static void setTxPower(uint8_t aPower)
{
    if (aPower != sPower)
    {
        uint8_t i;

        for (i = 0; i < (sizeof(sTxPowerTable) / sizeof(*sTxPowerTable) - 1); i++)
        {
            if (aPower >= sTxPowerTable[i])
            {
                break;
            }
        }

        otLogDebgPlat("Radio set tx power: %d, %d", aPower, i);

        radioTrxOff();

        PHY_SetTxPower(i);

        radioRestore();

        sPower = aPower;
    }
}

static void setChannel(uint8_t aChannel)
{
    if (aChannel != sChannel)
    {
        otLogDebgPlat("Radio set channel: %d", aChannel);

        radioTrxOff();

        PHY_SetChannel(aChannel);

        radioRestore();

        sChannel = aChannel;
    }
}

static void handleEnergyScan()
{
    if (sStartScan)
    {
        if ((otPlatAlarmMilliGetNow() - sScanStartTime) < sScanDuration)
        {
            int8_t curRssi = PHY_EdReq();

            if (curRssi > sMaxRssi)
            {
                sMaxRssi = curRssi;
            }
        }
        else
        {
            sStartScan = false;

            otPlatRadioEnergyScanDone(sInstance, sMaxRssi);

            radioRestore();
        }
    }
}

static void handleRx(void)
{
    if (sRxDone)
    {
        sRxDone = false;

        if (otPlatRadioGetPromiscuous(sInstance))
        {
            // Timestamp
            sReceiveFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
            sReceiveFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
        }

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioReceiveDone(sInstance, &sReceiveFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            // signal MAC layer for each received frame if promiscous is enabled
            // otherwise only signal MAC layer for non-ACK frame
            if (sPromiscuous || sReceiveFrame.mLength > IEEE802154_ACK_LENGTH)
            {
                otLogDebgPlat("Radio receive done, rssi: %d", sReceiveFrame.mInfo.mRxInfo.mRssi);

                otPlatRadioReceiveDone(sInstance, &sReceiveFrame, OT_ERROR_NONE);
            }
        }
    }
}

static void handleTx(void)
{
    if (sTxDone)
    {
        sTxDone = false;

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(sInstance, &sTransmitFrame, sTxStatus);
        }
        else
#endif
        {
            otLogDebgPlat("Radio transmit done, status: %d", sTxStatus);

            otPlatRadioTxDone(sInstance, &sTransmitFrame, NULL, sTxStatus);
        }
    }
}

/*******************************************************************************
 * PHY
 ******************************************************************************/

void PHY_DataInd(PHY_DataInd_t *ind)
{
    sReceiveFrame.mPsdu               = ind->data;
    sReceiveFrame.mLength             = ind->size + IEEE802154_FCS_SIZE;
    sReceiveFrame.mInfo.mRxInfo.mRssi = ind->rssi;

    sRxDone = true;
}

void PHY_DataConf(uint8_t status)
{
    switch (status)
    {
    case PHY_STATUS_SUCCESS:
        sTxStatus = OT_ERROR_NONE;
        break;

    case PHY_STATUS_CHANNEL_ACCESS_FAILURE:
        sTxStatus = OT_ERROR_CHANNEL_ACCESS_FAILURE;
        break;

    case PHY_STATUS_NO_ACK:
        sTxStatus = OT_ERROR_NO_ACK;
        break;

    default:
        sTxStatus = OT_ERROR_ABORT;
        break;
    }

    sTxDone = true;
}

/*******************************************************************************
 * Platform
 ******************************************************************************/
void samr21RadioInit(void)
{
    sTransmitFrame.mLength = 0;
    sTransmitFrame.mPsdu   = sTransmitPsdu;

    sReceiveFrame.mLength = 0;
    sReceiveFrame.mPsdu   = NULL;

    PHY_Init();
}

void samr21RadioProcess(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    PHY_TaskHandler();

    handleEnergyScan();
    handleRx();
    handleTx();
}

uint32_t samr21RadioRandomGet(void)
{
    uint32_t result;

    radioWakeup();

    result = PHY_RandomReq() << 16 | PHY_RandomReq();

    radioRestore();

    return result;
}

void samr21RadioRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    radioWakeup();

    for (uint16_t i = 0; i < aOutputLength / sizeof(uint16_t); i++)
    {
        *((uint16_t *)aOutput) = PHY_RandomReq();
        aOutput += sizeof(uint16_t);
    }

    for (uint16_t i = 0; i < aOutputLength % sizeof(uint16_t); i++)
    {
        aOutput[i] = PHY_RandomReq();
    }

    radioRestore();
}

/*******************************************************************************
 * Radio
 ******************************************************************************/
otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    samr21GetIeeeEui64(aInstance, aIeeeEui64);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgPlat("Set Pan ID: 0x%04X", aPanId);

    radioTrxOff();

    PHY_SetPanId(aPanId);

    radioRestore();
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    radioTrxOff();

    PHY_SetIEEEAddr((uint8_t *)aAddress);

    radioRestore();
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    radioTrxOff();

    PHY_SetShortAddr(aAddress);

    radioRestore();
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otLogDebgPlat("Radio enable");

    if (!otPlatRadioIsEnabled(aInstance))
    {
        radioSleep();

        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otLogDebgPlat("Radio disable");

    if (otPlatRadioIsEnabled(aInstance))
    {
        radioSleep();

        sState = OT_RADIO_STATE_DISABLED;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgPlat("Radio sleep");

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState == OT_RADIO_STATE_SLEEP || sState == OT_RADIO_STATE_RECEIVE, error = OT_ERROR_INVALID_STATE);

    radioSleep();

    sState = OT_RADIO_STATE_SLEEP;

exit:

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgPlat("Radio receive, channel: %d", aChannel);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    setChannel(aChannel);

    radioRxEnable();

    sState = OT_RADIO_STATE_RECEIVE;

exit:

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgPlat("Radio transmit");

    otEXPECT_ACTION(sState == OT_RADIO_STATE_RECEIVE, error = OT_ERROR_INVALID_STATE);

    uint8_t frame[OT_RADIO_FRAME_MAX_SIZE + 1];

    setChannel(aFrame->mChannel);

    frame[0] = aFrame->mLength - IEEE802154_FCS_SIZE;
    memcpy(frame + 1, aFrame->mPsdu, aFrame->mLength);

    PHY_DataReq(frame);

    otPlatRadioTxStarted(aInstance, aFrame);

    sState = OT_RADIO_STATE_TRANSMIT;

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
    return sMaxRssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    return OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_ACK_TIMEOUT;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPromiscuous;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    sPromiscuous = aEnable;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    sScanStartTime = otPlatAlarmMilliGetNow();
    sScanDuration  = aScanDuration;

    sMaxRssi = PHY_EdReq();

    sStartScan = true;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aPower = sPower;

exit:
    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otLogDebgPlat("Radio set default TX power: %d", aPower);

    setTxPower(aPower);

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return SAMR21_RECEIVE_SENSITIVITY;
}
