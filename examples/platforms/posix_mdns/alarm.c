/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "platform.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/time.h>

#include "lib/platform/exit_code.h"

#define DEFAULT_TIMEOUT_IN_SEC 10 // seconds

#ifdef CLOCK_MONOTONIC_RAW
#define OT_CLOCK_ID CLOCK_MONOTONIC_RAW
#else
#define OT_CLOCK_ID CLOCK_MONOTONIC
#endif

static bool     sIsMsRunning   = false;
static uint32_t sMsAlarm       = 0;
static uint32_t sSpeedUpFactor = 1;

static bool IsExpired(uint32_t aTime, uint32_t aNow)
{
    // Determine whether or not `aTime` is before or same as `aNow`.

    uint32_t diff = aNow - aTime;

    return (diff & (1U << 31)) == 0;
}

static uint32_t CalculateDuration(uint32_t aTime, uint32_t aNow)
{
    // Return the time duration from `aNow` to `aTime` if `aTimer` is
    // after `aNow`, otherwise return zero.

    return IsExpired(aTime, aNow) ? 0 : aTime - aNow;
}

static uint64_t GetNow(void)
{
    uint64_t now;

#if defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC)
    {
        struct timespec ts;
        int             err;

        err = clock_gettime(OT_CLOCK_ID, &ts);
        VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

        now = (uint64_t)ts.tv_sec * OT_US_PER_S + (uint64_t)ts.tv_nsec / OT_NS_PER_US;
    }
#else
    {
        struct timeval tv;
        int            err;

        err = gettimeofday(&tv, NULL);
        VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

        now = (uint64_t)tv.tv_sec * OT_US_PER_S + (uint64_t)tv.tv_usec;
    }
#endif

    now = now * sSpeedUpFactor;

    return now;
}

uint32_t otPlatAlarmMilliGetNow(void) { return (uint32_t)(GetNow() / OT_US_PER_MS); }

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMsAlarm     = aT0 + aDt;
    sIsMsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsMsRunning = false;
}

void platformAlarmInit(uint32_t aSpeedUpFactor) { sSpeedUpFactor = aSpeedUpFactor; }
void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    uint64_t remaining = DEFAULT_TIMEOUT_IN_SEC * OT_US_PER_S; // in usec.

    if (sIsMsRunning)
    {
        uint32_t msRemaining = CalculateDuration(sMsAlarm, otPlatAlarmMilliGetNow());

        remaining = ((uint64_t)msRemaining) * OT_US_PER_MS;
    }

    if (remaining == 0)
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
    else
    {
        remaining /= sSpeedUpFactor;

        if (remaining == 0)
        {
            remaining = 1;
        }

        aTimeout->tv_sec  = (time_t)(remaining / OT_US_PER_S);
        aTimeout->tv_usec = remaining % OT_US_PER_S;
    }
}

void platformAlarmProcess(otInstance *aInstance)
{
    if (sIsMsRunning && IsExpired(sMsAlarm, otPlatAlarmMilliGetNow()))
    {
        sIsMsRunning = false;
        otPlatAlarmMilliFired(aInstance);
    }
}
