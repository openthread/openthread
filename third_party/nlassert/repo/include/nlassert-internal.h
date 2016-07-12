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

#define __nlSHOULD_ASSERT(condition)    __nlUNLIKELY(!(condition))

#define __nlASSERT_UNUSED(statement)    do { (void)(statement); } while (0)

/** @cond */
#define __nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix) aPrefix ## aSuffix

#define _nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix) __nlSTATIC_ASSERT_CONCAT(aPrefix, aSuffix)
/** @endcond */

/**
 *  @def _nlSTATIC_ASSERT(aCondition)
 *
 *  @brief
 *    This checks, at compile-time, for the specified condition, which
 *    is expected to commonly be true and takes terminates compilation
 *    if the condition is false.
 *
 *  @note This is a package-internal interface. If C++11 or greater is
 *  available, this falls back to using C++11 intrinsic facilities:
 *  static_assert.
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
# define _nlSTATIC_ASSERT(aCondition, aMessage) enum { _nlSTATIC_ASSERT_CONCAT(knlSTATIC_ASSERT_LINE_, __LINE__) = 1 / (!!(aCondition)) };
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
    else do {} while (0)

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
    else do {} while (0)

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
    else do {} while (0)

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
    else do {} while (0)

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
    else do {} while (0)

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
