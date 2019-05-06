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
 *   This file implements the BLE Base Band driver for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "bb_api.h"
#include "bb_ble_api.h"
#include "bb_ble_drv.h"
#include "bb_drv.h"

#include "cordio/ble_init.h"
#include "utils/code_utils.h"

#include <string.h>
#include <openthread/error.h>
#include <openthread/platform/radio-ble.h>
#include <openthread/platform/random.h>

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
static uint8_t *sRxBuffer = NULL;
static uint16_t sRxLength = 0;

static const BbBleDrvChan_t *     sChannelParams = NULL;
static const BbBleDrvDataParam_t *sDataParams    = NULL;

void BbDrvInit(void)
{
}

void BbDrvEnable(void)
{
}

void BbDrvDisable(void)
{
}

uint32_t BbDrvGetCurrentTime(void)
{
    return otPlatRadioBleGetTickNow(bleGetThreadInstance());
}

bool_t BbDrvGetTimestamp(uint32_t *pTime)
{
    OT_UNUSED_VARIABLE(pTime);
    return false;
}

void BbBleDrvInit(void)
{
}

void BbBleDrvEnable(void)
{
    otPlatRadioBleEnable(bleGetThreadInstance());
}

void BbBleDrvDisable(void)
{
    otPlatRadioBleDisable(bleGetThreadInstance());
}

void BbBleDrvSetChannelParam(BbBleDrvChan_t *pChan)
{
    if (pChan != NULL)
    {
        sChannelParams = pChan;
    }
}

int8_t BbBleRfGetActualTxPower(int8_t txPwr, bool_t compFlag)
{
    OT_UNUSED_VARIABLE(txPwr);
    OT_UNUSED_VARIABLE(compFlag);

    return otPlatRadioBleGetTransmitPower(bleGetThreadInstance());
}

void BbBleDrvSetDataParams(const BbBleDrvDataParam_t *pParam)
{
    if (pParam != NULL)
    {
        sDataParams = pParam;
    }
}

void BbBleDrvSetOpParams(const BbBleDrvOpParam_t *pOpParam)
{
    otEXPECT(pOpParam != NULL);

    if (pOpParam->ifsSetup)
    {
        otPlatRadioBleEnableTifs(bleGetThreadInstance());
    }
    else
    {
        otPlatRadioBleDisableTifs(bleGetThreadInstance());
    }

exit:
    return;
}

void otPlatRadioBleTransmitDone(otInstance *aInstance, otError aError)
{
    otEXPECT(sDataParams != NULL);
    otEXPECT(sDataParams->txCback != NULL);

    sDataParams->txCback(aError == OT_ERROR_NONE ? BB_STATUS_SUCCESS : BB_STATUS_FAILED);
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void BbBleDrvTxData(BbBleDrvTxBufDesc_t descs[], uint8_t cnt)
{
    otRadioBleFrame *  frame;
    otRadioBleSettings settings;
    otRadioBleTime     time;
    uint16_t           offset;
    uint16_t           i;

    otEXPECT(sChannelParams != NULL);
    otEXPECT(sDataParams != NULL);
    otEXPECT((frame = otPlatRadioBleGetTransmitBuffer(bleGetThreadInstance())) != NULL);

    for (i = 0, offset = 0; i < cnt; i++)
    {
        otEXPECT(offset + descs[i].len <= OT_RADIO_BLE_FRAME_MAX_SIZE);

        memcpy(frame->mPdu + offset, descs[i].pBuf, descs[i].len);
        offset += descs[i].len;
    }

    frame->mLength = offset;
    time.mTicks    = sDataParams->due;
    time.mOffsetUs = sDataParams->dueOffsetUsec;

    settings.mChannel       = sChannelParams->chanIdx;
    settings.mAccessAddress = sChannelParams->accAddr;
    settings.mCrcInit       = sChannelParams->crcInit;

    otEXPECT(otPlatRadioBleSetTransmitPower(bleGetThreadInstance(), sChannelParams->txPower) == OT_ERROR_NONE);
    otPlatRadioBleTransmitAtTime(bleGetThreadInstance(), &settings, &time);

exit:
    return;
}

void BbBleDrvTxTifsData(BbBleDrvTxBufDesc_t descs[], uint8_t cnt)
{
    otRadioBleFrame *  frame;
    otRadioBleSettings settings;
    uint16_t           offset;
    uint16_t           i;

    otEXPECT(sChannelParams != NULL);
    otEXPECT((frame = otPlatRadioBleGetTransmitBuffer(bleGetThreadInstance())) != NULL);

    for (i = 0, offset = 0; i < cnt; i++)
    {
        otEXPECT(offset + descs[i].len <= OT_RADIO_BLE_FRAME_MAX_SIZE);

        memcpy(frame->mPdu + offset, descs[i].pBuf, descs[i].len);
        offset += descs[i].len;
    }

    frame->mLength          = offset;
    settings.mChannel       = sChannelParams->chanIdx;
    settings.mAccessAddress = sChannelParams->accAddr;
    settings.mCrcInit       = sChannelParams->crcInit;

    otEXPECT(otPlatRadioBleSetTransmitPower(bleGetThreadInstance(), sChannelParams->txPower) == OT_ERROR_NONE);
    otPlatRadioBleTransmitAtTifs(bleGetThreadInstance(), &settings);

exit:
    return;
}

void BbBleDrvRxData(uint8_t *pBuf, uint16_t len)
{
    otRadioBleTime     time;
    otRadioBleSettings settings;

    otEXPECT(sChannelParams != NULL);
    otEXPECT(sDataParams != NULL);

    sRxBuffer = pBuf;
    sRxLength = len;

    time.mTicks        = sDataParams->due;
    time.mOffsetUs     = sDataParams->dueOffsetUsec;
    time.mRxDurationUs = sDataParams->rxTimeoutUsec;

    settings.mChannel       = sChannelParams->chanIdx;
    settings.mAccessAddress = sChannelParams->accAddr;
    settings.mCrcInit       = sChannelParams->crcInit;

    otPlatRadioBleReceiveAtTime(bleGetThreadInstance(), &settings, &time);

exit:
    return;
}

void BbBleDrvRxTifsData(uint8_t *pBuf, uint16_t len)
{
    otRadioBleSettings settings;

    otEXPECT(sChannelParams != NULL);

    sRxBuffer = pBuf;
    sRxLength = len;

    settings.mChannel       = sChannelParams->chanIdx;
    settings.mAccessAddress = sChannelParams->accAddr;
    settings.mCrcInit       = sChannelParams->crcInit;

    otPlatRadioBleReceiveAtTifs(bleGetThreadInstance(), &settings);

exit:
    return;
}

void otPlatRadioBleReceiveDone(otInstance *aInstance, otRadioBleFrame *aFrame, otError aError)
{
    otEXPECT(sRxBuffer != NULL);
    otEXPECT(sDataParams != NULL);
    otEXPECT(sDataParams->rxCback != NULL);

    if (aFrame != NULL)
    {
        otEXPECT(aFrame->mLength <= sRxLength);

        memcpy(sRxBuffer, aFrame->mPdu, aFrame->mLength);
        sDataParams->rxCback(aError == OT_ERROR_NONE ? BB_STATUS_SUCCESS : BB_STATUS_FAILED, aFrame->mRxInfo.mRssi, 0,
                             aFrame->mRxInfo.mTicks, BB_PHY_OPTIONS_DEFAULT);
    }
    else
    {
        sDataParams->rxCback(aError == OT_ERROR_NONE ? BB_STATUS_SUCCESS : BB_STATUS_FAILED, 0, 0, 0,
                             BB_PHY_OPTIONS_DEFAULT);
    }

    sRxBuffer = NULL;
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void BbBleDrvCancelTifs(void)
{
    otPlatRadioBleCancelTifs(bleGetThreadInstance());
}

void BbBleDrvCancelData(void)
{
    otPlatRadioBleCancelData(bleGetThreadInstance());
}

void BbBleDrvRand(uint8_t *pBuf, uint8_t len)
{
    uint8_t i;

    for (i = 0; i < len; i++)
    {
        pBuf[i] = (uint8_t)(otPlatRandomGet());
    }
}

/**
 * Unused BLE module functions definition.
 *
 */

// This function is used by the BLE security module.
void BbBleDrvAesInitCipherBlock(BbBleEnc_t *pEnc, uint8_t id, uint8_t localDir)
{
    OT_UNUSED_VARIABLE(pEnc);
    OT_UNUSED_VARIABLE(id);
    OT_UNUSED_VARIABLE(localDir);
}

// This function is used by the BLE security module.
bool_t BbBleDrvAesCcmEncrypt(BbBleEnc_t *pEnc, uint8_t *pHdr, uint8_t *pBuf, uint8_t *pMic)
{
    OT_UNUSED_VARIABLE(pEnc);
    OT_UNUSED_VARIABLE(pHdr);
    OT_UNUSED_VARIABLE(pBuf);
    OT_UNUSED_VARIABLE(pMic);
    return false;
}

// This function is used by the BLE security module.
bool_t BbBleDrvAesCcmDecrypt(BbBleEnc_t *pEnc, uint8_t *pBuf)
{
    OT_UNUSED_VARIABLE(pEnc);
    OT_UNUSED_VARIABLE(pBuf);

    return false;
}

// This function is used by the BLE security module.
void LlMathAesEcb(const uint8_t *pKey, uint8_t *pOut, const uint8_t *pIn)
{
    OT_UNUSED_VARIABLE(pKey);
    OT_UNUSED_VARIABLE(pOut);
    OT_UNUSED_VARIABLE(pIn);
}

// This function is used by the BLE DTM module.
void BbBleDrvEnableDataWhitening(bool_t enable)
{
    OT_UNUSED_VARIABLE(enable);
}

// This function is used by the BLE DTM module.
void BbBleDrvEnablePrbs15(bool_t enable)
{
    OT_UNUSED_VARIABLE(enable);
}

// This function is used by the BLE 5.0 module.
void BbBleRfGetSupTxPower(int8_t *pMinTxPwr, int8_t *pMaxTxPwr)
{
    OT_UNUSED_VARIABLE(pMinTxPwr);
    OT_UNUSED_VARIABLE(pMaxTxPwr);
}

// This function is used by the BLE 5.0 module.
void BbBleRfReadRfPathComp(int16_t *pTxPathComp, int16_t *pRxPathComp)
{
    OT_UNUSED_VARIABLE(pTxPathComp);
    OT_UNUSED_VARIABLE(pRxPathComp);
}

// This function is used by the BLE 5.0 module.
bool_t BbBleRfWriteRfPathComp(int16_t txPathComp, int16_t rxPathComp)
{
    OT_UNUSED_VARIABLE(txPathComp);
    OT_UNUSED_VARIABLE(rxPathComp);

    return false;
}
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER
