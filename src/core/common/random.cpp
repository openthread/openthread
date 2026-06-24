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
 *   This file provides an implementation of OpenThread random number generation manager class.
 */

#include "random.hpp"

#include <openthread/platform/entropy.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"

namespace ot {
namespace Random {

uint16_t               Manager::sInitCount = 0;
Manager::NonCryptoPrng Manager::sPrng;

Manager::Manager(void)
{
    uint32_t seed;

    OT_ASSERT(sInitCount < 0xffff);

    VerifyOrExit(sInitCount == 0);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    otPlatCryptoRandomInit();
    SuccessOrAssert(Random::Crypto::Fill(seed));
#else
    SuccessOrAssert(otPlatEntropyGet(reinterpret_cast<uint8_t *>(&seed), sizeof(seed)));
#endif

    sPrng.Init(seed);

exit:
    sInitCount++;
}

Manager::~Manager(void)
{
    OT_ASSERT(sInitCount > 0);

    sInitCount--;
    VerifyOrExit(sInitCount == 0);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    otPlatCryptoRandomDeinit();
#endif

exit:
    return;
}

uint32_t Manager::NonCryptoGetUint32(void)
{
    OT_ASSERT(sInitCount > 0);

    return sPrng.GetNext();
}

//-------------------------------------------------------------------
// NonCryptoPrng

void Manager::NonCryptoPrng::Init(uint32_t aSeed)
{
    // The PRNG has a cycle of length 1 for the below two initial
    // seeds. For all other seed values the cycle is ~2^31 long.

    if ((aSeed == 0) || (aSeed == 0x7fffffff))
    {
        aSeed = 0x1;
    }

    mState = aSeed;
}

uint32_t Manager::NonCryptoPrng::GetNext(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    tmpstate = static_cast<uint64_t>(33614) * static_cast<uint64_t>(mState);
    q        = tmpstate & 0xffffffff;
    q        = q >> 1;
    p        = tmpstate >> 32;
    mlcg     = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    mState = mlcg;

    return mlcg;
}

//-------------------------------------------------------------------

namespace NonCrypto {

static uint32_t GenerateInRange(uint32_t aMin, uint32_t aMax)
{
    uint32_t value;
    uint32_t range;

    VerifyOrExit(aMin < aMax, value = aMin);

    value = Generate<uint32_t>();

    range = aMax - aMin;

    VerifyOrExit(range < NumericLimits<uint32_t>::kMax);

    value = aMin + (value % (range + 1));

exit:
    return value;
}

template <typename UintType> UintType GenerateUpToExcluding(UintType aMax)
{
    return GenerateFromMinUpToExcluding<UintType>(0, aMax);
}

template <typename UintType> UintType GenerateFromMinUpToExcluding(UintType aMin, UintType aMax)
{
    if (aMax > 0)
    {
        aMax--;
    }

    return GenerateInClosedRange<UintType>(aMin, aMax);
}

template <typename UintType> UintType GenerateInClosedRange(UintType aMin, UintType aMax)
{
    return static_cast<UintType>(GenerateInRange(aMin, aMax));
}

// Explicit instantiations

template uint8_t  GenerateUpToExcluding<uint8_t>(uint8_t aMax);
template uint16_t GenerateUpToExcluding<uint16_t>(uint16_t aMax);
template uint32_t GenerateUpToExcluding<uint32_t>(uint32_t aMax);

template uint8_t  GenerateFromMinUpToExcluding<uint8_t>(uint8_t aMin, uint8_t aMax);
template uint16_t GenerateFromMinUpToExcluding<uint16_t>(uint16_t aMin, uint16_t aMax);
template uint32_t GenerateFromMinUpToExcluding<uint32_t>(uint32_t aMin, uint32_t aMax);

template uint8_t  GenerateInClosedRange<uint8_t>(uint8_t aMin, uint8_t aMax);
template uint16_t GenerateInClosedRange<uint16_t>(uint16_t aMin, uint16_t aMax);
template uint32_t GenerateInClosedRange<uint32_t>(uint32_t aMin, uint32_t aMax);

void FillBuffer(uint8_t *aBuffer, uint16_t aSize)
{
    while (aSize-- != 0)
    {
        *aBuffer++ = Generate<uint8_t>();
    }
}

uint32_t AddJitter(uint32_t aValue, uint16_t aJitter)
{
    uint32_t delay = 0;

    VerifyOrExit(aValue != 0);

    aJitter = (aJitter <= aValue) ? aJitter : static_cast<uint16_t>(aValue);
    delay   = aValue + GenerateUpToExcluding<uint32_t>(2 * aJitter + 1) - aJitter;

exit:
    return delay;
}

} // namespace NonCrypto
} // namespace Random
} // namespace ot
