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
 *   This file implements a random number generator.
 *
 */

#include "openthread/platform/random.h"
#include "fsl_device_registers.h"
#include "fsl_rng.h"
#include <stdint.h>
#include <stdlib.h>
#include <utils/code_utils.h>


void JN5189RandomInit(void)
{
    trng_config_t config;
    uint32_t      seed;

    TRNG_GetDefaultConfig(&config);
    config.mode = trng_FreeRunning;

    otEXPECT(TRNG_Init(RNG, &config) == kStatus_Success);

    otEXPECT(TRNG_GetRandomData(RNG, &seed, sizeof(seed)) == kStatus_Success);

    srand(seed);

exit:
    return;

}

uint32_t otPlatRandomGet(void)
{
    return (uint32_t)rand();
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError status = OT_ERROR_NONE;

    otEXPECT_ACTION((aOutput != NULL), status = OT_ERROR_INVALID_ARGS);

    otEXPECT_ACTION(TRNG_GetRandomData(RNG, aOutput, aOutputLength) == kStatus_Success, status = OT_ERROR_FAILED);

exit:
    return status;
}
