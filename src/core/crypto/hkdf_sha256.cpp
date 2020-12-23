/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements HMAC-based Extract-and-Expand Key Derivation Function (HKDF) using SHA-256.
 */

#include "hkdf_sha256.hpp"

#include <string.h>

namespace ot {
namespace Crypto {

void HkdfSha256::Extract(const uint8_t *aSalt, uint16_t aSaltLength, const uint8_t *aInputKey, uint16_t aInputKeyLength)
{
    HmacSha256 hmac;

    // PRK is calculated as HMAC-Hash(aSalt, aInputKey)

    hmac.Start(aSalt, aSaltLength);
    hmac.Update(aInputKey, aInputKeyLength);
    hmac.Finish(mPrk);
}

void HkdfSha256::Expand(const uint8_t *aInfo, uint16_t aInfoLength, uint8_t *aOutputKey, uint16_t aOutputKeyLength)
{
    HmacSha256       hmac;
    HmacSha256::Hash hash;
    uint8_t          iter = 0;
    uint16_t         copyLength;

    // The aOutputKey is calculated as follows [RFC5889]:
    //
    //   N = ceil( aOutputKeyLength / HashSize)
    //   T = T(1) | T(2) | T(3) | ... | T(N)
    //   aOutputKey is first aOutputKeyLength of T
    //
    // Where:
    //   T(0) = empty string (zero length)
    //   T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
    //   T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
    //   T(3) = HMAC-Hash(PRK, T(2) | info | 0x03)
    //   ...

    while (aOutputKeyLength > 0)
    {
        hmac.Start(mPrk.GetBytes(), sizeof(mPrk));

        if (iter != 0)
        {
            hmac.Update(hash);
        }

        hmac.Update(aInfo, aInfoLength);

        iter++;
        hmac.Update(iter);
        hmac.Finish(hash);

        copyLength = (aOutputKeyLength > sizeof(hash)) ? sizeof(hash) : aOutputKeyLength;

        memcpy(aOutputKey, hash.GetBytes(), copyLength);
        aOutputKey += copyLength;
        aOutputKeyLength -= copyLength;
    }
}

} // namespace Crypto
} // namespace ot
