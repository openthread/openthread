/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes definitions for the SRP Coder API used to decode an encoded (compact) SRP message.
 */

#ifndef OPENTHREAD_SRP_CODER_H_
#define OPENTHREAD_SRP_CODER_H_

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-srp
 *
 * @brief
 *   This module includes functions for the SRP Coder to decode an encoded (compact) SRP message.
 *
 * @{
 *
 */

/**
 * Checks whether a given SRP message is encoded.
 *
 * Requires `OPENTHREAD_CONFIG_SRP_CODER_ENABLE`.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aBuffer    A buffer containing the message.
 * @param[in] aLength    Number of bytes in @p aBuffer.
 *
 * @retval TRUE   The message is an encoded SRP message.
 * @retval FALSE  The message is not an encoded SRP message.
 *
 */
bool otSrpCoderIsMessageEncoded(otInstance *aInstance, const uint8_t *aBuffer, uint16_t aLength);

/**
 * Decodes an encoded SRP message, reconstructing the original SRP message.
 *
 * On successful decoding, this function allocates and returns a new `otMessage` instance. The caller takes ownership
 * of this `otMessage` and MUST free it using `otMessageFree()` when it is no longer needed.
 *
 * The `aError` parameter is a pointer to an `otError` variable where the decoding result will be returned. It can be
 * set to `NULL` if the caller doesn't require the specific error code. Possible errors include:
 *
 * - `OT_ERROR_NONE`:         The coded message was decoded successfully.
 * - `OT_ERROR_INVALID_ARGS`: The message is not an encoded SRP message.
 * - `OT_ERROR_PARSE`:        Failed to parse the coded message (invalid format).
 * - `OT_ERROR_NO_BUFS`:      Insufficient memory to allocate the decoded message.
 *
 * @param[in]  aInstance   The OpenThread instance.
 * @param[in]  aBuffer     A buffer containing the encoded message to decode.
 * @param[in]  aLength     The length of the encoded message (in bytes) within @p aBuffer.
 * @param[out] aError      A pointer to an `otError` to return the decoding result. Can be `NULL`.
 *
 * @returns A pointer to the decoded `otMessage` (ownership transferred to the caller), or `NULL` if an error occurs.
 *
 */
otMessage *otSrpCoderDecode(otInstance *aInstance, const uint8_t *aBuffer, uint16_t aLength, otError *aError);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_SRP_CODER_H_
