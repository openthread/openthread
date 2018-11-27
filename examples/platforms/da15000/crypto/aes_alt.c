/*
 * Copyright (c) 2017, The OpenThread Authors.
 * All rights reserved.
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

#include "aes_alt.h"

#ifdef MBEDTLS_AES_ALT

#include <stdint.h>
#include <string.h>

#include <openthread/platform/toolchain.h>

#include "hw_aes_hash.h"
#include <common/code_utils.hpp>
#include "mbedtls/aes.h"

static void mbedtls_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = (unsigned char *)v;

    while (n--)
    {
        *p++ = 0;
    }
}

void mbedtls_aes_init(mbedtls_aes_context *ctx)
{
    memset(ctx, 0, sizeof(mbedtls_aes_context));
}

void mbedtls_aes_free(mbedtls_aes_context *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    hw_aes_hash_disable_clock();
    mbedtls_zeroize(ctx, sizeof(mbedtls_aes_context));
}

int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits)
{
    int retval = 0;

    switch (keybits)
    {
    case 128:
        ctx->hwKeyLen = HW_AES_128;
        memcpy(ctx->aes_enc_key, key, 16);
        break;

    case 192:
        ctx->hwKeyLen = HW_AES_192;
        memcpy(ctx->aes_enc_key, key, 24);
        break;

    case 256:
        ctx->hwKeyLen = HW_AES_256;
        memcpy(ctx->aes_enc_key, key, 32);
        break;

    default:
        retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
        break;
    }

    return retval;
}

int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits)
{
    int retval = 0;

    switch (keybits)
    {
    case 128:
        ctx->hwKeyLen = HW_AES_128;
        memcpy(ctx->aes_dec_key, key, 16);
        break;

    case 192:
        ctx->hwKeyLen = HW_AES_192;
        memcpy(ctx->aes_enc_key, key, 24);
        break;

    case 256:
        ctx->hwKeyLen = HW_AES_256;
        memcpy(ctx->aes_enc_key, key, 32);
        break;

    default:
        retval = MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
        break;
    }

    return retval;
}

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         0 if successful
 */
int mbedtls_aes_crypt_ecb(mbedtls_aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16])
{
    int retval = 0;

    hw_aes_hash_enable_clock();
    hw_aes_hash_mark_input_block_as_last();
    hw_aes_hash_cfg_aes_ecb(ctx->hwKeyLen);
    hw_aes_hash_store_keys(ctx->hwKeyLen, ctx->aes_enc_key, HW_AES_PERFORM_KEY_EXPANSION);
    hw_aes_hash_cfg_dma((uint8_t *)input, (uint8_t *)output, 16);

    switch (mode)
    {
    case MBEDTLS_AES_ENCRYPT:
        hw_aes_hash_encrypt();
        retval = 0;
        break;

    case MBEDTLS_AES_DECRYPT:
        hw_aes_hash_decrypt();
        retval = 0;
        break;

    default:
        retval = -1;
        break;
    }

    while (hw_aes_hash_is_active())
    {
        ;
    }

    return retval;
}

int mbedtls_aes_self_test(int verbose)
{
    OT_UNUSED_VARIABLE(verbose);

    /* 128-bit Key 2b7e151628aed2a6abf7158809cf4f3c */
    const uint8_t key_128b[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                                  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

    int                 retval = 0;
    mbedtls_aes_context aes;
    uint8_t             input[16]   = {0};
    uint8_t             output[16]  = {0};
    uint8_t             decrypt[16] = {0};

    strcpy((char *)input, (const char *)"hw_aes_test");

    mbedtls_aes_init(&aes);

    retval = mbedtls_aes_setkey_enc(&aes, (const unsigned char *)key_128b, 128);
    VerifyOrExit(retval != 0, retval = -1);

    retval = mbedtls_aes_setkey_dec(&aes, (const unsigned char *)key_128b, 128);
    VerifyOrExit(retval != 0, retval = -1);

    retval = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
    VerifyOrExit(retval != 0, retval = -1);

    retval = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, output, decrypt);
    VerifyOrExit(retval != 0, retval = -1);

    mbedtls_aes_free(&aes);

exit:
    return retval;
}

#endif /* MBEDTLS_AES_ALT */
