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
 *   This file provides an implementation of entropy source.
 */

#include "entropy.hpp"

#include <openthread/platform/entropy.h>
#include <openthread/platform/toolchain.h>

#include "code_utils.hpp"
#include "debug.hpp"

#ifndef OPENTHREAD_RADIO

#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>

#endif // OPENTHREAD_RADIO

namespace ot {
namespace Entropy {

static uint32_t sInitCnt = 0;

#ifndef OPENTHREAD_RADIO
static mbedtls_entropy_context sEntropy;

static int HandleMbedtlsEntropyPoll(void *aData, unsigned char *aOutput, size_t aInLen, size_t *aOutLen)
{
    OT_UNUSED_VARIABLE(aData);

    otError error;
    int     rval = 0;

    error = otPlatEntropyGet(reinterpret_cast<uint8_t *>(aOutput), static_cast<uint16_t>(aInLen));
    SuccessOrExit(error);

    if (aOutLen != NULL)
    {
        *aOutLen = aInLen;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        rval = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    return rval;
}

#endif // OPENTHREAD_RADIO

void Init(void)
{
    assert(sInitCnt < 0xffffffff);

#ifndef OPENTHREAD_RADIO
    if (sInitCnt == 0)
    {
        mbedtls_entropy_init(&sEntropy);
        mbedtls_entropy_add_source(&sEntropy, &HandleMbedtlsEntropyPoll, NULL, MBEDTLS_ENTROPY_MIN_HARDWARE,
                                   MBEDTLS_ENTROPY_SOURCE_STRONG);
    }
#endif // OPENTHREAD_RADIO

    sInitCnt++;
}

void Deinit(void)
{
    assert(sInitCnt > 0);

    sInitCnt--;

#ifndef OPENTHREAD_RADIO
    if (sInitCnt == 0)
    {
        mbedtls_entropy_free(&sEntropy);
    }
#endif // OPENTHREAD_RADIO
}

#ifndef OPENTHREAD_RADIO

mbedtls_entropy_context *MbedTlsContextGet(void)
{
    assert(sInitCnt > 0);
    return &sEntropy;
}

#endif // OPENTHREAD_RADIO

otError GetUint32(uint32_t &aVal)
{
    assert(sInitCnt > 0);
    return otPlatEntropyGet(reinterpret_cast<uint8_t *>(&aVal), sizeof(aVal));
}

} // namespace Entropy
} // namespace ot
