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
 * @brief This module defines the Clock Abstraction Layer for the 802.15.4 driver.
 *
 * Clock Abstraction Layer can be used by other modules to start and stop nRF52840 clocks.
 *
 * It is used by the Radio Scheduler (RSCH) to start the HF clock when entering continuous mode
 * and to stop the HF clock after exiting the continuous mode.
 *
 * It is also used by the standalone Low Power Timer Abstraction Layer implementation
 * to start the LF clock during the initialization.
 *
 */

#ifndef NRF_802154_CLOCK_H_
#define NRF_802154_CLOCK_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_802154_clock Clock Abstraction Layer for the 802.15.4 driver
 * @{
 * @ingroup nrf_802154_clock
 * @brief Clock Abstraction Layer interface for the 802.15.4 driver.
 *
 */

/**
 * @brief Initializes the clock driver.
 */
void nrf_802154_clock_init(void);

/**
 * @brief Deinitializes the clock driver.
 */
void nrf_802154_clock_deinit(void);

/**
 * @brief Starts the High Frequency Clock.
 *
 * This function is asynchronous, meant to request the ramp-up of the High Frequency Clock and exit.
 * When the High Frequency Clock is ready, @ref nrf_802154_hfclk_ready() is called.
 *
 */
void nrf_802154_clock_hfclk_start(void);

/**
 * @brief Stops the High Frequency Clock.
 */
void nrf_802154_clock_hfclk_stop(void);

/**
 * @brief Checks if the High Frequency Clock is running.
 *
 * @retval true  High Frequency Clock is running.
 * @retval false High Frequency Clock is not running.
 *
 */
bool nrf_802154_clock_hfclk_is_running(void);

/**
 * @brief Starts the Low Frequency Clock.
 *
 * This function is asynchronous, meant to request the ramp-up of the Low Frequency Clock and exit.
 * When the Low Frequency Clock is ready, @ref nrf_802154_lfclk_ready() is called.
 *
 */
void nrf_802154_clock_lfclk_start(void);

/**
 * @brief Stops the Low Frequency Clock.
 */
void nrf_802154_clock_lfclk_stop(void);

/**
 * @brief Checks if the Low Frequency Clock is running.
 *
 * @retval true  Low Frequency Clock is running.
 * @retval false Low Frequency Clock is not running.
 */
bool nrf_802154_clock_lfclk_is_running(void);

/**
 * @brief Callback function executed when the High Frequency Clock is ready.
 */
extern void nrf_802154_clock_hfclk_ready(void);

/**
 * @brief Callback function executed when the Low Frequency Clock is ready.
 */
extern void nrf_802154_clock_lfclk_ready(void);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_CLOCK_H_ */
