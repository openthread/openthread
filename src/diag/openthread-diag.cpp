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
#include "utils/parse_cmdline.hpp"
#include "utils/wrap_string.h"

#include <openthread/diag.h>

#include "diag_process.hpp"
#include "common/code_utils.hpp"

using namespace ot::Diagnostics;

void otDiagInit(otInstance *aInstance)
{
    Diag::Init(aInstance);
}

void otDiagProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    Diag::ProcessCmd(aArgCount, aArgVector, aOutput, aOutputMaxLen);
}

void otDiagProcessCmdLine(const char *aInput, char *aOutput, size_t aOutputMaxLen)
{
    enum
    {
        kMaxArgs          = OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX,
        kMaxCommandBuffer = OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE,
    };

    otError error = OT_ERROR_NONE;
    char    buffer[kMaxCommandBuffer];
    char *  argVector[kMaxArgs];
    uint8_t argCount = 0;

    VerifyOrExit(strnlen(aInput, kMaxCommandBuffer) < kMaxCommandBuffer, error = OT_ERROR_NO_BUFS);

    strcpy(buffer, aInput);
    error = ot::Utils::CmdLineParser::ParseCmd(buffer, argCount, argVector, kMaxArgs);

exit:

    switch (error)
    {
    case OT_ERROR_NONE:

        if (argCount >= 1)
        {
            Diag::ProcessCmd(argCount - 1, (argCount == 1) ? NULL : &argVector[1], aOutput, aOutputMaxLen);
        }
        else
        {
            Diag::ProcessCmd(0, NULL, aOutput, aOutputMaxLen);
        }

        break;

    case OT_ERROR_NO_BUFS:
        snprintf(aOutput, aOutputMaxLen, "failed: command string too long\r\n");
        break;

    case OT_ERROR_INVALID_ARGS:
        snprintf(aOutput, aOutputMaxLen, "failed: command string contains too many arguments\r\n");
        break;

    default:
        snprintf(aOutput, aOutputMaxLen, "failed to parse command string\r\n");
        break;
    }
}

bool otDiagIsEnabled(void)
{
    return Diag::IsEnabled();
}
