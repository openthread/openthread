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
 *   This file implements the BLE GAP interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "dm_api.h"

#include <stdio.h>
#include <string.h>

#include "ble/ble_gap.h"
#include "ble/ble_hci_driver.h"
#include "ble/ble_mgmt.h"
#include "utils/code_utils.h"

#include <openthread/platform/ble.h>

#if OPENTHREAD_ENABLE_TOBLE

enum
{
    kBleAddrTypeMask                 = 0xc0,
    kBleAddrTypeStatic               = 0xc0,
    kBleAddrTypePrivateResolvable    = 0x40,
    kBleAddrTypePrivateNonResolvable = 0x00,
};

enum
{
    kAdvChannel37  = 0x01,
    kAdvChannel38  = 0x02,
    kAdvChannel39  = 0x04,
    kAdvChannelAll = 0x07,
};

enum
{
    kAdvScanOwnAddrTypePublic                  = 0,
    kAdvScanOwnAddrTypeRandom                  = 1,
    kAdvScanOwnAddrTypeResolvablePrivatePublic = 2,
    kAdvScanOwnAddrTypeResolvablePrivateRandom = 3,
};

enum
{
    kAdvPeerAddrTypePublic = 0,
    kAdvPeerAddrTypeRandom = 1,
};

enum
{
    kAdvScanFilterNone                      = 0,
    kAdvScanFilterScanRequests              = 1,
    kAdvScanFilterConnectionRequests        = 2,
    kAdvScanFilterScanAndConnectionRequests = 3,
};

enum
{
    kAdvConnectableUndirected    = 0,
    kAdvConnectableDirected      = 1,
    kAdvScanableUndirected       = 2,
    kAdvNonConnectableUndirected = 3,
};

enum
{
    kScanFilterDuplicateDisabled = 0,
    kScanFilterDuplicateEnabled  = 1,
};

enum
{
    kConnFilterPolicyNone      = 0,
    kConnFilterPolicyWhiteList = 1,
};

enum
{
    kConnPeerAddrTypePublic         = 0,
    kConnPeerAddrTypeRandom         = 1,
    kConnPeerAddrTypePublicIdentity = 2,
    kConnPeerAddrTypeRandomIdentity = 3,
};

static otPlatBleGapConnParams sBleGapConnParams;
static dmConnId_t             sConnectionId = DM_CONN_ID_NONE;

static inline bool isInRange(uint16_t aValue, uint16_t aMin, uint16_t aMax)
{
    return ((aMin <= aValue) && (aValue <= aMax)) ? true : false;
}

static bool isPrandValid(const uint8_t *aBytes, uint8_t aLength)
{
    bool ret = true;

    // at least one bit of the random part of the static address shall be  equal to 0 and at least one bit of
    // the random part of the static address shall be equal to 1
    for (uint8_t i = 0; i < (aLength - 1); ++i)
    {
        otEXPECT((aBytes[i] == 0x00) || (aBytes[i] == 0xFF));
        otEXPECT((i == 0) || (aBytes[i] == aBytes[i - 1]));
    }

    otEXPECT_ACTION(!(((aBytes[aLength - 1] & 0x3F) == 0x3F) && (aBytes[aLength - 2] == 0xFF)), ret = false);
    otEXPECT_ACTION(!(((aBytes[aLength - 1] & 0x3F) == 0x00) && (aBytes[aLength - 2] == 0x00)), ret = false);

exit:
    return ret;
}

static bool isRandomPrivateResolvableAddress(const otPlatBleDeviceAddr *aAddress)
{
    bool ret;

    otEXPECT_ACTION((aAddress->mAddr[5] & kBleAddrTypeMask) == kBleAddrTypePrivateResolvable, ret = false);
    ret = isPrandValid(aAddress->mAddr + 3, 3);

exit:
    return ret;
}

enum
{
    kHciErrorNone = 0,
};

static void bleGapConnectedHandler(const wsfMsgHdr_t *aMsg)
{
    const hciLeConnCmplEvt_t *connEvent = (const hciLeConnCmplEvt_t *)aMsg;

    otEXPECT(connEvent->status == kHciErrorNone);

    sConnectionId = (dmConnId_t)connEvent->hdr.param;
    otPlatBleGapOnConnected(bleMgmtGetThreadInstance(), sConnectionId);

exit:
    return;
}

static void bleGapDisconnectedHandler(const wsfMsgHdr_t *aMsg)
{
    const hciDisconnectCmplEvt_t *disconnectEvent = (const hciDisconnectCmplEvt_t *)aMsg;

    sConnectionId = DM_CONN_ID_NONE;
    otPlatBleGapOnDisconnected(bleMgmtGetThreadInstance(), disconnectEvent->hdr.param);
}

enum
{
    kAdvReportEnentTypeAdvInd        = 0x00,
    kAdvReportEnentTypeAdvDirectInd  = 0x01,
    kAdvReportEnentTypeAdvScanInd    = 0x02,
    kAdvReportEnentTypeAdvNonConnInd = 0x03,
    kAdvReportEnentTypeScanResponse  = 0x04,
};

static void bleGapScanReportHandler(const wsfMsgHdr_t *aMsg)
{
    const hciLeAdvReportEvt_t *advReportEvent = (const hciLeAdvReportEvt_t *)aMsg;
    otBleRadioPacket           packet;
    otPlatBleDeviceAddr        devAddr;

    devAddr.mAddrType = advReportEvent->addrType;
    memcpy(devAddr.mAddr, advReportEvent->addr, OT_BLE_ADDRESS_LENGTH);

    // privacy is disabled by default, filter the private resolvable address
    otEXPECT(!((devAddr.mAddrType != kAdvScanOwnAddrTypeRandom) && isRandomPrivateResolvableAddress(&devAddr)));

    packet.mValue  = advReportEvent->pData;
    packet.mLength = advReportEvent->len;
    packet.mPower  = advReportEvent->rssi;

    switch (advReportEvent->eventType)
    {
    case kAdvReportEnentTypeAdvInd:
    case kAdvReportEnentTypeAdvDirectInd:
    case kAdvReportEnentTypeAdvScanInd:
    case kAdvReportEnentTypeAdvNonConnInd:
        otPlatBleGapOnAdvReceived(bleMgmtGetThreadInstance(), &devAddr, &packet);
        break;

    case kAdvReportEnentTypeScanResponse:
        otPlatBleGapOnScanRespReceived(bleMgmtGetThreadInstance(), &devAddr, &packet);
        break;
    }

exit:
    return;
}

void bleGapEventHandler(const wsfMsgHdr_t *aMsg)
{
    otEXPECT(aMsg != NULL);

    switch (aMsg->event)
    {
    case DM_CONN_OPEN_IND:
        bleGapConnectedHandler(aMsg);
        break;

    case DM_CONN_CLOSE_IND:
        bleGapDisconnectedHandler(aMsg);
        break;

    case DM_SCAN_REPORT_IND:
        bleGapScanReportHandler(aMsg);
        break;
    }

exit:
    return;
}

void bleGapReset(void)
{
    sConnectionId = DM_CONN_ID_NONE;
}

dmConnId_t bleGapGetConnectionId(void)
{
    return sConnectionId;
}

/*******************************************************************************
 * @section Bluetooth Low Energy management.
 ******************************************************************************/

otError otPlatBleGapAddressSet(otInstance *aInstance, const otPlatBleDeviceAddr *aAddress)
{
    // the public address cannot be set

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAddress);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleGapAddressGet(otInstance *aInstance, otPlatBleDeviceAddr *aAddress)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aInstance == bleMgmtGetThreadInstance(), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(aAddress != NULL, error = OT_ERROR_INVALID_ARGS);

    aAddress->mAddrType = OT_BLE_ADDRESS_TYPE_PUBLIC;
    memcpy(aAddress->mAddr, HciGetBdAddr(), OT_BLE_ADDRESS_LENGTH);

exit:
    return error;
}

/****************************************************************************
 * @section Bluetooth Low Energy GAP.
 ***************************************************************************/
otError otPlatBleGapAdvDataSet(otInstance *aInstance, const uint8_t *aAdvData, uint8_t aAdvDataLength)
{
    otError       error           = OT_ERROR_NONE;
    const uint8_t kMaxAdvDataSize = 31;
    uint8_t       buf[kMaxAdvDataSize];

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION((aAdvData != NULL) && (aAdvDataLength != 0), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(aAdvDataLength <= sizeof(buf), error = OT_ERROR_INVALID_ARGS);

    // avoid converting "const uint8_t*" to "uint8_t *"
    memcpy(buf, aAdvData, aAdvDataLength);
    DmAdvSetData(DM_ADV_HANDLE_DEFAULT, HCI_ADV_DATA_OP_COMP_FRAG, DM_DATA_LOC_ADV, aAdvDataLength, buf);

exit:
    return error;
}

otError otPlatBleGapAdvStart(otInstance *aInstance, uint16_t aInterval, uint8_t aType)
{
    otError             error             = OT_ERROR_NONE;
    uint8_t             advHandles[]      = {DM_ADV_HANDLE_DEFAULT};
    uint16_t            advDurations[]    = {0 /* infinite */};
    uint8_t             maxExtAdvEvents[] = {0};
    uint8_t             advType           = 0;
    otPlatBleDeviceAddr peerAddr;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(isInRange(aInterval, OT_BLE_ADV_INTERVAL_MIN, OT_BLE_ADV_INTERVAL_MAX),
                    error = OT_ERROR_INVALID_ARGS);

    if ((aType & OT_BLE_ADV_MODE_CONNECTABLE) && (aType & OT_BLE_ADV_MODE_SCANNABLE))
    {
        advType = kAdvConnectableUndirected;
    }
    else if ((!(aType & OT_BLE_ADV_MODE_CONNECTABLE)) && (!(aType & OT_BLE_ADV_MODE_SCANNABLE)))
    {
        advType = kAdvNonConnectableUndirected;
    }
    else if (aType & OT_BLE_ADV_MODE_SCANNABLE)
    {
        advType = kAdvScanableUndirected;
    }
    else
    {
        otEXPECT(0); // exit
    }

    memset(&peerAddr, 0, sizeof(peerAddr));

    DmAdvSetAddrType(kAdvScanOwnAddrTypePublic);
    DmAdvSetChannelMap(DM_ADV_HANDLE_DEFAULT, kAdvChannelAll);
    DmDevSetFilterPolicy(DM_FILT_POLICY_MODE_ADV, kAdvScanFilterNone);
    DmAdvSetInterval(DM_ADV_HANDLE_DEFAULT, aInterval, aInterval);
    DmAdvConfig(DM_ADV_HANDLE_DEFAULT, advType, kAdvPeerAddrTypePublic, peerAddr.mAddr);

    DmAdvStart(1, advHandles, advDurations, maxExtAdvEvents);

exit:
    return error;
}

otError otPlatBleGapAdvStop(otInstance *aInstance)
{
    otError error        = OT_ERROR_NONE;
    uint8_t advHandles[] = {DM_ADV_HANDLE_DEFAULT};

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    DmAdvStop(1, advHandles);

exit:
    return error;
}

otError otPlatBleGapScanResponseSet(otInstance *aInstance, const uint8_t *aScanResponse, uint8_t aScanResponseLength)
{
    otError       error               = OT_ERROR_NONE;
    const uint8_t kMaxScanRspDataSize = 31;
    uint8_t       buf[kMaxScanRspDataSize];

    otEXPECT_ACTION(aInstance == bleMgmtGetThreadInstance(), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION((aScanResponse != NULL) && (aScanResponseLength != 0), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(aScanResponseLength <= sizeof(buf), error = OT_ERROR_INVALID_ARGS);

    // avoid converting "const uint8_t*" to "uint8_t *"
    memcpy(buf, aScanResponse, aScanResponseLength);
    DmAdvSetData(DM_ADV_HANDLE_DEFAULT, HCI_ADV_DATA_OP_COMP_FRAG, DM_DATA_LOC_SCAN, aScanResponseLength, buf);

exit:
    return error;
}

otError otPlatBleGapScanStart(otInstance *aInstance, uint16_t aInterval, uint16_t aWindow)
{
    otError error    = OT_ERROR_INVALID_ARGS;
    uint8_t scanType = DM_SCAN_TYPE_ACTIVE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT(aWindow <= aInterval);
    otEXPECT(isInRange(aInterval, OT_BLE_SCAN_INTERVAL_MIN, OT_BLE_SCAN_INTERVAL_MAX));
    otEXPECT(isInRange(aWindow, OT_BLE_SCAN_WINDOW_MIN, OT_BLE_SCAN_WINDOW_MAX));

    DmScanSetInterval(HCI_INIT_PHY_LE_1M_BIT, &aInterval, &aWindow);
    DmScanSetAddrType(kAdvScanOwnAddrTypePublic);
    DmDevSetFilterPolicy(DM_FILT_POLICY_MODE_SCAN, kAdvScanFilterNone);

    DmScanStart(HCI_SCAN_PHY_LE_1M_BIT, DM_DISC_MODE_NONE, &scanType, kScanFilterDuplicateEnabled, 0, 0);
    error = OT_ERROR_NONE;

exit:
    return error;
}

otError otPlatBleGapScanStop(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    DmScanStop();

exit:
    return error;
}

otError otPlatBleGapConnParamsSet(otInstance *aInstance, const otPlatBleGapConnParams *aConnParams)
{
    otError error = OT_ERROR_INVALID_ARGS;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT(aConnParams != NULL);
    otEXPECT(aConnParams->mConnSlaveLatency <= OT_BLE_CONN_SLAVE_LATENCY_MAX);
    otEXPECT(aConnParams->mConnMinInterval <= aConnParams->mConnMaxInterval);
    otEXPECT(isInRange(aConnParams->mConnMinInterval, OT_BLE_CONN_INTERVAL_MIN, OT_BLE_CONN_INTERVAL_MAX));
    otEXPECT(isInRange(aConnParams->mConnMaxInterval, OT_BLE_CONN_INTERVAL_MIN, OT_BLE_CONN_INTERVAL_MAX));
    otEXPECT(isInRange(aConnParams->mConnSupervisionTimeout, OT_BLE_CONN_SUPERVISOR_TIMEOUT_MIN,
                       OT_BLE_CONN_SUPERVISOR_TIMEOUT_MAX));

    sBleGapConnParams = *aConnParams;
    error             = OT_ERROR_NONE;

exit:
    return error;
}

otError otPlatBleGapConnect(otInstance *aInstance, otPlatBleDeviceAddr *aAddress, uint16_t aInterval, uint16_t aWindow)
{
    otError       error = OT_ERROR_INVALID_ARGS;
    hciConnSpec_t connSpec;
    dmConnId_t    connId;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(sConnectionId == DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);
    otEXPECT(aAddress != NULL);
    otEXPECT(aWindow <= aInterval);
    otEXPECT(isInRange(aInterval, OT_BLE_SCAN_INTERVAL_MIN, OT_BLE_SCAN_INTERVAL_MAX));
    otEXPECT(isInRange(aWindow, OT_BLE_SCAN_WINDOW_MIN, OT_BLE_SCAN_WINDOW_MAX));

    // Force scan stop before initiating the scan used for connection
    otPlatBleGapScanStop(aInstance);

    DmConnSetScanInterval(aInterval, aWindow);
    DmDevSetFilterPolicy(DM_FILT_POLICY_MODE_INIT, kConnFilterPolicyNone);
    DmConnSetAddrType(kConnPeerAddrTypePublic);

    connSpec.connIntervalMin = sBleGapConnParams.mConnMinInterval;
    connSpec.connIntervalMax = sBleGapConnParams.mConnMaxInterval;
    connSpec.connLatency     = sBleGapConnParams.mConnSlaveLatency;
    connSpec.supTimeout      = sBleGapConnParams.mConnSupervisionTimeout;
    connSpec.minCeLen        = 0;
    connSpec.maxCeLen        = 0;

    DmConnSetConnSpec(&connSpec);

    connId = DmConnOpen(DM_CLIENT_ID_APP, HCI_INIT_PHY_LE_1M_BIT, aAddress->mAddrType, aAddress->mAddr);
    error  = (connId == DM_CONN_ID_NONE) ? OT_ERROR_FAILED : OT_ERROR_NONE;

exit:
    return error;
}

otError otPlatBleGapDisconnect(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(sConnectionId != DM_CONN_ID_NONE, error = OT_ERROR_INVALID_STATE);

    DmConnClose(DM_CLIENT_ID_APP, sConnectionId, 0);
    sConnectionId = DM_CONN_ID_NONE;

exit:
    return error;
}

/**
 * The BLE GAP module weak functions definition.
 *
 */
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
#endif // OPENTHREAD_ENABLE_TOBLE
