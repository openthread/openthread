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
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mbedtls/aes.h"

#include <string.h>

#ifdef MBEDTLS_AES_ALT

#include "aes_alt_cc310.h"
#include "cc310_mbedtls.h"
#include "ssi_pal_types.h"

static void aes_init(mbedtls_aes_context * ctx, SaSiAesEncryptMode_t mode)
{
    memset(&ctx->user_context, 0, sizeof(ctx->user_context));

    ctx->mode = mode;
    ctx->key.pKey = ctx->key_buffer;

    CC310_OPERATION_NO_RESULT(SaSi_AesInit(&ctx->user_context,
                                           mode,
                                           SASI_AES_MODE_ECB,
                                           SASI_AES_PADDING_NONE));
}

void aes_cc310_init(mbedtls_aes_context * ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    aes_init(ctx, SASI_AES_ENCRYPT);
}

void aes_cc310_free(mbedtls_aes_context * ctx)
{
    CC310_OPERATION_NO_RESULT(SaSi_AesFree(&ctx->user_context));
}

int aes_cc310_setkey_enc(mbedtls_aes_context * ctx,
                           const unsigned char * key,
                           unsigned int          keybits)
{
    SaSiError_t result = SASI_OK;

    ctx->key.keySize = (keybits + 7) / 8;
    ctx->key.pKey = ctx->key_buffer;

    memcpy(ctx->key_buffer, key, ctx->key.keySize);

    CC310_OPERATION(SaSi_AesSetKey(&ctx->user_context,
                                   SASI_AES_USER_KEY,
                                   &ctx->key,
                                   sizeof(ctx->key)),
                    result);

    return (int)result;
}

int aes_cc310_crypt_ecb(mbedtls_aes_context * ctx,
                          int                   mode,
                          const unsigned char   input[16],
                          unsigned char         output[16])
{
    SaSiAesEncryptMode_t reinit_mode = SASI_AES_ENCRYPT_MODE_LAST;
    SaSiError_t          result      = SASI_OK;

    if ((mode == MBEDTLS_AES_ENCRYPT) && (ctx->mode != SASI_AES_ENCRYPT))
    {
        reinit_mode = SASI_AES_ENCRYPT;
    }
    else if ((mode == MBEDTLS_AES_DECRYPT) && (ctx->mode != SASI_AES_DECRYPT))
    {
        reinit_mode = SASI_AES_DECRYPT;
    }

    if ((reinit_mode == SASI_AES_ENCRYPT) || (reinit_mode == SASI_AES_DECRYPT))
    {
        aes_init(ctx, reinit_mode);

        CC310_OPERATION(SaSi_AesSetKey(&ctx->user_context,
                                       SASI_AES_USER_KEY,
                                       &ctx->key,
                                       sizeof(ctx->key)),
                        result);

        if (result != SASI_OK)
        {
            return (int)result;
        }
    }

    CC310_OPERATION(SaSi_AesBlock(&ctx->user_context, (uint8_t *)input, 16, output), result);

    return (int)result;
}

#endif /* MBEDTLS_AES_ALT */
