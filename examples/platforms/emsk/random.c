/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *
 */

#include <common/code_utils.hpp>

#include "openthread/platform/random.h"
#include "openthread/platform/radio.h"
#include "platform-emsk.h"

#include <stdlib.h>

#include <stdio.h>

static unsigned int seed;

void emskRandomInit(void)
{
    unsigned int i;

    printf("Node No.:");
    scanf("%d", &i);
    printf("%d\n", i);
    seed = 10 + i;
    srand(seed);
}

uint32_t otPlatRandomGet(void)
{

    return (uint32_t)rand();
}

ThreadError otPlatRandomSecureGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
{
    ThreadError error = kThreadError_None;
    uint8_t channel = 0;

    otEXPECT_ACTION(aOutput && aOutputLength, error = kThreadError_InvalidArgs);

    /* disable radio*/
    if (otPlatRadioIsEnabled(sInstance))
    {
        channel = (mrf24j40_read_long_ctrl_reg(MRF24J40_RFCON0) >> 4) + 11;
        otPlatRadioSleep(sInstance);
        otPlatRadioDisable(sInstance);
    }


    /* generate random number */
    for (uint16_t length = 0; length < aInputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

    *aOutputLength = aInputLength;

    /* enable radio*/
    if (channel)
    {
        emskRadioInit();
        otPlatRadioEnable(sInstance);
        otPlatRadioReceive(sInstance, channel);
    }

exit:
    return error;
}
