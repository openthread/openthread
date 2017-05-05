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
 *   This file implements PBKDF2 using AES-CMAC-PRF-128
 */

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "pbkdf2_cmac.h"

#include "utils/wrap_string.h"

#include <mbedtls/cmac.h>

#if OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD

void otPbkdf2Cmac(
    const uint8_t *aPassword, uint16_t aPasswordLen,
    const uint8_t *aSalt, uint16_t aSaltLen,
    uint32_t aIterationCounter, uint16_t aKeyLen,
    uint8_t *aKey)
{
    uint32_t blockCounter = 0;
    uint16_t useLen = 0;
    uint16_t prfBlockLen = MBEDTLS_CIPHER_BLKSIZE_MAX;
    uint8_t prfInput[OT_PBKDF2_SALT_MAX_LEN + 4]; // Salt || INT(), for U1 calculation
    uint8_t prfOutput[MBEDTLS_CIPHER_BLKSIZE_MAX];
    uint8_t keyBlock[MBEDTLS_CIPHER_BLKSIZE_MAX];
    uint8_t *key = aKey;
    uint16_t keyLen = aKeyLen;

    while (keyLen)
    {
        memcpy(prfInput, aSalt, aSaltLen);

        blockCounter++;
        prfInput[aSaltLen + 0] = (uint8_t)(blockCounter >> 24);
        prfInput[aSaltLen + 1] = (uint8_t)(blockCounter >> 16);
        prfInput[aSaltLen + 2] = (uint8_t)(blockCounter >> 8);
        prfInput[aSaltLen + 3] = (uint8_t)(blockCounter);

        // Calculate U_1
        mbedtls_aes_cmac_prf_128(aPassword, aPasswordLen, prfInput, aSaltLen + 4, prfOutput);
        memcpy(keyBlock, prfOutput, prfBlockLen);

        for (uint32_t i = 1; i < aIterationCounter; i++)
        {
            memcpy(prfInput, prfOutput, prfBlockLen);

            // Calculate U_i
            mbedtls_aes_cmac_prf_128(aPassword, aPasswordLen, prfInput, prfBlockLen, prfOutput);

            // xor
            for (uint32_t j = 0; j < prfBlockLen; j++)
            {
                keyBlock[j] ^= prfOutput[j];
            }
        }

        useLen = (keyLen < prfBlockLen) ? keyLen : prfBlockLen;
        memcpy(key, keyBlock, useLen);
        key += useLen;
        keyLen -= useLen;
    }
}

#endif // OPENTHREAD_ENABLE_COMMISSIONER && OPENTHREAD_FTD
