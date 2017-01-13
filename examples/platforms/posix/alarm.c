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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <platform/alarm.h>
#include <platform/diag.h>

static bool s_is_running = false;
static struct timeval s_alarm;
static struct timeval s_start;

static void getNow(struct timeval *tv)
{
    gettimeofday(tv, NULL);
    timersub(tv, &s_start, tv);
}

void platformAlarmInit(void)
{
    gettimeofday(&s_start, NULL);
}

uint32_t otPlatAlarmGetNow(void)
{
    struct timeval tv;

    getNow(&tv);

    return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

void otPlatAlarmGetPreciseNow(otPlatAlarmTime *aNow)
{
    struct timeval tv;

    getNow(&tv);

    aNow->mMs = (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
    aNow->mUs = tv.tv_usec % 1000;
}

void otPlatAlarmStartAt(otInstance *aInstance, const otPlatAlarmTime *t0, const otPlatAlarmTime *dt)
{
    (void)aInstance;
    s_alarm.tv_sec = (t0->mMs + dt->mMs) / 1000;
    s_alarm.tv_usec = (((t0->mMs + dt->mMs) % 1000) * 1000) + (t0->mUs + dt->mUs);

    if (s_alarm.tv_usec >= 1000000)
    {
        s_alarm.tv_sec++;
        s_alarm.tv_usec -= 1000000;
    }

    s_is_running = true;
}

void otPlatAlarmStop(otInstance *aInstance)
{
    (void)aInstance;
    s_is_running = false;
}

void platformAlarmUpdateTimeout(struct timeval *aTimeout)
{
    struct timeval remaining;

    if (aTimeout == NULL)
    {
        return;
    }

    if (s_is_running)
    {
        struct timeval now;
        getNow(&now);

        timersub(&s_alarm, &now, &remaining);

        if (remaining.tv_sec > 0 || (remaining.tv_sec == 0 && remaining.tv_usec > 0))
        {
            memcpy(aTimeout, &remaining, sizeof(*aTimeout));
        }
        else
        {
            aTimeout->tv_sec = 0;
            aTimeout->tv_usec = 0;
        }
    }
    else
    {
        aTimeout->tv_sec = 10;
        aTimeout->tv_usec = 0;
    }
}

void platformAlarmProcess(otInstance *aInstance)
{
    struct timeval remaining;

    if (s_is_running)
    {
        struct timeval now;
        getNow(&now);

        timersub(&s_alarm, &now, &remaining);

        if (remaining.tv_sec < 0 || (remaining.tv_sec == 0 && remaining.tv_usec <= 0))
        {
            s_is_running = false;

#if OPENTHREAD_ENABLE_DIAG

            if (otPlatDiagModeGet())
            {
                otPlatDiagAlarmFired(aInstance);
            }
            else
#endif
            {
                otPlatAlarmFired(aInstance);
            }
        }
    }
}
