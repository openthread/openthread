/*
 *
 *    Copyright (c) 2012-2016 Nest Labs, Inc.
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
 *      compile-time assertion library.
 *
 */

#include <stdint.h>

#include "nlassert-test-config.h"

#define NL_ASSERT_PRODUCTION                         (NL_ASSERT_TEST_WANT_PRODUCTION)

#include <nlassert.h>

#include <nltest.h>

#if NL_ASSERT_TEST_WANT_STATIC_SUCCESS
#define _TEST_STATIC_OP ==
#else
#define _TEST_STATIC_OP !=
#endif /* NL_ASSERT_TEST_WANT_STATIC_SUCCESS */

/**
 *  Test static, compile-time assertions.
 */
static void TestStatic(nlTestSuite *inSuite, void *inContext)
{
    uint32_t test;

    nlSTATIC_ASSERT(sizeof(test) _TEST_STATIC_OP 4);

    nlSTATIC_ASSERT_PRINT(sizeof(test) _TEST_STATIC_OP 4, "nlASSERT_PRINT_STATIC Failed");

    nlSTATIC_ABORT(sizeof(test) _TEST_STATIC_OP 4);

    nlSTATIC_ABORT_PRINT(sizeof(test) _TEST_STATIC_OP 4, "nlABORT_PRINT_STATIC Failed");
}

static const nlTest sTests[] = {
    NL_TEST_DEF("static",       TestStatic),
    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "nlassert-static",
        &sTests[0],
        NULL,
        NULL
    };

    nlTestSetOutputStyle(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
