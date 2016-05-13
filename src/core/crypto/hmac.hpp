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
 *   This file includes definitions for computing HMACs.
 */

#ifndef HMAC_HPP_
#define HMAC_HPP_

#include <stdint.h>

#include <openthread-types.h>
#include <crypto/hash.hpp>

namespace Thread {
namespace Crypto {

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

/**
 * This class implements HMAC computation.
 *
 */
class Hmac
{
public:
    /**
     * This constructor intializes the object.
     *
     */
    explicit Hmac(Hash &aHash);

    /**
     * This method sets the key.
     *
     * @param[in]  aKey        A pointer to the key.
     * @param[in]  aKeyLength  The key length in bytes.
     *
     */
    void SetKey(const void *aKey, uint16_t aKeyLength);

    /**
     * This method initializes the HMAC computation.
     *
     */
    void Init(void);

    /**
     * This method inputs bytes into the HMAC computation.
     *
     * @param[in]  aBuf        A pointer to the input buffer.
     * @param[in]  aBufLength  The length of @p aBuf in bytes.
     *
     */
    void Input(const void *aBuf, uint16_t aBufLength);

    /**
     * This method finalizes the hash computation.
     *
     * @param[out]  aHash  A pointer to the output buffer.
     *
     */
    void Finalize(uint8_t *aHash);

private:
    enum
    {
        kMaxKeyLength = 64,
    };
    uint8_t mKey[kMaxKeyLength];
    uint8_t mKeyLength = 0;
    Hash *mHash;
};

/**
 * @}
 *
 */

}  // namespace Crypto
}  // namespace Thread

#endif  // CRYPTO_HMAC_HPP_
