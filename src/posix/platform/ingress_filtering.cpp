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
 *   This file includes the implementation of ingress filtering.
 */

#include "ingress_filtering.hpp"

#include <cstdio>
#include <cstring>

#include "common/code_utils.hpp"
#include "openthread/backbone_router_ftd.h"
#include "openthread/netdata.h"
#include "posix/platform/utils.hpp"

namespace ot {
namespace Posix {

#if OPENTHREAD_POSIX_CONFIG_INGRESS_FILTERING_ENABLE

static const char *           kIp6TablesCommand           = "ip6tables";
static const char *           kForwardChainName           = "FORWARD";
static const char *           kOtbrForwardChainNameFormat = "OTBR_FORWARD_%s";
static const char *           kAnyAddress                 = "::/0";
static const char *           kDrop                       = "DROP";
static const char *           kAccept                     = "ACCEPT";
static constexpr unsigned int kOtbrForwardChainNameLength = 64;
static char                   sOtbrForwardChainName[kOtbrForwardChainNameLength];

bool ChainExists(const char *aChain)
{
    return 0 == ExecuteCommand("%s -L %s", kIp6TablesCommand, aChain);
}

bool ChainContainsChain(const char *aParentChain, const char *aThreadIterface, const char *aChildChain)
{
    return 0 == ExecuteCommand("%s -C %s -o %s -j %s", kIp6TablesCommand, aParentChain, aThreadIterface, aChildChain);
}

otError PrependChildChain(const char *aParentChain, const char *aThreadInterface, const char *aChildChain)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(
        0 == ExecuteCommand("%s -I %s 1 -o %s -j %s", kIp6TablesCommand, aParentChain, aThreadInterface, aChildChain),
        error = OT_ERROR_FAILED);
exit:
    return error;
}

otError DeleteChildChain(const char *aParentChain, const char *aThreadInterface, const char *aChildChain)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(
        0 == ExecuteCommand("%s -D %s -o %s -j %s", kIp6TablesCommand, aParentChain, aThreadInterface, aChildChain),
        error = OT_ERROR_FAILED);
exit:
    return error;
}

otError CreateChain(const char *aChain)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(0 == ExecuteCommand("%s -N %s", kIp6TablesCommand, aChain), error = OT_ERROR_FAILED);
exit:
    return error;
}

otError FlushChain(const char *aChain)
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(0 == ExecuteCommand("%s -F %s", kIp6TablesCommand, aChain), error = OT_ERROR_FAILED);
exit:
    return error;
}

otError AppendRule(const char *aChain,
                   const char *aOutInterface,
                   const char *aSource,
                   const char *aDestination,
                   const char *aTarget,
                   const char *aOption = "")
{
    otError error = OT_ERROR_NONE;
    VerifyOrExit(0 == ExecuteCommand("%s -A %s -o %s -s %s -d %s -j %s %s", kIp6TablesCommand, aChain, aOutInterface,
                                     aSource, aDestination, aTarget, aOption),
                 error = OT_ERROR_FAILED);
exit:
    return error;
}

otError InitOtbrForwardChain(const char *aThreadInterface)
{
    otError error            = OT_ERROR_NONE;
    int     deleteChainTimes = 5;
    snprintf(sOtbrForwardChainName, sizeof(sOtbrForwardChainName), kOtbrForwardChainNameFormat, aThreadInterface);

    if (!ChainExists(sOtbrForwardChainName))
    {
        SuccessOrExit(error = CreateChain(sOtbrForwardChainName));
    }
    while (ChainContainsChain(kForwardChainName, aThreadInterface, sOtbrForwardChainName) && deleteChainTimes-- > 0)
    {
        SuccessOrExit(error = DeleteChildChain(kForwardChainName, aThreadInterface, sOtbrForwardChainName));
    }
    SuccessOrExit(error = PrependChildChain(kForwardChainName, aThreadInterface, sOtbrForwardChainName));
exit:
    return error;
}

otError UpdateRules(otInstance *aInstance, const char *aThreadInterface)
{
    otError               error    = OT_ERROR_NONE;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;
    otIp6Prefix           prefix;
    char                  prefixBuf[OT_IP6_PREFIX_STRING_SIZE];

    // 1. Flush the chain
    SuccessOrExit(error = FlushChain(sOtbrForwardChainName));

    // 2. Drop packets from OMR and mesh local addresses
    while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        if (config.mDp)
        {
            continue;
        }
        otIp6PrefixToString(&config.mPrefix, prefixBuf, sizeof(prefixBuf));
        SuccessOrExit(error = AppendRule(sOtbrForwardChainName, aThreadInterface, prefixBuf, kAnyAddress, kDrop));
    }
    memcpy(prefix.mPrefix.mFields.m8, otThreadGetMeshLocalPrefix(aInstance)->m8,
           sizeof(otThreadGetMeshLocalPrefix(aInstance)->m8));
    prefix.mLength = OT_IP6_PREFIX_BITSIZE;
    otIp6PrefixToString(&prefix, prefixBuf, sizeof(prefixBuf));
    SuccessOrExit(error = AppendRule(sOtbrForwardChainName, aThreadInterface, prefixBuf, kAnyAddress, kDrop));

    // 3. Accept packets to OMR and DUA addresses
    iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    while (otNetDataGetNextOnMeshPrefix(aInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        otIp6PrefixToString(&config.mPrefix, prefixBuf, sizeof(prefixBuf));
        SuccessOrExit(error = AppendRule(sOtbrForwardChainName, aThreadInterface, kAnyAddress, prefixBuf, kAccept));
    }

    // 4. Drop all unmatched unicast packets
    SuccessOrExit(error = AppendRule(sOtbrForwardChainName, aThreadInterface, kAnyAddress, kAnyAddress, kDrop,
                                     "-m pkttype --pkt-type unicast"));

    // 5. Accept all unmatched packets.
    SuccessOrExit(error = AppendRule(sOtbrForwardChainName, aThreadInterface, kAnyAddress, kAnyAddress, kAccept));

exit:
    if (error != OT_ERROR_NONE)
    {
        IgnoreError(FlushChain(sOtbrForwardChainName));
    }
    return error;
}

#endif // OPENTHREAD_POSIX_CONFIG_INGRESS_FILTERING_ENABLE

} // namespace Posix
} // namespace ot
