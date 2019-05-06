/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements the Wireless Software Foundation (WSF) interfaces for Cordio BLE stack.
 *
 */
#include <openthread-core-config.h>
#include <openthread/config.h>

#include "wsf_types.h"

#include "wsf_buf.h"
#include "wsf_mbed_os_adaptation.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_timer.h"

#include "cordio/ble_init.h"
#include "utils/code_utils.h"

#include <openthread/error.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/ble.h>

#if OPENTHREAD_ENABLE_BLE_HOST
static bool     sTaskletsPending = false;
static uint32_t sLastUpdateMs;

// This function is called by the ble stack to signal to user code to run wsfOsDispatcher().
void wsf_mbed_ble_signal_event(void)
{
    sTaskletsPending = true;
}

void wsf_mbed_os_critical_section_enter(void)
{
    // Intentionally empty.
}

void wsf_mbed_os_critical_section_exit(void)
{
    // Intentionally empty.
}

void bleWsfInit(void)
{
    sLastUpdateMs = otPlatAlarmMilliGetNow();
}

bool otPlatBleTaskletsArePending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sTaskletsPending;
}

void otPlatBleTaskletsProcess(otInstance *aInstance)
{
    uint32_t        now;
    uint32_t        delta;
    uint32_t        nextTimestamp;
    bool_t          timerRunning;
    wsfTimerTicks_t ticks;

    otEXPECT(bleGetState() != kStateDisabled);

    sTaskletsPending = false;

    now   = otPlatBleAlarmMilliGetNow();
    delta = (now > sLastUpdateMs) ? (now - sLastUpdateMs) : (sLastUpdateMs - now);
    ticks = delta / WSF_MS_PER_TICK;

    if (ticks > 0)
    {
        WsfTimerUpdate(ticks);
        sLastUpdateMs = now;
    }

    wsfOsDispatcher();

    if (wsfOsReadyToSleep())
    {
        nextTimestamp = (uint32_t)(WsfTimerNextExpiration(&timerRunning) * WSF_MS_PER_TICK);

        if (timerRunning)
        {
            otPlatBleAlarmMilliStartAt(aInstance, now, nextTimestamp);
        }
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return;
}

void otPlatBleAlarmMilliFired(otInstance *aInstance)
{
    otPlatBleTaskletsProcess(aInstance);
}

/**
 * The unused WSF adaptation functions definition.
 *
 */

/* LED */

void PalLedOn(uint8_t id)
{
    OT_UNUSED_VARIABLE(id);
}

void PalLedOff(uint8_t id)
{
    OT_UNUSED_VARIABLE(id);
}

/* RTC */

void PalRtcInit(void)
{
}

void PalRtcEnableCompareIrq(void)
{
}

void PalRtcDisableCompareIrq(void)
{
}

uint32_t PalRtcCounterGet(void)
{
    return 0;
}

void PalRtcCompareSet(uint32_t value)
{
    OT_UNUSED_VARIABLE(value);
}

uint32_t PalRtcCompareGet(void)
{
    return 0;
}

/* SYS */

bool_t PalSysIsBusy(void)
{
    return 0;
}

void PalSysAssertTrap(void)
{
}

void PalSysSleep(void)
{
}

#endif // OPENTHREAD_ENABLE_BLE_HOST
