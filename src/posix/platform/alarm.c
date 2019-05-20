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

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#ifdef OPENTHREAD_ENABLE_BLE_HOST
#include <openthread/platform/cordio/ble-alarm.h>
#endif
#include <openthread/platform/diag.h>

#include "code_utils.h"

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;
#endif

static uint32_t sSpeedUpFactor = 1;
#if OPENTHREAD_ENABLE_BLE_HOST
static bool     sIsBleMsRunning = false;
static uint32_t sBleMsAlarm     = 0;
#endif

#if !OPENTHREAD_POSIX_VIRTUAL_TIME
uint64_t otSysGetTime(void)
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
    return otSysGetTime() * sSpeedUpFactor;
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

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
uint32_t otPlatAlarmMicroGetNow(void)
{
    return (uint32_t)(otSysGetTime());
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
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

#if OPENTHREAD_ENABLE_BLE_HOST
void otCordioPlatAlarmTickStartAt(uint32_t aT0, uint32_t aDt)
{
    sBleMsAlarm     = aT0 + aDt;
    sIsBleMsRunning = true;
}

void otCordioPlatAlarmTickStop(void)
{
    sIsBleMsRunning = false;
}

uint32_t otCordioPlatAlarmTickGetNow(void)
{
    return otPlatAlarmMilliGetNow();
}

void otCordioPlatAlarmEnableInterrupt(void)
{
}

void otCordioPlatAlarmDisableInterrupt(void)
{
}
#endif // OPENTHREAD_ENABLE_BLE_HOST

static int64_t getMinUsRemaining(void)
{
    int64_t  usRemaining;
    int64_t  minUsRemaining = INT64_MAX;
    uint64_t now            = platformAlarmGetNow();

    if (sIsMsRunning)
    {
        usRemaining = (int64_t)(sMsAlarm - (uint32_t)(now / US_PER_MS));
        usRemaining *= US_PER_MS;
        usRemaining -= (now % US_PER_MS);

        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    if (sIsUsRunning)
    {
        usRemaining    = (int64_t)(sUsAlarm - now);
        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }
#endif

#if OPENTHREAD_ENABLE_BLE_HOST
    if (sIsBleMsRunning)
    {
        usRemaining = (int64_t)(sBleMsAlarm - (uint32_t)(now / US_PER_MS));
        usRemaining *= US_PER_MS;
        usRemaining -= (now % US_PER_MS);

        minUsRemaining = (usRemaining < minUsRemaining) ? usRemaining : minUsRemaining;
    }
#endif

    return minUsRemaining;
}

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int64_t remaining = getMinUsRemaining();

    assert(aTimeout != NULL);

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

        aTimeout->tv_sec  = (time_t)remaining / US_PER_S;
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

#if OPENTHREAD_ENABLE_BLE_HOST

    if (sIsBleMsRunning)
    {
        remaining = (int32_t)(sBleMsAlarm - otCordioPlatAlarmTickGetNow());

        if (remaining <= 0)
        {
            sIsBleMsRunning = false;
            otCordioPlatAlarmTickFired();
        }
    }

#endif // OPENTHREAD_ENABLE_BLE_HOST
}
