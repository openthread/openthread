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
 */

#include <assert.h>
#include <stdio.h>

#include "platform-posix.h"

#include <openthread/types.h>
#include <openthread/platform/random.h>

#include "code_utils.h"

static uint32_t sState = 1;

void platformRandomInit(void)
{
#if __SANITIZE_ADDRESS__ == 0

    otError error;

    error = otPlatRandomGetTrue((uint8_t *)&sState, sizeof(sState));
    assert(error == OT_ERROR_NONE);

#else // __SANITIZE_ADDRESS__

    // Multiplying NODE_ID assures that no two nodes gets the same seed within an hour.
    sState = (uint32_t)time(NULL) + (3600 * NODE_ID);

#endif // __SANITIZE_ADDRESS__
}

uint32_t otPlatRandomGet(void)
{
    uint32_t mlcg, p, q;
    uint64_t tmpstate;

    tmpstate = (uint64_t)33614 * (uint64_t)sState;
    q        = tmpstate & 0xffffffff;
    q        = q >> 1;
    p        = tmpstate >> 32;
    mlcg     = p + q;

    if (mlcg & 0x80000000)
    {
        mlcg &= 0x7fffffff;
        mlcg++;
    }

    sState = mlcg;

    return mlcg;
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

#if __SANITIZE_ADDRESS__ == 0

    FILE * file = NULL;
    size_t readLength;

    otEXPECT_ACTION(aOutput && aOutputLength, error = OT_ERROR_INVALID_ARGS);

    file = fopen("/dev/urandom", "rb");
    otEXPECT_ACTION(file != NULL, error = OT_ERROR_FAILED);

    readLength = fread(aOutput, 1, aOutputLength, file);
    otEXPECT_ACTION(readLength == aOutputLength, error = OT_ERROR_FAILED);

exit:

    if (file != NULL)
    {
        fclose(file);
    }

#else // __SANITIZE_ADDRESS__

    /*
     * THE IMPLEMENTATION BELOW IS NOT COMPLIANT WITH THE THREAD SPECIFICATION.
     *
     * Address Sanitizer triggers test failures when reading random
     * values from /dev/urandom.  The pseudo-random number generator
     * implementation below is only used to enable continuous
     * integration checks with Address Sanitizer enabled.
     */
    otEXPECT_ACTION(aOutput && aOutputLength, error = OT_ERROR_INVALID_ARGS);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)otPlatRandomGet();
    }

exit:

#endif // __SANITIZE_ADDRESS__

    return error;
}
