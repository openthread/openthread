/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#if OPENTHREAD_POSIX_VIRTUAL_TIME

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

#define US_PER_MS 1000

extern uint64_t sNow; // microseconds

static bool     sIsMsRunning = false;
static uint32_t sMsAlarm     = 0;

static bool     sIsUsRunning = false;
static uint32_t sUsAlarm     = 0;

void platformAlarmInit(uint32_t aSpeedUpFactor)
{
    OT_UNUSED_VARIABLE(aSpeedUpFactor);

    sNow = 0;
}

uint64_t platformAlarmGetNow(void)
{
    return sNow;
}

void platformAlarmAdvanceNow(uint64_t aDelta)
{
    sNow += aDelta;
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (uint32_t)(sNow / US_PER_MS);
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
    return (uint32_t)sNow;
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

int32_t platformAlarmGetNext(void)
{
    int32_t remaining = INT32_MAX;

    if (sIsMsRunning)
    {
        int32_t milli = (int32_t)(sMsAlarm - otPlatAlarmMilliGetNow());

        remaining = milli * US_PER_MS;

        if (remaining < 0 && milli > 0)
        {
            remaining = INT32_MAX;
        }
    }

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

    if (sIsUsRunning)
    {
        int32_t micro = (int32_t)(sUsAlarm - otPlatAlarmMicroGetNow());

        if (remaining > micro)
        {
            remaining = micro;
        }
    }

#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER

    return remaining;
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

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
