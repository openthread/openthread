/*
 *  Copyright (c) 2017-2022, The OpenThread Authors.
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
 *   This file implements the logging related functions.
 */

#include "log.hpp"

#include <ctype.h>

#include <openthread/platform/logging.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/string.hpp"

/*
 * Verify debug UART dependency.
 *
 * It is reasonable to only enable the debug UART and not enable logs to the DEBUG UART.
 */
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART) && (!OPENTHREAD_CONFIG_ENABLE_DEBUG_UART)
#error "OPENTHREAD_CONFIG_ENABLE_DEBUG_UART_LOG requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART"
#endif

#if OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME && !OPENTHREAD_CONFIG_UPTIME_ENABLE
#error "OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME requires OPENTHREAD_CONFIG_UPTIME_ENABLE"
#endif

#if OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME && OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
#error "OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME is not supported under OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE"
#endif

namespace ot {

#if OT_SHOULD_LOG

template <LogLevel kLogLevel> void Logger::LogAtLevel(const char *aModuleName, const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    LogVarArgs(aModuleName, kLogLevel, aFormat, args);
    va_end(args);
}

// Explicit instantiations
template void Logger::LogAtLevel<kLogLevelNone>(const char *aModuleName, const char *aFormat, ...);
template void Logger::LogAtLevel<kLogLevelCrit>(const char *aModuleName, const char *aFormat, ...);
template void Logger::LogAtLevel<kLogLevelWarn>(const char *aModuleName, const char *aFormat, ...);
template void Logger::LogAtLevel<kLogLevelNote>(const char *aModuleName, const char *aFormat, ...);
template void Logger::LogAtLevel<kLogLevelInfo>(const char *aModuleName, const char *aFormat, ...);
template void Logger::LogAtLevel<kLogLevelDebg>(const char *aModuleName, const char *aFormat, ...);

void Logger::LogInModule(const char *aModuleName, LogLevel aLogLevel, const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    LogVarArgs(aModuleName, aLogLevel, aFormat, args);
    va_end(args);
}

void Logger::LogVarArgs(const char *aModuleName, LogLevel aLogLevel, const char *aFormat, va_list aArgs)
{
    static const char kModuleNamePadding[] = "--------------";

    ot::String<OPENTHREAD_CONFIG_LOG_MAX_SIZE> logString;

    static_assert(sizeof(kModuleNamePadding) == kMaxLogModuleNameLength + 1, "Padding string is not correct");

#if OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME
    ot::Uptime::UptimeToString(ot::Instance::Get().Get<ot::Uptime>().GetUptime(), logString);
    logString.Append(" ");
#endif

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    VerifyOrExit(Instance::GetLogLevel() >= aLogLevel);
#endif

#if OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
    {
        static const char kLevelChars[] = {
            '-', /* kLogLevelNone */
            'C', /* kLogLevelCrit */
            'W', /* kLogLevelWarn */
            'N', /* kLogLevelNote */
            'I', /* kLogLevelInfo */
            'D', /* kLogLevelDebg */
        };

        logString.Append("[%c] ", kLevelChars[aLogLevel]);
    }
#endif

    logString.Append("%.*s%s: ", kMaxLogModuleNameLength, aModuleName,
                     &kModuleNamePadding[StringLength(aModuleName, kMaxLogModuleNameLength)]);

    logString.AppendVarArgs(aFormat, aArgs);

    logString.Append("%s", OPENTHREAD_CONFIG_LOG_SUFFIX);
    otPlatLog(aLogLevel, OT_LOG_REGION_CORE, "%s", logString.AsCString());

    ExitNow();

exit:
    return;
}

#if OPENTHREAD_CONFIG_LOG_PKT_DUMP

template <LogLevel kLogLevel>
void Logger::DumpAtLevel(const char *aModuleName, const char *aText, const void *aData, uint16_t aDataLength)
{
    DumpInModule(aModuleName, kLogLevel, aText, aData, aDataLength);
}

// Explicit instantiations
template void Logger::DumpAtLevel<kLogLevelNone>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);
template void Logger::DumpAtLevel<kLogLevelCrit>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);
template void Logger::DumpAtLevel<kLogLevelWarn>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);
template void Logger::DumpAtLevel<kLogLevelNote>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);
template void Logger::DumpAtLevel<kLogLevelInfo>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);
template void Logger::DumpAtLevel<kLogLevelDebg>(const char *aModuleName,
                                                 const char *aText,
                                                 const void *aData,
                                                 uint16_t    aDataLength);

void Logger::DumpLine(const char *aModuleName, LogLevel aLogLevel, const uint8_t *aData, const uint16_t aDataLength)
{
    ot::String<kStringLineLength> string;

    string.Append("|");

    for (uint8_t i = 0; i < kDumpBytesPerLine; i++)
    {
        if (i < aDataLength)
        {
            string.Append(" %02X", aData[i]);
        }
        else
        {
            string.Append(" ..");
        }

        if (!((i + 1) % 8))
        {
            string.Append(" |");
        }
    }

    string.Append(" ");

    for (uint8_t i = 0; i < kDumpBytesPerLine; i++)
    {
        char c = '.';

        if (i < aDataLength)
        {
            char byteAsChar = static_cast<char>(0x7f & aData[i]);

            if (isprint(byteAsChar))
            {
                c = byteAsChar;
            }
        }

        string.Append("%c", c);
    }

    LogInModule(aModuleName, aLogLevel, "%s", string.AsCString());
}

void Logger::DumpInModule(const char *aModuleName,
                          LogLevel    aLogLevel,
                          const char *aText,
                          const void *aData,
                          uint16_t    aDataLength)
{
    constexpr uint16_t kWidth         = 72;
    constexpr uint16_t kTextSuffixLen = sizeof("[ len=000]") - 1;

    uint16_t                      txtLen = StringLength(aText, kWidth - kTextSuffixLen) + kTextSuffixLen;
    ot::String<kStringLineLength> string;

    VerifyOrExit(otLoggingGetLevel() >= aLogLevel);

    for (uint16_t i = 0; i < static_cast<uint16_t>((kWidth - txtLen) / 2); i++)
    {
        string.Append("=");
    }

    string.Append("[%s len=%03u]", aText, aDataLength);

    for (uint16_t i = 0; i < static_cast<uint16_t>(kWidth - txtLen - (kWidth - txtLen) / 2); i++)
    {
        string.Append("=");
    }

    LogInModule(aModuleName, aLogLevel, "%s", string.AsCString());

    for (uint16_t i = 0; i < aDataLength; i += kDumpBytesPerLine)
    {
        DumpLine(aModuleName, aLogLevel, static_cast<const uint8_t *>(aData) + i,
                 OT_MIN((aDataLength - i), kDumpBytesPerLine));
    }

    string.Clear();

    for (uint16_t i = 0; i < kWidth; i++)
    {
        string.Append("-");
    }

    LogInModule(aModuleName, aLogLevel, "%s", string.AsCString());

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_LOG_PKT_DUMP

#endif // OT_SHOULD_LOG

} // namespace ot
