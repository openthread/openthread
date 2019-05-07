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
#define NS_PER_US 1000
#define US_PER_MS 1000
#define US_PER_S 1000000

#define DEFAULT_TIMEOUT 10 // seconds

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;
#endif

#if OPENTHREAD_ENABLE_BLE_HOST
static bool     sIsBleMsRunning = false;
static uint32_t sBleMsAlarm     = 0;
#endif

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
static bool     sIsBleUsRunning = false;
static uint32_t sBleUsAlarm     = 0;
#endif

static uint32_t sSpeedUpFactor = 1;

void platformAlarmInit(uint32_t aSpeedUpFactor)
{
    sSpeedUpFactor = aSpeedUpFactor;
}

#if defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC)
uint64_t platformGetNow(void)
{
    struct timespec now;
    int             err;

#ifdef CLOCK_MONOTONIC_RAW
    err = clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#else
    err = clock_gettime(CLOCK_MONOTONIC, &now);
#endif

    assert(err == 0);

    return (uint64_t)now.tv_sec * sSpeedUpFactor * US_PER_S + (uint64_t)now.tv_nsec * sSpeedUpFactor / NS_PER_US;
}
#else
uint64_t platformGetNow(void)
{
    struct timeval tv;
    int            err;

    err = gettimeofday(&tv, NULL);

    assert(err == 0);

    return (uint64_t)tv.tv_sec * sSpeedUpFactor * US_PER_S + (uint64_t)tv.tv_usec * sSpeedUpFactor;
}
#endif // defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC)

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

static int64_t getMinUsRemaining(void)
{
    int64_t usRemaining;
    int64_t minUsRemaining = DEFAULT_TIMEOUT * US_PER_S;

    if (sIsMsRunning)
    {
        usRemaining    = (int64_t)(sMsAlarm - otPlatAlarmMilliGetNow()) * US_PER_MS;
        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    if (sIsUsRunning)
    {
        usRemaining    = (int64_t)(sUsAlarm - otPlatAlarmMicroGetNow());
        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }
#endif

#if OPENTHREAD_ENABLE_BLE_HOST
    if (sIsBleMsRunning)
    {
        usRemaining    = (int64_t)(sBleMsAlarm - otPlatBleAlarmMilliGetNow()) * US_PER_MS;
        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }
#endif

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
    if (sIsBleUsRunning)
    {
        usRemaining    = (int64_t)(sBleUsAlarm - otPlatAlarmMicroGetNow());
        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }
#endif

    return minUsRemaining;
}

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int64_t usRemaining = getMinUsRemaining();

    if (aTimeout == NULL)
    {
        return;
    }

    if (usRemaining <= 0)
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
    else
    {
        usRemaining /= sSpeedUpFactor;

#ifndef _WIN32
        aTimeout->tv_sec = (time_t)usRemaining / US_PER_S;
#else
        aTimeout->tv_sec = (long)usRemaining / US_PER_S;
#endif
        aTimeout->tv_usec = usRemaining % US_PER_S;
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

#if OPENTHREAD_ENABLE_BLE_HOST
    if (sIsBleMsRunning)
    {
        remaining = (int32_t)(sBleMsAlarm - otPlatBleAlarmMilliGetNow());

        if (remaining <= 0)
        {
            sIsBleMsRunning = false;
            otPlatBleAlarmMilliFired(aInstance);
        }
    }
#endif // OPENTHREAD_ENABLE_BLE_HOST

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
    if (sIsBleUsRunning)
    {
        remaining = (int32_t)(sBleUsAlarm - platformBleAlarmMicroGetNow());

        if (remaining <= 0)
        {
            sIsBleUsRunning = false;
            platformBleAlarmMicroFired(aInstance);
        }
    }
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER
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

#if OPENTHREAD_ENABLE_BLE_HOST
void otPlatBleAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    sBleMsAlarm     = aT0 + aDt;
    sIsBleMsRunning = true;
}

void otPlatBleAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsBleMsRunning = false;
}

uint32_t otPlatBleAlarmMilliGetNow(void)
{
    return otPlatAlarmMilliGetNow();
}
#endif // OPENTHREAD_ENABLE_BLE_HOST

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
void platformBleAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    sBleUsAlarm     = aT0 + aDt;
    sIsBleUsRunning = true;
    OT_UNUSED_VARIABLE(aInstance);
}

void platformBleAlarmMicroStop(otInstance *aInstance)
{
    sIsBleUsRunning = false;
    OT_UNUSED_VARIABLE(aInstance);
}

uint32_t platformBleAlarmMicroGetNow(void)
{
    return (uint32_t)platformGetNow();
}
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0
