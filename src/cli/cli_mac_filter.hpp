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
 *   This file contains definitions for CLI to MAC Filter.
 */

#ifndef CLI_MAC_FILTER_HPP_
#define CLI_MAC_FILTER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE

#include <openthread/link.h>

#include "cli/cli_config.h"
#include "cli/cli_utils.hpp"

namespace ot {
namespace Cli {

/**
 * Implements the MAC Filter CLI interpreter.
 */
class MacFilter : private Utils
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     */
    MacFilter(otInstance *aInstance, OutputImplementer &aOutputImplementer)
        : Utils(aInstance, aOutputImplementer)
    {
    }

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
     */
    otError Process(Arg aArgs[]);

private:
    static constexpr uint8_t kIndentSize = 4;

    using Command = CommandEntry<MacFilter>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    // For use as input to  `OutputFilter()`
    static constexpr uint8_t kAddressFilter = (1U << 0);
    static constexpr uint8_t kRssFilter     = (1U << 1);

    void               OutputFilter(uint8_t aFilters);
    void               OutputEntry(const otMacFilterEntry &aEntry);
    static bool        IsDefaultRss(const otExtAddress &aExtAddress);
    static const char *AddressModeToString(otMacFilterAddressMode aMode);
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CLI_DNS_ENABLE

#endif // CLI_MAC_FILTER_HPP_
