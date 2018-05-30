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
 *   This file includes definitions for performing HMAC SHA-256 computations.
 */

#ifndef HMAC_SHA256_HPP_
#define HMAC_SHA256_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"

#include <mbedtls/md.h>

namespace ot {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

/**
 * This class implements HMAC SHA-256 computation.
 *
 */
class HmacSha256
{
public:
    enum
    {
        kHashSize = 32, ///< SHA-256 hash size (bytes)
    };

    /**
     * Constructor for initialization of mbedtls_md_context_t.
     *
     */
    HmacSha256();

    /**
     * Destructor for freeing of mbedtls_md_context_t.
     *
     */
    ~HmacSha256();

    /**
     * This method sets the key.
     *
     * @param[in]  aKey        A pointer to the key.
     * @param[in]  aKeyLength  The key length in bytes.
     *
     */
    void Start(const uint8_t *aKey, uint16_t aKeyLength);

    /**
     * This method inputs bytes into the HMAC computation.
     *
     * @param[in]  aBuf        A pointer to the input buffer.
     * @param[in]  aBufLength  The length of @p aBuf in bytes.
     *
     */
    void Update(const uint8_t *aBuf, uint16_t aBufLength);

    /**
     * This method finalizes the hash computation.
     *
     * @param[out]  aHash  A pointer to the output buffer.
     *
     */
    void Finish(uint8_t aHash[kHashSize]);

private:
    mbedtls_md_context_t mContext;
};

/**
 * @}
 *
 */

} // namespace Crypto
} // namespace ot

#endif // HMAC_SHA256_HPP_
