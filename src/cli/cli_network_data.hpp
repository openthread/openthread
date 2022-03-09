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
 * This class implements the Network Data CLI.
 *
 */
class NetworkData : private OutputWrapper
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
     * @param[in]  aOutput The CLI console output context
     *
     */
    explicit NetworkData(Output &aOutput)
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

    /**
     * This method outputs the prefix config.
     *
     * @param[in]  aConfig  The prefix config.
     *
     */
    void OutputPrefix(const otBorderRouterConfig &aConfig);

    /**
     * This method outputs the route config.
     *
     * @param[in]  aConfig  The route config.
     *
     */
    void OutputRoute(const otExternalRouteConfig &aConfig);

    /**
     * This method outputs the service config.
     *
     * @param[in]  aConfig  The service config.
     *
     */
    void OutputService(const otServiceConfig &aConfig);

    /**
     * This method converts the flags from a given prefix config to string.
     *
     * @param[in]  aConfig  The prefix config.
     * @param[out] aString  The string to populate from @a Config flags.
     *
     */
    static void PrefixFlagsToString(const otBorderRouterConfig &aConfig, FlagsString &aString);

    /**
     * This method converts the flags from a given route config to string.
     *
     * @param[in]  aConfig  The route config.
     * @param[out] aString  The string to populate from @a Config flags.
     *
     */
    static void RouteFlagsToString(const otExternalRouteConfig &aConfig, FlagsString &aString);

    /**
     * This static method converts a route preference value to human-readable string.
     *
     * @param[in] aPreference   The preference value to convert (`OT_ROUTE_PREFERENCE_*` values).
     *
     * @returns A string representation @p aPreference.
     *
     */
    static const char *PreferenceToString(signed int aPreference);

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
};

} // namespace Cli
} // namespace ot

#endif // CLI_NETWORK_DATA_HPP_
