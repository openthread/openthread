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
 *   This file implements critical sections used with requests by 802.15.4 driver.
 *
 */

#include "nrf_drv_radio802154_critical_section.h"

#include <assert.h>
#include <stdint.h>

#include "nrf_drv_radio802154_debug.h"
#include "nrf_drv_radio802154_fsm.h"
#include "hal/nrf_radio.h"
#include "raal/nrf_raal_api.h"

#include <cmsis/core_cmFunc.h>

#if RAAL_SOFTDEVICE
// When Softdevice is selected as arbiter critical sections should not be nested.
#define PREVENT_NESTED_CRIT_SECTIONS 1
#endif // RAAL_SOFTDEVICE

#if PREVENT_NESTED_CRIT_SECTIONS
/// Flag indicating if the driver entered critical section.
static volatile bool m_in_critical_section;
#endif // PREVENT_NESTED_CRIT_SECTIONS

void nrf_drv_radio802154_critical_section_enter(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_CRIT_SECT_ENTER);
    radio_state_t state;

#if PREVENT_NESTED_CRIT_SECTIONS
    __disable_irq();
    __DSB();
    __ISB();

    assert(!m_in_critical_section);
    m_in_critical_section = true;
#endif // PREVENT_NESTED_CRIT_SECTIONS

    nrf_raal_critical_section_enter();

    state = nrf_drv_radio802154_fsm_state_get();

    if (state != RADIO_STATE_WAITING_TIMESLOT &&
        state != RADIO_STATE_SLEEP)
    {
        NVIC_DisableIRQ(RADIO_IRQn);
#if !RAAL_SOFTDEVICE
        __DSB();
        __ISB();
#endif // !RAAL_SOFTDEVICE
    }

#if PREVENT_NESTED_CRIT_SECTIONS
    __enable_irq();
#endif // PREVENT_NESTED_CRIT_SECTIONS

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_CRIT_SECT_ENTER);
}

void nrf_drv_radio802154_critical_section_exit(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_CRIT_SECT_EXIT);

#if PREVENT_NESTED_CRIT_SECTIONS
    assert(m_in_critical_section);
    m_in_critical_section = false;
#endif // PREVENT_NESTED_CRIT_SECTIONS

    radio_state_t state;

    // RAAL critical section shall be exited before RADIO IRQ handler is enabled. In other case
    // RADIO IRQ handler may be called out of timeslot.
    nrf_raal_critical_section_exit();

    state = nrf_drv_radio802154_fsm_state_get();

    if (state != RADIO_STATE_WAITING_TIMESLOT &&
        state != RADIO_STATE_SLEEP)
    {
        NVIC_EnableIRQ(RADIO_IRQn);
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_CRIT_SECT_EXIT);
}

