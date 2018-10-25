/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *  This file defines OpenThread String class.
 */

#ifndef STRING_HPP_
#define STRING_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "utils/wrap_string.h"

#include <openthread/error.h>

#include "common/code_utils.hpp"

namespace ot {

/**
 * @addtogroup core-string
 *
 * @brief
 *   This module includes definitions for OpenThread String class.
 *
 * @{
 *
 */

/**
 * This class defines the base class for `String`.
 *
 */
class StringBase
{
protected:
    /**
     * This method appends `printf()` style formatted data to a given character string buffer.
     *
     * @param[in]    aBuffer  A pointer to buffer containing the string.
     * @param[in]    aSize    The size of the buffer (in bytes).
     * @param[inout] aLength  A reference to variable containing current length of string. On exit length is updated.
     * @param[in]    aFormat  A pointer to the format string.
     * @param[in]    aArgs    Arguments for the format specification.
     *
     * @retval OT_ERROR_NONE           Updated the string successfully.
     * @retval OT_ERROR_NO_BUFS        String could not fit in the storage.
     * @retval OT_ERROR_INVALID_ARGS   Arguments do not match the format string.
     */
    static otError Write(char *aBuffer, uint16_t aSize, uint16_t &aLength, const char *aFormat, va_list aArgs);
};

/**
 * This class defines a fixed-size string.
 *
 */
template <uint16_t SIZE> class String : private StringBase
{
public:
    enum
    {
        kSize = SIZE, ///< Size (number of characters) in the buffer storage for the string.
    };

    /**
     * This constructor initializes the `String` object as empty.
     *
     */
    String(void)
        : mLength(0)
    {
        mBuffer[0] = 0;
    }

    /**
     * This constructor initializes the `String` object using `printf()` style formatted data.
     *
     * @param[in] aFormat    A pointer to the format string.
     * @param[in] ...        Arguments for the format specification.
     *
     */
    String(const char *aFormat, ...)
        : mLength(0)
    {
        va_list args;
        va_start(args, aFormat);
        Write(mBuffer, kSize, mLength, aFormat, args);
        va_end(args);
    }

    /**
     * This method clears the string.
     *
     */
    void Clear(void)
    {
        mBuffer[0] = 0;
        mLength    = 0;
    }

    /**
     * This method gets the length of the string.
     *
     * Similar to `strlen()` the length does not include the null character at the end of the string.
     *
     * @returns The string length.
     *
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * This method returns the size (number of chars) in the buffer storage for the string.
     *
     * @returns The size of the buffer storage for the string.
     *
     */
    uint16_t GetSize(void) const { return kSize; }

    /**
     * This method returns the string as a null-terminated C string.
     *
     * @returns The null-terminated C string.
     *
     */
    const char *AsCString(void) const { return mBuffer; }

    /**
     * This method sets the string using `printf()` style formatted data.
     *
     * @param[in] aFormat    A pointer to the format string.
     * @param[in] ...        Arguments for the format specification.
     *
     * @retval OT_ERROR_NONE           Updated the string successfully.
     * @retval OT_ERROR_NO_BUFS        String could not fit in the storage.
     * @retval OT_ERROR_INVALID_ARGS   Arguments do not match the format string.
     *
     */
    otError Set(const char *aFormat, ...)
    {
        va_list args;
        otError error;

        va_start(args, aFormat);
        mLength = 0;
        error   = Write(mBuffer, kSize, mLength, aFormat, args);
        va_end(args);

        return error;
    }

    /**
     * This method appends `printf()` style formatted data to the `String` object.
     *
     * @param[in] aFormat    A pointer to the format string.
     * @param[in] ...        Arguments for the format specification.
     *
     * @retval OT_ERROR_NONE           Updated the string successfully.
     * @retval OT_ERROR_NO_BUFS        String could not fit in the storage.
     * @retval OT_ERROR_INVALID_ARGS   Arguments do not match the format string.
     *
     */
    otError Append(const char *aFormat, ...)
    {
        va_list args;
        otError error;

        va_start(args, aFormat);
        error = Write(mBuffer, kSize, mLength, aFormat, args);
        va_end(args);

        return error;
    }

private:
    uint16_t mLength;
    char     mBuffer[kSize];
};

/**
 * @}
 *
 */

} // namespace ot

#endif // STRING_HPP_
