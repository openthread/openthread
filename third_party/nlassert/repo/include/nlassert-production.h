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
 *      exception handling when #NL_ASSERT_PRODUCTION is true.
 *
 */

#ifndef NLASSERT_PRODUCTION_H
#define NLASSERT_PRODUCTION_H

#include "nlassert-internal.h"

// nlCHECK

/**
 *  @implements nlCHECK(aCondition)
 *
 */
#define nlCHECK(aCondition)                                                     __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlCHECK_ACTION(aCondition, anAction)
 *
 */
#define nlCHECK_ACTION(aCondition, anAction)                                    __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlCHECK_PRINT(aCondition, aMessage)
 *
 */
#define nlCHECK_PRINT(aCondition, aMessage)                                     __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlCHECK_SUCCESS(aStatus)
 *
 */
#define nlCHECK_SUCCESS(aStatus)                                                __nlASSERT_UNUSED(aStatus)

/**
 *  @implements nlCHECK_SUCCESS_ACTION(aStatus, anAction)
 *
 */
#define nlCHECK_SUCCESS_ACTION(aStatus, anAction)                               __nlASSERT_UNUSED(aStatus)

/**
 *  @implements nlCHECK_SUCCESS_PRINT(aStatus, aMessage)
 *
 */
#define nlCHECK_SUCCESS_PRINT(aStatus, aMessage)                                __nlASSERT_UNUSED(aStatus)

/**
 *  @implements nlNCHECK(aCondition)
 *
 */
#define nlNCHECK(aCondition)                                                    __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlNCHECK_ACTION(aCondition, anAction)
 *
 */
#define nlNCHECK_ACTION(aCondition, anAction)                                   __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlNCHECK_PRINT(aCondition, aMessage)
 *
 */
#define nlNCHECK_PRINT(aCondition, aMessage)                                    __nlASSERT_UNUSED(aCondition)

// nlVERIFY

/**
 *  @def NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for verify-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    enabled.
 *
 *  This may be used to override #NL_ASSERT_VERIFY_PRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_VERIFY_PRODUCTION_FLAGS NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup verify-style
 *
 */
#define NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT                               (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    enabled that govern the behavior for verify-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup verify-style
 *
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_VERIFY_PRODUCTION_FLAGS                                       NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_VERIFY_PRODUCTION_FLAGS)
#define NL_ASSERT_VERIFY_PRODUCTION_FLAGS                                       (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @implements nlVERIFY(aCondition)
 *
 */
#define nlVERIFY(aCondition)                                                    __nlVERIFY(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition)

/**
 *  @implements nlVERIFY_ACTION(aCondition, anAction)
 *
 */
#define nlVERIFY_ACTION(aCondition, anAction)                                   __nlVERIFY_ACTION(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @implements nlVERIFY_PRINT(aCondition, aMessage)
 *
 */
#define nlVERIFY_PRINT(aCondition, aMessage)                                    __nlVERIFY_PRINT(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition, aMessage)

/**
 *  @implements nlVERIFY_SUCCESS(aStatus)
 *
 */
#define nlVERIFY_SUCCESS(aStatus)                                               __nlVERIFY_SUCCESS(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aStatus)

/**
 *  @implements nlVERIFY_SUCCESS_ACTION(aStatus, anAction)
 *
 */
#define nlVERIFY_SUCCESS_ACTION(aStatus, anAction)                              __nlVERIFY_SUCCESS_ACTION(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aStatus, anAction)

/**
 *  @implements nlVERIFY_SUCCESS_PRINT(aStatus, aMessage)
 *
 */
#define nlVERIFY_SUCCESS_PRINT(aStatus, aMessage)                               __nlVERIFY_SUCCESS_PRINT(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aStatus, aMessage)

/**
 *  @implements nlNVERIFY(aCondition)
 *
 */
#define nlNVERIFY(aCondition)                                                   __nlNVERIFY(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition)

/**
 *  @implements nlNVERIFY_ACTION(aCondition, anAction)
 *
 */
#define nlNVERIFY_ACTION(aCondition, anAction)                                  __nlNVERIFY_ACTION(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition, anAction)

/**
 *  @implements nlNVERIFY_PRINT(aCondition, aMessage)
 *
 */
#define nlNVERIFY_PRINT(aCondition, aMessage)                                   __nlNVERIFY_PRINT(NL_ASSERT_VERIFY_PRODUCTION_FLAGS, aCondition, aMessage)

// nlDESIRE

/**
 *  @def NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for desire-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    enabled.
 *
 *  This may be used to override #NL_ASSERT_DESIRE_PRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_DESIRE_PRODUCTION_FLAGS NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup desire-style
 *
 */
#define NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT                               (NL_ASSERT_FLAG_NONE)

/**
 *  @def NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    enabled that govern the behavior for desire-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup desire-style
 *
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_DESIRE_PRODUCTION_FLAGS                                       NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_DESIRE_PRODUCTION_FLAGS)
#define NL_ASSERT_DESIRE_PRODUCTION_FLAGS                                       (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @implements nlDESIRE(aCondition, aLabel)
 *
 */
#define nlDESIRE(aCondition, aLabel)                                            __nlEXPECT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @implements nlDESIRE_PRINT(aCondition, aLabel, aMessage)
 *
 */
#define nlDESIRE_PRINT(aCondition, aLabel, aMessage)                            __nlEXPECT_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @implements nlDESIRE_ACTION(aCondition, aLabel, anAction)
 *
 */
#define nlDESIRE_ACTION(aCondition, aLabel, anAction)                           __nlEXPECT_ACTION(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @implements nlDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 */
#define nlDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)           __nlEXPECT_ACTION_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @implements nlDESIRE_SUCCESS(aStatus, aLabel)
 *
 */
#define nlDESIRE_SUCCESS(aStatus, aLabel)                                       __nlEXPECT_SUCCESS(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aStatus, aLabel)

/**
 *  @implements nlDESIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)
 *
 */
#define nlDESIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)                       __nlEXPECT_SUCCESS_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aStatus, aLabel, aMessage)

/**
 *  @implements nlDESIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)
 *
 */
#define nlDESIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)                      __nlEXPECT_SUCCESS_ACTION(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aStatus, aLabel, anAction)

/**
 *  @implements nlDESIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)
 *
 */
#define nlDESIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)      __nlEXPECT_SUCCESS_ACTION_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aStatus, aLabel, anAction, aMessage)

/**
 *  @implements nlNDESIRE(aCondition, aLabel)
 *
 */
#define nlNDESIRE(aCondition, aLabel)                                           __nlNEXPECT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @implements nlNDESIRE_PRINT(aCondition, aLabel, aMessage)
 *
 */
#define nlNDESIRE_PRINT(aCondition, aLabel, aMessage)                           __nlNEXPECT_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @implements nlNDESIRE_ACTION(aCondition, aLabel, anAction)
 *
 */
#define nlNDESIRE_ACTION(aCondition, aLabel, anAction)                          __nlNEXPECT_ACTION(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @implements nlNDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 */
#define nlNDESIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)          __nlNEXPECT_ACTION_PRINT(NL_ASSERT_DESIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

// nlREQUIRE

/**
 *  @def NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for require-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    enabled.
 *
 *  This may be used to override #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_REQUIRE_PRODUCTION_FLAGS NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup require-style
 *
 */
#define NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT                              (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    enabled that govern the behavior for require-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup require-style
 *
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_REQUIRE_PRODUCTION_FLAGS                                      NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS)
#define NL_ASSERT_REQUIRE_PRODUCTION_FLAGS                                      (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @implements nlREQUIRE(aCondition, aLabel)
 *
 */
#define nlREQUIRE(aCondition, aLabel)                                           __nlEXPECT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @implements nlREQUIRE_PRINT(aCondition, aLabel, aMessage)
 */
#define nlREQUIRE_PRINT(aCondition, aLabel, aMessage)                           __nlEXPECT_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @implements nlREQUIRE_ACTION(aCondition, aLabel, anAction)
 *
 */
#define nlREQUIRE_ACTION(aCondition, aLabel, anAction)                          __nlEXPECT_ACTION(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @implements nlREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 */
#define nlREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)          __nlEXPECT_ACTION_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @implements nlREQUIRE_SUCCESS(aStatus, aLabel)
 *
 */
#define nlREQUIRE_SUCCESS(aStatus, aLabel)                                      __nlEXPECT_SUCCESS(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aStatus, aLabel)

/**
 *  @implements nlREQUIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)
 *
 */
#define nlREQUIRE_SUCCESS_PRINT(aStatus, aLabel, aMessage)                      __nlEXPECT_SUCCESS_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aStatus, aLabel, aMessage)

/**
 *  @implements nlREQUIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)
 *
 */
#define nlREQUIRE_SUCCESS_ACTION(aStatus, aLabel, anAction)                     __nlEXPECT_SUCCESS_ACTION(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aStatus, aLabel, anAction)

/**
 *  @implements nlREQUIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)
 *
 */
#define nlREQUIRE_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)     __nlEXPECT_SUCCESS_ACTION_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aStatus, aLabel, anAction, aMessage)

/**
 *  @implements nlNREQUIRE(aCondition, aLabel)
 *
 */
#define nlNREQUIRE(aCondition, aLabel)                                          __nlNEXPECT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel)

/**
 *  @implements nlNREQUIRE_PRINT(aCondition, aLabel, aMessage)
 *
 */
#define nlNREQUIRE_PRINT(aCondition, aLabel, aMessage)                          __nlNEXPECT_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @implements nlNREQUIRE_ACTION(aCondition, aLabel, anAction)
 *
 */
#define nlNREQUIRE_ACTION(aCondition, aLabel, anAction)                         __nlNEXPECT_ACTION(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction)

/**
 *  @implements nlNREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 */
#define nlNREQUIRE_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)         __nlNEXPECT_ACTION_PRINT(NL_ASSERT_REQUIRE_PRODUCTION_FLAGS, aCondition, aLabel, anAction, aMessage)

// nlPRECONDITION

/**
 *  @def NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for precondition-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    enabled.
 *
 *  This may be used to override #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup precondition-style
 *
 */
#define NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT                         (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    disabled that govern the behavior for precondition-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup precondition-style
 *
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS                                 NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS)
#define NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS                                 (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @implements nlPRECONDITION(aCondition)
 *
 */
#define nlPRECONDITION(aCondition)                                              __nlPRECONDITION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return)

/**
 *  @implements nlPRECONDITION_ACTION(aCondition, anAction)
 *
 */
#define nlPRECONDITION_ACTION(aCondition, anAction)                             __nlPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return, anAction)

/**
 *  @implements nlPRECONDITION_PRINT(aCondition, aMessage)
 *
 */
#define nlPRECONDITION_PRINT(aCondition, aMessage)                              __nlPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return, aMessage)

/**
 *  @implements nlPRECONDITION_SUCCESS(aStatus)
 *
 */
#define nlPRECONDITION_SUCCESS(aStatus)                                         __nlPRECONDITION_SUCCESS(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return)

/**
 *  @implements nlPRECONDITION_SUCCESS_ACTION(aStatus, anAction)
 *
 */
#define nlPRECONDITION_SUCCESS_ACTION(aStatus, anAction)                        __nlPRECONDITION_SUCCESS_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return, anAction)

/**
 *  @implements nlPRECONDITION_SUCCESS_PRINT(aStatus, aMessage)
 *
 */
#define nlPRECONDITION_SUCCESS_PRINT(aStatus, aMessage)                         __nlPRECONDITION_SUCCESS_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return, aMessage)

/**
 *  @implements nlNPRECONDITION(aCondition)
 *
 */
#define nlNPRECONDITION(aCondition)                                             __nlNPRECONDITION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return)

/**
 *  @implements nlNPRECONDITION_ACTION(aCondition, anAction)
 *
 */
#define nlNPRECONDITION_ACTION(aCondition, anAction)                            __nlNPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return, anAction)

/**
 *  @implements nlNPRECONDITION_PRINT(aCondition, aMessage)
 *
 */
#define nlNPRECONDITION_PRINT(aCondition, aMessage)                             __nlNPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return, aMessage)

/**
 *  @implements nlPRECONDITION_VALUE(aCondition, aValue)
 *
 */
#define nlPRECONDITION_VALUE(aCondition, aValue)                                __nlPRECONDITION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue)

/**
 *  @implements nlPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)
 *
 */
#define nlPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)               __nlPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue, anAction)

/**
 *  @implements nlPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)
 *
 */
#define nlPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)                __nlPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue, aMessage)

/**
 *  @implements nlPRECONDITION_VALUE_SUCCESS(aStatus, aValue)
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS(aStatus, aValue)                           __nlPRECONDITION_SUCCESS(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return aValue)

/**
 *  @implements nlPRECONDITION_VALUE_SUCCESS_ACTION(aStatus, aValue, anAction)
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS_ACTION(aStatus, aValue, anAction)          __nlPRECONDITION_SUCCESS_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return aValue, anAction)

/**
 *  @implements nlPRECONDITION_VALUE_SUCCESS_PRINT(aStatus, aValue, aMessage)
 *
 */
#define nlPRECONDITION_VALUE_SUCCESS_PRINT(aStatus, aValue, aMessage)           __nlPRECONDITION_SUCCESS_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aStatus, return aValue, aMessage)

/**
 *  @implements nlNPRECONDITION_VALUE(aCondition, aValue)
 *
 */
#define nlNPRECONDITION_VALUE(aCondition, aValue)                               __nlNPRECONDITION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue)

/**
 *  @implements nlNPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)
 *
 */
#define nlNPRECONDITION_VALUE_ACTION(aCondition, aValue, anAction)              __nlNPRECONDITION_ACTION(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue, anAction)

/**
 *  @implements nlNPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)
 *
 */
#define nlNPRECONDITION_VALUE_PRINT(aCondition, aValue, aMessage)               __nlNPRECONDITION_PRINT(NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS, aCondition, return aValue, aMessage)

// nlASSERT

/**
 *  @implements nlASSERT(aCondition)
 *
 */
#define nlASSERT(aCondition)                                                    __nlASSERT_UNUSED(aCondition)

/**
 *  @implements nlASSERT_ACTION(aCondition, anAction)
 *
 */
#define nlASSERT_ACTION(aCondition, anAction)                                   __nlASSERT_UNUSED(aCondition)

// nlABORT

/**
 *  @def NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for abort-style
 *    exception family assertions when #NL_ASSERT_PRODUCTION is
 *    enabled.
 *
 *  This may be used to override #NL_ASSERT_ABORT_PRODUCTION_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_ABORT_PRODUCTION_FLAGS NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_ABORT_PRODUCTION_FLAGS
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup abort-style
 *
 */
#define NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT                                (NL_ASSERT_FLAG_BACKTRACE | NL_ASSERT_FLAG_LOG)

/**
 *  @def NL_ASSERT_ABORT_PRODUCTION_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags when #NL_ASSERT_PRODUCTION is
 *    enabled that govern the behavior for abort-style exception
 *    family assertions when the assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_PRODUCTION
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @ingroup abort-style
 *
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_ABORT_PRODUCTION_FLAGS                                        NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_ABORT_PRODUCTION_FLAGS)
#define NL_ASSERT_ABORT_PRODUCTION_FLAGS                                        (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @implements nlABORT(aCondition)
 *
 */
#define nlABORT(aCondition)                                                     __nlABORT(NL_ASSERT_ABORT_PRODUCTION_FLAGS, aCondition)

/**
 *  @implements nlABORT_ACTION(aCondition, anAction)
 *
 */
#define nlABORT_ACTION(aCondition, anAction)                                    __nlABORT_ACTION(NL_ASSERT_ABORT_PRODUCTION_FLAGS, aCondition, anAction)

#endif /* NLASSERT_PRODUCTION_H */
