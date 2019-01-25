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
 *   This file contains definitions for the CLI interpreter.
 */

#ifndef CLI_DATASET_HPP_
#define CLI_DATASET_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>

#include <openthread/dataset.h>

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the Dataset CLI interpreter.
 *
 */
class Dataset
{
public:
    Dataset(Interpreter &aInterpreter)
        : mInterpreter(aInterpreter)
    {
    }

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

private:
    struct Command
    {
        const char *mName;
        otError (Dataset::*mCommand)(int argc, char *argv[]);
    };

    void    OutputBytes(const uint8_t *aBytes, uint8_t aLength);
    otError Print(otOperationalDataset &aDataset);

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessActive(int argc, char *argv[]);
    otError ProcessActiveTimestamp(int argc, char *argv[]);
    otError ProcessChannel(int argc, char *argv[]);
    otError ProcessChannelMask(int argc, char *argv[]);
    otError ProcessClear(int argc, char *argv[]);
    otError ProcessCommit(int argc, char *argv[]);
    otError ProcessDelay(int argc, char *argv[]);
    otError ProcessExtPanId(int argc, char *argv[]);
    otError ProcessMasterKey(int argc, char *argv[]);
    otError ProcessMeshLocalPrefix(int argc, char *argv[]);
    otError ProcessNetworkName(int argc, char *argv[]);
    otError ProcessPanId(int argc, char *argv[]);
    otError ProcessPending(int argc, char *argv[]);
    otError ProcessPendingTimestamp(int argc, char *argv[]);
    otError ProcessMgmtSetCommand(int argc, char *argv[]);
    otError ProcessMgmtGetCommand(int argc, char *argv[]);
    otError ProcessPSKc(int argc, char *argv[]);
    otError ProcessSecurityPolicy(int argc, char *argv[]);

    Interpreter &mInterpreter;

    static const Command        sCommands[];
    static otOperationalDataset sDataset;
};

} // namespace Cli
} // namespace ot

#endif // CLI_DATASET_HPP_
