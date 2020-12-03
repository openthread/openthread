/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#include <stdint.h>
#include "nrf_cc3xx_platform_mutex.h"

/** @brief Alternate declaration of mbedtls mutex type
 * 
 * The RTOS may require allocation and freeing of resources
 * as the inner type of the mutex is represented by an 
 * RTOS-friendly void pointer.
 */
typedef nrf_cc3xx_platform_mutex_t mbedtls_threading_mutex_t;

#endif /* MBEDTLS_THREADING_ALT_H */
