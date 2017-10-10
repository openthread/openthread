/* Copyright (c) 2017, Nordic Semiconductor ASA
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
 *   This file implements common function for Front End Module control of the nRF 802.15.4 radio driver.
 *
 */

#include "nrf_fem_control_api.h"

#include "compiler_abstraction.h"
#include "nrf_fem_control_config.h"
#include "nrf_fem_control_internal.h"
#include "nrf.h"
#include "hal/nrf_gpio.h"
#include "hal/nrf_gpiote.h"
#include "hal/nrf_ppi.h"
#include "hal/nrf_radio.h"

static nrf_fem_control_cfg_t m_nrf_fem_control_cfg;     /**< FEM controller configuration. */
static volatile uint32_t     m_time_latch;              /**< Recently latched timer value. */

/**
 * @section GPIO control.
 */

/** Initialize GPIO according to configuration provided. */
static void gpio_init(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable)
    {
        nrf_gpio_cfg_output(m_nrf_fem_control_cfg.pa_cfg.gpio_pin);
        nrf_gpio_pin_write(m_nrf_fem_control_cfg.pa_cfg.gpio_pin,
                           !m_nrf_fem_control_cfg.pa_cfg.active_high);
    }

    if (m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_gpio_cfg_output(m_nrf_fem_control_cfg.lna_cfg.gpio_pin);
        nrf_gpio_pin_write(m_nrf_fem_control_cfg.lna_cfg.gpio_pin,
                           !m_nrf_fem_control_cfg.lna_cfg.active_high);
    }
}

/** Configure GPIOTE task. */
__STATIC_INLINE void gpiote_configure(uint8_t pin, nrf_gpiote_outinit_t init_val)
{
    nrf_gpiote_task_configure(m_nrf_fem_control_cfg.gpiote_ch_id,
                              pin,
                              NRF_GPIOTE_POLARITY_TOGGLE,
                              init_val);

    nrf_gpiote_task_enable(m_nrf_fem_control_cfg.gpiote_ch_id);
}

/**
 * @section PPI control.
 */

/** Initialize PPI according to configuration provided. */
static void ppi_init(void)
{
    nrf_ppi_channel_include_in_group((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set,
                                     (nrf_ppi_channel_group_t)m_nrf_fem_control_cfg.timer_ppi_grp);
    nrf_ppi_channel_include_in_group((nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr,
                                     (nrf_ppi_channel_group_t)m_nrf_fem_control_cfg.radio_ppi_grp);

    nrf_ppi_channel_and_fork_endpoint_setup(
        (nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_set,
        (uint32_t)(&NRF_TIMER0->EVENTS_COMPARE[TIMER_CC_FEM]),
        (uint32_t)(&NRF_GPIOTE->TASKS_OUT[m_nrf_fem_control_cfg.gpiote_ch_id]),
        (uint32_t)(&NRF_PPI->TASKS_CHG[m_nrf_fem_control_cfg.timer_ppi_grp].DIS));

    // Workaround for FTPAN-114, disable PPI to prevent second radio DISABLED event trigger.
    nrf_ppi_channel_and_fork_endpoint_setup(
        (nrf_ppi_channel_t)m_nrf_fem_control_cfg.ppi_ch_id_clr,
        (uint32_t)(&NRF_RADIO->EVENTS_DISABLED),
        (uint32_t)(&NRF_GPIOTE->TASKS_OUT[m_nrf_fem_control_cfg.gpiote_ch_id]),
        (uint32_t)(&NRF_PPI->TASKS_CHG[m_nrf_fem_control_cfg.radio_ppi_grp].DIS));
}

/** Enable PPI. */
__STATIC_INLINE void ppi_enable(void)
{
    NRF_PPI->CHENSET = (1 << m_nrf_fem_control_cfg.ppi_ch_id_set) |
                       (1 << m_nrf_fem_control_cfg.ppi_ch_id_clr);
}

/** Disable PPI. */
__STATIC_INLINE void ppi_disable(void)
{
    NRF_PPI->CHENCLR = (1 << m_nrf_fem_control_cfg.ppi_ch_id_set) |
                       (1 << m_nrf_fem_control_cfg.ppi_ch_id_clr);
}

/**
 * @section FEM API functions.
 */

void nrf_fem_control_cfg_set(const nrf_fem_control_cfg_t * p_cfg)
{
    m_nrf_fem_control_cfg = *p_cfg;

    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        gpio_init();
        ppi_init();
        nrf_fem_control_timer_init();
    }
    else
    {
        nrf_fem_control_timer_deinit();
    }
}

void nrf_fem_control_cfg_get(nrf_fem_control_cfg_t * p_cfg)
{
    *p_cfg = m_nrf_fem_control_cfg;
}

void nrf_fem_control_activate(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_fem_control_timer_start();
    }
}

void nrf_fem_control_deactivate(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        nrf_fem_control_timer_stop();
        nrf_gpiote_task_disable(m_nrf_fem_control_cfg.gpiote_ch_id);
        ppi_disable();
    }
}

void nrf_fem_control_time_latch(void)
{
    if (m_nrf_fem_control_cfg.pa_cfg.enable || m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        m_time_latch = nrf_fem_control_timer_time_get();
    }
}

void nrf_fem_control_pa_set(bool shorts_used)
{
    uint32_t target_time;

    if (m_nrf_fem_control_cfg.pa_cfg.enable)
    {
        gpiote_configure(m_nrf_fem_control_cfg.pa_cfg.gpio_pin,
                         (nrf_gpiote_outinit_t)!m_nrf_fem_control_cfg.pa_cfg.active_high);
        ppi_enable();

        uint32_t tifs = nrf_radio_ifs_get();

        target_time = tifs <= NRF_FEM_RADIO_TX_STARTUP_LATENCY_US ?
                      m_time_latch + NRF_FEM_RADIO_TX_STARTUP_LATENCY_US - NRF_FEM_PA_TIME_IN_ADVANCE :
                      m_time_latch + tifs - NRF_FEM_RADIO_TIFS_DRIFT_US - NRF_FEM_PA_TIME_IN_ADVANCE;

        if (shorts_used)
        {
            target_time -= nrf_fem_control_irq_delay_get();
        }

        nrf_fem_control_timer_set(target_time);
    }
}

void nrf_fem_control_lna_set(bool shorts_used)
{
    uint32_t target_time;

    if (m_nrf_fem_control_cfg.lna_cfg.enable)
    {
        gpiote_configure(m_nrf_fem_control_cfg.lna_cfg.gpio_pin,
                         (nrf_gpiote_outinit_t)!m_nrf_fem_control_cfg.lna_cfg.active_high);
        ppi_enable();

        target_time = m_time_latch + NRF_FEM_RADIO_RX_STARTUP_LATENCY_US -
                      NRF_FEM_LNA_TIME_IN_ADVANCE;

        if (shorts_used)
        {
            target_time -= nrf_fem_control_irq_delay_get();
        }

        nrf_fem_control_timer_set(target_time);
    }
}
