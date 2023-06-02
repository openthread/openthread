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
 *   The file implements POSIX system utilities.
 */

#include "utils.hpp"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <openthread/logging.h>

#include "common/code_utils.hpp"

namespace ot {
namespace Posix {

enum
{
    kSystemCommandMaxLength = 1024, ///< Max length of a system call command.
    kOutputBufferSize       = 1024, ///< Buffer size of command output.
};

otError ExecuteCommand(const char *aFormat, ...)
{
    char    cmd[kSystemCommandMaxLength];
    char    buf[kOutputBufferSize];
    va_list args;
    FILE *  file;
    int     exitCode;
    otError error = OT_ERROR_NONE;

    va_start(args, aFormat);
    vsnprintf(cmd, sizeof(cmd), aFormat, args);
    va_end(args);

    file = popen(cmd, "r");
    VerifyOrExit(file != nullptr, error = OT_ERROR_FAILED);
    while (fgets(buf, sizeof(buf), file))
    {
        size_t length = strlen(buf);
        if (buf[length] == '\n')
        {
            buf[length] = '\0';
        }
        otLogInfoPlat("%s", buf);
    }
    exitCode = pclose(file);
    otLogInfoPlat("Execute command `%s` = %d", cmd, exitCode);
    VerifyOrExit(exitCode == 0, error = OT_ERROR_FAILED);
exit:
    if (error != OT_ERROR_NONE && errno != 0)
    {
        otLogInfoPlat("Got an error when executing command `%s`: `%s`", cmd, strerror(errno));
    }
    return error;
}

} // namespace Posix
} // namespace ot
