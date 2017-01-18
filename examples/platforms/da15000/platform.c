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
 * @file
 * This file includes the platform-specific initializers.
 */

#include <assert.h>
#include <stdint.h>

#include <openthread.h>
#include "platform/alarm.h"
#include <openthread-types.h>
#include "platform-da15000.h"
#include "sdk_defs.h"
#include "hw_cpm.h"
#include "hw_watchdog.h"
#include "ftdf.h"
#include "hw_gpio.h"

static bool sBlink = false;
static int  sMsCounterInit;
static int  sMsCounter;

#define LEADER_BLINK_TIME     (200)
#define ROUTER_BLINK_TIME     (500)
#define CHILD_BLINK_TIME      (2000)


otInstance *sInstance;


void ClkInit(void)
{

    NVIC_ClearPendingIRQ(XTAL16RDY_IRQn);
    NVIC_EnableIRQ(XTAL16RDY_IRQn);                 // Activate XTAL16 Ready IRQ
    hw_cpm_set_divn(false);                         // External crystal is 16MHz
    hw_cpm_enable_rc32k();
    hw_cpm_lp_set_rc32k();
    hw_cpm_set_xtal16m_settling_time(dg_configXTAL16_SETTLE_TIME_RC32K);
    hw_cpm_enable_xtal16m();                        // Enable XTAL16M
    hw_cpm_configure_xtal32k_pins();                // Configure XTAL32K pins
    hw_cpm_configure_xtal32k();                     // Configure XTAL32K
    hw_cpm_enable_xtal32k();                        // Enable XTAL32K
    hw_watchdog_unfreeze();                         // Start watchdog

    while (!hw_cpm_is_xtal16m_started());           // Block until XTAL16M starts

    hw_watchdog_freeze();                           // Stop watchdog
    hw_cpm_set_recharge_period((uint16_t)dg_configSET_RECHARGE_PERIOD);
    hw_cpm_set_sysclk(SYS_CLK_IS_XTAL16M);
    hw_cpm_set_hclk_div(0);
    hw_cpm_set_pclk_div(0);
}

/*
 * Example function. Blink LED according to node state
 * Leader       - 5Hz
 * Router       - 2Hz
 * Child        - 0.5Hz
 */

void ExampleProcess(otInstance *aInstance)
{
    otDeviceRole  devRole;
    static int    thrValue;

    devRole = otThreadGetDeviceRole(aInstance);

    if (sBlink == false && otPlatAlarmGetNow() != 0)
    {
        sMsCounterInit = otPlatAlarmGetNow();
        sBlink = true;
    }

    sMsCounter = otPlatAlarmGetNow() - sMsCounterInit;

    switch (devRole)
    {
    case kDeviceRoleLeader:
        thrValue = LEADER_BLINK_TIME;
        break;

    case kDeviceRoleRouter:
        thrValue = ROUTER_BLINK_TIME;
        break;

    case kDeviceRoleChild:
        thrValue = CHILD_BLINK_TIME;
        break;

    default:
        thrValue = 0x00;
    }

    if ((thrValue != 0x00) && (sMsCounter >= thrValue))
    {
        hw_gpio_toggle(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        sMsCounterInit = otPlatAlarmGetNow();
    }

    if (thrValue == 0)
    {
        hw_gpio_set_inactive(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
    }
}


void PlatformInit(int argc, char *argv[])
{
    // Initialize System Clock
    ClkInit();
    // Initialize Random number generator
    da15000RandomInit();
    // Initialize Alarm
    da15000AlarmInit();
    // Initialize Radio
    da15000RadioInit();
    // enable interrupts
    portENABLE_INTERRUPTS();

    (void)argc;
    (void)argv;
}


void PlatformProcessDrivers(otInstance *aInstance)
{
    sInstance = aInstance;

    da15000AlarmProcess(aInstance);
    da15000UartProcess();
    da15000RadioProcess(aInstance);
    ExampleProcess(aInstance);
}
