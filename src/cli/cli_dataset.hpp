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

#include "cli/cli_output.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements the Dataset CLI interpreter.
 *
 */
class Dataset : private OutputWrapper
{
public:
    typedef Utils::CmdLineParser::Arg Arg;

    explicit Dataset(Output &aOutput)
        : OutputWrapper(aOutput)
    {
    }

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgs        An array of command line arguments.
     *
     */
    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<Dataset>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError Print(otOperationalDataset &aDataset);

#if OPENTHREAD_CONFIG_DATASET_UPDATER_ENABLE && OPENTHREAD_FTD
    otError     ProcessUpdater(Arg aArgs[]);
    static void HandleDatasetUpdater(otError aError, void *aContext);
    void        HandleDatasetUpdater(otError aError);
#endif

    void    OutputSecurityPolicy(const otSecurityPolicy &aSecurityPolicy);
    otError ParseSecurityPolicy(otSecurityPolicy &aSecurityPolicy, Arg *&aArgs);

    static otOperationalDataset sDataset;
};

} // namespace Cli
} // namespace ot

#endif // CLI_DATASET_HPP_
