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

#ifndef NRF_DRIVER_RADIO802154_SWI_H__
#define NRF_DRIVER_RADIO802154_SWI_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_driver_radio802154_swi 802.15.4 driver SWI management
 * @{
 * @ingroup nrf_driver_radio802154
 * @brief SWI manager for 802.15.4 driver.
 */

/**
 * @brief Initialize SWI module.
 */
void nrf_drv_radio802154_swi_init(void);

/**
 * @brief Notify next higher layer that a frame was received from SWI priority level.
 *
 * @param[in]  p_data  Array of bytes containing PSDU. First byte contains frame length, other contain the frame itself.
 * @param[in]  power   RSSI measured during the frame reception.
 * @param[in]  lqi     LQI indicating measured link quality during the frame reception.
 */
void nrf_drv_radio802154_swi_notify_received(uint8_t * p_data, int8_t power, int8_t lqi);

/**
 * @brief Notify next higher layer that a frame was transmitted from SWI priority level.
 *
 * @param[in]  p_ack  Pointer to buffer containing PSDU of ACK frame. NULL if ACK was not requested.
 * @param[in]  power  RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi    LQI of received frame of 0 if ACK was not requested.
 */
void nrf_drv_radio802154_swi_notify_transmitted(uint8_t * p_data, int8_t power, int8_t lqi);

/**
 * @brief Notify next higher layer that a frame was not transmitted from SWI priority level.
 */
void nrf_drv_radio802154_swi_notify_busy_channel(void);

/**
 * @brief Notify next higher layer that energy detection procedure ended from SWI priority level.
 *
 * @param[in]  result  Detected energy level.
 */
void nrf_drv_radio802154_swi_notify_energy_detected(int8_t result);

/**
 * @brief Request discarding of the timeslot from SWI priority level.
 *
 * @note This function should be called through notification module to prevent calling it from arbiter context.
 */
void nrf_drv_radio802154_swi_timeslot_exit(void);

/**
 * @brief Request entering sleep state from SWI priority.
 *
 * @param[out]  p_result  Result of entering sleep state.
 */
void nrf_drv_radio802154_swi_sleep(bool * p_result);

/**
 * @brief Request entering receive state from SWI priority.
 *
 * @param[in]   channel   Channel number used for receive procedure.
 * @param[out]  p_result  Result of entering receive state.
 */
void nrf_drv_radio802154_swi_receive(uint8_t channel, bool * p_result);

/**
 * @biref Request entering transmit state from SWI priority.
 *
 * @param[in]   p_data    Pointer to PSDU of the frame to transmit.
 * @param[in]   channel   Channel number used for requested transmission.
 * @param[in]   power     Transmitter power for requested transmission.
 * @param[out]  p_result  Result of entering transmit state.
 */
void nrf_drv_radio802154_swi_transmit(const uint8_t * p_data,
                                      uint8_t         channel,
                                      int8_t          power,
                                      bool          * p_result);

/**
 * @brief Request entering energy detection state from SWI priority.
 *
 * @param[in]   channel   Channel number used for the energy detection procedure.
 * @param[in]   time_us   Requested duration of energy detection procedure.
 * @param[out]  p_result  Result of entering energy detection state.
 */
void nrf_drv_radio802154_swi_energy_detection(uint8_t channel, uint32_t time_us, bool * p_result);

/**
 * @brief Notify FSM that given buffer is not used anymore and can be freed.
 *
 * @param[in]  p_data  Pointer to the buffer to free.
 */
void nrf_drv_radio802154_swi_buffer_free(uint8_t * p_data);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_DRIVER_RADIO802154_SWI_H__
