/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This is a fake "platform" logging header used by `test_log_platform_macro.cpp` to verify that
 *   `OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE` correctly routes `LogCrit()`/`LogWarn()`/... (and
 *   `DumpCrit()`/... ) to platform-provided macros instead of going through `ot::Logger`.
 *
 *   Every macro records its invocation (level, module name, formatted text) into `gTestLogPlatformCapture`
 *   so the test can assert on it with `VerifyOrQuit()`.
 */

#ifndef TEST_LOG_PLATFORM_MACRO_STUB_H_
#define TEST_LOG_PLATFORM_MACRO_STUB_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <openthread/platform/toolchain.h>

struct TestLogPlatformCapture
{
    int  mCallCount;
    char mLevel[16];
    char mModuleName[32];
    char mMessage[256];
};

extern TestLogPlatformCapture gTestLogPlatformCapture;

inline void TestLogPlatformCaptureReset(void)
{
    gTestLogPlatformCapture.mCallCount     = 0;
    gTestLogPlatformCapture.mLevel[0]      = '\0';
    gTestLogPlatformCapture.mModuleName[0] = '\0';
    gTestLogPlatformCapture.mMessage[0]    = '\0';
}

inline void TestLogPlatformRecordV(const char *aLevel, const char *aModuleName, const char *aFormat, va_list aArgs)
{
    gTestLogPlatformCapture.mCallCount++;
    snprintf(gTestLogPlatformCapture.mLevel, sizeof(gTestLogPlatformCapture.mLevel), "%s", aLevel);
    snprintf(gTestLogPlatformCapture.mModuleName, sizeof(gTestLogPlatformCapture.mModuleName), "%s", aModuleName);
    vsnprintf(gTestLogPlatformCapture.mMessage, sizeof(gTestLogPlatformCapture.mMessage), aFormat, aArgs);
}

inline void TestLogPlatformRecord(const char *aLevel, const char *aModuleName, const char *aFormat, ...)
    OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(3, 4);

inline void TestLogPlatformRecord(const char *aLevel, const char *aModuleName, const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    TestLogPlatformRecordV(aLevel, aModuleName, aFormat, args);
    va_end(args);
}

inline void TestLogPlatformRecordDump(const char *aLevel, const char *aModuleName, const char *aText)
{
    gTestLogPlatformCapture.mCallCount++;
    snprintf(gTestLogPlatformCapture.mLevel, sizeof(gTestLogPlatformCapture.mLevel), "%s-DUMP", aLevel);
    snprintf(gTestLogPlatformCapture.mModuleName, sizeof(gTestLogPlatformCapture.mModuleName), "%s", aModuleName);
    snprintf(gTestLogPlatformCapture.mMessage, sizeof(gTestLogPlatformCapture.mMessage), "%s", aText);
}

inline const char *TestLogLevelToString(int aLogLevel)
{
    // Mirrors `ot::LogLevel` (0=None, 1=Crit, 2=Warn, 3=Note, 4=Info, 5=Debg).
    static const char *const kLevelStrings[] = {"NONE", "CRIT", "WARN", "NOTE", "INFO", "DEBG"};

    return (aLogLevel >= 0 && aLogLevel <= 5) ? kLevelStrings[aLogLevel] : "UNKNOWN";
}

#define OT_LOG_PLATFORM_MODULE_REGISTER(aName) static const char *const kTestLogPlatformRegisteredModule = (aName)

#define OT_LOG_PLATFORM_CRIT(aModuleName, ...) TestLogPlatformRecord("CRIT", aModuleName, __VA_ARGS__)
#define OT_LOG_PLATFORM_WARN(aModuleName, ...) TestLogPlatformRecord("WARN", aModuleName, __VA_ARGS__)
#define OT_LOG_PLATFORM_NOTE(aModuleName, ...) TestLogPlatformRecord("NOTE", aModuleName, __VA_ARGS__)
#define OT_LOG_PLATFORM_INFO(aModuleName, ...) TestLogPlatformRecord("INFO", aModuleName, __VA_ARGS__)
#define OT_LOG_PLATFORM_DEBG(aModuleName, ...) TestLogPlatformRecord("DEBG", aModuleName, __VA_ARGS__)

#define TEST_LOG_PLATFORM_ON_ERROR_(aLevel, aModuleName, aError, ...) \
    do                                                                \
    {                                                                 \
        if ((aError) != 0)                                            \
        {                                                             \
            TestLogPlatformRecord(aLevel, aModuleName, __VA_ARGS__);  \
        }                                                             \
    } while (false)

#define OT_LOG_PLATFORM_CRIT_ON_ERROR(aModuleName, aError, ...) \
    TEST_LOG_PLATFORM_ON_ERROR_("CRIT", aModuleName, aError, __VA_ARGS__)
#define OT_LOG_PLATFORM_WARN_ON_ERROR(aModuleName, aError, ...) \
    TEST_LOG_PLATFORM_ON_ERROR_("WARN", aModuleName, aError, __VA_ARGS__)
#define OT_LOG_PLATFORM_NOTE_ON_ERROR(aModuleName, aError, ...) \
    TEST_LOG_PLATFORM_ON_ERROR_("NOTE", aModuleName, aError, __VA_ARGS__)
#define OT_LOG_PLATFORM_INFO_ON_ERROR(aModuleName, aError, ...) \
    TEST_LOG_PLATFORM_ON_ERROR_("INFO", aModuleName, aError, __VA_ARGS__)
#define OT_LOG_PLATFORM_DEBG_ON_ERROR(aModuleName, aError, ...) \
    TEST_LOG_PLATFORM_ON_ERROR_("DEBG", aModuleName, aError, __VA_ARGS__)

#define OT_LOG_PLATFORM_LOG_AT(aModuleName, aLogLevel, ...) \
    TestLogPlatformRecord(TestLogLevelToString(static_cast<int>(aLogLevel)), aModuleName, __VA_ARGS__)

#define OT_LOG_PLATFORM_DUMP_CRIT(aModuleName, aText, aData, aDataLength) \
    TestLogPlatformRecordDump("CRIT", aModuleName, aText)
#define OT_LOG_PLATFORM_DUMP_WARN(aModuleName, aText, aData, aDataLength) \
    TestLogPlatformRecordDump("WARN", aModuleName, aText)
#define OT_LOG_PLATFORM_DUMP_NOTE(aModuleName, aText, aData, aDataLength) \
    TestLogPlatformRecordDump("NOTE", aModuleName, aText)
#define OT_LOG_PLATFORM_DUMP_INFO(aModuleName, aText, aData, aDataLength) \
    TestLogPlatformRecordDump("INFO", aModuleName, aText)
#define OT_LOG_PLATFORM_DUMP_DEBG(aModuleName, aText, aData, aDataLength) \
    TestLogPlatformRecordDump("DEBG", aModuleName, aText)

#endif // TEST_LOG_PLATFORM_MACRO_STUB_H_
