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
 * @brief This module contains Finite State Machine of nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_DRV_RADIO802154_FSM_H_
#define NRF_DRV_RADIO802154_FSM_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_drv_radio802154_rx_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief States of nRF 802.15.4 driver.
 */
typedef enum
{
    // Sleep
    RADIO_STATE_DISABLING,         // Entering low power (DISABLED) mode
    RADIO_STATE_SLEEP,             // Low power (DISABLED) mode

    // Receive
    RADIO_STATE_WAITING_TIMESLOT,  // Radio is inactive due to denied time slot
    RADIO_STATE_WAITING_RX_FRAME,  // Waiting for frame in receiver mode
    RADIO_STATE_RX_HEADER,         // Received SFD, receiving MAC header
    RADIO_STATE_RX_FRAME,          // Received MAC destination address, receiving rest of the frame
    RADIO_STATE_TX_ACK,            // Received frame and transmitting ACK

    // Transmit
    RADIO_STATE_CCA,               // Performing CCA
    RADIO_STATE_TX_FRAME,          // Transmitting data frame (or beacon)
    RADIO_STATE_RX_ACK,            // Receiving ACK after transmitted frame

    // Energy Detection
    RADIO_STATE_ED,                // Performing Energy Detection procedure
} radio_state_t;

/**
 * @brief Initialize 802.15.4 driver FSM.
 */
void nrf_drv_radio802154_fsm_init(void);

/**
 * @brief Deinitialize 802.15.4 driver FSM.
 */
void nrf_drv_radio802154_fsm_deinit(void);

/**
 * @brief Get current state of nRF 802.15.4 driver.
 *
 * @return  Current state of the 802.15.4 driver.
 */
radio_state_t nrf_drv_radio802154_fsm_state_get(void);

/***************************************************************************************************
 * @section State machine transition requests
 **************************************************************************************************/

/**
 * @brief Request transition to SLEEP state.
 *
 * @note This function shall be called from critical section context. It shall not be interrupted
 *       by RADIO event handler or RAAL notification.
 *
 * @note This function should be called when the driver is in RECEIVE state.
 *
 * @retval  true   Entering SLEEP state succeeded.
 * @retval  false  Entering SLEEP state failed (driver is performing other procedure).
 */
bool nrf_drv_radio802154_fsm_sleep(void);

/**
 * @brief Request transition to RECEIVE state.
 *
 * @note This function shall be called from critical section context. It shall not be interrupted
 *       by RADIO event handler or RAAL notification.
 *
 * @note This function should be called when the driver is in SLEEP or TRANSMIT state.
 *
 * @param[in]  channel  Channel number that the radio should use to receive frames.
 *
 * @retval  true   Entering RECEIVE state succeeded.
 * @retval  false  Entering RECEIVE state failed (driver is performing other procedure).
 */
bool nrf_drv_radio802154_fsm_receive(uint8_t channel);

/**
 * @brief Request transition to TRANSMIT state.
 *
 * @note This function shall be called from critical section context. It shall not be interrupted
 *       by RADIO event handler or RAAL notification.
 *
 * @note This function should be called when the driver is in RECEIVE state.
 *
 * @param[in]  p_data   Pointer to a frame to transmit.
 * @param[in]  channel  Channel number that the radio should use to transmit the frame.
 * @param[in]  power    Transmission power.
 * @param[in]  cca      If the driver should perform CCA procedure before transmission.
 *
 * @retval  true   Entering TRANSMIT state succeeded.
 * @retval  false  Entering TRANSMIT state failed (driver is performing other procedure).
 */
bool nrf_drv_radio802154_fsm_transmit(const uint8_t * p_data, uint8_t channel, int8_t power, bool cca);

/**
 * @brief Request transition to ENERGY_DETECTION state.
 *
 * @note This function shall be called from critical section context. It shall not be interrupted
 *       by RADIO event handler or RAAL notification.
 *
 * @note This function should be called when the driver is in SLEEP or RECEIVE state. When Energy
 *       detection procedure is finished the driver will transit to RECEIVE state.
 *
 * @param[in]  channel  Channel number that the radio should use to perform energy detection.
 * @param[in]  time_us  Minimal time of energy detection procedure.
 *
 * @retval  true   Entering ENERGY_DETECTION state succeeded.
 * @retval  false  Entering ENERGY_DETECTION state failed (driver is performing other procedure).
 */
bool nrf_drv_radio802154_fsm_energy_detection(uint8_t channel, uint32_t time_us);

/***************************************************************************************************
 * @section State machine notifications
 **************************************************************************************************/

/**
 * @brief Notify the FSM that higher layer freed a frame buffer.
 *
 * When there were no free buffers available the FSM does not start receiver. If FSM receives this
 * notification in changes internal state to make sure receiver is started if requested.
 *
 * @note This function shall be called from critical section context. It shall not be interrupted
 *       by RADIO event handler or RAAL notification.
 *
 * @param[in]  p_buffer  Pointer to buffer that has been freed.
 */
void nrf_drv_radio802154_fsm_notify_buffer_free(rx_buffer_t * p_buffer);

#ifdef __cplusplus
}
#endif

#endif /* NRF_DRV_RADIO802154_FSM_H_ */

