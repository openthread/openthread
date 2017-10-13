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
 * @brief
 *  This file defines utility encoding and decoding functions.
 */

#ifndef OPENTHREAD_ENCODING_H_
#define OPENTHREAD_ENCODING_H_

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This functions encodes binary input data into base32-thread format string.
 *
 * @param[in]     aInput         The input binary data.
 * @param[in]     aInputLength   The length of the input data.
 * @param[out]    aOutput        The point to the output buffer.
 * @param[in,out] aOutputLength  The length of the output buffer (in) and encoded string (out).
 *
 */
otError otBase32Encode(const uint8_t *aInput, uint32_t aInputLength, char *aOutput, uint32_t *aOutputLength);

/**
 * This functions decodes base32-thread encoded string into binary data.
 *
 * @param[in]     aInput         The input base32-thread encoded string.
 * @param[out]    aOutput        The pointer to the output buffer.
 * @param[in,out] aOutputLength  The length of the output buffer (in) and data (out).
 *
 */
otError otBase32Decode(const char *aInput, uint8_t *aOutput, uint32_t *aOutputLength);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* OPENTHREAD_ENCODING_H_ */
