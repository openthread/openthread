/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 * @brief
 *   This file defines a toolchain abstraction layer through macros.
 *
 */

#ifndef _TOOLCHAIN_H_
#define _TOOLCHAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

// =========== TOOLCHAIN SELECTION : START ===========

#if defined(__GNUC__) || defined(__CC_ARM)

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
// http://www.keil.com/support/man/docs/armcc/armcc_chr1359124973480.htm

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_END                  __attribute__((packed))
#define OT_TOOL_DEPRECATED(symbol)          __attribute__((deprecated))

#elif defined(__ICCARM__) || defined(__ICC8051__)

// http://supp.iar.com/FilesPublic/UPDINFO/004916/arm/doc/EWARM_DevelopmentGuide.ENU.pdf

#include "intrinsics.h"

#define OT_TOOL_PACKED_BEGIN                __packed
#define OT_TOOL_PACKED_END                  
#define OT_TOOL_DEPRECATED(symbol)

#elif defined(_MSC_VER)

#define OT_TOOL_PACKED_BEGIN                __pragma(pack(push,1))
#define OT_TOOL_PACKED_END                  __pragma(pack(pop))
#define OT_TOOL_DEPRECATED(symbol)          __pragma(deprecated(symbol))

#elif defined(__SDCC)

// 8051 is always byte aligned and packed

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_END
#define OT_TOOL_DEPRECATED(symbol)

#else

#error "Error: No valid Toolchain specified"

#endif

// =========== TOOLCHAIN SELECTION : END ===========

#ifdef __cplusplus
}
#endif

#endif // _TOOL_H_
