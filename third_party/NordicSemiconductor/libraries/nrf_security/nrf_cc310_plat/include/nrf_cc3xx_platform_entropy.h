/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc3xx_platform_entropy nrf_cc3xx_platform entropy generation APIs
 * @ingroup nrf_cc3xx_platform
 * @{
 * @brief The nrf_cc3xx_platform_entropy APIs provides TRNG using Arm CC3xx
 *        hardware acceleration.
 */
#ifndef NRF_CC3XX_PLATFORM_ENTROPY_H__
#define NRF_CC3XX_PLATFORM_ENTROPY_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf_cc3xx_platform_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**@brief Function to generate entropy using Arm CryptoCell cc3xx
 *
 * This API corresponds to mbedtls_hardware_poll. It provides TRNG using
 * the Arm CryptoCell cc3xx hardware accelerator.
 *
 * @note This API is only usable if @ref nrf_cc3xx_platform_init was run
 *       prior to calling it.
 *
 * @param[out]	buffer  Pointer to buffer to hold the entropy data.
 * @param[in]	length  Length of the buffer to fill with entropy data.
 * @param[out]	olen    Pointer to variable that will hold the length of
 *                      generated entropy.
 *
 * @retval 0 on success
 * @return Any other error code returned from mbedtls_hardware_poll
 */
int nrf_cc3xx_platform_entropy_get(uint8_t *buffer,
                                   size_t length,
                                   size_t* olen);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC3XX_PLATFORM_ENTROPY_H__ */

/** @} */
