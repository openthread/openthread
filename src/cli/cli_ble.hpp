/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file defines a simple CLI Bluetooth LE command set.
 */

#ifndef CLI_BLE_HPP_
#define CLI_BLE_HPP_

#include "openthread-core-config.h"

#include <common/instance.hpp>
#include <openthread/cli.h>
#include <openthread/error.h>

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements a CLI-based Bluetooth LE example.
 *
 */
class Ble
{
public:
    /**
     * Constructor for ot::Cli::Ble command processor.
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    Ble(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc    The number of elements in argv.
     * @param[in]  argv    A pointer to an array of command line arguments.
     * @param[in]  aServer Server object to output to.
     *
     */
    otError Process(otInstance *aInstance, int argc, char *argv[]);

private:
    struct Command
    {
        const char *mName;
        otError (Ble::*mCommand)(int argc, char *argv[]);
    };

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessStart(int argc, char *argv[]);
    otError ProcessStop(int argc, char *argv[]);
    otError ProcessReset(int argc, char *argv[]);

    otError ProcessAdvertise(int argc, char *argv[]);
    otError ProcessScan(int argc, char *argv[]);
    otError ProcessConnect(int argc, char *argv[]);

    otError ProcessMtu(int argc, char *argv[]);
    otError ProcessBdAddr(int argc, char *argv[]);

    otError ProcessGatt(int argc, char *argv[]);

    otError ProcessChannel(int argc, char *argv[]);

    Interpreter &mInterpreter;

    static const Command sCommands[];
};

} // namespace Cli
} // namespace ot

#endif // CLI_BLE_HPP_
