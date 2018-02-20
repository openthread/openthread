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
 *   This file implements true random number generator.
 */

#include <string.h>

#include <openthread/platform/random.h>

#include "platform-da15000.h"

#include "hw_trng.h"
#include "sdk_defs.h"

#define RANDOM_SIZE_OF_BUFFER 32

static uint32_t sRandomNumbers[RANDOM_SIZE_OF_BUFFER];
static uint8_t  sRandomNextNumberIndex  = 0;
static bool     sRandomGeneratorStarted = false;

static void RandomCallback(void)
{
    hw_trng_get_numbers(sRandomNumbers, RANDOM_SIZE_OF_BUFFER);
    sRandomNextNumberIndex = 0;
    hw_trng_stop();
    hw_trng_disable_clk();
    hw_trng_disable_interrupt();
    sRandomGeneratorStarted = false;
}

static void StartGenerator(void)
{
    hw_trng_enable(RandomCallback);
    sRandomGeneratorStarted = true;
}

void da15000RandomInit(void)
{
    StartGenerator();
}

uint32_t otPlatRandomGet(void)
{
    uint32_t randomNumber;
    bool     randomGet = false;

    do
    {
        GLOBAL_INT_DISABLE();

        if (sRandomNextNumberIndex < RANDOM_SIZE_OF_BUFFER)
        {
            randomNumber = sRandomNumbers[sRandomNextNumberIndex++];
            randomGet    = true;

            if (sRandomNextNumberIndex == RANDOM_SIZE_OF_BUFFER)
            {
                StartGenerator();
            }
        }
        else if (hw_trng_get_fifo_level() > 0)
        {
            randomNumber = hw_trng_get_number();
            randomGet    = true;
        }
        else if (!sRandomGeneratorStarted)
        {
            StartGenerator();
        }

        GLOBAL_INT_RESTORE();
    } while (!randomGet);

    return randomNumber;
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    uint32_t randomNumber;

    while (aOutputLength)
    {
        randomNumber = otPlatRandomGet();

        if (aOutputLength < 4)
        {
            memcpy(aOutput, &randomNumber, aOutputLength);
            aOutputLength = 0;
        }
        else
        {
            memcpy(aOutput, &randomNumber, 4);
            aOutputLength -= 4;
            aOutput += 4;
        }
    }

    return OT_ERROR_NONE;
}
