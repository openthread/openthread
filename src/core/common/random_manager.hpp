/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *  This file includes definitions for OpenThread random number generation manager class.
 */

#ifndef RANDOM_MANAGER_HPP_
#define RANDOM_MANAGER_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/platform/crypto.h>

#include "common/error.hpp"
#include "common/non_copyable.hpp"

namespace ot {

/**
 * This class manages random number generator initialization/deinitialization.
 *
 */
class RandomManager : private NonCopyable
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    RandomManager(void);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~RandomManager(void);

    /**
     * This static method generates and returns a random value using a non-crypto Pseudo Random Number Generator.
     *
     * @returns    A random `uint32_t` value.
     *
     */
    static uint32_t NonCryptoGetUint32(void);

#if !OPENTHREAD_RADIO
    /**
     * This static method fills a given buffer with cryptographically secure random bytes.
     *
     * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
     * @param[in]  aSize    Size of buffer (number of bytes to fill).
     *
     * @retval kErrorNone    Successfully filled buffer with random values.
     *
     */
    static Error CryptoFillBuffer(uint8_t *aBuffer, uint16_t aSize) { return otPlatCryptoRandomGet(aBuffer, aSize); }
#endif

private:
    class NonCryptoPrng // A non-crypto Pseudo Random Number Generator (PRNG)
    {
    public:
        void     Init(uint32_t aSeed);
        uint32_t GetNext(void);

    private:
        uint32_t mState;
    };

    static uint16_t      sInitCount;
    static NonCryptoPrng sPrng;
};

} // namespace ot

#endif // RANDOM_MANAGER_HPP_
