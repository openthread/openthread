/*
 *
 *   Copyright 2015-2016 Nest Labs Inc. All Rights Reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

/**
 *    @file
 *      This file provides an ISO/IEC 9899:1999-, C89-, and
 *      C99-compatible Standard C Library header and assertion
 *      interface definition, implemented atop Nest Labs assertion
 *      checking and runtime exception interfaces such that consistent
 *      platform and system capabilities, behavior, and output may
 *      be implemented and enforced across the two interfaces.
 *
 *      Systems wishing to use this in lieu of their Standard C
 *      Library header of the same name should ensure that their
 *      toolchain is configured to either ignore or deprioritize
 *      standard search paths while placing the directory this header
 *      is contained in among the preferred header search paths.
 *
 */

#ifndef NL_ASSERT_STDC_ASSERT_H
#define NL_ASSERT_STDC_ASSERT_H

#include <nlassert.h>

/**
 *  @def assert(aExpression)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    aborts the current program execution if the condition is false.
 *
 *  This provides a workalike macro for the ISO/IEC 9899:1999, C89,
 *  and C99 Standard C Library
 *  [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html)
 *  macro interface and bases it upon the equivalent #nlABORT macro,
 *  which has the same semantics, and the same behavior when active. The
 *  difference between the #nlABORT macro and this assert() macro is that
 *  #nlABORT is always active, while this one is inactive when NDEBUG is
 *  defined and active when NDEBUG is undefined. Note that when this macro
 *  is inactive, the test is __completely__ elided; side effects, if any,
 *  in the tested expression will not be produced.
 *
 *  System integrators may want to use this as opposed to their native
 *  Standard C Library assert() to ensure consistent capabilities,
 *  behavior, and output across their software modules when runtime
 *  assertions occur where the Nest Labs assertion checking and
 *  runtime exception handling is also used.
 *
 *  @param[in]  aExpression  A Boolean expression to be evaluated.
 *
 *  @sa #nlASSERT
 *
 */
#ifdef NDEBUG
#ifdef __cplusplus
#define assert(aExpression) (static_cast<void> (0))
#else
#define assert(aExpression) ((void) (0))
#endif
#else
#define assert(aExpression) nlABORT(aExpression)
#endif

#endif /* NL_ASSERT_STDC_ASSERT_H */
