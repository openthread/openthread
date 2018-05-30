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
#include "utils/wrap_string.h"

#if defined(OPENTHREAD_TARGET_DARWIN) || defined(OPENTHREAD_TARGET_LINUX)

#include <assert.h>

#elif defined(_KERNEL_MODE)

#include <wdm.h>

#define assert(exp) ((!(exp)) ? (RtlAssert(#exp, __FILE__, __LINE__, NULL), FALSE) : TRUE)

#elif defined(_WIN32)

#include <assert.h>

#elif OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT

#include "openthread/platform/misc.h"

/**
 * Allow the build system to provide a custom file name.
 *
 */
#ifndef FILE_NAME
#define FILE_NAME __FILE__
#endif

#define assert(cond)                               \
    do                                             \
    {                                              \
        if (!(cond))                               \
        {                                          \
            otPlatAssertFail(FILE_NAME, __LINE__); \
            while (1)                              \
            {                                      \
            }                                      \
        }                                          \
    } while (0)

#else

#define assert(cond)  \
    do                \
    {                 \
        if (!(cond))  \
        {             \
            while (1) \
            {         \
            }         \
        }             \
    } while (0)

#endif

#endif // DEBUG_HPP_
