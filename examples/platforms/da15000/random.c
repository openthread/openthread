/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include <platform/random.h>
#include "platform-da15000.h"
#include "black_orca.h"
#include "hw_trng.h"
#include <string.h>



#define HW_TRNG_RAM             (0x40040000)

static uint32_t s_state = 1;
static uint32_t seed;

int zhal_get_entropy(uint8_t *outEntropy, size_t inSize)
{
    uint32_t            randword;
    unsigned char       *pbuf           = outEntropy;
    const size_t        req_words       = inSize >> 2;
    const unsigned char *pbuf_end       = outEntropy + inSize;
    const unsigned char *preq_words_end = (unsigned char *)(((uint32_t *)outEntropy) + req_words);
    ptrdiff_t           remainder_bytes = pbuf_end - preq_words_end;

    hw_trng_enable(NULL);

    for (pbuf = outEntropy; pbuf < preq_words_end; pbuf += 4)
    {
        /* Wait for a random word to become available in the TRNG FIFO. */
        while ((TRNG->TRNG_FIFOLVL_REG & TRNG_TRNG_FIFOLVL_REG_TRNG_FIFOLVL_Msk) == 0);

        randword = *((volatile uint32_t *)HW_TRNG_RAM);
        memcpy(pbuf, &randword, 4);
    }

    if (remainder_bytes)
    {
        while ((TRNG->TRNG_FIFOLVL_REG & TRNG_TRNG_FIFOLVL_REG_TRNG_FIFOLVL_Msk) == 0);

        randword = *((volatile uint32_t *)HW_TRNG_RAM);
        memcpy(pbuf, &randword, remainder_bytes);
    }

    hw_trng_disable();

    return 0;
}

void da15000RandomInit(void)
{
    zhal_get_entropy((uint8_t *)&seed, sizeof(seed));
    s_state = seed;
}

uint32_t otPlatRandomGet(void)
{
    uint32_t mlcg;
    zhal_get_entropy((uint8_t *)&mlcg, sizeof(mlcg));
    return mlcg;
}

ThreadError otPlatRandomSecureGet(uint16_t aInputLength, uint8_t *aOutput, uint16_t *aOutputLength)
{

    for (uint16_t length = 0; length < aInputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

    *aOutputLength = aInputLength;

    return kThreadError_None;
}
