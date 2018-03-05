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
 * @file
 *   This file implements requests to the driver triggered by the MAC layer through SWI.
 *
 */

#include "nrf_802154_request.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_core.h"
#include "nrf_802154_critical_section.h"
#include "nrf_802154_debug.h"
#include "nrf_802154_rx_buffer.h"
#include "nrf_802154_swi.h"
#include "hal/nrf_radio.h"

#include <nrf.h>

#define REQUEST_FUNCTION(func_core, params_core, func_swi, params_swi)                             \
    bool result = false;                                                                           \
                                                                                                   \
    if (active_vector_priority_is_high())                                                          \
    {                                                                                              \
        if (nrf_802154_critical_section_enter())                                                   \
        {                                                                                          \
            result = func_core params_core;                                                        \
            nrf_802154_critical_section_exit();                                                    \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            result = false;                                                                        \
        }                                                                                          \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        func_swi params_swi;                                                                       \
    }                                                                                              \
                                                                                                   \
    return result;

#define REQUEST_FUNCTION_NO_ARGS(func_core, func_swi)                                              \
        REQUEST_FUNCTION(func_core, (), func_swi, (&result))

#define REQUEST_FUNCTION_1_ARG(func_core, func_swi, arg)                                           \
        REQUEST_FUNCTION(func_core, (arg), func_swi, (arg, &result))

#define REQUEST_FUNCTION_2_ARGS(func_core, func_swi, arg1, arg2)                                   \
        REQUEST_FUNCTION(func_core, (arg1, arg2), func_swi, (arg1, arg2, &result))

#define REQUEST_FUNCTION_3_ARGS(func_core, func_swi, arg1, arg2, arg3)                             \
        REQUEST_FUNCTION(func_core, (arg1, arg2, arg3), func_swi, (arg1, arg2, arg3, &result))

#define REQUEST_FUNCTION_4_ARGS(func_core, func_swi, arg1, arg2, arg3, arg4)                       \
        REQUEST_FUNCTION(func_core,                                                                \
                         (arg1, arg2, arg3, arg4),                                                 \
                         func_swi,                                                                 \
                         (arg1, arg2, arg3, arg4, &result))

#define REQUEST_FUNCTION_5_ARGS(func_core, func_swi, arg1, arg2, arg3, arg4, arg5)                 \
        REQUEST_FUNCTION(func_core,                                                                \
                         (arg1, arg2, arg3, arg4, arg5),                                           \
                         func_swi,                                                                 \
                         (arg1, arg2, arg3, arg4, arg5, &result))

/** Check if active vector priority is high enough to call requests directly.
 *
 *  @retval  true   Active vector priority is greater or equal to SWI priority.
 *  @retval  false  Active vector priority is lower than SWI priority.
 */
static bool active_vector_priority_is_high(void)
{

    return nrf_802154_critical_section_active_vector_priority_get() <= NRF_802154_SWI_PRIORITY;
}

void nrf_802154_request_init(void)
{
    nrf_802154_swi_init();
}

bool nrf_802154_request_sleep(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION_1_ARG(nrf_802154_core_sleep,
                           nrf_802154_swi_sleep,
                           term_lvl)
}

bool nrf_802154_request_receive(nrf_802154_term_t              term_lvl,
                                req_originator_t               req_orig,
                                nrf_802154_notification_func_t notify_function)
{
    REQUEST_FUNCTION_3_ARGS(nrf_802154_core_receive,
                            nrf_802154_swi_receive,
                            term_lvl,
                            req_orig,
                            notify_function)
}

bool nrf_802154_request_transmit(nrf_802154_term_t              term_lvl,
                                 req_originator_t               req_orig,
                                 const uint8_t                * p_data,
                                 bool                           cca,
                                 nrf_802154_notification_func_t notify_function)
{
    REQUEST_FUNCTION_5_ARGS(nrf_802154_core_transmit,
                            nrf_802154_swi_transmit,
                            term_lvl,
                            req_orig,
                            p_data,
                            cca,
                            notify_function)
}

bool nrf_802154_request_energy_detection(nrf_802154_term_t term_lvl,
                                         uint32_t          time_us)
{
    REQUEST_FUNCTION_2_ARGS(nrf_802154_core_energy_detection,
                            nrf_802154_swi_energy_detection,
                            term_lvl,
                            time_us)
}

bool nrf_802154_request_cca(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION_1_ARG(nrf_802154_core_cca,
                           nrf_802154_swi_cca,
                           term_lvl)
}

bool nrf_802154_request_continuous_carrier(nrf_802154_term_t term_lvl)
{
    REQUEST_FUNCTION_1_ARG(nrf_802154_core_continuous_carrier,
                           nrf_802154_swi_continuous_carrier,
                           term_lvl)
}

bool nrf_802154_request_buffer_free(uint8_t * p_data)
{
    REQUEST_FUNCTION_1_ARG(nrf_802154_core_notify_buffer_free,
                           nrf_802154_swi_buffer_free,
                           p_data)
}

bool nrf_802154_request_channel_update(void)
{
    REQUEST_FUNCTION_NO_ARGS(nrf_802154_core_channel_update,
                             nrf_802154_swi_channel_update)
}

bool nrf_802154_request_cca_cfg_update(void)
{
    REQUEST_FUNCTION_NO_ARGS(nrf_802154_core_cca_cfg_update,
                             nrf_802154_swi_cca_cfg_update)
}

