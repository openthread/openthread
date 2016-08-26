/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements a pseudo-random number generator.
 *
 * @warning
 *   This implementation is not a true random number generator and does @em satisfy the Thread requirements.
 */

#include <openthread-types.h>
#include <platform/random.h>
#include "platform-posix.h"

static uint32_t s_state = 1;

void posixRandomInit(void)
{
    s_state = NODE_ID;
}

uint32_t otPlatRandomGet(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    tmpstate = (uint64_t)33614 * (uint64_t)s_state;
    q = tmpstate & 0xffffffff;
    q = q >> 1;
    p = tmpstate >> 32;
    mlcg = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    s_state = mlcg;

    return mlcg;
}

ThreadError otPlatSecureRandomGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
{
    uint16_t length = 0;

    while (length < aInputLength)
    {
        aOutput[length++] = (uint8_t)otPlatRandomGet();
    }

    *aOutputLength = aInputLength;

    return kThreadError_None;
}
