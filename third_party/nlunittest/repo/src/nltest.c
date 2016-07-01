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
 *      This file implements functions that effect a simple, portable
 *      unit test suite framework.
 *
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <nltest.h>

/* Function Prototypes */

static void def_log_name(struct _nlTestSuite* inSuite);
static void def_log_setup(struct _nlTestSuite* inSuite, int inResult, int inWidth);
static void def_log_test(struct _nlTestSuite *inSuite, int inWidth, int inIndex);
static void def_log_teardown(struct _nlTestSuite* inSuite, int inResult, int inWidth);
static void def_log_statTest(struct _nlTestSuite* inSuite);
static void def_log_statAssert(struct _nlTestSuite* inSuite);
static void csv_log_name(struct _nlTestSuite* inSuite);
static void csv_log_setup(struct _nlTestSuite* inSuite, int inResult, int inWidth);
static void csv_log_test(struct _nlTestSuite *inSuite, int inWidth, int inIndex);
static void csv_log_teardown(struct _nlTestSuite* inSuite, int inResult, int inWidth);
static void csv_log_statTest(struct _nlTestSuite* inSuite);
static void csv_log_statAssert(struct _nlTestSuite* inSuite);

/* Global Variables */

static nl_test_output_logger_t nl_test_logger_default = {
    def_log_name,
    def_log_setup,
    def_log_test,
    def_log_teardown,
    def_log_statTest,
    def_log_statAssert,
};

static nl_test_output_logger_t nl_test_logger_csv = {
    csv_log_name,
    csv_log_setup,
    csv_log_test,
    csv_log_teardown,
    csv_log_statTest,
    csv_log_statAssert,
};

/* Global Output Style Variable */

static const nl_test_output_logger_t *logger_output = &nl_test_logger_default;

static int isSentinel(const nlTest* inTests, size_t inIndex)
{
    if (inTests[inIndex].name == NULL
        && inTests[inIndex].function == NULL)
    {
        return 1;
    }

    return 0;
}

/* Human-readable (Default) Output Functions */

static void def_log_name(struct _nlTestSuite* inSuite)
{
    printf("[ %s ]\n", inSuite->name);
}

static void def_log_setup(struct _nlTestSuite* inSuite, int inResult, int inWidth)
{
    printf("[ %s : %-*s ] : %s\n", inSuite->name, inWidth, "Setup", inResult == FAILURE ? "FAILED" : "PASSED");
}

static void def_log_test(struct _nlTestSuite *inSuite, int inWidth, int inIndex)
{
    printf("[ %s : %-*s ] : %s\n", inSuite->name, inWidth, inSuite->tests[inIndex].name, inSuite->flagError ? "FAILED" : "PASSED");
}

static void def_log_teardown(struct _nlTestSuite* inSuite, int inResult, int inWidth)
{
    printf("[ %s : %-*s ] : %s\n", inSuite->name, inWidth, "TearDown", inResult == FAILURE ? "FAILED" : "PASSED");
}

static void def_log_statTest(struct _nlTestSuite* inSuite)
{
    printf("Failed Tests:   %d / %d\n", inSuite->failedTests, inSuite->runTests);
}

static void def_log_statAssert(struct _nlTestSuite* inSuite)
{
    printf("Failed Asserts: %d / %d\n", inSuite->failedAssertions, inSuite->performedAssertions);
}

/* CSV Output Functions */

static void csv_log_name(struct _nlTestSuite* inSuite)
{
    printf("'#0:','%s'\n", inSuite->name);
}

static void csv_log_setup(struct _nlTestSuite* inSuite, int inResult, int inWidth)
{
    printf("'#1:','%-*s','%s'\n", inWidth, "Setup", inResult == FAILURE ? "FAILED" : "PASSED");
}

static void csv_log_test(struct _nlTestSuite *inSuite, int inWidth, int inIndex)
{
    printf("'#2:','%-*s','%s'\n", inWidth, inSuite->tests[inIndex].name, inSuite->flagError ? "FAILED" : "PASSED");
}

static void csv_log_teardown(struct _nlTestSuite* inSuite, int inResult, int inWidth)
{
    printf("'#3:','%-*s','%s'\n", inWidth, "Teardown", inResult == FAILURE ? "FAILED" : "PASSED");
}

static void csv_log_statTest(struct _nlTestSuite* inSuite)
{
    printf("'#4','%d','%d'\n", inSuite->failedTests, inSuite->runTests);
}

static void csv_log_statAssert(struct _nlTestSuite* inSuite)
{
    printf("'#5','%d','%d'\n", inSuite->failedAssertions, inSuite->performedAssertions);
}

void nlTestRunner(struct _nlTestSuite* inSuite, void* inContext)
{
    int i = 0;
    size_t len, max = 0;

    logger_output->PrintName(inSuite);

    /* Determine the maximum test name length */

    for (i = 0; i < kTestSuiteMaxTests; i++)
    {
        if (isSentinel(inSuite->tests, i))
            break;

        len = strlen(inSuite->tests[i].name);
        if (len > max)
            max = len;
    }
    
    inSuite->runTests = 0;
    inSuite->failedTests = 0;
    inSuite->performedAssertions = 0;
    inSuite->failedAssertions = 0;

    /* Run the tests and display the test and summary result */
    if (inSuite->setup != NULL)
    {
        int resSetup = inSuite->setup(inContext);
        logger_output->PrintSetup(inSuite, resSetup, max);
    }
    for (i = 0; i < kTestSuiteMaxTests; i++)
    {
        if (isSentinel(inSuite->tests, i))
            break;

        inSuite->flagError = false;
        inSuite->tests[i].function(inSuite, inContext);
        inSuite->runTests += 1;
        if (inSuite->flagError)
            inSuite->failedTests += 1;

        logger_output->PrintTest(inSuite, max, i);
    }
    if (inSuite->tear_down != NULL)
    {
        int resTeardown = inSuite->tear_down(inContext);
        logger_output->PrintTeardown(inSuite, resTeardown, max);
    }
}

int nlTestRunnerStats(struct _nlTestSuite* inSuite)
{
    logger_output->PrintStatTests(inSuite);
    logger_output->PrintStatAsserts(inSuite);

    return 0 - inSuite->failedTests;
}

void nlTestSetOutputStyle(nlTestOutputStyle inStyle)
{
    if (inStyle == OUTPUT_DEF)
    {
        logger_output = &nl_test_logger_default;
    }
    else if (inStyle == OUTPUT_CSV)
    {
        logger_output = &nl_test_logger_csv;
    }
}

void nlTestSetLogger(const nlTestOutputLogger* inLogger)
{
    logger_output = inLogger;
}

