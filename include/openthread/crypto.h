/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *  This file defines the OpenThread crypto C APIs.
 */

#ifndef OPENTHREAD_CRYPTO_H_
#define OPENTHREAD_CRYPTO_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/platform/crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-crypto
 *
 * @brief
 *   This module includes cryptographic functions.
 *
 * @{
 *
 */

/**
 * Represents a SHA-256 hash.
 *
 */
typedef otPlatCryptoSha256Hash otCryptoSha256Hash;

/**
 * Performs HMAC computation.
 *
 * @param[in]     aKey           A pointer to the key.
 * @param[in]     aBuf           A pointer to the input buffer.
 * @param[in]     aBufLength     The length of @p aBuf in bytes.
 * @param[out]    aHash          A pointer to a `otCryptoSha256Hash` structure to output the hash value.
 *
 */
void otCryptoHmacSha256(const otCryptoKey *aKey, const uint8_t *aBuf, uint16_t aBufLength, otCryptoSha256Hash *aHash);

/**
 * Performs AES CCM computation.
 *
 * @param[in]      aKey           A pointer to the key.
 * @param[in]      aTagLength     Length of tag in bytes.
 * @param[in]      aNonce         A pointer to the nonce.
 * @param[in]      aNonceLength   Length of nonce in bytes.
 *
 * @param[in]      aHeader        A pointer to the header.
 * @param[in]      aHeaderLength  Length of header in bytes.
 *
 * @param[in,out]  aPlainText     A pointer to the plaintext.
 * @param[in,out]  aCipherText    A pointer to the ciphertext.
 * @param[in]      aLength        Plaintext length in bytes.
 * @param[in]      aEncrypt       `true` on encrypt and `false` on decrypt.
 *
 * @param[out]     aTag           A pointer to the tag.
 *
 */
void otCryptoAesCcm(const otCryptoKey *aKey,
                    uint8_t            aTagLength,
                    const void        *aNonce,
                    uint8_t            aNonceLength,
                    const void        *aHeader,
                    uint32_t           aHeaderLength,
                    void              *aPlainText,
                    void              *aCipherText,
                    uint32_t           aLength,
                    bool               aEncrypt,
                    void              *aTag);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_CRYPTO_H_
