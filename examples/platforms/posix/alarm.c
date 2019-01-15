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

#include "platform-posix.h"

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#define MS_PER_S 1000
#define US_PER_MS 1000
#define US_PER_S 1000000

#define DEFAULT_TIMEOUT 10 // seconds

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;

static uint32_t sSpeedUpFactor = 1;

static struct timeval sStart;

void platformAlarmInit(uint32_t aSpeedUpFactor)
{
    sSpeedUpFactor = aSpeedUpFactor;
    gettimeofday(&sStart, NULL);
}

uint64_t platformGetNow(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    timersub(&tv, &sStart, &tv);

    return (uint64_t)tv.tv_sec * sSpeedUpFactor * US_PER_S + (uint64_t)tv.tv_usec * sSpeedUpFactor;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(platformGetNow() / US_PER_MS);
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

uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)platformGetNow();
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

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int32_t usRemaining = DEFAULT_TIMEOUT * US_PER_S;
    int32_t msRemaining = DEFAULT_TIMEOUT * MS_PER_S;

    if (aTimeout == NULL)
    {
        return;
    }

    if (sIsUsRunning)
    {
        usRemaining = (int32_t)(sUsAlarm - otPlatAlarmMicroGetNow());
    }

    if (sIsMsRunning)
    {
        msRemaining = (int32_t)(sMsAlarm - otPlatAlarmMilliGetNow());
    }

    if (usRemaining <= 0 || msRemaining <= 0)
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
    else
    {
        int64_t remaining = ((int64_t)msRemaining) * US_PER_MS;

        if (usRemaining < remaining)
        {
            remaining = usRemaining;
        }

        remaining /= sSpeedUpFactor;

        if (remaining == 0)
        {
            remaining = 1;
        }

#ifndef _WIN32
        aTimeout->tv_sec = (time_t)remaining / US_PER_S;
#else
        aTimeout->tv_sec = (long)remaining / US_PER_S;
#endif
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
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

    if (sIsUsRunning)
    {
        remaining = (int32_t)(sUsAlarm - otPlatAlarmMicroGetNow());

        if (remaining <= 0)
        {
            sIsUsRunning = false;

            otPlatAlarmMicroFired(aInstance);
        }
    }

#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
}

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
uint64_t otPlatTimeGet(void)
{
    return platformGetNow();
}

uint16_t otPlatTimeGetXtalAccuracy(void)
{
    return 0;
}
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0
