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

#ifndef OT_POSIX_PLATFORM_RESOLVER_HPP_
#define OT_POSIX_PLATFORM_RESOLVER_HPP_

#include <openthread/openthread-system.h>
#include <openthread/platform/dns.h>

#include <arpa/inet.h>
#include <sys/select.h>

#include "logger.hpp"

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

namespace ot {
namespace Posix {

class Resolver : public Logger<Resolver>
{
public:
    static const char kLogModuleName[]; ///< Module name used for logging.

    constexpr static ssize_t kMaxDnsMessageSize           = 512;
    constexpr static ssize_t kMaxUpstreamTransactionCount = 16;
    constexpr static ssize_t kMaxUpstreamServerCount      = 3;
    constexpr static ssize_t kMaxRecursiveServerCount     = 3;

    /**
     * Initialize the upstream DNS resolver.
     */
    void Init(void);

    /**
     * Sets up the upstream DNS resolver.
     *
     * @note This method is called after OpenThread instance is created.
     */
    void Setup(void);

    /**
     * Sends the query to the upstream.
     *
     * @param[in] aTxn   A pointer to the OpenThread upstream DNS query transaction.
     * @param[in] aQuery A pointer to a message for the payload of the DNS query.
     */
    void Query(otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery);

    /**
     * Cancels a upstream DNS query transaction.
     *
     * @param[in] aTxn   A pointer to the OpenThread upstream DNS query transaction.
     */
    void Cancel(otPlatDnsUpstreamQuery *aTxn);

    /**
     * Updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[in,out]  aContext  The mainloop context.
     */
    void UpdateFdSet(otSysMainloopContext &aContext);

    /**
     * Handles the result of select.
     *
     * @param[in]  aContext  The mainloop context.
     */
    void Process(const otSysMainloopContext &aContext);

    /**
     * Sets whether to retrieve upstream DNS servers from "resolv.conf".
     *
     * @param[in] aEnabled  TRUE if enable retrieving upstream DNS servers from "resolv.conf", FALSE otherwise.
     */
    void SetResolvConfEnabled(bool aEnabled) { mIsResolvConfEnabled = aEnabled; }

    /**
     * Sets the upstream DNS servers.
     *
     * @param[in] aUpstreamDnsServers  A pointer to the list of upstream DNS server addresses. Each address could be an
     *                                 IPv6 address or an IPv4-mapped IPv6 address.
     * @param[in] aNumServers          The number of upstream DNS servers.
     */
    void SetUpstreamDnsServers(const otIp6Address *aUpstreamDnsServers, uint32_t aNumServers);

    /**
     * Sets the list of recursive DNS servers.
     *
     * @param[in] aRecursiveDnsServers A pointer to the list of IPv6 recursive DNS server addresses.
     * @param[in] aNumServers          The number of recursive DNS servers.
     *
     */
    void SetRecursiveDnsServerList(const otIp6Address *aRecursiveDnsServers, uint32_t aNumServers);

private:
    static constexpr uint64_t kDnsServerListNullCacheTimeoutMs = 1 * 60 * 1000;  // 1 minute
    static constexpr uint64_t kDnsServerListCacheTimeoutMs     = 10 * 60 * 1000; // 10 minutes

    struct Transaction
    {
        otPlatDnsUpstreamQuery *mThreadTxn;
        int                     mUdpFd4;
        int                     mUdpFd6;
    };

    static int CreateUdpSocket(sa_family_t aFamily);

    Transaction *GetTransaction(otPlatDnsUpstreamQuery *aThreadTxn);
    Transaction *AllocateTransaction(otPlatDnsUpstreamQuery *aThreadTxn);

    otError SendQueryToServer(Transaction        *aTxn,
                              const otIp6Address &aServerAddress,
                              const char         *aPacket,
                              uint16_t            aLength);
    void    ForwardResponse(otPlatDnsUpstreamQuery *aThreadTxn, int aFd);
    void    CloseTransaction(Transaction *aTxn);
    void    TryRefreshDnsServerList(void);
    void    LoadDnsServerListFromConf(void);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    static void BorderRoutingRdnssCallback(void *aResolver);
    void        BorderRoutingRdnssCallback(void);
#endif

    bool         mIsResolvConfEnabled    = OPENTHREAD_POSIX_CONFIG_RESOLV_CONF_ENABLED_INIT;
    uint32_t     mUpstreamDnsServerCount = 0;
    otIp6Address mUpstreamDnsServerList[kMaxUpstreamServerCount];
    uint64_t     mUpstreamDnsServerListFreshness = 0;

    uint32_t     mRecursiveDnsServerCount = 0;
    otIp6Address mRecursiveDnsServerList[kMaxRecursiveServerCount];

    Transaction mUpstreamTransaction[kMaxUpstreamTransactionCount];
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

#endif // OT_POSIX_PLATFORM_RESOLVER_HPP_
