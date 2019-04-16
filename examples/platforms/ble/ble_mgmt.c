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
 *   This file implements the BLE management interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "wsf_types.h"

#include "att_api.h"
#include "att_handler.h"
#include "dm_handler.h"
#include "hci_drv.h"
#include "hci_handler.h"
#include "l2c_api.h"
#include "l2c_handler.h"
#include "smp_api.h"
#include "smp_handler.h"
#include "wsf_buf.h"
#include "wsf_mbed_os_adaptation.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_timer.h"

#include "ble/ble_gap.h"
#include "ble/ble_gatt.h"
#include "ble/ble_hci_driver.h"
#include "ble/ble_l2cap.h"
#include "ble/ble_mgmt.h"
#include "common/logging.hpp"
#include "utils/code_utils.h"

#include <assert.h>
#include <openthread/error.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/ble.h>
#include <openthread/platform/logging.h>

#if OPENTHREAD_ENABLE_TOBLE || OPENTHREAD_ENABLE_CLI_BLE

#if OPENTHREAD_ENABLE_L2CAP
enum
{
    kStackBufferSize = 4832,
};

static wsfBufPoolDesc_t sPoolDesc[] = {{16, 16}, {32, 16}, {64, 8}, {128, 4}, {272, 1}, {1300, 2}};
#else
enum
{
    kStackBufferSize = 2250,
};

static wsfBufPoolDesc_t sPoolDesc[] = {{16, 16}, {32, 16}, {64, 8}, {128, 4}, {272, 1}};
#endif

/* WSF heap allocation */
uint8_t *SystemHeapStart;
uint32_t SystemHeapSize;

enum
{
    kStateIdle           = 0,
    kStateInitializing   = 1,
    kStateInitialized    = 2,
    kStateDeinitializing = 3,
};

enum
{
    kBleResetTimeout = 100,
};

__attribute__((aligned(4))) static uint8_t sStackBuffer[kStackBufferSize];

static uint8_t     sState            = kStateIdle;
static otInstance *sInstance         = NULL;
static bool        sStackInitialized = false;
static bool        sTaskletsPending  = false;
static uint32_t    sLastUpdateMs;
static wsfTimer_t  sTimer;

static void bleStackInit(void);

// This function is called by the ble stack to signal to user code to run wsfOsDispatcher().
void wsf_mbed_ble_signal_event(void)
{
    sTaskletsPending = true;
}

void wsf_mbed_os_critical_section_enter(void)
{
    // Intentionally empty.
}

void wsf_mbed_os_critical_section_exit(void)
{
    // Intentionally empty.
}

otError otPlatBleEnable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((sInstance == NULL) && (aInstance != NULL), error = OT_ERROR_FAILED);
    otEXPECT_ACTION(sState == kStateIdle, error = OT_ERROR_FAILED);

    sInstance     = aInstance;
    sState        = kStateInitializing;
    sLastUpdateMs = otPlatAlarmMilliGetNow();

    bleStackInit();
    bleHciEnable();
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

bool otPlatBleTaskletsArePending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sTaskletsPending;
}

void otPlatBleTaskletsProcess(otInstance *aInstance)
{
    uint32_t        now;
    uint32_t        delta;
    uint32_t        nextTimestamp;
    bool_t          timerRunning;
    wsfTimerTicks_t ticks;

    otEXPECT(sState != kStateIdle);

    sTaskletsPending = false;

    now   = otPlatBleAlarmMilliGetNow();
    delta = (now > sLastUpdateMs) ? (now - sLastUpdateMs) : (sLastUpdateMs - now);
    ticks = delta / WSF_MS_PER_TICK;

    if (ticks > 0)
    {
        WsfTimerUpdate(ticks);
        sLastUpdateMs = now;
    }

    wsfOsDispatcher();

    if (wsfOsReadyToSleep()) {
        nextTimestamp = (uint32_t) (WsfTimerNextExpiration(&timerRunning) * WSF_MS_PER_TICK);

        if (timerRunning)
        {
            otPlatBleAlarmMilliStartAt(aInstance, now, nextTimestamp);
        }
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void otPlatBleAlarmMilliFired(otInstance *aInstance)
{
    otPlatBleTaskletsProcess(aInstance);
}

otInstance *bleMgmtGetThreadInstance(void)
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
        // initialize extended module if supported
        if (HciGetLeSupFeat() & HCI_LE_SUP_FEAT_LE_EXT_ADV)
        {
            DmExtAdvInit();
            DmExtScanInit();
            DmExtConnMasterInit();
            DmExtConnSlaveInit();
        }

        if (sState == kStateInitializing)
        {
            sState = kStateInitialized;

            WsfTimerStop(&sTimer);
            otPlatBleOnEnabled(sInstance);
        }
        else if (sState == kStateDeinitializing)
        {
            sState = kStateIdle;

            WsfTimerStop(&sTimer);
            bleGattReset();
#if OPENTHREAD_ENABLE_L2CAP
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
#if OPENTHREAD_ENABLE_L2CAP
        bleL2capReset();
#endif
        bleGapReset();
        bleHciDisable();
    }

    sState    = kStateIdle;
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

static void bleStackInit(void)
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
 * The WSF adaptation functions definition.
 *
 */

/* LED */

void PalLedOn(uint8_t id)
{
    OT_UNUSED_VARIABLE(id);
}

void PalLedOff(uint8_t id)
{
    OT_UNUSED_VARIABLE(id);
}

/* RTC */

void PalRtcInit(void)
{
}

void PalRtcEnableCompareIrq(void)
{
}

void PalRtcDisableCompareIrq(void)
{
}

uint32_t PalRtcCounterGet(void)
{
    return 0;
}

void PalRtcCompareSet(uint32_t value)
{
    OT_UNUSED_VARIABLE(value);
}

uint32_t PalRtcCompareGet(void)
{
    return 0;
}

/* SYS */

bool_t PalSysIsBusy(void)
{
    return 0;
}

void PalSysAssertTrap(void)
{
}

void PalSysSleep(void)
{
}

/**
 * The BLE Management module weak functions definition.
 *
 */
OT_TOOL_WEAK void otPlatBleOnEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

#endif // OPENTHREAD_ENABLE_TOBLE || OPENTHREAD_ENABLE_CLI_BLE
