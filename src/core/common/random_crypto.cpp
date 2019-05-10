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
 *   This file provides an implementation of cryptographic random number generator.
 */

#include "random.hpp"

#include "utils/wrap_stdint.h"

#include "entropy.hpp"
#include "crypto/mbedtls.hpp"

#include <mbedtls/entropy.h>

#include "debug.hpp"

namespace ot {
namespace Random {
namespace Crypto {

static uint32_t                 sInitCnt = 0;
static mbedtls_ctr_drbg_context sCtrDrbg;

void Init(void)
{
    assert(sInitCnt < 0xffffffff);

    if (sInitCnt == 0)
    {
        mbedtls_ctr_drbg_init(&sCtrDrbg);
        mbedtls_ctr_drbg_seed(&sCtrDrbg, mbedtls_entropy_func, Entropy::MbedTlsContextGet(), NULL, 0);
    }

    sInitCnt++;
}

void Deinit(void)
{
    assert(sInitCnt > 0);

    sInitCnt--;

    if (sInitCnt == 0)
    {
        mbedtls_ctr_drbg_free(&sCtrDrbg);
    }
}

mbedtls_ctr_drbg_context *MbedTlsContextGet(void)
{
    assert(sInitCnt > 0);
    return &sCtrDrbg;
}

otError FillBuffer(uint8_t *aBuffer, uint16_t aSize)
{
    int rval = 0;

    assert(sInitCnt > 0);

    rval = mbedtls_ctr_drbg_random(&sCtrDrbg, static_cast<unsigned char *>(aBuffer), static_cast<size_t>(aSize));

    return ot::Crypto::MbedTls::MapError(rval);
}

} // namespace Crypto
} // namespace Random
} // namespace ot
