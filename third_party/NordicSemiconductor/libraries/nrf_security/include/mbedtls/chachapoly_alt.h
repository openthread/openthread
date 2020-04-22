/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_CHACHAPOLY_ALT_H
#define MBEDTLS_CHACHAPOLY_ALT_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

#if defined(MBEDTLS_CHACHAPOLY_ALT)


#define MBEDTLS_CHACHAPOLY_KEY_SIZE_BYTES 32


typedef struct mbedtls_chachapoly_context
{
    unsigned char key[MBEDTLS_CHACHAPOLY_KEY_SIZE_BYTES];
}
mbedtls_chachapoly_context;

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


#endif /* MBEDTLS_CHACHAPOLY_ALT_H */
