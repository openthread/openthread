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

#ifndef AES_ALT_SOFT_H
#define AES_ALT_SOFT_H

#include <openthread-core-config.h>

#include <stddef.h>
#include <stdint.h>

#ifdef MBEDTLS_AES_ALT

#if NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT

#include "aes_alt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize AES context
 *
 * @param[in,out] ctx AES context to be initialized
 */
void aes_soft_init(mbedtls_aes_context * ctx);

/**
 * @brief Clear AES context
 *
 * @param[in,out] ctx AES context to be cleared
 */
void aes_soft_free(mbedtls_aes_context * ctx);

/**
 * @brief AES key schedule (encryption)
 *
 * @param[in,out] ctx     AES context to be initialized
 * @param[in]     key     encryption key
 * @param[in]     keybits must be 128, 192 or 256
 *
 * @return 0 if successful
 */
int aes_soft_setkey_enc(mbedtls_aes_context * ctx,
                         const unsigned char * key,
                         unsigned int          keybits);

/**
 * @brief AES-ECB block encryption/decryption
 *
 * @param[in,out] ctx    AES context
 * @param[in]     mode   MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT
 * @param[in]     input  16-byte input block
 * @param[out]    output 16-byte output block
 *
 * @return 0 if successful
 */
int aes_soft_crypt_ecb(mbedtls_aes_context * ctx,
                        int                   mode,
                        const unsigned char   input[16],
                        unsigned char         output[16]);

#ifdef __cplusplus
}
#endif

#endif /* NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT */

#endif /* MBEDTLS_AES_ALT */

#endif /* AES_ALT_SOFT_H */
