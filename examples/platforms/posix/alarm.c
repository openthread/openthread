/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <platform/alarm.h>
#include "platform-posix.h"

static bool s_is_running = false;
static uint32_t s_alarm = 0;
static struct timeval s_start;

void posixAlarmInit(void)
{
    gettimeofday(&s_start, NULL);
}

uint32_t otPlatAlarmGetNow(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    timersub(&tv, &s_start, &tv);

    return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

void otPlatAlarmStartAt(uint32_t t0, uint32_t dt)
{
    s_alarm = t0 + dt;
    s_is_running = true;
}

void otPlatAlarmStop(void)
{
    s_is_running = false;
}

void posixAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int32_t remaining;

    if (aTimeout == NULL)
    {
        return;
    }

    if (s_is_running)
    {
        remaining = (int32_t)(s_alarm - otPlatAlarmGetNow());

        if (remaining > 0)
        {
            aTimeout->tv_sec = remaining / 1000;
            aTimeout->tv_usec = (remaining % 1000) * 1000;
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

void posixAlarmProcess(void)
{
    int32_t remaining;

    if (s_is_running)
    {
        remaining = (int32_t)(s_alarm - otPlatAlarmGetNow());

        if (remaining <= 0)
        {
            s_is_running = false;
            otPlatAlarmFired();
        }
    }
}
