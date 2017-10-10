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
 *   This file implements internal functions of Front End Module control for standalone version of
 *   the nRF 802.15.4 radio driver.
 *
 */

#include "nrf_fem_control_internal.h"

#include "hal/nrf_timer.h"

static bool m_timer_started = false;  /**< Information if timer is running. */

void nrf_fem_control_timer_start(void)
{
    if (!m_timer_started)
    {
        nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_START);

        m_timer_started = true;
    }
}

void nrf_fem_control_timer_stop(void)
{
    nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_STOP);

    m_timer_started = false;
}

void nrf_fem_control_timer_set(uint32_t target)
{
    nrf_timer_cc_write(NRF_TIMER0, (nrf_timer_cc_channel_t)TIMER_CC_FEM, target);
}

uint32_t nrf_fem_control_timer_time_get(void)
{
    NRF_TIMER0->TASKS_CAPTURE[TIMER_CC_CAPTURE] = 1;
    return NRF_TIMER0->CC[TIMER_CC_CAPTURE];
}

void nrf_fem_control_timer_init(void)
{
    nrf_fem_control_timer_stop();
    nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CLEAR);

    nrf_timer_mode_set(NRF_TIMER0, NRF_TIMER_MODE_TIMER);
    nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
    nrf_timer_frequency_set(NRF_TIMER0, NRF_TIMER_FREQ_1MHz);
}

void nrf_fem_control_timer_deinit(void)
{
    nrf_fem_control_timer_stop();
    nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_CLEAR);
}

uint32_t nrf_fem_control_irq_delay_get(void)
{
    return 0;
}
