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
 *   This file implements Cordio BLE stack initialization.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "wsf_types.h"

#include "att_api.h"
#include "att_handler.h"
#include "dm_handler.h"
#include "hci_handler.h"
#include "l2c_api.h"
#include "l2c_handler.h"
#include "smp_api.h"
#include "smp_handler.h"
#include "wsf_buf.h"
#include "wsf_mbed_os_adaptation.h"

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
#include "lctr_int_conn.h"
#include "ll_defs.h"
#include "cordio/ble_config.h"
#include "cordio/ble_controller_init.h"
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER

#include "cordio/ble_gap.h"
#include "cordio/ble_gatt.h"
#include "cordio/ble_hci_driver.h"
#include "cordio/ble_init.h"
#include "cordio/ble_l2cap.h"
#include "cordio/ble_wsf.h"
#include "utils/code_utils.h"

#include <assert.h>
#include <openthread/error.h>
#include <openthread/platform/ble.h>
#if OPENTHREAD_ENABLE_BLE_CONTROLLER
#include <openthread/platform/radio-ble.h>
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER

#if OPENTHREAD_ENABLE_BLE_HOST
#if OPENTHREAD_ENABLE_BLE_CONTROLLER

#define WSF_MSG_HEADROOM_LENGTH 20
#define ACL_NUM_BUFFERS (BLE_STACK_NUM_ACL_TRANSMIT_BUFFERS + BLE_STACK_NUM_ACL_RECEIVE_BUFFERS)
#define ACL_BUFFER_SIZE                                                                         \
    (WSF_MSG_HEADROOM_LENGTH + LCTR_DATA_PDU_START_OFFSET + HCI_ACL_HDR_LEN + LL_DATA_HDR_LEN + \
     BLE_STACK_MAX_ACL_DATA_LENGTH + LL_DATA_MIC_LEN)

#if OPENTHREAD_ENABLE_BLE_L2CAP
enum
{
    kStackBufferSize = 9600,
};

static wsfBufPoolDesc_t sPoolDesc[] = {
    {16, 16 + 8}, {32, 16 + 4}, {64, 8}, {128, 4 + BLE_STACK_MAX_ADV_REPORTS}, {ACL_BUFFER_SIZE, ACL_NUM_BUFFERS},
    {272, 1},     {1300, 2}};
#else  // OPENTHREAD_ENABLE_BLE_L2CAP
enum
{
    kStackBufferSize = 6944,
};

static wsfBufPoolDesc_t sPoolDesc[] = {
    {16, 16 + 8}, {32, 16 + 4}, {64, 8}, {128, 4 + BLE_STACK_MAX_ADV_REPORTS}, {ACL_BUFFER_SIZE, ACL_NUM_BUFFERS},
    {272, 1}};
#endif // !OPENTHREAD_ENABLE_BLE_L2CAP
#else  // OPENTHREAD_ENABLE_BLE_CONTROLLER
#if OPENTHREAD_ENABLE_BLE_L2CAP
enum
{
    kStackBufferSize = 4832,
};

static wsfBufPoolDesc_t sPoolDesc[] = {{16, 16}, {32, 16}, {64, 8}, {128, 4}, {272, 1}, {1300, 2}};
#else  // OPENTHREAD_ENABLE_BLE_L2CAP
enum
{
    kStackBufferSize = 2250,
};

static wsfBufPoolDesc_t sPoolDesc[] = {{16, 16}, {32, 16}, {64, 8}, {128, 4}, {272, 1}};
#endif // !OPENTHREAD_ENABLE_BLE_L2CAP
#endif // !OPENTHREAD_ENABLE_BLE_CONTROLLER

/* WSF heap allocation */
uint8_t *SystemHeapStart;
uint32_t SystemHeapSize;

enum
{
    kBleResetTimeout = 100,
};

__attribute__((aligned(4))) static uint8_t sStackBuffer[kStackBufferSize];

static uint8_t     sState            = kStateDisabled;
static otInstance *sInstance         = NULL;
static bool        sStackInitialized = false;
static wsfTimer_t  sTimer;

static void bleHostInit(void);

otError otPlatBleEnable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((sInstance == NULL) && (aInstance != NULL), error = OT_ERROR_FAILED);
    otEXPECT_ACTION(sState == kStateDisabled, error = OT_ERROR_FAILED);

    sInstance = aInstance;
    sState    = kStateInitializing;

    bleHciEnable();
    bleWsfInit();
#if OPENTHREAD_ENABLE_BLE_CONTROLLER
    bleControllerInit();
#endif
    bleHostInit();
    bleHciInit();
    DmDevReset();

    WsfTimerStartMs(&sTimer, kBleResetTimeout);

exit:
    return error;
}

otError otPlatBleDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((sInstance != NULL) && (aInstance = sInstance), error = OT_ERROR_FAILED);
    otEXPECT_ACTION(sState == kStateInitialized, error = OT_ERROR_FAILED);

    sInstance = NULL;
    sState    = kStateDeinitializing;

    DmDevReset();
    WsfTimerStartMs(&sTimer, kBleResetTimeout);

exit:
    return error;
}

bool otPlatBleIsEnabled(otInstance *aInstance)
{
    bool ret = false;

    otEXPECT_ACTION((sInstance != NULL) && (aInstance = sInstance), ret = false);
    ret = (sState == kStateInitialized) ? true : false;

exit:
    return ret;
}

uint8_t bleGetState(void)
{
    return sState;
}

otInstance *bleGetThreadInstance(void)
{
    return sInstance;
}

static void bleStackHandler(wsfEventMask_t aEvent, wsfMsgHdr_t *aMsg)
{
    otEXPECT(aMsg != NULL);

    switch (aMsg->event)
    {
    case DM_RESET_CMPL_IND:
    {
        if (sState == kStateInitializing)
        {
            sState = kStateInitialized;

            WsfTimerStop(&sTimer);
            otPlatBleOnEnabled(sInstance);
        }
        else if (sState == kStateDeinitializing)
        {
            sState = kStateDisabled;

            WsfTimerStop(&sTimer);
            bleGattReset();
#if OPENTHREAD_ENABLE_BLE_L2CAP
            bleL2capReset();
#endif
            bleGapReset();
            bleHciDisable();
        }
        break;
    }

    default:
        bleGapEventHandler(aMsg);
        break;
    }

    OT_UNUSED_VARIABLE(aEvent);

exit:
    return;
}

static void bleDeviceManagerHandler(dmEvt_t *aDmEvent)
{
    bleStackHandler(0, &aDmEvent->hdr);
}

static void bleConnectionHandler(dmEvt_t *aDmEvent)
{
    dmConnId_t connId = (dmConnId_t)aDmEvent->hdr.param;

    switch (aDmEvent->hdr.event)
    {
    case DM_CONN_OPEN_IND:
        // set up CCC table with uninitialized (all zero) values
        AttsCccInitTable(connId, NULL);
        break;

    case DM_CONN_CLOSE_IND:
        // clear CCC table on connection close
        AttsCccClearTable(connId);
        break;

    default:
        break;
    }
}

static void bleTimerHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (sState == kStateDeinitializing)
    {
        bleGattReset();
#if OPENTHREAD_ENABLE_BLE_L2CAP
        bleL2capReset();
#endif
        bleGapReset();
        bleHciDisable();
    }

    sState    = kStateDisabled;
    sInstance = NULL;

    OT_UNUSED_VARIABLE(event);
    OT_UNUSED_VARIABLE(pMsg);
}

static uint8_t bleGattServerAttsAuthHandler(dmConnId_t aConnId, uint8_t aPermit, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aConnId);
    OT_UNUSED_VARIABLE(aPermit);
    OT_UNUSED_VARIABLE(aHandle);

    return 0;
}

static void bleHostInit(void)
{
    uint32_t       bytesUsed;
    wsfHandlerId_t handlerId;

    otEXPECT(!sStackInitialized);
    sStackInitialized = true;

    SystemHeapStart = sStackBuffer;
    SystemHeapSize  = sizeof(sStackBuffer);

    bytesUsed = WsfBufInit(sizeof(sPoolDesc) / sizeof(wsfBufPoolDesc_t), sPoolDesc);
    assert(bytesUsed != 0);
    SystemHeapStart += bytesUsed;
    SystemHeapSize -= bytesUsed;

    WsfTimerInit();
    SecInit();

    SecRandInit();
    SecAesInit();
    SecCmacInit();
    SecEccInit();

    handlerId = WsfOsSetNextHandler(HciHandler);
    HciHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(DmHandler);
    DmAdvInit();
    DmScanInit();
    DmConnInit();
    DmConnMasterInit();
    DmConnSlaveInit();
    DmSecInit();
    DmPhyInit();

    DmSecLescInit();
    DmPrivInit();
    DmHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
    L2cSlaveHandlerInit(handlerId);
    L2cInit();
    L2cSlaveInit();
    L2cMasterInit();

    handlerId = WsfOsSetNextHandler(L2cCocHandler);
    L2cCocInit();
    L2cCocHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(handlerId);
    AttsInit();
    AttsIndInit();
    AttsSignInit();
    AttsAuthorRegister(bleGattServerAttsAuthHandler);
    AttcInit();
    AttcSignInit();

    handlerId = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(handlerId);
    SmprInit();
    SmprScInit();
    SmpiInit();
    SmpiScInit();

    WsfOsSetNextHandler(bleStackHandler);

    DmRegister(bleDeviceManagerHandler);
    DmConnRegister(DM_CLIENT_ID_APP, bleDeviceManagerHandler);
    AttConnRegister(bleConnectionHandler);
    AttRegister(bleAttHandler);

    sTimer.handlerId = WsfOsSetNextHandler(bleTimerHandler);

exit:
    return;
}

/**
 * The BLE Management module weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatBleOnEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

#endif // OPENTHREAD_ENABLE_BLE_HOST
