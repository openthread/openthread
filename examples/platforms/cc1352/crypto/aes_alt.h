/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MBEDTLS_AES_ALT_H
#define MBEDTLS_AES_ALT_H

#ifndef MBEDTLS_CONFIG_FILE
#include "cc1352-mbedtls-config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#ifdef MBEDTLS_AES_ALT

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t     magic;
    signed char key_idx;
} mbedtls_aes_context;

/**
 * @brief Initialize AES context
 *
 * @param [in,out] ctx  AES context to be initialized
 */
void mbedtls_aes_init(mbedtls_aes_context *ctx);

/**
 * @brief          Clear AES context
 *
 * @param [in,out] ctx  AES context to be cleared
 */
void mbedtls_aes_free(mbedtls_aes_context *ctx);

/**
 * @brief          AES key schedule (encryption)
 *
 * @param [in,out] ctx      AES context to be used
 * @param [in]     key      Encryption key
 * @param [in]     keybits  Must be 128
 *
 * @retval 0                                   If successful
 * @retval MBEDTLS_ERR_AES_INVALID_KEY_LENGTH  If keybits was not 128
 */
int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits);

/**
 * @brief          AES key schedule (decryption)
 *
 * @param [in,out] ctx      AES context to be used
 * @param [in]     key      Decryption key
 * @param [in]     keybits  Must be 128
 *
 * @retval 0                                   If successful
 * @retval MBEDTLS_ERR_AES_INVALID_KEY_LENGTH  If keybits was not 128
 */
int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key,
                           unsigned int keybits);

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * @return The return value of @ref CRYPTOAesEcb.
 * @retval 0                        If successful
 * @retval AES_KEYSTORE_READ_ERROR  If the indicated keystore ram could not be read
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx, int mode, const unsigned char input[16],
                          unsigned char output[16]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_ALT_H */
