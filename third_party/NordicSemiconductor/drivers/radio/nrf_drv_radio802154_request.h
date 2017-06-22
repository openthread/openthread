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

#ifndef NRF_DRIVER_RADIO802154_REQUEST_H__
#define NRF_DRIVER_RADIO802154_REQUEST_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_driver_radio802154_request 802.15.4 driver request
 * @{
 * @ingroup nrf_driver_radio802154
 * @brief Requests to the driver triggered from the MAC layer.
 */

/**
 * @brief Initialize request module.
 */
void nrf_drv_radio802154_request_init(void);

/**
 * @brief Request entering sleep state.
 *
 * @retval  true   The driver will enter sleep state.
 * @retval  false  The driver cannot enter sleep state due to ongoing operation.
 */
bool nrf_drv_radio802154_request_sleep(void);

/**
 * @brief Request entering receive state.
 *
 * @param[in]  channel  Channel number used to receive data.
 *
 * @retval  true   The driver will enter receive state.
 * @retval  false  The driver cannot enter receive state due to ongoing operation.
 */
bool nrf_drv_radio802154_request_receive(uint8_t channel);

/**
 * @brief Request entering transmit state.
 *
 * @param[in]  p_data   Pointer to the frame to transmit.
 * @param[in]  channel  Channel number used to transmit the frame.
 * @param[in]  power    Transmitter power used to transmit the frame.
 *
 * @retval  true   The driver will enter transmit state.
 * @retval  false  The driver cannot enter transmit state due to ongoing operation.
 */
bool nrf_drv_radio802154_request_transmit(const uint8_t * p_data, uint8_t channel, int8_t power);

/**
 * @brief Request entering energy detection state.
 *
 * @param[in]  channel  Channel number used to perform energy detection procedure.
 * @param[in]  time_us  Requested duration of energy detection procedure.
 *
 * @retval  true   The driver will enter energy detection state.
 * @retval  false  The driver cannot enter energy detection state due to ongoing operation.
 */
bool nrf_drv_radio802154_request_energy_detection(uint8_t channel, uint32_t time_us);

/**
 * @brief Request the driver to free given buffer.
 *
 * @param[in]  p_data  Pointer to the buffer to free.
 */
void nrf_drv_radio802154_request_buffer_free(uint8_t * p_data);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_DRIVER_RADIO802154_REQUEST_H__
