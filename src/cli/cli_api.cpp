/*
 *  Copyright (c) 2016-2026, The OpenThread Authors.
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
 *   This file implements the CLI public APIs.
 */

#include <openthread/cli.h>

#include "cli.hpp"

namespace ot {
namespace Cli {

extern "C" size_t otCliInterpreterGetSize(void) { return Interpreter::GetSize(); }

extern "C" otCliInterpreter *otCliInterpreterInit(void               *aBuffer,
                                                  size_t              aSize,
                                                  otInstance         *aInstance,
                                                  otCliOutputCallback aCallback,
                                                  void               *aContext)
{
    return Interpreter::Init(aBuffer, aSize, aInstance, aCallback, aContext);
}

#if OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
extern "C" void otCliInterpreterSetPromptConfig(otCliInterpreter *aInterpreter, bool aEnable)
{
    static_cast<Interpreter *>(aInterpreter)->SetPromptConfig(aEnable);
}
#endif

extern "C" void otCliInterpreterInputLine(otCliInterpreter *aInterpreter, char *aLine)
{
    static_cast<Interpreter *>(aInterpreter)->ProcessLine(aLine);
}

extern "C" void otCliInterpreterFinalize(otCliInterpreter *aInterpreter)
{
    static_cast<Interpreter *>(aInterpreter)->Finalize();
}

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_CLI_STATIC_INTERPRETER_ENABLE

extern "C" void otCliInit(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext)
{
    Interpreter::Init(aInstance, aCallback, aContext);

#if OPENTHREAD_CONFIG_CLI_VENDOR_COMMANDS_ENABLE && OPENTHREAD_CONFIG_CLI_MAX_USER_CMD_ENTRIES > 1
    otCliVendorSetUserCommands();
#endif
}

extern "C" otCliInterpreter *otCliGetStaticInterpreter(void) { return &Interpreter::GetInterpreter(); }

extern "C" void otCliInputLine(char *aLine) { Interpreter::GetInterpreter().ProcessLine(aLine); }

extern "C" otError otCliSetUserCommands(const otCliCommand *aUserCommands, uint8_t aLength, void *aContext)
{
    return Interpreter::GetInterpreter().SetUserCommands(aUserCommands, aLength, aContext);
}

extern "C" void otCliOutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    Interpreter::GetInterpreter().OutputBytes(aBytes, aLength);
}

extern "C" void otCliOutputFormat(const char *aFmt, ...)
{
    va_list args;

    va_start(args, aFmt);
    Interpreter::GetInterpreter().OutputFormatV(aFmt, args);
    va_end(args);
}

extern "C" void otCliAppendResult(otError aError) { Interpreter::GetInterpreter().OutputResult(aError); }

extern "C" void otCliPlatLogv(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, va_list aArgs)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);

    VerifyOrExit(Interpreter::IsInitialized());

    // CLI output is being used for logging, so we set the flag
    // `EmittingCommandOutput` to false indicate this.
    Interpreter::GetInterpreter().SetEmittingCommandOutput(false);
    Interpreter::GetInterpreter().OutputFormatV(aFormat, aArgs);
    Interpreter::GetInterpreter().OutputNewLine();
    Interpreter::GetInterpreter().SetEmittingCommandOutput(true);

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_CLI_STATIC_INTERPRETER_ENABLE

} // namespace Cli
} // namespace ot
