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
 *   This file includes definitions for Verhoeff checksum calculation and validation.
 */

#ifndef VERHOEFF_CHECKSUM_HPP_
#define VERHOEFF_CHECKSUM_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

#include <openthread/verhoeff_checksum.h>

#include "common/error.hpp"

namespace ot {
namespace Utils {

class VerhoeffChecksum
{
public:
    /**
     * Specifies the maximum length of decimal string input.
     */
    static constexpr uint16_t kMaxStringLength = OT_VERHOEFF_CHECKSUM_MAX_STRING_LENGTH;

    /**
     * Calculates the Verhoeff checksum for a given decimal string.
     *
     *
     * @param[in] a DecimalString  The string containing decimal digits.
     * @param[out] aChecksum       Reference to a `char` to return the calculated checksum.
     *
     * @retval kErrorNone          Successfully calculated the checksum, @p aChecksum is updated.
     * @retval kErrorInvalidArgs   The @p aDecimalString is not valid, i.e. it either contains chars other than
     *                             ['0'-'9'], or is longer than `kMaxStringLength`.
     */
    static Error Calculate(const char *aDecimalString, char &aChecksum);

    /**
     * Validates the Verhoeff checksum for a given decimal string.
     *
     * @param[in] aDecimalString   The string containing decimal digits (last char is treated as checksum).
     *
     * @retval kErrorNone            Successfully validated the checksum in @p aDecimalString.
     * @retval kErrorFailed          Checksum is not valid.
     * @retval kErrorInvalidArgs     The @p aDecimalString is not valid, i.e. it either contains chars other than
     *                               ['0'-'9'], or is longer than `kMaxStringLength`.
     */
    static Error Validate(const char *aDecimalString);

    VerhoeffChecksum(void) = delete;

private:
    static Error   ComputeCode(const char *aDecimalString, uint8_t &aCode, bool aValidate);
    static uint8_t Lookup(uint8_t aIndex, const uint8_t aCompressedArray[]);
    static uint8_t Multiply(uint8_t aFirst, uint8_t aSecond);
    static uint8_t Permute(uint8_t aPosition, uint8_t aValue);
    static uint8_t InverseOf(uint8_t aValue);
};

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

#endif // VERHOEFF_CHECKSUM_HPP_
