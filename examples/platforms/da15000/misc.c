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


#include <openthread/config.h>

#include <openthread/openthread.h>
#include <openthread/platform/misc.h>

#include "hw_cpm.h"
#include "hw_watchdog.h"
#include "sdk_defs.h"

static void ClkChange(sys_clk_t lastClock, sys_clk_t newClock);
static sys_clk_t ClkGet(void);
static void ClkSet(sys_clk_t clock);

void otPlatReset(otInstance *aInstance)
{
    (void)aInstance;
    WDOG->WATCHDOG_CTRL_REG |= (1 << (WDOG_WATCHDOG_CTRL_REG_NMI_RST_Pos));
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    (void)aInstance;
    return OT_PLAT_RESET_REASON_POWER_ON;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}

void otPlatCommissioningClkChange(otInstance *aInstance, otClockSpeed aSpeed)
{
#if (OPENTHREAD_ENABLE_JOINER || (OPENTHREAD_FTD && OPENTHREAD_ENABLE_COMMISSIONER))

    switch (aSpeed)
    {
    case OT_CLOCK_HIGH:
        ClkChange(sysclk_XTAL16M, sysclk_PLL96);
        break;

    case OT_CLOCK_LOW:
        ClkChange(sysclk_PLL96, sysclk_XTAL16M);
        break;

    default:
        break;
    }

#endif
}

static sys_clk_t ClkGet(void)
{
    sys_clk_t clk = sysclk_RC16;
    uint32_t hw_clk = hw_cpm_get_sysclk();

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
            hw_cpm_enable_xtal16m();        // Enable XTAL16M
        }

        hw_cpm_set_sysclk(SYS_CLK_IS_XTAL16M);  // Set XTAL16 as sys_clk
        hw_watchdog_unfreeze();                 // Start watchdog

        while (!hw_cpm_is_xtal16m_started());   // Block until XTAL16M starts

        hw_watchdog_freeze();                   // Stop watchdog
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);
        break;

    case sysclk_PLL48:
        if (hw_cpm_is_pll_locked() == 0)
        {
            hw_watchdog_unfreeze();         // Start watchdog
            hw_cpm_pll_sys_on();            // Turn on PLL
            hw_watchdog_freeze();           // Stop watchdog
        }

        hw_cpm_enable_pll_divider();        // Enable divider (div by 2)
        hw_cpm_set_sysclk(SYS_CLK_IS_PLL);
        hw_cpm_set_hclk_div(0);
        hw_cpm_set_pclk_div(0);
        break;

    case sysclk_PLL96:
        if (hw_cpm_is_pll_locked() == 0)
        {
            hw_watchdog_unfreeze();         // Start watchdog
            hw_cpm_pll_sys_on();            // Turn on PLL
            hw_watchdog_freeze();           // Stop watchdog
        }

        hw_cpm_disable_pll_divider();       // Enable divider (div by 2)
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

