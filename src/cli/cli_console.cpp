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
 *   This file implements the CLI server on the CONSOLE service.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cli/cli.hpp>
#include <cli/cli_console.hpp>
#include <common/new.hpp>

namespace Thread {
namespace Cli {

static Console *sServer;

static otDEFINE_ALIGNED_VAR(sCliConsoleRaw, sizeof(Console), uint64_t);

extern "C" void otCliConsoleInit(otInstance *aInstance, otCliConsoleOutputCallback aCallback, void *aContext)
{
    sServer = new(&sCliConsoleRaw) Console(aInstance);
    sServer->SetOutputCallback(aCallback);
    sServer->SetContext(aContext);
}

extern "C" void otCliConsoleInputLine(char *aBuf, uint16_t aBufLength)
{
    sServer->ReceiveTask(aBuf, aBufLength);
}

Console::Console(otInstance *aInstance)
    : mCallback(NULL),
      mInterpreter(aInstance)
{

}

void Console::SetContext(void *aContext)
{
    mContext = aContext;
}

void Console::SetOutputCallback(otCliConsoleOutputCallback aCallback)
{
    mCallback = aCallback;
}

void Console::ReceiveTask(char *aBuf, uint16_t aBufLength)
{
    mInterpreter.ProcessLine(aBuf, aBufLength, *this);
}

int Console::Output(const char *aBuf, uint16_t aBufLength)
{
    return mCallback(aBuf, aBufLength, mContext);
}

int Console::OutputFormat(const char *fmt, ...)
{
    char buf[kMaxLineLength];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return Output(buf, static_cast<uint16_t>(strlen(buf)));
}

}  // namespace Cli
}  // namespace Thread
