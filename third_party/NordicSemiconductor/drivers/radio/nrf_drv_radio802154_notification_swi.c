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
 *   This file implements notifications triggered by the nrf 802.15.4 radio driver via SWI.
 *
 */

#include "nrf_drv_radio802154_notification.h"

#include <stdbool.h>
#include <stdint.h>

#include "nrf_drv_radio802154_swi.h"
#include "raal/nrf_raal_api.h"

void nrf_drv_radio802154_notification_init(void)
{
    nrf_drv_radio802154_swi_init();
}

void nrf_drv_radio802154_notify_received(uint8_t * p_data, int8_t power, int8_t lqi)
{
    nrf_drv_radio802154_swi_notify_received(p_data, power, lqi);
}

void nrf_drv_radio802154_notify_transmitted(uint8_t * p_ack, int8_t power, int8_t lqi)
{
    nrf_drv_radio802154_swi_notify_transmitted(p_ack, power, lqi);
}

void nrf_drv_radio802154_notify_busy_channel(void)
{
    nrf_drv_radio802154_swi_notify_busy_channel();
}

void nrf_drv_radio802154_notify_energy_detected(uint8_t result)
{
    nrf_drv_radio802154_swi_notify_energy_detected(result);
}

void nrf_drv_radio802154_notify_cca(bool is_free)
{
    nrf_drv_radio802154_swi_notify_cca(is_free);
}

