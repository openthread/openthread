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
 *   This file implements the BLE GATT interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "wsf_types.h"

#include "att_api.h"
#include "dm_api.h"
#include "l2c_defs.h"
#include "wsf_math.h"

#include <string.h>

#include "cordio/ble_gap.h"
#include "cordio/ble_config.h"
#include "cordio/ble_init.h"
#include "cordio/ble_utils.h"
#include "utils/code_utils.h"

#include <openthread/platform/ble.h>
#include <openthread/platform/toolchain.h>

#if OPENTHREAD_ENABLE_BLE_HOST

enum
{
    kUuidFormat16Bit  = 0x01,
    kUuidFormat128Bit = 0x02,
};

enum
{
    kMtuStateIdle                = 0,
    kMtuStateSentMtuRequest      = 1,
    kMtuStateReceivedMtuResponse = 2,
    kMtuStateTimeout             = 3,
};

enum
{
    kExchangeMtuTimeout = 5000
};

enum
{
    kGattSubscribeValue   = 0x0002,
    kGattUnsubscribeValue = 0x0000,
};

enum
{
    kMaxGattGapAttrNum     = 5,
    kMaxGattCharsNum       = 2,
    kMaxGattAttrNum        = (kMaxGattCharsNum << 2) + 1,
    kMaxGattLengthArrayNum = kMaxGattCharsNum,
    kMaxGattCccdNum        = kMaxGattCharsNum,
};

OT_TOOL_PACKED_BEGIN struct Characteristic
{
    uint8_t  mProperties;                   ///< Characteristic Properties
    uint16_t mCharVauleHanle;               ///< Characteristic Value Handle
    uint8_t  mCharUuid[OT_BLE_UUID_LENGTH]; ///< Characteristic UUID
    uint8_t  mUuidLength;                   ///< The Length of Characteristic UUID
} OT_TOOL_PACKED_END;

typedef struct Characteristic Characteristic;

typedef struct GapService
{
    attsGroup_t    mService;
    Characteristic mDeviceNameChar;
    char           mDeviceName[OT_BLE_DEV_NAME_MAX_LENGTH];
    uint16_t       mDeviceNameLength;
    Characteristic mAppearanceChar;
    uint16_t       mAppearance;
    attsAttr_t     mAttributes[kMaxGattGapAttrNum];
} GapService;

typedef struct GattService
{
    attsGroup_t    mService;
    Characteristic mCharacteristics[kMaxGattCharsNum];
    attsAttr_t     mAttributes[kMaxGattAttrNum];
    uint16_t       mLengthArrays[kMaxGattLengthArrayNum];
    uint8_t        mCharacteristicIndex;
    uint8_t        mAttributeIndex;
    uint8_t        mLengthArrayIndex;
} GattService;

typedef struct Cccd
{
    uint16_t     mValues[kMaxGattCccdNum];
    attsCccSet_t mCccds[kMaxGattCccdNum];
    uint8_t      mCccdIndex;
} Cccd;

uint16_t sGattHandle = 0;

static GapService sGapService = {
    .mService.startHandle = 0,
};
static GattService sGattService = {.mService.startHandle = 0,
                                    .mCharacteristicIndex = 0,
                                    .mAttributeIndex      = 0,
                                    .mLengthArrayIndex    = 0};
static Cccd        sCccd         = {.mCccdIndex = 0};

static otPlatBleUuid sServiceDiscoverUuid;

static uint16_t sCccdWriteHandle = 0;
static uint16_t sCharDiscoverEndHandle;
static uint16_t sDescDiscoverEndHandle;

static bool sServicesDiscovered;
static bool sServiceDiscovered;
static bool sCharacteristicDiscovered;
static bool sDescriptorDiscovered;

static wsfTimer_t sTimer;
static uint8_t    sMtuState            = kMtuStateIdle;
static uint8_t    sMtu                 = 0;
static bool       sWaittingMtuResponse = false;

static void SetUuid16(otPlatBleUuid *aUuid, const uint8_t *aUuid16)
{
    aUuid->mType          = OT_BLE_UUID_TYPE_16;
    aUuid->mValue.mUuid16 = *(uint16_t *)(aUuid16);
}

static void SetUuid128(otPlatBleUuid *aUuid, uint8_t *aUuid128)
{
    aUuid->mType           = OT_BLE_UUID_TYPE_128;
    aUuid->mValue.mUuid128 = aUuid128;
}

static uint8_t GetUuidLength(const otPlatBleUuid *aUuid)
{
    uint8_t len = 0;

    if (aUuid->mType == OT_BLE_UUID_TYPE_128)
    {
        len = OT_BLE_UUID_LENGTH;
    }
    else if (aUuid->mType == OT_BLE_UUID_TYPE_16)
    {
        len = OT_BLE_UUID16_LENGTH;
    }

    return len;
}

static uint8_t *GetUuid(const otPlatBleUuid *aUuid)
{
    uint8_t *uuid = NULL;

    if (aUuid->mType == OT_BLE_UUID_TYPE_128)
    {
        uuid = aUuid->mValue.mUuid128;
    }
    else if (aUuid->mType == OT_BLE_UUID_TYPE_16)
    {
        uuid = (uint8_t *)(&aUuid->mValue.mUuid16);
    }

    return uuid;
}

static void SetCharacteristic(Characteristic *aChar, uint8_t aProperities, uint16_t aHandle, const otPlatBleUuid *aUuid)
{
    aChar->mProperties     = aProperities;
    aChar->mCharVauleHanle = aHandle;
    aChar->mUuidLength     = GetUuidLength(aUuid);

    memcpy(aChar->mCharUuid, GetUuid(aUuid), GetUuidLength(aUuid));
}

static uint8_t GetCharacteristicLength(const Characteristic *aChar)
{
    return sizeof(aChar->mProperties) + sizeof(aChar->mCharVauleHanle) + aChar->mUuidLength;
}

static otError AttToOtError(uint8_t aError)
{
    otError error;

    switch (aError)
    {
    case ATT_SUCCESS:
        error = OT_ERROR_NONE;
        break;
    case ATT_ERR_NOT_FOUND:
        error = OT_ERROR_NOT_FOUND;
        break;
    default:
        error = OT_ERROR_FAILED;
        break;
    }

    return error;
}

static void gattTimerHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    OT_UNUSED_VARIABLE(event);
    OT_UNUSED_VARIABLE(pMsg);

    sMtuState = kMtuStateTimeout;
}

void bleGattInit(void)
{
    sTimer.handlerId = WsfOsSetNextHandler(gattTimerHandler);
}

void bleGattReset(void)
{
    if (sGapService.mService.startHandle != 0)
    {
        AttsRemoveGroup(sGapService.mService.startHandle);
    }

    if (sGattService.mService.startHandle != 0)
    {
        AttsRemoveGroup(sGattService.mService.startHandle);
    }

    memset(&sGapService, 0, sizeof(sGapService));
    memset(&sGattService, 0, sizeof(sGattService));

    sCccd.mCccdIndex = 0;
    sGattHandle      = 0;
    sCccdWriteHandle = 0;
    sMtuState        = kMtuStateIdle;

    WsfTimerStop(&sTimer);
}

void bleGattGapConnectedHandler(const wsfMsgHdr_t *aMsg)
{
    const hciLeConnCmplEvt_t *connEvent = (const hciLeConnCmplEvt_t *)aMsg;
    dmConnId_t                connId;
    uint8_t                   localMtu;

    otEXPECT(connEvent->status == 0);

    // Note:
    // <1> If the device is master and localMtu unequals to ATT_DEFAULT_MTU, the Cordio stack
    //     automatically send MTU Exchange Request packet to the peer when the BLE connection is connected.
    // <2> If no MTU Exchange Response packet is received after a MTU Exchange Request packet is sent, the
    //     Cordio stack won't generate an event to notify user.
    // <3> If the Cordio stack has previously sent an MTU Exchange Request packet, the Cordio stack won't
    //     send MTU Exchange Request packet again even if user calls function AttcMtuReq().

    localMtu = WSF_MIN(BLE_STACK_ATT_MTU, (HciGetMaxRxAclLen() - L2C_HDR_LEN));
    connId   = (dmConnId_t)connEvent->hdr.param;

    otEXPECT((DmConnRole(connId) == DM_ROLE_MASTER) && (localMtu != ATT_DEFAULT_MTU));

    sMtuState = kMtuStateSentMtuRequest;
    WsfTimerStartMs(&sTimer, kExchangeMtuTimeout);

exit:
    return;
}

void bleGattGapDisconnectedHandler(const wsfMsgHdr_t *aMsg)
{
    sMtuState = kMtuStateIdle;
    OT_UNUSED_VARIABLE(aMsg);
}

static void gattProcessMtuUpdateInd(attEvt_t *aEvent)
{
    if (aEvent->hdr.status == ATT_SUCCESS)
    {
        sMtu      = aEvent->mtu;
        sMtuState = kMtuStateReceivedMtuResponse;
        WsfTimerStop(&sTimer);
    }

    if (sWaittingMtuResponse)
    {
        sWaittingMtuResponse = false;
        otPlatBleGattClientOnMtuExchangeResponse(bleGetThreadInstance(), aEvent->mtu, AttToOtError(aEvent->hdr.status));
    }
}

static void gattProcessClientReadRsp(attEvt_t *aEvent)
{
    if (aEvent->hdr.status == ATT_SUCCESS)
    {
        otBleRadioPacket packet;

        packet.mValue  = aEvent->pValue;
        packet.mLength = aEvent->valueLen;

        otPlatBleGattClientOnReadResponse(bleGetThreadInstance(), &packet);
    }
}

static void gattProcessClientWriteRsp(attEvt_t *aEvent)
{
    if (aEvent->handle == sCccdWriteHandle)
    {
        sCccdWriteHandle = 0;
        otEXPECT(aEvent->hdr.status == ATT_SUCCESS);
        otPlatBleGattClientOnSubscribeResponse(bleGetThreadInstance(), aEvent->handle);
    }
    else
    {
        otEXPECT(aEvent->hdr.status == ATT_SUCCESS);
        otPlatBleGattClientOnWriteResponse(bleGetThreadInstance(), aEvent->handle);
    }

exit:
    return;
}

static void gattProcessClientReadByGroupRsp(attEvt_t *aEvent)
{
    otError  error          = AttToOtError(aEvent->hdr.status);
    uint8_t *p              = aEvent->pValue;
    uint8_t *end            = aEvent->pValue + aEvent->valueLen;
    uint16_t endGroupHandle = 0;
    uint8_t  length;
    uint16_t attHandle;
    uint16_t uuid;

    otEXPECT(error == OT_ERROR_NONE);

    BYTES_TO_UINT8(length, p);

    otEXPECT_ACTION(length > sizeof(attHandle) + sizeof(endGroupHandle), error = OT_ERROR_FAILED);

    length = length - sizeof(attHandle) - sizeof(endGroupHandle);
    otEXPECT_ACTION((length == OT_BLE_UUID_LENGTH) || (length == OT_BLE_UUID16_LENGTH), error = OT_ERROR_FAILED);

    while (p < end)
    {
        BYTES_TO_UINT16(attHandle, p);
        BYTES_TO_UINT16(endGroupHandle, p);
        if (length == OT_BLE_UUID16_LENGTH)
        {
            sServicesDiscovered = true;

            BYTES_TO_UINT16(uuid, p);
            otPlatBleGattClientOnServiceDiscovered(bleGetThreadInstance(), attHandle, endGroupHandle, uuid,
                                                   OT_ERROR_NONE);
        }
        else
        {
            p += OT_BLE_UUID_LENGTH;
        }
    }

    if (endGroupHandle < ATT_HANDLE_MAX)
    {
        uuid = ATT_UUID_PRIMARY_SERVICE;

        otEXPECT(bleGapGetConnectionId() != DM_CONN_ID_NONE);
        AttcReadByGroupTypeReq(bleGapGetConnectionId(), endGroupHandle + 1, ATT_HANDLE_MAX, sizeof(uuid),
                               (uint8_t *)&uuid, false);
    }

exit:
    if ((!sServicesDiscovered) && (error != OT_ERROR_NONE))
    {
        otPlatBleGattClientOnServiceDiscovered(bleGetThreadInstance(), 0, 0, 0, error);
    }

    return;
}

static void gattProcessClientFindByTypeValueRsp(attEvt_t *aEvent)
{
    otError  error = AttToOtError(aEvent->hdr.status);
    uint8_t *p     = aEvent->pValue;
    uint8_t *end   = aEvent->pValue + aEvent->valueLen;
    uint16_t attrHandle;
    uint16_t groupEndHandle = 0;

    otEXPECT(error == OT_ERROR_NONE);
    otEXPECT_ACTION(aEvent->valueLen >= sizeof(attrHandle) + sizeof(groupEndHandle), error = OT_ERROR_FAILED);

    while (p < end)
    {
        BYTES_TO_UINT16(attrHandle, p);
        BYTES_TO_UINT16(groupEndHandle, p);

        sServiceDiscovered = true;
        otPlatBleGattClientOnServiceDiscovered(bleGetThreadInstance(), attrHandle, groupEndHandle,
                                               sServiceDiscoverUuid.mValue.mUuid16, OT_ERROR_NONE);
    }

    if (groupEndHandle != ATT_HANDLE_MAX)
    {
        otEXPECT(bleGapGetConnectionId() != DM_CONN_ID_NONE);

        AttcFindByTypeValueReq(bleGapGetConnectionId(), groupEndHandle + 1, ATT_HANDLE_MAX, ATT_UUID_PRIMARY_SERVICE,
                               GetUuidLength(&sServiceDiscoverUuid), GetUuid(&sServiceDiscoverUuid), false);
    }

exit:
    if (!sServiceDiscovered && (error != OT_ERROR_NONE))
    {
        otPlatBleGattClientOnServiceDiscovered(bleGetThreadInstance(), 0, 0, 0, error);
    }

    return;
}

static void gattProcessClientReadByTypeRsp(attEvt_t *aEvent)
{
    const uint8_t               kMinReadByTypeRspLength = 8;
    const uint8_t               kNumGattChars           = 5;
    otPlatBleGattCharacteristic gattChars[kNumGattChars];
    otError                     error      = AttToOtError(aEvent->hdr.status);
    uint8_t *                   p          = aEvent->pValue;
    uint8_t *                   end        = aEvent->pValue + aEvent->valueLen;
    uint8_t                     i          = 0;
    uint16_t                    attrHandle = 0;
    uint8_t                     pairLength;
    uint8_t                     uuidLength;
    uint8_t                     properties;
    uint16_t                    charsValueHandle;

    otEXPECT(error == OT_ERROR_NONE);
    otEXPECT_ACTION(aEvent->valueLen >= kMinReadByTypeRspLength, error = OT_ERROR_FAILED);

    BYTES_TO_UINT8(pairLength, p);
    uuidLength = pairLength - sizeof(attrHandle) - sizeof(properties) - sizeof(charsValueHandle);

    while ((p < end) && (i < kNumGattChars))
    {
        BYTES_TO_UINT16(attrHandle, p);
        BYTES_TO_UINT8(properties, p);
        BYTES_TO_UINT16(charsValueHandle, p);

        if (uuidLength == OT_BLE_UUID16_LENGTH)
        {
            SetUuid16(&gattChars[i].mUuid, p);
            p += OT_BLE_UUID16_LENGTH;
        }
        else if (uuidLength == OT_BLE_UUID_LENGTH)
        {
            SetUuid128(&gattChars[i].mUuid, p);
            p += OT_BLE_UUID_LENGTH;
        }
        else
        {
            otEXPECT_ACTION(0, error = OT_ERROR_FAILED);
        }

        gattChars[i].mHandleValue = charsValueHandle;
        gattChars[i].mProperties  = properties;
        i++;
    }

    sCharacteristicDiscovered = true;
    otPlatBleGattClientOnCharacteristicsDiscoverDone(bleGetThreadInstance(), gattChars, i, OT_ERROR_NONE);

    if (attrHandle + 1 <= sCharDiscoverEndHandle)
    {
        uint16_t uuid = ATT_UUID_CHARACTERISTIC;

        otEXPECT(bleGapGetConnectionId() != DM_CONN_ID_NONE);
        AttcReadByTypeReq(bleGapGetConnectionId(), attrHandle + 1, sCharDiscoverEndHandle, sizeof(uuid),
                          (uint8_t *)&uuid, false);
    }

exit:

    if (!sCharacteristicDiscovered && (error != OT_ERROR_NONE))
    {
        otPlatBleGattClientOnCharacteristicsDiscoverDone(bleGetThreadInstance(), NULL, 0, error);
    }

    return;
}

static void gattProcessClientFindInfoRsp(attEvt_t *aEvent)
{
    const uint8_t           kNumDescriptors = 5;
    otPlatBleGattDescriptor gattDescriptors[kNumDescriptors];
    otError                 error = AttToOtError(aEvent->hdr.status);
    uint8_t *               p     = aEvent->pValue;
    uint8_t *               end   = aEvent->pValue + aEvent->valueLen;
    uint8_t                 i     = 0;
    uint8_t                 format;

    otEXPECT(error == OT_ERROR_NONE);
    otEXPECT_ACTION(aEvent->valueLen >= sizeof(format) + sizeof(gattDescriptors[0].mHandle) + OT_BLE_UUID16_LENGTH,
                    error = OT_ERROR_FAILED);

    BYTES_TO_UINT8(format, p);

    while ((p < end) && (i < kNumDescriptors))
    {
        BYTES_TO_UINT16(gattDescriptors[i].mHandle, p);

        if (format == kUuidFormat16Bit)
        {
            SetUuid16(&gattDescriptors[i].mUuid, p);
            p += OT_BLE_UUID16_LENGTH;
        }
        else if (format == kUuidFormat128Bit)
        {
            SetUuid128(&gattDescriptors[i].mUuid, p);
            p += OT_BLE_UUID_LENGTH;
        }
        else
        {
            otEXPECT_ACTION(0, error = OT_ERROR_FAILED);
        }

        i++;
    }

    sDescriptorDiscovered = true;
    otPlatBleGattClientOnDescriptorsDiscoverDone(bleGetThreadInstance(), gattDescriptors, i, OT_ERROR_NONE);

    if (gattDescriptors[i - 1].mHandle + 1 <= sDescDiscoverEndHandle)
    {
        otEXPECT(bleGapGetConnectionId() != DM_CONN_ID_NONE);
        AttcFindInfoReq(bleGapGetConnectionId(), gattDescriptors[i - 1].mHandle + 1, sDescDiscoverEndHandle, false);
    }

exit:
    if (!sDescriptorDiscovered && (error != OT_ERROR_NONE))
    {
        otPlatBleGattClientOnDescriptorsDiscoverDone(bleGetThreadInstance(), NULL, 0, error);
    }

    return;
}

static void gattProcessClientHandleValueInd(attEvt_t *aEvent)
{
    otBleRadioPacket packet;

    otEXPECT(aEvent->hdr.status == ATT_SUCCESS);

    packet.mValue  = aEvent->pValue;
    packet.mLength = aEvent->valueLen;

    otPlatBleGattClientOnIndication(bleGetThreadInstance(), aEvent->handle, &packet);

exit:
    return;
}

static void gattProcessClientHandleValueConf(attEvt_t *aEvent)
{
    otEXPECT(aEvent->hdr.status == ATT_SUCCESS);

    otPlatBleGattServerOnIndicationConfirmation(bleGetThreadInstance(), aEvent->handle);

exit:
    return;
}

void bleAttHandler(attEvt_t *aEvent)
{
    switch (aEvent->hdr.event)
    {
    case ATT_MTU_UPDATE_IND:
        gattProcessMtuUpdateInd(aEvent);
        break;

    case ATTC_READ_RSP:
        gattProcessClientReadRsp(aEvent);
        break;

    case ATTC_WRITE_RSP:
        gattProcessClientWriteRsp(aEvent);
        break;

    case ATTC_READ_BY_GROUP_TYPE_RSP:
        gattProcessClientReadByGroupRsp(aEvent);
        break;

    case ATTC_FIND_BY_TYPE_VALUE_RSP:
        gattProcessClientFindByTypeValueRsp(aEvent);
        break;

    case ATTC_READ_BY_TYPE_RSP:
        gattProcessClientReadByTypeRsp(aEvent);
        break;

    case ATTC_FIND_INFO_RSP:
        gattProcessClientFindInfoRsp(aEvent);
        break;

    case ATTC_HANDLE_VALUE_IND:
        gattProcessClientHandleValueInd(aEvent);
        break;

    case ATTS_HANDLE_VALUE_CNF:
        gattProcessClientHandleValueConf(aEvent);
        break;
    }
}

otError otPlatBleGattClientMtuExchangeRequest(otInstance *aInstance, uint16_t aMtu)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    if (sMtuState == kMtuStateReceivedMtuResponse)
    {
        otPlatBleGattClientOnMtuExchangeResponse(bleGetThreadInstance(), sMtu, OT_ERROR_NONE);
    }
    else if (sMtuState == kMtuStateTimeout)
    {
        error = OT_ERROR_FAILED;
    }
    else if (sMtuState == kMtuStateSentMtuRequest)
    {
        sWaittingMtuResponse = true;
    }
    else
    {
        sWaittingMtuResponse = true;
        sMtuState            = kMtuStateSentMtuRequest;

        AttcMtuReq(bleGapGetConnectionId(), aMtu);
        WsfTimerStartMs(&sTimer, kExchangeMtuTimeout);
    }

exit:
    return error;
}

otError otPlatBleGattMtuGet(otInstance *aInstance, uint16_t *aMtu)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_FAILED);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_FAILED);
    otEXPECT_ACTION(sMtuState == kMtuStateReceivedMtuResponse, error = OT_ERROR_FAILED);

    *aMtu = AttGetMtu(bleGapGetConnectionId());

exit:
    return error;
}

otError otPlatBleGattClientRead(otInstance *aInstance, uint16_t aHandle)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    AttcReadReq(bleGapGetConnectionId(), aHandle);

exit:
    return error;
}

otError otPlatBleGattClientWrite(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(aPacket != NULL, error = OT_ERROR_INVALID_ARGS);

    AttcWriteReq(bleGapGetConnectionId(), aHandle, aPacket->mLength, aPacket->mValue);

exit:
    return error;
}

otError otPlatBleGattClientSubscribeRequest(otInstance *aInstance, uint16_t aHandle, bool aSubscribing)
{
    otError  error = OT_ERROR_NONE;
    uint16_t subscribeReqValue;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    subscribeReqValue = aSubscribing ? kGattSubscribeValue : kGattUnsubscribeValue;
    AttcWriteReq(bleGapGetConnectionId(), aHandle, sizeof(subscribeReqValue), (uint8_t *)&subscribeReqValue);

    sCccdWriteHandle = aHandle;

exit:
    return error;
}

static uint8_t GattServerReadCallback(dmConnId_t  aConnectionId,
                                      uint16_t    aHandle,
                                      uint8_t     aOperation,
                                      uint16_t    aOffset,
                                      attsAttr_t *aAttr)
{
    uint8_t error = ATT_ERR_NOT_SUP;

    if ((bleGapGetConnectionId() == aConnectionId) && (aOperation == ATT_PDU_READ_REQ) && (aOffset == 0))
    {
        otBleRadioPacket packet;

        otPlatBleGattServerOnReadRequest(bleGetThreadInstance(), aHandle, &packet);

        aAttr->pValue = packet.mValue;
        *aAttr->pLen  = packet.mLength;

        error = ATT_SUCCESS;
    }

    return error;
}

static uint8_t GattServerWriteCallback(dmConnId_t  aConnectionId,
                                       uint16_t    aHandle,
                                       uint8_t     aOperation,
                                       uint16_t    aOffset,
                                       uint16_t    aLength,
                                       uint8_t *   aValue,
                                       attsAttr_t *aAttr)
{
    uint8_t error = ATT_ERR_NOT_SUP;

    if ((bleGapGetConnectionId() == aConnectionId) && (aOperation == ATT_PDU_WRITE_REQ) && (aOffset == 0))
    {
        otBleRadioPacket packet;

        packet.mValue  = aValue;
        packet.mLength = aLength;

        otPlatBleGattServerOnWriteRequest(bleGetThreadInstance(), aHandle, &packet);
        error = ATT_SUCCESS;
    }

    OT_UNUSED_VARIABLE(aAttr);
    return error;
}

otError otPlatBleGapServiceSet(otInstance *aInstance, const char *aDeviceName, uint16_t aAppearance)
{
    otError       error = OT_ERROR_NONE;
    attsAttr_t *  attr  = sGapService.mAttributes;
    otPlatBleUuid uuid;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(sGapService.mService.startHandle == 0, error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(strlen(aDeviceName) <= OT_BLE_DEV_NAME_MAX_LENGTH, error = OT_ERROR_INVALID_ARGS);

    memcpy(sGapService.mDeviceName, aDeviceName, strlen(aDeviceName));
    sGapService.mAppearance = aAppearance;

    sGattHandle++;
    sGapService.mService.startHandle = sGattHandle;

    attr->pUuid       = attPrimSvcUuid;
    attr->pValue      = (uint8_t *)attGapSvcUuid;
    attr->maxLen      = sizeof(attGapSvcUuid);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    // incremented by two to get a pointer to the value handle
    sGattHandle += 2;

    SetUuid16(&uuid, attDnChUuid);
    SetCharacteristic(&sGapService.mDeviceNameChar, ATT_PROP_READ, sGattHandle, &uuid);

    attr++;
    attr->pUuid       = attChUuid;
    attr->pValue      = (uint8_t *)&sGapService.mDeviceNameChar;
    attr->maxLen      = GetCharacteristicLength(&sGapService.mDeviceNameChar);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    sGapService.mDeviceNameLength = (uint16_t)strlen(aDeviceName);

    attr++;
    attr->pUuid       = attDnChUuid;
    attr->pValue      = (uint8_t *)sGapService.mDeviceName;
    attr->maxLen      = OT_BLE_DEV_NAME_MAX_LENGTH;
    attr->pLen        = &sGapService.mDeviceNameLength;
    attr->settings    = ATTS_SET_VARIABLE_LEN;
    attr->permissions = ATTS_PERMIT_READ;

    // incremented by two to get a pointer to the value handle
    sGattHandle += 2;

    SetUuid16(&uuid, attApChUuid);
    SetCharacteristic(&sGapService.mAppearanceChar, ATT_PROP_READ, sGattHandle, &uuid);

    attr++;
    attr->pUuid       = attChUuid;
    attr->pValue      = (uint8_t *)&sGapService.mAppearanceChar;
    attr->maxLen      = GetCharacteristicLength(&sGapService.mAppearanceChar);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    attr++;
    attr->pUuid       = attApChUuid;
    attr->pValue      = (uint8_t *)&sGapService.mAppearance;
    attr->maxLen      = sizeof(sGapService.mAppearance);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    sGapService.mService.pNext      = NULL;
    sGapService.mService.pAttr      = sGapService.mAttributes;
    sGapService.mService.readCback  = GattServerReadCallback;
    sGapService.mService.writeCback = GattServerWriteCallback;
    sGapService.mService.endHandle  = sGattHandle;

    AttsAddGroup(&sGapService.mService);

exit:
    return error;
}

static otError AddPrimaryServiceAttribute(const otPlatBleUuid *aUuid, uint16_t *aHandle)
{
    otError     error = OT_ERROR_NONE;
    attsAttr_t *attr  = &sGattService.mAttributes[sGattService.mAttributeIndex];

    otEXPECT_ACTION(sGattService.mAttributeIndex < kMaxGattAttrNum, error = OT_ERROR_NO_BUFS);

    sGattHandle++;
    sGattService.mAttributeIndex++;

    attr->pUuid       = attPrimSvcUuid;
    attr->pValue      = GetUuid(aUuid);
    attr->maxLen      = GetUuidLength(aUuid);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    *aHandle = sGattHandle;

exit:
    return error;
}

static void SetAttributeSetting(attsAttr_t *aAttr, otPlatBleGattCharacteristic *aChar)
{
    if (aChar->mProperties & OT_BLE_CHAR_PROP_READ)
    {
        aAttr->settings |= ATTS_SET_READ_CBACK;
        aAttr->permissions |= ATTS_PERMIT_READ;
    }

    if (aChar->mProperties & OT_BLE_CHAR_PROP_WRITE)
    {
        aAttr->settings |= ATTS_SET_WRITE_CBACK;
        aAttr->permissions |= ATTS_PERMIT_WRITE;
    }

    if (aChar->mProperties & OT_BLE_CHAR_PROP_AUTH_SIGNED_WRITE)
    {
        aAttr->settings |= ATTS_SET_ALLOW_SIGNED;
    }

    if (aChar->mUuid.mType == OT_BLE_UUID_TYPE_128)
    {
        aAttr->settings |= ATTS_SET_UUID_128;
    }
}

static otError AddCharacteristicAttribute(otPlatBleGattCharacteristic *aChar)
{
    otError         error = OT_ERROR_NONE;
    attsAttr_t *    attr  = &sGattService.mAttributes[sGattService.mAttributeIndex];
    Characteristic *characteristic;

    otEXPECT_ACTION(sGattService.mAttributeIndex + 1 < kMaxGattAttrNum, error = OT_ERROR_NO_BUFS);
    otEXPECT_ACTION(sGattService.mCharacteristicIndex < kMaxGattCharsNum, error = OT_ERROR_NO_BUFS);

    // incremented by two to get a pointer to the value handle
    sGattHandle += 2;
    sGattService.mAttributeIndex += 2;

    characteristic = &sGattService.mCharacteristics[sGattService.mCharacteristicIndex++];
    SetCharacteristic(characteristic, aChar->mProperties, sGattHandle, &aChar->mUuid);

    // create characteristic declaration attribute
    attr->pUuid       = attChUuid;
    attr->pValue      = (uint8_t *)characteristic;
    attr->maxLen      = GetCharacteristicLength(characteristic);
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = ATTS_PERMIT_READ;

    // create characteristic value attribute
    attr++;
    attr->pUuid       = GetUuid(&aChar->mUuid);
    attr->pValue      = NULL;
    attr->maxLen      = aChar->mMaxAttrLength;
    attr->pLen        = &attr->maxLen;
    attr->settings    = 0;
    attr->permissions = 0;

    if (aChar->mProperties & OT_BLE_CHAR_PROP_WRITE)
    {
        otEXPECT_ACTION(sGattService.mLengthArrayIndex < kMaxGattLengthArrayNum, error = OT_ERROR_NO_BUFS);

        attr->settings = ATTS_SET_VARIABLE_LEN;
        attr->pLen     = &sGattService.mLengthArrays[sGattService.mLengthArrayIndex++];
    }

    SetAttributeSetting(attr, aChar);

    // output characteristic value handle
    aChar->mHandleValue = sGattHandle;

    if ((aChar->mProperties & OT_BLE_CHAR_PROP_NOTIFY) || (aChar->mProperties & OT_BLE_CHAR_PROP_INDICATE))
    {
        otEXPECT_ACTION(sGattService.mAttributeIndex < kMaxGattAttrNum, error = OT_ERROR_NO_BUFS);
        otEXPECT_ACTION(sCccd.mCccdIndex < kMaxGattCccdNum, error = OT_ERROR_NO_BUFS);

        // create client characteristic configuration descriptor
        sGattHandle++;
        sGattService.mAttributeIndex++;

        attr++;
        attr->pUuid       = attCliChCfgUuid;
        attr->pValue      = (uint8_t *)&sCccd.mValues[sCccd.mCccdIndex];
        attr->maxLen      = sizeof(sCccd.mValues[sCccd.mCccdIndex]);
        attr->pLen        = &attr->maxLen;
        attr->settings    = ATTS_SET_CCC;
        attr->permissions = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE;

        sCccd.mCccds[sCccd.mCccdIndex].handle     = sGattHandle;
        sCccd.mCccds[sCccd.mCccdIndex].valueRange = 0;
        sCccd.mCccds[sCccd.mCccdIndex].secLevel   = DM_SEC_LEVEL_NONE;

        if (aChar->mProperties & OT_BLE_CHAR_PROP_INDICATE)
        {
            sCccd.mCccds[sCccd.mCccdIndex].valueRange |= ATT_CLIENT_CFG_INDICATE;
        }

        if (aChar->mProperties & OT_BLE_CHAR_PROP_NOTIFY)
        {
            sCccd.mCccds[sCccd.mCccdIndex].valueRange |= ATT_CLIENT_CFG_NOTIFY;
        }

        sCccd.mCccdIndex++;

        // output CCCD handle
        aChar->mHandleCccd = sGattHandle;
    }
    else
    {
        // output CCCD handle
        aChar->mHandleCccd = OT_BLE_INVALID_HANDLE;
    }

exit:
    return error;
}

static void GattServerCccdCallback(attsCccEvt_t *aEvent)
{
    bool subscribing;

    if (aEvent->hdr.event == ATTS_CCC_STATE_IND)
    {
        subscribing = (aEvent->value & ATT_CLIENT_CFG_INDICATE) ? true : false;
        otPlatBleGattServerOnSubscribeRequest(bleGetThreadInstance(), aEvent->handle, subscribing);
    }
}

static void resetGattService(void)
{
    sGattService.mCharacteristicIndex = 0;
    sGattService.mAttributeIndex      = 0;
    sGattService.mLengthArrayIndex    = 0;
    sCccd.mCccdIndex                  = 0;
}

otError otPlatBleGattServerServicesRegister(otInstance *aInstance, otPlatBleGattService *aServices)
{
    otError                      error  = OT_ERROR_NONE;
    uint16_t                     handle = sGattHandle;
    otPlatBleGattCharacteristic *characteristic;

    otEXPECT_ACTION(aServices != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(sGattService.mAttributeIndex == 0, error = OT_ERROR_INVALID_STATE);

    otEXPECT((error = AddPrimaryServiceAttribute(&aServices->mUuid, &aServices->mHandle)) == OT_ERROR_NONE);

    sGattService.mService.startHandle = sGattHandle;
    characteristic                     = aServices->mCharacteristics;

    while (characteristic->mUuid.mType != OT_BLE_UUID_TYPE_NONE)
    {
        otEXPECT((error = AddCharacteristicAttribute(characteristic)) == OT_ERROR_NONE);
        characteristic++;
    }

    sGattService.mService.pNext      = NULL;
    sGattService.mService.pAttr      = sGattService.mAttributes;
    sGattService.mService.readCback  = GattServerReadCallback;
    sGattService.mService.writeCback = GattServerWriteCallback;
    sGattService.mService.endHandle  = sGattHandle;

    AttsAddGroup(&sGattService.mService);
    AttsCccRegister(sCccd.mCccdIndex, (attsCccSet_t *)sCccd.mCccds, GattServerCccdCallback);

exit:
    if (error == OT_ERROR_NO_BUFS)
    {
        sGattHandle = handle;
        resetGattService();
    }

    return error;
}

otError otPlatBleGattServerIndicate(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    AttsHandleValueInd(bleGapGetConnectionId(), aHandle, aPacket->mLength, aPacket->mValue);

exit:
    return error;
}

otError otPlatBleGattClientServicesDiscover(otInstance *aInstance)
{
    otError  error = OT_ERROR_NONE;
    uint16_t uuid  = ATT_UUID_PRIMARY_SERVICE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    sServicesDiscovered = false;
    AttcReadByGroupTypeReq(bleGapGetConnectionId(), ATT_HANDLE_START, ATT_HANDLE_MAX, sizeof(uuid), (uint8_t *)&uuid,
                           false);

exit:
    return error;
}

otError otPlatBleGattClientServiceDiscover(otInstance *aInstance, const otPlatBleUuid *aUuid)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION((aUuid != NULL) && (aUuid->mType != OT_BLE_UUID_TYPE_NONE), error = OT_ERROR_INVALID_ARGS);

    sServiceDiscovered   = false;
    sServiceDiscoverUuid = *aUuid;
    AttcFindByTypeValueReq(bleGapGetConnectionId(), ATT_HANDLE_START, ATT_HANDLE_MAX, ATT_UUID_PRIMARY_SERVICE,
                           GetUuidLength(aUuid), GetUuid(aUuid), false);

exit:
    return error;
}

otError otPlatBleGattClientCharacteristicsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    otError  error = OT_ERROR_NONE;
    uint16_t uuid  = ATT_UUID_CHARACTERISTIC;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    sCharacteristicDiscovered = false;
    sCharDiscoverEndHandle    = aEndHandle;
    AttcReadByTypeReq(bleGapGetConnectionId(), aStartHandle, aEndHandle, sizeof(uuid), (uint8_t *)&uuid, false);

exit:
    return error;
}

otError otPlatBleGattClientDescriptorsDiscover(otInstance *aInstance, uint16_t aStartHandle, uint16_t aEndHandle)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(bleGapGetConnectionId() != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    sDescriptorDiscovered  = false;
    sDescDiscoverEndHandle = aEndHandle;
    AttcFindInfoReq(bleGapGetConnectionId(), aStartHandle, aEndHandle, false);

exit:
    return error;
}

/**
 * The BLE GATT module weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatBleGattClientOnReadResponse(otInstance *aInstance, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPacket);
}

OT_TOOL_WEAK void otPlatBleGattClientOnWriteResponse(otInstance *aInstance, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
}

OT_TOOL_WEAK void otPlatBleGattServerOnReadRequest(otInstance *aInstance, uint16_t aHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aHandle);
    OT_UNUSED_VARIABLE(aPacket);
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

OT_TOOL_WEAK void otPlatBleGattClientOnMtuExchangeResponse(otInstance *aInstance, uint16_t aMtu, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aError);
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
#endif // OPENTHREAD_ENABLE_BLE_HOST
