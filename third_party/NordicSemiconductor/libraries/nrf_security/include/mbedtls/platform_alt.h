/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_PLATFORM_ALT_H
#define MBEDTLS_PLATFORM_ALT_H

#include <stddef.h>
#include <stdint.h>

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)

#define MBEDTLS_ERR_PLATFORM_SUCCESS                          (0)
#define MBEDTLS_ERR_PLATFORM_ERROR_PARAM_NULL                 (-0x7001)
#define MBEDTLS_ERR_PLATFORM_ERROR_INTERNAL                   (-0x7002)
#define MBEDTLS_ERR_PLATFORM_ERROR_RNG_TEST_FAILED            (-0x7003)
#define MBEDTLS_ERR_PLATFORM_ERROR_HW_VERSION_FAILED          (-0x7004)
#define MBEDTLS_ERR_PLATFORM_ERROR_PARAM_WRITE_FAILED         (-0x7005)
#define MBEDTLS_ERR_PLATFORM_ERROR_MUTEX_NOT_INITIALIZED      (-0x7016)
#define MBEDTLS_ERR_PLATFORM_ERROR_MUTEX_FAILED               (-0x7017)
#define MBEDTLS_ERR_PLATFORM_ERROR_ENTROPY_NOT_INITIALIZED    (-0x7018)
#define MBEDTLS_ERR_PLATFORM_ERROR_ENTROPY_TRNG_TOO_LONG      (-0x7019)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief   The platform context structure.
 *
 */
typedef struct {
	char dummy; /**< A placeholder member, as empty structs are not portable. */
}
mbedtls_platform_context;


/** @brief Function to initialize platform without rng support
 *
 * Call this function instead of mbedtls_platform_setup if RNG is not required.
 * e.g. to conserve code size of improve startup time.
 *
 * @note It is possible to run mbedtls_platform_setup after calling
 *       this API if RNG is suddenly required. Calling mbedtls_platform_teardown
 *       is not required to be used, in this case.
 *
 * @warning Only deterministic cryptographic is supported if this API is used
 * 			to initalize the HW.
 */
int mbedtls_platform_setup_no_rng(void);


#ifdef __cplusplus
}
#endif

#endif  /* MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT */

#endif  /* MBEDTLS_PLATFORM_ALT_H */
