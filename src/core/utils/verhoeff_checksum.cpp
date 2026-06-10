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
 *   This file implements Verhoeff checksum calculation and validation.
 */

#include "verhoeff_checksum.hpp"

#if OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE

#include "common/code_utils.hpp"
#include "common/string.hpp"

namespace ot {
namespace Utils {

uint8_t VerhoeffChecksum::Lookup(uint8_t aIndex, const uint8_t aCompressedArray[])
{
    // The values in the array are [0-9]. To save space, two
    // entries are saved as a single byte in @p aCompressedArray,
    // such that higher 4-bit corresponds to one entry, and the
    // lower 4-bit to the next entry.

    uint8_t result = aCompressedArray[aIndex / 2];

    if ((aIndex & 1) == 0)
    {
        result >>= 4;
    }
    else
    {
        result &= 0x0f;
    }

    return result;
}

uint8_t VerhoeffChecksum::Multiply(uint8_t aFirst, uint8_t aSecond)
{
    static uint8_t kMultiplication[][5] = {{0x01, 0x23, 0x45, 0x67, 0x89}, {0x12, 0x34, 0x06, 0x78, 0x95},
                                           {0x23, 0x40, 0x17, 0x89, 0x56}, {0x34, 0x01, 0x28, 0x95, 0x67},
                                           {0x40, 0x12, 0x39, 0x56, 0x78}, {0x59, 0x87, 0x60, 0x43, 0x21},
                                           {0x65, 0x98, 0x71, 0x04, 0x32}, {0x76, 0x59, 0x82, 0x10, 0x43},
                                           {0x87, 0x65, 0x93, 0x21, 0x04}, {0x98, 0x76, 0x54, 0x32, 0x10}};

    return Lookup(aSecond, kMultiplication[aFirst]);
}

uint8_t VerhoeffChecksum::Permute(uint8_t aPosition, uint8_t aValue)
{
    static uint8_t kPermutation[][5] = {{0x01, 0x23, 0x45, 0x67, 0x89}, {0x15, 0x76, 0x28, 0x30, 0x94},
                                        {0x58, 0x03, 0x79, 0x61, 0x42}, {0x89, 0x16, 0x04, 0x35, 0x27},
                                        {0x94, 0x53, 0x12, 0x68, 0x70}, {0x42, 0x86, 0x57, 0x39, 0x01},
                                        {0x27, 0x93, 0x80, 0x64, 0x15}, {0x70, 0x46, 0x91, 0x32, 0x58}};

    return Lookup(aValue, kPermutation[aPosition]);
}

uint8_t VerhoeffChecksum::InverseOf(uint8_t aValue)
{
    static uint8_t kInverse[] = {0x04, 0x32, 0x15, 0x67, 0x89};

    return Lookup(aValue, kInverse);
}

Error VerhoeffChecksum::Calculate(const char *aDecimalString, char &aChecksum)
{
    Error   error;
    uint8_t code;

    SuccessOrExit(error = ComputeCode(aDecimalString, code, /* aValidate */ false));
    aChecksum = static_cast<char>('0' + InverseOf(code));

exit:
    return error;
}

Error VerhoeffChecksum::Validate(const char *aDecimalString)
{
    Error   error;
    uint8_t code;

    SuccessOrExit(error = ComputeCode(aDecimalString, code, /* aValidate */ true));
    VerifyOrExit(code == 0, error = kErrorFailed);

exit:
    return error;
}

Error VerhoeffChecksum::ComputeCode(const char *aDecimalString, uint8_t &aCode, bool aValidate)
{
    Error    error  = kErrorNone;
    uint8_t  code   = 0;
    uint16_t index  = 0;
    uint16_t length = StringLength(aDecimalString, kMaxStringLength + 1);

    VerifyOrExit(length <= kMaxStringLength, error = kErrorInvalidArgs);

    if (!aValidate)
    {
        length++;
        index++;
    }

    for (; index < length; ++index)
    {
        char digit = aDecimalString[length - index - 1];

        VerifyOrExit(digit >= '0' && digit <= '9', error = kErrorInvalidArgs);
        code = Multiply(code, Permute(index % 8, static_cast<uint8_t>(digit - '0')));
    }

    aCode = code;

exit:
    return error;
}

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE
