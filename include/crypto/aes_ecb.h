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
 * @brief
 *   This file includes the platform abstraction for AES ECB computations.
 */

#ifndef AES_ECB_H_
#define AES_ECB_H_

#ifndef OPEN_THREAD_DRIVER
#include <stdint.h>
#endif

#include <openthread-types.h>
#include <cryptocontext.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup core-security
 *
 * @{
 *
 */

enum
{
    otAesBlockSize = 16,  ///< AES-128 block size.
};

/**
 * This method sets the key.
 *
 * @param[in]  aKey        A pointer to the key.
 * @param[in]  aKeyLength  Length of the key in bytes.
 *
 */
void otCryptoAesEcbSetKey(otCryptoContext *aCryptoContext, const void *aKey, uint16_t aKeyLength);

/**
 * This method encrypts data.
 *
 * @param[in]   aInput   A pointer to the input.
 * @param[out]  aOutput  A pointer to the output.
 *
 */
void otCryptoAesEcbEncrypt(otCryptoContext *aCryptoContext, const uint8_t aInput[otAesBlockSize],
                           uint8_t aOutput[otAesBlockSize]);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // AES_ECB_H_
