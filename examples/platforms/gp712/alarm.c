/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#include "alarm_qorvo.h"
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>

void qorvoAlarmInit(void)
{
}

uint32_t otPlatAlarmMilliGetNow(void)
{
    return qorvoAlarmGetTimeMs();
}

static void qorvoAlarmFired(void *aInstance)
{
    otPlatAlarmMilliFired((otInstance *)aInstance);
}

void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    OT_UNUSED_VARIABLE(t0);

    qorvoAlarmUnScheduleEventArg((qorvoAlarmCallback_t)qorvoAlarmFired, aInstance);
    qorvoAlarmScheduleEventArg(dt * 1000, qorvoAlarmFired, aInstance);
}

void otPlatAlarmMilliStop(otInstance *aInstance)
{
    qorvoAlarmUnScheduleEventArg((qorvoAlarmCallback_t)qorvoAlarmFired, aInstance);
}

void qorvoAlarmUpdateTimeout(struct timeval *aTimeout)
{
    OT_UNUSED_VARIABLE(aTimeout);
}

void qorvoAlarmProcess(void)
{
}
