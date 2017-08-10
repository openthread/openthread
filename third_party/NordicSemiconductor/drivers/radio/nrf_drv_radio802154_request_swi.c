
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
 *   This file implements requests to the driver triggered by the MAC layer through SWI.
 *
 */

#include "nrf_drv_radio802154_request.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_drv_radio802154_config.h"
#include "nrf_drv_radio802154_critical_section.h"
#include "nrf_drv_radio802154_debug.h"
#include "nrf_drv_radio802154_fsm.h"
#include "nrf_drv_radio802154_rx_buffer.h"
#include "nrf_drv_radio802154_swi.h"
#include "hal/nrf_radio.h"

#include <cmsis/core_cmFunc.h>
#include <cmsis/core_cm4.h>

#define CMSIS_IRQ_NUM_VECTACTIVE_DIFF 16

/** Check if active vector priority is high enough to call requests directly.
 *
 *  @retval  true   Active vector priority is greater or equal to SWI priority.
 *  @retval  false  Active vector priority is lower than SWI priority.
 */
static bool active_vector_priority_is_high(void)
{
    uint32_t  active_vector_id = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    IRQn_Type irq_number;
    uint32_t  active_priority;

    // Check if this function is called from main thread.
    if (active_vector_id == 0)
    {
        return false;
    }

    assert(active_vector_id >= CMSIS_IRQ_NUM_VECTACTIVE_DIFF);

    irq_number      = (IRQn_Type)(active_vector_id - CMSIS_IRQ_NUM_VECTACTIVE_DIFF);
    active_priority = NVIC_GetPriority(irq_number);

    return active_priority <= RADIO_NOTIFICATION_SWI_PRIORITY;
}

void nrf_drv_radio802154_request_init(void)
{
    nrf_drv_radio802154_swi_init();
}

bool nrf_drv_radio802154_request_sleep(void)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_sleep();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_sleep(&result);
    }

    return result;
}

bool nrf_drv_radio802154_request_receive(void)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_receive();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_receive(&result);
    }

    return result;
}

bool nrf_drv_radio802154_request_transmit(const uint8_t * p_data, bool cca)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_transmit(p_data, cca);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_transmit(p_data, cca, &result);
    }

    return result;
}

bool nrf_drv_radio802154_request_energy_detection(uint32_t time_us)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_energy_detection(time_us);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_energy_detection(time_us, &result);
    }

    return result;
}

bool nrf_drv_radio802154_request_cca(void)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_cca();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_cca(&result);
    }

    return result;
}

bool nrf_drv_radio802154_request_continuous_carrier(void)
{
    bool result = false;

    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_continuous_carrier();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_continuous_carrier(&result);
    }

    return result;
}

void nrf_drv_radio802154_request_buffer_free(uint8_t * p_data)
{
    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        nrf_drv_radio802154_fsm_notify_buffer_free((rx_buffer_t *)p_data);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_buffer_free(p_data);
    }
}

void nrf_drv_radio802154_request_channel_update(void)
{
    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        nrf_drv_radio802154_fsm_channel_update();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_channel_update();
    }
}

void nrf_drv_radio802154_request_cca_cfg_update(void)
{
    if (active_vector_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        nrf_drv_radio802154_fsm_cca_cfg_update();
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_cca_cfg_update();
    }
}

