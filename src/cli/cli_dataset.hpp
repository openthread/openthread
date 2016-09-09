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
    const char *mName;                                                      ///< A pointer to the command string.
    ThreadError(*mCommand)(otInstance *aInstance, int argc,
                           char *argv[]);    ///< A function pointer to process the command.
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
    static ThreadError Process(otInstance *aInstance, int argc, char *argv[], Server &aServer);

private:
    static void OutputBytes(const uint8_t *aBytes, uint8_t aLength);
    static ThreadError Print(otOperationalDataset &aDataset);

    static ThreadError ProcessHelp(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessActive(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessActiveTimestamp(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessChannel(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessChannelMask(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessClear(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessCommit(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessDelay(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessExtPanId(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessMasterKey(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessMeshLocalPrefix(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessNetworkName(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessPanId(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessPending(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessPendingTimestamp(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessMgmtSetCommand(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessMgmtGetCommand(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessPSKc(otInstance *aInstance, int argc, char *argv[]);
    static ThreadError ProcessSecurityPolicy(otInstance *aInstance, int argc, char *argv[]);

    static const DatasetCommand sCommands[];
    static otOperationalDataset sDataset;
    static Server *sServer;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_DATASET_HPP_
