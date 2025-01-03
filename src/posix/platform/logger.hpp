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
 *   This file implements the `Logger` class for use by POSIX platform module.
 */

#ifndef OT_POSIX_PLATFORM_LOGGER_HPP_
#define OT_POSIX_PLATFORM_LOGGER_HPP_

#include "openthread-posix-config.h"

#include <openthread/logging.h>

namespace ot {
namespace Posix {

/**
 * Provides logging methods for a specific POSIX module.
 *
 * The `Type` class MUST provide a `static const char kLogModuleName[]` which specifies the POSIX log module name to
 * include in the platform logs (using `otLogPlatArgs()`).
 *
 * Users of this class should follow CRTP-style inheritance, i.e., the `Type` class itself should inherit from
 * `Logger<Type>`.
 */
template <typename Type> class Logger
{
public:
    /**
     * Emits a log message at critical log level.
     *
     * @param[in]  aFormat  The format string.
     * @param[in]  ...      Arguments for the format specification.
     */
    static void LogCrit(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2)
    {
        va_list args;

        va_start(args, aFormat);
        otLogPlatArgs(OT_LOG_LEVEL_CRIT, Type::kLogModuleName, aFormat, args);
        va_end(args);
    }

    /**
     * Emits a log message at warning log level.
     *
     * @param[in]  aFormat  The format string.
     * @param[in]  ...      Arguments for the format specification.
     */
    static void LogWarn(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2)
    {
        va_list args;

        va_start(args, aFormat);
        otLogPlatArgs(OT_LOG_LEVEL_WARN, Type::kLogModuleName, aFormat, args);
        va_end(args);
    }

    /**
     * Emits a log message at note log level.
     *
     * @param[in]  aFormat  The format string.
     * @param[in]  ...      Arguments for the format specification.
     */
    static void LogNote(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2)
    {
        va_list args;

        va_start(args, aFormat);
        otLogPlatArgs(OT_LOG_LEVEL_NOTE, Type::kLogModuleName, aFormat, args);
        va_end(args);
    }

    /**
     * Emits a log message at info log level.
     *
     * @param[in]  aFormat  The format string.
     * @param[in]  ...      Arguments for the format specification.
     */
    static void LogInfo(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2)
    {
        va_list args;

        va_start(args, aFormat);
        otLogPlatArgs(OT_LOG_LEVEL_INFO, Type::kLogModuleName, aFormat, args);
        va_end(args);
    }

    /**
     * Emits a log message at debug log level.
     *
     * @param[in]  aFormat  The format string.
     * @param[in]  ...      Arguments for the format specification.
     */
    static void LogDebg(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(1, 2)
    {
        va_list args;

        va_start(args, aFormat);
        otLogPlatArgs(OT_LOG_LEVEL_DEBG, Type::kLogModuleName, aFormat, args);
        va_end(args);
    }
};

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_PLATFORM_LOGGER_HPP_
