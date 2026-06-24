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

#ifndef OT_CORE_COMMON_RANDOM_HPP_
#define OT_CORE_COMMON_RANDOM_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/platform/crypto.h>

#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/non_copyable.hpp"
#include "common/type_traits.hpp"

namespace ot {
namespace Random {

/**
 * Manages random number generator initialization/deinitialization.
 */
class Manager : private NonCopyable
{
public:
    /**
     * Initializes the object.
     */
    Manager(void);

    /**
     * This destructor deinitializes the object.
     */
    ~Manager(void);

    /**
     * Generates and returns a random value using a non-crypto Pseudo Random Number Generator.
     *
     * @returns    A random `uint32_t` value.
     */
    static uint32_t NonCryptoGetUint32(void);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * Fills a given buffer with cryptographically secure random bytes.
     *
     * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
     * @param[in]  aSize    Size of buffer (number of bytes to fill).
     *
     * @retval kErrorNone    Successfully filled buffer with random values.
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
 * Generates and returns a random value of a given unsigned integer type.
 *
 * @tparam UintType  The unsigned integer type to generate (must be `uint8_t`, `uint16_t`, or `uint32_t`).
 *
 * @returns A random `UintType` value.
 */
template <typename UintType> inline UintType Generate(void)
{
    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType must be an unsigned int");
    static_assert(!TypeTraits::IsSame<UintType, uint64_t>::kValue, "UintType cannot be `uint64_t`");

    return static_cast<UintType>(Manager::NonCryptoGetUint32());
}

/**
 * Generates and returns a random value of a given unsigned integer type within range `[0, aMax)`.
 *
 * @tparam UintType  The unsigned integer type to generate (must be `uint8_t`, `uint16_t`, or `uint32_t`).
 *
 * @param[in] aMax  The upper bound (exclusive). If zero, zero is returned.
 *
 * @returns A random `UintType` value in the range `[0, aMax)`.
 */
template <typename UintType> UintType GenerateUpToExcluding(UintType aMax);
/**
 * Generates and returns a random value of a given unsigned integer type within range `[aMin, aMax)`.
 *
 * @tparam UintType  The unsigned integer type to generate (must be `uint8_t`, `uint16_t`, or `uint32_t`).
 *
 * @param[in] aMin  The lower bound (inclusive).
 * @param[in] aMax  The upper bound (exclusive). If @p aMax <= @p aMin, @p aMin is returned.
 *
 * @returns A random `UintType` value in the range `[aMin, aMax)`.
 */
template <typename UintType> UintType GenerateFromMinUpToExcluding(UintType aMin, UintType aMax);

/**
 * Generates and returns a random value of a given unsigned integer type within a closed range `[aMin, aMax]`.
 *
 * @tparam UintType  The unsigned integer type to generate (must be `uint8_t`, `uint16_t`, or `uint32_t`).
 *
 * @param[in] aMin  The lower bound (inclusive).
 * @param[in] aMax  The upper bound (inclusive). If @p aMax < @p aMin, @p aMin is returned.
 *
 * @returns A random `UintType` value in the range `[aMin, aMax]`.
 */
template <typename UintType> UintType GenerateInClosedRange(UintType aMin, UintType aMax);
/**
 * Fills a given buffer with random bytes.
 *
 * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
 * @param[in]  aSize    Size of buffer (number of bytes to fill).
 */
void FillBuffer(uint8_t *aBuffer, uint16_t aSize);

/**
 * Fills a given object with random bytes.
 *
 * @tparam    ObjectType   The object type to fill.
 *
 * @param[in] aObject      A reference to the object to fill.
 */
template <typename ObjectType> void Fill(ObjectType &aObject)
{
    static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

    FillBuffer(reinterpret_cast<uint8_t *>(&aObject), sizeof(ObjectType));
}

/**
 * Adds a random jitter within a given range to a given value.
 *
 * @param[in]  aValue     A value to which the random jitter is added.
 * @param[in]  aJitter    Maximum jitter. Random jitter is selected from the range `[-aJitter, aJitter]`.
 *
 * @returns    The given value with an added random jitter.
 */
uint32_t AddJitter(uint32_t aValue, uint16_t aJitter);

} // namespace NonCrypto

#if OPENTHREAD_FTD || OPENTHREAD_MTD

namespace Crypto {

/**
 * Fills a given buffer with cryptographically secure random bytes.
 *
 * @param[out] aBuffer  A pointer to a buffer to fill with the random bytes.
 * @param[in]  aSize    Size of buffer (number of bytes to fill).
 *
 * @retval kErrorNone    Successfully filled buffer with random values.
 */
inline Error FillBuffer(uint8_t *aBuffer, uint16_t aSize) { return Manager::CryptoFillBuffer(aBuffer, aSize); }

/**
 * Fills a given object with cryptographically secure random bytes.
 *
 * @tparam    ObjectType   The object type to fill.
 *
 * @param[in] aObject      A reference to the object to fill.
 *
 * @retval kErrorNone    Successfully filled @p aObject with random values.
 * @retval kErrorFailed  Failed to generate secure random bytes to fill the object.
 */
template <typename ObjectType> Error Fill(ObjectType &aObject)
{
    static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

    return FillBuffer(reinterpret_cast<uint8_t *>(&aObject), sizeof(ObjectType));
}

} // namespace Crypto

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

} // namespace Random
} // namespace ot

#endif // OT_CORE_COMMON_RANDOM_HPP_
