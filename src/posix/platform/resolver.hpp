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

#ifndef POSIX_PLATFORM_RESOLVER_HPP_
#define POSIX_PLATFORM_RESOLVER_HPP_

#include <openthread/openthread-system.h>
#include <openthread/platform/dns.h>

#include <arpa/inet.h>
#include <sys/select.h>

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

namespace ot {
namespace Posix {

class Resolver
{
public:
    constexpr static ssize_t kMaxDnsMessageSize           = 512;
    constexpr static ssize_t kMaxUpstreamTransactionCount = 16;
    constexpr static ssize_t kMaxUpstreamServerCount      = 3;

    /**
     * Initialize the upstream DNS resolver.
     *
     */
    void Init(void);

    /**
     * Sends the query to the upstream.
     *
     * @param[in] aTxn   A pointer to the OpenThread upstream DNS query transaction.
     * @param[in] aQuery A pointer to a message for the payload of the DNS query.
     *
     */
    void Query(otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery);

    /**
     * Cancels a upstream DNS query transaction.
     *
     * @param[in] aTxn   A pointer to the OpenThread upstream DNS query transaction.
     *
     */
    void Cancel(otPlatDnsUpstreamQuery *aTxn);

    /**
     * Updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[in,out]  aReadFdSet   A reference to the read file descriptors.
     * @param[in,out]  aErrorFdSet  A reference to the error file descriptors.
     * @param[in,out]  aMaxFd       A reference to the max file descriptor.
     * @param[in,out]  aTimeout     A reference to the timeout.
     *
     */
    void UpdateFdSet(otSysMainloopContext &aContext);

    /**
     * Handles the result of select.
     *
     * @param[in]  aReadFdSet   A reference to the read file descriptors.
     * @param[in]  aErrorFdSet  A reference to the error file descriptors.
     *
     */
    void Process(const otSysMainloopContext &aContext);

private:
    static constexpr uint64_t kDnsServerListNullCacheTimeoutMs = 1 * 60 * 1000;  // 1 minute
    static constexpr uint64_t kDnsServerListCacheTimeoutMs     = 10 * 60 * 1000; // 10 minutes

    struct Transaction
    {
        otPlatDnsUpstreamQuery *mThreadTxn;
        int                     mUdpFd;
    };

    Transaction *GetTransaction(int aFd);
    Transaction *GetTransaction(otPlatDnsUpstreamQuery *aThreadTxn);
    Transaction *AllocateTransaction(otPlatDnsUpstreamQuery *aThreadTxn);

    void ForwardResponse(Transaction *aTxn);
    void CloseTransaction(Transaction *aTxn);
    void FinishTransaction(int aFd);
    void TryRefreshDnsServerList(void);
    void LoadDnsServerListFromConf(void);

    int       mUpstreamDnsServerCount = 0;
    in_addr_t mUpstreamDnsServerList[kMaxUpstreamServerCount];
    uint64_t  mUpstreamDnsServerListFreshness = 0;

    Transaction mUpstreamTransaction[kMaxUpstreamTransactionCount];
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

#endif // POSIX_PLATFORM_RESOLVER_HPP_
