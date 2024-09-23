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
 * @brief
 *  This file defines the top-level functions for the OpenThread CLI server.
 */

#ifndef OPENTHREAD_CLI_H_
#define OPENTHREAD_CLI_H_

#include <stdarg.h>
#include <stdint.h>

#include <openthread/instance.h>
#include <openthread/platform/logging.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a CLI command.
 */
typedef struct otCliCommand
{
    const char *mName; ///< A pointer to the command string.
    otError (*mCommand)(void   *aContext,
                        uint8_t aArgsLength,
                        char   *aArgs[]); ///< A function pointer to process the command.
} otCliCommand;

/**
 * @addtogroup api-cli
 *
 * @brief
 *   This module includes functions that control the Thread stack's execution.
 *
 * @{
 */

/**
 * Pointer is called to notify about Console output.
 *
 * @param[out] aContext    A user context pointer.
 * @param[in]  aFormat     The format string.
 * @param[in]  aArguments  The format string arguments.
 *
 * @returns                Number of bytes written by the callback.
 */
typedef int (*otCliOutputCallback)(void *aContext, const char *aFormat, va_list aArguments);

/**
 * Initialize the CLI module.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aCallback   A callback method called to process CLI output.
 * @param[in]  aContext    A user context pointer.
 */
void otCliInit(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext);

/**
 * Is called to feed in a console input line.
 *
 * @param[in]  aBuf        A pointer to a null-terminated string.
 */
void otCliInputLine(char *aBuf);

/**
 * Set a user command table.
 *
 * @param[in]  aUserCommands  A pointer to an array with user commands.
 * @param[in]  aLength        @p aUserCommands length.
 * @param[in]  aContext       @p The context passed to the handler.
 *
 * @retval OT_ERROR_NONE    Successfully updated command table with commands from @p aUserCommands.
 * @retval OT_ERROR_FAILED  Maximum number of command entries have already been set.
 */
otError otCliSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength, void *aContext);

/**
 * Write a number of bytes to the CLI console as a hex string.
 *
 * @param[in]  aBytes   A pointer to data which should be printed.
 * @param[in]  aLength  @p aBytes length.
 */
void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength);

/**
 * Write formatted string to the CLI console
 *
 * @param[in]  aFmt   A pointer to the format string.
 * @param[in]  ...    A matching list of arguments.
 */
void otCliOutputFormat(const char *aFmt, ...);

/**
 * Write error code to the CLI console
 *
 * If the @p aError is `OT_ERROR_PENDING` nothing will be outputted.
 *
 * @param[in]  aError Error code value.
 */
void otCliAppendResult(otError aError);

/**
 * Callback to write the OpenThread Log to the CLI console
 *
 * @param[in]  aLogLevel   The log level.
 * @param[in]  aLogRegion  The log region.
 * @param[in]  aFormat     A pointer to the format string.
 * @param[in]  aArgs       va_list matching aFormat.
 */
void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs);

/**
 * Callback to allow vendor specific commands to be added to the user command table.
 *
 * Available when `OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE` is enabled and
 * `OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES` is greater than 1.
 */
extern void otCliVendorSetUserCommands(void);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_CLI_H_
