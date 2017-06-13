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

#ifndef NRF_RAAL_CONFIG_H__
#define NRF_RAAL_CONFIG_H__

#include <nrf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_raal_config RAAL configuration
 * @{
 * @ingroup nrf_driver_radio802154
 * @brief Configuration of Radio Arbiter Abstraction Layer.
 */

/**
 * @def NRF_RAAL_MAX_CLEAN_UP_TIME_US
 *
 * Maximum time within radio driver needs to do any clean-up actions on RADIO peripheral
 * and stop using it completely.
 *
 */
#ifndef NRF_RAAL_MAX_CLEAN_UP_TIME_US
#define NRF_RAAL_MAX_CLEAN_UP_TIME_US  100
#endif

/**
 * @def NRF_RAAL_HFCLK_START
 *
 * Macro to request High Frequency Clock start. It may use external driver or OS function.
 *
 */
#define NRF_RAAL_HFCLK_START()                                                                     \
    do {                                                                                           \
        NRF_CLOCK->TASKS_HFCLKSTART = 1;                                                           \
                                                                                                   \
        while(NRF_CLOCK->HFCLKSTAT != (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) {}    \
    } while(0);

/**
 * @def NRF_RAAL_HFCLK_STOP
 *
 * Macro to release High Frequency Clock. It may use external driver or OS function.
 *
 */
#define NRF_RAAL_HFCLK_STOP()                                                                      \
    do {                                                                                           \
        NRF_CLOCK->TASKS_HFCLKSTOP = 1;                                                            \
    } while(0);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_RAAL_CONFIG_H__
