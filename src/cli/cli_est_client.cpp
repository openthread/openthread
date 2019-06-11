/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a simple CLI for the EST Client service.
 */

#include "cli_est_client.hpp"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include <mbedtls/debug.h>
#include <openthread/ip6.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

namespace ot {
namespace Cli {

const struct EstClient::Command EstClient::sCommands[] = {
    {"help", &EstClient::ProcessHelp},         {"start", &EstClient::ProcessStart},
};

EstClient::EstClient(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
}

otError EstClient::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError EstClient::ProcessStart(int argc, char *argv[])
{
    otError error;
    bool    verifyPeerCert = true;

    if (argc > 1)
    {
        if (strcmp(argv[1], "false") == 0)
        {
            verifyPeerCert = false;
        }
        else if (strcmp(argv[1], "true") != 0)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otEstClientStart(mInterpreter.mInstance, verifyPeerCert, verifyPeerCert));

exit:
    return error;
}

otError EstClient::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc, argv);
                break;
            }
        }
    }

    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
