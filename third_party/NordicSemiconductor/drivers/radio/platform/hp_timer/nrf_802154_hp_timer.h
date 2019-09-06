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

/**
 * @brief Module that defines API of High Precision Timer for the 802.15.4 driver.
 *
 */

#ifndef NRF_802154_HP_TIMER_H_
#define NRF_802154_HP_TIMER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_hp_timer High Precision Timer for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_hp_timer
 * @brief High Precision Timer for the 802.15.4 driver.
 *
 * The High Precision Timer is used only when the radio is in use. It is not
 * used when the radio is in the sleep mode or out of the RAAL timeslots.
 * This timer is meant to provide at least 1-microsecond precision. It is intended to be used
 * for precise frame timestamps or synchronous radio operations.
 *
 * @note The High Precision Timer is relative. To use it as an absolute timer,
 *       synchronize it with the Low Power Timer using the Timer Coordinator module.
 *
 */

/**
 * @brief Initializes the timer.
 */
void nrf_802154_hp_timer_init(void);

/**
 * @brief Deinitializes the timer.
 */
void nrf_802154_hp_timer_deinit(void);

/**
 * @brief Starts the timer.
 *
 * The timer starts counting when this command is called.
 */
void nrf_802154_hp_timer_start(void);

/**
 * @brief Stops the timer.
 *
 * The timer stops counting and enters the low power mode.
 */
void nrf_802154_hp_timer_stop(void);

/**
 * @brief Gets the value indicated by the timer right now.
 *
 * @note The returned value is relative to the @ref nrf_802154_hp_timer_start call time. It is not
 *       synchronized with the LP timer.
 *
 * @returns Current timer value in microseconds.
 *
 */
uint32_t nrf_802154_hp_timer_current_time_get(void);

/**
 * @brief Gets the task used to synchronize the timer with the LP timer.
 *
 * @returns  Address of the task.
 *
 */
uint32_t nrf_802154_hp_timer_sync_task_get(void);

/**
 * @brief Configures the timer to detect if the synchronization task was triggered.
 */
void nrf_802154_hp_timer_sync_prepare(void);

/**
 * @brief Gets the timestamp of the synchronization event.
 *
 * @param[out]  p_timestamp  Timestamp of the synchronization event.
 *
 * @retval true   Synchronization was performed and @p p_timestamp is valid.
 * @retval false  Synchronization was not performed. @p p_timestamp was not modified.
 *
 */
bool nrf_802154_hp_timer_sync_time_get(uint32_t * p_timestamp);

/**
 * @brief Gets the task used to make timestamp of an event.
 *
 * This function is to be used to configure PPI.
 * It configures the timer to detect if the returned task was triggered to return
 * a valid value by @ref nrf_802154_hp_timer_timestamp_get.
 *
 * @returns  Address of the task.
 */
uint32_t nrf_802154_hp_timer_timestamp_task_get(void);

/**
 * @brief Gets the timestamp of the last event.
 *
 * @returns Timestamp of the last event that triggered
 *          the @ref nrf_802154_hp_timer_timestamp_task_get task.
 */
uint32_t nrf_802154_hp_timer_timestamp_get(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_HP_TIMER_H_ */
