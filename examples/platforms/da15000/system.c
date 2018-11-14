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

#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/thread.h>
#include <openthread/platform/alarm-milli.h>

#include "platform-da15000.h"

#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_qspi.h"
#include "hw_watchdog.h"
#include "sdk_defs.h"

static bool sBlink = false;
static int  sMsCounterInit;
static int  sMsCounter;

// clang-format off
#define ALIVE_LED_PERIOD      (50000)
#define ALIVE_LED_DUTY        (500)
#define LEADER_BLINK_TIME     (200)
#define ROUTER_BLINK_TIME     (500)
#define CHILD_BLINK_TIME      (2000)
// clang-format on

static otInstance *sInstance = NULL;

/*
 * Example function. Blink LED according to node state
 * Leader       - 5Hz
 * Router       - 2Hz
 * Child        - 0.5Hz
 */
static void ExampleProcess(otInstance *aInstance)
{
    static int   aliveLEDcounter = 0;
    otDeviceRole devRole;
    static int   thrValue;

    devRole = otThreadGetDeviceRole(aInstance);

    if (sBlink == false && otPlatAlarmMilliGetNow() != 0)
    {
        sMsCounterInit = otPlatAlarmMilliGetNow();
        sBlink         = true;
    }

    sMsCounter = otPlatAlarmMilliGetNow() - sMsCounterInit;

    switch (devRole)
    {
    case OT_DEVICE_ROLE_LEADER:
        thrValue = LEADER_BLINK_TIME;
        break;

    case OT_DEVICE_ROLE_ROUTER:
        thrValue = ROUTER_BLINK_TIME;
        break;

    case OT_DEVICE_ROLE_CHILD:
        thrValue = CHILD_BLINK_TIME;
        break;

    default:
        thrValue = 0x00;
    }

    if ((thrValue != 0x00) && (sMsCounter >= thrValue))
    {
        hw_gpio_toggle(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        sMsCounterInit = otPlatAlarmMilliGetNow();
    }

    if (thrValue == 0)
    {
        // No specific role, let's generate 'alive blink'
        // to inform that we are running.
        // Loop counter is used to run even if timers
        // are not initialized yet.
        aliveLEDcounter++;

        if (aliveLEDcounter > ALIVE_LED_PERIOD)
        {
            aliveLEDcounter = 0;
            hw_gpio_set_active(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        }

        if (aliveLEDcounter > ALIVE_LED_DUTY)
        {
            hw_gpio_set_inactive(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        }
    }
}

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    // Initialize Random number generator
    da15000RandomInit();
    // Initialize Alarm
    da15000AlarmInit();
    // Initialize Radio
    da15000RadioInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && (OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_JOINER)
static sys_clk_t ClkGet(void)
{
    sys_clk_t clk    = sysclk_RC16;
    uint32_t  hw_clk = hw_cpm_get_sysclk();

    switch (hw_clk)
    {
    case SYS_CLK_IS_RC16:
        clk = sysclk_RC16;
        break;

    case SYS_CLK_IS_XTAL16M:
        if (dg_configEXT_CRYSTAL_FREQ == EXT_CRYSTAL_IS_16M)
        {
            clk = sysclk_XTAL16M;
        }
        else
        {
            clk = sysclk_XTAL32M;
        }

        break;

    case SYS_CLK_IS_PLL:
        if (hw_cpm_get_pll_divider_status() == 1)
        {
            clk = sysclk_PLL48;
        }
        else
        {
            clk = sysclk_PLL96;
        }

        break;

    case SYS_CLK_IS_LP:

        // fall-through

    default:
        ASSERT_WARNING(0);
        break;
    }

    return clk;
}

static void ClkSet(sys_clk_t clock)
{
    switch (clock)
    {
    case sysclk_XTAL16M:
        if (!hw_cpm_check_xtal16m_status()) // XTAL16M disabled
        {
            hw_cpm_enable_xtal16m(); // Enable XTAL16M
        }

        hw_cpm_set_sysclk(SYS_CLK_IS_XTAL16M); // Set XTAL16 as sys_clk
        hw_watchdog_unfreeze();                // Start watchdog

        while (!hw_cpm_is_xtal16m_started())
            ; // Block until XTAL16M starts

        hw_qspi_set_div(HW_QSPI_DIV_1);
        hw_watchdog_freeze(); // Stop watchdog
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);
        break;

    case sysclk_PLL48:
        if (hw_cpm_is_pll_locked() == 0)
        {
            hw_cpm_pll_sys_on(); // Turn on PLL
        }

        hw_cpm_enable_pll_divider(); // Enable divider (div by 2)
        hw_qspi_set_div(HW_QSPI_DIV_1);
        hw_cpm_set_sysclk(SYS_CLK_IS_PLL);
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);
        break;

    case sysclk_PLL96:
        if (hw_cpm_is_pll_locked() == 0)
        {
            hw_cpm_pll_sys_on(); // Turn on PLL
        }

        hw_cpm_disable_pll_divider(); // Disable divider (div by 1)
        hw_qspi_set_div(HW_QSPI_DIV_2);
        hw_cpm_set_sysclk(SYS_CLK_IS_PLL);
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);
        break;

    default:
        break;
    }
}

static void ClkChange(sys_clk_t lastClock, sys_clk_t newClock)
{
    if (ClkGet() == lastClock)
    {
        ClkSet(newClock);
    }
}

static void StateChangedCallback(uint32_t aFlags, void *aContext)
{
    if ((aFlags & OT_CHANGED_COMMISSIONER_STATE) != 0)
    {
        if (otCommissionerGetState(sInstance) == OT_COMMISSIONER_STATE_ACTIVE)
        {
            ClkChange(sysclk_XTAL16M, sysclk_PLL96);
        }
        else
        {
            ClkChange(sysclk_PLL96, sysclk_XTAL16M);
        }
    }

    if ((aFlags & OT_CHANGED_JOINER_STATE) != 0)
    {
        if (otJoinerGetState(sInstance) != OT_JOINER_STATE_IDLE)
        {
            ClkChange(sysclk_XTAL16M, sysclk_PLL96);
        }
        else
        {
            ClkChange(sysclk_PLL96, sysclk_XTAL16M);
        }
    }
}
#endif // (OPENTHREAD_MTD || OPENTHREAD_FTD) && (OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_JOINER)

void otSysProcessDrivers(otInstance *aInstance)
{
    if (sInstance == NULL)
    {
        sInstance = aInstance;
#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && (OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_JOINER)
        otSetStateChangedCallback(aInstance, StateChangedCallback, 0);
#endif // (OPENTHREAD_MTD || OPENTHREAD_FTD) && (OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_JOINER)
    }

    da15000UartProcess();
    da15000RadioProcess(aInstance);
    da15000AlarmProcess(aInstance);
    ExampleProcess(aInstance);
}
