/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 * @addtogroup mbedtls_glue_aes
 * @{
 */
#ifndef MBEDTLS_AES_ALT_H
#define MBEDTLS_AES_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "../config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#define CC310_MBEDTLS_AES_CONTEXT_WORDS         (24)    //!< AES context size in words in nrf_cc310_mbedcrypto library.
#define VANILLA_MBEDTLS_AES_CONTEXT_WORDS       (70)    //!< AES context size in words in standard mbed TLS.
#define VANILLA_MBEDTLS_AES_XTS_CONTEXT_WORDS   (140)   //!< AES XTS context size in words in standard mbed TLS.

#if defined(MBEDTLS_AES_ALT)

#include <stdint.h>


/** @brief Typedef of mbedcrypto AES glue context.
 */
typedef struct mbedtls_aes_context
{
    union
    {
#if defined(CONFIG_CC310_MBEDTLS_AES_C)
        uint32_t buffer_cc310[CC310_MBEDTLS_AES_CONTEXT_WORDS];                //!< Array the size of an AES context in the nrf_cc310_mbedcrypto library.
#endif /* CONFIG_CC310_MBEDTLS_AES_C */
#if defined(CONFIG_VANILLA_MBEDTLS_AES_C)
        uint32_t buffer_vanilla_mbedtls[VANILLA_MBEDTLS_AES_CONTEXT_WORDS];    //!< Array the size of an AES context in vanilla mbed TLS.
#endif /* CONFIG_VANILLA_MBEDTLS_AES_C */
        uint32_t dummy;                                                        //!< Dummy value in case no backend is enabled.
    } buffer;                                                                  //!< Union with size of the largest enabled backend context.
    void* handle;   //!< Pointer to the function table in an initialized glue context.
} mbedtls_aes_context;


/** @brief Typedef of mbedcrypto glue AES XTS context
 */
typedef struct mbedtls_aes_xts_context
{
    union
    {
#if defined(CONFIG_VANILLA_MBEDTLS_CIPHER_MODE_XTS)
        uint32_t buffer_vanilla_mbedtls[VANILLA_MBEDTLS_AES_XTS_CONTEXT_WORDS];    //!< Array the size of the memory required for an AES XTS context in vanilla mbed TLS.
#endif /* CONFIG_VANILLA_MBEDTLS_CIPHER_MODE_XTS */
        uint32_t dummy;                                                            //!< Dummy value in case no backend is enabled.
    } buffer;                                                                      //!< Array with length equal to the size of the largest enabled backend.
    void* handle;   //!< Pointer to the function table in an initialized glue context.
} mbedtls_aes_xts_context;

#endif /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_ALT_H */

/** @} */
