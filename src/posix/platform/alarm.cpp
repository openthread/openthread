/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#include "common/code_utils.hpp"

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;
#endif

static uint32_t sSpeedUpFactor = 1;

#if !OPENTHREAD_POSIX_VIRTUAL_TIME
uint64_t platformGetTime(void)
{
    struct timespec now;

#ifdef CLOCK_MONOTONIC_RAW
    VerifyOrDie(clock_gettime(CLOCK_MONOTONIC_RAW, &now) == 0, OT_EXIT_FAILURE);
#else
    VerifyOrDie(clock_gettime(CLOCK_MONOTONIC, &now) == 0, OT_EXIT_FAILURE);
#endif

    return (uint64_t)now.tv_sec * US_PER_S + (uint64_t)now.tv_nsec / NS_PER_US;
}
#endif // !OPENTHREAD_POSIX_VIRTUAL_TIME

static uint64_t platformAlarmGetNow(void)
{
    return platformGetTime() * sSpeedUpFactor;
}

void platformAlarmInit(uint32_t aSpeedUpFactor)
{
    sSpeedUpFactor = aSpeedUpFactor;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(platformAlarmGetNow() / US_PER_MS);
}

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

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)(platformGetTime());
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sUsAlarm     = aT0 + aDt;
    sIsUsRunning = true;
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsUsRunning = false;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int64_t  remaining = INT32_MAX;
    uint64_t now       = platformAlarmGetNow();

    assert(aTimeout != NULL);

    if (sIsMsRunning)
    {
        remaining = (int32_t)(sMsAlarm - (uint32_t)(now / US_PER_MS));
        VerifyOrExit(remaining > 0);
        remaining *= US_PER_MS;
        remaining -= (now % US_PER_MS);
    }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (sIsUsRunning)
    {
        int32_t usRemaining = (int32_t)(sUsAlarm - (uint32_t)now);

        if (usRemaining < remaining)
        {
            remaining = usRemaining;
        }
    }
#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

exit:
    if (remaining <= 0)
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

        aTimeout->tv_sec  = (time_t)(remaining / US_PER_S);
        aTimeout->tv_usec = remaining % US_PER_S;
    }
}

void platformAlarmProcess(otInstance *aInstance)
{
    int32_t remaining;

    if (sIsMsRunning)
    {
        remaining = (int32_t)(sMsAlarm - otPlatAlarmMilliGetNow());

        if (remaining <= 0)
        {
            sIsMsRunning = false;

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

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

    if (sIsUsRunning)
    {
        remaining = (int32_t)(sUsAlarm - otPlatAlarmMicroGetNow());

        if (remaining <= 0)
        {
            sIsUsRunning = false;

            otPlatAlarmMicroFired(aInstance);
        }
    }

#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
}
