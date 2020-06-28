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

/**
 * @brief Module that defines the Low Power Timer Abstraction Layer for the 802.15.4 driver.
 *
 */

#ifndef NRF_802154_LP_TIMER_API_H_
#define NRF_802154_LP_TIMER_API_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_timer Low Power Timer Abstraction Layer for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_timer
 * @brief Low Power Timer Abstraction Layer interface for the 802.15.4 driver.
 *
 * The Low Power Timer Abstraction Layer is an abstraction layer of the timer that is meant
 * to be used by the nRF 802.15.4 driver. This timer is intended to provide low latency
 * (max. 100 microseconds) to allow the implementation of the following features in the driver code:
 * * Timing out when waiting for an ACK frame
 * * SIFS and LIFS
 * * CSMA/CA
 * * CSL
 * * Auto polling by rx-off-when-idle devices
 *
 * @note Most of the Low Power Timer Abstraction Layer API is not intended to be called directly
 *       by the 802.15.4 driver modules. This API is used by the Timer Scheduler module included
 *       in the driver. Other modules should use the Timer Scheduler API. The exception are
 *       initialization and deinitialization functions @ref nrf_802154_lp_timer_init() and
 *       @ref nrf_802154_lp_timer_deinit(), as well as critical section management
 *       @ref nrf_802154_lp_timer_critical_section_enter() and
 *       @ref nrf_802154_lp_timer_critical_section_exit(), as these functions are called from
 *       the nrf_802154_critical_section module and from the global initialization
 *       and deinitialization functions @ref nrf_802154_init() and @ref nrf_802154_deinit().
 */

/**
 * @brief Initializes the Timer.
 */
void nrf_802154_lp_timer_init(void);

/**
 * @brief Deinitializes the Timer.
 */
void nrf_802154_lp_timer_deinit(void);

/**
 * @brief Enters the critical section of the timer.
 *
 * In the critical section, the timer cannot execute the @ref nrf_802154_lp_timer_fired() function.
 *
 * @note The critical section cannot be nested.
 *
 */
void nrf_802154_lp_timer_critical_section_enter(void);

/**
 * @brief Exits the critical section of the timer.
 *
 * In the critical section, the timer cannot execute the @ref nrf_802154_lp_timer_fired() function.
 *
 * @note The critical section cannot be nested.
 */
void nrf_802154_lp_timer_critical_section_exit(void);

/**
 * @brief Gets the current time.
 *
 * @pre Before getting the current time, the timer must be initialized with
 * @ref nrf_802154_lp_timer_init(). This is the only requirement that must be met before using this
 * function.
 *
 * @returns Current time in microseconds.
 */
uint32_t nrf_802154_lp_timer_time_get(void);

/**
 * @brief Gets the granularity of the timer.
 *
 * This function can be used to round up or round down the time calculations.
 *
 * @returns Timer granularity in microseconds.
 */
uint32_t nrf_802154_lp_timer_granularity_get(void);

/**
 * @brief Starts a one-shot timer that expires at the specified time.
 *
 * This function starts a one-shot timer that will expire @p dt microseconds after @p t0 time.
 * If the timer is running when this function is called, the running timer is stopped
 * automatically.
 *
 * On timer expiration, the @ref nrf_802154_lp_timer_fired function will be called.
 * The timer stops automatically after the expiration.
 *
 * @param[in]  t0  Number of microseconds representing timer start time.
 * @param[in]  dt  Time of the timer expiration as time elapsed from @p t0, in microseconds.
 */
void nrf_802154_lp_timer_start(uint32_t t0, uint32_t dt);

/**
 * @brief Stops the currently running timer.
 */
void nrf_802154_lp_timer_stop(void);

/**
 * @brief Checks if the timer is currently running.
 *
 * @retval  true   Timer is running.
 * @retval  false  Timer is not running.
 */
bool nrf_802154_lp_timer_is_running(void);

/**
 * @brief Starts a one-shot synchronization timer that expires at the nearest possible timepoint.
 *
 * On timer expiration, the @ref nrf_802154_lp_timer_synchronized function is called and the
 * event returned by @ref nrf_802154_lp_timer_sync_event_get is triggered.
 *
 * @note @ref nrf_802154_lp_timer_synchronized may be called multiple times.
 */
void nrf_802154_lp_timer_sync_start_now(void);

/**
 * @brief Starts a one-shot synchronization timer that expires at the specified time.
 *
 * This function starts a one-shot synchronization timer that expires @p dt microseconds after
 * @p t0 time.
 *
 * On timer expiration, @ref nrf_802154_lp_timer_synchronized function is called and
 * the event returned by @ref nrf_802154_lp_timer_sync_event_get is triggered.
 *
 * @param[in]  t0  Number of microseconds that represents the timer start time.
 * @param[in]  dt  Time of the timer expiration as time elapsed from @p t0, in microseconds.
 */
void nrf_802154_lp_timer_sync_start_at(uint32_t t0, uint32_t dt);

/**
 * @brief Stops the currently running synchronization timer.
 */
void nrf_802154_lp_timer_sync_stop(void);

/**
 * @brief Gets the event used to synchronize this timer with the HP Timer.
 *
 * @returns  Address of the peripheral register corresponding to the event
 *           to be used for the timer synchronization.
 *
 */
uint32_t nrf_802154_lp_timer_sync_event_get(void);

/**
 * @brief Gets the timestamp of the synchronization event.
 *
 * @returns  Timestamp of the synchronization event.
 */
uint32_t nrf_802154_lp_timer_sync_time_get(void);

/**
 * @brief Callback function executed when the timer expires.
 */
extern void nrf_802154_lp_timer_fired(void);

/**
 * @brief Callback function executed when the synchronization timer expires.
 */
extern void nrf_802154_lp_timer_synchronized(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_LP_TIMER_API_H_ */
