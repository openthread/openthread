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
 *   This file implements critical sections used with requests by 802.15.4 driver.
 *
 */

#include "nrf_802154_critical_section.h"

#include <assert.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_debug.h"
#include "hal/nrf_radio.h"
#include "platform/timer/nrf_802154_timer.h"
#include "raal/nrf_raal_api.h"

#include <nrf.h>

#define CMSIS_IRQ_NUM_VECTACTIVE_DIFF 16

#define NESTED_CRITICAL_SECTION_ALLOWED_PRIORITY_NONE (-1)

static volatile uint8_t m_nested_critical_section_counter;           ///< Counter of nested critical sections
static volatile int8_t  m_nested_critical_section_allowed_priority;  ///< Indicator if nested critical sections are currently allowed

/** @brief Enter critical section for RADIO peripheral
 *
 * @note RADIO peripheral registers (and NVIC) are modified only when timeslot is granted for the
 *       802.15.4 driver.
 */
static void radio_critical_section_enter(void)
{
    if (nrf_raal_timeslot_is_granted())
    {
        NVIC_DisableIRQ(RADIO_IRQn);
        __DSB();
        __ISB();
    }
}

/** @brief Exit critical section for RADIO peripheral
 *
 * @note RADIO peripheral registers (and NVIC) are modified only when timeslot is granted for the
 *       802.15.4 driver.
 */
static void radio_critical_section_exit(void)
{
    if (nrf_raal_timeslot_is_granted())
    {
        NVIC_EnableIRQ(RADIO_IRQn);
    }
}

/** @brief Convert active priority value to int8_t type.
 *
 * @param[in]  active_priority  Active priority in uint32_t format
 *
 * @return Active_priority value in int8_t format.
 */
static int8_t active_priority_convert(uint32_t active_priority)
{
    return active_priority == UINT32_MAX ? INT8_MAX : (int8_t)active_priority;
}

/** @brief Check if active vector priority is equal to priority that allows nested crit sections.
 *
 * @retval true   Active vector priority allows nested critical sections.
 * @retval false  Active vector priority denies nested critical sections.
 */
static bool nested_critical_section_is_allowed_in_this_context(void)
{
    return m_nested_critical_section_allowed_priority ==
            active_priority_convert(
                    nrf_802154_critical_section_active_vector_priority_get());
}

static bool critical_section_enter(bool forced)
{
    bool    result = true;
    uint8_t cnt;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CRIT_SECT_ENTER);

    do
    {
        cnt = __LDREXB(&m_nested_critical_section_counter);

        assert(cnt < UINT8_MAX);

        if (!forced && cnt > 0 && !nested_critical_section_is_allowed_in_this_context())
        {
            __CLREX();
            result = false;
            break;
        }

        radio_critical_section_enter();
        nrf_raal_critical_section_enter();
    }
    while (__STREXB(cnt + 1, &m_nested_critical_section_counter));

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CRIT_SECT_ENTER);
    return result;
}

void nrf_802154_critical_section_init(void)
{
    m_nested_critical_section_counter          = 0;
    m_nested_critical_section_allowed_priority = NESTED_CRITICAL_SECTION_ALLOWED_PRIORITY_NONE;
}

bool nrf_802154_critical_section_enter(void)
{
    return critical_section_enter(false);
}

void nrf_802154_critical_section_forcefully_enter(void)
{
    bool critical_section_entered = critical_section_enter(true);
    assert(critical_section_entered);
}

void nrf_802154_critical_section_exit(void)
{
    uint8_t     cnt;
    static bool exiting_crit_sect;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_CRIT_SECT_EXIT);

    do
    {
        cnt = __LDREXB(&m_nested_critical_section_counter);

        assert(cnt > 0);

        if (cnt == 1)
        {
            assert(!exiting_crit_sect);
            exiting_crit_sect = true;

            // RAAL critical section shall be exited before RADIO IRQ handler is enabled. In other
            // case RADIO IRQ handler may be called out of timeslot.
            nrf_raal_critical_section_exit();
            radio_critical_section_exit();

            exiting_crit_sect = false;
        }
    }
    while (__STREXB(cnt - 1, &m_nested_critical_section_counter));

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_CRIT_SECT_EXIT);
}

void nrf_802154_critical_section_nesting_allow(void)
{
    assert(m_nested_critical_section_allowed_priority ==
            NESTED_CRITICAL_SECTION_ALLOWED_PRIORITY_NONE);

    m_nested_critical_section_allowed_priority = active_priority_convert(
            nrf_802154_critical_section_active_vector_priority_get());
}

void nrf_802154_critical_section_nesting_deny(void)
{
    assert(m_nested_critical_section_allowed_priority >= 0);

    m_nested_critical_section_allowed_priority = NESTED_CRITICAL_SECTION_ALLOWED_PRIORITY_NONE;
}

uint32_t nrf_802154_critical_section_active_vector_priority_get(void)
{
    uint32_t  active_vector_id = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    IRQn_Type irq_number;
    uint32_t  active_priority;

    // Check if this function is called from main thread.
    if (active_vector_id == 0)
    {
        return UINT32_MAX;
    }

    assert(active_vector_id >= CMSIS_IRQ_NUM_VECTACTIVE_DIFF);

    irq_number      = (IRQn_Type)(active_vector_id - CMSIS_IRQ_NUM_VECTACTIVE_DIFF);
    active_priority = NVIC_GetPriority(irq_number);

    return active_priority;
}

