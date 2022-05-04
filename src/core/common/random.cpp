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

namespace ot {
namespace Random {

uint16_t               Manager::sInitCount = 0;
Manager::NonCryptoPrng Manager::sPrng;

Manager::Manager(void)
{
    uint32_t seed;

    OT_ASSERT(sInitCount < 0xffff);

    VerifyOrExit(sInitCount == 0);

#if !OPENTHREAD_RADIO
    otPlatCryptoRandomInit();
    SuccessOrAssert(Random::Crypto::FillBuffer(reinterpret_cast<uint8_t *>(&seed), sizeof(seed)));
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

#if !OPENTHREAD_RADIO
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

uint8_t GetUint8InRange(uint8_t aMin, uint8_t aMax)
{
    OT_ASSERT(aMax > aMin);

    return (aMin + (GetUint8() % (aMax - aMin)));
}

uint16_t GetUint16InRange(uint16_t aMin, uint16_t aMax)
{
    OT_ASSERT(aMax > aMin);
    return (aMin + (GetUint16() % (aMax - aMin)));
}

uint32_t GetUint32InRange(uint32_t aMin, uint32_t aMax)
{
    OT_ASSERT(aMax > aMin);
    return (aMin + (GetUint32() % (aMax - aMin)));
}

void FillBuffer(uint8_t *aBuffer, uint16_t aSize)
{
    while (aSize-- != 0)
    {
        *aBuffer++ = GetUint8();
    }
}

uint32_t AddJitter(uint32_t aValue, uint16_t aJitter)
{
    aJitter = (aJitter <= aValue) ? aJitter : static_cast<uint16_t>(aValue);

    return aValue + GetUint32InRange(0, 2 * aJitter + 1) - aJitter;
}

} // namespace NonCrypto
} // namespace Random
} // namespace ot
