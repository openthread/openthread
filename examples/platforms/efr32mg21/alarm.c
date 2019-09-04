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

#define XTAL_ACCURACY 200
#define US_IN_MS 1000

// Minimum duration of an alarm in milliseconds. Used to avoid setting the absolute
// expiry time of an alarm to the current time or slightly in the past.
#define TIMER_EPSILON_MS 1

// The longest Rail can set a timer is 53 minutes.  Timers of a longer duration
// must wake up before this and set another timer for the remainder.  We currently
// split long delays in 30 minute intervals using a value of 1800000.
#define RAIL_TIMER_MAX_DELTA_MS 1800000

static uint32_t sTimerHi   = 0;
static uint32_t sTimerLo   = 0;
static uint32_t sAlarmT0   = 0;
static uint32_t sAlarmDt   = 0;
static bool     sIsRunning = false;

static void RAILCb_TimerExpired(RAIL_Handle_t aHandle)
{
}

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
    uint32_t      expires_microsec;
    RAIL_Status_t status;

    assert(gRailHandle != NULL);

    if (sIsRunning)
    {
        RAIL_CancelTimer(gRailHandle);
    }

    sAlarmT0 = t0;
    sAlarmDt = dt;

    if (dt > RAIL_TIMER_MAX_DELTA_MS)
    {
        dt = RAIL_TIMER_MAX_DELTA_MS;
    }
    else if (dt < TIMER_EPSILON_MS)
    {
        dt = TIMER_EPSILON_MS;
    }

    expires_microsec = (t0 + dt) * US_IN_MS;
    status           = RAIL_SetTimer(gRailHandle, expires_microsec, RAIL_TIME_ABSOLUTE, RAILCb_TimerExpired);

    if (status != RAIL_STATUS_NO_ERROR)
    {
        // The RAIL timer could not be set due to expiration time being in the past with respect to RAIL's current
        // time which is in microseconds. We fallback to using a relative timer from the current time.

        expires_microsec = dt * US_IN_MS;
        status           = RAIL_SetTimer(gRailHandle, expires_microsec, RAIL_TIME_DELAY, RAILCb_TimerExpired);

        if (status != RAIL_STATUS_NO_ERROR)
        {
            otLogCritPlat("Alarm start timer failed, status: %d, dt: %u, t0: %u, now: %u", status, dt, t0,
                          otPlatAlarmMilliGetNow());
            assert(false);
        }
    }

    sIsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsRunning = false;

    assert(gRailHandle != NULL);
    RAIL_CancelTimer(gRailHandle);
}

void efr32AlarmProcess(otInstance *aInstance)
{
    uint32_t      now;
    uint32_t      new_expires_microsec;
    uint32_t      dt;
    RAIL_Status_t status;

    otEXPECT(sIsRunning);

    assert(gRailHandle != NULL);

    if (RAIL_IsTimerExpired(gRailHandle))
    {
        sIsRunning = false;

        if (sAlarmDt > RAIL_TIMER_MAX_DELTA_MS)
        {
            // We split longer delays in two due to the maximum allowed timer in RAIL.  Here we
            // re-arm the RAIL timer with the remaining part of the alarm.

            now = otPlatAlarmMilliGetNow();
            dt  = (sAlarmT0 + sAlarmDt) - now;

            if (dt > RAIL_TIMER_MAX_DELTA_MS)
            {
                dt = RAIL_TIMER_MAX_DELTA_MS;
            }
            else if (dt < TIMER_EPSILON_MS)
            {
                dt = TIMER_EPSILON_MS;
            }

            new_expires_microsec = (now + dt) * US_IN_MS;
            status = RAIL_SetTimer(gRailHandle, new_expires_microsec, RAIL_TIME_ABSOLUTE, RAILCb_TimerExpired);

            if (status != RAIL_STATUS_NO_ERROR)
            {
                new_expires_microsec = dt * US_IN_MS;
                status = RAIL_SetTimer(gRailHandle, new_expires_microsec, RAIL_TIME_DELAY, RAILCb_TimerExpired);

                if (status != RAIL_STATUS_NO_ERROR)
                {
                    otLogCritPlat("Alarm extend timer failed, status: %d, dt: %u, now: %u", status, dt,
                                  otPlatAlarmMilliGetNow());
                    assert(false);
                }
            }

            sIsRunning = true;
        }
        else
        {
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
exit:
    return;
}
