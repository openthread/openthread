/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes functions for debugging.
 */

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include "openthread-core-config.h"

#include <ctype.h>
#include <stdio.h>

#if OPENTHREAD_CONFIG_ASSERT_ENABLE

#if OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT

#include <openthread/platform/misc.h>

/**
 * Allow the build system to provide a custom file name.
 */
#ifndef FILE_NAME
#define FILE_NAME __FILE__
#endif

#define OT_ASSERT(cond)                            \
    do                                             \
    {                                              \
        if (cond)                                  \
        {                                          \
        }                                          \
        else                                       \
        {                                          \
            otPlatAssertFail(FILE_NAME, __LINE__); \
            while (1)                              \
            {                                      \
            }                                      \
        }                                          \
    } while (0)

#elif defined(__APPLE__) || defined(__linux__)

#include <assert.h>

#define OT_ASSERT(cond) assert(cond)

#else // OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT

#define OT_ASSERT(cond) \
    do                  \
    {                   \
        if (cond)       \
        {               \
        }               \
        else            \
        {               \
            while (1)   \
            {           \
            }           \
        }               \
    } while (0)

#endif // OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT

#else // OPENTHREAD_CONFIG_ASSERT_ENABLE

#define OT_ASSERT(cond)

#endif // OPENTHREAD_CONFIG_ASSERT_ENABLE

/**
 * Checks a given status (which is expected to be successful) against zero (0) which indicates success,
 * and `OT_ASSERT()` if it is not.
 *
 * @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 */
#define SuccessOrAssert(aStatus) \
    do                           \
    {                            \
        if ((aStatus) != 0)      \
        {                        \
            OT_ASSERT(false);    \
        }                        \
    } while (false)

/**
 * @def AssertPointerIsNotNull
 *
 * Asserts that a given pointer (API input parameter) is not `nullptr`. This macro checks the pointer only
 * when `OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL` is enabled. Otherwise it is an empty macro.
 *
 * @param[in]  aPointer   The pointer variable (API input parameter) to check.
 */
#if OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL
#define AssertPointerIsNotNull(aPointer) OT_ASSERT((aPointer) != nullptr)
#else
#define AssertPointerIsNotNull(aPointer)
#endif

#endif // DEBUG_HPP_
