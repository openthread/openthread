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
 * @brief This module defines Radio Scheduler interface.
 *
 */

#ifndef NRF_802154_RSCH_H_
#define NRF_802154_RSCH_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rsch Radio Scheduler
 * @{
 * @ingroup nrf_802154
 * @brief Radio Scheduler interface.
 *
 * Radio Scheduler is responsible to schedule in time radio activities and preconditions. It is
 * expected that the Radio Scheduler module manages timings to meet requirements requested from the
 * core module.
 *
 * Examples of radio activity preconditions are: High-Frequency Clock running, radio arbiter (RAAL)
 * granted access to the RADIO peripheral.
 */

/**
 * @brief List of preconditions that have to be met before any radio activity.
 */
typedef enum
{
    RSCH_PREC_HFCLK,
    RSCH_PREC_RAAL,
    RSCH_PREC_CNT,
} rsch_prec_t;

/**
 * @brief Initialize Radio Scheduler.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 * Initialize Radio Scheduler.
 *
 * @note Radio Scheduler starts in inactive mode after initialization. In order to start radio activity
 *       @ref nrf_802154_rsch_continuous_mode_enter should be called.
 *
 */
void nrf_802154_rsch_init(void);

/**
 * @brief Uninitialize Radio Scheduler.
 *
 */
void nrf_802154_rsch_uninit(void);

/**
 * @brief Enter continuous radio mode.
 *
 * In the continuous mode the radio scheduler should try to satisfy all preconditions as long as
 * possible in order to give to the radio driver core as much radio time as possible while
 * disturbing the other activities as little as possible.
 *
 * @note The start of a timeslot will be indicated by @ref nrf_802154_rsch_prec_approved call.
 *
 */
void nrf_802154_rsch_continuous_mode_enter(void);

/**
 * @brief Exit continuous radio mode.
 *
 * In this mode the radio scheduler should not try to satisfy any of radio activity preconditions
 * unless it was requested by other functionalities of this module.
 *
 */
void nrf_802154_rsch_continuous_mode_exit(void);

/**
 * @brief Request timeslot for radio communication immediately.
 *
 * This function should be called only after @ref nrf_802154_rsch_prec_approved indicated the
 * start of a timeslot.
 *
 * @param[in] length_us  Requested radio timeslot length in microsecond.
 *
 * @retval true   The radio driver now has exclusive access to the RADIO peripheral for the
 *                full length of the timeslot.
 * @retval false  Slot cannot be assigned due to other activities.
 *
 */
bool nrf_802154_rsch_timeslot_request(uint32_t length_us);

/**
 * @brief Request timeslot in the future.
 *
 * Request timeslot that should be granted in the future. Function parameters provides data when
 * the timeslot should start and how long should it last. When requested timeslot starts the
 * @ref nrf_802154_rsch_delayed_timeslot_started is called. If requested timeslot cannot be granted
 * with requested parameters, the @ref nrf_802154_rsch_delayed_timeslot_failed is called.
 *
 * @note Time parameters use the same units that are used in the Timer Scheduler module.
 *
 * @param[in]  t0      Base time of the timestamp of the timeslot start [us].
 * @param[in]  dt      Time delta between @p t0 and the timestamp of the timeslot start [us].
 * @param[in]  length  Requested radio timeslot length [us].
 *
 * @retval true   Requested timeslot has been scheduled.
 * @retval false  Requested timeslot cannot be scheduled and will not be granted.
 */
bool nrf_802154_rsch_delayed_timeslot_request(uint32_t t0, uint32_t dt, uint32_t length);

/**
 * @brief Check if the RSCH precondition is satisfied.
 *
 * @param[in]  prec    RSCH precondition to be checked.
 * 
 * @retval true   Precondition @p prec is currently granted.
 * @retval false  Precondition @p prec is not currently granted.
 */
bool nrf_802154_rsch_prec_is_approved(rsch_prec_t prec);

/**
 * @brief Get left time of currently granted timeslot [us].
 *
 * @returns  Number of microseconds left in currently granted timeslot.
 */
uint32_t nrf_802154_rsch_timeslot_us_left_get(void);

/**
 * @brief The Radio Scheduler calls this function to notify the core
 *        about granting all preconditions.
 *
 * The radio driver now has exclusive access to the peripherals until
 * @ref nrf_802154_rsch_prec_denied is called.
 *
 * @note The end of the timeslot will be indicated by @ref nrf_802154_rsch_prec_denied function.
 *
 */
extern void nrf_802154_rsch_prec_approved(void);

/**
 * @brief The Radio Scheduler calls this function to notify the core
 *        about denial of one or all preconditions.
 *
 * Depending on the preconditions configuration, radio driver has NRF_RAAL_MAX_CLEAN_UP_TIME_US
 * microseconds to do any clean-up actions on RADIO peripheral and stop using it completely.
 * Thus the Radio Scheduler has to call this function NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds
 * before the timeslot is finished.
 *
 * If the Radio Scheduler is in the continuous mode, the next timeslot will be indicated again by
 * the @ref nrf_802154_rsch_prec_approved.
 *
 * @note Because the radio driver core needs to stop any operation on the RADIO peripheral within
 *       NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds, this function should be called with high
 *       interrupt priority level to avoid unwanted delays.
 * 
 * @note This function may be called after @ref nrf_802154_rsch_continuous_mode_exit is called.
 *
 */
extern void nrf_802154_rsch_prec_denied(void);

/**
 * @brief Notification that previously requested delayed timeslot has started just now.
 */
extern void nrf_802154_rsch_delayed_timeslot_started(void);

/**
 * @brief Notification that previously requested delayed timeslot cannot be started.
 *
 * This function may be called when any of radio activity precondition is not satisfied at the
 * time when the timeslot should start.
 */
extern void nrf_802154_rsch_delayed_timeslot_failed(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_RSCH_H_ */
