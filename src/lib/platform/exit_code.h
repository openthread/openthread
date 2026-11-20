/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file contains header for exit code utilities.
 */

#ifndef OT_LIB_PLATFORM_EXIT_CODE_H_
#define OT_LIB_PLATFORM_EXIT_CODE_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/logging.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents exit codes used when OpenThread exits.
 */
enum
{
    /**
     * Success.
     */
    OT_EXIT_SUCCESS = 0,

    /**
     * Generic failure.
     */
    OT_EXIT_FAILURE = 1,

    /**
     * Invalid arguments.
     */
    OT_EXIT_INVALID_ARGUMENTS = 2,

    /**
     * Incompatible radio spinel.
     */
    OT_EXIT_RADIO_SPINEL_INCOMPATIBLE = 3,

    /**
     * Unexpected radio spinel reset.
     */
    OT_EXIT_RADIO_SPINEL_RESET = 4,

    /**
     * System call or library function error.
     */
    OT_EXIT_ERROR_ERRNO = 5,

    /**
     * No response from radio spinel.
     */
    OT_EXIT_RADIO_SPINEL_NO_RESPONSE = 6,

    /**
     * Invalid state.
     */
    OT_EXIT_INVALID_STATE = 7,

    /**
     * RCP chip reset is not able to be done by OT
     */
    OT_EXIT_RCP_RESET_REQUIRED = 8,
};

/**
 * Compiler-specific indication that a function does not return.
 */
#if defined(__GNUC__) || defined(__clang__)
#define OT_EXIT_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define OT_EXIT_NORETURN __declspec(noreturn)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define OT_EXIT_NORETURN _Noreturn
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#define OT_EXIT_NORETURN [[noreturn]]
#else
#define OT_EXIT_NORETURN
#endif

/**
 * Pointer type for a callback invoked right before the process terminates
 * fatally.
 *
 * @param[in]  aExitCode  The exit code.
 * @param[in]  aContext   A pointer to arbitrary context information.
 */
typedef void (*otExitHandler)(uint8_t aExitCode, void *aContext);

/**
 * Registers a callback to be invoked right before the process terminates
 * fatally.
 *
 * Subsequent calls overwrite any previously registered handler. Passing NULL
 * for @p aHandler clears the registered handler.
 *
 * @param[in]  aHandler  A pointer to the callback function, or NULL to clear.
 * @param[in]  aContext  A pointer to arbitrary context information.
 */
void otExitSetHandler(otExitHandler aHandler, void *aContext);

/**
 * Indicates whether a fatal exit is currently in progress.
 *
 * @retval TRUE   A fatal exit is currently in progress.
 * @retval FALSE  No fatal exit is currently in progress.
 */
bool otExitIsExiting(void);

/**
 * Invokes the registered fatal exit handler (if any) and terminates the
 * process via `exit()`.
 *
 * On the first call, this function records @p aExitCode as the process exit
 * code and invokes the registered handler (if any) before calling `exit()`.
 * Re-entrant calls do not re-invoke the handler and preserve the original
 * termination mode (calling `abort()` if the outer termination was initiated
 * via `otAbortNow()`, or `exit()` with the original recorded exit code
 * otherwise).
 *
 * @param[in]  aExitCode  The exit code.
 */
OT_EXIT_NORETURN void otExitNow(uint8_t aExitCode);

/**
 * Invokes the registered fatal exit handler (if any) and terminates the
 * process via `abort()`.
 *
 * On the first call, this function records abort termination and invokes the
 * registered handler (if any) with @p aExitCode before calling `abort()`.
 * Re-entrant calls do not re-invoke the handler and preserve the original
 * termination mode (calling `abort()` if the outer termination was initiated
 * via `otAbortNow()`, or `exit()` with the original recorded exit code
 * otherwise).
 *
 * @param[in]  aExitCode  The exit code passed to the registered handler.
 */
OT_EXIT_NORETURN void otAbortNow(uint8_t aExitCode);

/**
 * Converts an exit code into a string.
 *
 * @param[in]  aExitCode  An exit code.
 *
 * @returns  A string representation of an exit code.
 */
const char *otExitCodeToString(uint8_t aExitCode);

/**
 * Checks for the specified condition, which is expected to commonly be true,
 * and both records exit status and terminates the program if the condition is false.
 *
 * @param[in]   aCondition  The condition to verify
 * @param[in]   aExitCode   The exit code.
 */
#define VerifyOrDie(aCondition, aExitCode)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (aCondition)                                                                            \
        {                                                                                          \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            uint8_t     _verifyOrDieExitCode = (aExitCode);                                        \
            const char *start                = strrchr(__FILE__, '/');                             \
            OT_UNUSED_VARIABLE(start);                                                             \
            otLogCritPlat("%s() at %s:%d: %s", __func__, (start ? start + 1 : __FILE__), __LINE__, \
                          otExitCodeToString(_verifyOrDieExitCode));                               \
            otExitNow(_verifyOrDieExitCode);                                                       \
        }                                                                                          \
    } while (false)

/**
 * Checks for the specified error code, which is expected to commonly be successful,
 * and both records exit status and terminates the program if the error code is unsuccessful.
 *
 * @param[in]  aError  An error code to be evaluated against OT_ERROR_NONE.
 */
#define SuccessOrDie(aError)                                                                                      \
    do                                                                                                            \
    {                                                                                                             \
        otError _successOrDieError = (aError);                                                                    \
        VerifyOrDie(_successOrDieError == OT_ERROR_NONE,                                                          \
                    (_successOrDieError == OT_ERROR_INVALID_ARGS ? OT_EXIT_INVALID_ARGUMENTS : OT_EXIT_FAILURE)); \
    } while (false)

/**
 * Unconditionally both records exit status and terminates the program.
 *
 * @param[in]   aExitCode   The exit code.
 */
#define DieNow(aExitCode) VerifyOrDie(false, aExitCode)

/**
 * Unconditionally both records exit status and exit message and terminates the program.
 *
 * @param[in]   aMessage    The exit message.
 * @param[in]   aExitCode   The exit code.
 */
#define DieNowWithMessage(aMessage, aExitCode)                                                       \
    do                                                                                               \
    {                                                                                                \
        uint8_t _dieNowExitCode = (aExitCode);                                                       \
        otLogCritPlat("exit(%d): %s line %d, %s, %s", _dieNowExitCode, __func__, __LINE__, aMessage, \
                      otExitCodeToString(_dieNowExitCode));                                          \
        otExitNow(_dieNowExitCode);                                                                  \
    } while (false)

#ifdef __cplusplus
}
#endif

#endif // OT_LIB_PLATFORM_EXIT_CODE_H_
