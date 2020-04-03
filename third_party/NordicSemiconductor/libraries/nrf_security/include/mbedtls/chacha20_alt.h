/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_CHACHA20_ALT_H
#define MBEDTLS_CHACHA20_ALT_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif


#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/*! The size of the ChaCha user-context in words. */
#define MBEDTLS_CHACHA_USER_CTX_SIZE_IN_WORDS         17
/*! The size of the ChaCha block in Bytes. */
#define MBEDTLS_CHACHA_BLOCK_SIZE_BYTES               64
/*! The size of the ChaCha block in Bytes. As defined in rfc7539 */
#define MBEDTLS_CHACHA_NONCE_SIZE_BYTES               12
/*! The size of the ChaCha key in Bytes. */
#define MBEDTLS_CHACHA_KEY_SIZE_BYTES                 32
/*! Internal type to identify 12 byte nonce */
#define MBEDTLS_CHACHA_NONCE_SIZE_12BYTE_TYPE         1

/*! The definition of the 12-Byte array of the nonce buffer. */
typedef uint8_t mbedtls_chacha_nonce[MBEDTLS_CHACHA_NONCE_SIZE_BYTES];

/*! The definition of the key buffer of the ChaCha engine. */
typedef uint8_t mbedtls_chacha_key[MBEDTLS_CHACHA_KEY_SIZE_BYTES];

#if defined(MBEDTLS_CHACHA20_ALT)

typedef struct
{
    uint32_t buf[MBEDTLS_CHACHA_USER_CTX_SIZE_IN_WORDS];
}
mbedtls_chacha20_context;

#endif

#ifdef __cplusplus
}
#endif


#endif /* MBEDTLS_CHACHA20_ALT_H */
