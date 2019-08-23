/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#ifndef NRF_802154_CSMA_CA_H__
#define NRF_802154_CSMA_CA_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_const.h"
#include "nrf_802154_types.h"

/**
 * @defgroup nrf_802154_csma_ca 802.15.4 driver CSMA-CA support
 * @{
 * @ingroup nrf_802154
 * @brief CSMA-CA procedure.
 */

/**
 * @brief Starts the CSMA-CA procedure for the transmission of a given frame.
 *
 * If the CSMA-CA procedure is successful and the frame is transmitted,
 * the @ref nrf_802154_tx_started() function is called. If the procedure failed and the frame
 * cannot be transmitted due to busy channel, the @ref nrf_802154_transmit_failed() function
 * is called.
 *
 * @note CSMA-CA does not time out automatically when waiting for ACK. Waiting for ACK must be
 *       timed out by the next layer. The ACK timeout timer must start when
 *       the @ref nrf_802154_tx_started() function is called.
 *
 * @param[in]  p_data    Pointer to a buffer the contains PHR and PSDU of the frame
 *                       that is to be transmitted.
 */
void nrf_802154_csma_ca_start(const uint8_t * p_data);

/**
 * @brief Aborts the ongoing CSMA-CA procedure.
 *
 * @note Do not call this function during the execution of @ref nrf_802154_csma_ca_start
 *       (from ISR with higher priority), as it will result in an unrecoverable runtime error.
 *
 * If CSMA-CA is not running during the call, this function does nothing and returns true.
 *
 * @param[in]     term_lvl  Termination level of this request. Selects procedures to abort.
 * @param[in]     req_orig  Module that originates this request.

 * @retval true   CSMA-CA procedure is not running anymore.
 * @retval false  CSMA-CA cannot be stopped because of a too low termination level.
 */
bool nrf_802154_csma_ca_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig);

/**
 * @brief Handles a TX failed event.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame
 *                      that was not transmitted.
 * @param[in]  error    Cause of failed transmission.
 *
 * @retval  true   TX failed event is to be propagated to the MAC layer.
 * @retval  false  TX failed event is not to be propagated to the MAC layer. It is handled
 *                 internally in the CSMA-CA module.
 */
bool nrf_802154_csma_ca_tx_failed_hook(const uint8_t * p_frame, nrf_802154_tx_error_t error);

/**
 * @brief Handles a TX started event.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame
 *                      that is being transmitted.
 *
 * @retval  true   TX started event is to be propagated to the MAC layer.
 * @retval  false  TX started event is not to be propagated to the MAC layer. It is handled
 *                 internally in the CSMA-CA module.
 */
bool nrf_802154_csma_ca_tx_started_hook(const uint8_t * p_frame);

/**
 *@}
 **/

#endif // NRF_802154_CSMA_CA_H__
