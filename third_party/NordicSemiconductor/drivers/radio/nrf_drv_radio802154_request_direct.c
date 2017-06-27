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
 *   This file implements requests to the driver triggered directly by the MAC layer.
 *
 */

#include "nrf_drv_radio802154_request.h"

#include <stdbool.h>
#include <stdint.h>

#include "nrf_drv_radio802154_critical_section.h"
#include "nrf_drv_radio802154_fsm.h"
#include "hal/nrf_radio.h"

#include <cmsis/core_cmFunc.h>

void nrf_drv_radio802154_request_init(void)
{
    // Intentionally empty
}

bool nrf_drv_radio802154_request_sleep(void)
{
    bool result;
    nrf_drv_radio802154_critical_section_enter();

    result = nrf_drv_radio802154_fsm_sleep();

    nrf_drv_radio802154_critical_section_exit();
    return result;
}

bool nrf_drv_radio802154_request_receive(uint8_t channel)
{
    bool result;
    nrf_drv_radio802154_critical_section_enter();

    result = nrf_drv_radio802154_fsm_receive(channel);

    nrf_drv_radio802154_critical_section_exit();
    return result;
}

bool nrf_drv_radio802154_request_transmit(const uint8_t * p_data, uint8_t channel, int8_t power, bool cca)
{
    bool result;
    nrf_drv_radio802154_critical_section_enter();

    result = nrf_drv_radio802154_fsm_transmit(p_data, channel, power, cca);

    nrf_drv_radio802154_critical_section_exit();
    return result;
}

bool nrf_drv_radio802154_request_energy_detection(uint8_t channel, uint32_t time_us)
{
    bool result;
    nrf_drv_radio802154_critical_section_enter();

    result = nrf_drv_radio802154_fsm_energy_detection(channel, time_us);

    nrf_drv_radio802154_critical_section_exit();
    return result;
}

void nrf_drv_radio802154_request_buffer_free(uint8_t * p_data)
{
    nrf_drv_radio802154_critical_section_enter();

    nrf_drv_radio802154_fsm_notify_buffer_free((rx_buffer_t *)p_data);

    nrf_drv_radio802154_critical_section_exit();
}
