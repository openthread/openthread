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

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This macro verifies a given error status to be successful (compared against value zero (0)), otherwise, it emits a
 * given error messages and exits the program.
 *
 * @param[in]  aStatus     A scalar error status to be evaluated against zero (0).
 * @param[in]  aMessage    A message (text string) to print on failure.
 *
 */
#define SuccessOrQuit(aStatus, aMessage)                                                \
    do                                                                                  \
    {                                                                                   \
        if ((aStatus) != 0)                                                             \
        {                                                                               \
            fprintf(stderr, "\nFAILED %s:%d - %s\n", __FUNCTION__, __LINE__, aMessage); \
            exit(-1);                                                                   \
        }                                                                               \
    } while (false)

/**
 * This macro verifies that a given boolean condition is true, otherwise, it emits a given error message and exits the
 * program.
 *
 * @param[in]  aCondition  A Boolean expression to be evaluated.
 * @param[in]  aMessage    A message (text string) to print on failure.
 *
 */
#define VerifyOrQuit(aCondition, aMessage)                                              \
    do                                                                                  \
    {                                                                                   \
        if (!(aCondition))                                                              \
        {                                                                               \
            fprintf(stderr, "\nFAILED %s:%d - %s\n", __FUNCTION__, __LINE__, aMessage); \
            exit(-1);                                                                   \
        }                                                                               \
    } while (false)

//#define CompileTimeAssert(COND, MSG) typedef char __C_ASSERT__[(COND)?1:-1]

// I would use the above definition for CompileTimeAssert, but I am getting the following errors
// when I run 'make -f examples/Makefile-posix distcheck':
//
//      error: typedef "__C_ASSERT__" locally defined but not used [-Werror=unused-local-typedefs]
//
#define CompileTimeAssert(COND, MSG)

#define Log(aFormat, ...) printf(aFormat "\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
