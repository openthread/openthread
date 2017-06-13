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
 *   This file implements buffer management for frames received by nRF 802.15.4 radio driver.
 *
 */

#include "nrf_drv_radio802154_rx_buffer.h"

#include <stddef.h>

#include "nrf_drv_radio802154_config.h"

#if RADIO_RX_BUFFERS < 1
#error Not enough rx buffers in the 802.15.4 radio driver.
#endif

#if defined ( __GNUC__ )

// Receive buffer (EasyDMA cannot address whole RAM. Place buffer in the special section.)
rx_buffer_t nrf_drv_radio802154_rx_buffers[RADIO_RX_BUFFERS]
                __attribute__ ((section ("nrf_radio_buffer.nrf_drv_radio802154_rx_buffers")));

#elif defined ( __ICCARM__ )

#pragma location="NRF_RADIO_BUFFER"
static rx_buffer_t nrf_drv_radio802154_rx_buffers[RADIO_RX_BUFFERS];

#endif

void nrf_drv_radio802154_rx_buffer_init(void)
{
    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        nrf_drv_radio802154_rx_buffers[i].free = true;
    }
}

rx_buffer_t * nrf_drv_radio802154_rx_buffer_free_find(void)
{
    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        if (nrf_drv_radio802154_rx_buffers[i].free)
        {
            return &nrf_drv_radio802154_rx_buffers[i];
        }
    }

    return NULL;
}
