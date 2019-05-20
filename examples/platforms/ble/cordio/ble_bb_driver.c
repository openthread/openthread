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

#include <assert.h>
#include <string.h>
#include <openthread/error.h>
#include <openthread/platform/cordio/radio-ble.h>
#include <openthread/platform/random.h>

#if OPENTHREAD_ENABLE_BLE_CONTROLLER

#define BLE_RADIO_NUM_FRAME_DESC 3

static const BbBleDrvDataParam_t *sDataParams = NULL;

static uint8_t ConvertErrorCode(otRadioBleError aError)
{
    uint8_t error = BB_STATUS_FAILED;

    switch (aError)
    {
    case OT_BLE_RADIO_ERROR_NONE:
        error = BB_STATUS_SUCCESS;
        break;

    case OT_BLE_RADIO_ERROR_CRC:
        error = BB_STATUS_CRC_FAILED;
        break;

    case OT_BLE_RADIO_ERROR_RX_TIMEOUT:
        error = BB_STATUS_RX_TIMEOUT;
        break;

    case OT_BLE_RADIO_ERROR_FAILED:
        error = BB_STATUS_FAILED;
        break;
    }

    return error;
}

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
    return otCordioPlatRadioGetTickNow(bleGetThreadInstance());
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
    otCordioPlatRadioEnable(bleGetThreadInstance());
}

void BbBleDrvDisable(void)
{
    otCordioPlatRadioDisable(bleGetThreadInstance());
}

void BbBleDrvSetChannelParam(BbBleDrvChan_t *pChan)
{
    otRadioBleChannelParams channelParams;

    channelParams.mChannel       = pChan->chanIdx;
    channelParams.mAccessAddress = pChan->accAddr;
    channelParams.mCrcInit       = pChan->crcInit;

    otCordioPlatRadioSetChannelParameters(bleGetThreadInstance(), &channelParams);
    otCordioPlatRadioSetTransmitPower(bleGetThreadInstance(), pChan->txPower);
}

int8_t BbBleRfGetActualTxPower(int8_t txPwr, bool_t compFlag)
{
    OT_UNUSED_VARIABLE(txPwr);
    OT_UNUSED_VARIABLE(compFlag);

    return otCordioPlatRadioGetTransmitPower(bleGetThreadInstance());
}

void BbBleDrvSetDataParams(const BbBleDrvDataParam_t *pParam)
{
    sDataParams = pParam;
}

void BbBleDrvSetOpParams(const BbBleDrvOpParam_t *pOpParam)
{
    otEXPECT(pOpParam != NULL);

    if (pOpParam->ifsSetup)
    {
        otCordioPlatRadioEnableTifs(bleGetThreadInstance());
    }
    else
    {
        otCordioPlatRadioDisableTifs(bleGetThreadInstance());
    }

exit:
    return;
}

void otCordioPlatRadioTransmitDone(otInstance *aInstance, otRadioBleError aError)
{
    otEXPECT(sDataParams != NULL);
    otEXPECT(sDataParams->txCback != NULL);

    sDataParams->txCback(ConvertErrorCode(aError));
    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void BbBleDrvTxData(BbBleDrvTxBufDesc_t descs[], uint8_t cnt)
{
    otRadioBleBufferDescriptor bufferDescriptors[BLE_RADIO_NUM_FRAME_DESC];
    otRadioBleTime             time;
    uint16_t                   i;

    otEXPECT(sDataParams != NULL);
    otEXPECT(cnt <= BLE_RADIO_NUM_FRAME_DESC);

    for (i = 0; i < cnt; i++)
    {
        bufferDescriptors[i].mBuffer = descs[i].pBuf;
        bufferDescriptors[i].mLength = descs[i].len;
    }

    time.mTicks    = sDataParams->due;
    time.mOffsetUs = sDataParams->dueOffsetUsec;

    otCordioPlatRadioTransmitAtTime(bleGetThreadInstance(), bufferDescriptors, cnt, &time);

exit:
    return;
}

void BbBleDrvTxTifsData(BbBleDrvTxBufDesc_t descs[], uint8_t cnt)
{
    otRadioBleBufferDescriptor bufferDescriptors[BLE_RADIO_NUM_FRAME_DESC];
    uint16_t                   i;

    otEXPECT(cnt <= BLE_RADIO_NUM_FRAME_DESC);

    for (i = 0; i < cnt; i++)
    {
        bufferDescriptors[i].mBuffer = descs[i].pBuf;
        bufferDescriptors[i].mLength = descs[i].len;
    }

    otCordioPlatRadioTransmitAtTifs(bleGetThreadInstance(), bufferDescriptors, cnt);

exit:
    return;
}

void BbBleDrvRxData(uint8_t *pBuf, uint16_t len)
{
    otRadioBleBufferDescriptor bufferDescriptor;
    otRadioBleTime             time;

    otEXPECT(sDataParams != NULL);

    time.mTicks        = sDataParams->due;
    time.mOffsetUs     = sDataParams->dueOffsetUsec;
    time.mRxDurationUs = sDataParams->rxTimeoutUsec;

    bufferDescriptor.mBuffer = pBuf;
    bufferDescriptor.mLength = len;

    otCordioPlatRadioReceiveAtTime(bleGetThreadInstance(), &bufferDescriptor, &time);

exit:
    return;
}

void BbBleDrvRxTifsData(uint8_t *pBuf, uint16_t len)
{
    otRadioBleBufferDescriptor bufferDescriptor;

    bufferDescriptor.mBuffer = pBuf;
    bufferDescriptor.mLength = len;

    otCordioPlatRadioReceiveAtTifs(bleGetThreadInstance(), &bufferDescriptor);
}

void otCordioPlatRadioReceiveDone(otInstance *aInstance, otRadioBleRxInfo *aRxInfo, otRadioBleError aError)
{
    otEXPECT(sDataParams != NULL);
    otEXPECT(sDataParams->rxCback != NULL);

    if (aError == OT_BLE_RADIO_ERROR_NONE)
    {
        assert(aRxInfo != NULL);
        sDataParams->rxCback(ConvertErrorCode(aError), aRxInfo->mRssi, 0, aRxInfo->mTicks, BB_PHY_OPTIONS_DEFAULT);
    }
    else
    {
        sDataParams->rxCback(ConvertErrorCode(aError), 0, 0, 0, BB_PHY_OPTIONS_DEFAULT);
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void BbBleDrvCancelTifs(void)
{
    otCordioPlatRadioCancelTifs(bleGetThreadInstance());
}

void BbBleDrvCancelData(void)
{
    otCordioPlatRadioCancelData(bleGetThreadInstance());
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
