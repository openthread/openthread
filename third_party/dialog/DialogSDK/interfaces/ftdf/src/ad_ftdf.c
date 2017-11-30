/**
 ****************************************************************************************
 *
 * @file ad_ftdf.c
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

#if dg_configRF_ADAPTER
#include "ad_rf.h"
#else
#include "hw_rf.h"
#endif

#include "sys_tcs.h"

#include "sdk_defs.h"
#include "ad_ftdf.h"
#include "internal.h"
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

PRIVILEGED_DATA ftdf_boolean_t explicit_sleep; /* = FTDF_FALSE; */
PRIVILEGED_DATA sleep_status_t sleep_status;

PRIVILEGED_DATA ftdf_ext_address_t u_ext_address;

static void ad_ftdf_sleep(void)
{
        ftdf_critical_var();
        ftdf_enter_critical();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go sleep
        ftdf_exit_critical();

        // don't go-go before I sleep
        while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_DOWN) == 0x0) {

        }
#if FTDF_DBG_BLOCK_SLEEP_ENABLE
        hw_gpio_set_inactive(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN);
#endif
}

void ad_ftdf_wake_up_internal(bool sync)
{
        if ( sleep_status != BLOCK_SLEEPING ) {
                return;
        }

        ftdf_critical_var();
        ftdf_enter_critical();
        /* Wake up power domains */
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go wake up
        ftdf_exit_critical();

        // don't go-go before I sleep
        while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0) {

        }
#if FTDF_DBG_BLOCK_SLEEP_ENABLE
        hw_gpio_configure_pin(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN,
                HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
#endif

        /* Power on and configure RF */
#if dg_configRF_ADAPTER
        ad_rf_request_on(false);
#else
        hw_rf_request_on(false);
#endif

        /* Apply trim values */
        sys_tcs_apply(tcs_ftdf);

#if dg_configRF_ADAPTER
        ad_rf_request_recommended_settings();
#else
        hw_rf_request_recommended_settings();
#endif


        if (sync) {
                ftdf_init_lmac( );
                sleep_status = BLOCK_ACTIVE;
        } else {
                /* Wake up FTDF block */
                sleep_status = BLOCK_WAKING_UP;
                ftdf_wakeup();
#if defined(FTDF_NO_CSL) && defined(FTDF_NO_TSCH)
                ad_ftdf_wake_up_ready();
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

void sleep_when_possible(uint8_t explicit_request, uint32_t sleep_time )
{
        ftdf_boolean_t block_sleep = FTDF_FALSE;

        if ((!AD_FTDF_SLEEP_WHEN_IDLE && !explicit_request) || sleep_status != BLOCK_ACTIVE) {
                return;
        }

        ftdf_usec_t us;


        ftdf_critical_var();
        ftdf_enter_critical();
#ifdef FTDF_PHY_API
        if (explicit_request && (REG_GETF(FTDF, FTDF_LMAC_CONTROL_STATUS_REG, LMACREADY4SLEEP) == 0)) {

                /* clear a previous interrupt */
                FTDF->FTDF_LMAC_CONTROL_DELTA_REG = REG_MSK(FTDF, FTDF_LMAC_CONTROL_DELTA_REG, LMACREADY4SLEEP_D);

                /* Enable (unmask) interrupt */
                REG_SET_BIT(FTDF, FTDF_LMAC_CONTROL_MASK_REG, LMACREADY4SLEEP_M);
                us = 0;
        } else {
                us = ftdf_can_sleep();
        }
#else
        us = ftdf_can_sleep();
#endif
        /* Try to sleep as much as needed (if sleep_time is 0, then sleep as much as possible).
         * Otherwise, sleep as much as possible. */
        if ( explicit_request == FTDF_TRUE ) {
                if (sleep_time && us > sleep_time)
                        us = sleep_time;
        }

        if (us > ( WUP_LATENCY / 1000000) + AD_FTDF_SLEEP_COMPENSATION) {
                // Subtract sleep compensation from the sleep time, compensating for delays
                us -= AD_FTDF_SLEEP_COMPENSATION;

                block_sleep = ftdf_prepare_for_sleep(us);

                if (block_sleep) {
                        // FTDF ready to sleep, disable clocks
                        sleep_status = BLOCK_SLEEPING;
                        explicit_sleep = explicit_request;
#if FTDF_USE_SLEEP_DURING_BACKOFF
                        ftdf_sdb_fsm_sleep();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
#if dg_configUSE_FTDF_DDPHY == 1
                        ftdf_ddphy_save();
#endif /* dg_configUSE_FTDF_DDPHY */
                        ad_ftdf_sleep();
                } else {
#if FTDF_USE_SLEEP_DURING_BACKOFF
                        ftdf_sdb_fsm_abort_sleep();
#endif /* FTDF_USE_SLEEP_DURING_BACKOFF */
                }
        }
        ftdf_exit_critical();
        if (block_sleep) {
#if dg_configRF_ADAPTER
                ad_rf_request_off(false);
#else
                hw_rf_request_off(false);
#endif
        }
}

/**
 * @brief Initialization function of FTDF adapter
 */
void ad_ftdf_init(void)
{
        /* Wake up power domains */
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, FTDF_SLEEP);                 // go wake up
        GLOBAL_INT_RESTORE();

        // don't go-go before I sleep
        while (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP) == 0x0) {

        }

#if FTDF_DBG_BLOCK_SLEEP_ENABLE
        hw_gpio_configure_pin(FTDF_DBG_BLOCK_SLEEP_GPIO_PORT, FTDF_DBG_BLOCK_SLEEP_GPIO_PIN,
                HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
#endif
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_ENABLE);   // on
        REG_SETF(CRG_TOP, CLK_RADIO_REG, FTDF_MAC_DIV, 0);      // divide by 1
        GLOBAL_INT_RESTORE();

        /* Power on and configure RF */
#if dg_configRF_ADAPTER
        ad_rf_request_on(false);
#else
        hw_rf_request_on(false);
#endif

        /* Apply trim values */
        sys_tcs_apply(tcs_ftdf);

#if dg_configRF_ADAPTER
        ad_rf_request_recommended_settings();
#else
        hw_rf_request_recommended_settings();
#endif

        ftdf_set_sleep_attributes( AD_FTDF_LP_CLOCK_CYCLE, AD_FTDF_WUP_LATENCY );

#ifdef FTDF_PHY_API
        ad_ftdf_init_phy_api();
#else
        ad_ftdf_init_mac_api();
#endif
}

void ad_ftdf_sleep_cb(ftdf_usec_t sleep_time )
{
        sleep_when_possible( FTDF_TRUE, sleep_time );
}

#if FTDF_DBG_BUS_ENABLE
void ad_ftdf_dbg_bus_gpio_config(void)
{

#if FTDF_DBG_BUS_USE_PORT_4 == 1

        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_0, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_1, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_2, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_3, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_4, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_5, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_6, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);
        hw_gpio_set_pin_function(HW_GPIO_PORT_4, HW_GPIO_PIN_7, HW_GPIO_MODE_OUTPUT,
                HW_GPIO_FUNC_FTDF_DIAG);


#else
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
#endif /* FTDF_DBG_BUS_USE_PORT_4 == 1 */
}
#endif /* FTDF_DBG_BUS_ENABLE */
#endif
