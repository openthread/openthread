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
 *   This file implements the top-level interface to diagnostics module.
 */

#include "openthread-core-config.h"

#include <stdio.h>
#include <stdlib.h>
#include "utils/wrap_string.h"

#include <openthread/diag.h>

#include "diag_process.hpp"
#include "common/code_utils.hpp"

using namespace ot::Diagnostics;

void otDiagInit(otInstance *aInstance)
{
    Diag::Init(aInstance);
}

const char *otDiagProcessCmd(int aArgCount, char *aArgVector[])
{
    return Diag::ProcessCmd(aArgCount, aArgVector);
}

static bool IsSpace(char aChar)
{
    return (aChar == ' ') || (aChar == '\t');
}

static bool IsNullOrNewline(char aChar)
{
    return (aChar == 0) || (aChar == '\n') || (aChar == '\r');
}

const char *otDiagProcessCmdLine(const char *aInput)
{
    enum
    {
        kMaxArgs = 32,
        kMaxCommandBuffer = 256,
    };

    otError error = OT_ERROR_NONE;
    char buffer[kMaxCommandBuffer];
    char *argVector[kMaxArgs];
    int argCount = 0;
    char *bufPtr = &buffer[0];
    uint16_t bufLen = sizeof(buffer);
    const char *output = "\r\n";

    while (!IsNullOrNewline(*aInput))
    {
        while (IsSpace(*aInput))
        {
            aInput++;
        }

        argVector[argCount] = bufPtr;

        while (!IsSpace(*aInput) && !IsNullOrNewline(*aInput))
        {
            *bufPtr++ = *aInput++;
            VerifyOrExit(--bufLen > 0, error = OT_ERROR_NO_BUFS);
        }

        if (argVector[argCount] != bufPtr)
        {
            *bufPtr++ = 0;
            VerifyOrExit(--bufLen > 0, error = OT_ERROR_NO_BUFS);

            argCount++;
            VerifyOrExit(argCount < kMaxArgs, error = OT_ERROR_INVALID_ARGS);
        }
    }

exit:

    switch (error)
    {
    case OT_ERROR_NONE:

        if (argCount >= 1)
        {
            output = Diag::ProcessCmd(argCount - 1, (argCount == 1) ? NULL : &argVector[1]);
        }
        else
        {
            output = Diag::ProcessCmd(0, NULL);
        }

        break;

    case OT_ERROR_NO_BUFS:
        output = "failed: command string too long\r\n";
        break;

    case OT_ERROR_INVALID_ARGS:
        output = "failed: command string contains too many arguments\r\n";
        break;

    default:
        output = "failed to parse command string\n\r";
        break;
    }

    return output;
}

bool otDiagIsEnabled(void)
{
    return Diag::IsEnabled();
}
