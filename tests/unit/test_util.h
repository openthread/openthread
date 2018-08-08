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

#ifndef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

// Enable main functions
#define ENABLE_TEST_MAIN

#define SuccessOrQuit(ERR, MSG)                                                    \
    do                                                                             \
    {                                                                              \
        if ((ERR) != OT_ERROR_NONE)                                                \
        {                                                                          \
            fprintf(stderr, "\nFAILED %s:%d - %s\n", __FUNCTION__, __LINE__, MSG); \
            exit(-1);                                                              \
        }                                                                          \
    } while (false)

#define VerifyOrQuit(TST, MSG)                                                     \
    do                                                                             \
    {                                                                              \
        if (!(TST))                                                                \
        {                                                                          \
            fprintf(stderr, "\nFAILED %s:%d - %s\n", __FUNCTION__, __LINE__, MSG); \
            exit(-1);                                                              \
        }                                                                          \
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

#else

typedef void (*utAssertTrue)(bool condition, const wchar_t *message);
extern utAssertTrue s_AssertTrue;

typedef void (*utLogMessage)(const char *format, ...);
extern utLogMessage s_LogMessage;

#define SuccessOrQuit(ERR, MSG) s_AssertTrue((ERR) == OT_ERROR_NONE, L##MSG)

#define VerifyOrQuit(ERR, MSG) s_AssertTrue(ERR, L##MSG)

#define CompileTimeAssert(COND, MSG) static_assert(COND, MSG)

#define Log(aFormat, ...) s_LogMessage(aFormat, ##__VA_ARGS__)

#endif

#endif
