/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for the alarm.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "openthread-system.h"
#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include "common/logging.hpp"

#include "platform-efr32.h"
#include "utils/code_utils.h"

#include "em_core.h"
#include "rail.h"
#include "sl_sleeptimer.h"

#define XTAL_ACCURACY 200

static sl_sleeptimer_timer_handle_t sl_handle;
static uint32_t                     sAlarm     = 0;
static bool                         sIsRunning = false;

static void AlarmCallback(sl_sleeptimer_timer_handle_t *aHandle, void *aData)
{
    otSysEventSignalPending();
}

void efr32AlarmInit(void)
{
    memset(&sl_handle, 0, sizeof sl_handle);
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    uint64_t    ticks;
    uint64_t    now;
    sl_status_t status;

    ticks  = sl_sleeptimer_get_tick_count64();
    status = sl_sleeptimer_tick64_to_ms(ticks, &now);
    assert(status == SL_STATUS_OK);
    return (uint32_t)now;
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
    return XTAL_ACCURACY;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    sl_status_t status;
    int32_t     remaining;
    uint32_t    ticks;

    sl_sleeptimer_stop_timer(&sl_handle);

    sAlarm     = aT0 + aDt;
    remaining  = (int32_t)(sAlarm - otPlatAlarmMilliGetNow());
    sIsRunning = true;

    if (remaining <= 0)
    {
        otSysEventSignalPending();
    }
    else
    {
        status = sl_sleeptimer_ms32_to_tick(remaining, &ticks);
        assert(status == SL_STATUS_OK);

        status = sl_sleeptimer_start_timer(&sl_handle, ticks, AlarmCallback, NULL, 0,
                                           SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
        assert(status == SL_STATUS_OK);
    }
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sl_sleeptimer_stop_timer(&sl_handle);
    sIsRunning = false;
}

void efr32AlarmProcess(otInstance *aInstance)
{
    int32_t remaining;

    if (sIsRunning)
    {
        remaining = (int32_t)(sAlarm - otPlatAlarmMilliGetNow());

        if (remaining <= 0)
        {
            sIsRunning = false;

#if OPENTHREAD_CONFIG_DIAG_ENABLE
            if (otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired(aInstance);
            }
            else
#endif
            {
                otPlatAlarmMilliFired(aInstance);
            }
        }
    }
}
