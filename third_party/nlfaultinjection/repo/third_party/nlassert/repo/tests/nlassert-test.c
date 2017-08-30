/*
 *
 *    Copyright (c) 2012-2015 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

/**
 *    @file
 *      This file implements a unit test suite for the Nest Labs
 *      runtime assertion library.
 *
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nlassert-test-config.h"

#define NL_ASSERT_COMPONENT_STRING "nlassert-test"

static void nlassert_test_abort(void);
static void nlassert_test_backtrace(void);
static void nlassert_test_log(const char *format, ...);
static void nlassert_test_trap(void);

#define NL_ASSERT_PRODUCTION                         (NL_ASSERT_TEST_WANT_PRODUCTION)

/*
 * Set the assertion trigger flags as desired by the test harness configuration
 */
#if NL_ASSERT_TEST_WANT_BACKTRACE && NL_ASSERT_TEST_WANT_LOG && NL_ASSERT_TEST_WANT_TRAP
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG | NL_ASSERT_FLAG_TRAP)
#elif NL_ASSERT_TEST_WANT_BACKTRACE && NL_ASSERT_TEST_WANT_LOG
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)
#elif NL_ASSERT_TEST_WANT_BACKTRACE && NL_ASSERT_TEST_WANT_TRAP
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_TRAP)
#elif NL_ASSERT_TEST_WANT_LOG && NL_ASSERT_TEST_WANT_TRAP
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_LOG       | NL_ASSERT_FLAG_TRAP)
#elif NL_ASSERT_TEST_WANT_BACKTRACE
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_BACKTRACE)
#elif NL_ASSERT_TEST_WANT_LOG
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_LOG)
#elif NL_ASSERT_TEST_WANT_TRAP
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_TRAP)
#else
#define __NL_TEST_ASSERT_FLAGS                       (NL_ASSERT_FLAG_NONE)
#endif

/*
 * Configure both the non-production and production ssertion interface
 * family trigger flags with the defaults, if so requested, or as set
 * above.
 */
#if NL_ASSERT_TEST_WANT_DEFAULTS
# define NL_ASSERT_USE_FLAGS_DEFAULT                 1
#else /* !NL_ASSERT_TEST_WANT_DEFAULTS */
# define NL_ASSERT_EXPECT_FLAGS                      __NL_TEST_ASSERT_FLAGS

# define NL_ASSERT_ABORT_PRODUCTION_FLAGS            __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_VERIFY_PRODUCTION_FLAGS           __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS     __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_DESIRE_PRODUCTION_FLAGS           __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_REQUIRE_PRODUCTION_FLAGS          __NL_TEST_ASSERT_FLAGS

# define NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS        __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_ABORT_NONPRODUCTION_FLAGS         __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_CHECK_NONPRODUCTION_FLAGS         __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS        __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS  __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS        __NL_TEST_ASSERT_FLAGS
# define NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS       __NL_TEST_ASSERT_FLAGS
#endif /* NL_ASSERT_TEST_WANT_DEFAULTS */

/*
 * Abstract the flags for each assertion style such that the
 * abstracted flags mnemonic can be passed unconditionally to the test
 * initialization code.
 */
#if NL_ASSERT_TEST_WANT_PRODUCTION
# define __NL_ASSERT_TEST_ASSERT_FLAGS               (NL_ASSERT_FLAG_NONE)
# define __NL_ASSERT_TEST_ABORT_FLAGS                NL_ASSERT_ABORT_PRODUCTION_FLAGS
# define __NL_ASSERT_TEST_CHECK_FLAGS                (NL_ASSERT_FLAG_NONE)
# define __NL_ASSERT_TEST_VERIFY_FLAGS               NL_ASSERT_VERIFY_PRODUCTION_FLAGS
# define __NL_ASSERT_TEST_PRECONDITION_FLAGS         NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
# define __NL_ASSERT_TEST_EXPECT_FLAGS               NL_ASSERT_EXPECT_FLAGS
# define __NL_ASSERT_TEST_DESIRE_FLAGS               NL_ASSERT_DESIRE_PRODUCTION_FLAGS
# define __NL_ASSERT_TEST_REQUIRE_FLAGS              NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
#else /* !NL_ASSERT_TEST_WANT_PRODUCTION */
# define __NL_ASSERT_TEST_ASSERT_FLAGS               NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_ABORT_FLAGS                NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_CHECK_FLAGS                NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_VERIFY_FLAGS               NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_PRECONDITION_FLAGS         NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_EXPECT_FLAGS               NL_ASSERT_EXPECT_FLAGS
# define __NL_ASSERT_TEST_DESIRE_FLAGS               NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
# define __NL_ASSERT_TEST_REQUIRE_FLAGS              NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
#endif /* NL_ASSERT_TEST_WANT_PRODUCTION */

#define NL_ASSERT_ABORT()                       \
    do {                                        \
        nlassert_test_abort();                  \
    } while (0)

#define NL_ASSERT_BACKTRACE()                   \
    do {                                        \
        nlassert_test_backtrace();              \
    } while (0)

#define NL_ASSERT_LOG(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)         \
    do {                                                                                  \
        nlassert_test_log(NL_ASSERT_LOG_FORMAT_DEFAULT,                                   \
                          aPrefix,                                                        \
                          (((aName) == 0) || (*(aName) == '\0')) ? "" : aName,            \
                          (((aName) == 0) || (*(aName) == '\0')) ? "" : ": ",             \
                          aCondition,                                                     \
                          ((aMessage == 0) ? "" : aMessage),                              \
                          ((aMessage == 0) ? "" : ", "),                                  \
                          aFile,                                                          \
                          aLine);                                                         \
    } while (0)

#define NL_ASSERT_TRAP()                        \
    do {                                        \
        nlassert_test_trap();                   \
    } while (0)

#if NLASSERT_TEST_STDC
#include <assert.h>
#endif
#include <nlassert.h>

#include <nlunit-test.h>

// Type Defintions

struct AssertStyleContext {
    bool                      mShouldAbort;
    bool                      mShouldBacktrace;
    bool                      mShouldLog;
    bool                      mShouldTrap;
    bool                      mShouldLogActionOnly;
};

struct TestBufferContext {
    char *                    mBuffer;
    size_t                    mBufferOffset;
    size_t                    mBufferSize;
};

struct TestContext {
    struct TestBufferContext  mActual;
    struct TestBufferContext  mExpected;
    bool                      mWantProduction;
    bool                      mIsProduction;
    bool                      mDidAbort;
    bool                      mDidBacktrace;
    bool                      mDidLog;
    bool                      mDidTrap;
    struct AssertStyleContext mAssert;
    struct AssertStyleContext mAbort;
    struct AssertStyleContext mCheck;
    struct AssertStyleContext mVerify;
    struct AssertStyleContext mPrecondition;
    struct AssertStyleContext mExpect;
    struct AssertStyleContext mDesire;
    struct AssertStyleContext mRequire;
};

// Global Variables

static struct TestContext sContext;

/**
 *  Reset a test buffer context by setting the offset to zero.
 */
static void TestBufferContextReset(struct TestBufferContext *inContext)
{
    if (inContext->mBuffer != NULL)
    {
        inContext->mBufferOffset = 0;
        inContext->mBuffer[0]    = '\0';
    }
}

/**
 *  Reset a test context by resetting both the actual and expected
 *  buffers and setting all of the abort, backtrace, log, and trap
 *  Booleans to false.
 */
static void TestContextReset(struct TestContext *inContext)
{
    TestBufferContextReset(&inContext->mActual);
    TestBufferContextReset(&inContext->mExpected);

    sContext.mDidAbort     = false;
    sContext.mDidBacktrace = false;
    sContext.mDidLog       = false;
    sContext.mDidTrap      = false;
}

/**
 *  Initialize the assertion style context by determining whether or
 *  not a backtrace, log, and trap are expected on assertion trigger.
 */
static void AssertStyleContextInit(struct AssertStyleContext *inContext, const uint32_t inFlags, bool inAbort, bool inLogActionOnly)
{

    inContext->mShouldAbort         = inAbort;
    inContext->mShouldBacktrace     = inFlags & NL_ASSERT_FLAG_BACKTRACE;
    inContext->mShouldLog           = inFlags & NL_ASSERT_FLAG_LOG;
    inContext->mShouldTrap          = inFlags & NL_ASSERT_FLAG_TRAP;
    inContext->mShouldLogActionOnly = inLogActionOnly;
}

/**
 *  Test-specific hook for #NL_ASSERT_ABORT used with nlASSERT.
 */
static void __attribute__((unused)) nlassert_test_abort(void)
{
    sContext.mDidAbort = true;
}

/**
 *  Test-specific hook for #NL_ASSERT_BACKTRACE used with all styles of
 *  assertion.
 */
static void __attribute__((unused)) nlassert_test_backtrace(void)
{
    sContext.mDidBacktrace = true;
}

/**
 *  Test-specific hook for #NL_ASSERT_TRAP used with all styles of
 *  assertion.
 */
static void __attribute__((unused)) nlassert_test_trap(void)
{
    sContext.mDidTrap = true;
}

static void nlassert_test_vlog_with_buffer(struct TestBufferContext *inBuffer, const char *inFormat, va_list inArguments)
{
    const size_t size = inBuffer->mBufferSize - inBuffer->mBufferOffset;
    char *       p    = inBuffer->mBuffer + inBuffer->mBufferOffset;
    int          n;

    n = vsnprintf(p, size, inFormat, inArguments);

    if (n > 0) {
        inBuffer->mBufferOffset += n;
    }
}

static void nlassert_test_log_with_buffer(struct TestBufferContext *inBuffer, const char *inFormat, ...)
{
    va_list      args;

    va_start(args, inFormat);

    nlassert_test_vlog_with_buffer(inBuffer, inFormat, args);

    va_end(args);
}

static void _nlassert_test_log(const char *inFormat, ...)
{
    va_list args;

    va_start(args, inFormat);

    nlassert_test_vlog_with_buffer(&sContext.mActual, inFormat, args);

    va_end(args);
}

/**
 *  Test-specific hook for #NL_ASSERT_LOG used with all styles of
 *  assertion.
 */
static void __attribute__((unused)) nlassert_test_log(const char *inFormat, ...)
{
    va_list args;

    sContext.mDidLog = true;

    va_start(args, inFormat);

    nlassert_test_vlog_with_buffer(&sContext.mActual, inFormat, args);

    va_end(args);
}

static void nlassert_test_action(const char *function, const char *message)
{
    _nlassert_test_log("%s: %s test\n", function, message);
}

#define nlASSERT_TEST_ACTION(message) nlassert_test_action(__func__, message)

#define nlASSERT_TEST_CHECK_EXPECTED(context, style, assertion, extra, action, line) \
    nlAssertTestCheckExpected(context, style, assertion, extra, action, __FILE__, line)

static bool nlAssertTestCheckExpected(struct TestContext *inContext, const struct AssertStyleContext *inStyle, const char *inAssertion, const char *inExtra, const char *inAction, const char *inFile, unsigned int inLine)
{
    int  comparison;
    bool matches;

    /*
     * Only log an expected message if the particular assertion style
     * should have done so. Otherwise, only log an action message if
     * we are not production or if the assertion style if not 'check',
     * since production 'check' assertions are completely compiled
     * out.
     */
    if (inStyle->mShouldLog)
    {
        nlassert_test_log_with_buffer(&inContext->mExpected,
                                      "%s%s%s%s%s%s%s%s%u%s%s",
                                      "ASSERT: ",
                                      NL_ASSERT_COMPONENT_STRING ": ",
                                      inAssertion,
                                      ", ",
                                      inExtra,
                                      "file: ",
                                      inFile,
                                      ", line: ",
                                      inLine,
                                      "\n",
                                      inAction);
    }
    else if (inStyle->mShouldLogActionOnly)
    {
        nlassert_test_log_with_buffer(&inContext->mExpected,
                                      "%s",
                                      inAction);
    }

    comparison = strcmp(inContext->mActual.mBuffer, inContext->mExpected.mBuffer);
    matches    = (comparison == 0);

    if (!matches) {
        fprintf(stderr, "ACTUAL @ %u: %s\n", inLine, inContext->mActual.mBuffer);
        fprintf(stderr, "EXPECT @ %u: %s\n", inLine, inContext->mExpected.mBuffer);
    }

    return (matches);
}

/**
 *  Test whether the production vs. non-production aspect of
 *  assertions is in effect and matches what is expected.
 */
static void TestProduction(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext *theContext = inContext;

    NL_TEST_ASSERT(inSuite, theContext->mWantProduction == NL_ASSERT_PRODUCTION);
}

#if NLASSERT_TEST_STDC
/**
 *  Test the Standard C Library macro workalike for correct operation.
 */
static void TestStandardCAssert(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = inContext;
    bool                 assert1;
    bool                 matches;

    // Test the assert, Standard C Library workalike

    assert1 = false;

    assert(assert1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mAssert, "assert1", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldAbort     == theContext->mDidAbort);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);
}
#endif

/**
 *  Test the nlASSERT macro for correct operation.
 */
static void TestNestLabsAssert(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = inContext;
    bool                 assert1, assert2;
    bool                 matches;

    // Test nlASSERT

    assert1 = assert2 = false;

    nlASSERT(assert1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mAssert, "assert1", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldAbort     == theContext->mDidAbort);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlASSERT_ACTION(assert2, nlASSERT_TEST_ACTION("assert2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mAssert, "assert2", "", "TestNestLabsAssert: assert2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldAbort     == theContext->mDidAbort);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mAssert.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);
}

static void TestAssert(nlTestSuite *inSuite, void *inContext)
{
#if NLASSERT_TEST_STDC
    TestStandardCAssert(inSuite, inContext);
#endif
    TestNestLabsAssert(inSuite, inContext);
}

/**
 *  Test the nlABORT macro for correct operation.
 */
static void TestAbort(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = inContext;
    bool                 abort1, abort2;
    bool                 matches;

    // Test nlABORT

    abort1 = abort2 = false;

    nlABORT(abort1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mAbort, "abort1", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldAbort     == theContext->mDidAbort);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlABORT_ACTION(abort2, nlASSERT_TEST_ACTION("abort2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mAbort, "abort2", "", "TestAbort: abort2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldAbort     == theContext->mDidAbort);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mAbort.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);
}

/**
 *  Test the nl[N]CHECK* style of macros for correct operation.
 */
static void TestCheck(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = inContext;
    bool                 check1, check2, check3;
    bool                 check5, check6, check7;
    int                  status1, status2, status3;
    bool                 matches;

    // Test nlCHECK

    check1 = check2 = check3 = false;

    nlCHECK(check1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "check1", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlCHECK_ACTION(check2, nlASSERT_TEST_ACTION("check2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "check2", "", "TestCheck: check2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlCHECK_PRINT(check3, "nlCHECK_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "check3", "nlCHECK_PRINT test, ", "",  __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // Test nlCHECK_SUCCESS

    status1 = status2 = status3 = -1;

    nlCHECK_SUCCESS(status1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "status1 == 0", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlCHECK_SUCCESS_ACTION(status2, nlASSERT_TEST_ACTION("status2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "status2 == 0", "", "TestCheck: status2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlCHECK_SUCCESS_PRINT(status3, "nlCHECK_SUCCESS_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "status3 == 0", "nlCHECK_SUCCESS_PRINT test, ", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // Test nlNCHECK

    check5 = check6 = check7 = true;

    nlNCHECK(check5);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "!(check5)", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNCHECK_ACTION(check6, nlASSERT_TEST_ACTION("check6"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "!(check6)", "", "TestCheck: check6 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNCHECK_PRINT(check7, "nlNCHECK_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mCheck, "!(check7)", "nlNCHECK_PRINT test, ", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mCheck.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);
}

/**
 *  Test the nl[N]VERIFY* style of macros for correct operation.
 */
static void TestVerify(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = inContext;
    bool                 verify1, verify2, verify3;
    bool                 verify4, verify5, verify6, verify7, verify8;
    int                  status1, status2, status3, status4;
    bool                 matches;

    // nlVERIFY

    verify1 = verify2 = verify3 = false;

    nlVERIFY(verify1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "verify1", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlVERIFY_ACTION(verify2, nlASSERT_TEST_ACTION("verify2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "verify2", "", "TestVerify: verify2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlVERIFY_PRINT(verify3, "nlVERIFY_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "verify3", "nlVERIFY_PRINT test, ", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlVERIFY_SUCCESS

    status1 = status2 = status3 = -1;

    nlVERIFY_SUCCESS(status1);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "status1 == 0", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlVERIFY_SUCCESS_ACTION(status2, nlASSERT_TEST_ACTION("status2"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "status2 == 0", "", "TestVerify: status2 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlVERIFY_SUCCESS_PRINT(status3, "nlVERIFY_SUCCESS_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "status3 == 0", "nlVERIFY_SUCCESS_PRINT test, ", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlNVERIFY

    verify4 = verify5 = verify6 = true;

    nlNVERIFY(verify4);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "!(verify4)", "", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNVERIFY_ACTION(verify5, nlASSERT_TEST_ACTION("verify5"));
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "!(verify5)", "", "TestVerify: verify5 test\n", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNVERIFY_PRINT(verify6, "nlNVERIFY_PRINT test");
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "!(verify6)", "nlNVERIFY_PRINT test, ", "", __LINE__ - 1);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // Tests to make sure the break keyword works as an action inside
    // control structures
   
    verify7 = false;
    status4 = -1;
    verify8 = true;

    // Because 'break' is the action, any post-action triggers
    // (i.e. trap) will be skipped. Consequently, don't assert that
    // test post-condition.

    while (true)
    {
        nlVERIFY_ACTION(verify7, break);
    }
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "verify7", "", "", __LINE__ - 2);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // Because 'break' is the action, any post-action triggers
    // (i.e. trap) will be skipped. Consequently, don't assert that
    // test post-condition.

    while (true)
    {
        nlVERIFY_SUCCESS_ACTION(status4, break);
    }
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "status4 == 0", "", "", __LINE__ - 2);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // Because 'break' is the action, any post-action triggers
    // (i.e. trap) will be skipped. Consequently, don't assert that
    // test post-condition.

    while (true)
    {
        nlNVERIFY_ACTION(verify8, break);
    }
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mVerify, "!(verify8)", "", "", __LINE__ - 2);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);
}

static void TestPreconditionVoid(unsigned int *outLine)
{
    const bool precondition1 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION(precondition1);
}

static void TestPreconditionActionVoid(unsigned int *outLine)
{
    const bool precondition2 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_ACTION(precondition2, nlASSERT_TEST_ACTION("precondition2"));
}

static void TestPreconditionPrintVoid(unsigned int *outLine)
{
    const bool precondition3 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_PRINT(precondition3, "nlPRECONDITION_PRINT test");
}

static void TestNotPreconditionVoid(unsigned int *outLine)
{
    const bool precondition4 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION(precondition4);
}

static void TestNotPreconditionActionVoid(unsigned int *outLine)
{
    const bool precondition5 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION_ACTION(precondition5, nlASSERT_TEST_ACTION("precondition5"));
}

static void TestNotPreconditionPrintVoid(unsigned int *outLine)
{
    const bool precondition6 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION_PRINT(precondition6, "nlNPRECONDITION_PRINT test");
}

static void TestPreconditionSuccessVoid(unsigned int *outLine)
{
    const int status1 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_SUCCESS(status1);
}

static void TestPreconditionSuccessActionVoid(unsigned int *outLine)
{
    const int status2 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_SUCCESS_ACTION(status2, nlASSERT_TEST_ACTION("status2"));
}

static void TestPreconditionSuccessPrintVoid(unsigned int *outLine)
{
    const int status3 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_SUCCESS_PRINT(status3, "nlPRECONDITION_SUCCESS_PRINT test");
}

static int TestPreconditionValue(unsigned int *outLine, int inStatus)
{
    const bool precondition7 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE(precondition7, inStatus);

    return 0;
}

static int TestPreconditionValueAction(unsigned int *outLine, int inStatus)
{
    const bool precondition8 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE_ACTION(precondition8, inStatus, nlASSERT_TEST_ACTION("precondition8"));

    return 0;
}

static int TestPreconditionValuePrint(unsigned int *outLine, int inStatus)
{
    const bool precondition9 = false;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE_PRINT(precondition9, inStatus, "nlPRECONDITION_VALUE_PRINT test");

    return 0;
}

static int TestPreconditionValueSuccess(unsigned int *outLine, int inStatus)
{
    const int status4 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE_SUCCESS(status4, inStatus);

    return 0;
}

static int TestPreconditionValueSuccessAction(unsigned int *outLine, int inStatus)
{
    const int status5 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE_SUCCESS_ACTION(status5, inStatus, nlASSERT_TEST_ACTION("status5"));

    return 0;
}

static int TestPreconditionValueSuccessPrint(unsigned int *outLine, int inStatus)
{
    const int status6 = -1;

    *outLine = __LINE__ + 2;

    nlPRECONDITION_VALUE_SUCCESS_PRINT(status6, inStatus, "nlPRECONDITION_VALUE_SUCCESS_PRINT test");

    return 0;
}

static int TestNotPreconditionValue(unsigned int *outLine, int inStatus)
{
    const bool precondition10 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION_VALUE(precondition10, inStatus);

    return 0;
}

static int TestNotPreconditionValueAction(unsigned int *outLine, int inStatus)
{
    const bool precondition11 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION_VALUE_ACTION(precondition11, inStatus, nlASSERT_TEST_ACTION("precondition11"));

    return 0;
}

static int TestNotPreconditionValuePrint(unsigned int *outLine, int inStatus)
{
    const bool precondition12 = true;

    *outLine = __LINE__ + 2;

    nlNPRECONDITION_VALUE_PRINT(precondition12, inStatus, "nlNPRECONDITION_VALUE_PRINT test");

    return 0;
}

/**
 *  Test the nl[N]PRECONDITION* style of macros for correct operation.
 */
static void TestPrecondition(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = (struct TestContext *)(inContext);
    unsigned int         line;
    int                  status;
    bool                 matches;

    // nlPRECONDITION{,_ACTION,_PRINT}

    TestPreconditionVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition1", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mPrecondition.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mPrecondition.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mPrecondition.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestPreconditionActionVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition2", "", "TestPreconditionActionVoid: precondition2 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestPreconditionPrintVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition3", "nlPRECONDITION_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlNPRECONDITION{,_ACTION,_PRINT}

    TestNotPreconditionVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition4)", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestNotPreconditionActionVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition5)", "", "TestNotPreconditionActionVoid: precondition5 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestNotPreconditionPrintVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition6)", "nlNPRECONDITION_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlPRECONDITION_SUCCESS{,_ACTION,_PRINT}

    TestPreconditionSuccessVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status1 == 0", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestPreconditionSuccessActionVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status2 == 0", "", "TestPreconditionSuccessActionVoid: status2 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    TestPreconditionSuccessPrintVoid(&line);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status3 == 0", "nlPRECONDITION_SUCCESS_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlPRECONDITION_VALUE{,_ACTION,_PRINT}

    status = TestPreconditionValue(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition7", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestPreconditionValueAction(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition8", "", "TestPreconditionValueAction: precondition8 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestPreconditionValuePrint(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "precondition9", "nlPRECONDITION_VALUE_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    // nlPRECONDITION_VALUE_SUCCESS{,_ACTION,_PRINT}

    status = TestPreconditionValueSuccess(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status4 == 0", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestPreconditionValueSuccessAction(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status5 == 0", "", "TestPreconditionValueSuccessAction: status5 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestPreconditionValueSuccessPrint(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "status6 == 0", "nlPRECONDITION_VALUE_SUCCESS_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    // nlNPRECONDITION_VALUE{,_ACTION,_PRINT}

    status = TestNotPreconditionValue(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition10)", "", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestNotPreconditionValueAction(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition11)", "", "TestNotPreconditionValueAction: precondition11 test\n", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);

    status = TestNotPreconditionValuePrint(&line, -EINVAL);
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mPrecondition, "!(precondition12)", "nlNPRECONDITION_VALUE_PRINT test, ", "", line);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mVerify.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    NL_TEST_ASSERT(inSuite, status == -EINVAL);
    TestContextReset(theContext);
}

/**
 *  Test the nl[N]EXPECT* style of macros for correct operation.
 */
static void TestExpect(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = (struct TestContext *)(inContext);
    bool                 expect1, expect2, expect3, expect4;
    bool                 expect5, expect6, expect7, expect8;
    int                  status1, status2, status3, status4;
    bool                 matches;

    // Establish test preconditions.

    expect1 = expect2 = expect3 = expect4 = false;
    expect5 = expect6 = expect7 = expect8 = true;
    status1 = status2 = status3 = status4 = -1;

    // nlEXPECT{,_PRINT,_ACTION,_ACTION_PRINT}

    nlEXPECT(expect1, expect_next1);
    goto expect_next2;
 expect_next1:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "expect1", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_PRINT(expect2, expect_next2, "nlEXPECT_PRINT test");
    goto expect_next3;
 expect_next2:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "expect2", "nlEXPECT_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_ACTION(expect3, expect_next3, nlASSERT_TEST_ACTION("expect3"));
    goto expect_next4;
 expect_next3:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "expect3", "", "TestExpect: expect3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_ACTION_PRINT(expect4, expect_next4, nlASSERT_TEST_ACTION("expect4"), "nlEXPECT_ACTION_PRINT");
 expect_next4:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "expect4", "nlEXPECT_ACTION_PRINT, ", "TestExpect: expect4 test\n", __LINE__ - 2);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlEXPECT_SUCCESS{,_PRINT,_ACTION,_ACTION_PRINT}

    nlEXPECT_SUCCESS(status1, expect_next5);
    goto expect_next6;
 expect_next5:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "status1 == 0", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_SUCCESS_PRINT(status2, expect_next6, "nlEXPECT_SUCCESS_PRINT test");
    goto expect_next7;
 expect_next6:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "status2 == 0", "nlEXPECT_SUCCESS_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_SUCCESS_ACTION(status3, expect_next7, nlASSERT_TEST_ACTION("status3"));
    goto expect_next8;
 expect_next7:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "status3 == 0", "", "TestExpect: status3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlEXPECT_SUCCESS_ACTION_PRINT(status4, expect_next8, nlASSERT_TEST_ACTION("status4"), "nlEXPECT_SUCCESS_ACTION_PRINT test");
    goto expect_next9;
 expect_next8:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "status4 == 0", "nlEXPECT_SUCCESS_ACTION_PRINT test, ", "TestExpect: status4 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlNEXPECT{,PRINT,ACTION,ACTION_PRINT}

    nlNEXPECT(expect5, expect_next9);
    goto expect_next10;
 expect_next9:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "!(expect5)", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNEXPECT_PRINT(expect6, expect_next10, "nlNEXPECT_PRINT test");
    goto expect_next11;
 expect_next10:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "!(expect6)", "nlNEXPECT_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNEXPECT_ACTION(expect7, expect_next11, nlASSERT_TEST_ACTION("expect7"));
    goto expect_next12;
 expect_next11:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "!(expect7)", "", "TestExpect: expect7 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNEXPECT_ACTION_PRINT(expect8, expect_next12, nlASSERT_TEST_ACTION("expect8"), "nlNEXPECT_ACTION_PRINT test");
    goto expect_next13;
 expect_next12:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mExpect, "!(expect8)", "nlNEXPECT_ACTION_PRINT test, ", "TestExpect: expect8 test\n",  __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mExpect.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

 expect_next13:
    return;
}

/**
 *  Test the nl[N]DESIRE* style of macros for correct operation.
 */
static void TestDesire(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = (struct TestContext *)(inContext);
    bool                 desire1, desire2, desire3, desire4;
    bool                 desire5, desire6, desire7, desire8;
    int                  status1, status2, status3, status4;
    bool                 matches;

    // Establish test preconditions.

    desire1 = desire2 = desire3 = desire4 = false;
    desire5 = desire6 = desire7 = desire8 = true;
    status1 = status2 = status3 = status4 = -1;

    // nlDESIRE{,_PRINT,_ACTION,_ACTION_PRINT}

    nlDESIRE(desire1, desire_next1);
    goto desire_next2;
 desire_next1:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "desire1", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_PRINT(desire2, desire_next2, "nlDESIRE_PRINT test");
    goto desire_next3;
 desire_next2:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "desire2", "nlDESIRE_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_ACTION(desire3, desire_next3, nlASSERT_TEST_ACTION("desire3"));
    goto desire_next4;
 desire_next3:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "desire3", "", "TestDesire: desire3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_ACTION_PRINT(desire4, desire_next4, nlASSERT_TEST_ACTION("desire4"), "nlDESIRE_ACTION_PRINT");
    goto desire_next5;
 desire_next4:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "desire4", "nlDESIRE_ACTION_PRINT, ", "TestDesire: desire4 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlDESIRE_SUCCESS{,_PRINT,_ACTION,_ACTION_PRINT}

    nlDESIRE_SUCCESS(status1, desire_next5);
    goto desire_next6;
 desire_next5:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "status1 == 0", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_SUCCESS_PRINT(status2, desire_next6, "nlDESIRE_SUCCESS_PRINT test");
    goto desire_next7;
 desire_next6:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "status2 == 0", "nlDESIRE_SUCCESS_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_SUCCESS_ACTION(status3, desire_next7, nlASSERT_TEST_ACTION("status3"));
    goto desire_next8;
 desire_next7:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "status3 == 0", "", "TestDesire: status3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlDESIRE_SUCCESS_ACTION_PRINT(status4, desire_next8, nlASSERT_TEST_ACTION("status4"), "nlDESIRE_SUCCESS_ACTION_PRINT test");
    goto desire_next9;
 desire_next8:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "status4 == 0", "nlDESIRE_SUCCESS_ACTION_PRINT test, ", "TestDesire: status4 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlNDESIRE{,PRINT,ACTION,ACTION_PRINT}

    nlNDESIRE(desire5, desire_next9);
    goto desire_next10;
 desire_next9:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "!(desire5)", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNDESIRE_PRINT(desire6, desire_next10, "nlNDESIRE_PRINT test");
    goto desire_next11;
 desire_next10:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "!(desire6)", "nlNDESIRE_PRINT test, ", "",  __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNDESIRE_ACTION(desire7, desire_next11, nlASSERT_TEST_ACTION("desire7"));
    goto desire_next12;
 desire_next11:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "!(desire7)", "", "TestDesire: desire7 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNDESIRE_ACTION_PRINT(desire8, desire_next12, nlASSERT_TEST_ACTION("desire8"), "nlNDESIRE_ACTION_PRINT test");
    goto desire_next13;
 desire_next12:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mDesire, "!(desire8)", "nlNDESIRE_ACTION_PRINT test, ", "TestDesire: desire8 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mDesire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

 desire_next13:
    return;
}

/**
 *  Test the nl[N]REQUIRE* style of macros for correct operation.
 */
static void TestRequire(nlTestSuite *inSuite, void *inContext)
{
    struct TestContext * theContext = (struct TestContext *)(inContext);
    bool                 require1, require2, require3, require4;
    bool                 require5, require6, require7, require8;
    int                  status1, status2, status3, status4;
    bool                 matches;

    // Establish test preconditions.

    require1 = require2 = require3 = require4 = false;
    require5 = require6 = require7 = require8 = true;
    status1 = status2 = status3 = status4 = -1;

    // nlREQUIRE{,_PRINT,_ACTION,_ACTION_PRINT}

    nlREQUIRE(require1, require_next1);
    goto require_next2;
 require_next1:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "require1", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_PRINT(require2, require_next2, "nlREQUIRE_PRINT test");
    goto require_next3;
 require_next2:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "require2", "nlREQUIRE_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_ACTION(require3, require_next3, nlASSERT_TEST_ACTION("require3"));
    goto require_next4;
 require_next3:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "require3", "", "TestRequire: require3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_ACTION_PRINT(require4, require_next4, nlASSERT_TEST_ACTION("require4"), "nlREQUIRE_ACTION_PRINT");
    goto require_next5;
 require_next4:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "require4", "nlREQUIRE_ACTION_PRINT, ", "TestRequire: require4 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlREQUIRE_SUCCESS{,_PRINT,_ACTION,_ACTION_PRINT}

    nlREQUIRE_SUCCESS(status1, require_next5);
    goto require_next6;
 require_next5:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "status1 == 0", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_SUCCESS_PRINT(status2, require_next6, "nlREQUIRE_SUCCESS_PRINT test");
    goto require_next7;
 require_next6:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "status2 == 0", "nlREQUIRE_SUCCESS_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_SUCCESS_ACTION(status3, require_next7, nlASSERT_TEST_ACTION("status3"));
    goto require_next8;
 require_next7:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "status3 == 0", "", "TestRequire: status3 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlREQUIRE_SUCCESS_ACTION_PRINT(status4, require_next8, nlASSERT_TEST_ACTION("status4"), "nlREQUIRE_SUCCESS_ACTION_PRINT test");
    goto require_next9;
 require_next8:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "status4 == 0", "nlREQUIRE_SUCCESS_ACTION_PRINT test, ", "TestRequire: status4 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    // nlNREQUIRE{,PRINT,ACTION,ACTION_PRINT}

    nlNREQUIRE(require5, require_next9);
    goto require_next10;
 require_next9:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "!(require5)", "", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNREQUIRE_PRINT(require6, require_next10, "nlNREQUIRE_PRINT test");
    goto require_next11;
 require_next10:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "!(require6)", "nlNREQUIRE_PRINT test, ", "", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNREQUIRE_ACTION(require7, require_next11, nlASSERT_TEST_ACTION("require7"));
    goto require_next12;
 require_next11:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "!(require7)", "", "TestRequire: require7 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

    nlNREQUIRE_ACTION_PRINT(require8, require_next12, nlASSERT_TEST_ACTION("require8"), "nlNREQUIRE_ACTION_PRINT test");
    goto require_next13;
 require_next12:
    matches = nlASSERT_TEST_CHECK_EXPECTED(theContext, &theContext->mRequire, "!(require8)", "nlNREQUIRE_ACTION_PRINT test, ", "TestRequire: require8 test\n", __LINE__ - 3);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldBacktrace == theContext->mDidBacktrace);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldLog       == theContext->mDidLog);
    NL_TEST_ASSERT(inSuite, theContext->mRequire.mShouldTrap      == theContext->mDidTrap);
    NL_TEST_ASSERT(inSuite, matches);
    TestContextReset(theContext);

 require_next13:
    return;
}

static const nlTest sTests[] = {
    NL_TEST_DEF("production",   TestProduction),
    NL_TEST_DEF("assert",       TestAssert),
    NL_TEST_DEF("abort",        TestAbort),
    NL_TEST_DEF("check",        TestCheck),
    NL_TEST_DEF("verify",       TestVerify),
    NL_TEST_DEF("precondition", TestPrecondition),
    NL_TEST_DEF("expect",       TestExpect),
    NL_TEST_DEF("desire",       TestDesire),
    NL_TEST_DEF("require",      TestRequire),
    NL_TEST_SENTINEL()
};

/**
 *  Set up the test suite by resetting the test context, initializing
 *  the expected configuration flags, and allocating memory for the
 *  actual and expected logging buffers.
 */
static int TestSetup(void *inContext)
{
    struct TestContext *theContext = (struct TestContext *)(inContext);
    const size_t        offset     = 0;
    const size_t        size       = 1024;
    int                 status     = SUCCESS;
    void *              p;
    bool                abort;

    TestContextReset(theContext);

    theContext->mWantProduction = NL_ASSERT_TEST_WANT_PRODUCTION;
    theContext->mIsProduction   = NL_ASSERT_PRODUCTION;

    /*
     * nlASSERT is absent in production code and should only abort in
     * non-production code when NL_ASSERT_TEST_WANT_ABORT is enabled.
     */
#if NL_ASSERT_PRODUCTION
    abort      = false;
#else
    abort      = NL_ASSERT_TEST_WANT_ABORT;
#endif

    AssertStyleContextInit(&theContext->mAssert,       __NL_ASSERT_TEST_ASSERT_FLAGS,       abort,                     !theContext->mIsProduction);
    AssertStyleContextInit(&theContext->mAbort,        __NL_ASSERT_TEST_ABORT_FLAGS,        NL_ASSERT_TEST_WANT_ABORT, true);
    AssertStyleContextInit(&theContext->mCheck,        __NL_ASSERT_TEST_CHECK_FLAGS,        false,                     !theContext->mIsProduction);
    AssertStyleContextInit(&theContext->mVerify,       __NL_ASSERT_TEST_VERIFY_FLAGS,       false,                     true);
    AssertStyleContextInit(&theContext->mPrecondition, __NL_ASSERT_TEST_PRECONDITION_FLAGS, false,                     true);
    AssertStyleContextInit(&theContext->mExpect,       __NL_ASSERT_TEST_EXPECT_FLAGS,       false,                     true);
    AssertStyleContextInit(&theContext->mDesire,       __NL_ASSERT_TEST_DESIRE_FLAGS,       false,                     true);
    AssertStyleContextInit(&theContext->mRequire,      __NL_ASSERT_TEST_REQUIRE_FLAGS,      false,                     true);

    p = malloc(size);

    if (p == NULL)
    {
        status = FAILURE;
    }
    else
    {
        theContext->mActual.mBuffer = p;
        theContext->mActual.mBufferOffset = offset;
        theContext->mActual.mBufferSize = size;

        p = malloc(size);

        if (p == NULL)
        {
            status = FAILURE;
        }
        else
        {
            theContext->mExpected.mBuffer = p;
            theContext->mExpected.mBufferOffset = offset;
            theContext->mExpected.mBufferSize = size;
        }
    }

    return (status);
}

/**
 *  Tear down the test suite by deallocating memory for the actual and
 *  expected logging buffers and resetting the test context.
 */
static int TestTeardown(void *inContext)
{
    struct TestContext *theContext = (struct TestContext *)(inContext);

    if (theContext->mActual.mBuffer != NULL) {
        free(theContext->mActual.mBuffer);
    }

    if (theContext->mExpected.mBuffer != NULL) {
        free(theContext->mExpected.mBuffer);
    }

    TestContextReset(theContext);

    return (SUCCESS);
}

int main(void)
{
    nlTestSuite theSuite = {
        "nlassert",
        &sTests[0],
        TestSetup,
        TestTeardown
    };

    nlTestSetOutputStyle(OUTPUT_CSV);

    nlTestRunner(&theSuite, &sContext);

    return nlTestRunnerStats(&theSuite);
}
