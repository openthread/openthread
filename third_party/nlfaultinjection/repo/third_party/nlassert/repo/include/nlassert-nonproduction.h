/*
 *   Copyright 2010-2016 Nest Labs Inc. All Rights Reserved.
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
 *      This file defines macros and interfaces for performing
 *      compile- and run-time assertion checking and run-time
 *      exception handling when #NL_ASSERT_PRODUCTION is false.
 *
 */

#ifndef NLCORE_NLASSERT_NONPRODUCTION_H
#define NLCORE_NLASSERT_NONPRODUCTION_H

#include "nlassert-internal.h"

/**
 *  @ingroup check-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for check-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_CHECK_NONPRODUCTION_FLAGS NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT                             (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG | NL_ASSERT_FLAG_TRAP)

/**
 *  @def NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for check-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_CHECK_NONPRODUCTION_FLAGS                                     NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS)
#define NL_ASSERT_CHECK_NONPRODUCTION_FLAGS                                     (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlCHECK(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, if
 *    the condition is false.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY
 *
 */
#define nlCHECK(aCondition)                                                     __nlCHECK(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlCHECK_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    executes @p anAction if the condition is false.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_ACTION
 *
 */
#define nlCHECK_ACTION(aCondition, anAction)                                    __nlCHECK_ACTION(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @def nlCHECK_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    prints @p aMessage if the condition is false.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_PRINT
 *
 */
#define nlCHECK_PRINT(aCondition, aMessage)                                     __nlCHECK_PRINT(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition, aMessage)

/**
 *  @def nlCHECK_SUCCESS(aStatus)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, if the status is unsuccessful.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_SUCCESS
 *
 */
#define nlCHECK_SUCCESS(aStatus)                                                __nlCHECK_SUCCESS(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aStatus)

/**
 *  @def nlCHECK_SUCCESS_ACTION(aStatus, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, and executes @p anAction if the status is
 *    unsuccessful.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_SUCCESS_ACTION
 *
 */
#define nlCHECK_SUCCESS_ACTION(aStatus, anAction)                               __nlCHECK_SUCCESS_ACTION(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aStatus, anAction)

/**
 *  @def nlCHECK_SUCCESS_PRINT(aStatus, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, and prints @p aMessage if the status is
 *    unsuccessful.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_SUCCESS_PRINT
 *
 */
#define nlCHECK_SUCCESS_PRINT(aStatus, aMessage)                                __nlCHECK_SUCCESS_PRINT(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aStatus, aMessage)

/**
 *  @def nlNCHECK(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, if
 *    the condition is true.
 *
 *  @note This is the logical inverse of #nlCHECK
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlCHECK
 *  @sa #nlNVERIFY
 *
 */
#define nlNCHECK(aCondition)                                                    __nlNCHECK(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlNCHECK_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, and
 *    executes @p anAction if the condition is true.
 *
 *  @note This is the logical inverse of #nlCHECK_ACTION
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_ACTION
 *  @sa #nlNVERIFY_ACTION
 *
 */
#define nlNCHECK_ACTION(aCondition, anAction)                                   __nlNCHECK_ACTION(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @def nlNCHECK_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, and
 *    prints @p aMessage if the condition is true.
 *
 *  @note This is the logical inverse of #nlCHECK_PRINT
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_PRINT
 *  @sa #nlNVERIFY_PRINT
 *
 */
#define nlNCHECK_PRINT(aCondition, aMessage)                                    __nlNCHECK_PRINT(NL_ASSERT_CHECK_NONPRODUCTION_FLAGS, aCondition, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup verify-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for verify-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT                            (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG | NL_ASSERT_FLAG_TRAP)

/**
 *  @def NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for verify-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS                                    NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS)
#define NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS                                    (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlVERIFY(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, if
 *    the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK
 *
 */
#define nlVERIFY(aCondition)                                                    __nlVERIFY(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlVERIFY_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    executes @p anAction if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_ACTION
 *
 */
#define nlVERIFY_ACTION(aCondition, anAction)                                   __nlVERIFY_ACTION(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @def nlVERIFY_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    prints @p aMessage if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_PRINT
 *
 */
#define nlVERIFY_PRINT(aCondition, aMessage)                                    __nlVERIFY_PRINT(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition, aMessage)

/**
 *  @def nlVERIFY_SUCCESS(aStatus)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, if the status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_SUCCESS
 *
 */
#define nlVERIFY_SUCCESS(aStatus)                                               __nlVERIFY_SUCCESS(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aStatus)

/**
 *  @def nlVERIFY_SUCCESS_ACTION(aStatus, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, and executes @p anAction if the status is
 *    unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_SUCCESS_ACTION
 *
 */
#define nlVERIFY_SUCCESS_ACTION(aStatus, anAction)                              __nlVERIFY_SUCCESS_ACTION(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aStatus, anAction)

/**
 *  @def nlVERIFY_SUCCESS_PRINT(aStatus, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)) and takes action, based
 *    on configuration, and prints @p aMessage if the status is
 *    unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_SUCCESS_PRINT
 *
 */
#define nlVERIFY_SUCCESS_PRINT(aStatus, aMessage)                               __nlVERIFY_SUCCESS_PRINT(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aStatus, aMessage)

/**
 *  @def nlNVERIFY(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, if
 *    the condition is true.
 *
 *  @note This is the logical inverse of #nlVERIFY
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY
 *  @sa #nlNCHECK
 *
 */
#define nlNVERIFY(aCondition)                                                   __nlNVERIFY(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlNVERIFY_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, and
 *    executes @p anAction if the condition is true.
 *
 *  @note This is the logical inverse of #nlVERIFY_ACTION
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_ACTION
 *  @sa #nlNCHECK_ACTION
 *
 */
#define nlNVERIFY_ACTION(aCondition, anAction)                                  __nlNVERIFY_ACTION(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @def nlNVERIFY_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false and takes action, based on configuration, and
 *    prints @p aMessage if the condition is true.
 *
 *  @note This is the logical inverse of #nlVERIFY_PRINT
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #nlVERIFY_PRINT
 *  @sa #nlNCHECK_PRINT
 *
 */
#define nlNVERIFY_PRINT(aCondition, aMessage)                                   __nlNVERIFY_PRINT(NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS, aCondition, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup desire-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for desire-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT                            (NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for desire-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS                                    NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS)
#define NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS                                    (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlDESIRE(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and branches to @p aLabel if the condition is
 *    false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT
 *  @sa #nlREQUIRE
 *
 */
#define nlDESIRE(aCondition, aLabel)                                            __nlEXPECT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @def nlDESIRE_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_PRINT
 *  @sa #nlREQUIRE_PRINT
 *
 */
#define nlDESIRE_PRINT(aCondition, aLabel, aMessage)                            __nlEXPECT_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlDESIRE_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both executes @p anAction and branches to
 *    @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION
 *  @sa #nlREQUIRE_ACTION
 *
 */
#define nlDESIRE_ACTION(aCondition, aLabel, anAction)                           __nlEXPECT_ACTION(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION_PRINT
 *  @sa #nlREQUIRE_ACTION_PRINT
 *
 */
#define nlDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)           __nlEXPECT_ACTION_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @def nlDESIRE_SUCCESS(aStatus, aLabel)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and branches to @p
 *    aLabel if the status is unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS
 *  @sa #nlREQUIRE_SUCCESS
 *
 */
#define nlDESIRE_SUCCESS(aStatus, aLabel)                                       __nlEXPECT_SUCCESS(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aStatus, aLabel)

/**
 *  @def nlDESIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both prints @p
 *    aMessage and then branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_PRINT
 *  @sa #nlREQUIRE_SUCCESS_PRINT
 *
 */
#define nlDESIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)                       __nlEXPECT_SUCCESS_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, aMessage)

/**
 *  @def nlDESIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both executes @p
 *    anAction and branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION
 *  @sa #nlREQUIRE_SUCCESS_ACTION
 *
 */
#define nlDESIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)                      __nlEXPECT_SUCCESS_ACTION(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, anAction)

/**
 *  @def nlDESIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), prints @p aMessage,
 *    executes @p anAction, and branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION_PRINT
 *  @sa #nlREQUIRE_SUCCESS_ACTION_PRINT
 *
 */
#define nlDESIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)      __nlEXPECT_SUCCESS_ACTION_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, anAction, aMessage)

/**
 *  @def nlNDESIRE(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and branches to @p aLabel if the condition is
 *    true.
 *
 *  @note This is the logical inverse of #nlDESIRE.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlDESIRE
 *  @sa #nlNEXPECT
 *  @sa #nlNREQUIRE
 *
 */
#define nlNDESIRE(aCondition, aLabel)                                           __nlNEXPECT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @def nlNDESIRE_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlDESIRE_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlDESIRE_PRINT
 *  @sa #nlNEXPECT_PRINT
 *  @sa #nlNREQUIRE_PRINT
 *
 */
#define nlNDESIRE_PRINT(aCondition, aLabel, aMessage)                           __nlNEXPECT_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlNDESIRE_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both executes @p anAction and branches to
 *    @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlDESIRE_ACTION.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlDESIRE_ACTION
 *  @sa #nlNEXPECT_ACTION
 *  @sa #nlNREQUIRE_ACTION
 *
 */
#define nlNDESIRE_ACTION(aCondition, aLabel, anAction)                          __nlNEXPECT_ACTION(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlNDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlDESIRE_ACTION_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlDESIRE_ACTION_PRINT
 *  @sa #nlNEXPECT_ACTION_PRINT
 *  @sa #nlNREQUIRE_ACTION_PRINT
 *
 */
#define nlNDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)          __nlNEXPECT_ACTION_PRINT(NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup require-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for require-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT                           (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG | NL_ASSERT_FLAG_TRAP)

/**
 *  @def NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for require-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS                                   NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS)
#define NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS                                   (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlREQUIRE(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and branches to @p aLabel if the condition is
 *    false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT
 *  @sa #nlDESIRE
 *
 */
#define nlREQUIRE(aCondition, aLabel)                                           __nlEXPECT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @def nlREQUIRE_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_PRINT
 *  @sa #nlDESIRE_PRINT
 *
 */
#define nlREQUIRE_PRINT(aCondition, aLabel, aMessage)                           __nlEXPECT_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlREQUIRE_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both executes @p anAction and branches to
 *    @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION
 *  @sa #nlDESIRE_ACTION
 *
 */
#define nlREQUIRE_ACTION(aCondition, aLabel, anAction)                          __nlEXPECT_ACTION(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION_PRINT
 *  @sa #nlDESIRE_ACTION_PRINT
 *
 */
#define nlREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)          __nlEXPECT_ACTION_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @def nlREQUIRE_SUCCESS(aStatus, aLabel)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and branches to @p
 *    aLabel if the status is unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS
 *  @sa #nlDESIRE_SUCCESS
 *
 */
#define nlREQUIRE_SUCCESS(aStatus, aLabel)                                      __nlEXPECT_SUCCESS(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aStatus, aLabel)

/**
 *  @def nlREQUIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both prints @p
 *    aMessage and then branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_PRINT
 *  @sa #nlDESIRE_SUCCESS_PRINT
 *
 */
#define nlREQUIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)                      __nlEXPECT_SUCCESS_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, aMessage)

/**
 *  @def nlREQUIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both executes @p
 *    anAction and branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION
 *  @sa #nlDESIRE_SUCCESS_ACTION
 *
 */
#define nlREQUIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)                     __nlEXPECT_SUCCESS_ACTION(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, anAction)

/**
 *  @def nlREQUIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), prints @p aMessage,
 *    executes @p anAction, and branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION_PRINT
 *  @sa #nlDESIRE_SUCCESS_ACTION_PRINT
 *
 */
#define nlREQUIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)     __nlEXPECT_SUCCESS_ACTION_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aStatus, aLabel, anAction, aMessage)

/**
 *  @def nlNREQUIRE(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and branches to @p aLabel if the condition is
 *    true.
 *
 *  @note This is the logical inverse of #nlREQUIRE.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlREQUIRE
 *  @sa #nlNEXPECT
 *  @sa #nlNDESIRE
 *
 */
#define nlNREQUIRE(aCondition, aLabel)                                          __nlNEXPECT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @def nlNREQUIRE_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlREQUIRE_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlREQUIRE_PRINT
 *  @sa #nlNEXPECT_PRINT
 *  @sa #nlNDESIRE_PRINT
 *
 */
#define nlNREQUIRE_PRINT(aCondition, aLabel, aMessage)                          __nlNEXPECT_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlNREQUIRE_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both executes @p anAction and branches to
 *    @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlREQUIRE_ACTION.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlREQUIRE_ACTION
 *  @sa #nlNEXPECT_ACTION
 *  @sa #nlNDESIRE_ACTION
 *
 */
#define nlNREQUIRE_ACTION(aCondition, aLabel, anAction)                         __nlNEXPECT_ACTION(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlNREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlREQUIRE_ACTION_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #nlREQUIRE_ACTION_PRINT
 *  @sa #nlNEXPECT_ACTION_PRINT
 *  @sa #nlNDESIRE_ACTION_PRINT
 *
 */
#define nlNREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)         __nlNEXPECT_ACTION_PRINT(NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup precondition-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for precondition-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT                      (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG | NL_ASSERT_FLAG_TRAP)

/**
 *  @def NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for precondition-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS                              NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS)
#define NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS                              (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlPRECONDITION(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and returns from the enclosing function if the
 *    condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT
 *  @sa #nlDESIRE
 *  @sa #nlREQUIRE
 *
 */
#define nlPRECONDITION(aCondition)                                              __nlPRECONDITION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return)

/**
 *  @def nlPRECONDITION_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both executes @p anAction and returns from
 *    the enclosing function if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION
 *  @sa #nlDESIRE_ACTION
 *  @sa #nlREQUIRE_ACTION
 *
 */
#define nlPRECONDITION_ACTION(aCondition, anAction)                             __nlPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return, anAction)

/**
 *  @def nlPRECONDITION_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both prints @p aMessage and then returns
 *    from the enclosing function if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_PRINT
 *  @sa #nlDESIRE_PRINT
 *  @sa #nlREQUIRE_PRINT
 *
 */
#define nlPRECONDITION_PRINT(aCondition, aMessage)                              __nlPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return, aMessage)

/**
 *  @def nlPRECONDITION_SUCCESS(aStatus)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and returns from the
 *    enclosing function if the status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS
 *  @sa #nlDESIRE_SUCCESS
 *  @sa #nlREQUIRE_SUCCESS
 *
 */
#define nlPRECONDITION_SUCCESS(aStatus)                                         __nlPRECONDITION_SUCCESS(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return)

/**
 *  @def nlPRECONDITION_SUCCESS_ACTION(aStatus, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both executes @p
 *    anAction and returns from the enclosing function if the status
 *    is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION
 *  @sa #nlDESIRE_SUCCESS_ACTION
 *  @sa #nlREQUIRE_SUCCESS_ACTION
 *
 */
#define nlPRECONDITION_SUCCESS_ACTION(aStatus, anAction)                        __nlPRECONDITION_SUCCESS_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return, anAction)

/**
 *  @def nlPRECONDITION_SUCCESS_PRINT(aStatus, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both prints @p
 *    aMessage and then returns from the enclosing function if the
 *    status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_PRINT
 *  @sa #nlDESIRE_SUCCESS_PRINT
 *  @sa #nlREQUIRE_SUCCESS_PRINT
 *
 */
#define nlPRECONDITION_SUCCESS_PRINT(aStatus, aMessage)                         __nlPRECONDITION_SUCCESS_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return, aMessage)

/**
 *  @def nlNPRECONDITION(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and returns from the enclosing function if
 *    the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION
 *
 *  @sa #nlNEXPECT
 *  @sa #nlNDESIRE
 *  @sa #nlNREQUIRE
 *
 */
#define nlNPRECONDITION(aCondition)                                             __nlNPRECONDITION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return)

/**
 *  @def nlNPRECONDITION_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both executes @p anAction and returns from
 *    the enclosing function if the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION_ACTION.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION_ACTION
 *
 *  @sa #nlNEXPECT_ACTION
 *  @sa #nlNDESIRE_ACTION
 *  @sa #nlNREQUIRE_ACTION
 *
 */
#define nlNPRECONDITION_ACTION(aCondition, anAction)                            __nlNPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return, anAction)

/**
 *  @def nlNPRECONDITION_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both prints @p aMessage and then returns
 *    from the enclosing function if the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION_PRINT.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION_PRINT
 *
 *  @sa #nlNEXPECT_PRINT
 *  @sa #nlNDESIRE_PRINT
 *  @sa #nlNREQUIRE_PRINT
 *
 */
#define nlNPRECONDITION_PRINT(aCondition, aMessage)                             __nlNPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return, aMessage)

/**
 *  @def nlPRECONDITION_VALUE(aCondition, aValue)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and returns @p aValue from the enclosing
 *    function if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT
 *  @sa #nlDESIRE
 *  @sa #nlREQUIRE
 *
 */
#define nlPRECONDITION_VALUE(aCondition, aValue)                                __nlPRECONDITION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue)

/**
 *  @def nlPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both executes @p anAction and returns @p
 *    aValue from the enclosing function if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_ACTION
 *  @sa #nlDESIRE_ACTION
 *  @sa #nlREQUIRE_ACTION
 *
 */
#define nlPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)               __nlPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue, anAction)

/**
 *  @def nlPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both prints @p aMessage and then returns
 *    @p aValue from the enclosing function if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_PRINT
 *  @sa #nlDESIRE_PRINT
 *  @sa #nlREQUIRE_PRINT
 *
 */
#define nlPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)                __nlPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue, aMessage)

/**
 *  @def nlPRECONDITION_VALUE_SUCCESS(aStatus, aValue)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and returns @p aValue
 *    from the enclosing function if the status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS
 *  @sa #nlDESIRE_SUCCESS
 *  @sa #nlREQUIRE_SUCCESS
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS(aStatus, aValue)                           __nlPRECONDITION_SUCCESS(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return aValue)

/**
 *  @def nlPRECONDITION_VALUE_SUCCESS_ACTION(aStatus, aValue, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both executes @p
 *    anAction and returns @p aValue from the enclosing function if
 *    the status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_ACTION
 *  @sa #nlDESIRE_SUCCESS_ACTION
 *  @sa #nlREQUIRE_SUCCESS_ACTION
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS_ACTION(aStatus, aValue, anAction)          __nlPRECONDITION_SUCCESS_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return aValue, anAction)

/**
 *  @def nlPRECONDITION_VALUE_SUCCESS_PRINT(aStatus, aValue, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both prints @p
 *    aMessage and then returns @p aValue from the enclosing function
 *    if the status is unsuccessful.
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlEXPECT_SUCCESS_PRINT
 *  @sa #nlDESIRE_SUCCESS_PRINT
 *  @sa #nlREQUIRE_SUCCESS_PRINT
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS_PRINT(aStatus, aValue, aMessage)           __nlPRECONDITION_SUCCESS_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aStatus, return aValue, aMessage)

/**
 *  @def nlNPRECONDITION_VALUE(aCondition, aValue)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and returns @p aValue from the enclosing
 *    function if the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION_VALUE.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION_VALUE
 *
 *  @sa #nlNEXPECT
 *  @sa #nlNDESIRE
 *  @sa #nlNREQUIRE
 *
 */
#define nlNPRECONDITION_VALUE(aCondition, aValue)                               __nlNPRECONDITION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue)

/**
 *  @def nlNPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both executes @p anAction and returns @p
 *    aValue from the enclosing function if the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION_VALUE_ACTION.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION_VALUE_ACTION
 *
 *  @sa #nlNEXPECT_ACTION
 *  @sa #nlNDESIRE_ACTION
 *  @sa #nlNREQUIRE_ACTION
 *
 */
#define nlNPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)              __nlNPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue, anAction)

/**
 *  @def nlNPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both prints @p aMessage and then returns
 *    @p aValue from the enclosing function if the condition is true.
 *
 *  @note This is the logical inverse of #nlPRECONDITION_VALUE_PRINT.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aValue      The value to be returned when the assertion
 *                          fails.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #nlPRECONDITION_VALUE_PRINT
 *
 *  @sa #nlNEXPECT_PRINT
 *  @sa #nlNDESIRE_PRINT
 *  @sa #nlNREQUIRE_PRINT
 *
 */
#define nlNPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)               __nlNPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS, aCondition, return aValue, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup assert-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for assert-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT                            (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for assert-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS                                     NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS)
#define NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS                                    (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlASSERT(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    invokes #NL_ASSERT_ABORT() if the condition is false.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
 *
 *  @sa #nlCHECK
 *  @sa #nlVERIFY
 *  @sa #nlABORT
 *
 */
#define nlASSERT(aCondition)                                                    __nlABORT(NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlASSERT_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    both executes @p anAction and then invokes #NL_ASSERT_ABORT() if
 *    the condition is false.
 *
 *  @note If #NL_ASSERT_PRODUCTION is true, the check is made inactive. But
 *  side-effects, if any, in the asserted expression will still be produced.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_ACTION
 *  @sa #nlVERIFY_ACTION
 *  @sa #nlABORT_ACTION
 *
 */
#define nlASSERT_ACTION(aCondition, anAction)                                   __nlABORT_ACTION(NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @}
 *
 */

/**
 *  @ingroup abort-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for abort-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    disabled.
 *
 *  This may be used to override #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_ABORT_NONPRODUCTION_FLAGS NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT                             (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for abort-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_ABORT_NONPRODUCTION_FLAGS                                     NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_ABORT_NONPRODUCTION_FLAGS)
#define NL_ASSERT_ABORT_NONPRODUCTION_FLAGS                                     (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlABORT(aCondition)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    invokes #NL_ASSERT_ABORT() if the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_ABORT_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK
 *  @sa #nlVERIFY
 *  @sa #nlASSERT
 *
 */
#define nlABORT(aCondition)                                                     __nlABORT(NL_ASSERT_ABORT_NONPRODUCTION_FLAGS, aCondition)

/**
 *  @def nlABORT_ACTION(aCondition, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true and takes action, based on configuration, and
 *    both executes @p anAction and then invokes #NL_ASSERT_ABORT() if
 *    the condition is false.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
 *  @sa #NL_ASSERT_ABORT_PRODUCTION_FLAGS
 *
 *  @sa #nlCHECK_ACTION
 *  @sa #nlVERIFY_ACTION
 *  @sa #nlASSERT_ACTION
 *
 */
#define nlABORT_ACTION(aCondition, anAction)                                    __nlABORT_ACTION(NL_ASSERT_ABORT_NONPRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @}
 *
 */

#endif /* NLCORE_NLASSERT_NONPRODUCTION_H */
