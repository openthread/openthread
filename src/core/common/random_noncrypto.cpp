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
 *   This file provides an implementation of software random number generator.
 */

#include "random.hpp"

#include "debug.hpp"
#include "entropy.hpp"

namespace ot {
namespace Random {
namespace NonCrypto {

static uint32_t sState;
static bool     sSeeded;

uint32_t GetUint32(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    assert(sSeeded);

    tmpstate = (uint64_t)33614 * (uint64_t)sState;
    q        = tmpstate & 0xffffffff;
    q        = q >> 1;
    p        = tmpstate >> 32;
    mlcg     = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    sState = mlcg;

    return mlcg;
}

void Seed(uint32_t aSeed)
{
    sSeeded = true;
    sState  = aSeed;
}

} // namespace NonCrypto
} // namespace Random
} // namespace ot
