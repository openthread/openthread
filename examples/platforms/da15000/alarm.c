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

/**
* @file alarm.c
* Platform abstraction for the alarm
*/

#include <stdbool.h>
#include <stdint.h>
#include <platform/alarm.h>
#include "hw_timer0.h"
#include "hw_gpio.h"
#include "platform-da15000.h"

static bool s_is_running = false;
static uint32_t s_alarm = 0;
static uint32_t s_counter;
volatile bool s_alarm_fired = false;

static void timer0_interrupt_cb(void)
{
    s_counter++;
}

void da15000AlarmProcess(otInstance *aInstance)
{
    int32_t remaining;

    if (s_is_running)
    {
        remaining = s_alarm - s_counter;

        if (remaining <= 0)
        {
            s_is_running = false;
            otPlatAlarmFired(aInstance);
        }
    }
}

void da15000AlarmInit(void)
{
    hw_timer0_init(NULL);
    hw_timer0_set_clock_source(HW_TIMER0_CLK_SRC_FAST);
    hw_timer0_set_pwm_mode(HW_TIMER0_MODE_PWM);
    hw_timer0_set_fast_clock_div(HW_TIMER0_FAST_CLK_DIV_4);
    hw_timer0_set_t0_reload(0x07D0, 0x07D0);
    hw_timer0_register_int(timer0_interrupt_cb);
    hw_timer0_set_on_clock_div(false);
}

uint32_t otPlatAlarmGetNow(void)
{
    return s_counter;
}

void otPlatAlarmStartAt(otInstance *aInstance, uint32_t t0, uint32_t dt)
{
    (void)aInstance;
    s_alarm = t0 + dt;
    s_is_running = true;

    if (s_counter == 0)
    {
        hw_timer0_enable();
    }


    if (s_is_running == false)
    {
        s_is_running = true;
    }

    hw_timer0_unfreeze();
}

void otPlatAlarmStop(otInstance *aInstance)
{
    (void)aInstance;
    s_is_running = false;
    hw_timer0_freeze();
}



