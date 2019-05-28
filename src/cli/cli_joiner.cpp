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
 *   This file implements a simple CLI for the Joiner role.
 */

#include "cli_joiner.hpp"

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

#if OPENTHREAD_ENABLE_JOINER

namespace ot {
namespace Cli {

const struct Joiner::Command Joiner::sCommands[] = {
    {"help", &Joiner::ProcessHelp},
    {"id", &Joiner::ProcessId},
    {"start", &Joiner::ProcessStart},
    {"stop", &Joiner::ProcessStop},
};

otError Joiner::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Joiner::ProcessId(int argc, char *argv[])
{
    otError      error;
    otExtAddress joinerId;

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    SuccessOrExit(error = otJoinerGetId(mInterpreter.mInstance, &joinerId));

    mInterpreter.OutputBytes(joinerId.m8, sizeof(joinerId));
    mInterpreter.mServer->OutputFormat("\r\n");

exit:
    return error;
}

otError Joiner::ProcessStart(int argc, char *argv[])
{
    otError     error;
    const char *provisioningUrl = NULL;

    VerifyOrExit(argc > 1, error = OT_ERROR_INVALID_ARGS);

    if (argc > 2)
    {
        provisioningUrl = argv[2];
    }

    error = otJoinerStart(mInterpreter.mInstance, argv[1], provisioningUrl, PACKAGE_NAME,
                          OPENTHREAD_CONFIG_PLATFORM_INFO, PACKAGE_VERSION, NULL, &Joiner::HandleCallback, this);

exit:
    return error;
}

otError Joiner::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otJoinerStop(mInterpreter.mInstance);
}

otError Joiner::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
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

void Joiner::HandleCallback(otError aError, void *aContext)
{
    static_cast<Joiner *>(aContext)->HandleCallback(aError);
}

void Joiner::HandleCallback(otError aError)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        mInterpreter.mServer->OutputFormat("Join success\r\n");
        break;

    default:
        mInterpreter.mServer->OutputFormat("Join failed [%s]\r\n", otThreadErrorToString(aError));
        break;
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_JOINER
