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
 * @brief This module defines Radio Arbiter Abstraction Layer interface.
 *
 */

#ifndef NRF_RAAL_SOFTDEVICE_H_
#define NRF_RAAL_SOFTDEVICE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief RAAL Softdevice default parameters. */
#define NRF_RAAL_TIMESLOT_DEFAULT_LENGTH      6400
#define NRF_RAAL_TIMESLOT_DEFAULT_ALLOC_ITERS 5
#define NRF_RAAL_TIMESLOT_DEFAULT_SAFE_MARGIN 91
#define NRF_RAAL_TIMESLOT_DEFAULT_TIMEOUT     100000
#define NRF_RAAL_TIMESLOT_DEFAULT_MAX_LENGTH  120000000
#define NRF_RAAL_DEFAULT_LF_CLK_ACCURACY_PPM  25

/** @brief RAAL Softdevice configuration parameters. */
typedef struct
{
    uint32_t timeslot_length;        /**< Timeslot length requested by the module in microseconds. */
    uint32_t timeslot_timeout;       /**< Longest acceptable delay until the start of the requested timeslot in microseconds. */
    uint32_t timeslot_max_length;    /**< Maximum single timeslot length created by extension processing in microseconds. */
    uint16_t timeslot_alloc_iters;   /**< Maximum number of iteration of dividing timeslot_length by factor of 2 performed by arbiter. */
    uint16_t timeslot_safe_margin;   /**< Safe margin before timeslot is finished and nrf_raal_timeslot_ended should be called in microseconds. */
    uint16_t lf_clk_accuracy_ppm;    /**< Clock accuracy in ppm unit. */
} nrf_raal_softdevice_cfg_t;

/**
 * @brief Function used to inform RAAL client about Softdevice's SoC events.
 *
 */
void nrf_raal_softdevice_soc_evt_handler(uint32_t evt_id);

/**
 * @brief Function used to set non-default parameters of RAAL.
 *
 */
void nrf_raal_softdevice_config(const nrf_raal_softdevice_cfg_t * p_cfg);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_RAAL_SOFTDEVICE_H_ */
