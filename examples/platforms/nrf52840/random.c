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
 *   This file implements a random number generator.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <utils/code_utils.h>
#include <openthread/platform/random.h>

#include "platform-nrf5.h"

#if SOFTDEVICE_PRESENT
#include "softdevice.h"
#else
#include <hal/nrf_rng.h>

static uint8_t           sBuffer[RNG_BUFFER_SIZE];
static volatile uint32_t sReadPosition;
static volatile uint32_t sWritePosition;

static inline uint32_t bufferCount(void)
{
    uint32_t writePos = sWritePosition;
    return writePos - sReadPosition;
}

static inline bool bufferIsEmpty(void)
{
    return (bufferCount() == 0);
}

static inline bool bufferIsUint32Ready(void)
{
    return (bufferCount() >= 4);
}

static inline bool bufferIsFull(void)
{
    return (bufferCount() >= RNG_BUFFER_SIZE);
}

static inline void bufferPut(uint8_t val)
{
    if (!bufferIsFull())
    {
        sBuffer[(sWritePosition++) % RNG_BUFFER_SIZE] = val;
    }
}

static inline uint8_t bufferGet()
{
    uint8_t retVal = 0;

    if (!bufferIsEmpty())
    {
        retVal = sBuffer[sReadPosition++ % RNG_BUFFER_SIZE];
    }

    return retVal;
}

static inline uint32_t bufferGetUint32()
{
    uint32_t retVal = 0;

    if (bufferIsUint32Ready())
    {
        for (uint32_t i = 0; i < 4; i++)
        {
            retVal <<= 8;
            retVal |= bufferGet();
        }
    }

    return retVal;
}

static void generatorStart(void)
{
    nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);
    nrf_rng_int_enable(NRF_RNG_INT_VALRDY_MASK);
    nrf_rng_task_trigger(NRF_RNG_TASK_START);
}

static void generatorStop(void)
{
    nrf_rng_int_disable(NRF_RNG_INT_VALRDY_MASK);
    nrf_rng_task_trigger(NRF_RNG_TASK_STOP);
}

void RNG_IRQHandler(void)
{
    if (nrf_rng_event_get(NRF_RNG_EVENT_VALRDY) && nrf_rng_int_get(NRF_RNG_INT_VALRDY_MASK))
    {
        nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);

        bufferPut(nrf_rng_random_value_get());

        if (bufferIsFull())
        {
            generatorStop();
        }
    }
}
#endif // SOFTDEVICE_PRESENT

void nrf5RandomInit(void)
{
    uint32_t seed = 0;

#if SOFTDEVICE_PRESENT
    uint32_t retval;

    do
    {
        // Wait for the first randomized 4 bytes, to randomize software generator seed.
        retval = sd_rand_application_vector_get((uint8_t *)&seed, sizeof(seed));
    } while (retval != NRF_SUCCESS && seed == 0);

#else  // SOFTDEVICE_PRESENT
    memset(sBuffer, 0, sizeof(sBuffer));
    sReadPosition  = 0;
    sWritePosition = 0;

    NVIC_SetPriority(RNG_IRQn, RNG_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(RNG_IRQn);
    NVIC_EnableIRQ(RNG_IRQn);

    nrf_rng_error_correction_enable();
    nrf_rng_shorts_disable(NRF_RNG_SHORT_VALRDY_STOP_MASK);
    generatorStart();

    // Wait for the first randomized 4 bytes, to randomize software generator seed.
    while (!bufferIsUint32Ready())
        ;

    seed = bufferGetUint32();
#endif // SOFTDEVICE_PRESENT

    srand(seed);
}

void nrf5RandomDeinit(void)
{
#ifndef SOFTDEVICE_PRESENT
    generatorStop();

    NVIC_DisableIRQ(RNG_IRQn);
    NVIC_ClearPendingIRQ(RNG_IRQn);
    NVIC_SetPriority(RNG_IRQn, 0);
#endif // SOFTDEVICE_PRESENT
}

uint32_t otPlatRandomGet(void)
{
    return (uint32_t)rand();
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  copyLength;
    uint16_t index = 0;

    otEXPECT_ACTION(aOutput && aOutputLength, error = OT_ERROR_INVALID_ARGS);

    do
    {
#if SOFTDEVICE_PRESENT
        sd_rand_application_bytes_available_get(&copyLength);
#else  // SOFTDEVICE_PRESENT
        copyLength = (uint8_t)bufferCount();
#endif // SOFTDEVICE_PRESENT

        if (copyLength > aOutputLength - index)
        {
            copyLength = aOutputLength - index;
        }

        if (copyLength > 0)
        {
#if SOFTDEVICE_PRESENT
            uint32_t retval = sd_rand_application_vector_get(aOutput + index, copyLength);
            otEXPECT_ACTION(retval == NRF_SUCCESS, error = OT_ERROR_FAILED);
#else  // SOFTDEVICE_PRESENT

            for (uint32_t i = 0; i < copyLength; i++)
            {
                aOutput[i + index] = bufferGet();
            }

            generatorStart();
#endif // SOFTDEVICE_PRESENT

            index += copyLength;
        }
    } while (index < aOutputLength);

exit:
    return error;
}
