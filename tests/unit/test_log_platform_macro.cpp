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
 *   This test verifies `OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE`: when enabled, `RegisterLogModule()` and
 *   every `Log{Crit,Warn,Note,Info,Debg}()` / `Log{Level}OnError()` / `LogAt()` / `Dump{Level}()` macro must
 *   expand to the platform-provided `OT_LOG_PLATFORM_*` macros instead of going through `ot::Logger`. The same
 *   applies to the public `otLog{Level}Plat()`/`otDump{Level}Plat()` API declared in `<openthread/logging.h>`.
 *   It also verifies that `LogAlways()`/`LogCert()`/`DumpAlways()`/`DumpCert()`/`otLogPlat()`/`otLogCli()` remain
 *   unaffected and never reach the platform macros, since they aren't tied to a specific log level.
 *
 *   This translation unit is compiled (see `tests/unit/CMakeLists.txt`) with
 *   `OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE=1` and `OPENTHREAD_CONFIG_LOG_OFFLOADING_HEADER_FILE`
 *   pointing at `test_log_platform_macro_stub.h`, which records every platform-macro invocation into
 *   `gTestLogPlatformCapture` so we can assert on the captured level/module-name/message here.
 */

#include "test_platform.h"

#include "test_util.h"

#include "common/log.hpp"

TestLogPlatformCapture gTestLogPlatformCapture;

namespace ot {

RegisterLogModule("TestLogPlat");

void TestLogPlatformMacroModuleRegister(void)
{
    // `RegisterLogModule()` must invoke `OT_LOG_PLATFORM_MODULE_REGISTER()` with the module name, in addition to
    // defining `kLogModuleName`. The stub records it into `kTestLogPlatformRegisteredModule` (file-scope, defined
    // right above by the `RegisterLogModule("TestLogPlat")` call at the top of this file).
    VerifyOrQuit(strcmp(kTestLogPlatformRegisteredModule, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(kLogModuleName, "TestLogPlat") == 0);
}

void TestLogPlatformMacroBasicLevels(void)
{
    TestLogPlatformCaptureReset();
    LogCrit("crit %d", 1);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "crit 1") == 0);

    TestLogPlatformCaptureReset();
    LogWarn("warn %d", 2);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "WARN") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "warn 2") == 0);

    TestLogPlatformCaptureReset();
    LogNote("note %d", 3);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "NOTE") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "note 3") == 0);

    TestLogPlatformCaptureReset();
    LogInfo("info %d", 4);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "info 4") == 0);

    TestLogPlatformCaptureReset();
    LogDebg("debg %d", 5);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "DEBG") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "debg 5") == 0);
}

void TestLogPlatformMacroOnError(void)
{
    // No error (`kErrorNone`): none of the `Log{Level}OnError()` macros should log anything.

    TestLogPlatformCaptureReset();
    LogCritOnError(kErrorNone, "should not log");
    LogWarnOnError(kErrorNone, "should not log");
    LogNoteOnError(kErrorNone, "should not log");
    LogInfoOnError(kErrorNone, "should not log");
    LogDebgOnError(kErrorNone, "should not log");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "Log{Level}OnError() must not log when there is no error");

    // With an error, each `Log{Level}OnError()` must log once, at its own level.

    TestLogPlatformCaptureReset();
    LogCritOnError(kErrorFailed, "the operation");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1, "LogCritOnError() must log when there is an error");
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    LogWarnOnError(kErrorFailed, "the operation");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1, "LogWarnOnError() must log when there is an error");
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "WARN") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    LogNoteOnError(kErrorFailed, "the operation");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1, "LogNoteOnError() must log when there is an error");
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "NOTE") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    LogInfoOnError(kErrorFailed, "the operation");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1, "LogInfoOnError() must log when there is an error");
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    LogDebgOnError(kErrorFailed, "the operation");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1, "LogDebgOnError() must log when there is an error");
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "DEBG") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
}

void TestLogPlatformMacroLogAt(void)
{
    TestLogPlatformCaptureReset();
    LogAt(kLogLevelInfo, "logat %d", 6);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "logat 6") == 0);

    TestLogPlatformCaptureReset();
    LogAt(kLogLevelCrit, "logat crit");
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT") == 0);
}

void TestLogPlatformMacroDump(void)
{
    static const uint8_t kData[] = {0x01, 0x02, 0x03};

    TestLogPlatformCaptureReset();
    DumpCrit("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "dump text") == 0);

    TestLogPlatformCaptureReset();
    DumpWarn("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "WARN-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    DumpNote("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "NOTE-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);

    TestLogPlatformCaptureReset();
    DumpInfo("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "dump text") == 0);

    TestLogPlatformCaptureReset();
    DumpDebg("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "DEBG-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "TestLogPlat") == 0);
}

void TestLogPlatformMacroPublicPlatApi(void)
{
    // The public `otLog{Level}Plat()`/`otDump{Level}Plat()` API (declared in `<openthread/logging.h>`, used by
    // platform code to route its own logs through OpenThread) must also expand to the platform-provided macros
    // when this feature is enabled, using `"Platform"` as the fixed module name.
    static const uint8_t kData[] = {0x01, 0x02, 0x03};

    TestLogPlatformCaptureReset();
    otLogCritPlat("plat crit %d", 1);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "plat crit 1") == 0);

    TestLogPlatformCaptureReset();
    otLogWarnPlat("plat warn %d", 2);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "WARN") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otLogNotePlat("plat note %d", 3);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "NOTE") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otLogInfoPlat("plat info %d", 4);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otLogDebgPlat("plat debg %d", 5);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "DEBG") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otDumpCritPlat("plat dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "CRIT-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mMessage, "plat dump text") == 0);

    TestLogPlatformCaptureReset();
    otDumpWarnPlat("plat dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "WARN-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otDumpNotePlat("plat dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "NOTE-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otDumpInfoPlat("plat dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "INFO-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);

    TestLogPlatformCaptureReset();
    otDumpDebgPlat("plat dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 1);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mLevel, "DEBG-DUMP") == 0);
    VerifyOrQuit(strcmp(gTestLogPlatformCapture.mModuleName, "Platform") == 0);
}

void TestLogPlatformMacroUnaffectedMacros(void)
{
    // `LogAlways()`, `LogCert()`, `DumpAlways()`, and `DumpCert()` are documented to always go through `ot::Logger`
    // and are *not* affected by `OPENTHREAD_CONFIG_LOG_OFFLOADING_ENABLE`, since they aren't tied to a specific
    // log level. Verify none of them ever reach the platform-macro stub (i.e. `gTestLogPlatformCapture` stays
    // untouched), even though this translation unit is compiled with the feature enabled.
    static const uint8_t kData[] = {0x01, 0x02, 0x03};

    TestLogPlatformCaptureReset();
    LogAlways("always %d", 1);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "LogAlways() must not go through the platform macros");

    TestLogPlatformCaptureReset();
    LogCert("cert %d", 2);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "LogCert() must not go through the platform macros");

    TestLogPlatformCaptureReset();
    DumpAlways("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "DumpAlways() must not go through the platform macros");

    TestLogPlatformCaptureReset();
    DumpCert("dump text", kData, sizeof(kData));
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "DumpCert() must not go through the platform macros");

    // Likewise, `otLogPlat()` and `otLogCli()` carry a runtime module name/log level and are not tied to a fixed
    // per-level macro name, so they must always go through `Logger` too.

    TestLogPlatformCaptureReset();
    otLogPlat(OT_LOG_LEVEL_INFO, "SubModule", "plat %d", 3);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "otLogPlat() must not go through the platform macros");

    TestLogPlatformCaptureReset();
    otLogCli(OT_LOG_LEVEL_INFO, "cli %d", 4);
    VerifyOrQuit(gTestLogPlatformCapture.mCallCount == 0, "otLogCli() must not go through the platform macros");
}

} // namespace ot

int main(void)
{
    ot::TestLogPlatformMacroModuleRegister();
    ot::TestLogPlatformMacroBasicLevels();
    ot::TestLogPlatformMacroOnError();
    ot::TestLogPlatformMacroLogAt();
    ot::TestLogPlatformMacroDump();
    ot::TestLogPlatformMacroPublicPlatApi();
    ot::TestLogPlatformMacroUnaffectedMacros();
    printf("All tests passed\n");
    return 0;
}
