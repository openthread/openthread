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
 * @brief
 *   This file includes the implementation of OTBR firewall.
 */

#include "firewall.hpp"

#include <string.h>

#include <openthread/logging.h>
#include <openthread/netdata.h>

#include "common/code_utils.hpp"
#include "posix/platform/utils.hpp"

namespace ot {
namespace Posix {

#if defined(__linux__) && OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE

#if !OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || !OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
#error Configurations 'OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE' and 'OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE' are required.
#endif

static const char kIpsetCommand[]             = OPENTHREAD_POSIX_CONFIG_IPSET_BINARY;
static const char kIngressDenySrcIpSet[]      = "otbr-ingress-deny-src";
static const char kIngressDenySrcSwapIpSet[]  = "otbr-ingress-deny-src-swap";
static const char kIngressAllowDstIpSet[]     = "otbr-ingress-allow-dst";
static const char kIngressAllowDstSwapIpSet[] = "otbr-ingress-allow-dst-swap";

class IpSetManager
{
public:
    otError FlushIpSet(const char *aName);
    otError AddToIpSet(const char *aSetName, const char *aAddress);
    otError SwapIpSets(const char *aSetName1, const char *aSetName2);
};

inline otError IpSetManager::FlushIpSet(const char *aName)
{
    return ExecuteCommand("%s flush %s", kIpsetCommand, aName);
}

inline otError IpSetManager::AddToIpSet(const char *aSetName, const char *aAddress)
{
    return ExecuteCommand("%s add %s %s -exist", kIpsetCommand, aSetName, aAddress);
}

inline otError IpSetManager::SwapIpSets(const char *aSetName1, const char *aSetName2)
{
    return ExecuteCommand("%s swap %s %s", kIpsetCommand, aSetName1, aSetName2);
}

void UpdateIpSets(otInstance *aInstance)
{
    otError               error    = OT_ERROR_NONE;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;
    otIp6Prefix           prefix;
    char                  prefixBuf[OT_IP6_PREFIX_STRING_SIZE];
    IpSetManager          ipSetManager;

    // 1. Flush the '*-swap' ipsets
    SuccessOrExit(error = ipSetManager.FlushIpSet(kIngressAllowDstSwapIpSet));
    SuccessOrExit(error = ipSetManager.FlushIpSet(kIngressDenySrcSwapIpSet));

    // 2. Update otbr-deny-src-swap
    while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        if (config.mDp)
        {
            continue;
        }
        otIp6PrefixToString(&config.mPrefix, prefixBuf, sizeof(prefixBuf));
        SuccessOrExit(error = ipSetManager.AddToIpSet(kIngressDenySrcSwapIpSet, prefixBuf));
    }
    memcpy(prefix.mPrefix.mFields.m8, otThreadGetMeshLocalPrefix(aInstance)->m8,
           sizeof(otThreadGetMeshLocalPrefix(aInstance)->m8));
    prefix.mLength = OT_IP6_PREFIX_BITSIZE;
    otIp6PrefixToString(&prefix, prefixBuf, sizeof(prefixBuf));
    SuccessOrExit(error = ipSetManager.AddToIpSet(kIngressDenySrcSwapIpSet, prefixBuf));

    // 3. Update otbr-allow-dst-swap
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        otIp6PrefixToString(&config.mPrefix, prefixBuf, sizeof(prefixBuf));
        SuccessOrExit(error = ipSetManager.AddToIpSet(kIngressAllowDstSwapIpSet, prefixBuf));
    }

    // 4. Swap ipsets to let them take effect
    SuccessOrExit(error = ipSetManager.SwapIpSets(kIngressDenySrcSwapIpSet, kIngressDenySrcIpSet));
    SuccessOrExit(error = ipSetManager.SwapIpSets(kIngressAllowDstSwapIpSet, kIngressAllowDstIpSet));

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("Failed to update ipsets: %s", otThreadErrorToString(error));
    }
}

#endif // defined(__linux__) && OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE

} // namespace Posix
} // namespace ot
