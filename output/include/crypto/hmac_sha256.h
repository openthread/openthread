/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

/**
 * @file
 * @brief
 *   This file includes the platform abstraction for HMAC SHA-256 computations.
 */

#ifndef HMAC_SHA256_H_
#define HMAC_SHA256_H_

#include <stdint.h>

#include <cryptocontext.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

enum
{
    otCryptoSha256Size = 32,  ///< SHA-256 hash size (bytes)
};

/**
 * This method sets the key.
 *
 * @param[in]  aCryptoContext  The crypto context used.
 * @param[in]  aKey            A pointer to the key.
 * @param[in]  aKeyLength      The key length in bytes.
 *
 */
void otCryptoHmacSha256Start(otCryptoContext *aCryptoContext, const void *aKey, uint16_t aKeyLength);

/**
 * This method inputs bytes into the HMAC computation.
 *
 * @param[in]  aCryptoContext  The crypto context used.
 * @param[in]  aBuf            A pointer to the input buffer.
 * @param[in]  aBufLength      The length of @p aBuf in bytes.
 *
 */
void otCryptoHmacSha256Update(otCryptoContext *aCryptoContext, const void *aBuf, uint16_t aBufLength);

/**
 * This method finalizes the hash computation.
 *
 * @param[in]   aCryptoContext  The crypto context used.
 * @param[out]  aHash  A pointer to the output buffer.
 *
 */
void otCryptoHmacSha256Finish(otCryptoContext *aCryptoContext, uint8_t aHash[otCryptoSha256Size]);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // HMAC_SHA256_H_
