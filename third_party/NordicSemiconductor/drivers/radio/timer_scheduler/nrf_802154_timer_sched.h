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
 * @brief Module that provides the timer scheduling functionality for the 802.15.4 driver.
 *
 * Use the timer scheduler module to implement the strict timing features specified in the IEEE 802.15.4, like:
 * * CSL
 * * Timing out when waiting for ACK frames
 * * CSMA/CA
 * * Inter-frame spacing: SIFS, LIFS (AIFS is implemented without using the timer module)
 *
 */

#ifndef NRF_802154_TIMER_SCHED_H_
#define NRF_802154_TIMER_SCHED_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_timer_sched Timer Scheduler module for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_timer_sched
 * @brief The Timer Scheduler module for the 802.15.4 driver.
 *
 */

/**
 * @brief Type of function called when the timer expires.
 *
 * @param[inout]  p_context  Pointer to user-defined memory location. Can be NULL.
 */
typedef void (* nrf_802154_timer_callback_t)(void * p_context);

/**
 * @brief Type for the driver instance.
 */
typedef struct nrf_802154_timer_s nrf_802154_timer_t;

/**
 * @brief Structure containing timer data used by the timer module.
 */
struct nrf_802154_timer_s
{
    uint32_t                    t0;        ///< Base time of the timer, in microseconds.
    uint32_t                    dt;        ///< Timer expiration delta from @p t0, in microseconds.
    nrf_802154_timer_callback_t callback;  ///< Callback function called when timer expires.
    void                      * p_context; ///< User-defined context passed to the callback function.
    nrf_802154_timer_t        * p_next;    ///< Pointer to the next running timer.
};

/**
 * @brief Initializes the timer scheduler.
 */
void nrf_802154_timer_sched_init(void);

/**
 * @brief Deinitializes the timer scheduler.
 */
void nrf_802154_timer_sched_deinit(void);

/**
 * @brief Gets the current time.
 *
 * This function can be used to set the base time in the @ref nrf_802154_timer_t structure.
 *
 * @returns Current time in microseconds [us].
 */
uint32_t nrf_802154_timer_sched_time_get(void);

/**
 * @brief Gets the granularity of the timer that runs the timer scheduler.
 *
 * @returns Granularity of the timer, in microseconds [us].
 */
uint32_t nrf_802154_timer_sched_granularity_get(void);

/**
 * @brief Checks if the given time is in the future.
 *
 * @param[in]  now  Current time. @ref nrf_802154_timer_sched_time_get().
 * @param[in]  t0   Base of time compared with @p now.
 * @param[in]  dt   Time delta from @p t0 compared with @p now.
 *
 * @retval true   Given time @p t0 @p dt is in future (compared to given @p now).
 * @retval false  Given time @p t0 @p dt is not in future (compared to given @p now).
 */
bool nrf_802154_timer_sched_time_is_in_future(uint32_t now, uint32_t t0, uint32_t dt);

/**
 * @brief Gets timer time that remains to expiration.
 *
 * @param[in]  p_timer   Pointer to the timer to check the remaining time.
 *
 * @returns Remaining time in microseconds, or zero if the timer has already expired.
 *
 */
uint32_t nrf_802154_timer_sched_remaining_time_get(const nrf_802154_timer_t * p_timer);

/**
 * @brief Starts the given timer and adds it to the scheduler.
 *
 * @note Fields @c t0, @c dt, @c callback and @c p_context must be filled in @p p_timer before
 *       calling this function. The @c callback field cannot be NULL.
 *
 * @note Due to the timer granularity, the callback function cannot be called exactly
 *       at the specified time. Use @p round_up to specify if the given timer should expire before
 *       or after the time given in the @p p_timer structure. The @c dt field of the @p p_timer
 *       is updated with the rounded-up value.
 *
 * @param[inout]  p_timer   Pointer to the timer to be started and added to the scheduler.
 * @param[in]     round_up  True if the timer is to expire after the specified time.
 *                          False if it is to expire before the specified time.
 */
void nrf_802154_timer_sched_add(nrf_802154_timer_t * p_timer, bool round_up);

/**
 * @brief Stops the given timer and removes it from the scheduler.
 *
 * @param[in,out]   p_timer         Pointer to the timer to be stopped and removed
 *                                  from the scheduler.
 * @param[out]      p_was_running   Inform a caller if the timer was running.
 *                                  Pass NULL if irrelevant.
 */
void nrf_802154_timer_sched_remove(nrf_802154_timer_t * p_timer, bool * p_was_running);

/**
 * @brief Checks if the given timer is already scheduled.
 *
 * @param[in]  p_timer  Pointer to the timer to check.
 *
 * @retval true   Given timer is already scheduled.
 * @retval false  Given timer is not scheduled.
 */
bool nrf_802154_timer_sched_is_running(nrf_802154_timer_t * p_timer);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_TIMER_SCHED_H_ */
