/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_CC310_PLATFORM_MUTEX_H__
#define NRF_CC310_PLATFORM_MUTEX_H__

#include "nrf_cc3xx_platform_mutex.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NRF_CC310_PLATFORM_MUTEX_MASK_INVALID        NRF_CC3XX_PLATFORM_MUTEX_MASK_INVALID
#define NRF_CC310_PLATFORM_MUTEX_MASK_IS_VALID       NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_VALID
#define NRF_CC310_PLATFORM_MUTEX_MASK_IS_ALLOCATED   NRF_CC3XX_PLATFORM_MUTEX_MASK_IS_ALLOCATED

#define  nrf_cc310_platform_mutex_t nrf_cc3xx_platform_mutex_t

#define nrf_cc310_platform_mutex_init_fn_t nrf_cc3xx_platform_mutex_init_fn_t

#define nrf_cc310_platform_mutex_free_fn_t nrf_cc3xx_platform_mutex_free_fn_t

#define nrf_cc310_platform_mutex_free_fn_t nrf_cc3xx_platform_mutex_free_fn_t

#define nrf_cc310_platform_mutex_unlock_fn_t nrf_cc3xx_platform_mutex_unlock_fn_t

#define nrf_cc310_platform_mutex_apis_t nrf_cc3xx_platform_mutex_apis_t

#define nrf_cc310_platform_mutexes_t nrf_cc3xx_platform_mutexes_t

#define nrf_cc310_platform_set_mutexes nrf_cc3xx_platform_set_mutexes

#define nrf_cc310_platform_mutex_init nrf_cc3xx_platform_mutex_init

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC3XX_PLATFORM_MUTEX_H__ */

/** @} */
