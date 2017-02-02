/**
 ****************************************************************************************
 *
 * @file ftdf.c
 *
 * @brief FTDF FreeRTOS Adapter
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ****************************************************************************************
 */
#ifdef CONFIG_USE_FTDF

#include <string.h>
#include <stdbool.h>
#ifndef FTDF_PHY_API
#include "osal.h"
#include "queue.h"

#include "sys_power_mgr.h"
#endif

#include "ad_rf.h"

#include "sys_tcs.h"

#include "sdk_defs.h"
#include "ad_ftdf.h"
#include "internal.h"
#include "regmap.h"
#if FTDF_DBG_BUS_ENABLE
#include "hw_gpio.h"
#endif /* FTDF_DBG_BUS_ENABLE */

#if FTDF_DBG_BLOCK_SLEEP_ENABLE
#include "hw_gpio.h"
#endif

#if dg_configUSE_FTDF_DDPHY == 1
#include "radio.h"
#endif

#define WUP_LATENCY  (AD_FTDF_LP_CLOCK_CYCLE * AD_FTDF_WUP_LATENCY)

#ifdef FTDF_PHY_API
#ifndef PRIVILEGED_DATA
#define PRIVILEGED_DATA __attribute__((section("privileged_data_zi")))
#endif
#ifndef INITIALIZED_PRIVILEGED_DATA
#define INITIALIZED_PRIVILEGED_DATA __attribute__((section("privileged_data_init")))
#endif
#endif

PRIVILEGED_DATA FTDF_Boolean explicit_sleep; /* = FTDF_FALSE; */
PRIVILEGED_DATA eSleepStatus sleep_status;

PRIVILEGED_DATA FTDF_ExtAddress uExtAddress;

static void ad_ftdf_sleep(void)
{

    FTDF_criticalVar();
    FTDF_enterCritical();
    REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go sleep
    FTDF_exitCritical();

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_DOWN) == 0x0)    // don't go-go before I sleep
    {
    }

#if FTDF_DBG_BLOCK_SLEEP_ENABLE
    hw_gpio_set_inactive(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN);
#endif
}

void ad_ftdf_wake_up_internal(bool sync)
{
    if (sleep_status != BLOCK_SLEEPING)
    {
        return;
    }

    FTDF_criticalVar();
    FTDF_enterCritical();
    /* Wake up power domains */
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go wake up
    FTDF_exitCritical();

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0)      // don't go-go before I sleep
    {
    }

#if FTDF_DBG_BLOCK_SLEEP_ENABLE
    hw_gpio_configure_pin(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
#endif

    /* Power on and configure RF */
    ad_rf_request_on(false);

    /* Apply trim values */
//        sys_tcs_apply(tcs_ftdf);

    ad_rf_request_recommended_settings();

    if (sync)
    {
        FTDF_initLmac();
        sleep_status = BLOCK_ACTIVE;
    }
    else
    {
        /* Wake up FTDF block */
        sleep_status = BLOCK_WAKING_UP;
        FTDF_wakeUp();
#if defined(FTDF_NO_CSL) && defined(FTDF_NO_TSCH)
        ad_ftdf_wakeUpReady();
#endif
    }

}

void ad_ftdf_wake_up_async(void)
{
    ad_ftdf_wake_up_internal(false);
}

void ad_ftdf_wake_up_sync(void)
{
    ad_ftdf_wake_up_internal(true);
}

void sleep_when_possible(uint8_t explicit_request, uint32_t sleepTime)
{
    FTDF_Boolean blockSleep = FTDF_FALSE;

    if ((!AD_FTDF_SLEEP_WHEN_IDLE && !explicit_request) || sleep_status != BLOCK_ACTIVE)
    {
        return;
    }

    FTDF_USec us;


    FTDF_criticalVar();
    FTDF_enterCritical();
#ifdef FTDF_PHY_API

    if (explicit_request && FTDF_GET_FIELD(ON_OFF_REGMAP_LMACREADY4SLEEP) == 0)
    {
        volatile uint32_t *lmacCtrlMask = FTDF_GET_REG_ADDR(ON_OFF_REGMAP_LMAC_CONTROL_MASK);
        volatile uint32_t *lmacReady4sleepEvent = FTDF_GET_FIELD_ADDR(ON_OFF_REGMAP_LMACREADY4SLEEP_D);

        /* clear a previous interrupt */
        *lmacReady4sleepEvent = MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_D;

        /* Enable (unmask) interrupt */
        *lmacCtrlMask |= MSK_F_FTDF_ON_OFF_REGMAP_LMACREADY4SLEEP_M;
        us = 0;
    }
    else
    {
        us = FTDF_canSleep();
    }

#else
    us = FTDF_canSleep();
#endif

    /* Try to sleep as much as needed (if sleepTime is 0, then sleep as much as possible).
     * Otherwise, sleep as much as possible. */
    if (explicit_request == FTDF_TRUE)
    {
        if (sleepTime && us > sleepTime)
        {
            us = sleepTime;
        }
    }

    if (us > (WUP_LATENCY / 1000000) + AD_FTDF_SLEEP_COMPENSATION)
    {
        // Subtract sleep compensation from the sleep time, compensating for delays
        us -= AD_FTDF_SLEEP_COMPENSATION;

        blockSleep = FTDF_prepareForSleep(us);

        if (blockSleep)
        {
            // FTDF ready to sleep, disable clocks
            sleep_status = BLOCK_SLEEPING;
            explicit_sleep = explicit_request;
#if FTDF_USE_SLEEP_DURING_BACKOFF
            FTDF_sdbFsmSleep();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#if dg_configUSE_FTDF_DDPHY == 1
            FTDF_ddphySave();
#endif /* dg_configUSE_FTDF_DDPHY */
            ad_ftdf_sleep();
        }
        else
        {
#if FTDF_USE_SLEEP_DURING_BACKOFF
            FTDF_sdbFsmAbortSleep();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
        }
    }

    FTDF_exitCritical();

    if (blockSleep)
    {
        ad_rf_request_off(false);
    }
}

/**
 * @brief Initialization function of FTDF adapter
 */
void ad_ftdf_init(void)
{
    /* Wake up power domains */
    REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go wake up

    while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0)      // don't go-go before I sleep
    {
    }

#if FTDF_DBG_BLOCK_SLEEP_ENABLE
    hw_gpio_configure_pin(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
#endif
    REG_SET_BIT(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_ENABLE);   // on
    REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_DIV, 0);      // divide by 1

    /* Power on and configure RF */
    ad_rf_request_on(false);

    /* Apply trim values */
    sys_tcs_apply(tcs_ftdf);

    ad_rf_request_recommended_settings();

    FTDF_setSleepAttributes(AD_FTDF_LP_CLOCK_CYCLE,
                            AD_FTDF_WUP_LATENCY);

#ifdef FTDF_PHY_API
    ad_ftdf_init_phy_api();
#else
    ad_ftdf_init_mac_api();
#endif
}

void ad_ftdf_sleepCb(FTDF_USec sleepTime)
{
    sleep_when_possible(FTDF_TRUE, sleepTime);


}

#if FTDF_DBG_BUS_ENABLE
void ad_ftdf_dbgBusGpioConfig(void)
{
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_4, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_5, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_6, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_7, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
#if FTDF_DBG_BUS_USE_SWDIO_PIN
    /*
     * Note that this pin has a conflict with SWD. Disable the debugger in order to use this
     * pin.
     */
    hw_gpio_set_pin_function(HW_GPIO_PORT_0, HW_GPIO_PIN_6, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
#endif
    hw_gpio_set_pin_function(HW_GPIO_PORT_0, HW_GPIO_PIN_7, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
#if FTDF_DBG_BUS_USE_GPIO_P1_3_P2_2
    /*
     * Note that these pins have a conflict with the pins used for UART by default. Configure
     * UART on different pins if you would like to use the pins below for diagnostics.
     */
    hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_3, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
    hw_gpio_set_pin_function(HW_GPIO_PORT_2, HW_GPIO_PIN_3, HW_GPIO_MODE_OUTPUT,
                             HW_GPIO_FUNC_FTDF_DIAG);
#endif
}
#endif /* FTDF_DBG_BUS_ENABLE */
#endif
