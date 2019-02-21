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
 *   This file implements the OpenThread BLE APIs for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "ble/ble_gap.h"
#include "ble/ble_mgmt.h"

#include <openthread/platform/ble.h>
#include <openthread/platform/toolchain.h>

#if OPENTHREAD_ENABLE_TOBLE

/*******************************************************************************
 * @section Bluetooth Low Energy management.
 ******************************************************************************/
otError otPlatBleEnable(otInstance *aInstance)
{
    return bleMgmtEnable(aInstance);
}

otError otPlatBleDisable(otInstance *aInstance)
{
    return bleMgmtDisable(aInstance);
}

bool otPlatBleIsEnabled(otInstance *aInstance)
{
    return bleMgmtIsEnabled(aInstance);
}

void otPlatBleTaskletsProcess(otInstance *aInstance)
{
    bleMgmtTaskletsProcess(aInstance);
}

/****************************************************************************
 * @section Bluetooth Low Energy GAP.
 ***************************************************************************/

otError otPlatBleGapAddressSet(otInstance *aInstance, const otPlatBleDeviceAddr *aAddress)
{
    return bleGapAddressSet(aInstance, aAddress);
}

otError otPlatBleGapAddressGet(otInstance *aInstance, otPlatBleDeviceAddr *aAddress)
{
    return bleGapAddressGet(aInstance, aAddress);
}

otError otPlatBleGapServiceSet(otInstance *aInstance, const char *aDeviceName, uint16_t aAppearance)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aDeviceName);
    OT_UNUSED_VARIABLE(aAppearance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGapAdvDataSet(otInstance *aInstance, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    return bleGapAdvDataSet(aInstance, aAdvData, aAdvDataLength);
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval, uint8_t aType)
{
    return bleGapAdvStart(aInstance, aInterval, aType);
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
    return bleGapAdvStop(aInstance);
}

otError otPlatBleGapScanResponseSet(otInstance *aInstance, const uint8_t *aScanResponse, uint8_t aScanResponseLength)
{
    return bleGapScanResponseSet(aInstance, aScanResponse, aScanResponseLength);
}

otError otPlatBleGapScanStart(otInstance *aInstance, uint16_t aInterval, uint16_t aWindow)
{
    return bleGapScanStart(aInstance, aInterval, aWindow);
}

otError otPlatBleGapScanStop(otInstance *aInstance)
{
    return bleGapScanStop(aInstance);
}

otError otPlatBleGapConnParamsSet(otInstance *aInstance, const otPlatBleGapConnParams *aConnParams)
{
    return bleGapConnParamsSet(aInstance, aConnParams);
}

otError otPlatBleGapConnect(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, uint16_t aInterval, uint16_t aWindow)
{
    return bleGapConnect(aInstance, aAddress, aInterval, aWindow);
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
    return bleGapDisconnect(aInstance);
}

OT_TOOL_WEAK void otPlatBleGapOnConnected(otInstance *aInstance, uint16_t aConnectionId)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aConnectionId);
}

OT_TOOL_WEAK void otPlatBleGapOnDisconnected(otInstance *aInstance, uint16_t aConnectionId)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aConnectionId);
}

OT_TOOL_WEAK void otPlatBleGapOnAdvReceived(otInstance *         aInstance,
                                            otPlatBleDeviceAddr *aAddress,
                                            otBleRadioPacket *   aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aPacket);
}

OT_TOOL_WEAK void otPlatBleGapOnScanRespReceived(otInstance *         aInstance,
                                                 otPlatBleDeviceAddr *aAddress,
                                                 otBleRadioPacket *   aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aPacket);
}

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Client.
 ******************************************************************************/
otError otPlatBleGattClientRead(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPacket);
}

otError otPlatBleGattClientWrite(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
}

otError otPlatBleGattClientSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aSubscribing);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnSubscribeResponse(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
}

OT_TOOL_WEAK void otPlatBleGattClientOnIndication(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
}

otError otPlatBleGattClientServicesDiscover(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGattClientServiceDiscover(otInstance *aInstance, const otPlatBleUuid *aUuid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aUuid);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnServiceDiscovered(otInstance *aInstance,
                                                         uint16_t    aStartHandle,
                                                         uint16_t    aEndHandle,
                                                         uint16_t    aServiceUuid,
                                                         otError     aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartHandle);
    OT_UNUSED_VARIABLE(aEndHandle);
    OT_UNUSED_VARIABLE(aServiceUuid);
    OT_UNUSED_VARIABLE(aError);
}

otError otPlatBleGattClientCharacteristicsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartHandle);
    OT_UNUSED_VARIABLE(aEndHandle);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnCharacteristicsDiscoverDone(otInstance *                 aInstance,
                                                                   otPlatBleGattCharacteristic *aChars,
                                                                   uint16_t                     aCount,
                                                                   otError                      aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aChars);
    OT_UNUSED_VARIABLE(aCount);
    OT_UNUSED_VARIABLE(aError);
}

otError otPlatBleGattClientDescriptorsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aStartHandle);
    OT_UNUSED_VARIABLE(aEndHandle);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnDescriptorsDiscoverDone(otInstance *             aInstance,
                                                               otPlatBleGattDescriptor *aDescs,
                                                               uint16_t                 aCount,
                                                               otError                  aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aDescs);
    OT_UNUSED_VARIABLE(aCount);
    OT_UNUSED_VARIABLE(aError);
}

otError otPlatBleGattClientMtuExchangeRequest(otInstance *aInstance, uint16_t aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMtu);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aError);
}

/*******************************************************************************
 * @section Bluetooth Low Energy GATT Server.
 ******************************************************************************/
otError otPlatBleGattServerServiceRegister(otInstance *aInstance, const otPlatBleUuid *aUuid, uint16_t *aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aUuid);
    OT_UNUSED_VARIABLE(aHandle);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGattServerCharacteristicRegister(otInstance *                 aInstance,
                                                  uint16_t                     aServiceHandle,
                                                  otPlatBleGattCharacteristic *aChar,
                                                  bool                         aCccd)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aServiceHandle);
    OT_UNUSED_VARIABLE(aChar);
    OT_UNUSED_VARIABLE(aCccd);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleGattServerOnIndicationConfirmation(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
}

OT_TOOL_WEAK void otPlatBleGattServerOnWriteRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
}

OT_TOOL_WEAK void otPlatBleGattServerOnSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aSubscribing);
}

otError otPlatBleL2capConnectionRequest(otInstance *aInstance, uint16_t aPsm, uint16_t aMtu, uint16_t *aCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPsm);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aCid);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capConnectionResponse(otInstance *        aInstance,
                                         otPlatBleL2capError aError,
                                         uint16_t            aMtu,
                                         uint16_t *          aCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aCid);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capSduSend(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLocalCid);
    OT_UNUSED_VARIABLE(aPeerCid);
    OT_UNUSED_VARIABLE(aPacket);
    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatBleL2capOnConnectionRequest(otInstance *aInstance,
                                                    uint16_t    aPsm,
                                                    uint16_t    aMtu,
                                                    uint16_t    aPeerCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPsm);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aPeerCid);
}

OT_TOOL_WEAK void otPlatBleL2capOnSduReceived(otInstance *      aInstance,
                                              uint16_t          aLocalCid,
                                              uint16_t          aPeerCid,
                                              otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLocalCid);
    OT_UNUSED_VARIABLE(aPeerCid);
    OT_UNUSED_VARIABLE(aPacket);
}

OT_TOOL_WEAK void otPlatBleL2capOnSduSent(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

OT_TOOL_WEAK void otPlatBleL2capOnDisconnect(otInstance *aInstance, uint16_t aLocalCid, uint16_t aPeerCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLocalCid);
    OT_UNUSED_VARIABLE(aPeerCid);
}

OT_TOOL_WEAK void otPlatBleL2capOnConnectionResponse(otInstance *        aInstance,
                                                     otPlatBleL2capError aError,
                                                     uint16_t            aMtu,
                                                     uint16_t            aPeerCid)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aPeerCid);
}
#endif // OPENTHREAD_ENABLE_TOBLE
