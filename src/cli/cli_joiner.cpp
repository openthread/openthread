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

#include <inttypes.h>

#include "cli/cli.hpp"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

namespace ot {
namespace Cli {

const struct Joiner::Command Joiner::sCommands[] = {
    {"discerner", &Joiner::ProcessDiscerner}, {"help", &Joiner::ProcessHelp}, {"id", &Joiner::ProcessId},
    {"start", &Joiner::ProcessStart},         {"stop", &Joiner::ProcessStop},
};

otError Joiner::ProcessDiscerner(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength == 2)
    {
        otJoinerDiscerner discerner;

        memset(&discerner, 0, sizeof(discerner));
        if (strcmp(aArgs[1], "clear") == 0)
        {
            SuccessOrExit(error = otJoinerSetDiscerner(mInterpreter.mInstance, nullptr));
        }
        else
        {
            VerifyOrExit(OT_ERROR_NONE == Interpreter::ParseJoinerDiscerner(aArgs[1], discerner),
                         error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = otJoinerSetDiscerner(mInterpreter.mInstance, &discerner));
        }
    }
    else if (aArgsLength == 1)
    {
        const otJoinerDiscerner *discerner = otJoinerGetDiscerner(mInterpreter.mInstance);

        VerifyOrExit(discerner != nullptr, error = OT_ERROR_NOT_FOUND);

        mInterpreter.OutputLine("0x%" PRIx64 "/%u", discerner->mValue, discerner->mLength);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

otError Joiner::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine("%s", command.mName);
    }

    return OT_ERROR_NONE;
}

otError Joiner::ProcessId(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    const otExtAddress *joinerId;

    joinerId = otJoinerGetId(mInterpreter.mInstance);

    mInterpreter.OutputBytes(joinerId->m8, sizeof(otExtAddress));
    mInterpreter.OutputLine("");

    return OT_ERROR_NONE;
}

otError Joiner::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    otError     error;
    const char *provisioningUrl = nullptr;

    VerifyOrExit(aArgsLength > 1, error = OT_ERROR_INVALID_ARGS);

    if (aArgsLength > 2)
    {
        provisioningUrl = aArgs[2];
    }

    error = otJoinerStart(mInterpreter.mInstance, aArgs[1], provisioningUrl, PACKAGE_NAME,
                          OPENTHREAD_CONFIG_PLATFORM_INFO, PACKAGE_VERSION, nullptr, &Joiner::HandleCallback, this);

exit:
    return error;
}

otError Joiner::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otJoinerStop(mInterpreter.mInstance);

    return OT_ERROR_NONE;
}

otError Joiner::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgsLength < 1)
    {
        IgnoreError(ProcessHelp(0, nullptr));
    }
    else
    {
        for (const Command &command : sCommands)
        {
            if (strcmp(aArgs[0], command.mName) == 0)
            {
                error = (this->*command.mCommand)(aArgsLength, aArgs);
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
        mInterpreter.OutputLine("Join success");
        break;

    default:
        mInterpreter.OutputLine("Join failed [%s]", otThreadErrorToString(aError));
        break;
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE
