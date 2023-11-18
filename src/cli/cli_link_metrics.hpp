/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file contains definitions for the CLI interpreter for Link Metrics function.
 */

#ifndef CLI_LINK_METRICS_HPP_
#define CLI_LINK_METRICS_HPP_

#include "openthread-core-config.h"

#include <openthread/link_metrics.h>

#include "cli/cli_output.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

namespace ot {
namespace Cli {

/**
 * Implements the Link Metrics CLI interpreter.
 *
 */

class LinkMetrics : private Output
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
    LinkMetrics(otInstance *aInstance, OutputImplementer &aOutputImplementer);

    /**
     * Processes a CLI sub-command.
     *
     * @param[in]  aArgs     An array of command line arguments.
     *
     * @retval OT_ERROR_NONE              Successfully executed the CLI command.
     * @retval OT_ERROR_PENDING           The CLI command was successfully started but final result is pending.
     * @retval OT_ERROR_INVALID_COMMAND   Invalid or unknown CLI command.
     * @retval OT_ERROR_INVALID_ARGS      Invalid arguments.
     * @retval ...                        Error during execution of the CLI command.
     *
     */
    otError Process(Arg aArgs[]);

private:
    static constexpr uint8_t kIndentSize = 4;

    using Command = CommandEntry<LinkMetrics>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ParseLinkMetricsFlags(otLinkMetrics &aLinkMetrics, const Arg &aFlags);

    void PrintLinkMetricsValue(const otLinkMetricsValues *aMetricsValues);

    static void HandleLinkMetricsReport(const otIp6Address        *aAddress,
                                        const otLinkMetricsValues *aMetricsValues,
                                        otLinkMetricsStatus        aStatus,
                                        void                      *aContext);

    void HandleLinkMetricsReport(const otIp6Address        *aAddress,
                                 const otLinkMetricsValues *aMetricsValues,
                                 otLinkMetricsStatus        aStatus);

    static void HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress,
                                              otLinkMetricsStatus aStatus,
                                              void               *aContext);

    void HandleLinkMetricsMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus);

    static void HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                                 const otExtAddress        *aExtAddress,
                                                 const otLinkMetricsValues *aMetricsValues,
                                                 void                      *aContext);

    void HandleLinkMetricsEnhAckProbingIe(otShortAddress             aShortAddress,
                                          const otExtAddress        *aExtAddress,
                                          const otLinkMetricsValues *aMetricsValues);

    const char *LinkMetricsStatusToStr(otLinkMetricsStatus aStatus);

    void OutputResult(otError aError);

    bool mLinkMetricsQueryInProgress;
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#endif // CLI_LINK_METRICS_HPP_
