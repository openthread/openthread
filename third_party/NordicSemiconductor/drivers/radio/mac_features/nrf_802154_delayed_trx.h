/* Copyright (c) 2018, Nordic Semiconductor ASA
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

#ifndef NRF_802154_DELAYED_TRX_H__
#define NRF_802154_DELAYED_TRX_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_delayed_trx Delayed transmission and reception window features
 * @{
 * @ingroup nrf_802154
 * @brief Delayed transmission or receive window.
 *
 * This module implements delayed transmission and receive window features used in the CSL and TSCH
 * modes.
 */

/**
 * @brief Requests transmission of a frame at a given time.
 *
 * If the requested transmission is successful and the frame is transmitted, the
 * @ref nrf_802154_tx_started function is called. If the requested frame cannot be transmitted
 * at the given time, the @ref nrf_802154_transmit_failed function is called.
 *
 * @note The delayed transmission does not time out automatically when waiting for ACK.
 *       Waiting for ACK must be timed out by the next higher layer or the ACK timeout module.
 *       The ACK timeout timer must start when the @ref nrf_802154_tx_started function is called.
 *
 * @param[in]  p_data   Pointer to a buffer containing PHR and PSDU of the frame to be transmitted.
 * @param[in]  cca      If the driver is to perform the CCA procedure before the transmission.
 * @param[in]  t0       Base of delay time in microseconds.
 * @param[in]  dt       Delta of the delay time from @p t0 in microseconds.
 * @param[in]  channel  Number of the channel on which the frame is to be transmitted.
 */
bool nrf_802154_delayed_trx_transmit(const uint8_t * p_data,
                                     bool            cca,
                                     uint32_t        t0,
                                     uint32_t        dt,
                                     uint8_t         channel);

/**
 * @brief Cancels a transmission scheduled by a call to @ref nrf_802154_delayed_trx_transmit.
 *
 * This function does not cancel transmission if the transmission is already ongoing.
 *
 * @retval true     Successfully cancelled a scheduled transmission.
 * @retval false    No delayed transmission was scheduled.
 */
bool nrf_802154_delayed_trx_transmit_cancel(void);

/**
 *@}
 **/

/**
 * @brief Requests the reception of a frame at a given time.
 *
 * If the request is accepted and a frame is received during the defined time slot,
 * the @ref nrf_802154_received function is called. If the request is rejected due
 * to a denied timeslot request or the reception timeout expires,
 * the @ref nrf_802154_receive_failed function is called.
 *
 * @param[in]  t0       Base of delay time in microseconds.
 * @param[in]  dt       Delta of delay time from @p t0 in microseconds.
 * @param[in]  timeout  Reception timeout (counted from @p t0 + @p dt) in microseconds.
 * @param[in]  channel  Number of the channel on which the frame is to be received.
 */
bool nrf_802154_delayed_trx_receive(uint32_t t0,
                                    uint32_t dt,
                                    uint32_t timeout,
                                    uint8_t  channel);

/**
 * @brief Cancels a reception scheduled by a call to @ref nrf_802154_delayed_trx_receive.
 *
 * After a call to this function, no reception timeout event will be notified.
 *
 * @retval true     Successfully cancelled a scheduled transmission.
 * @retval false    No delayed reception was scheduled.
 */
bool nrf_802154_delayed_trx_receive_cancel(void);

/**
 * @brief Aborts an ongoing delayed reception procedure.
 *
 * @param[in]  term_lvl  Termination level set by the request to abort the ongoing operation.
 * @param[in]  req_orig  Module that originates this request.
 *
 * If the delayed transmission/reception procedures are not running during the call,
 * this function does nothing.
 *
 * @retval  true   Transmission/reception procedures have been stopped.
 * @retval  false  Transmission/reception procedures were not running.
 *
 */
bool nrf_802154_delayed_trx_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig);

/**
 * @brief Extends the timeout timer when the reception start is detected and there is not enough
 *        time left for a delayed RX operation.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame
 *                      that is being received.
 *
 * If the delayed transmission/reception procedures are not running during call,
 * this function does nothing.
 *
 */
void nrf_802154_delayed_trx_rx_started_hook(const uint8_t * p_frame);

/**
 *@}
 **/

#endif // NRF_802154_DELAYED_TRX_H__
