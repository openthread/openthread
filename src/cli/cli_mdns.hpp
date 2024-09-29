/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef CLI_MDNS_HPP_
#define CLI_MDNS_HPP_

#include "openthread-core-config.h"

#include <openthread/mdns.h>

#include "cli/cli_config.h"
#include "cli/cli_utils.hpp"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE

namespace ot {
namespace Cli {

/**
 * Implements the mDNS CLI interpreter.
 */
class Mdns : private Utils
{
public:
    /**
     * Constructor.
     *
     * @param[in]  aInstance            The OpenThread Instance.
     * @param[in]  aOutputImplementer   An `OutputImplementer`.
     */
    Mdns(otInstance *aInstance, OutputImplementer &aOutputImplementer)
        : Utils(aInstance, aOutputImplementer)
        , mInfraIfIndex(0)
        , mRequestId(0)
        , mWaitingForCallback(false)
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
    using Command = CommandEntry<Mdns>;

    static constexpr uint8_t  kIndentSize     = 4;
    static constexpr uint16_t kMaxAddresses   = 16;
    static constexpr uint16_t kStringSize     = 400;
    static constexpr uint16_t kMaxSubTypes    = 8;
    static constexpr uint16_t kMaxTxtDataSize = 200;
    static constexpr uint16_t kMaxKeyDataSize = 200;

    enum IpAddressType : uint8_t
    {
        kIp6Address,
        kIp4Address,
    };

    struct Buffers // Used to populate `otMdnsService` field
    {
        char        mString[kStringSize];
        const char *mSubTypeLabels[kMaxSubTypes];
        uint8_t     mTxtData[kMaxTxtDataSize];
    };

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    void    OutputHost(const otMdnsHost &aHost);
    void    OutputService(const otMdnsService &aService);
    void    OutputKey(const otMdnsKey &aKey);
    void    OutputState(otMdnsEntryState aState);
    void    OutputCacheInfo(const otMdnsCacheInfo &aInfo);
    otError ProcessRegisterHost(Arg aArgs[]);
    otError ProcessRegisterService(Arg aArgs[]);
    otError ProcessRegisterKey(Arg aArgs[]);
    void    HandleRegisterationDone(otMdnsRequestId aRequestId, otError aError);
    void    HandleBrowseResult(const otMdnsBrowseResult &aResult);
    void    HandleSrvResult(const otMdnsSrvResult &aResult);
    void    HandleTxtResult(const otMdnsTxtResult &aResult);
    void    HandleAddressResult(const otMdnsAddressResult &aResult, IpAddressType aType);

    static otError ParseStartOrStop(const Arg &aArg, bool &aIsStart);
    static void    HandleRegisterationDone(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError);
    static void    HandleBrowseResult(otInstance *aInstance, const otMdnsBrowseResult *aResult);
    static void    HandleSrvResult(otInstance *aInstance, const otMdnsSrvResult *aResult);
    static void    HandleTxtResult(otInstance *aInstance, const otMdnsTxtResult *aResult);
    static void    HandleIp6AddressResult(otInstance *aInstance, const otMdnsAddressResult *aResult);
    static void    HandleIp4AddressResult(otInstance *aInstance, const otMdnsAddressResult *aResult);

    static otError ParseServiceArgs(Arg aArgs[], otMdnsService &aService, Buffers &aBuffers);

    uint32_t        mInfraIfIndex;
    otMdnsRequestId mRequestId;
    bool            mWaitingForCallback;
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE

#endif // CLI_MDNS_HPP_
