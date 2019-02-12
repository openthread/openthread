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

#include "ble/ble_hci_driver.h"
#include "common/logging.hpp"
#include "utils/code_utils.h"

#include <assert.h>
#include <openthread/error.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/ble-host.h>
#include <openthread/platform/logging.h>

#if OPENTHREAD_ENABLE_TOBLE

enum
{
    kBleCordioBufferSize  = 2250,
    kBleCordioMaxRxAclLen = 100,
};

enum
{
    kStateIdle         = 0,
    kStateInitializing = 1,
    kStateInitialized  = 2,
};

static const wsfBufPoolDesc_t sPoolDesc[] = {{16, 16}, {32, 16}, {64, 8}, {128, 4}, {272, 1}};

__attribute__((aligned(4))) static uint8_t sBuffer[kBleCordioBufferSize];

static uint32_t sLastUpdateMs;
static uint8_t  sState = kStateIdle;

static void utilsBleHostSetup(void);

/*
 * This function will signal to the user code by calling signalEventsToProcess.
 * It is registered and called into the Wsf Stack.
 */
void wsf_mbed_ble_signal_event(void)
{
    // utilsBleHostProcess();
}

void wsf_mbed_os_critical_section_enter(void)
{
    // Intentionally empty.
}

void wsf_mbed_os_critical_section_exit(void)
{
    // Intentionally empty.
}

//-------------------------------
otError otBleHostInit(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState == kStateIdle, error = OT_ERROR_FAILED);

    utilsBleHostSetup();
    utilsBleHciInit();

    DmDevReset();
    sState = kStateInitializing;

exit:
    return error;
}

otError otBleHostDeinit(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState == kStateInitialized, error = OT_ERROR_INVALID_STATE);

    sState = kStateIdle;

    utilsBleHciDeInit();

    // gattServer().reset();
    // getGattClient().reset();
    // getGap().reset();

exit:
    return error;
}

void otBleHostTaskletsProcess(otInstance *aInstance)
{
    uint32_t        now = otPlatAlarmMilliGetNow();
    uint32_t        delta;
    wsfTimerTicks_t ticks;

    otEXPECT(sState == kStateInitialized);

    delta = (now > sLastUpdateMs) ? (now - sLastUpdateMs) : (sLastUpdateMs - now);
    ticks = delta / WSF_MS_PER_TICK;

    if (ticks > 0)
    {
        WsfTimerUpdate(ticks);
        sLastUpdateMs = now;
    }

    wsfOsDispatcher();

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void utilsBleHostInitDone(otError error)
{
    OT_UNUSED_VARIABLE(error);
    // ble.gap().onDisconnection(disconnectionCallback);
    // ble.gap().onConnection(connectionCallback);

    // ble.gattClient().onDataRead(triggerToggledWrite);
    // ble.gattClient().onDataWrite(triggerRead);

    // scan interval: 400ms and scan window: 400ms.
    // Every 400ms the device will scan for 400ms
    // This means that the device will scan continuously.

    // ble.gap().setScanParams(400, 400);
    // ble.gap().startScan(advertisementCallback);
}

static void BleStackHandler(wsfEventMask_t aEvent, wsfMsgHdr_t *aMsg)
{
    otEXPECT(aMsg != NULL);

    // if (ble::pal::vendor::cordio::CordioSecurityManager::get_security_manager().sm_handler(aMsg)) {
    //     return;
    // }

    switch (aMsg->event)
    {
    case DM_RESET_CMPL_IND:
    {
        // BLE::InitializationCompleteCallbackContext context =
        // {
        //     BLE::Instance(::BLE::DEFAULT_INSTANCE),
        //     BLE_ERROR_NONE
        // };

        // initialize extended module if supported
        if (HciGetLeSupFeat() & HCI_LE_SUP_FEAT_LE_EXT_ADV)
        {
            DmExtAdvInit();
            DmExtScanInit();
            DmExtConnMasterInit();
            DmExtConnSlaveInit();
        }

        // deviceInstance().getGattServer().initialize();
        // deviceInstance().initialization_status = INITIALIZED;
        // _init_callback.call(&context);
        sState = kStateInitialized;
        utilsBleHostInitDone(OT_ERROR_NONE);
        break;
    }
    default:
        // ble::pal::vendor::cordio::Gap::gap_handler(msg);
        break;
    }

    OT_UNUSED_VARIABLE(aEvent);

exit:
    return;
}

static void BleDeviceManagerHandler(dmEvt_t *aDmEvent)
{
    BleStackHandler(0, &aDmEvent->hdr);
}

/*
 * AttServerInitDeInitCback callback is used to Initialize/Deinitialize
 * the CCC Table of the ATT Server when a remote peer requests to Open
 * or Close the connection.
 */
static void BleConnectionHandler(dmEvt_t *aDmEvent)
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

static uint8_t BleGattServerAttsAuthHandler(dmConnId_t aConnId, uint8_t aPermit, uint16_t aHandle)
{
    OT_UNUSED_VARIABLE(aConnId);
    OT_UNUSED_VARIABLE(aPermit);
    OT_UNUSED_VARIABLE(aHandle);

    return 0;
}

static void BleAttClientHandler(attEvt_t* aEvent)
{
    OT_UNUSED_VARIABLE(aEvent);
}

static void utilsBleHostSetup(void)
{
    uint32_t       bytesUsed;
    wsfHandlerId_t handlerId;

    bytesUsed = WsfBufInit(sizeof(sBuffer), sBuffer, sizeof(sPoolDesc) / sizeof(wsfBufPoolDesc_t), sPoolDesc);

    assert(bytesUsed != 0);
    if (bytesUsed < sizeof(sBuffer))
    {
        otLogNotePlat("Too much memory allocated for Cordio memory pool, reduce kBleCordioBufferSize by %d bytes",
                      sizeof(sBuffer) - bytesUsed);
    }

    sLastUpdateMs = otPlatAlarmMilliGetNow();

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

    handlerId = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(handlerId);
    AttsInit();
    AttsIndInit();
    AttsSignInit();
    AttsAuthorRegister(BleGattServerAttsAuthHandler);
    AttcInit();
    AttcSignInit();

    handlerId = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(handlerId);
    SmprInit();
    SmprScInit();
    SmpiInit();
    SmpiScInit();

    WsfOsSetNextHandler(BleStackHandler);

    HciSetMaxRxAclLen(kBleCordioMaxRxAclLen);

    DmRegister(BleDeviceManagerHandler);
    DmConnRegister(DM_CLIENT_ID_APP, BleDeviceManagerHandler);
    AttConnRegister(BleConnectionHandler);
    AttRegister(BleAttClientHandler);
}

#endif // OPENTHREAD_ENABLE_TOBLE
