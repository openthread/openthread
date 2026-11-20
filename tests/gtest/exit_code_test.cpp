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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include <openthread-core-config.h>
#include <openthread/error.h>
#include <openthread/logging.h>
#include <openthread/platform/logging.h>

#include "fake_platform.hpp"
#include "lib/platform/exit_code.h"

namespace {

bool sCapturePlatformLog = false;

} // namespace

extern "C" void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    if (sCapturePlatformLog)
    {
        va_list args;

        va_start(args, aFormat);
        std::vfprintf(stderr, aFormat, args);
        std::fputc('\n', stderr);
        std::fflush(stderr);
        va_end(args);
    }
}

extern "C" void otPlatLogOutput(otInstance *aInstance, otLogLevel aLogLevel, const char *aLogLine)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLogLevel);

    if (sCapturePlatformLog)
    {
        std::fprintf(stderr, "%s\n", aLogLine);
        std::fflush(stderr);
    }
}

namespace {

using ::testing::AllOf;
using ::testing::ExitedWithCode;
using ::testing::HasSubstr;
using ::testing::KilledBySignal;
using ::testing::Not;

struct HandlerContext
{
    int mTag;
    int mCallCount;
};

void RecordExitHandler(uint8_t aExitCode, void *aContext)
{
    auto *context = static_cast<HandlerContext *>(aContext);

    context->mCallCount++;
    std::fprintf(stderr, "EXIT_HANDLER:code=%u:tag=%d:count=%d:exiting=%d\n", static_cast<unsigned int>(aExitCode),
                 context->mTag, context->mCallCount, otExitIsExiting() ? 1 : 0);
    std::fflush(stderr);
}

void ReentrantDieHandler(uint8_t aExitCode, void *aContext)
{
    auto *context = static_cast<HandlerContext *>(aContext);

    context->mCallCount++;
    std::fprintf(stderr, "REENTRANT_DIE:code=%u:count=%d:exiting=%d\n", static_cast<unsigned int>(aExitCode),
                 context->mCallCount, otExitIsExiting() ? 1 : 0);
    std::fflush(stderr);

    DieNow(OT_EXIT_INVALID_STATE);
}

void ReentrantDieWithMessageHandler(uint8_t aExitCode, void *aContext)
{
    auto *context = static_cast<HandlerContext *>(aContext);

    context->mCallCount++;
    std::fprintf(stderr, "REENTRANT_DIE_MSG:code=%u:count=%d:exiting=%d\n", static_cast<unsigned int>(aExitCode),
                 context->mCallCount, otExitIsExiting() ? 1 : 0);
    std::fflush(stderr);

    DieNowWithMessage("nested fatal error", OT_EXIT_INVALID_STATE);
}

void ReentrantAbortHandler(uint8_t aExitCode, void *aContext)
{
    auto *context = static_cast<HandlerContext *>(aContext);

    context->mCallCount++;
    std::fprintf(stderr, "REENTRANT_ABORT:code=%u:count=%d:exiting=%d\n", static_cast<unsigned int>(aExitCode),
                 context->mCallCount, otExitIsExiting() ? 1 : 0);
    std::fflush(stderr);

    otAbortNow(OT_EXIT_INVALID_STATE);
}

#if (OPENTHREAD_CONFIG_LOG_OUTPUT != OPENTHREAD_CONFIG_LOG_OUTPUT_NONE) && OPENTHREAD_CONFIG_LOG_PLATFORM && \
    (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT)
void MutateErrnoHandler(uint8_t aExitCode, void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    errno = EINVAL;
    std::fprintf(stderr, "ERRNO_HANDLER:code=%u:errno=%d\n", static_cast<unsigned int>(aExitCode), errno);
    std::fflush(stderr);
}
#endif

TEST(ExitCodeDeathTest, DieNowInvokesHandlerWithExitCodeAndContext)
{
    HandlerContext context{42, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &context);
            DieNow(OT_EXIT_RADIO_SPINEL_RESET);
        },
        ExitedWithCode(OT_EXIT_RADIO_SPINEL_RESET), HasSubstr("EXIT_HANDLER:code=4:tag=42:count=1:exiting=1"));
}

TEST(ExitCodeDeathTest, DieNowWithMessageInvokesHandlerAndEvaluatesExitCodeOnce)
{
    HandlerContext context{77, 0};

    EXPECT_EXIT(
        {
            int evalCount = 0;

            auto getExitCode = [&evalCount](void) -> uint8_t {
                evalCount++;
                return (evalCount == 1) ? OT_EXIT_RADIO_SPINEL_NO_RESPONSE : OT_EXIT_FAILURE;
            };

            otExitSetHandler(RecordExitHandler, &context);
            DieNowWithMessage("fatal test error", getExitCode());
        },
        ExitedWithCode(OT_EXIT_RADIO_SPINEL_NO_RESPONSE), HasSubstr("EXIT_HANDLER:code=6:tag=77:count=1:exiting=1"));
}

TEST(ExitCodeDeathTest, VerifyOrDieFalseInvokesHandlerAndEvaluatesExitCodeOnce)
{
    HandlerContext context{99, 0};

    EXPECT_EXIT(
        {
            int evalCount = 0;

            auto getExitCode = [&evalCount](void) -> uint8_t {
                evalCount++;
                return (evalCount == 1) ? OT_EXIT_RCP_RESET_REQUIRED : OT_EXIT_FAILURE;
            };

            otExitSetHandler(RecordExitHandler, &context);
            VerifyOrDie(false, getExitCode());
        },
        ExitedWithCode(OT_EXIT_RCP_RESET_REQUIRED), HasSubstr("EXIT_HANDLER:code=8:tag=99:count=1:exiting=1"));
}

TEST(ExitCodeDeathTest, SuccessOrDieInvokesHandlerWithMappedExitCode)
{
    HandlerContext context{11, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &context);
            SuccessOrDie(OT_ERROR_INVALID_ARGS);
        },
        ExitedWithCode(OT_EXIT_INVALID_ARGUMENTS), HasSubstr("EXIT_HANDLER:code=2:tag=11:count=1:exiting=1"));

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &context);
            SuccessOrDie(OT_ERROR_FAILED);
        },
        ExitedWithCode(OT_EXIT_FAILURE), HasSubstr("EXIT_HANDLER:code=1:tag=11:count=1:exiting=1"));
}

TEST(ExitCodeDeathTest, ReentrantDieNowInvokesHandlerOnlyOnceAndPreservesExitCode)
{
    HandlerContext context{0, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(ReentrantDieHandler, &context);
            DieNow(OT_EXIT_RADIO_SPINEL_NO_RESPONSE);
        },
        ExitedWithCode(OT_EXIT_RADIO_SPINEL_NO_RESPONSE),
        AllOf(HasSubstr("REENTRANT_DIE:code=6:count=1:exiting=1"), Not(HasSubstr("count=2"))));
}

TEST(ExitCodeDeathTest, AbortNowInvokesHandlerAndTerminatesWithSigabrt)
{
    HandlerContext context{42, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &context);
            otAbortNow(OT_EXIT_RADIO_SPINEL_RESET);
        },
        KilledBySignal(SIGABRT), HasSubstr("EXIT_HANDLER:code=4:tag=42:count=1:exiting=1"));
}

TEST(ExitCodeDeathTest, ReentrantDieNowDuringAbortPreservesSigabrt)
{
    HandlerContext context{0, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(ReentrantDieHandler, &context);
            otAbortNow(OT_EXIT_RADIO_SPINEL_RESET);
        },
        KilledBySignal(SIGABRT), AllOf(HasSubstr("REENTRANT_DIE:code=4:count=1:exiting=1"), Not(HasSubstr("count=2"))));
}

TEST(ExitCodeDeathTest, ReentrantDieNowWithMessageDuringAbortPreservesSigabrt)
{
    HandlerContext context{0, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(ReentrantDieWithMessageHandler, &context);
            otAbortNow(OT_EXIT_RADIO_SPINEL_RESET);
        },
        KilledBySignal(SIGABRT),
        AllOf(HasSubstr("REENTRANT_DIE_MSG:code=4:count=1:exiting=1"), Not(HasSubstr("count=2"))));
}

TEST(ExitCodeDeathTest, ReentrantAbortNowDuringDieNowPreservesOriginalExitCode)
{
    HandlerContext context{0, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(ReentrantAbortHandler, &context);
            DieNow(OT_EXIT_RADIO_SPINEL_NO_RESPONSE);
        },
        ExitedWithCode(OT_EXIT_RADIO_SPINEL_NO_RESPONSE),
        AllOf(HasSubstr("REENTRANT_ABORT:code=6:count=1:exiting=1"), Not(HasSubstr("count=2"))));
}

TEST(ExitCodeDeathTest, ReentrantAbortNowDuringAbortPreservesSigabrtAndInvokesOnce)
{
    HandlerContext context{0, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(ReentrantAbortHandler, &context);
            otAbortNow(OT_EXIT_RADIO_SPINEL_RESET);
        },
        KilledBySignal(SIGABRT),
        AllOf(HasSubstr("REENTRANT_ABORT:code=4:count=1:exiting=1"), Not(HasSubstr("count=2"))));
}

TEST(ExitCodeDeathTest, ClearingHandlerPreventsInvocation)
{
    HandlerContext context{55, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &context);
            otExitSetHandler(nullptr, nullptr);
            DieNow(OT_EXIT_FAILURE);
        },
        ExitedWithCode(OT_EXIT_FAILURE), Not(HasSubstr("EXIT_HANDLER:")));
}

TEST(ExitCodeDeathTest, OverwritingHandlerUsesLatestHandlerAndContext)
{
    HandlerContext firstContext{1, 0};
    HandlerContext secondContext{2, 0};

    EXPECT_EXIT(
        {
            otExitSetHandler(RecordExitHandler, &firstContext);
            otExitSetHandler(RecordExitHandler, &secondContext);
            DieNow(OT_EXIT_INVALID_STATE);
        },
        ExitedWithCode(OT_EXIT_INVALID_STATE),
        AllOf(HasSubstr("EXIT_HANDLER:code=7:tag=2:count=1:exiting=1"), Not(HasSubstr("tag=1"))));
}

#if (OPENTHREAD_CONFIG_LOG_OUTPUT != OPENTHREAD_CONFIG_LOG_OUTPUT_NONE) && OPENTHREAD_CONFIG_LOG_PLATFORM && \
    (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_CRIT)
TEST(ExitCodeDeathTest, LogsOriginalErrnoBeforeExitHandlerRuns)
{
    ot::FakePlatform platform;
    std::string      expectedErr = std::strerror(ENOENT);
    std::string      mutatedErr  = std::strerror(EINVAL);

    ASSERT_NE(expectedErr, mutatedErr);
#if OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
    ASSERT_EQ(otLoggingSetLevel(OT_LOG_LEVEL_CRIT), OT_ERROR_NONE);
#endif

    EXPECT_EXIT(
        {
            sCapturePlatformLog = true;
            otExitSetHandler(MutateErrnoHandler, nullptr);
            errno = ENOENT;
            DieNow(OT_EXIT_ERROR_ERRNO);
        },
        ExitedWithCode(OT_EXIT_ERROR_ERRNO),
        AllOf(HasSubstr(expectedErr), Not(HasSubstr(mutatedErr)), HasSubstr("ERRNO_HANDLER:code=5:")));

    EXPECT_EXIT(
        {
            sCapturePlatformLog = true;
            otExitSetHandler(MutateErrnoHandler, nullptr);
            errno = ENOENT;
            DieNowWithMessage("errno test", OT_EXIT_ERROR_ERRNO);
        },
        ExitedWithCode(OT_EXIT_ERROR_ERRNO),
        AllOf(HasSubstr(expectedErr), Not(HasSubstr(mutatedErr)), HasSubstr("ERRNO_HANDLER:code=5:")));
}
#endif

TEST(ExitCodeTest, PassingChecksDoNotInvokeHandler)
{
    HandlerContext context{123, 0};

    otExitSetHandler(RecordExitHandler, &context);

    VerifyOrDie(true, OT_EXIT_FAILURE);
    SuccessOrDie(OT_ERROR_NONE);

    otExitSetHandler(nullptr, nullptr);

    EXPECT_FALSE(otExitIsExiting());
    EXPECT_EQ(context.mCallCount, 0);
}

} // namespace
