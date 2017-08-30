/*
 *   Copyright 2010-2016 Nest Labs Inc. All Rights Reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *    Description:
 *      This file defines macros internal to the implementation of the
 *      Nest Labs assertion and exception checking facility.
 *
 */

#ifndef NLASSERT_INTERNAL_H
#define NLASSERT_INTERNAL_H

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
# define __nlLIKELY(condition)        __builtin_expect(condition, 1)
# define __nlUNLIKELY(condition)      __builtin_expect(condition, 0)
#else
# define __nlLIKELY(condition)        (condition)
# define __nlUNLIKELY(condition)      (condition)
#endif /* defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__) */

// Notes on the unusual design of __nlSHOULD_ASSERT and __nl_ASSERT_UNUSED:
//
//
// For non-production builds (aka "development" or "debug" builds)
// ===============================================================
//
// The nlASSERT(condition) macro evaluates the "condition" expression and, if
// it is false, calls NL_ASSERT_ABORT() to abort execution (after optionally
// performing some logging and debugging operations). Partially expanded, for
// clarity, it looks like this:
//
//     if (__nlSHOULD_ASSERT(condition))
//     {
//         do
//         {
//             /* optional logging, backtrace, and/or trap handling */
//         } while (0);
//
//         {
//             NL_ASSERT_ABORT();
//         }
//     }
//     else do { /* nothing */ } while (0)
//
// NOTE: The "if (foo) / else do { } while (0)" construct is just a more
//       robust version of the standard "do / while (0)" macro wrapper.
//       It is explained below, in the comments accompanying __nlCHECK.
//
// The __nlSHOULD_ASSERT(condition) macro must evaluate to true if "condition"
// is false; conceptually, its definition is "!(condition)". But it is not
// actually defined that way, for this reason:
//
// It is not uncommon for an equality test like "if (x == y)" to be accidentally
// written as an assignment: "if (x = y)". GCC and similar compilers can detect
// this and emit a warning, but the warning is suppressed if the assignment is
// surrounded by an extra pair of parentheses to indicate that it is
// intentional: "if ((x = y))". So if __nlSHOULD_ASSERT(condition) were defined
// as "!(condition)", the parentheses around "condition" -- required by the "!"
// operator -- would be seen by the compiler as an indication that the
// assignment was intentional, so no warning would be emitted if, for example,
// "nlASSERT(x == y)" were mistakenly written as "nlASSERT(x = y)".
//
// Therefore, __nlSHOULD_ASSERT(condition) is defined so that there will be no
// extra parentheses around "condition" when the nlASSERT(condition) macro is
// expanded. With this definition, nlASSERT(condition) expands to:
//
//     if (condition)
//     {
//         /* do nothing */
//     }
//     else if (1)
//     {
//         do
//         {
//             /* optional logging, backtrace, and/or trap handling */
//         } while (0);
//
//         {
//             NL_ASSERT_ABORT();
//         }
//     }
//     else do { /* nothing */ } while (0)
//
// GCC's branch-prediction hinting mechanism ("__builtin_expect(condition,1)")
// would also suppress the "unintended assignment" warning, so is not used in
// the macro definition. But the macro compiles to exactly the same assembly
// code as it would if the hint were included, so omitting the hint incurs no
// speed or memory cost. This is true for both ARM and x86 ISAs (tested under
// GCC 4.x.x, with optimization for speed enabled via compiler flag -O3).
//
//
// For production builds (aka "release" builds)
// ============================================
//
// The nlASSERT(condition) macro is disabled by defining it as
// __nlASSERT_UNUSED(condition).
//
// The  __nlASSERT_UNUSED(condition) macro must not perform any logging or
// debugging operations, and it must not abort program execution even when
// "condition" is false. It cannot simply be defined as "(void)0", however,
// because it must allow side effects in "condition", if any, to occur exactly
// as they would in the non-production version of the macro. And it can't be
// defined as (void)(condition) because it is desirable for an unintended
// assignment in "condition" to be caught by the compiler, just as it would be
// in a non-production build.
//
// Therefore, __nl_ASSERT_UNUSED(condition) is defined so that "condition" is
// treated as a truth value, to ensure that unintended assignment will be
// caught, and so that "condition" is evaluated at runtime if and only if it
// would also be evaluated by the non-production version of nlASSERT(condition),
// to ensure that side effects will occur identically.

#define __nlSHOULD_ASSERT(condition) condition) { /* do nothing */ } else if (1

#define __nlASSERT_UNUSED(condition) do { if (condition) { /* do nothing */ } } while (0)

/** @cond */
#define __nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix) aPrefix ## aSuffix

#define _nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix) __nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix)
/** @endcond */

/**
 *  @def _nlSTATIC_ASSERT(aCondition)
 *
 *  @brief
 *    This checks, at compile-time, for the specified condition, which
 *    is expected to commonly be true and terminates compilation if the
 *    condition is false. It is legal, in both C and C++, anywhere that
 *    a declaration would be.
 *
 *  @note This is a package-internal interface. If C++11/C11 or greater is
 *  available, this falls back to using C++11/C11 intrinsic facilities:
 *  static_assert or _Static_assert.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion
 *                          failure. Note, this message is not
 *                          actually emitted in any meaningful way for
 *                          non-C11 or -C++11 code. It serves to
 *                          simply comment or annotate the assertion
 *                          and to provide interface parallelism with
 *                          the run-time assertion interfaces.
 *
 */
#if defined(__cplusplus) && (__cplusplus >= 201103L)
# define _nlSTATIC_ASSERT(aCondition, aMessage) static_assert(aCondition, aMessage)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
# define _nlSTATIC_ASSERT(aCondition, aMessage) _Static_assert(aCondition, aMessage)
#else
# ifdef __COUNTER__
#  define _nlSTATIC_ASSERT(aCondition, aMessage) typedef char _nlSTATIC_ASSERT_CONCAT(STATIC_ASSERT_t_, __COUNTER__)[(aCondition) ? 1 : -1] __attribute__ ((unused))
# else
#  define _nlSTATIC_ASSERT(aCondition, aMessage) typedef char _nlSTATIC_ASSERT_CONCAT(STATIC_ASSERT_t_, __LINE__)[(aCondition) ? 1 : -1] __attribute__ ((unused))
# endif
#endif /* defined(__cplusplus) && (__cplusplus >= 201103L) */

// __nlSTATIC_ASSERT_UNUSED(aCondition)
//
// Can be used everywhere that _nlSTATIC_ASSERT can, and behaves exactly the
// same way except that it never asserts.
#if defined(__cplusplus) && (__cplusplus >= 201103L)
# define __nlSTATIC_ASSERT_UNUSED(aCondition, aMessage) static_assert((aCondition) || 1, aMessage)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
# define __nlSTATIC_ASSERT_UNUSED(aCondition, aMessage) _Static_assert((aCondition) || 1, aMessage)
#else
# ifdef __COUNTER__
#  define __nlSTATIC_ASSERT_UNUSED(aCondition, aMessage) typedef char _nlSTATIC_ASSERT_CONCAT(STATIC_ASSERT_t_, __COUNTER__)[((aCondition) || 1) ? 1 : -1] __attribute__ ((unused))
# else
#  define __nlSTATIC_ASSERT_UNUSED(aCondition, aMessage) typedef char _nlSTATIC_ASSERT_CONCAT(STATIC_ASSERT_t_, __LINE__)[((aCondition) || 1) ? 1 : -1] __attribute__ ((unused))
# endif
#endif /* defined(__cplusplus) && (__cplusplus >= 201103L) */

#define __NL_ASSERT_MAYBE_RUN_TRIGGERS(flags, prefix, name, condition, label, file, line, message)             \
    do                                                                                    \
    {                                                                                     \
        if ((flags) & NL_ASSERT_FLAG_LOG) {                                               \
            NL_ASSERT_LOG(prefix, name, condition, label, file, line, message);           \
        }                                                                                 \
        if ((flags) & NL_ASSERT_FLAG_BACKTRACE) {                                         \
            NL_ASSERT_BACKTRACE();                                                        \
        }                                                                                 \
        if ((flags) & NL_ASSERT_FLAG_TRAP) {                                              \
            NL_ASSERT_TRAP();                                                             \
        }                                                                                 \
    } while (0)

#define __NL_ASSERT_MAYBE_RUN_PRE_ACTION_TRIGGERS(flags, prefix, name, condition, label, file, line, message)  \
    do                                                                                    \
    {                                                                                     \
        if ((flags) & NL_ASSERT_FLAG_LOG) {                                               \
            NL_ASSERT_LOG(prefix, name, condition, label, file, line, message);           \
        }                                                                                 \
        if ((flags) & NL_ASSERT_FLAG_BACKTRACE) {                                         \
            NL_ASSERT_BACKTRACE();                                                        \
        }                                                                                 \
    } while (0)

#define __NL_ASSERT_MAYBE_RUN_POST_ACTION_TRIGGERS(flags, prefix, name, condition, label, file, line, message) \
    do                                                                                    \
    {                                                                                     \
        if ((flags) & NL_ASSERT_FLAG_TRAP) {                                              \
            NL_ASSERT_TRAP();                                                             \
        }                                                                                 \
    } while (0)

// __nlEXPECT

#define __nlEXPECT(flags, condition, label)                                               \
    do                                                                                    \
    {                                                                                     \
        if (__nlSHOULD_ASSERT(condition))                                                 \
        {                                                                                 \
            __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                       \
                                           NL_ASSERT_PREFIX_STRING,                       \
                                           NL_ASSERT_COMPONENT_STRING,                    \
                                           #condition,                                    \
                                           #label,                                        \
                                           NL_ASSERT_FILE,                                \
                                           __LINE__,                                      \
                                           0);                                            \
            goto label;                                                                   \
        }                                                                                 \
    } while (0)

#define __nlEXPECT_PRINT(flags, condition, label, message)                                \
    do                                                                                    \
    {                                                                                     \
        if (__nlSHOULD_ASSERT(condition))                                                 \
        {                                                                                 \
            __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                       \
                                           NL_ASSERT_PREFIX_STRING,                       \
                                           NL_ASSERT_COMPONENT_STRING,                    \
                                           #condition,                                    \
                                           #label,                                        \
                                           NL_ASSERT_FILE,                                \
                                           __LINE__,                                      \
                                           message);                                      \
            goto label;                                                                   \
        }                                                                                 \
    } while (0)

#define __nlEXPECT_ACTION(flags, condition, label, action)                                \
    do                                                                                    \
    {                                                                                     \
        if (__nlSHOULD_ASSERT(condition))                                                 \
        {                                                                                 \
            __NL_ASSERT_MAYBE_RUN_PRE_ACTION_TRIGGERS((flags),                            \
                                                      NL_ASSERT_PREFIX_STRING,            \
                                                      NL_ASSERT_COMPONENT_STRING,         \
                                                      #condition,                         \
                                                      #label,                             \
                                                      NL_ASSERT_FILE,                     \
                                                      __LINE__,                           \
                                                      0);                                 \
            {                                                                             \
                action;                                                                   \
            }                                                                             \
            __NL_ASSERT_MAYBE_RUN_POST_ACTION_TRIGGERS((flags),                           \
                                                       NL_ASSERT_PREFIX_STRING,           \
                                                       NL_ASSERT_COMPONENT_STRING,        \
                                                       #condition,                        \
                                                       #label,                            \
                                                       NL_ASSERT_FILE,                    \
                                                       __LINE__,                          \
                                                       0);                                \
            goto label;                                                                   \
        }                                                                                 \
    } while (0)

#define __nlEXPECT_ACTION_PRINT(flags, condition, label, action, message)                 \
    do                                                                                    \
    {                                                                                     \
        if (__nlSHOULD_ASSERT(condition))                                                 \
        {                                                                                 \
            __NL_ASSERT_MAYBE_RUN_PRE_ACTION_TRIGGERS((flags),                            \
                                                      NL_ASSERT_PREFIX_STRING,            \
                                                      NL_ASSERT_COMPONENT_STRING,         \
                                                      #condition,                         \
                                                      #label,                             \
                                                      NL_ASSERT_FILE,                     \
                                                      __LINE__,                           \
                                                      message);                           \
            {                                                                             \
                action;                                                                   \
            }                                                                             \
            __NL_ASSERT_MAYBE_RUN_POST_ACTION_TRIGGERS((flags),                           \
                                                       NL_ASSERT_PREFIX_STRING,           \
                                                       NL_ASSERT_COMPONENT_STRING,        \
                                                       #condition,                        \
                                                       #label,                            \
                                                       NL_ASSERT_FILE,                    \
                                                       __LINE__,                          \
                                                       message);                          \
            goto label;                                                                   \
        }                                                                                 \
    } while (0)

#define __nlEXPECT_SUCCESS(flags, status, label)                                          __nlEXPECT(flags, status == 0, label)
#define __nlEXPECT_SUCCESS_PRINT(flags, status, label, message)                           __nlEXPECT_PRINT(flags, status == 0, label, message)
#define __nlEXPECT_SUCCESS_ACTION(flags, status, label, action)                           __nlEXPECT_ACTION(flags, status == 0, label, action)
#define __nlEXPECT_SUCCESS_ACTION_PRINT(flags, status, label, action, message)            __nlEXPECT_ACTION_PRINT(flags, status == 0, label, action, message)

#define __nlNEXPECT(flags, condition, label)                                              __nlEXPECT(flags, !(condition), label)
#define __nlNEXPECT_PRINT(flags, condition, label, message)                               __nlEXPECT_PRINT(flags, !(condition), label, message)
#define __nlNEXPECT_ACTION(flags, condition, label, action)                               __nlEXPECT_ACTION(flags, !(condition), label, action)
#define __nlNEXPECT_ACTION_PRINT(flags, condition, label, action, message)                __nlEXPECT_ACTION_PRINT(flags, !(condition), label, action, message)

// __nlCHECK
//
// NOTE: Some of these macros take a C statement as a parameter. The unusual
// "else do {} while(0)" construct allows those macros to work properly when
// that parameter is set to "continue" or "break" (which isn't unexpected in
// an assert macro).
//
// If the macros were written in the standard way by wrapping them in a
// "do/while(0)", they could fail silently: A "continue" or "break" statement,
// intended to break out of one or all iterations of the loop containing the
// macro invocation -- would instead just break out of the macro's internal
// do-while.

#define __nlCHECK(flags, condition)                                                       \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                           \
                                       NL_ASSERT_PREFIX_STRING,                           \
                                       NL_ASSERT_COMPONENT_STRING,                        \
                                       #condition,                                        \
                                       0,                                                 \
                                       NL_ASSERT_FILE,                                    \
                                       __LINE__,                                          \
                                       0);                                                \
    }                                                                                     \
    else do {} while (0)

#define __nlCHECK_ACTION(flags, condition, action)                                        \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_PRE_ACTION_TRIGGERS((flags),                                \
                                                  NL_ASSERT_PREFIX_STRING,                \
                                                  NL_ASSERT_COMPONENT_STRING,             \
                                                  #condition,                             \
                                                  0,                                      \
                                                  NL_ASSERT_FILE,                         \
                                                  __LINE__,                               \
                                                  0);                                     \
        {                                                                                 \
            action;                                                                       \
        }                                                                                 \
        __NL_ASSERT_MAYBE_RUN_POST_ACTION_TRIGGERS((flags),                               \
                                                   NL_ASSERT_PREFIX_STRING,               \
                                                   NL_ASSERT_COMPONENT_STRING,            \
                                                   #condition,                            \
                                                   0,                                     \
                                                   NL_ASSERT_FILE,                        \
                                                   __LINE__,                              \
                                                   0);                                    \
    }                                                                                     \
    else do {} while (0) /* This is explained in the comment above __nlCHECK */

#define __nlCHECK_PRINT(flags, condition, message)                                        \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                           \
                                       NL_ASSERT_PREFIX_STRING,                           \
                                       NL_ASSERT_COMPONENT_STRING,                        \
                                       #condition,                                        \
                                       0,                                                 \
                                       NL_ASSERT_FILE,                                    \
                                       __LINE__,                                          \
                                       message);                                          \
    }                                                                                     \
    else do {} while (0) /* This is explained in the comment above __nlCHECK */

#define __nlCHECK_SUCCESS(flags, status)                                                  __nlCHECK(flags, status == 0)
#define __nlCHECK_SUCCESS_ACTION(flags, status, action)                                   __nlCHECK_ACTION(flags, status == 0, action)
#define __nlCHECK_SUCCESS_PRINT(flags, status, message)                                   __nlCHECK_PRINT(flags, status == 0, message)

#define __nlNCHECK(flags, condition)                                                      __nlCHECK(flags, !(condition))
#define __nlNCHECK_ACTION(flags, condition, action)                                       __nlCHECK_ACTION(flags, !(condition), action)
#define __nlNCHECK_PRINT(flags, condition, message)                                       __nlCHECK_PRINT(flags, !(condition), message)

// __nlVERIFY

#define __nlVERIFY(flags, condition)                                                      __nlCHECK(flags, condition)
#define __nlVERIFY_ACTION(flags, condition, action)                                       __nlCHECK_ACTION(flags, condition, action)
#define __nlVERIFY_PRINT(flags, condition, message)                                       __nlCHECK_PRINT(flags, condition, message)

#define __nlVERIFY_SUCCESS(flags, status)                                                 __nlCHECK_SUCCESS(flags, status)
#define __nlVERIFY_SUCCESS_ACTION(flags, status, action)                                  __nlCHECK_SUCCESS_ACTION(flags, status, action)
#define __nlVERIFY_SUCCESS_PRINT(flags, status, message)                                  __nlCHECK_SUCCESS_PRINT(flags, status, message)

#define __nlNVERIFY(flags, condition)                                                     __nlNCHECK(flags, condition)
#define __nlNVERIFY_ACTION(flags, condition, action)                                      __nlNCHECK_ACTION(flags, condition, action)
#define __nlNVERIFY_PRINT(flags, condition, message)                                      __nlNCHECK_PRINT(flags, condition, message)

// __nlPRECONDITION

#define __nlPRECONDITION(flags, condition, termination)                                   \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                           \
                                       NL_ASSERT_PREFIX_STRING,                           \
                                       NL_ASSERT_COMPONENT_STRING,                        \
                                       #condition,                                        \
                                       0,                                                 \
                                       NL_ASSERT_FILE,                                    \
                                       __LINE__,                                          \
                                       0);                                                \
        {                                                                                 \
            termination;                                                                  \
        }                                                                                 \
    }                                                                                     \
    else do {} while (0) /* This is explained in the comment above __nlCHECK */

#define __nlPRECONDITION_ACTION(flags, condition, termination, action)                    \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_PRE_ACTION_TRIGGERS((flags),                                \
                                                  NL_ASSERT_PREFIX_STRING,                \
                                                  NL_ASSERT_COMPONENT_STRING,             \
                                                  #condition,                             \
                                                  0,                                      \
                                                  NL_ASSERT_FILE,                         \
                                                  __LINE__,                               \
                                                  0);                                     \
        {                                                                                 \
            action;                                                                       \
        }                                                                                 \
        __NL_ASSERT_MAYBE_RUN_POST_ACTION_TRIGGERS((flags),                               \
                                                   NL_ASSERT_PREFIX_STRING,               \
                                                   NL_ASSERT_COMPONENT_STRING,            \
                                                   #condition,                            \
                                                   0,                                     \
                                                   NL_ASSERT_FILE,                        \
                                                   __LINE__,                              \
                                                   0);                                    \
        {                                                                                 \
            termination;                                                                  \
        }                                                                                 \
    }                                                                                     \
    else do {} while (0) /* This is explained in the comment above __nlCHECK */

#define __nlPRECONDITION_PRINT(flags, condition, termination, message)                    \
    if (__nlSHOULD_ASSERT(condition))                                                     \
    {                                                                                     \
        __NL_ASSERT_MAYBE_RUN_TRIGGERS((flags),                                           \
                                       NL_ASSERT_PREFIX_STRING,                           \
                                       NL_ASSERT_COMPONENT_STRING,                        \
                                       #condition,                                        \
                                       0,                                                 \
                                       NL_ASSERT_FILE,                                    \
                                       __LINE__,                                          \
                                       message);                                          \
        {                                                                                 \
            termination;                                                                  \
        }                                                                                 \
    }                                                                                     \
    else do {} while (0) /* This is explained in the comment above __nlCHECK */

#define __nlPRECONDITION_SUCCESS(flags, status, termination)                              __nlPRECONDITION(flags, status == 0, termination)
#define __nlPRECONDITION_SUCCESS_ACTION(flags, status, termination, action)               __nlPRECONDITION_ACTION(flags, status == 0, termination, action)
#define __nlPRECONDITION_SUCCESS_PRINT(flags, status, termination, message)               __nlPRECONDITION_PRINT(flags, status == 0, termination, message)

#define __nlNPRECONDITION(flags, condition, termination)                                  __nlPRECONDITION(flags, !(condition), termination)
#define __nlNPRECONDITION_ACTION(flags, condition, termination, action)                   __nlPRECONDITION_ACTION(flags, !(condition), termination, action)
#define __nlNPRECONDITION_PRINT(flags, condition, termination, message)                   __nlPRECONDITION_PRINT(flags, !(condition), termination, message)

// __nlABORT

#define __nlABORT(flags, condition)                                                       __nlPRECONDITION(flags, condition, NL_ASSERT_ABORT())

#define __nlABORT_ACTION(flags, condition, action)                                        __nlPRECONDITION_ACTION(flags, condition, NL_ASSERT_ABORT(), action)

#endif /* NLASSERT_INTERNAL_H */
