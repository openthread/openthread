
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

static bool active_verctor_priority_is_high(void)
{
    uint32_t active_vector_id = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    uint32_t active_priority = NVIC_GetPriority((IRQn_Type)active_vector_id);

    return active_priority >= RADIO_NOTIFICATION_SWI_PRIORITY;
}

void nrf_drv_radio802154_request_init(void)
{
    nrf_drv_radio802154_swi_init();
}

bool nrf_drv_radio802154_request_sleep(void)
{
    bool result = false;

    if (active_verctor_priority_is_high())
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

bool nrf_drv_radio802154_request_receive(uint8_t channel)
{
    bool result = false;

    if (active_verctor_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_receive(channel);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_receive(channel, &result);
    }

    return result;
}

bool nrf_drv_radio802154_request_transmit(const uint8_t * p_data, uint8_t channel, int8_t power)
{
    bool result = false;

    if (active_verctor_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_transmit(p_data, channel, power);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_transmit(p_data, channel, power, &result);
    }

    return result;
}

bool nrf_drv_radio802154_request_energy_detection(uint8_t channel, uint32_t time_us)
{
    bool result = false;

    if (active_verctor_priority_is_high())
    {
        nrf_drv_radio802154_critical_section_enter();
        result = nrf_drv_radio802154_fsm_energy_detection(channel, time_us);
        nrf_drv_radio802154_critical_section_exit();
    }
    else
    {
        nrf_drv_radio802154_swi_energy_detection(channel, time_us, &result);
    }

    return result;
}

void nrf_drv_radio802154_request_buffer_free(uint8_t * p_data)
{
    nrf_drv_radio802154_swi_buffer_free(p_data);
}
