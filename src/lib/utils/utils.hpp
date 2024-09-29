/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes macros for validating runtime conditions.
 *
 *   The macros in this file should be exclusively used by code under 'lib'. It's RECOMMENDED that only
 *   include this file in sources under 'lib' and not include this file in headers under 'lib'. Because
 *   headers under 'lib' may be included externally and the common macros may cause redefinition.
 */

#ifndef LIB_UTILS_CODE_UTILS_HPP_
#define LIB_UTILS_CODE_UTILS_HPP_

#include <stdbool.h>
#include <stddef.h>

/**
 * Checks for the specified status, which is expected to commonly be successful, and branches to the local
 * label 'exit' if the status is unsuccessful.
 *
 * @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 */
#define EXPECT_NO_ERROR(aStatus) \
    do                           \
    {                            \
        if ((aStatus) != 0)      \
        {                        \
            goto exit;           \
        }                        \
    } while (false)

/**
 * Does nothing. This is passed to EXPECT when there is no action to do.
 */
#define NO_ACTION

/**
 * Checks for the specified condition, which is expected to commonly be true, and both executes @a ... and
 * branches to the local label 'exit' if the condition is false.
 *
 * @param[in]  aCondition  A Boolean expression to be evaluated.
 * @param[in]  aAction     An optional expression or block to execute when the assertion fails.
 */
#define EXPECT(aCondition, aAction) \
    do                              \
    {                               \
        if (!(aCondition))          \
        {                           \
            aAction;                \
            goto exit;              \
        }                           \
    } while (false)

/**
 * Unconditionally executes @a ... and branches to the local label 'exit'.
 *
 * @note The use of this interface implies neither success nor failure for the overall exit status of the enclosing
 *       function body.
 *
 * @param[in]  ...         An optional expression or block to execute when the assertion fails.
 */
#define EXIT_NOW(...) \
    do                \
    {                 \
        __VA_ARGS__;  \
        goto exit;    \
    } while (false)

/**
 * Executes the `statement` and ignores the return value.
 *
 * This is primarily used to indicate the intention of developer that the return value of a function/method can be
 * safely ignored.
 *
 * @param[in]  aStatement  The function/method to execute.
 */
#define IGNORE_RETURN(aStatement) \
    do                            \
    {                             \
        if (aStatement)           \
        {                         \
        }                         \
    } while (false)

/**
 * Overload the new operator to new an object at a specific address.
 *
 * @param[in]  p  The pointer of the address.
 */
inline void *operator new(size_t, void *p) throw() { return p; }

#endif // LIB_UTILS_CODE_UTILS_HPP_
