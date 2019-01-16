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

#ifndef OPENTHREAD_PLATFORM_TOOLCHAIN_H_
#define OPENTHREAD_PLATFORM_TOOLCHAIN_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#pragma warning(disable : 4214) // nonstandard extension used: bit field types other than int
#ifdef _KERNEL_MODE
#include <ntdef.h>
#else
#include <windows.h>
#endif
#endif /* _WIN32 */

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

/**
 * @def OT_TOOL_WEAK
 *
 * Compiler-specific weak symbol modifier.
 *
 */

/**
 * @def OT_CALL
 *
 * Compiler-specific function modifier, ie: Win DLL support
 *
 */

/**
 * @def OT_CDECL
 *
 * Compiler-specific function modifier, ie: Win DLL support
 *
 */

// =========== TOOLCHAIN SELECTION : START ===========

#if defined(__GNUC__) || defined(__clang__) || defined(__CC_ARM) || defined(__TI_ARM__)

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
// http://www.keil.com/support/man/docs/armcc/armcc_chr1359124973480.htm

#define OT_TOOL_PACKED_BEGIN
#define OT_TOOL_PACKED_FIELD __attribute__((packed))
#define OT_TOOL_PACKED_END __attribute__((packed))
#define OT_TOOL_WEAK __attribute__((weak))

#define OT_TOOL_ALIGN(X)

#elif defined(__ICCARM__) || defined(__ICC8051__)

// http://supp.iar.com/FilesPublic/UPDINFO/004916/arm/doc/EWARM_DevelopmentGuide.ENU.pdf

#include "intrinsics.h"

#define OT_TOOL_PACKED_BEGIN __packed
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END
#define OT_TOOL_WEAK __weak

#define OT_TOOL_ALIGN(X)

#elif defined(_MSC_VER)

#define OT_TOOL_PACKED_BEGIN __pragma(pack(push, 1))
#define OT_TOOL_PACKED_FIELD
#define OT_TOOL_PACKED_END __pragma(pack(pop))
#define OT_TOOL_WEAK

#define OT_TOOL_ALIGN(X) __declspec(align(4))

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
 * @def OT_UNUSED_VARIABLE
 *
 * Suppress unused variable warning in specific toolchains.
 *
 */

/**
 * @def OT_UNREACHABLE_CODE
 *
 * Suppress Unreachable code warning in specific toolchains.
 *
 */

#if defined(__ICCARM__)

#include <stddef.h>

#define OT_UNUSED_VARIABLE(VARIABLE) \
    do                               \
    {                                \
        if (&VARIABLE == NULL)       \
        {                            \
        }                            \
    } while (false)

#define OT_UNREACHABLE_CODE(CODE)                                                                    \
    _Pragma("diag_suppress=Pe111") _Pragma("diag_suppress=Pe128") CODE _Pragma("diag_default=Pe111") \
        _Pragma("diag_default=Pe128")

#elif defined(__CC_ARM)

#include <stddef.h>

#define OT_UNUSED_VARIABLE(VARIABLE) \
    do                               \
    {                                \
        if (&VARIABLE == NULL)       \
        {                            \
        }                            \
    } while (false)

#define OT_UNREACHABLE_CODE(CODE) CODE

#elif defined(__TI_ARM__)

#include <stddef.h>

#define OT_UNUSED_VARIABLE(VARIABLE) \
    do                               \
    {                                \
        if (&VARIABLE == NULL)       \
        {                            \
        }                            \
    } while (false)

/*
 * #112-D statement is unreachable
 * #129-D loop is not reachable
 */
#define OT_UNREACHABLE_CODE(CODE) \
    _Pragma("diag_push") _Pragma("diag_suppress 112") _Pragma("diag_suppress 129") CODE _Pragma("diag_pop")

#else

#define OT_UNUSED_VARIABLE(VARIABLE) \
    do                               \
    {                                \
        (void)(VARIABLE);            \
    } while (false)

#define OT_UNREACHABLE_CODE(CODE) CODE

#endif

/*
 * Keil and IAR compiler doesn't provide type limits for C++.
 */
#if defined(__CC_ARM) || defined(__ICCARM__)

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#endif

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_TOOLCHAIN_H_
