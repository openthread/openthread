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
 *   This file implements the OpenThread logging related APIs.
 */

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/string.hpp"
#include "instance/instance.hpp"

using namespace ot;

otLogLevel otLoggingGetLevel(void) { return static_cast<otLogLevel>(Instance::GetLogLevel()); }

#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
otError otLoggingSetLevel(otLogLevel aLogLevel)
{
    Error error = kErrorNone;

    VerifyOrExit(aLogLevel <= kLogLevelDebg && aLogLevel >= kLogLevelNone, error = kErrorInvalidArgs);
    Instance::SetLogLevel(static_cast<LogLevel>(aLogLevel));

exit:
    return error;
}
#endif

static const char kPlatformModuleName[] = "Platform";

void otLogCritPlat(const char *aFormat, ...)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_CRIT) && OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    Logger::LogVarArgs(kPlatformModuleName, kLogLevelCrit, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aFormat);
    OT_UNUSED_VARIABLE(kPlatformModuleName);
#endif
}

void otLogWarnPlat(const char *aFormat, ...)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN) && OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    Logger::LogVarArgs(kPlatformModuleName, kLogLevelWarn, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aFormat);
#endif
}

void otLogNotePlat(const char *aFormat, ...)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE) && OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    Logger::LogVarArgs(kPlatformModuleName, kLogLevelNote, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aFormat);
#endif
}

void otLogInfoPlat(const char *aFormat, ...)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO) && OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    Logger::LogVarArgs(kPlatformModuleName, kLogLevelInfo, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aFormat);
#endif
}

void otLogDebgPlat(const char *aFormat, ...)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG) && OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    Logger::LogVarArgs(kPlatformModuleName, kLogLevelDebg, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aFormat);
#endif
}

void otDumpCritPlat(const char *aText, const void *aData, uint16_t aDataLength)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_CRIT) && OPENTHREAD_CONFIG_LOG_PLATFORM && OPENTHREAD_CONFIG_LOG_PKT_DUMP
    Logger::DumpInModule(kPlatformModuleName, kLogLevelCrit, aText, aData, aDataLength);
#else
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aDataLength);
#endif
}

void otDumpWarnPlat(const char *aText, const void *aData, uint16_t aDataLength)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN) && OPENTHREAD_CONFIG_LOG_PLATFORM && OPENTHREAD_CONFIG_LOG_PKT_DUMP
    Logger::DumpInModule(kPlatformModuleName, kLogLevelWarn, aText, aData, aDataLength);
#else
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aDataLength);
#endif
}

void otDumpNotePlat(const char *aText, const void *aData, uint16_t aDataLength)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE) && OPENTHREAD_CONFIG_LOG_PLATFORM && OPENTHREAD_CONFIG_LOG_PKT_DUMP
    Logger::DumpInModule(kPlatformModuleName, kLogLevelNote, aText, aData, aDataLength);
#else
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aDataLength);
#endif
}

void otDumpInfoPlat(const char *aText, const void *aData, uint16_t aDataLength)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO) && OPENTHREAD_CONFIG_LOG_PLATFORM && OPENTHREAD_CONFIG_LOG_PKT_DUMP
    Logger::DumpInModule(kPlatformModuleName, kLogLevelInfo, aText, aData, aDataLength);
#else
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aDataLength);
#endif
}

void otDumpDebgPlat(const char *aText, const void *aData, uint16_t aDataLength)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG) && OPENTHREAD_CONFIG_LOG_PLATFORM && OPENTHREAD_CONFIG_LOG_PKT_DUMP
    Logger::DumpInModule(kPlatformModuleName, kLogLevelDebg, aText, aData, aDataLength);
#else
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aDataLength);
#endif
}

void otLogPlat(otLogLevel aLogLevel, const char *aPlatModuleName, const char *aFormat, ...)
{
#if OPENTHREAD_CONFIG_LOG_PLATFORM
    va_list args;

    va_start(args, aFormat);
    otLogPlatArgs(aLogLevel, aPlatModuleName, aFormat, args);
    va_end(args);
#else
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aPlatModuleName);
    OT_UNUSED_VARIABLE(aFormat);
#endif
}

void otLogPlatArgs(otLogLevel aLogLevel, const char *aPlatModuleName, const char *aFormat, va_list aArgs)
{
#if OT_SHOULD_LOG && OPENTHREAD_CONFIG_LOG_PLATFORM
    String<kMaxLogModuleNameLength> moduleName;

    OT_ASSERT(aLogLevel >= kLogLevelNone && aLogLevel <= kLogLevelDebg);

    moduleName.Append("P-%s", aPlatModuleName);
    Logger::LogVarArgs(moduleName.AsCString(), static_cast<LogLevel>(aLogLevel), aFormat, aArgs);
#else
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aPlatModuleName);
    OT_UNUSED_VARIABLE(aFormat);
    OT_UNUSED_VARIABLE(aArgs);
#endif
}

void otLogCli(otLogLevel aLogLevel, const char *aFormat, ...)
{
#if OT_SHOULD_LOG && OPENTHREAD_CONFIG_LOG_CLI
    static const char kCliModuleName[] = "Cli";

    va_list args;

    OT_ASSERT(aLogLevel >= kLogLevelNone && aLogLevel <= kLogLevelDebg);
    VerifyOrExit(aLogLevel >= kLogLevelNone && aLogLevel <= kLogLevelDebg);

    va_start(args, aFormat);
    Logger::LogVarArgs(kCliModuleName, static_cast<LogLevel>(aLogLevel), aFormat, args);
    va_end(args);
exit:
#else
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aFormat);
#endif
    return;
}

otError otLogGenerateNextHexDumpLine(otLogHexDumpInfo *aInfo)
{
    AssertPointerIsNotNull(aInfo);

    return GenerateNextHexDumpLine(*aInfo);
}
