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
 *   This file includes definitions for performing AES-ECB computations.
 */

#ifndef AES_ECB_HPP_
#define AES_ECB_HPP_

#include "openthread-core-config.h"

#include <mbedtls/aes.h>

namespace ot {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

/**
 * This class implements AES ECB computation.
 *
 */
class AesEcb
{
public:
    enum
    {
        kBlockSize = 16, ///< AES-128 block size (bytes).
    };

    /**
     * Constructor to initialize the mbedtls_aes_context.
     *
     */
    AesEcb(void);

    /**
     * Destructor to free the mbedtls_aes_context.
     *
     */
    ~AesEcb(void);

#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    /**
     * This method sets the key.
     *
     * @param[in]  aKeyRef     Key Reference for ECB operation
     *
     */
    void SetKey(const uint32_t aKeyRef);
#else
    /**
     * This method sets the key.
     *
     * @param[in]  aKey        A pointer to the key.
     * @param[in]  aKeyLength  The key length in bits.
     *
     */
    void SetKey(const uint8_t *aKey, uint16_t aKeyLength);
#endif

    /**
     * This method encrypts data.
     *
     * @param[in]   aInput   A pointer to the input buffer.
     * @param[out]  aOutput  A pointer to the output buffer.
     *
     */
    void Encrypt(const uint8_t aInput[kBlockSize], uint8_t aOutput[kBlockSize]);

private:
#if OPENTHREAD_CONFIG_PSA_CRYPTO_ENABLE
    uint32_t            mKeyRef;
#else
    mbedtls_aes_context mContext;
#endif
};

/**
 * @}
 *
 */

} // namespace Crypto
} // namespace ot

#endif // AES_ECB_HPP_
