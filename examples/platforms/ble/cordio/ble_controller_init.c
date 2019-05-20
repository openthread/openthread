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
 *   This file implements the BLE Controller initialization interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "bb_api.h"
#include "chci_api.h"
#include "lctr_int_conn.h"
#include "ll_init_api.h"

#include "cordio/ble_config.h"
#include "cordio/ble_init.h"
#include "utils/code_utils.h"

#include <assert.h>
#include <openthread/error.h>
#include <openthread/platform/ble.h>
#include <openthread/platform/cordio/radio-ble.h>

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
#define LL_IMPL_REV 0x2303 // Typical implementation revision number

enum
{
    kCtrlPoolBufferSize = 3952,
};

__attribute__((aligned(4))) static uint8_t sCtrlPoolBuffer[kCtrlPoolBufferSize];

static BbRtCfg_t sBbRtCfg = {.clkPpm          = 0,
                             .rfSetupDelayUs  = BB_RF_SETUP_DELAY_US,
                             .maxScanPeriodMs = BB_MAX_SCAN_PERIOD_MS,
                             .schSetupDelayUs = BB_SCH_SETUP_DELAY_US};

const LlRtCfg_t sLlRtCfg = {
    // Device
    .compId  = LL_COMP_ID_ARM,
    .implRev = LL_IMPL_REV,
    .btVer   = LL_VER_BT_CORE_SPEC_4_2,

    // Advertiser
    .maxAdvSets        = 0,
    .maxAdvReports     = BLE_STACK_MAX_ADV_REPORTS,
    .maxExtAdvDataLen  = 0,
    .defExtAdvDataFrag = 0,

    // Scanner
    .maxScanReqRcvdEvt = 0,
    .maxExtScanDataLen = 0,

    // Connection
    .maxConn      = BLE_STACK_MAX_BLE_CONNECTIONS,
    .numTxBufs    = BLE_STACK_NUM_ACL_TRANSMIT_BUFFERS,
    .numRxBufs    = BLE_STACK_NUM_ACL_RECEIVE_BUFFERS,
    .maxAclLen    = BLE_STACK_MAX_ACL_DATA_LENGTH,
    .defTxPwrLvl  = 0,
    .ceJitterUsec = 0,

    // DTM
    .dtmRxSyncMs = 10000,

    // PHY
    .phy2mSup          = FALSE,
    .phyCodedSup       = FALSE,
    .stableModIdxTxSup = FALSE,
    .stableModIdxRxSup = FALSE};

static LlInitRtCfg_t sLlInitRtCfg = {.pBbRtCfg     = &sBbRtCfg,
                                     .wlSizeCfg    = 0,
                                     .rlSizeCfg    = 0,
                                     .plSizeCfg    = 0,
                                     .pLlRtCfg     = &sLlRtCfg,
                                     .pFreeMem     = sCtrlPoolBuffer,
                                     .freeMemAvail = kCtrlPoolBufferSize};

void bleControllerInit(void)
{
    static bool         initialized = false;
    otPlatBleDeviceAddr bdaddr;

    otEXPECT(!initialized);

    initialized     = true;
    sBbRtCfg.clkPpm = otPlatRadioBleGetXtalAccuracy(bleGetThreadInstance());

    assert(LlInitControllerInit(&sLlInitRtCfg) != 0);

    otPlatRadioBleGetPublicAddress(bleGetThreadInstance(), &bdaddr);
    LlSetBdAddr(bdaddr.mAddr);

exit:
    return;
}
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER
