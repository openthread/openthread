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
 * @brief Front End Module control internal functions.
 *
 */

#ifndef NRF_FEM_CONTROL_INTERNAL_H_
#define NRF_FEM_CONTROL_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_fem_control_api.h"

#define TIMER_CC_FEM     3  /**< Compare channel used by the module. */
#define TIMER_CC_CAPTURE 2  /**< Compare channel used for time capture. */

/**@brief Start timer used by the module. */
void nrf_fem_control_timer_start(void);

/**@brief Stop timer used by the module. */
void nrf_fem_control_timer_stop(void);

/**@brief Set timer CC target value.
 *
 * @param[in] target Traget time.
 */
void nrf_fem_control_timer_set(uint32_t target);

/**@brief Get current time from the timer.
 *
 * @retval Current timer time.
 */
uint32_t nrf_fem_control_timer_time_get(void);

/**@brief Initialize timer. */
void nrf_fem_control_timer_init(void);

/**@brief De-initialize timer. */
void nrf_fem_control_timer_deinit(void);

/**@brief Get configuration specific irq entry delay.
 *
 * @retval Delay value.
 */
uint32_t nrf_fem_control_irq_delay_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_CONTROL_INTERNAL_H_ */
