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
 *   This file implements an entropy source based on TRNG.
 *
 */

#include <openthread/platform/entropy.h>

#include <openthread/config.h>

#include <stddef.h>

#include <utils/code_utils.h>

#include <driverlib/prcm.h>
#include <driverlib/trng.h>

enum
{
    CC1352_TRNG_MIN_SAMPLES_PER_CYCLE = (1 << 6),
    CC1352_TRNG_MAX_SAMPLES_PER_CYCLE = (1 << 24),
    CC1352_TRNG_CLOCKS_PER_SAMPLE     = 0,
};

/**
 * \note if more than 32 bits of entropy are needed, the TRNG core produces
 * 64 bits of random data, we just ignore the upper 32 bytes
 */

/**
 * Function documented in platform-cc1352.h
 */
void cc1352RandomInit(void)
{
    PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

    while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) != PRCM_DOMAIN_POWER_ON)
        ;

    PRCMPeripheralRunEnable(PRCM_PERIPH_TRNG);
    PRCMPeripheralSleepEnable(PRCM_DOMAIN_PERIPH);
    PRCMPeripheralDeepSleepEnable(PRCM_DOMAIN_PERIPH);
    PRCMLoadSet();
    TRNGConfigure(CC1352_TRNG_MIN_SAMPLES_PER_CYCLE, CC1352_TRNG_MAX_SAMPLES_PER_CYCLE, CC1352_TRNG_CLOCKS_PER_SAMPLE);
    TRNGEnable();
}

/**
 * Fill an arbitrary area with random data.
 *
 * @param [out] aOutput Area to place the random data.
 * @param [in]  aLen    Size of the area to place random data.
 * @param [out] oLen    How much of the output was written to.
 *
 * @return Indication of error.
 * @retval 0 No error occurred.
 */
static int TRNGPoll(unsigned char *aOutput, size_t aLen)
{
    size_t length = 0;
    union
    {
        uint32_t u32[2];
        uint8_t  u8[8];
    } buffer;

    while (length < aLen)
    {
        if (length % 8 == 0)
        {
            /* we've run to the end of the buffer */
            while (!(TRNGStatusGet() & TRNG_NUMBER_READY))
                ;

            /*
             * don't use TRNGNumberGet here because it will tell the TRNG to
             * refill the entropy pool, instead we do it ourself.
             */
            buffer.u32[0]                        = HWREG(TRNG_BASE + TRNG_O_OUT0);
            buffer.u32[1]                        = HWREG(TRNG_BASE + TRNG_O_OUT1);
            HWREG(TRNG_BASE + TRNG_O_IRQFLAGCLR) = 0x1;
        }

        aOutput[length] = buffer.u8[length % 8];

        length++;
    }

    return 0;
}

/**
 * Function documented in platform/entropy.h
 */
otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error  = OT_ERROR_NONE;
    size_t  length = aOutputLength;

    otEXPECT_ACTION(aOutput, error = OT_ERROR_INVALID_ARGS);

    otEXPECT_ACTION(TRNGPoll((unsigned char *)aOutput, length) == 0, error = OT_ERROR_FAILED);

exit:
    return error;
}
