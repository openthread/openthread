/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#include "platform-emsk.h"
#include <stdbool.h>
#include <stdint.h>
#include "openthread/platform/alarm-milli.h"

static uint32_t sCounter = 0;
static uint32_t expires;
static bool     sIsRunning = false;

void emskAlarmInit(void)
{
    sCounter = OSP_GET_CUR_MS();
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return (OSP_GET_CUR_MS() - sCounter);
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    OT_UNUSED_VARIABLE(aInstance);

    expires    = t0 + dt;
    sIsRunning = true;
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sIsRunning = false;
}

void emskAlarmUpdateTimeout(int32_t *aTimeout)
{
    int32_t remaining;

    otEXPECT(aTimeout != NULL);

    if (sIsRunning)
    {
        remaining = (int32_t)(expires - otPlatAlarmMilliGetNow());

        if (remaining > 0)
        {
            *aTimeout = remaining;
        }
        else
        {
            *aTimeout = 0;
        }
    }
    else
    {
        *aTimeout = 10000;
    }

exit:
    return;
}

void emskAlarmProcess(otInstance *aInstance)
{
    int32_t remaining;

    if (sIsRunning)
    {
        remaining = (int32_t)(expires - otPlatAlarmMilliGetNow());

        if (remaining <= 0)
        {
            sIsRunning = false;
            otPlatAlarmMilliFired(aInstance);
        }
    }
}
