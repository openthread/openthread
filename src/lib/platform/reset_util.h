/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#ifndef OT_LIB_PLATFORM_RESET_UTIL_H_
#define OT_LIB_PLATFORM_RESET_UTIL_H_

#if defined(OPENTHREAD_ENABLE_COVERAGE) && OPENTHREAD_ENABLE_COVERAGE && defined(__GNUC__)
#if __GNUC__ >= 11 || (defined(__clang__) && (defined(__APPLE__) && (__clang_major__ >= 13)) || \
                       (!defined(__APPLE__) && (__clang_major__ >= 12)))
void __gcov_dump();
void __gcov_reset();

static void flush_gcov(void)
{
    __gcov_dump();
    __gcov_reset();
}
#else
void __gcov_flush(void);
#define flush_gcov __gcov_flush
#endif /* __GNUC__ >= 11 || (defined(__clang__) && (defined(__APPLE__) && (__clang_major__ >= 13)) || \
                             (!defined(__APPLE__) && (__clang_major__ >= 12))) */
#else
#define flush_gcov()
#endif // defined(OPENTHREAD_ENABLE_COVERAGE) && OPENTHREAD_ENABLE_COVERAGE && defined(__GNUC__)

#if defined(__linux__) || defined(__APPLE__)
#include <setjmp.h>
#include <unistd.h>
jmp_buf gResetJump;

#define OT_SETUP_RESET_JUMP(kArgv) \
    if (setjmp(gResetJump))        \
    {                              \
        alarm(0);                  \
        flush_gcov();              \
        execvp(kArgv[0], kArgv);   \
    }

#else
#define OT_SETUP_RESET_JUMP(ARGV)
#endif // defined(__linux__) || defined(__APPLE__)

#endif // OT_LIB_PLATFORM_RESET_UTIL_H_
