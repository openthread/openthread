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
 *   This file implements the BLE L2CAP interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "dm_api.h"
#include "hci_api.h"
#include "l2c_api.h"

#include <stdio.h>
#include <string.h>

#include "ble/ble_l2cap.h"
#include "ble/ble_mgmt.h"
#include "utils/code_utils.h"

#include <openthread/platform/ble.h>

#if OPENTHREAD_ENABLE_TOBLE || OPENTHREAD_ENABLE_CLI_BLE
#if OPENTHREAD_ENABLE_L2CAP
enum
{
    kL2capMaxNumConnections       = 1, //< Maximum number of L2CAP connections.
    kL2capInvalidConnectionHandle = 0, //< Invalid L2CAP connection handle.
    kL2capMaxCredits              = 1, //< Maximum credits for L2CAP flow control.
};

typedef struct L2capConnnection
{
    l2cCocRegId_t      mRegisterId;    //< L2CAP register identifier returned by the Cordio stack.
    uint16_t           mGapConnId;     //< BLE GAP connection ID.
    uint16_t           mPsm;           //< L2CAP protocol/service multiplexer
    uint16_t           mLocalCid;      //< L2CAP local channel ID.
    otPlatBleL2capRole mRole : 2;      //< L2CAP role.
    bool               mConnected : 1; //< Indicates if the L2CAP connection has been established.
    bool               mIsValid : 1;   //< Indicates if the entry is valid.
} L2capConnnection;

static L2capConnnection sL2capConnections[kL2capMaxNumConnections];

static uint8_t l2capGetFreeConnection(void)
{
    uint8_t i;

    for (i = 0; i < kL2capMaxNumConnections; i++)
    {
        if (!sL2capConnections[i].mIsValid)
        {
            memset(&sL2capConnections[i], 0, sizeof(L2capConnnection));
            sL2capConnections[i].mIsValid = true;
            break;
        }
    }

    return (i >= kL2capMaxNumConnections) ? kL2capInvalidConnectionHandle : i + 1;
}

static void l2capFreeConnection(uint8_t aL2capHandle)
{
    if ((0 < aL2capHandle) && (aL2capHandle < kL2capMaxNumConnections + 1) &&
        sL2capConnections[aL2capHandle - 1].mIsValid)
    {
        sL2capConnections[aL2capHandle - 1].mIsValid = false;
    }
}

static L2capConnnection *l2capGetConnection(uint8_t aL2capHandle)
{
    L2capConnnection *conn = NULL;

    if ((0 < aL2capHandle) && (aL2capHandle < kL2capMaxNumConnections + 1) &&
        sL2capConnections[aL2capHandle - 1].mIsValid)
    {
        conn = &sL2capConnections[aL2capHandle - 1];
    }

    return conn;
}

static uint8_t l2capFindHandleByPsm(uint16_t aConnectionId, uint16_t aPsm, otPlatBleL2capRole aRole)
{
    L2capConnnection *conn = &sL2capConnections[0];
    uint8_t           i;

    for (i = 0; i < kL2capMaxNumConnections; i++)
    {
        if (conn->mIsValid && (conn->mPsm == aPsm) && (conn->mGapConnId == aConnectionId) && (conn->mRole == aRole))
        {
            break;
        }

        conn++;
    }

    return (i >= kL2capMaxNumConnections) ? kL2capInvalidConnectionHandle : i + 1;
}

static uint8_t l2capFindHandleByCid(uint16_t aConnectionId, uint16_t aLocalCid)
{
    L2capConnnection *conn = &sL2capConnections[0];
    uint8_t           i;

    for (i = 0; i < kL2capMaxNumConnections; i++)
    {
        if (conn->mIsValid && (conn->mLocalCid == aLocalCid) && (conn->mGapConnId == aConnectionId))
        {
            break;
        }

        conn++;
    }

    return (i >= kL2capMaxNumConnections) ? kL2capInvalidConnectionHandle : i + 1;
}

static void l2capCallback(l2cCocEvt_t *pMsg, bool aIsInitiator)
{
    uint8_t  l2capHandle;
    uint16_t gapConnId = pMsg->hdr.param;

    switch (pMsg->hdr.event)
    {
    case L2C_COC_CONNECT_IND:
    {
        l2cCocConnectInd_t *connInd = (l2cCocConnectInd_t *)pMsg;
        L2capConnnection *  conn;
        uint8_t             role;

        role        = aIsInitiator ? OT_BLE_L2CAP_ROLE_INITIATOR : OT_BLE_L2CAP_ROLE_ACCEPTOR;
        l2capHandle = l2capFindHandleByPsm(gapConnId, connInd->psm, role);
        otEXPECT(l2capHandle != kL2capInvalidConnectionHandle);

        otEXPECT((conn = l2capGetConnection(l2capHandle)) != NULL);
        conn->mConnected = true;

        if (aIsInitiator)
        {
            otEXPECT(conn->mLocalCid == connInd->cid);
            otPlatBleL2capOnConnectionResponse(bleMgmtGetThreadInstance(), l2capHandle, connInd->peerMtu);
        }
        else
        {
            conn->mLocalCid = connInd->cid;
            otPlatBleL2capOnConnectionRequest(bleMgmtGetThreadInstance(), l2capHandle, connInd->peerMtu);
        }

        break;
    }
    case L2C_COC_DATA_CNF:
    {
        l2cCocDataCnf_t *dataCnf = (l2cCocDataCnf_t *)pMsg;
        otError          error;

        l2capHandle = l2capFindHandleByCid(gapConnId, dataCnf->cid);
        otEXPECT(l2capHandle != kL2capInvalidConnectionHandle);

        error = dataCnf->hdr.status == L2C_COC_DATA_SUCCESS ? OT_ERROR_NONE : OT_ERROR_FAILED;
        otPlatBleL2capOnSduSent(bleMgmtGetThreadInstance(), l2capHandle, error);

        break;
    }
    case L2C_COC_DATA_IND:
    {
        l2cCocDataInd_t *dataInd = (l2cCocDataInd_t *)pMsg;
        otBleRadioPacket packet;

        packet.mValue  = dataInd->pData;
        packet.mLength = dataInd->dataLen;

        l2capHandle = l2capFindHandleByCid(gapConnId, dataInd->cid);
        otEXPECT(l2capHandle != kL2capInvalidConnectionHandle);

        otPlatBleL2capOnSduReceived(bleMgmtGetThreadInstance(), l2capHandle, &packet);
        break;
    }
    case L2C_COC_DISCONNECT_IND:
    {
        l2cCocDisconnectInd_t *disconnectInd = (l2cCocDisconnectInd_t *)pMsg;
        L2capConnnection *     conn;

        l2capHandle = l2capFindHandleByCid(gapConnId, disconnectInd->cid);
        otEXPECT(l2capHandle != kL2capInvalidConnectionHandle);

        otEXPECT((conn = l2capGetConnection(l2capHandle)) != NULL);
        conn->mConnected = false;

        otPlatBleL2capOnDisconnect(bleMgmtGetThreadInstance(), l2capHandle);
        break;
    }
    }

exit:
    return;
}

static void l2capInitiatorCallback(l2cCocEvt_t *pMsg)
{
    l2capCallback(pMsg, true);
}

static void l2capAcceptorCallback(l2cCocEvt_t *pMsg)
{
    l2capCallback(pMsg, false);
}

void bleL2capReset(void)
{
    memset(sL2capConnections, 0, sizeof(sL2capConnections));
}

otError otPlatBleL2capConnectionRegister(otInstance *       aInstance,
                                         uint16_t           aConnectionId,
                                         uint16_t           aPsm,
                                         uint16_t           aMtu,
                                         otPlatBleL2capRole aRole,
                                         uint8_t *          aL2capHandle)
{
    otError           error = OT_ERROR_NONE;
    l2cCocReg_t       cocReg;
    l2cCocRegId_t     regId;
    L2capConnnection *conn;
    uint8_t           l2capHandle;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(l2capFindHandleByPsm(aConnectionId, aPsm, aRole) == kL2capInvalidConnectionHandle,
                    error = OT_ERROR_DUPLICATED);

    l2capHandle = l2capGetFreeConnection();
    otEXPECT_ACTION(l2capHandle != kL2capInvalidConnectionHandle, error = OT_ERROR_NO_BUFS);

    conn = l2capGetConnection(l2capHandle);
    otEXPECT_ACTION(conn != NULL, error = OT_ERROR_FAILED);

    cocReg.psm      = aPsm;
    cocReg.mtu      = aMtu;
    cocReg.mps      = HciGetMaxRxAclLen() - L2C_HDR_LEN;
    cocReg.credits  = kL2capMaxCredits;
    cocReg.secLevel = DM_SEC_LEVEL_NONE;
    cocReg.authoriz = false;

    if (aRole == OT_BLE_L2CAP_ROLE_INITIATOR)
    {
        cocReg.role = L2C_COC_ROLE_INITIATOR;
        regId       = L2cCocRegister(l2capInitiatorCallback, &cocReg);
    }
    else
    {
        cocReg.role = L2C_COC_ROLE_ACCEPTOR;
        regId       = L2cCocRegister(l2capAcceptorCallback, &cocReg);
    }

    otEXPECT_ACTION(regId != L2C_COC_REG_ID_NONE, error = OT_ERROR_FAILED);

    conn->mGapConnId  = aConnectionId;
    conn->mPsm        = aPsm;
    conn->mRole       = aRole;
    conn->mRegisterId = regId;
    *aL2capHandle     = l2capHandle;

exit:
    if (error == OT_ERROR_FAILED)
    {
        l2capFreeConnection(l2capHandle);
    }

    return error;
}

otError otPlatBleL2capConnectionDeregister(otInstance *aInstance, uint8_t aL2capHandle)
{
    otError           error = OT_ERROR_NONE;
    L2capConnnection *conn;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);

    conn = l2capGetConnection(aL2capHandle);
    otEXPECT_ACTION((conn != NULL) || (!conn->mConnected), error = OT_ERROR_FAILED);

    L2cCocDeregister(conn->mRegisterId);
    l2capFreeConnection(aL2capHandle);

exit:
    return error;
}

otError otPlatBleL2capConnectionRequest(otInstance *aInstance, uint8_t aL2capHandle)
{
    otError           error = OT_ERROR_NONE;
    L2capConnnection *conn;
    uint16_t          cid;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION((conn = l2capGetConnection(aL2capHandle)) != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION((conn->mRole == OT_BLE_L2CAP_ROLE_INITIATOR) && !conn->mConnected, error = OT_ERROR_FAILED);

    cid = L2cCocConnectReq((dmConnId_t)conn->mGapConnId, conn->mRegisterId, conn->mPsm);
    otEXPECT_ACTION(cid != L2C_COC_CID_NONE, error = OT_ERROR_FAILED);

    conn->mLocalCid = cid;

exit:
    return error;
}

otError otPlatBleL2capSduSend(otInstance *aInstance, uint8_t aL2capHandle, otBleRadioPacket *aPacket)
{
    otError           error = OT_ERROR_NONE;
    L2capConnnection *conn;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);

    conn = l2capGetConnection(aL2capHandle);
    otEXPECT_ACTION((conn != NULL) && conn->mConnected, error = OT_ERROR_FAILED);

    L2cCocDataReq(conn->mLocalCid, aPacket->mLength, aPacket->mValue);

exit:
    return error;
}

otError otPlatBleL2capDisconnect(otInstance *aInstance, uint8_t aL2capHandle)
{
    otError           error = OT_ERROR_NONE;
    L2capConnnection *conn;

    otEXPECT_ACTION(otPlatBleIsEnabled(aInstance), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION((conn = l2capGetConnection(aL2capHandle)) != NULL, error = OT_ERROR_FAILED);
    otEXPECT(conn->mConnected);

    L2cCocDisconnectReq(conn->mLocalCid);

exit:
    return error;
}
#else
otError otPlatBleL2capConnectionRegister(otInstance *       aInstance,
                                         uint16_t           aConnectionId,
                                         uint16_t           aPsm,
                                         uint16_t           aMtu,
                                         otPlatBleL2capRole aRole,
                                         uint8_t *          aL2capHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aConnectionId);
    OT_UNUSED_VARIABLE(aPsm);
    OT_UNUSED_VARIABLE(aMtu);
    OT_UNUSED_VARIABLE(aRole);
    OT_UNUSED_VARIABLE(aL2capHandle);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capConnectionDeregister(otInstance *aInstance, uint8_t aL2capHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capConnectionRequest(otInstance *aInstance, uint8_t aL2capHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capSduSend(otInstance *aInstance, uint8_t aL2capHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
    OT_UNUSED_VARIABLE(aPacket);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatBleL2capDisconnect(otInstance *aInstance, uint8_t aL2capHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);

    return OT_ERROR_NOT_IMPLEMENTED;
}

#endif // OPENTHREAD_ENABLE_L2CAP

/**
 * The BLE L2CAP module weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatBleL2capOnConnectionRequest(otInstance *aInstance, uint8_t aL2capHandle, uint16_t aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
    OT_UNUSED_VARIABLE(aMtu);
}

OT_TOOL_WEAK void otPlatBleL2capOnConnectionResponse(otInstance *aInstance, uint8_t aL2capHandle, uint16_t aMtu)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
    OT_UNUSED_VARIABLE(aMtu);
}

OT_TOOL_WEAK void otPlatBleL2capOnSduReceived(otInstance *aInstance, uint8_t aL2capHandle, otBleRadioPacket *aPacket)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
    OT_UNUSED_VARIABLE(aPacket);
}

OT_TOOL_WEAK void otPlatBleL2capOnSduSent(otInstance *aInstance, uint8_t aL2capHandle, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
    OT_UNUSED_VARIABLE(aError);
}

OT_TOOL_WEAK void otPlatBleL2capOnDisconnect(otInstance *aInstance, uint8_t aL2capHandle)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aL2capHandle);
}
#endif // OPENTHREAD_ENABLE_TOBLE || OPENTHREAD_ENABLE_CLI_BLE
