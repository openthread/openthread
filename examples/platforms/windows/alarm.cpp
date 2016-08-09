/*
 *  Copyright (c) 2016, Microsoft Corporation.
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

#include <windows.h>
#include <openthread.h>

#include <platform\alarm.h>
#include "platform-windows.h"

static bool s_is_running = false;
static ULONG s_alarm = 0;
static ULONG s_start;

void windowsAlarmInit(void)
{
    s_start = GetTickCount();
}

EXTERN_C uint32_t otPlatAlarmGetNow()
{
    return GetTickCount();
}

EXTERN_C void otPlatAlarmStartAt(otContext *, uint32_t t0, uint32_t dt)
{
    s_alarm = t0 + dt;
    s_is_running = true;
}

EXTERN_C void otPlatAlarmStop(otContext *)
{
    s_is_running = false;
}

void windowsAlarmUpdateTimeout(struct timeval *aTimeout)
{
    int32_t remaining;

    if (aTimeout == NULL)
    {
        return;
    }

    if (s_is_running)
    {
        remaining = s_alarm - GetTickCount();

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

void windowsAlarmProcess(otContext *aContext)
{
    int32_t remaining;

    if (s_is_running)
    {
        remaining = s_alarm - GetTickCount();

        if (remaining <= 0)
        {
            s_is_running = false;
            otPlatAlarmFired(aContext);
        }
    }
}
