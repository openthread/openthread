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
 *   This file implements cli BLE.
 *
 */

#include <openthread/platform/ble-host.h>

#include "cli_ble.hpp"
#include "cli_server.hpp"
#include "cli/cli.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"

namespace ot {
namespace Cli {

const struct Ble::Command Ble::sCommands[] = {{"help", &Ble::ProcessHelp},
                                              {"init", &Ble::ProcessInit},
                                              {"deinit", &Ble::ProcessDeinit}};

Ble::Ble(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
}

otError Ble::Process(int argc, char *argv[])
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
                error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
                break;
            }
        }
    }
    return error;
}

otError Ble::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Ble::ProcessInit(int argc, char *argv[])
{
    otError error;

    OT_UNUSED_VARIABLE(argv);

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    error = otBleHostInit();

exit:
    return error;
}

otError Ble::ProcessDeinit(int argc, char *argv[])
{
    otError error;

    OT_UNUSED_VARIABLE(argv);

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    error = otBleHostDeinit();

exit:
    return error;
}

void Ble::OutputBytes(const uint8_t *aBytes, uint8_t aLength) const
{
    for (int i = 0; i < aLength; i++)
    {
        mInterpreter.mServer->OutputFormat("%02x ", aBytes[i]);
    }
}

} // namespace Cli
} // namespace ot
