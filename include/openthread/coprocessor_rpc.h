/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes the OpenThread API for Co-processor RPC (CRPC).
 */

#ifndef OPENTHREAD_COPROCESSOR_RPC_H_
#define OPENTHREAD_COPROCESSOR_RPC_H_

#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-coprocessor-rpc
 *
 * @brief
 *   This module includes functions that allow a Host processor to execute
 *   remote procedure calls on a co-processor over spinel
 *
 * @{
 *
 */

/**
 * Append an error code to the output buffer
 *
 * @param[in] aError
 */
void otCliAppendResult(otError aError);

/**
 * This method sets the user command error
 *
 * @param[in]  aError         An error
 *
 */
void otCliSetUserCommandError(otError aError);

/**
 * Call the corresponding handler for a command
 *
 * This function will look through @p aCommands to find a command that matches
 * @p aArgs[0]. If found, the handler function for the command will be called
 * with the remaining args passed to it.
 *
 * @param[in]  aContext         a context
 * @param[in]  aArgsLength      number of args
 * @param[in]  aArgs            list of args
 * @param[in]  aCommandsLength  number of commands in @p aCommands
 * @param[in]  aCommands        list of commands
 *
 * @retval \ref OT_ERROR_INVALID_COMMAND if no matching command was found
 * @retval \ref OT_ERROR_NONE if a matching command was found and the handler was called
 *
 */
otError otCRPCHandleCommand(void *             aContext,
                            uint8_t            aArgsLength,
                            char *             aArgs[],
                            uint8_t            aCommandsLength,
                            const otCliCommand aCommands[]);

/**
 * Output a byte array as hex to the output buffer
 *
 * @param[in] aBytes            Bytes to output
 * @param[in] aLength           Number of bytes
 */
void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength);

/**
 * Write all command names in @p aCommands to the output buffer
 *
 * @param[in] aCommands         an array of commands
 * @param[in] aCommandsLength   length of @p aCommands
 */
void otCliOutputCommands(const otCliCommand aCommands[], size_t aCommandsLength);

/**
 * Write formatted output to the output buffer
 *
 * @param[in]  aFormat      A pointer to the format string.
 * @param[in]  aArguments   A variable list of arguments for format.
 *
 * @returns The number of bytes placed in the output buffer.
 *
 */
int otCliOutputFormatV(const char *aFormat, va_list aArguments);

/**
 * Write formatted string to the output buffer
 *
 * @param[in]  aFmt   A pointer to the format string.
 * @param[in]  ...    A matching list of arguments.
 *
 */
void otCliOutputFormat(const char *aFmt, ...);

/**
 * Write an IPv6 address to the output buffer.
 *
 * @param[in]  aAddress  A pointer to the IPv6 address.
 *
 * @returns The number of bytes placed in the output queue.
 *
 * @retval  -1  Driver is broken.
 *
 */
int otCliOutputIp6Address(const otIp6Address *aAddress);

/**
 * Process a command line
 *
 * @param[in]   aArgsLength     The number of elements in @p aArgs.
 * @param[in]   aArgs           An array of arguments.
 * @param[out]  aOutput         The execution result.
 * @param[in]   aOutputMaxLen   The output buffer size.
 *
 * @retval  OT_ERROR_INVALID_ARGS       The command is supported but invalid arguments provided.
 * @retval  OT_ERROR_NONE               The command is successfully process.
 * @retval  OT_ERROR_NOT_IMPLEMENTED    The command is not supported.
 *
 */
otError otCRPCProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen);

/**
 * Process a command-line string
 *
 * @param[in]   aString         A NULL-terminated input string.
 * @param[out]  aOutput         The execution result.
 * @param[in]   aOutputMaxLen   The output buffer size.
 *
 */
void otCRPCProcessCmdLine(const char *aString, char *aOutput, size_t aOutputMaxLen);

/**
 * Output all available CRPC built-in commands and user commands
 *
 * @param aContext
 * @param aArgsLength
 * @param aArgs
 */
void otCRPCProcessHelp(void *aContext, uint8_t aArgsLength, char *aArgs[]);

/**
 * Set a user command table.
 *
 * @param[in]  aUserCommands  A pointer to an array with user commands.
 * @param[in]  aLength        @p aUserCommands length.
 * @param[in]  aContext       @p The context passed to the handler.
 *
 */
void otCRPCSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength, void *aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_COPROCESSOR_RPC_H_
