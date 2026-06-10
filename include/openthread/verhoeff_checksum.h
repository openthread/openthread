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
 *   This file defines APIs for Verhoeff checksum calculation and validation.
 */

#ifndef OPENTHREAD_VERHOEFF_CHECKSUM_H_
#define OPENTHREAD_VERHOEFF_CHECKSUM_H_

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-verhoeff-checksum
 *
 * @brief
 *   This module includes functions for Verhoeff checksum calculation and validation.
 *
 *   The functions in this module are available when `OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE` is enabled.
 *
 * @{
 */

/**
 * Specifies the maximum length of decimal string input in `otVerhoeffChecksum` functions.
 */
#define OT_VERHOEFF_CHECKSUM_MAX_STRING_LENGTH 128

/**
 * Calculates the Verhoeff checksum for a given decimal string.
 *
 * Requires `OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE`.
 *
 * @param[in]  aDecimalString   The string containing decimal digits.
 * @param[out] aChecksum        Pointer to a `char` to return the calculated checksum.
 *
 * @retval OT_ERROR_NONE            Successfully calculated the checksum, @p aChecksum is updated.
 * @retval OT_ERROR_INVALID_ARGS    The @p aDecimalString is not valid, i.e. it either contains chars other than
 *                                  ['0'-'9'], or is longer than `OT_VERHOEFF_CHECKSUM_MAX_STRING_LENGTH`.
 */
otError otVerhoeffChecksumCalculate(const char *aDecimalString, char *aChecksum);

/**
 * Validates the Verhoeff checksum for a given decimal string.
 *
 * Requires `OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE`.
 *
 * @param[in] aDecimalString   The string containing decimal digits (last char is treated as checksum).
 *
 * @retval OT_ERROR_NONE            Successfully validated the checksum in @p aDecimalString.
 * @retval OT_ERROR_FAILED          Checksum validation failed.
 * @retval OT_ERROR_INVALID_ARGS    The @p aDecimalString is not valid, i.e. it either contains chars other than
 *                                  ['0'-'9'], or is longer than `OT_VERHOEFF_CHECKSUM_MAX_STRING_LENGTH`.
 */
otError otVerhoeffChecksumValidate(const char *aDecimalString);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_VERHOEFF_CHECKSUM_H_
