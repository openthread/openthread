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
 *   This file implements an entropy source.
 *
 */

#include "platform-b91.h"
#include <openthread/platform/entropy.h>
#include <openthread/platform/radio.h>
#include "utils/code_utils.h"

void b91RandomInit(void)
{
    trng_init();
}

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    uint16_t i;
    uint16_t temp;
    uint32_t rn;

    if ((aOutput == NULL) || (aOutputLength == 0))
    {
        return OT_ERROR_INVALID_ARGS;
    }

    temp = aOutputLength / 4;
    for (i = 0; i < temp; i++)
    {
        rn                         = trng_rand();
        *((unsigned int *)aOutput) = rn;
        aOutput += 4;
    }

    aOutputLength -= temp * 4;
    if (aOutputLength > 0)
    {
        rn = trng_rand();
        for (i = 0; i < aOutputLength; i++)
        {
            *aOutput = rn;
            aOutput++;
            rn >>= 8;
        }
    }

    return OT_ERROR_NONE;
}
