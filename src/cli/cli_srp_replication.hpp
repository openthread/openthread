/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file contains definitions for a CLI to control SRP Replication (SRPL).
 */

#ifndef CLI_SRP_REPLICATION_HPP_
#define CLI_SRP_REPLICATION_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#include <openthread/srp_replication.h>

#include "cli/cli_config.h"
#include "cli/cli_output.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements the SRP Replication CLI interpreter.
 *
 */
class SrpReplication : private Output
{
public:
    typedef Utils::CmdLineParser::Arg Arg;

    /**
     * Constructor
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     *
     */
    SrpReplication(otInstance *aInstance, OutputImplementer &aOutputImplementer);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgs        A pointer an array of command line arguments.
     *
     */
    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<SrpReplication>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    void OutputIdInHexFormat(uint64_t aId);

    static const char *SessionStateToString(otSrpReplicationSessionState aState);
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

#endif // CLI_SRP_REPLICATION_HPP_
