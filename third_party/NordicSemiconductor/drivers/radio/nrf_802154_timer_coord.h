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
 * @brief Module that defines the Timer Coordinator interface.
 *
 */

#ifndef NRF_802154_TIMER_COORD_H_
#define NRF_802154_TIMER_COORD_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_timer_coord Timer Coordinator
 * @{
 * @ingroup nrf_802154
 * @brief The Timer Coordinator interface.
 *
 * Timer Coordinator is responsible for synchronizing and coordinating operations of the Low Power
 * timer that counts the absolute time and the High Precision timer that counts the time relative to
 * a timeslot.
 */

/**
 * @brief Initializes the Timer Coordinator module.
 *
 */
void nrf_802154_timer_coord_init(void);

/**
 * @brief Deinitializes the Timer Coordinator module.
 *
 */
void nrf_802154_timer_coord_uninit(void);

/**
 * @brief Starts the Timer Coordinator module.
 *
 * This function starts the HP timer and synchronizes it with the LP timer.
 *
 * Once started, Timer Coordinator resynchronizes automatically in a constant interval.
 */
void nrf_802154_timer_coord_start(void);

/**
 * @brief Stops the Timer Coordinator module.
 *
 * This function stops the HP timer.
 */
void nrf_802154_timer_coord_stop(void);

/**
 * @brief Prepares for getting a precise timestamp of the given event.
 *
 * @param[in]  event_addr  Address of the peripheral register corresponding to the event that
 *                         is to be time-stamped.
 */
void nrf_802154_timer_coord_timestamp_prepare(uint32_t event_addr);

/**
 * @brief Gets the timestamp of the recently prepared event.
 *
 * If the recently prepared event occurred a few times since the preparation, this function returns
 * the timestamp of the first occurrence.
 * If the requested event did not occur since the preparation or the HP timer is not synchronized,
 * this function returns false.
 *
 * @param[out]  p_timestamp  Precise absolute timestamp of the recently prepared event,
 *                           in microseconds (us).
 *
 * @retval true   Timestamp is available.
 * @retval false  Timestamp is unavailable.
 */
bool nrf_802154_timer_coord_timestamp_get(uint32_t * p_timestamp);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_TIMER_COORD_H_ */
