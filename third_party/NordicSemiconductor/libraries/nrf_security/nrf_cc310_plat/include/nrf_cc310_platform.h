/**
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 * @defgroup nrf_cc310_platform nRF CC310 Platform library
 * @{
 * @brief nrf_cc310_platform library containing CC310
 * hardware initialization and entropy gathering APIs. The library also contains
 * APIs and companion source-files to setus RTOS dependent mutex and abort
 * functionality for the nrf_cc310_mbedcrypto library in Zephyr RTOS and FreeRTOS.
 * @}
 *
 * @defgroup nrf_cc310_platform_init nrf_cc310_platform initialization APIs
 * @ingroup nrf_cc310_platform
 * @{
 * @brief The nrf_cc310_platform APIs provides functions related to
 *        initialization of the Arm CC310 hardware accelerator for usage in
 *        nrf_cc310_platform and dependent libraries.
 */
#ifndef NRF_CC310_PLATFORM_H__
#define NRF_CC310_PLATFORM_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nrf_cc310_platform_defines.h"
#include "nrf_cc310_platform_abort.h"
#include "nrf_cc310_platform_mutex.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**@brief Function to initialize the nrf_cc310_platform with rng support
 *
 * @return Zero on success, otherwise a non-zero error code.
 */
int nrf_cc310_platform_init(void);


/**@brief Function to initialize the nrf_cc310_platform without rng support
 *
 * @return Zero on success, otherwise a non-zero error code.
 */
int nrf_cc310_platform_init_no_rng(void);


/** @brief Function to deintialize the nrf_cc310_platform
 *
 * @return Zero on success, otherwise a non-zero error code.
 */
int nrf_cc310_platform_deinit(void);


/** @brief Function to check if the nrf_cc310_platform is initialized
 *
 * @retval True if initialized, otherwise false.
 */
bool nrf_cc310_platform_is_initialized(void);


/** @brief Function to check if the nrf_cc310_platform is initialized
 *         with RNG support
 *
 * @retval True if RNG is initialized, otherwise false.
 */
bool nrf_cc310_platform_rng_is_initialized(void);


/** @brief ISR Function for processing of CC310 Interrupts.
 *         This CC310 interrupt service routine function should be called for
 *         interrupt processing.
 *         Either by placing this functions directly in the vector table or by
 *         calling it from the ISR in the OS.
 */
void CRYPTOCELL_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CC310_PLATFORM_H__ */
