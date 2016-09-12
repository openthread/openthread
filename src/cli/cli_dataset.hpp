/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdarg.h>

#include <cli/cli_server.hpp>
#include <thread/meshcop_dataset.hpp>

namespace Thread {
namespace Cli {

/**
 * This structure represents a CLI command.
 *
 */
struct DatasetCommand
{
    const char *mName;                                ///< A pointer to the command string.
    ThreadError(*mCommand)(int argc, char *argv[]);   ///< A function pointer to process the command.
};

/**
 * This class implements the CLI interpreter.
 *
 */
class Dataset
{
public:
    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    static ThreadError Process(int argc, char *argv[], Server &aServer);

private:
    static void OutputBytes(const uint8_t *aBytes, uint8_t aLength);
    static ThreadError Print(otOperationalDataset &aDataset);

    static ThreadError ProcessHelp(int argc, char *argv[]);
    static ThreadError ProcessActive(int argc, char *argv[]);
    static ThreadError ProcessActiveTimestamp(int argc, char *argv[]);
    static ThreadError ProcessChannel(int argc, char *argv[]);
    static ThreadError ProcessClear(int argc, char *argv[]);
    static ThreadError ProcessCommit(int argc, char *argv[]);
    static ThreadError ProcessDelay(int argc, char *argv[]);
    static ThreadError ProcessExtPanId(int argc, char *argv[]);
    static ThreadError ProcessMasterKey(int argc, char *argv[]);
    static ThreadError ProcessMeshLocalPrefix(int argc, char *argv[]);
    static ThreadError ProcessNetworkName(int argc, char *argv[]);
    static ThreadError ProcessPanId(int argc, char *argv[]);
    static ThreadError ProcessPending(int argc, char *argv[]);
    static ThreadError ProcessPendingTimestamp(int argc, char *argv[]);
    static ThreadError ProcessMgmtSetCommand(int argc, char *argv[]);
    static ThreadError ProcessMgmtGetCommand(int argc, char *argv[]);
    static ThreadError ProcessPSKc(int argc, char *argv[]);
    static ThreadError ProcessSecurityPolicy(int argc, char *argv[]);

    static const DatasetCommand sCommands[];
    static otOperationalDataset sDataset;
    static Server *sServer;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_DATASET_HPP_
