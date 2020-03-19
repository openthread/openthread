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
    explicit Dataset(Interpreter &aInterpreter)
        : mInterpreter(aInterpreter)
    {
    }

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgsLength  The number of elements in @p aArgs.
     * @param[in]  aArgs        An array of command line arguments.
     *
     */
    otError Process(int aArgsLength, char *aArgs[]);

private:
    struct Command
    {
        const char *mName;
        otError (Dataset::*mCommand)(int aArgsLength, char *aArgs[]);
    };

    void    OutputBytes(const uint8_t *aBytes, uint8_t aLength);
    otError Print(otOperationalDataset &aDataset);

    otError ProcessHelp(int aArgsLength, char *aArgs[]);
    otError ProcessActive(int aArgsLength, char *aArgs[]);
    otError ProcessActiveTimestamp(int aArgsLength, char *aArgs[]);
    otError ProcessChannel(int aArgsLength, char *aArgs[]);
    otError ProcessChannelMask(int aArgsLength, char *aArgs[]);
    otError ProcessClear(int aArgsLength, char *aArgs[]);
    otError ProcessCommit(int aArgsLength, char *aArgs[]);
    otError ProcessDelay(int aArgsLength, char *aArgs[]);
    otError ProcessExtPanId(int aArgsLength, char *aArgs[]);
    otError ProcessInit(int aArgsLength, char *aArgs[]);
    otError ProcessMasterKey(int aArgsLength, char *aArgs[]);
    otError ProcessMeshLocalPrefix(int aArgsLength, char *aArgs[]);
    otError ProcessNetworkName(int aArgsLength, char *aArgs[]);
    otError ProcessPanId(int aArgsLength, char *aArgs[]);
    otError ProcessPending(int aArgsLength, char *aArgs[]);
    otError ProcessPendingTimestamp(int aArgsLength, char *aArgs[]);
    otError ProcessMgmtSetCommand(int aArgsLength, char *aArgs[]);
    otError ProcessMgmtGetCommand(int aArgsLength, char *aArgs[]);
    otError ProcessPskc(int aArgsLength, char *aArgs[]);
    otError ProcessSecurityPolicy(int aArgsLength, char *aArgs[]);

    Interpreter &mInterpreter;

    static const Command        sCommands[];
    static otOperationalDataset sDataset;
};

} // namespace Cli
} // namespace ot

#endif // CLI_DATASET_HPP_
