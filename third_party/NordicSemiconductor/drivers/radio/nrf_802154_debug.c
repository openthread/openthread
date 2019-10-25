/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 *   This file implements debug helpers for the nRF 802.15.4 radio driver.
 *
 */

#include "nrf_802154_debug.h"

#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_ppi.h"

#if ENABLE_DEBUG_LOG
/// Buffer used to store debug log messages.
volatile uint32_t nrf_802154_debug_log_buffer[NRF_802154_DEBUG_LOG_BUFFER_LEN];
/// Index of the log buffer pointing to the element that should be filled with next log message.
volatile uint32_t nrf_802154_debug_log_ptr = 0;

#endif

#if ENABLE_DEBUG_GPIO
/**
 * @brief Initialize PPI to toggle GPIO pins on radio events.
 */
static void radio_event_gpio_toggle_init(void)
{
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_END);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_DISABLED);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_READY);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_FRAMESTART);
    nrf_gpio_cfg_output(PIN_DBG_RADIO_EVT_EDEND);

    nrf_gpiote_task_configure(GPIOTE_DBG_RADIO_EVT_END,
                              PIN_DBG_RADIO_EVT_END,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(GPIOTE_DBG_RADIO_EVT_DISABLED,
                              PIN_DBG_RADIO_EVT_DISABLED,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(GPIOTE_DBG_RADIO_EVT_READY,
                              PIN_DBG_RADIO_EVT_READY,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(GPIOTE_DBG_RADIO_EVT_FRAMESTART,
                              PIN_DBG_RADIO_EVT_FRAMESTART,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_configure(GPIOTE_DBG_RADIO_EVT_EDEND,
                              PIN_DBG_RADIO_EVT_EDEND,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              NRF_GPIOTE_INITIAL_VALUE_HIGH);

    nrf_gpiote_task_enable(GPIOTE_DBG_RADIO_EVT_END);
    nrf_gpiote_task_enable(GPIOTE_DBG_RADIO_EVT_DISABLED);
    nrf_gpiote_task_enable(GPIOTE_DBG_RADIO_EVT_READY);
    nrf_gpiote_task_enable(GPIOTE_DBG_RADIO_EVT_FRAMESTART);
    nrf_gpiote_task_enable(GPIOTE_DBG_RADIO_EVT_EDEND);

    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_END,
                                   (uint32_t)&NRF_RADIO->EVENTS_END,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_0));
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_DISABLED,
                                   (uint32_t)&NRF_RADIO->EVENTS_DISABLED,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_1));
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_READY,
                                   (uint32_t)&NRF_RADIO->EVENTS_READY,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_2));
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_FRAMESTART,
                                   (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_3));
    nrf_ppi_channel_endpoint_setup((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_EDEND,
                                   (uint32_t)&NRF_RADIO->EVENTS_EDEND,
                                   nrf_gpiote_task_addr_get(NRF_GPIOTE_TASKS_OUT_4));

    nrf_ppi_channel_enable((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_END);
    nrf_ppi_channel_enable((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_DISABLED);
    nrf_ppi_channel_enable((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_READY);
    nrf_ppi_channel_enable((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_FRAMESTART);
    nrf_ppi_channel_enable((nrf_ppi_channel_t)PPI_DBG_RADIO_EVT_EDEND);
}

/**
 * @brief Initialize GPIO to set it simulated arbiter events.
 */
static void raal_simulator_gpio_init(void)
{
#if RAAL_SIMULATOR
    nrf_gpio_cfg_output(PIN_DBG_TIMESLOT_ACTIVE);
    nrf_gpio_cfg_output(PIN_DBG_RAAL_CRITICAL_SECTION);
#endif
}

#endif // ENABLE_DEBUG_GPIO

void nrf_802154_debug_init(void)
{
#if ENABLE_DEBUG_GPIO
    radio_event_gpio_toggle_init();
    raal_simulator_gpio_init();
#endif // ENABLE_DEBUG_GPIO
}

#if ENABLE_DEBUG_ASSERT
void __assert_func(const char * file, int line, const char * func, const char * cond)
{
    (void)file;
    (void)line;
    (void)func;
    (void)cond;

#if defined(ENABLE_DEBUG_ASSERT_BKPT) && (ENABLE_DEBUG_ASSERT_BKPT != 0)
    __BKPT(0);
#endif
    __disable_irq();

    while (1)
        ;
}

void __aeabi_assert(const char * expr, const char * file, int line)
{
    (void)expr;
    (void)file;
    (void)line;

#if defined(ENABLE_DEBUG_ASSERT_BKPT) && (ENABLE_DEBUG_ASSERT_BKPT != 0)
    __BKPT(0);
#endif
    __disable_irq();

    while (1)
        ;
}

#endif // ENABLE_DEBUG_ASSERT
