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
 *   This file implements the Co-processor RPC (CRPC) module.
 */

#include "rpc.hpp"

#if OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE

#include <stdio.h>
#include <stdlib.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/new.hpp"
#include "openthread/coprocessor_rpc.h"
#include "utils/parse_cmdline.hpp"

/**
 * Deliver the platform specific coprocessor RPC commands to radio only ncp
 *
 * @param[in]   aInstance       The OpenThread instance structure.
 * @param[in]   aArgsLength     The number of elements in @p aArgs.
 * @param[in]   aArgs           An array of arguments.
 * @param[out]  aOutput         The execution result.
 * @param[in]   aOutputMaxLen   The output buffer size.
 *
 * NOTE: This only needs to be implemented for the POSIX platform
 */
OT_TOOL_WEAK
otError otPlatCRPCProcess(otInstance *aInstance,
                          uint8_t     aArgsLength,
                          char *      aArgs[],
                          char *      aOutput,
                          size_t      aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aOutput);
    OT_UNUSED_VARIABLE(aOutputMaxLen);

    return ot::kErrorInvalidCommand;
}

namespace ot {
namespace Coprocessor {

RPC *RPC::sRPC = nullptr;
static OT_DEFINE_ALIGNED_VAR(sRPCRaw, sizeof(RPC), uint64_t);

#if OPENTHREAD_COPROCESSOR
#include <openthread/ip6.h>
#include "common/encoding.hpp"
using ot::Encoding::BigEndian::HostSwap16;

const RPC::Command RPC::sCommands[] = {
    {"help-crpc", otCRPCProcessHelp},
};
#else

RPC::Arg RPC::mCachedCommands[RPC::kMaxCommands];
char     RPC::mCachedCommandsBuffer[RPC::kCommandCacheBufferLength];
uint8_t  RPC::mCachedCommandsLength = 0;
#endif

RPC::RPC(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_COPROCESSOR
    , mOutputBuffer(nullptr)
    , mOutputBufferCount(0)
    , mOutputBufferMaxLen(0)
    , mUserCommands(nullptr)
    , mUserCommandsContext(nullptr)
    , mUserCommandsLength(0)
#else
#endif
{
    if (!IsInitialized())
    {
        Initialize(aInstance);
    }
}

void RPC::Initialize(Instance &aInstance)
{
    char        help[]      = "help-crpc\n";
    char *      helpCmd[]   = {help};
    static bool initStarted = false;
    OT_UNUSED_VARIABLE(helpCmd);

    VerifyOrExit((RPC::sRPC == nullptr) && !initStarted);
    initStarted = true;
    RPC::sRPC   = new (&sRPCRaw) RPC(aInstance);

#if !OPENTHREAD_COPROCESSOR
    // Initialize a response buffer
    memset(ot::Coprocessor::RPC::mCachedCommandsBuffer, 0, sizeof(ot::Coprocessor::RPC::mCachedCommandsBuffer));

    // Get a list of supported commands
    SuccessOrExit(otPlatCRPCProcess(&RPC::sRPC->GetInstance(), OT_ARRAY_LENGTH(helpCmd), helpCmd,
                                    RPC::sRPC->mCachedCommandsBuffer, sizeof(RPC::sRPC->mCachedCommandsBuffer)));

    // Parse response string into mCachedCommands to make it iterable
    SuccessOrExit(Utils::CmdLineParser::ParseCmd(RPC::sRPC->mCachedCommandsBuffer,
                                                 RPC::sRPC->mCachedCommands,
                                                 OT_ARRAY_LENGTH(RPC::sRPC->mCachedCommands)));

    // Get the number of supported commands
    mCachedCommandsLength = Arg::GetArgsLength(RPC::sRPC->mCachedCommands);
#endif
exit:
    return;
}

void RPC::ProcessLine(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    Error   error = kErrorNone;
    char    buffer[kMaxCommandBuffer];
    char *  args[kMaxArgs];
    uint8_t argCount = 0;

    VerifyOrExit(StringLength(aString, kMaxCommandBuffer) < kMaxCommandBuffer, error = kErrorNoBufs);

    strcpy(buffer, aString);
    argCount = kMaxArgs;
    error    = ParseCmd(buffer, argCount, args);

exit:

    switch (error)
    {
    case kErrorNone:
        aOutput[0] = '\0'; // In case there is no output.
        IgnoreError(ProcessCmd(argCount, &args[0], aOutput, aOutputMaxLen));
        break;

    case kErrorNoBufs:
        snprintf(aOutput, aOutputMaxLen, "failed: command string too long\r\n");
        break;

    case kErrorInvalidArgs:
        snprintf(aOutput, aOutputMaxLen, "failed: command string contains too many arguments\r\n");
        break;

    default:
        snprintf(aOutput, aOutputMaxLen, "failed to parse command string\r\n");
        break;
    }
}

Error RPC::ParseCmd(char *aString, uint8_t &aArgsLength, char *aArgs[])
{
    Error                     error;
    Utils::CmdLineParser::Arg args[kMaxArgs];

    // Parse command string to a string array
    SuccessOrExit(error = Utils::CmdLineParser::ParseCmd(aString, args, aArgsLength));
    Utils::CmdLineParser::Arg::CopyArgsToStringArray(args, aArgs);

    // Get number of args parsed
    aArgsLength = Arg::GetArgsLength(args);

exit:
    return error;
}

#if OPENTHREAD_COPROCESSOR

Error RPC::ProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    Error error = kErrorInvalidCommand;
    VerifyOrExit(aArgsLength > 0);

    aOutput[0] = '\0';

    SetOutputBuffer(aOutput, aOutputMaxLen);

    // Check built-in commands
    VerifyOrExit(kErrorNone !=
                 (error = HandleCommand(NULL, aArgsLength, aArgs, OT_ARRAY_LENGTH(sCommands), sCommands)));

    // Check user commands
    mUserCommandsError = OT_ERROR_NONE;

    error = HandleCommand(mUserCommandsContext, aArgsLength, aArgs, mUserCommandsLength, mUserCommands);

    if (kErrorNone == error)
    {
        // User command executed. Check for error
        error = mUserCommandsError;
    }


exit:
    if (OT_ERROR_NONE != error)
    {
        OutputResult(error);
    }
    ClearOutputBuffer();
    return error;
}

#else

Error RPC::ProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    Error error = kErrorInvalidCommand;
    VerifyOrExit(aArgsLength > 0);

    aOutput[0] = '\0';

    for (uint8_t i = 0; i < mCachedCommandsLength; i++)
    {
        Arg &command = mCachedCommands[i];
        if (command == aArgs[0])
        {
            // more platform specific features will be processed under platform layer
            SuccessOrExit(error = otPlatCRPCProcess(&GetInstance(), aArgsLength, aArgs, aOutput, aOutputMaxLen));
            ExitNow();
        }
    }

exit:
    // Add more platform specific features here.
    if (error == kErrorInvalidCommand && aArgsLength > 1)
    {
        snprintf(aOutput, aOutputMaxLen, "feature '%s' is not supported\r\n", aArgs[0]);
    }

    return error;
}
#endif

Error RPC::HandleCommand(void *        aContext,
                         uint8_t       aArgsLength,
                         char *        aArgs[],
                         uint8_t       aCommandsLength,
                         const Command aCommands[])
{

    return otCRPCHandleCommand(aContext, aArgsLength, aArgs, aCommandsLength, aCommands);
}

#if OPENTHREAD_COPROCESSOR

void RPC::OutputBytes(const uint8_t *aBytes, uint16_t aLength)
{
    for (uint16_t i = 0; i < aLength; i++)
    {
        OutputFormat("%02x", aBytes[i]);
    }
}

void RPC::OutputCommands(const Command aCommands[], size_t aCommandsLength)
{
    VerifyOrExit(aCommands != NULL);

    for (size_t i = 0; i < aCommandsLength; i++)
    {
        OutputFormat("%s\n", aCommands[i].mName);
    }

exit:
    return;
}

int RPC::OutputFormat(const char *aFormat, ...)
{
    int     rval;
    va_list ap;

    va_start(ap, aFormat);
    rval = OutputFormatV(aFormat, ap);
    va_end(ap);

    return rval;
}

void RPC::OutputFormat(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list ap;

    OutputSpaces(aIndentSize);

    va_start(ap, aFormat);
    OutputFormatV(aFormat, ap);
    va_end(ap);
}

int RPC::OutputFormatV(const char *aFormat, va_list aArguments)
{
    int rval = 0;
    int remaining = mOutputBufferMaxLen - mOutputBufferCount;

    VerifyOrExit(mOutputBuffer && (remaining > 0));
    rval = vsnprintf(&mOutputBuffer[mOutputBufferCount], remaining, aFormat, aArguments);
    if (rval > 0)
    {
        mOutputBufferCount += static_cast<size_t>(rval);
    }
exit:
    return rval;
}

int RPC::OutputIp6Address(const otIp6Address &aAddress)
{
    return OutputFormat(
        "%x:%x:%x:%x:%x:%x:%x:%x", HostSwap16(aAddress.mFields.m16[0]), HostSwap16(aAddress.mFields.m16[1]),
        HostSwap16(aAddress.mFields.m16[2]), HostSwap16(aAddress.mFields.m16[3]), HostSwap16(aAddress.mFields.m16[4]),
        HostSwap16(aAddress.mFields.m16[5]), HostSwap16(aAddress.mFields.m16[6]), HostSwap16(aAddress.mFields.m16[7]));
}

void RPC::OutputLine(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputFormat("\r\n");
}

void RPC::OutputLine(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list args;

    OutputSpaces(aIndentSize);

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputFormat("\r\n");
}

void RPC::OutputResult(otError aError)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        OutputLine("Done");
        break;

    case OT_ERROR_PENDING:
        break;

    default:
        OutputLine("Error %d: %s", aError, otThreadErrorToString(aError));
    }
}

void RPC::OutputSpaces(uint8_t aCount)
{
    char format[sizeof("%256s")];

    snprintf(format, sizeof(format), "%%%us", aCount);

    OutputFormat(format, "");
}

void RPC::ProcessHelp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    OutputCommands(sCommands, OT_ARRAY_LENGTH(sCommands));
    OutputCommands(mUserCommands, mUserCommandsLength);
}

void RPC::SetOutputBuffer(char *aOutput, size_t aOutputMaxLen)
{
    mOutputBuffer       = aOutput;
    mOutputBufferMaxLen = aOutputMaxLen;
    mOutputBufferCount  = 0;
}

void RPC::ClearOutputBuffer(void)
{
    mOutputBuffer       = nullptr;
    mOutputBufferMaxLen = 0;
}

void RPC::SetUserCommands(const otCliCommand *aCommands, uint8_t aLength, void *aContext)
{
    mUserCommands        = aCommands;
    mUserCommandsLength  = aLength;
    mUserCommandsContext = aContext;
}

void RPC::SetUserCommandError(otError aError)
{
    mUserCommandsError = aError;
}


#endif
} // namespace Coprocessor
} // namespace ot

#endif // OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE
