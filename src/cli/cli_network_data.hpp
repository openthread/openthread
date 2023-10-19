/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file contains definitions for Network Data CLI commands.
 */

#ifndef CLI_NETWORK_DATA_HPP_
#define CLI_NETWORK_DATA_HPP_

#include "openthread-core-config.h"

#include <openthread/netdata.h>

#include "cli/cli_output.hpp"

namespace ot {
namespace Cli {

/**
 * Implements the Network Data CLI.
 *
 */
class NetworkData : private Output
{
public:
    typedef Utils::CmdLineParser::Arg Arg;

    /**
     * This constant specifies the string size for representing Network Data prefix/route entry flags.
     *
     * BorderRouter (OnMeshPrefix) TLV uses `uint16_t` for its flags and ExternalRoute uses `uint8_t`, though some of
     * the bits are not currently used and reserved for future, so 17 chars string (16 flags plus null char at end of
     * string) covers current and future flags.
     *
     */
    static constexpr uint16_t kFlagsStringSize = 17;

    typedef char FlagsString[kFlagsStringSize]; ///< Flags String type (char array of `kFlagsStringSize`).

    /**
     * Constructor
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     *
     */
    NetworkData(otInstance *aInstance, OutputImplementer &aOutputImplementer);

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

    /**
     * Outputs the prefix config.
     *
     * @param[in]  aConfig  The prefix config.
     *
     */
    void OutputPrefix(const otBorderRouterConfig &aConfig);

    /**
     * Outputs the route config.
     *
     * @param[in]  aConfig  The route config.
     *
     */
    void OutputRoute(const otExternalRouteConfig &aConfig);

    /**
     * Outputs the service config.
     *
     * @param[in]  aConfig  The service config.
     *
     */
    void OutputService(const otServiceConfig &aConfig);

    /**
     * Converts the flags from a given prefix config to string.
     *
     * @param[in]  aConfig  The prefix config.
     * @param[out] aString  The string to populate from @a Config flags.
     *
     */
    static void PrefixFlagsToString(const otBorderRouterConfig &aConfig, FlagsString &aString);

    /**
     * Converts the flags from a given route config to string.
     *
     * @param[in]  aConfig  The route config.
     * @param[out] aString  The string to populate from @a Config flags.
     *
     */
    static void RouteFlagsToString(const otExternalRouteConfig &aConfig, FlagsString &aString);

private:
    using Command = CommandEntry<NetworkData>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError GetNextPrefix(otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig, bool aLocal);
    otError GetNextRoute(otNetworkDataIterator *aIterator, otExternalRouteConfig *aConfig, bool aLocal);
    otError GetNextService(otNetworkDataIterator *aIterator, otServiceConfig *aConfig, bool aLocal);

    otError OutputBinary(bool aLocal);
    void    OutputPrefixes(bool aLocal);
    void    OutputRoutes(bool aLocal);
    void    OutputServices(bool aLocal);
    void    OutputLowpanContexts(bool aLocal);
    void    OutputCommissioningDataset(bool aLocal);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    static void HandleNetdataFull(void *aContext) { static_cast<NetworkData *>(aContext)->HandleNetdataFull(); }
    void        HandleNetdataFull(void) { mFullCallbackWasCalled = true; }

    bool mFullCallbackWasCalled;
#endif
};

} // namespace Cli
} // namespace ot

#endif // CLI_NETWORK_DATA_HPP_
