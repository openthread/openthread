/* Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_FEM_CONTROL_CONFIG_H_
#define NRF_FEM_CONTROL_CONFIG_H_

#if ENABLE_FEM
#include "nrf_fem_config.h"
#endif // ENABLE_FEM

#include <stdint.h>
#include <stdbool.h>

#include "nrf_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @section Configuration
 */

#if ENABLE_FEM

/**
 * @brief Configures the PA and LNA device interface.
 *
 * This function sets device interface parameters for the PA/LNA module.
 * The module can then be used to control a power amplifier or a low noise amplifier (or both) through the given interface and resources.
 *
 * The function also sets the PPI and GPIOTE channels to be configured for the PA/LNA interface.
 *
 * @param[in] p_config Pointer to the interface parameters for the PA/LNA device.
 *
 * @retval   ::NRF_SUCCESS                 PA/LNA control successfully configured.
 * @retval   ::NRF_ERROR_NOT_SUPPORTED     PA/LNA is not available.
 *
 */
int32_t nrf_fem_interface_configuration_set(nrf_fem_interface_config_t const * const p_config);

/**
 * @brief Retrieves the configuration of PA and LNA device interface.
 *
 * This function gets device interface parameters for the PA/LNA module.
 * The module can then be used to control a power amplifier or a low noise amplifier (or both) through the given interface and resources.
 *
 *
 * @param[in] p_config Pointer to the interface parameters for the PA/LNA device to be populated.
 *
 * @retval   ::NRF_SUCCESS                 PA/LNA control successfully configured.
 * @retval   ::NRF_ERROR_NOT_SUPPORTED     PA/LNA is not available.
 *
 */
int32_t nrf_fem_interface_configuration_get(nrf_fem_interface_config_t * p_config);

#else // ENABLE_FEM

typedef void nrf_fem_interface_config_t;

static inline int32_t nrf_fem_interface_configuration_set(
    nrf_fem_interface_config_t const * const p_config)
{
    (void)p_config;
    return NRF_ERROR_NOT_SUPPORTED;
}

static inline int32_t nrf_fem_interface_configuration_get(nrf_fem_interface_config_t * p_config)
{
    (void)p_config;
    return NRF_ERROR_NOT_SUPPORTED;
}

#endif // ENABLE_FEM

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_CONTROL_CONFIG_H_ */
