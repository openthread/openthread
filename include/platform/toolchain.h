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
 * @defgroup toolchain Toolchain
 * @ingroup platform
 *
 * @brief
 *   This module defines a toolchain abstraction layer through macros.
 *
 * @{
 *
 */

#ifndef TOOLCHAIN_H_
#define TOOLCHAIN_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @def OT_TOOL_PACKED_BEGIN
 *
 * Compiler-specific indication that a class or struct must be byte packed.
 *
 * Usage:
 *
 *    @code
 *
 *    typedef
 *    OT_TOOL_PACKED_BEGIN
 *    struct {
 *       char field1;
 *       union {
 *          char field2;
 *          long field2;
 *       } OT_TOOL_PACKED_FIELD;
 *    } OT_TOOL_PACKED_END packed_struct_t;
 *
 *    @endcode
 */

/**
 * @def OT_TOOL_PACKED_FIELD
 *
 * Indicate to the compiler a nested struct or union to be packed
 * within byte packed class or struct.
 *
 */

/**
 * @def OT_TOOL_PACKED_END
 *
 * Compiler-specific indication at the end of a byte packed class or struct.
 *
 */

/**
 * @def OT_TOOL_DEPRECATED
 *
 * Indicate to the compiler to warn upon use that a field or function
 * has been deprecated.
 *
 * @param[in]  symbol      The name of the field or function to deprecate.
 *
 */

// =========== TOOLCHAIN SELECTION : START ===========

#if defined(__GNUC__) || defined(__CC_ARM)

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
// http://www.keil.com/support/man/docs/armcc/armcc_chr1359124973480.htm

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD                __attribute__((packed))
#define OT_TOOL_PACKED_END                  __attribute__((packed))
#define OT_TOOL_DEPRECATED(symbol)          __attribute__((deprecated))

#elif defined(__ICCARM__) || defined(__ICC8051__)

// http://supp.iar.com/FilesPublic/UPDINFO/004916/arm/doc/EWARM_DevelopmentGuide.ENU.pdf

#include "intrinsics.h"

#define OT_TOOL_PACKED_BEGIN                __packed
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_DEPRECATED(symbol)

#elif defined(_MSC_VER)

#define OT_TOOL_PACKED_BEGIN                __pragma(pack(push,1))
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END                  __pragma(pack(pop))
#define OT_TOOL_DEPRECATED(symbol)          __pragma(deprecated(symbol))

#elif defined(__SDCC)

// 8051 is always byte aligned and packed

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_DEPRECATED(symbol)

#else

#error "Error: No valid Toolchain specified"

// Symbols for Doxygen

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_DEPRECATED(symbol)

#endif

// =========== TOOLCHAIN SELECTION : END ===========

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // TOOLCHAIN_H_
