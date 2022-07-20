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
 *   This file implements the OpenThread Co-processor RPC (CRPC) API.
 */

#include "openthread-core-config.h"

#include <openthread/coprocessor_rpc.h>

#include "common/error.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"

namespace ot {
namespace Coprocessor {

extern "C" Error otCRPCHandleCommand(void *             aContext,
                                     uint8_t            aArgsLength,
                                     char *             aArgs[],
                                     uint8_t            aCommandsLength,
                                     const otCliCommand aCommands[])
{
    Error error = kErrorInvalidCommand;

    VerifyOrExit(aArgs && aArgsLength != 0 && aCommands && aCommandsLength != 0);

    for (size_t i = 0; i < aCommandsLength; i++)
    {
        if (strcmp(aArgs[0], aCommands[i].mName) == 0)
        {
            // Command found, call command handler
            (aCommands[i].mCommand)(aContext, aArgsLength - 1, (aArgsLength > 1) ? &aArgs[1] : nullptr);
            error = kErrorNone;
            ExitNow();
        }
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE
#if OPENTHREAD_COPROCESSOR
extern "C" void otCliAppendResult(otError aError)
{
    RPC::GetRPC().OutputResult(aError);
}

extern "C" void otCliSetUserCommandError(otError aError)
{
    RPC::GetRPC().SetUserCommandError(aError);
}

extern "C" void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    RPC::GetRPC().OutputBytes(aBytes, aLength);
}

extern "C" void otCliOutputCommands(const otCliCommand aCommands[], size_t aCommandsLength)
{
    RPC::GetRPC().OutputCommands(aCommands, aCommandsLength);
}

extern "C" void otCliOutputFormat(const char *aFmt, ...)
{
    va_list aAp;
    va_start(aAp, aFmt);
    RPC::GetRPC().OutputFormatV(aFmt, aAp);
    va_end(aAp);
}

extern "C" int otCliOutputIp6Address(const otIp6Address *aAddress)
{
    const otIp6Address &address = *aAddress;

    return RPC::GetRPC().OutputIp6Address(address);
}
#endif

extern "C" void otCRPCProcessCmdLine(const char *aString, char *aOutput, size_t aOutputMaxLen)
{
    RPC::GetRPC().ProcessLine(aString, aOutput, aOutputMaxLen);
}

extern "C" otError otCRPCProcessCmd(uint8_t aArgsLength, char *aArgs[], char *aOutput, size_t aOutputMaxLen)
{
    return RPC::GetRPC().ProcessCmd(aArgsLength, aArgs, aOutput, aOutputMaxLen);
}

#if OPENTHREAD_COPROCESSOR
extern "C" void otCRPCProcessHelp(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    RPC::GetRPC().ProcessHelp(aContext, aArgsLength, aArgs);
}

extern "C" void otCRPCSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength, void *aContext)
{
    RPC::GetRPC().SetUserCommands(aUserCommands, aLength, aContext);
}
#endif

#endif // OPENTHREAD_CONFIG_COPROCESSOR_RPC_ENABLE

} // namespace Coprocessor
} // namespace ot
