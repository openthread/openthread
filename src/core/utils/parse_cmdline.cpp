/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the command line parser.
 */

#include "parse_cmdline.hpp"

#include "common/code_utils.hpp"

namespace ot {
namespace Utils {

static bool IsSpaceOrNewLine(char aChar)
{
    return (aChar == ' ') || (aChar == '\t') || (aChar == '\r') || (aChar == '\n');
}

otError CmdLineParser::ParseCmd(char *aString, uint8_t &aArgc, char *aArgv[], uint8_t aArgcMax)
{
    otError error = OT_ERROR_NONE;
    char *  cmd;

    aArgc = 0;

    for (cmd = aString; IsSpaceOrNewLine(*cmd) && *cmd; cmd++)
        ;

    if (*cmd)
    {
        aArgv[aArgc++] = cmd++; // the first argument
    }

    for (; *cmd; cmd++)
    {
        if (IsSpaceOrNewLine(*cmd))
        {
            *cmd = '\0';
        }
        else if (*(cmd - 1) == '\0')
        {
            VerifyOrExit(aArgc < aArgcMax, error = OT_ERROR_INVALID_ARGS);
            aArgv[aArgc++] = cmd;
        }
    }

exit:
    return error;
}

} // namespace Utils
} // namespace ot
