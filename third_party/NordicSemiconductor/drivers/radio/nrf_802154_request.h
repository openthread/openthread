/* Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_802154_REQUEST_H__
#define NRF_802154_REQUEST_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_request 802.15.4 driver request
 * @{
 * @ingroup nrf_802154
 * @brief Requests to the driver triggered from the MAC layer.
 */

/**
 * @brief Initializes the request module.
 */
void nrf_802154_request_init(void);

/**
 * @brief Requests entering the @ref RADIO_STATE_SLEEP state for the driver.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   The driver will enter sleep state.
 * @retval  false  The driver cannot enter the sleep state due to an ongoing operation.
 */
bool nrf_802154_request_sleep(nrf_802154_term_t term_lvl);

/**
 * @brief Requests entering the @ref RADIO_STATE_RX state for the driver.
 *
 * @param[in]  term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]  req_orig         Module that originates this request.
 * @param[in]  notify_function  Function called to notify the status of this procedure. May be NULL.
 * @param[in]  notify_abort     If the abort notification is to be triggered automatically.
 *
 * @retval  true   The driver will enter the receive state.
 * @retval  false  The driver cannot enter the receive state due to ongoing operation.
 */
bool nrf_802154_request_receive(nrf_802154_term_t              term_lvl,
                                req_originator_t               req_orig,
                                nrf_802154_notification_func_t notify_function,
                                bool                           notify_abort);

/**
 * @brief Request entering the @ref RADIO_STATE_TX state for the driver.
 *
 * @param[in]  term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]  req_orig         Module that originates this request.
 * @param[in]  p_data           Pointer to the frame to transmit.
 * @param[in]  cca              If the driver is to perform the CCA procedure before transmission.
 * @param[in]  immediate        If true, the driver schedules transmission immediately or never.
 *                              If false, the transmission can be postponed until the TX
 *                              preconditions are met.
 * @param[in]  notify_function  Function called to notify the status of this procedure. May be NULL.
 *
 * @retval  true   The driver will enter the transmit state.
 * @retval  false  The driver cannot enter the transmit state due to an ongoing operation.
 */
bool nrf_802154_request_transmit(nrf_802154_term_t              term_lvl,
                                 req_originator_t               req_orig,
                                 const uint8_t                * p_data,
                                 bool                           cca,
                                 bool                           immediate,
                                 nrf_802154_notification_func_t notify_function);

/**
 * @brief Requests entering the @ref RADIO_STATE_ED state.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[in]  time_us   Requested duration of the energy detection procedure.
 *
 * @retval  true   The driver will enter energy detection state.
 * @retval  false  The driver cannot enter the energy detection state due to an ongoing operation.
 */
bool nrf_802154_request_energy_detection(nrf_802154_term_t term_lvl, uint32_t time_us);

/**
 * @brief Requests entering the @ref RADIO_STATE_CCA state.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   The driver will enter the CCA state.
 * @retval  false  The driver cannot enter the CCA state due to an ongoing operation.
 */
bool nrf_802154_request_cca(nrf_802154_term_t term_lvl);

/**
 * @brief Requests entering the @ref RADIO_STATE_CONTINUOUS_CARRIER state.
 *
 * @param[in]  term_lvl  Termination level of this request. Selects procedures to abort.
 *
 * @retval  true   The driver will enter the continuous carrier state.
 * @retval  false  The driver cannot enter the continuous carrier state due to an ongoing operation.
 */
bool nrf_802154_request_continuous_carrier(nrf_802154_term_t term_lvl);

/**
 * @brief Requests the driver to free the given buffer.
 *
 * @param[in]  p_data  Pointer to the buffer to be freed.
 */
bool nrf_802154_request_buffer_free(uint8_t * p_data);

/**
 * @brief Requests the driver to update the channel number used by the RADIO peripheral.
 */
bool nrf_802154_request_channel_update(void);

/**
 * @brief Requests the driver to update the CCA configuration used by the RADIO peripheral.
 */
bool nrf_802154_request_cca_cfg_update(void);

/**
 * @brief Requests the RSSI measurement.
 */
bool nrf_802154_request_rssi_measure(void);

/**
 * @brief Requests getting the last RSSI measurement result.
 */
bool nrf_802154_request_rssi_measurement_get(int8_t * p_rssi);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_REQUEST_H__
