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

#ifndef NRF_802154_SWI_H__
#define NRF_802154_SWI_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154.h"
#include "nrf_802154_const.h"
#include "nrf_802154_notification.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_swi 802.15.4 driver SWI management
 * @{
 * @ingroup nrf_802154
 * @brief SWI manager for the 802.15.4 driver.
 */

/**
 * @brief Initializes the SWI module.
 */
void nrf_802154_swi_init(void);

/**
 * @brief Notifies the next higher layer that a frame was received.
 *
 * The notification is triggered from the SWI priority level.
 *
 * @param[in]  p_data  Pointer to a buffer that contains PHR and PSDU of the received frame.
 * @param[in]  power   RSSI measured during the frame reception.
 * @param[in]  lqi     LQI that indicates the measured link quality during the frame reception.
 */
void nrf_802154_swi_notify_received(uint8_t * p_data, int8_t power, uint8_t lqi);

/**
 * @brief Notifies the next higher layer that the reception of a frame failed.
 *
 * @param[in]  error  Error code that indicates reason of the failure.
 */
void nrf_802154_swi_notify_receive_failed(nrf_802154_rx_error_t error);

/**
 * @brief Notifies the next higher layer that a frame was transmitted
 *
 * The notification is triggered from the SWI priority level.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the transmitted frame.
 * @param[in]  p_ack    Pointer to a buffer that contains PHR and PSDU of ACK frame. NULL if ACK was
 *                      not requested.
 * @param[in]  power    RSSI of the received frame, or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of the received frame, or 0 if ACK was not requested.
 */
void nrf_802154_swi_notify_transmitted(const uint8_t * p_frame,
                                       uint8_t       * p_data,
                                       int8_t          power,
                                       uint8_t         lqi);

/**
 * @brief Notifies the next higher layer that a frame was not transmitted from the SWI priority
 * level.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame that failed
 *                      the transmission.
 * @param[in]  error    Reason of the transmission failure.
 */
void nrf_802154_swi_notify_transmit_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error);

/**
 * @brief Notifies the next higher layer that the energy detection procedure ended from
 * the SWI priority level.
 *
 * @param[in]  result  Detected energy level.
 */
void nrf_802154_swi_notify_energy_detected(uint8_t result);

/**
 * @brief Notifies the next higher layer that the energy detection procedure failed from
 * the SWI priority level.
 *
 * @param[in]  error  Reason of the energy detection failure.
 */
void nrf_802154_swi_notify_energy_detection_failed(nrf_802154_ed_error_t error);

/**
 * @brief Notifies the next higher layer that the Clear Channel Assessment (CCA) procedure ended.
 *
 * The notification is triggered from the SWI priority level.
 *
 * @param[in]  channel_free  If a free channel was detected.
 */
void nrf_802154_swi_notify_cca(bool channel_free);

/**
 * @brief Notifies the next higher layer that the Clear Channel Assessment (CCA) procedure failed.
 *
 * The notification is triggered from the SWI priority level.
 *
 * @param[in]  error  Reason of the CCA failure.
 */
void nrf_802154_swi_notify_cca_failed(nrf_802154_cca_error_t error);

/**
 * @brief Requests a stop of the HF clock.
 *
 * The notification is triggered from the SWI priority level.
 *
 * @note This function is to be called through notification module to prevent calling it from
 *       the arbiter context.
 */
void nrf_802154_swi_hfclk_stop(void);

/**
 * @brief Terminates the stopping of the HF clock.
 *
 * @note This function terminates the stopping of the HF clock only if it has not been performed
 * yet.
 */
void nrf_802154_swi_hfclk_stop_terminate(void);

/**
 * @brief Requests entering the @ref RADIO_STATE_SLEEP state from the SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering the sleep state.
 */
void nrf_802154_swi_sleep(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Requests entering the @ref RADIO_STATE_RX state from the SWI priority.
 *
 * @param[in]   term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]   req_orig         Module that originates this request.
 * @param[in]   notify_function  Function called to notify the status of the procedure. May be NULL.
 * @param[in]   notify_abort     If abort notification should be triggered automatically.
 * @param[out]  p_result         Result of entering the receive state.
 */
void nrf_802154_swi_receive(nrf_802154_term_t              term_lvl,
                            req_originator_t               req_orig,
                            nrf_802154_notification_func_t notify_function,
                            bool                           notify_abort,
                            bool                         * p_result);

/**
 * @biref Requests entering the @ref RADIO_STATE_TX state from the SWI priority.
 *
 * @param[in]   term_lvl         Termination level of this request. Selects procedures to abort.
 * @param[in]   req_orig         Module that originates this request.
 * @param[in]   p_data           Pointer to a buffer that contains PHR and PSDU of the frame to be
 *                               transmitted.
 * @param[in]   cca              If the driver should perform the CCA procedure before transmission.
 * @param[in]   immediate        If true, the driver schedules transmission immediately or never;
 *                               if false, the transmission may be postponed until TX preconditions
 *                               are met.
 * @param[in]   notify_function  Function called to notify the status of this procedure instead of
 *                               the default notification. If NULL, the default notification
 *                               is used.
 * @param[out]  p_result         Result of entering the transmit state.
 */
void nrf_802154_swi_transmit(nrf_802154_term_t              term_lvl,
                             req_originator_t               req_orig,
                             const uint8_t                * p_data,
                             bool                           cca,
                             bool                           immediate,
                             nrf_802154_notification_func_t notify_function,
                             bool                         * p_result);

/**
 * @brief Requests entering the @ref RADIO_STATE_ED state from the SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[in]   time_us   Requested duration of the energy detection procedure.
 * @param[out]  p_result  Result of entering the energy detection state.
 */
void nrf_802154_swi_energy_detection(nrf_802154_term_t term_lvl,
                                     uint32_t          time_us,
                                     bool            * p_result);

/**
 * @brief Requests entering the @ref RADIO_STATE_CCA state from the SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering the CCA state.
 */
void nrf_802154_swi_cca(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Requests entering the @ref RADIO_STATE_CONTINUOUS_CARRIER state from the SWI priority.
 *
 * @param[in]   term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[out]  p_result  Result of entering the continuous carrier state.
 */
void nrf_802154_swi_continuous_carrier(nrf_802154_term_t term_lvl, bool * p_result);

/**
 * @brief Notifies the core module that the given buffer is not used anymore and can be freed.
 *
 * @param[in]  p_data  Pointer to the buffer to be freed.
 */
void nrf_802154_swi_buffer_free(uint8_t * p_data, bool * p_result);

/**
 * @brief Notifies the core module that the next higher layer has requested a channel change.
 */
void nrf_802154_swi_channel_update(bool * p_result);

/**
 * @brief Notifies the core module that the next higher layer has requested a CCA configuration
 * change.
 */
void nrf_802154_swi_cca_cfg_update(bool * p_result);

/**
 * @brief Notifies the core module that the next higher layer requested the RSSI measurement.
 */
void nrf_802154_swi_rssi_measure(bool * p_result);

/**
 * @brief Gets the last RSSI measurement result from the core module.
 */
void nrf_802154_swi_rssi_measurement_get(int8_t * p_rssi, bool * p_result);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_802154_SWI_H__
