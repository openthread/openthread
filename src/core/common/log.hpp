/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes logging related  definitions.
 */

#ifndef LOG_HPP_
#define LOG_HPP_

#include "openthread-core-config.h"

#include <openthread/logging.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/toolchain.h>

#include "common/error.hpp"

namespace ot {

/**
 * @def OT_SHOULD_LOG
 *
 * This definition indicates whether or not logging is enabled.
 *
 */
#define OT_SHOULD_LOG (OPENTHREAD_CONFIG_LOG_OUTPUT != OPENTHREAD_CONFIG_LOG_OUTPUT_NONE)

/**
 * Indicates whether the OpenThread logging is enabled at a given log level.
 *
 * @param[in] aLevel   The log level to check.
 *
 * @returns TRUE if logging is enabled at @p aLevel, FALSE otherwise.
 *
 */
#define OT_SHOULD_LOG_AT(aLevel) (OT_SHOULD_LOG && (OPENTHREAD_CONFIG_LOG_LEVEL >= (aLevel)))

/**
 * Represents the log level.
 *
 */
enum LogLevel : uint8_t
{
    kLogLevelNone = OT_LOG_LEVEL_NONE, ///< None (disable logs)
    kLogLevelCrit = OT_LOG_LEVEL_CRIT, ///< Critical log level
    kLogLevelWarn = OT_LOG_LEVEL_WARN, ///< Warning log level
    kLogLevelNote = OT_LOG_LEVEL_NOTE, ///< Note log level
    kLogLevelInfo = OT_LOG_LEVEL_INFO, ///< Info log level
    kLogLevelDebg = OT_LOG_LEVEL_DEBG, ///< Debug log level
};

constexpr uint8_t kMaxLogModuleNameLength = 14; ///< Maximum module name length

#if OT_SHOULD_LOG && (OPENTHREAD_CONFIG_LOG_LEVEL != OT_LOG_LEVEL_NONE)
/**
 * Registers log module name.
 *
 * Is used in a `cpp` file to register the log module name for that file before using any other logging
 * functions or macros (e.g., `LogInfo()` or `DumpInfo()`, ...) in the file.
 *
 * @param[in] aName  The log module name string (MUST be shorter than `kMaxLogModuleNameLength`).
 *
 */
#define RegisterLogModule(aName)                                     \
    constexpr char kLogModuleName[] = aName;                         \
    namespace {                                                      \
    /* Defining this type to silence "unused constant" warning/error \
     * for `kLogModuleName` under any log level config.              \
     */                                                              \
    using DummyType = char[sizeof(kLogModuleName)];                  \
    }                                                                \
    static_assert(sizeof(kLogModuleName) <= kMaxLogModuleNameLength + 1, "Log module name is too long")

#else
#define RegisterLogModule(aName) static_assert(true, "Consume the required semi-colon at the end of macro")
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_CRIT)
/**
 * Emits a log message at critical log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogCrit(...) Logger::LogAtLevel<kLogLevelCrit>(kLogModuleName, __VA_ARGS__)
#else
#define LogCrit(...)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
/**
 * Emits a log message at warning log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogWarn(...) Logger::LogAtLevel<kLogLevelWarn>(kLogModuleName, __VA_ARGS__)
#else
#define LogWarn(...)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)
/**
 * Emits a log message at note log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogNote(...) Logger::LogAtLevel<kLogLevelNote>(kLogModuleName, __VA_ARGS__)
#else
#define LogNote(...)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
/**
 * Emits a log message at info log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogInfo(...) Logger::LogAtLevel<kLogLevelInfo>(kLogModuleName, __VA_ARGS__)
#else
#define LogInfo(...)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
/**
 * Emits a log message at debug log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogDebg(...) Logger::LogAtLevel<kLogLevelDebg>(kLogModuleName, __VA_ARGS__)
#else
#define LogDebg(...)
#endif

#if OT_SHOULD_LOG
/**
 * Emits a log message at a given log level.
 *
 * @param[in] aLogLevel  The log level to use.
 * @param[in] ...        Argument for the format specification.
 *
 */
#define LogAt(aLogLevel, ...) Logger::LogInModule(kLogModuleName, aLogLevel, __VA_ARGS__)
#else
#define LogAt(aLogLevel, ...)
#endif

#if OT_SHOULD_LOG
/**
 * Emits a log message independent of the configured log level.
 *
 * @param[in]  ...   Arguments for the format specification.
 *
 */
#define LogAlways(...) Logger::LogInModule("", kLogLevelNone, __VA_ARGS__)
#else
#define LogAlways(...)
#endif

#if OT_SHOULD_LOG && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
/**
 * Emit a log message for the certification test.
 *
 * @param[in]  ...  Arguments for the format specification.
 *
 */
#define LogCert(...) LogAlways(__VA_ARGS__)
#else
#define LogCert(...)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_CRIT) && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump at log level critical.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpCrit(aText, aData, aDataLength) Logger::Dump<kLogLevelCrit, kLogModuleName>(aText, aData, aDataLength)
#else
#define DumpCrit(aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN) && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump at log level warning.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpWarn(aText, aData, aDataLength) Logger::Dump<kLogLevelWarn, kLogModuleName>(aText, aData, aDataLength)
#else
#define DumpWarn(aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE) && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump at log level note.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpNote(aText, aData, aDataLength) Logger::Dump<kLogLevelNote, kLogModuleName>(aText, aData, aDataLength)
#else
#define DumpNote(aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO) && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump at log level info.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpInfo(aText, aData, aDataLength) Logger::Dump<kLogLevelInfo, kLogModuleName>(aText, aData, aDataLength)
#else
#define DumpInfo(aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG) && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump at log level debug.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpDebg(aText, aData, aDataLength) Logger::Dump<kLogLevelDebg, kLogModuleName>(aText, aData, aDataLength)
#else
#define DumpDebg(aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump independent of the configured log level.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpAlways(aText, aData, aDataLength) Logger::DumpInModule("", kLogLevelNone, aText, aData, aDataLength)
#endif

#if OT_SHOULD_LOG && OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE && OPENTHREAD_CONFIG_LOG_PKT_DUMP
/**
 * Generates a memory dump for certification test.
 *
 * @param[in]  aText         A string that is printed before the bytes.
 * @param[in]  aData         A pointer to the data buffer.
 * @param[in]  aDataLength   Number of bytes in @p aData.
 *
 */
#define DumpCert(aText, aData, aDataLength) DumpAlways(aText, aData, aDataLength)
#else
#define DumpCert(aText, aData, aDataLength)
#endif

//----------------------------------------------------------------------------------------------------------------------

#if OT_SHOULD_LOG

class Logger
{
    // The `Logger` class implements the logging methods.
    //
    // The `Logger` methods are not intended to be directly used
    // and instead the logging macros should be used.

public:
    static void LogInModule(const char *aModuleName, LogLevel aLogLevel, const char *aFormat, ...)
        OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(3, 4);

    template <LogLevel kLogLevel>
    static void LogAtLevel(const char *aModuleName, const char *aFormat, ...)
        OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

    static void LogVarArgs(const char *aModuleName, LogLevel aLogLevel, const char *aFormat, va_list aArgs);

#if OPENTHREAD_CONFIG_LOG_PKT_DUMP
    static constexpr uint8_t kStringLineLength = 80;
    static constexpr uint8_t kDumpBytesPerLine = 16;

    template <LogLevel kLogLevel, const char *kModuleName>
    static void Dump(const char *aText, const void *aData, uint16_t aDataLength)
    {
        DumpAtLevel<kLogLevel>(kModuleName, aText, aData, aDataLength);
    }

    static void DumpInModule(const char *aModuleName,
                             LogLevel    aLogLevel,
                             const char *aText,
                             const void *aData,
                             uint16_t    aDataLength);

    template <LogLevel kLogLevel>
    static void DumpAtLevel(const char *aModuleName, const char *aText, const void *aData, uint16_t aDataLength);
#endif
};

extern template void Logger::LogAtLevel<kLogLevelNone>(const char *aModuleName, const char *aFormat, ...);
extern template void Logger::LogAtLevel<kLogLevelCrit>(const char *aModuleName, const char *aFormat, ...);
extern template void Logger::LogAtLevel<kLogLevelWarn>(const char *aModuleName, const char *aFormat, ...);
extern template void Logger::LogAtLevel<kLogLevelNote>(const char *aModuleName, const char *aFormat, ...);
extern template void Logger::LogAtLevel<kLogLevelInfo>(const char *aModuleName, const char *aFormat, ...);
extern template void Logger::LogAtLevel<kLogLevelDebg>(const char *aModuleName, const char *aFormat, ...);

#if OPENTHREAD_CONFIG_LOG_PKT_DUMP
extern template void Logger::DumpAtLevel<kLogLevelNone>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
extern template void Logger::DumpAtLevel<kLogLevelCrit>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
extern template void Logger::DumpAtLevel<kLogLevelWarn>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
extern template void Logger::DumpAtLevel<kLogLevelNote>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
extern template void Logger::DumpAtLevel<kLogLevelInfo>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
extern template void Logger::DumpAtLevel<kLogLevelDebg>(const char *aModuleName,
                                                        const char *aText,
                                                        const void *aData,
                                                        uint16_t    aDataLength);
#endif // OPENTHREAD_CONFIG_LOG_PKT_DUMP
#endif // OT_SHOULD_LOG

typedef otLogHexDumpInfo HexDumpInfo; ///< Represents the hex dump info.

/**
 * Generates the next hex dump line.
 *
 * Can call this method back-to-back to generate the hex dump output line by line. On the first call the `mIterator`
 * field in @p aInfo MUST be set to zero.
 *
 * Here is an example of the generated hex dump output:
 *
 *  "==========================[{mTitle} len=070]============================"
 *  "| 41 D8 87 34 12 FF FF 25 | 4C 57 DA F2 FB 2F 62 7F | A..4...%LW.../b. |"
 *  "| 3B 01 F0 4D 4C 4D 4C 54 | 4F 00 15 15 00 00 00 00 | ;..MLMLTO....... |"
 *  "| 00 00 00 01 80 DB 60 82 | 7E 33 72 3B CC B3 A1 84 | ......`.~3r;.... |"
 *  "| 3B E6 AD B2 0B 45 E7 45 | C5 B9 00 1A CB 2D 6D 1C | ;....E.E.....-m. |"
 *  "| 10 3E 3C F5 D3 70       |                         | .><..p           |"
 *  "------------------------------------------------------------------------"
 *
 * @param[in,out] aInfo    A reference to a `LogHexDumpInfo` to use to generate hex dump.
 *
 * @retval kErrorNone      Successfully generated the next line, `mLine` field in @p aInfo is updated.
 * @retval kErrorNotFound  Reached the end and no more line to generate.
 *
 */
Error GenerateNextHexDumpLine(HexDumpInfo &aInfo);

} // namespace ot

#endif // LOG_HPP_
