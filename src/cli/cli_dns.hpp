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
 *   This file contains definitions for CLI to DNS (client and resolver).
 */

#ifndef CLI_DNS_HPP_
#define CLI_DNS_HPP_

#include "openthread-core-config.h"

#ifdef OPENTHREAD_CLI_DNS_ENABLE
#error "OPENTHREAD_CLI_DNS_ENABLE MUST not be defined directly, it is derived from `OPENTHREAD_CONFIG_*` configs".
#endif

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE || OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE || \
    OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#define OPENTHREAD_CLI_DNS_ENABLE 1
#else
#define OPENTHREAD_CLI_DNS_ENABLE 0
#endif

#if OPENTHREAD_CLI_DNS_ENABLE

#include <openthread/dns.h>
#include <openthread/dns_client.h>
#include <openthread/dnssd_server.h>

#include "cli/cli_config.h"
#include "cli/cli_output.hpp"

namespace ot {
namespace Cli {

/**
 * Implements the DNS CLI interpreter.
 *
 */
class Dns : private Output
{
public:
    typedef Utils::CmdLineParser::Arg Arg;

    /**
     * Constructor.
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     *
     */
    Dns(otInstance *aInstance, OutputImplementer &aOutputImplementer)
        : Output(aInstance, aOutputImplementer)
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
     *
     */
    otError Process(Arg aArgs[]);

private:
    static constexpr uint8_t  kIndentSize     = 4;
    static constexpr uint16_t kMaxTxtDataSize = OPENTHREAD_CONFIG_CLI_TXT_RECORD_MAX_SIZE;

    using Command = CommandEntry<Dns>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

#if OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE
    void        OutputResult(otError aError);
    otError     GetDnsConfig(Arg aArgs[], otDnsQueryConfig *&aConfig);
    const char *DnsConfigServiceModeToString(otDnsServiceMode aMode) const;
    otError     ParseDnsServiceMode(const Arg &aArg, otDnsServiceMode &aMode) const;
    static void HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse, void *aContext);
    void        HandleDnsAddressResponse(otError aError, const otDnsAddressResponse *aResponse);
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    static const char *const kServiceModeStrings[];

    typedef otError (&ResolveServiceFn)(otInstance *,
                                        const char *,
                                        const char *,
                                        otDnsServiceCallback,
                                        void *,
                                        const otDnsQueryConfig *);

    otError     ProcessService(Arg aArgs[], ResolveServiceFn aResolveServiceFn);
    void        OutputDnsServiceInfo(uint8_t aIndentSize, const otDnsServiceInfo &aServiceInfo);
    static void HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse, void *aContext);
    void        HandleDnsBrowseResponse(otError aError, const otDnsBrowseResponse *aResponse);
    static void HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse, void *aContext);
    void        HandleDnsServiceResponse(otError aError, const otDnsServiceResponse *aResponse);
#endif
#endif
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CLI_DNS_ENABLE

#endif // CLI_DNS_HPP_
