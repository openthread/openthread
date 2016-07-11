/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
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
 *      This file defines configuration mnemonics that can be enabled
 *      or disabled to effect different test configurations for the
 *      runtime assertion library.
 *
 */

#ifndef NLASSSERT_TEST_CONFIG_H
#define NLASSSERT_TEST_CONFIG_H

/**
 *  @def NL_ASSERT_TEST_WANT_STATIC_SUCCESS
 *
 *  @brief
 *    TBD
 *
 */
#ifndef NL_ASSERT_TEST_WANT_STATIC_SUCCESS
#define NL_ASSERT_TEST_WANT_STATIC_SUCCESS         1
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_PRODUCTION
 *
 *  @brief
 *    TBD
 *
 */
#ifndef NL_ASSERT_TEST_WANT_PRODUCTION
#define NL_ASSERT_TEST_WANT_PRODUCTION             0
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_DEFAULTS
 *
 *  @brief
 *    TBD
 *
 */
#ifndef NL_ASSERT_TEST_WANT_DEFAULTS
#define NL_ASSERT_TEST_WANT_DEFAULTS               0
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_ABORT
 *
 *  @brief
 *    Enable (1) or disable (0) a test harness-specific override for
 *    #NL_ASSERT_ABORT().
 *
 *    Enabling this enables tests to ensure that abort functionality
 *    works correctly.
 *
 */
#ifndef NL_ASSERT_TEST_WANT_ABORT
#define NL_ASSERT_TEST_WANT_ABORT                  1
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_LOG
 *
 *  @brief
 *    TBD
 *
 *    TBD
 *
 */
#ifndef NL_ASSERT_TEST_WANT_LOG
#define NL_ASSERT_TEST_WANT_LOG                    0
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_BACKTRACE
 *
 *  @brief
 *    Enable (1) or disable (0) a test harness-specific override for
 *    #NL_ASSERT_BACKTRACE().
 *
 *    Enabling this enables tests to ensure that backtrace functionality
 *    works correctly.
 *
 */
#ifndef NL_ASSERT_TEST_WANT_BACKTRACE
#define NL_ASSERT_TEST_WANT_BACKTRACE              0
#endif

/**
 *  @def NL_ASSERT_TEST_WANT_TRAP
 *
 *  @brief
 *    Enable (1) or disable (0) a test harness-specific override for
 *    #NL_ASSERT_TRAP().
 *
 *    Enabling this enables tests to ensure that trap functionality
 *    works correctly.
 *
 */
#ifndef NL_ASSERT_TEST_WANT_TRAP
#define NL_ASSERT_TEST_WANT_TRAP                   0
#endif

#endif /* NLASSSERT_TEST_CONFIG_H */
