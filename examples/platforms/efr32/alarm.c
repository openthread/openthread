/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include <stdbool.h>
#include <stdint.h>

#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "utils/code_utils.h"

#include "em_core.h"
#include "rail.h"

#define XTAL_ACCURACY 200
#define US_IN_MS 1000

static uint32_t sTimerHi   = 0;
static uint32_t sTimerLo   = 0;
static uint32_t sAlarmT0   = 0;
static uint32_t sAlarmDt   = 0;
static bool     sIsRunning = false;

void efr32AlarmInit(void)
{
}

uint64_t otPlatTimeGet(void)
{
    uint32_t timer_lo;
    uint64_t timer_us;

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    timer_lo = RAIL_GetTime();

    if (timer_lo < sTimerLo)
    {
        sTimerHi++;
    }

    sTimerLo = timer_lo;

    timer_us = (((uint64_t)sTimerHi << 32) | sTimerLo);

    CORE_EXIT_CRITICAL();

    return timer_us;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return otPlatTimeGet() / US_IN_MS;
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
    return XTAL_ACCURACY;
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sAlarmT0   = t0;
    sAlarmDt   = dt;
    sIsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsRunning = false;
}

void efr32AlarmProcess(otInstance *aInstance)
{
    uint32_t now = otPlatAlarmMilliGetNow();
    uint32_t expires;
    bool     fire = false;

    otEXPECT(sIsRunning);

    expires = sAlarmT0 + sAlarmDt;

    if (sAlarmT0 <= now)
    {
        fire = (expires >= sAlarmT0 && expires <= now);
    }
    else
    {
        fire = (expires >= sAlarmT0 || expires <= now);
    }

    if (fire)
    {
        sIsRunning = false;

#if OPENTHREAD_ENABLE_DIAG

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

exit:
    return;
}

void RAILCb_TimerExpired(void)
{
}
