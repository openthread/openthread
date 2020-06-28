/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_CCM_ALT_H
#define MBEDTLS_CCM_ALT_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/cipher.h"

#if defined (MBEDTLS_CCM_ALT)

#define MBEDTLS_ERR_CCM_BAD_INPUT      -0x000D /**< Bad input parameters to function. */
#define MBEDTLS_ERR_CCM_AUTH_FAILED    -0x000F /**< Authenticated decryption failed. */

/* The Size of the CCM context.*/
#define MBEDTLS_CCM_CONTEXT_SIZE_IN_WORDS (96)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          The CCM context-type definition. The CCM context is passed
 *                 to the APIs called.
 */
typedef struct {
    uint32_t buf[MBEDTLS_CCM_CONTEXT_SIZE_IN_WORDS];
}
mbedtls_ccm_context;

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CCM_ALT */

#endif /* MBEDTLS_CCM_ALT_H */
