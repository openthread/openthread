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
 * @brief Module that defines the Radio Arbiter Abstraction Layer interface.
 *
 */

#ifndef NRF_RAAL_API_H_
#define NRF_RAAL_API_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_raal Radio Arbiter Abstraction Layer
 * @{
 * @ingroup nrf_802154
 * @brief The Radio Arbiter Abstraction Layer interface.
 */

/**
 * @brief Initializes Radio Arbiter Abstraction Layer client.
 *
 * This function must be called once, before any other function from this module.
 *
 * @note The arbiter starts in the inactive mode after the initialization.
 *       To start the radio activity, the @p nrf_raal_continuous_mode_enter method must be called.
 *
 */
void nrf_raal_init(void);

/**
 * @brief Deinitializes Radio Arbiter Abstraction Layer client.
 *
 */
void nrf_raal_uninit(void);

/**
 * @brief Puts the arbiter into the continuous radio mode.
 *
 * In this mode, the radio arbiter tries to create long continuous timeslots that will give
 * the radio driver as much radio time as possible while disturbing the other activities as little
 * as possible.
 *
 * @note The start of a timeslot is indicated by the @p nrf_raal_timeslot_started call.
 *
 */
void nrf_raal_continuous_mode_enter(void);

/**
 * @brief Moves the arbiter out of the continuous mode.
 *
 * In this mode, the radio arbiter does not extend or allocate any more timeslots for
 * the radio driver.
 *
 */
void nrf_raal_continuous_mode_exit(void);

/**
 * @brief Sends a confirmation to RAAL that the current part of the continuous timeslot is ended.
 *
 * The core cannot use the RADIO peripheral after this call until the timeslot is started again.
 */
void nrf_raal_continuous_ended(void);

/**
 * @brief Requests a timeslot for radio communication.
 *
 * This method is to be called only after @p nrf_raal_timeslot_started indicated the start
 * of a timeslot.
 *
 * @param[in] length_us  Requested radio timeslot length in microseconds.
 *
 * @retval true  Radio driver has now exclusive access to the RADIO peripheral for the
 *               full length of the timeslot.
 * @retval false Slot cannot be assigned due to other activities.
 *
 */
bool nrf_raal_timeslot_request(uint32_t length_us);

/**
 * @brief Gets the remaining time of the currently granted timeslot, in microseconds.
 *
 * @returns Number of microseconds left in the currently granted timeslot.
 */
uint32_t nrf_raal_timeslot_us_left_get(void);

/**
 * @brief Notifies the radio driver about the start of a timeslot. Called by the RAAL client.
 *
 * The radio driver has now exclusive access to the peripherals until @p nrf_raal_timeslot_ended
 * is called.
 *
 * @note The high frequency clock must be enabled when this function is called.
 * @note The end of the timeslot is indicated by the @p nrf_raal_timeslot_ended function.
 *
 */
extern void nrf_raal_timeslot_started(void);

/**
 * @brief Notifies the radio driver about the end of a timeslot. Called by the RAAL client.
 *
 * Depending on the RAAL client configuration, the radio driver has NRF_RAAL_MAX_CLEAN_UP_TIME_US
 * microseconds to do any clean-up actions on the RADIO peripheral and stop using it.
 * For this reason, the arbiter must call this function NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds
 * before timeslot is finished.
 *
 * If RAAL is in the continuous mode, the next timeslot is indicated again by
 * the @p nrf_raal_timeslot_started function.
 *
 * The radio driver will assume that the timeslot has been finished after
 * the @p nrf_raal_continuous_mode_exit call.
 *
 * @note Because the radio driver must stop any operation on the RADIO peripheral within
 *       NRF_RAAL_MAX_CLEAN_UP_TIME_US microseconds, this method is to be called with high
 *       interrupt priority level to avoid unwanted delays.
 *
 */
extern void nrf_raal_timeslot_ended(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_RAAL_API_H_ */
