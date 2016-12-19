/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the CLI interpreter.
 */

#include <common/logging.hpp>
#include <openthread/platform/ble.h>
#include <openthread/platform/toolchain.h>

OT_TOOL_WEAK
void otPlatBleOnEnabled(otInstance *aInstance)
{
    (void)aInstance;

    otLogInfoBle("[API] %s id=%d", __func__);
}

OT_TOOL_WEAK
void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    (void)aInstance;
    (void)aConnectionId;

    otLogInfoBle("[API] %s id=%d", __func__, aConnectionId);
}

OT_TOOL_WEAK
void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    (void)aInstance;
    (void)aConnectionId;

    otLogInfoBle("[API] %s id=%d", __func__, aConnectionId);
}

OT_TOOL_WEAK
void otPlatBleGapOnAdvReceived(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aAddress;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGapOnScanRespReceived(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aAddress;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle)
{
    (void)aInstance;
    (void)aHandle;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance, uint16_t aHandle)
{
    (void)aInstance;
    (void)aHandle;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnIndication(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aHandle;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                            uint16_t    aStartHandle,
                                            uint16_t    aEndHandle,
                                            uint16_t    aServiceUuid,
                                            otError     aError)
{
    (void)aInstance;
    (void)aStartHandle;
    (void)aEndHandle;
    (void)aServiceUuid;
    (void)aError;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                      otPlatBleGattCharacteristic *aChars,
                                                      uint16_t                     aCount,
                                                      otError                      aError)
{
    (void)aInstance;
    (void)aChars;
    (void)aCount;
    (void)aError;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                  otPlatBleGattDescriptor *aDescs,
                                                  uint16_t                 aCount,
                                                  otError                  aError)
{
    (void)aInstance;
    (void)aDescs;
    (void)aCount;
    (void)aError;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError)
{
    (void)aInstance;
    (void)aMtu;
    (void)aError;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance, uint16_t aHandle)
{
    (void)aInstance;
    (void)aHandle;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aHandle;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattServerOnReadRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aHandle;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    (void)aInstance;
    (void)aHandle;
    (void)aSubscribing;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleL2capOnConnectionRequest(otInstance *aInstance, uint16_t aPsm, uint16_t aMtu, uint16_t aPeerCid)
{
    (void)aInstance;
    (void)aPsm;
    (void)aMtu;
    (void)aPeerCid;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleL2capOnConnectionResponse(otInstance *        aInstance,
                                        otPlatBleL2capError aResult,
                                        uint16_t            aMtu,
                                        uint16_t            aPeerCid)
{
    (void)aInstance;
    (void)aResult;
    (void)aMtu;
    (void)aPeerCid;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleL2capOnSduReceived(otInstance *      aInstance,
                                 uint16_t          aLocalCid,
                                 uint16_t          aPeerCid,
                                 otBleRadioPacket *aPacket)
{
    (void)aInstance;
    (void)aLocalCid;
    (void)aPeerCid;
    (void)aPacket;

    otLogInfoBle("[API] %s", __func__);
}

OT_TOOL_WEAK
void otPlatBleL2capOnSduSent(otInstance *aInstance)
{
    (void)aInstance;
}

OT_TOOL_WEAK
void otPlatBleL2capOnDisconnect(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid)
{
    (void)aInstance;
    (void)aLocalCid;
    (void)aPeerCid;

    otLogInfoBle("[API] %s", __func__);
}
