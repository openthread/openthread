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
 *   This file contains definitions for the Co-processor Remote Procedure Call (RPC) module.
 */

#ifndef COPROCESSOR_RPC_HPP_
#define COPROCESSOR_RPC_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE

#include <string.h>

#include <openthread/cli.h>
#include <openthread/platform/radio.h>

#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "utils/parse_cmdline.hpp"

namespace ot {
namespace Coprocessor {

class RPC : public InstanceLocator, private NonCopyable
{
public:
    typedef Utils::CmdLineParser::Arg Arg;
    typedef otCliCommand              Command;

    /**
     * Constructor.
     *
     * @param[in]  aInstance  The OpenThread instance.
     *
     */
    explicit RPC(Instance &aInstance);

    /**
     * This method returns whether @ref sRPC is initialized.
     *
     * @returns  Whether @ref sRPC is initialized.
     *
     */
    static bool IsInitialized(void) { return sRPC != nullptr; }

    /**
     * This method returns a reference to the RPC object.
     *
     * @returns A reference to the RPC object.
     *
     */
    static RPC &GetRPC(void)
    {
        OT_ASSERT(sRPC != nullptr);

        return *sRPC;
    }

    /**
     * Initialize the RPC object.
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    static void Initialize(Instance &aInstance);

    /**
     * Process a RPC command line.
     *
     * @param[in]   aString        A null-terminated input string.
     * @param[out]  aOutput        The execution result.
     * @param[in]   aOutputMaxLen  The output buffer size.
     *
     */
    void ProcessLine(const char *aString, char *aOutput, size_t aOutputMaxLen);

    /**
     * Process a RPC command line.
     *
     * @param[in]   aArgsLength    The number of args in @p aArgs.
     * @param[in]   aArgs          The arguments of command line.
     * @param[out]  aOutput        The execution result.
     * @param[in]   aOutputMaxLen  The output buffer size.
     *
     * @retval  kErrorInvalidArgs       The command is supported but invalid arguments provided.
     * @retval  kErrorNone              The command is successfully process.
     * @retval  kErrorNotImplemented    The command is not supported.
     *
     */
    Error ProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen);

    /**
     * Call the corresponding handler for a command
     *
     * This method will look through @p aCommands to find a @ref CRPC::Command
     * that matches @p aArgs[0]. If found, the handler function for the command
     * will be called with the remaining args passed to it.
     *
     * @param[in]  aContext         a context
     * @param[in]  aArgsLength      number of args
     * @param[in]  aArgs            list of args
     * @param[in]  aCommandsLength  number of commands in @p aCommands
     * @param[in]  aCommands        list of commands
     *
     * @retval false if no matching command was found
     * @retval true if a matching command was found and the handler was called
     *
     */
    static Error HandleCommand(void *        aContext,
                               uint8_t       aArgsLength,
                               char *        aArgs[],
                               uint8_t       aCommandsLength,
                               const Command aCommands[]);

#if OPENTHREAD_COPROCESSOR
    /**
     * Set the user command table.
     *
     * @param[in]  aUserCommands  A pointer to an array with user commands.
     * @param[in]  aLength        @p aUserCommands length.
     * @param[in]  aContext       @p aUserCommands context.
     *
     */
    void SetUserCommands(const Command aCommands[], uint8_t aLength, void *aContext);

    /**
     * This method sets the user command error
     *
     * @param[in]  aError         An error
     *
     */
    void SetUserCommandError(otError aError);

    /**
     * Write a number of bytes to the output buffer as a hex string.
     *
     * @param[in]  aBytes   A pointer to data which should be printed.
     * @param[in]  aLength  @p aBytes length.
     *
     */
    void OutputBytes(const uint8_t *aBytes, uint16_t aLength);

    /**
     * Write a number of bytes to the output buffer as a hex string.
     *
     * @tparam kBytesLength   The length of @p aBytes array.
     *
     * @param[in]  aBytes     A array of @p kBytesLength bytes which should be printed.
     *
     */
    template <uint8_t kBytesLength> void OutputBytes(const uint8_t (&aBytes)[kBytesLength])
    {
        OutputBytes(aBytes, kBytesLength);
    }

    /**
     * Write all commands in @p aCommands to the output buffer
     *
     * @param[in]  aCommands        List of commands
     * @param[in]  aCommandsLength  Number of commands in @p aCommands
     *
     */
    void OutputCommands(const Command aCommands[], size_t aCommandsLength);

    /**
     * Write formatted output to the output buffer.
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     *
     * @returns The number of bytes placed in the output buffer.
     *
     * @retval  -1  Driver is broken.
     *
     */
    int OutputFormat(const char *aFormat, ...);

    /**
     * Write formatted output (to which it prepends a given number indentation space chars) to the
     * client.
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     *
     */
    void OutputFormat(uint8_t aIndentSize, const char *aFormat, ...);

    /**
     * Write formatted output to the output buffer.
     *
     * @param[in]  aFormat      A pointer to the format string.
     * @param[in]  aArguments   A variable list of arguments for format.
     *
     * @returns The number of bytes placed in the output buffer.
     *
     */
    int OutputFormatV(const char *aFormat, va_list aArguments);

    /**
     * Write an IPv6 address to the output buffer.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @returns The number of bytes placed in the output buffer.
     *
     * @retval  -1  Driver is broken.
     *
     */
    int OutputIp6Address(const otIp6Address &aAddress);

    /**
     * Write formatted output (to which it also appends newline `\r\n`) to the output buffer.
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     *
     */
    void OutputLine(const char *aFormat, ...);

    /**
     * Write formatted output (to which it prepends a given number indentation space chars and appends
     * newline `\r\n`) to the output buffer.
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     *
     */
    void OutputLine(uint8_t aIndentSize, const char *aFormat, ...);

    /**
     * Write a success or error message the client.
     *
     * If the @p aError is `OT_ERROR_PENDING` nothing will be outputted.
     *
     * @param[in]  aError  The error code.
     *
     */
    void OutputResult(otError aError);

    /**
     * Write a given number of space chars to the output buffer.
     *
     * @param[in] aCount  Number of space chars to output.
     *
     */
    void OutputSpaces(uint8_t aCount);

    /**
     * Write an Extended MAC Address to the output buffer.
     *
     * param[in] aExtAddress  The Extended MAC Address to output.
     *
     */
    void OutputExtAddress(const otExtAddress &aExtAddress) { OutputBytes(aExtAddress.m8); }

    void ProcessHelp(void *aContext, uint8_t aArgsLength, char *aArgs[]);

#endif
    enum
    {
        kMaxCommands              = OPENTHREAD_CONFIG_COPROCESSOR_RPC_COMMANDS_MAX,
        kMaxArgs                  = OPENTHREAD_CONFIG_COPROCESSOR_RPC_CMD_LINE_ARGS_MAX,
        kMaxCommandBuffer         = OPENTHREAD_CONFIG_COPROCESSOR_RPC_OUTPUT_BUFFER_SIZE,
        kCommandCacheBufferLength = OPENTHREAD_CONFIG_COPROCESSOR_RPC_COMMAND_CACHE_BUFFER_SIZE,
    };

protected:
    static RPC *sRPC;

private:
#if OPENTHREAD_COPROCESSOR
    char * mOutputBuffer;
    size_t mOutputBufferCount;
    size_t mOutputBufferMaxLen;

    /**
     * Store the output buffer pointer and size
     *
     * These will be used later by @ref OutputFormat
     *
     * @param[in]   aOutput         A output buffer
     * @param[in]   aOutputMaxLen   The size of @p aOutput
     */
    void SetOutputBuffer(char *aOutput, size_t aOutputMaxLen);

    /**
     * Clear the output buffer variables
     */
    void ClearOutputBuffer(void);
#endif

#if OPENTHREAD_COPROCESSOR
    const Command *mUserCommands;
    void *         mUserCommandsContext;
    otError        mUserCommandsError;
    uint8_t        mUserCommandsLength;
#else
    static Arg     mCachedCommands[kMaxCommands];
    static char    mCachedCommandsBuffer[kCommandCacheBufferLength];
    static uint8_t mCachedCommandsLength;
#endif
    static const Command sCommands[];

    /**
     * Parse a string into an array of argumentsparses a given command line string and breaks it into an argument list.
     *
     * @param[in]   aString         input string
     * @param[out]  aArgsLength     The argument counter of the command line.
     * @param[out]  aArgs           The argument vector of the command line.
     */
    Error ParseCmd(char *aString, uint8_t &aArgsLength, char *aArgs[]);
};

} // namespace Coprocessor
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE

#endif // COPROCESSOR_RPC_HPP_
