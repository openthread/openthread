/**
 *    Copyright 2012-2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file defines macros, constants, data structures, and
 *      functions that effect a simple, portable unit test suite
 *      framework.
 *
 */

#ifndef __NLTEST_H_INCLUDED__
#define __NLTEST_H_INCLUDED__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#else
#endif /* __cplusplus */

/**
 * @addtogroup typedef Type Definitions
 *
 * @{
 *
 */

struct _nlTestSuite;

/**
 * This defines a function entry point for a test in a test suite.
 *
 * @param[inout]  inSuite     A pointer to the test suite being run.
 * @param[inout]  inContext   A pointer to test suite-specific context
 *                            provided by the test suite driver.
 *
 */
typedef void (*nlTestFunction)(struct _nlTestSuite* inSuite, void* inContext);

/**
 * This structure is used to define a single test for use in a test suite.
 *
 */
typedef struct _nlTest
{
    const char*    name;                /**< Brief descriptive name of the test */
    nlTestFunction function;            /**< Function entry point for the test */
} nlTest;

/**
 * This structure is used to define the test suite, an array of tests
 * for a given module.
 *
 * It has a name for the suite, the array of tests, as well as a setup
 * function which is called before the execution of the tests and a
 * teardown function which is called afterwards.
 *
 */
typedef struct _nlTestSuite
{
    /* Public Members */
    const char*    name;                /**< Brief descriptive name of the test suite */
    const nlTest*  tests;               /**< Array of tests in the suite */

    /**
     * This function is responsible for, if non-NULL, performing
     * initial setup for the test suite, before running.
     *
     * @param[inout]  inContext   A pointer to test suite-specific context
     *                            provided by the test suite driver.
     *
     */ 
    int            (*setup)(void *inContext);

    /**
     * This function is responsible for, if non-NULL, performing
     * final tear down for the test suite, after running.
     *
     * @param[inout]  inContext   A pointer to test suite-specific context
     *                            provided by the test suite driver.
     *
     */ 
    int            (*tear_down)(void *inContext);

    int            runTests;            /**< Total number of tests performed in the suite */
    int            failedTests;         /**< Total number of tests failed in the suite */
    int            performedAssertions; /**< Total number of test assertions performed in the suite */
    int            failedAssertions;    /**< Total number of test assertions failed in the suite */
    
    /* Private Members */
    bool           flagError;
} nlTestSuite;

/**
 * Output style for tests and test summaries.
 */
typedef enum _nlTestOutputStyle
{
    OUTPUT_DEF = 0,   /**< Generate human-readable output (default). */
    OUTPUT_CSV = 1,   /**< Generate machine-readable, comma-separated value (CSV) output. */
} nlTestOutputStyle;

/**
 * This structure contains function pointers for functions that output
 * the test suite name as well as the status of:
 *
 *   * Test suite setup and teardown functions.
 *   * Tests and result functions (failed tests, failed assertions).
 *
 * All functions take a pointer to the test suite along with
 * additional parameters, as needed, for the particular function.
 *
 * Custom instances of this structure may be instantiated and
 * populated with custom output functions and then globally set via
 * nlTestSetLogger.
 *
 */
typedef struct _nlTestOutputLogger {
    /**
     * This function is responsible for rendering the name of the test
     * suite.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the name should be rendered.
     *
     */ 
    void (*PrintName)(struct _nlTestSuite*inSuite);

    /**
     * This function is responsible for rendering the status of the test
     * suite setup.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the test suite setup status should be
     *                             rendered.
     *  @param[in]    inResult     The status of the test suite setup to
     *                             be rendered.
     *  @param[in]    inWidth      The maximum width, in characters,
     *                             allowed for rendering the setup name
     *                             or phase.
     *
     */ 
    void (*PrintSetup)(struct _nlTestSuite*inSuite, int inResult, int inWidth);

    /**
     * This function is responsible for rendering the summary of a test
     * run, indicating success or failure of that test.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the test run status summary should be
     *                             rendered.
     *  @param[in]    inWidth      The maximum width, in characters,
     *                             allowed for rendering the test name.
     *  @param[in]    inIndex      The index of the test in the suite
     *                             for which to render the summary of the
     *                             test run.
     *
     */
    void (*PrintTest)(struct _nlTestSuite*inSuite, int inWidth, int inIndex);

    /**
     * This function is responsible for rendering the status of the test
     * suite teardown.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the test suite teardown status should be
     *                             rendered.
     *  @param[in]    inResult     The status of the test suite setup to
     *                             be rendered.
     *  @param[in]    inWidth      The maximum width, in characters,
     *                             allowed for rendering the setup name
     *                             or phase.
     *
     */
    void (*PrintTeardown)(struct _nlTestSuite*inSuite, int inResult, int inWidth);

    /**
     * This function is responsible for rendering the test suite run
     * statistics, including the number of failed tests and the total
     * number of tests run.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the test suite test statistics should be
     *                             rendered.
     *
     */
    void (*PrintStatTests)(struct _nlTestSuite*inSuite);

    /**
     * This function is responsible for rendering the test suite assertion
     * statistics, including the number of failed assertions and the total
     * number of assertions evaluated.
     *
     *  @param[in]    inSuite      A pointer to the test suite for which
     *                             the test suite assertion statistics should
     *                             be rendered.
     *
     */
    void (*PrintStatAsserts)(struct _nlTestSuite*inSuite);
} nlTestOutputLogger;

/**
 *  @}
 *
 */

/**
 *  @addtogroup cpp Preprocessor Definitions and Macros
 *  
 *  @{
 */

/**
 *  @def kTestSuiteMaxTests
 *
 *  @brief
 *    Defines the maximum number of tests allowed in a single test suite.
 *
 */    
#define kTestSuiteMaxTests (64)

/**
 *  @def SUCCESS
 *
 *  @brief
 *    Defines successful return status from test suite functions
 *
 */
#define SUCCESS 0

/**
 *  @def FAILURE
 *
 *  @brief
 *    Defines failed return status from test suite functions
 *
 */
#define FAILURE -1

#ifdef __cplusplus
#define _NL_TEST_ASSIGN(field, value)	value
#else
#define _NL_TEST_ASSIGN(field, value)	.field = value
#endif /* __cplusplus */

/**
 *  @def NL_TEST_DEF(inName, inFunction)
 *
 *  @brief
 *    This macro makes an test assignment in a test suite, associating
 *    the specified function @a inFunction with the provided string @a
 *    inName.
 *
 *  @param[in]    inName       A pointer to a NULL-terminated C string
 *                             briefly describing the test.
 *  @param[in]    inFunction   A pointer to the function entry point for
 *                             the test.
 *
 */
#define NL_TEST_DEF(inName, inFunction)               \
{                                                     \
    _NL_TEST_ASSIGN(name, inName),                    \
    _NL_TEST_ASSIGN(function, inFunction)             \
}

/**
 *  @def NL_TEST_SENTINEL()
 *
 *  @brief
 *    This macro must be used as the final entry to terminate an array
 *    of test assignments to a test suite.
 *
 */
#define NL_TEST_SENTINEL()                            NL_TEST_DEF(NULL, NULL)

/**
 *  @def NL_TEST_ASSERT(inSuite, inCondition)
 *
 *  @brief
 *    This is used to assert the results of a conditional check
 *    through out a test in a test suite.
 *
 *  @param[in]    inSuite      A pointer to the test suite the assertion
 *                             should be accounted against.
 *  @param[in]    inCondition  Code for the logical predicate to be checked
 *                             for truth. If the condition fails, the
 *                             assertion fails.
 *
 */
#define NL_TEST_ASSERT(inSuite, inCondition)          \
    do {                                              \
        (inSuite)->performedAssertions += 1;          \
                                                      \
        if (!(inCondition))                           \
        {                                             \
            printf("Failed assert: %s in %s:%u\n",    \
                   #inCondition, __FILE__, __LINE__); \
            (inSuite)->failedAssertions += 1;         \
            (inSuite)->flagError = true;              \
        }                                             \
    } while (0)

/**
 * @}
 *
 */

/**
 * This runs all the functions for each test specified in the current
 * suite and outputs the results for each function using the
 * currently-set logger methods.
 *
 * @param[inout]  inSuite     A pointer to the test suite being run.
 * @param[inout]  inContext   A pointer to test suite-specific context
 *                            that will be provided to each test invoked
 *                            within the suite.
 *
 */
extern void nlTestRunner(struct _nlTestSuite* inSuite, void* inContext);

/**
 * This summarizes the number of run and failed tests as well as the
 * number of performed and failed assertions for the suite using the
 * currently-set logger methods.
 *
 * @param[inout]  inSuite     A pointer to the test suite being run.
 *
 * @returns SUCCESS on success; otherwise, FAILURE.
 */
extern int  nlTestRunnerStats(struct _nlTestSuite* inSuite);

/**
 * This globally sets the output style to be used for summarizing a
 * suite test run.
 *
 * @note This supports selecting among built-in logger methods. Custom
 *       logger methods may be set through the nlTestSetLogger() interface.
 *
 * @param[in]     inStyle     The style to be used for summarizing a
 *                            suite test run.
 *
 */
extern void nlTestSetOutputStyle(nlTestOutputStyle inStyle);

/**
 * This globally sets the logger methods to be used for summarizing a
 * suite test run.
 *
 * @param[in]     inLogger    A pointer to the logger methods to be used
 *                            for summarizing a suite test run.
 *
 */
extern void nlTestSetLogger(const nlTestOutputLogger* inLogger);

/**
 *  @addtogroup compat Compatibility Types and Interfaces
 *  
 *  Deprecated legacy types and interfaces. New usage of these types
 *  and interfaces is discouraged.
 *
 *  @{
 */

/**
 * Legacy type for output style for tests and test summaries.
 *
 */
typedef nlTestOutputStyle  nl_test_outputStyle;

/**
 * Legacy type for output functions.
 *
 */
typedef nlTestOutputLogger nl_test_output_logger_t;

/**
 *  @def nl_test_set_output_style(inStyle)
 *
 *  @note See nlTestSetOutputStyle() for the equivalent non-deprecated
 *        interface.
 *
 *  @brief
 *    This globally sets the output style to be used for summarizing a
 *    suite test run.
 *
 *  @param[in]    inStyle     The style to be used for summarizing a
 *                            suite test run.
 *
 */
#define nl_test_set_output_style(inStyle) nlTestSetOutputStyle(inStyle)

/**
 *  @def nl_test_set_logger(inLogger)
 *
 *  @note See nlTestSetLogger() for the equivalent non-deprecated
 *        interface.
 *
 *  @brief
 *    This globally sets the output style to be used for summarizing a
 *    suite test run.
 *
 *  @param[in]    inLogger    A pointer to the logger methods to be used
 *                            for summarizing a suite test run.
 *
 */
#define nl_test_set_logger(inLogger)      nlTestSetLogger(inLogger)

/**
 *  @}
 *
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NLTEST_H_INCLUDED__ */
