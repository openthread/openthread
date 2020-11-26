/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef NRF_CC310_PLATFORM_H__
#define NRF_CC310_PLATFORM_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nrf_cc3xx_platform_defines.h"
#include "nrf_cc3xx_platform_abort.h"
#include "nrf_cc3xx_platform_mutex.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define nrf_cc310_platform_init nrf_cc3xx_platform_init

#define  nrf_cc310_platform_init_no_rng nrf_cc3xx_platform_init_no_rng

#define nrf_cc310_platform_deinit nrf_cc3xx_platform_deinit

#define nrf_cc310_platform_is_initialized nrf_cc3xx_platform_is_initialized

#define nrf_cc310_platform_rng_is_initialized nrf_cc3xx_platform_rng_is_initialized

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC310_PLATFORM_H__ */

/** @} */
