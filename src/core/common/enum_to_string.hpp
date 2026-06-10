/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file defines utility macros for converting enums to string.
 */

#ifndef OT_CORE_COMMON_ENUM_TO_STRING_HPP_
#define OT_CORE_COMMON_ENUM_TO_STRING_HPP_

#include "common/string.hpp"

/**
 * Defines a `static constexpr` array named `kStrings[]` for enum to string conversion and validates it at
 * compile-time.
 *
 * This macro uses the X-Macro pattern. It requires a list macro (`kMapList`) that defines the mapping between enum
 * values and their string representations.
 *
 * The `kMapList` macro MUST accept a single parameter, which is a "visitor" macro (`_`). The visitor macro will be
 * called for each entry in the list.
 *
 * The entries in `kMapList` MUST be sorted based on their enum value, starting from zero. Each entry MUST be a `_()`
 * call with the enum value and its corresponding string literal.
 *
 * It is recommended to use `_` as the visitor macro parameter name. This helps `clang-format` to format the list with
 * one entry per line, improving readability.
 *
 * Example:
 *
 *     #define StateMapList(_)       \
 *         _(kStateIdle, "Idle")     \
 *         _(kStateStart, "Start")   \
 *         _(kStateRunning, "Running")
 *
 * `DefineEnumStringArray(StateMapList)` will expand at compile time to:
 *
 * 1. An `const` array definition:
 *    `static constexpr const char *const kStrings[] = { "Idle", "Start", "Running", };`
 *
 * 2. A set of static assertions for validation:
 *    `static_assert(AreConstStringsEqual(kStrings[kStateIdle], "Idle"), "kStateIdle is invalid");`
 *    `static_assert(AreConstStringsEqual(kStrings[kStateStart], "Start"), "kStateStart is invalid");`
 *    // ... and so on for all entries.
 *
 * The `kStrings[]` array can then be used to convert an enum value to its string representation.
 *
 * @param[in] kMapList   The X-Macro list that defines the enum-to-string mapping.
 */
#define DefineEnumStringArray(kMapList)                                                     \
    static constexpr const char *const kStrings[] = {kMapList(_DefineEnumMapArrayElement)}; \
    kMapList(_ValidateEnumMapEntry)

//---------------------------------------------------------------------------------------------------------------------
// Private macros

#define _DefineEnumMapArrayElement(kEnum, kString) kString,

#define _ValidateEnumMapEntry(kEnum, kString) \
    static_assert(AreConstStringsEqual(kStrings[kEnum], kString), #kEnum " value is incorrect. list is not sorted");

#endif // OT_CORE_COMMON_ENUM_TO_STRING_HPP_
