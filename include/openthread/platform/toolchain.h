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
 * @addtogroup plat-toolchain
 *
 * @brief
 *   This module defines a toolchain abstraction layer through macros.
 *
 * Usage:
 *
 *    @code
 *
 *    typedef
 *    OT_TOOL_PACKED_BEGIN
 *    struct
 *    {
 *        char mField1;
 *        union
 *        {
 *            char mField2;
 *            long mField3;
 *        } OT_TOOL_PACKED_FIELD;
 *    } OT_TOOL_PACKED_END packed_struct_t;
 *
 *    @endcode
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
 * @def OT_TOOL_ALIGN
 *
 * Compiler-specific alignment modifier.
 *
 */

// =========== TOOLCHAIN SELECTION : START ===========

#if defined(__GNUC__) || defined(__clang__) || defined(__CC_ARM) || defined(__TI_ARM__)

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
// http://www.keil.com/support/man/docs/armcc/armcc_chr1359124973480.htm

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD                __attribute__((packed))
#define OT_TOOL_PACKED_END                  __attribute__((packed))
#define OT_TOOL_WEAK                        __attribute__((weak))

#define OT_TOOL_ALIGN(X)

#elif defined(__ICCARM__) || defined(__ICC8051__)

// http://supp.iar.com/FilesPublic/UPDINFO/004916/arm/doc/EWARM_DevelopmentGuide.ENU.pdf

#include "intrinsics.h"

#define OT_TOOL_PACKED_BEGIN                __packed
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_WEAK                        __weak


#define OT_TOOL_ALIGN(X)

#elif defined(_MSC_VER)

#define OT_TOOL_PACKED_BEGIN                __pragma(pack(push,1))
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END                  __pragma(pack(pop))
#define OT_TOOL_WEAK


#define OT_TOOL_ALIGN(X)                    __declspec(align(4))

#elif defined(__SDCC)

// Structures are packed by default in sdcc, as it primarily targets 8-bit MCUs.

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_WEAK

#define OT_TOOL_ALIGN(X)

#else

#error "Error: No valid Toolchain specified"

// Symbols for Doxygen

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_WEAK

#define OT_TOOL_ALIGN(X)

#endif

// =========== TOOLCHAIN SELECTION : END ===========

/**
 * @def OTAPI
 *
 * Compiler-specific modifier for public API declarations.
 *
 */

/**
 * @def OTCALL
 *
 * Compiler-specific modifier to export functions in a DLL.
 *
 */

#ifdef _MSC_VER

#ifdef _WIN64
#define OT_CDECL
#else
#define OT_CDECL __cdecl
#endif

#else

#define OT_CALL
#define OT_CDECL

#endif

#ifdef OTDLL
#ifndef OTAPI
#define OTAPI __declspec(dllimport)
#endif
#define OTCALL WINAPI
#else
#define OTAPI
#define OTCALL
#endif

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // TOOLCHAIN_H_
