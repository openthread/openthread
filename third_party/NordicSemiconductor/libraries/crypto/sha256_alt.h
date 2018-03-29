/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *
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

#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef MBEDTLS_SHA256_ALT

#include "crys_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__CC_ARM)
#pragma anon_unions
#endif

/**
 * @brief SHA-256 context structure
 */
typedef union
{
    struct
    {
        CRYS_HASHUserContext_t    user_context; ///< User context for CC310 SHA256
        CRYS_HASH_OperationMode_t mode;         ///< CC310 hash operation mode
    };
    struct
    {
        uint32_t      total[2];                 ///< number of bytes processed
        uint32_t      state[8];                 ///< intermediate digest state
        unsigned char buffer[64];               ///< data block being processed
        int           is224;                    ///< 0 => SHA-256, else SHA-224
    };
}
mbedtls_sha256_context;

#if defined(__CC_ARM)
#pragma no_anon_unions
#endif

/**
 * @brief Initialize SHA-256 context
 *
 * @param [in,out] ctx SHA-256 context to be initialized
 */
void mbedtls_sha256_init(mbedtls_sha256_context *ctx);

/**
 * @brief Clear SHA-256 context
 *
 * @param [in,out] ctx SHA-256 context to be cleared
 */
void mbedtls_sha256_free(mbedtls_sha256_context *ctx);

/**
 * @brief Clone (the state of) a SHA-256 context
 *
 * @param [out] dst The destination context
 * @param [in] src The context to be cloned
 */
void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                          const mbedtls_sha256_context *src);

/**
 * @brief SHA-256 context setup
 *
 * @param [in,out] ctx context to be initialized
 * @param [in] is224 0 = use SHA256, 1 = use SHA224
 */
void mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224);

/**
 * @brief SHA-256 process buffer
 *
 * @param [in,out] ctx SHA-256 context
 * @param [in] input buffer holding the  data
 * @param [in] ilen length of the input data
 */
void mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen);

/**
 * @brief SHA-256 final digest
 *
 * @param [in,out] ctx SHA-256 context
 * @param [out] output SHA-224/256 checksum result
 */
void mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char output[32]);

/* Internal use */
void mbedtls_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64]);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_ALT_H */
