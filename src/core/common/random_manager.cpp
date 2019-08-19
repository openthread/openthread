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

#include "random_manager.hpp"

#include <openthread/platform/entropy.h>

#ifndef OPENTHREAD_RADIO
#include <mbedtls/entropy_poll.h>
#endif

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/random.hpp"
#include "crypto/mbedtls.hpp"

#define FIXED_POINT_FRACTIONAL_BITS 6
#define FLOAT_TO_FIXED(x) ((uint32_t)(x * (1 << FIXED_POINT_FRACTIONAL_BITS)))

namespace ot {

uint16_t                     RandomManager::sInitCount = 0;
RandomManager::NonCryptoPrng RandomManager::sPrng;

#ifndef OPENTHREAD_RADIO
RandomManager::Entropy       RandomManager::sEntropy;
RandomManager::CryptoCtrDrbg RandomManager::sCtrDrbg;
#endif

RandomManager::RandomManager(void)
{
    uint32_t seed;
    otError  error;

    assert(sInitCount < 0xffff);

    VerifyOrExit(sInitCount == 0);

#ifndef OPENTHREAD_RADIO
    sEntropy.Init();
#endif

    error = otPlatEntropyGet(reinterpret_cast<uint8_t *>(&seed), sizeof(seed));
    assert(error == OT_ERROR_NONE);

    sPrng.Init(seed);

#ifndef OPENTHREAD_RADIO
    sCtrDrbg.Init();
#endif

exit:
    sInitCount++;
}

RandomManager::~RandomManager(void)
{
    assert(sInitCount > 0);

    sInitCount--;
    VerifyOrExit(sInitCount == 0);

#ifndef OPENTHREAD_RADIO
    sCtrDrbg.Deinit();
    sEntropy.Deinit();
#endif

exit:
    return;
}

uint32_t RandomManager::NonCryptoGetUint32(void)
{
    assert(sInitCount > 0);

    return sPrng.GetNext();
}

//-------------------------------------------------------------------
// NonCryptoPrng

void RandomManager::NonCryptoPrng::Init(uint32_t aSeed)
{
    // The PRNG has a cycle of length 1 for the below two initial
    // seeds. For all other seed values the cycle is ~2^31 long.

    if ((aSeed == 0) || (aSeed == 0x7fffffff))
    {
        aSeed = 0x1;
    }

    mState = static_cast<uint32_t>(FLOAT_TO_FIXED(aSeed));
}

uint32_t RandomManager::NonCryptoPrng::FixedPointDivision(uint32_t aDividend, uint32_t aDivisor)
{
    uint32_t divResult = static_cast<uint32_t>(((uint64_t)aDividend * (1 << FIXED_POINT_FRACTIONAL_BITS)) / aDivisor);
    return divResult;
}

uint32_t RandomManager::NonCryptoPrng::ComputeLcgRandom(uint32_t  aNumBits,
                                                        uint32_t  aSeedVal,
                                                        uint32_t *aDunif,
                                                        uint32_t  aNdim)
{
    uint16_t i      = 0;
    uint32_t DZ     = 0;
    uint32_t DOVER  = 0;
    uint32_t DZ1    = 0;
    uint32_t DZ2    = 0;
    uint32_t DOVER1 = 0;
    uint32_t DOVER2 = 0;
    uint32_t DTWO31 = 0;
    uint32_t DMDLS  = 0;
    uint32_t DA1    = 0;
    uint32_t DA2    = 0;

    DTWO31 = static_cast<uint32_t>(FLOAT_TO_FIXED(2147483648.0)); /* DTWO31=2**31  */
    DMDLS  = static_cast<uint32_t>(FLOAT_TO_FIXED(2147483647.0)); /* DMDLS=2**31-1 */
    DA1    = static_cast<uint32_t>(FLOAT_TO_FIXED(41160.0));      /* DA1=950706376 MOD 2**16 */
    DA2    = static_cast<uint32_t>(FLOAT_TO_FIXED(950665216.0));  /* DA2=950706376-DA1 */

    DZ = aSeedVal;
    if (aNumBits > aNdim)
    {
        aNumBits = aNdim;
    }

    for (i = 1; i <= aNumBits; i++)
    {
        DZ  = static_cast<uint32_t>(DZ);
        DZ1 = DZ * DA1;
        DZ2 = DZ * DA2;

        DOVER1 = FixedPointDivision(DZ1, DTWO31);
        DOVER2 = FixedPointDivision(DZ2, DTWO31);

        DZ1           = DZ1 - DOVER1 * DTWO31;
        DZ2           = DZ2 - DOVER2 * DTWO31;
        DZ            = DZ1 + DZ2 + DOVER1 + DOVER2;
        DOVER         = FixedPointDivision(DZ, DMDLS);
        DZ            = DZ - DOVER * DMDLS;
        aDunif[i - 1] = FixedPointDivision(DZ, DMDLS);
        aSeedVal      = DZ;
    }

    return aSeedVal;
}

uint32_t RandomManager::NonCryptoPrng::GetNext(void)
{
    const uint16_t NUM_OF_BITS          = 32;
    uint32_t       DUNIF[NUM_OF_BITS]   = {0};
    uint32_t       epsilon[NUM_OF_BITS] = {0};
    uint8_t        i                    = 0;
    uint16_t       bit                  = 0;
    uint16_t       num_0s               = 0;
    uint16_t       num_1s               = 0;
    uint16_t       bitsRead             = 0;
    uint32_t       iRandomValue         = 0;

    mState = ComputeLcgRandom(NUM_OF_BITS, mState, DUNIF, NUM_OF_BITS);

    for (i = 0; i < NUM_OF_BITS; i++)
    {
        if (DUNIF[i] < (static_cast<uint32_t>(FLOAT_TO_FIXED(0.5))))
        {
            bit = 0;
            num_0s++;
        }
        else
        {
            bit = 1;
            num_1s++;
        }
        bitsRead++;
        epsilon[i] = bit;
    }
    for (i = 0; i < NUM_OF_BITS; i++)
    {
        if (epsilon[i] == 0)
        {
            iRandomValue = iRandomValue * 2 + 0;
        }
        else
        {
            iRandomValue = iRandomValue * 2 + 1;
        }
    }

    return iRandomValue;
}

#ifndef OPENTHREAD_RADIO

//-------------------------------------------------------------------
// Entropy

void RandomManager::Entropy::Init(void)
{
    mbedtls_entropy_init(&mEntropyContext);
    mbedtls_entropy_add_source(&mEntropyContext, &RandomManager::Entropy::HandleMbedtlsEntropyPoll, NULL,
                               MBEDTLS_ENTROPY_MIN_HARDWARE, MBEDTLS_ENTROPY_SOURCE_STRONG);
}

void RandomManager::Entropy::Deinit(void)
{
    mbedtls_entropy_free(&mEntropyContext);
}

int RandomManager::Entropy::HandleMbedtlsEntropyPoll(void *         aData,
                                                     unsigned char *aOutput,
                                                     size_t         aInLen,
                                                     size_t *       aOutLen)
{
    int rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

    SuccessOrExit(otPlatEntropyGet(reinterpret_cast<uint8_t *>(aOutput), static_cast<uint16_t>(aInLen)));
    rval = 0;

    VerifyOrExit(aOutLen != NULL);
    *aOutLen = aInLen;

exit:
    OT_UNUSED_VARIABLE(aData);
    return rval;
}

//-------------------------------------------------------------------
// CryptoCtrDrbg

void RandomManager::CryptoCtrDrbg::Init(void)
{
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, RandomManager::GetMbedTlsEntropyContext(), NULL, 0);
}

void RandomManager::CryptoCtrDrbg::Deinit(void)
{
    mbedtls_ctr_drbg_free(&mCtrDrbg);
}

otError RandomManager::CryptoCtrDrbg::FillBuffer(uint8_t *aBuffer, uint16_t aSize)
{
    return ot::Crypto::MbedTls::MapError(
        mbedtls_ctr_drbg_random(&mCtrDrbg, static_cast<unsigned char *>(aBuffer), static_cast<size_t>(aSize)));
}

#endif // #ifndef OPENTHREAD_RADIO

} // namespace ot
