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
 *  This file includes definitions for OpenThread random number generation.
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/platform/crypto.h>

#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/non_copyable.hpp"

namespace ot {
namespace Random {

/**
 * This class manages random number generator initialization/deinitialization.
 *
 */
class Manager : private NonCopyable
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    Manager(void);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~Manager(void);

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

namespace NonCrypto {

/**
 * This function generates and returns a random `uint32_t` value.
 *
 * @returns    A random `uint32_t` value.
 *
 */
inline uint32_t GetUint32(void)
{
    return Manager::NonCryptoGetUint32();
}

/**
 * This function generates and returns a random byte.
 *
 * @returns A random `uint8_t` value.
 *
 */
inline uint8_t GetUint8(void)
{
    return static_cast<uint8_t>(GetUint32() & 0xff);
}

/**
 * This function generates and returns a random `uint16_t` value.
 *
 * @returns A random `uint16_t` value.
 *
 */
inline uint16_t GetUint16(void)
{
    return static_cast<uint16_t>(GetUint32() & 0xffff);
}

/**
 * This function generates and returns a random `uint8_t` value within a given range `[aMin, aMax)`.
 *
 * @param[in]  aMin  A minimum value (this value can be included in returned random result).
 * @param[in]  aMax  A maximum value (this value is excluded from returned random result).
 *
 * @returns    A random `uint8_t` value in the given range (i.e., aMin <= random value < aMax).
 *
 */
uint8_t GetUint8InRange(uint8_t aMin, uint8_t aMax);

/**
 * This function generates and returns a random `uint16_t` value within a given range `[aMin, aMax)`.
 *
 * @note The returned random value can include the @p aMin value but excludes the @p aMax.
 *
 * @param[in]  aMin  A minimum value (this value can be included in returned random result).
 * @param[in]  aMax  A maximum value (this value is excluded from returned random result).
 *
 * @returns    A random `uint16_t` value in the given range (i.e., aMin <= random value < aMax).
 *
 */
uint16_t GetUint16InRange(uint16_t aMin, uint16_t aMax);

/**
 * This function generates and returns a random `uint32_t` value within a given range `[aMin, aMax)`.
 *
 * @note The returned random value can include the @p aMin value but excludes the @p aMax.
 *
 * @param[in]  aMin  A minimum value (this value can be included in returned random result).
 * @param[in]  aMax  A maximum value (this value is excluded from returned random result).
 *
 * @returns    A random `uint32_t` value in the given range (i.e., aMin <= random value < aMax).
 *
 */
uint32_t GetUint32InRange(uint32_t aMin, uint32_t aMax);

/**
 * This function fills a given buffer with random bytes.
 *
 * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
 * @param[in]  aSize    Size of buffer (number of bytes to fill).
 *
 */
void FillBuffer(uint8_t *aBuffer, uint16_t aSize);

/**
 * This function adds a random jitter within a given range to a given value.
 *
 * @param[in]  aValue     A value to which the random jitter is added.
 * @param[in]  aJitter    Maximum jitter. Random jitter is selected from the range `[-aJitter, aJitter]`.
 *
 * @returns    The given value with an added random jitter.
 *
 */
uint32_t AddJitter(uint32_t aValue, uint16_t aJitter);

} // namespace NonCrypto

#if !OPENTHREAD_RADIO

namespace Crypto {

/**
 * This function fills a given buffer with cryptographically secure random bytes.
 *
 * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
 * @param[in]  aSize    Size of buffer (number of bytes to fill).
 *
 * @retval kErrorNone    Successfully filled buffer with random values.
 *
 */
inline Error FillBuffer(uint8_t *aBuffer, uint16_t aSize)
{
    return Manager::CryptoFillBuffer(aBuffer, aSize);
}

} // namespace Crypto

#endif // !OPENTHREAD_RADIO

} // namespace Random
} // namespace ot

#endif // RANDOM_HPP_
