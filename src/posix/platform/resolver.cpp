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

#include "resolver.hpp"
#include "ip6_utils.hpp"

#include "platform-posix.h"

#include <openthread/border_routing.h>
#include <openthread/logging.h>
#include <openthread/message.h>
#include <openthread/nat64.h>
#include <openthread/openthread-system.h>
#include <openthread/udp.h>
#include <openthread/platform/dns.h>
#include <openthread/platform/time.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <cassert>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <string>

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

using namespace ot::Posix::Ip6Utils;

namespace {
constexpr char kResolvConfFullPath[] = "/etc/resolv.conf";
constexpr char kNameserverItem[]     = "nameserver";
} // namespace

ot::Posix::Resolver gResolver;

namespace ot {
namespace Posix {

const char Resolver::kLogModuleName[] = "Resolver";

void Resolver::Init(void)
{
    memset(mUpstreamTransaction, 0, sizeof(mUpstreamTransaction));
    LoadDnsServerListFromConf();
}

void Resolver::Setup(void)
{
    OT_ASSERT(gInstance != nullptr);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    otBorderRoutingSetRdnssAddrCallback(gInstance, &Resolver::BorderRoutingRdnssCallback, this);
#endif
}

void Resolver::TryRefreshDnsServerList(void)
{
    uint64_t now = otPlatTimeGet();

    if (now > mUpstreamDnsServerListFreshness + kDnsServerListCacheTimeoutMs ||
        (mUpstreamDnsServerCount == 0 && now > mUpstreamDnsServerListFreshness + kDnsServerListNullCacheTimeoutMs))
    {
        LoadDnsServerListFromConf();
    }
}

void Resolver::LoadDnsServerListFromConf(void)
{
    std::string   line;
    std::ifstream fp;
    otIp4Address  ip4Address;
    otIp6Address  ip6Address;

    VerifyOrExit(mIsResolvConfEnabled);

    mUpstreamDnsServerCount = 0;

    fp.open(kResolvConfFullPath);

    while (fp.good() && std::getline(fp, line) && mUpstreamDnsServerCount < kMaxUpstreamServerCount)
    {
        const char *addressString = &line.c_str()[sizeof(kNameserverItem)];

        // Skip the lines that don't start with "nameserver"
        if (line.find(kNameserverItem, 0))
        {
            continue;
        }

        if (inet_pton(AF_INET, addressString, &ip4Address) == 1)
        {
            otIp4ToIp4MappedIp6Address(&ip4Address, &ip6Address);
        }
        else if (inet_pton(AF_INET6, addressString, &ip6Address) != 1)
        {
            continue;
        }

        LogInfo("Got nameserver #%u: %s", mUpstreamDnsServerCount, addressString);
        mUpstreamDnsServerList[mUpstreamDnsServerCount] = ip6Address;
        mUpstreamDnsServerCount++;
    }

    if (mUpstreamDnsServerCount == 0)
    {
        LogCrit("No domain name servers found in %s, default to 127.0.0.1", kResolvConfFullPath);
    }

    mUpstreamDnsServerListFreshness = otPlatTimeGet();
exit:
    return;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void Resolver::BorderRoutingRdnssCallback(void *aResolver)
{
    static_cast<Resolver *>(aResolver)->BorderRoutingRdnssCallback();
}

void Resolver::BorderRoutingRdnssCallback(void)
{
    otBorderRoutingPrefixTableIterator iterator;
    otBorderRoutingRdnssAddrEntry      entry;
    otBorderRoutingRdnssAddrEntry      rdnssEntries[kMaxRecursiveServerCount + 1];
    otIp6Address                       rdnssServers[kMaxRecursiveServerCount];
    uint32_t                           numEntries = 0;

    otBorderRoutingPrefixTableInitIterator(gInstance, &iterator);

    while (otBorderRoutingGetNextRdnssAddrEntry(gInstance, &iterator, &entry) == OT_ERROR_NONE)
    {
        uint32_t i = 0;

        // Check if the entry address is already in the list.
        for (; i < numEntries; ++i)
        {
            if (otIp6IsAddressEqual(&entry.mAddress, &rdnssEntries[i].mAddress))
            {
                rdnssEntries[i].mLifetime = OT_MAX(rdnssEntries[i].mLifetime, entry.mLifetime);

                break;
            }
        }

        // If the address is not a duplicate, add the entry to the entry list.
        if (i == numEntries)
        {
            rdnssEntries[numEntries++] = entry;

            std::sort(rdnssEntries, rdnssEntries + numEntries,
                      [](const otBorderRoutingRdnssAddrEntry &a, const otBorderRoutingRdnssAddrEntry &b) {
                          bool result = false;

                          if (a.mLifetime != b.mLifetime)
                          {
                              result = a.mLifetime > b.mLifetime;
                          }
                          else
                          {
                              // If lifetimes are equal, prefer the one with the larger numeric values
                              for (uint8_t j = 0; j < sizeof(otIp6Address); j++)
                              {
                                  if (a.mAddress.mFields.m8[j] != b.mAddress.mFields.m8[j])
                                  {
                                      result = a.mAddress.mFields.m8[j] > b.mAddress.mFields.m8[j];
                                      break;
                                  }
                              }
                          }

                          return result;
                      });

            numEntries = OT_MIN(numEntries, kMaxRecursiveServerCount);
        }
    }

    for (uint32_t i = 0; i < numEntries; i++)
    {
        rdnssServers[i] = rdnssEntries[i].mAddress;
    }

    SetRecursiveDnsServerList(rdnssServers, numEntries);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

otError Resolver::SendQueryToServer(Transaction        *aTxn,
                                    const otIp6Address &aServerAddress,
                                    const char         *aPacket,
                                    uint16_t            aLength)
{
    otError      error = OT_ERROR_NONE;
    otIp4Address ip4Addr;
    sockaddr_in  serverAddr4;
    sockaddr_in6 serverAddr6;

    if (otIp4FromIp4MappedIp6Address(&aServerAddress, &ip4Addr) == OT_ERROR_NONE)
    {
        memcpy(&serverAddr4.sin_addr.s_addr, &ip4Addr, sizeof(otIp4Address));
        serverAddr4.sin_family = AF_INET;
        serverAddr4.sin_port   = htons(53);

        VerifyOrExit(sendto(aTxn->mUdpFd4, aPacket, aLength, MSG_DONTWAIT, reinterpret_cast<sockaddr *>(&serverAddr4),
                            sizeof(serverAddr4)) > 0,
                     error = OT_ERROR_NO_ROUTE);
    }
    else
    {
        memcpy(&serverAddr6.sin6_addr, &aServerAddress, sizeof(otIp6Address));
        serverAddr6.sin6_family = AF_INET6;
        serverAddr6.sin6_port   = htons(53);

        VerifyOrExit(sendto(aTxn->mUdpFd6, aPacket, aLength, MSG_DONTWAIT, reinterpret_cast<sockaddr *>(&serverAddr6),
                            sizeof(serverAddr6)) > 0,
                     error = OT_ERROR_NO_ROUTE);
    }

exit:
    return error;
}

void Resolver::Query(otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery)
{
    char         packet[kMaxDnsMessageSize];
    otError      error  = OT_ERROR_NONE;
    uint16_t     length = otMessageGetLength(aQuery);
    Transaction *txn    = nullptr;

    VerifyOrExit(length <= kMaxDnsMessageSize, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(otMessageRead(aQuery, 0, &packet, sizeof(packet)) == length, error = OT_ERROR_NO_BUFS);

    txn = AllocateTransaction(aTxn);
    VerifyOrExit(txn != nullptr, error = OT_ERROR_NO_BUFS);
    TryRefreshDnsServerList();

    for (uint32_t i = 0; i < mRecursiveDnsServerCount; i++)
    {
        SuccessOrExit(error = SendQueryToServer(txn, mRecursiveDnsServerList[i], packet, length));

        LogInfo("Forwarded DNS query %p to %s", static_cast<void *>(aTxn),
                Ip6AddressString(&mRecursiveDnsServerList[i]).AsCString());
    }

    for (uint32_t i = 0; i < mUpstreamDnsServerCount; i++)
    {
        SuccessOrExit(error = SendQueryToServer(txn, mUpstreamDnsServerList[i], packet, length));

        LogInfo("Forwarded DNS query %p to %s", static_cast<void *>(aTxn),
                Ip6AddressString(&mUpstreamDnsServerList[i]).AsCString());
    }

    LogInfo("Forwarded DNS query %p to %d server(s).", static_cast<void *>(aTxn),
            mRecursiveDnsServerCount + mUpstreamDnsServerCount);

exit:
    if (error != OT_ERROR_NONE)
    {
        LogWarn("Failed to forward DNS query %p to server: %s", static_cast<void *>(aTxn),
                otThreadErrorToString(error));
        if (txn != nullptr)
        {
            CloseTransaction(txn);
        }
    }
}

void Resolver::Cancel(otPlatDnsUpstreamQuery *aTxn)
{
    Transaction *txn = GetTransaction(aTxn);

    if (txn != nullptr)
    {
        CloseTransaction(txn);
    }

    otPlatDnsUpstreamQueryDone(gInstance, aTxn, nullptr);
}

Resolver::Transaction *Resolver::AllocateTransaction(otPlatDnsUpstreamQuery *aThreadTxn)
{
    int          fd4OrError = 0;
    int          fd6OrError = 0;
    Transaction *ret        = nullptr;

    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn == nullptr)
        {
            fd4OrError = CreateUdpSocket(AF_INET);
            if (fd4OrError < 0)
            {
                LogInfo("Failed to create socket for upstream resolver: %d", fd4OrError);
                break;
            }

            fd6OrError = CreateUdpSocket(AF_INET6);
            if (fd6OrError < 0)
            {
                LogInfo("Failed to create socket for upstream resolver: %d", fd6OrError);
                break;
            }

            ret             = &txn;
            ret->mUdpFd4    = fd4OrError;
            ret->mUdpFd6    = fd6OrError;
            ret->mThreadTxn = aThreadTxn;
            break;
        }
    }

    return ret;
}

void Resolver::ForwardResponse(otPlatDnsUpstreamQuery *aThreadTxn, int aFd)
{
    char       response[kMaxDnsMessageSize];
    ssize_t    readSize;
    otError    error   = OT_ERROR_NONE;
    otMessage *message = nullptr;

    VerifyOrExit((readSize = read(aFd, response, sizeof(response))) > 0);

    message = otUdpNewMessage(gInstance, nullptr);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageAppend(message, response, readSize));

    otPlatDnsUpstreamQueryDone(gInstance, aThreadTxn, message);
    message = nullptr;

exit:
    if (readSize < 0)
    {
        LogInfo("Failed to read response from upstream resolver socket: %d", errno);
    }
    if (error != OT_ERROR_NONE)
    {
        LogInfo("Failed to forward upstream DNS response: %s", otThreadErrorToString(error));
    }
    if (message != nullptr)
    {
        otMessageFree(message);
    }
}

Resolver::Transaction *Resolver::GetTransaction(otPlatDnsUpstreamQuery *aThreadTxn)
{
    Transaction *ret = nullptr;

    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn == aThreadTxn)
        {
            ret = &txn;
            break;
        }
    }

    return ret;
}

void Resolver::CloseTransaction(Transaction *aTxn)
{
    if (aTxn->mUdpFd4 >= 0)
    {
        close(aTxn->mUdpFd4);
        aTxn->mUdpFd4 = -1;
    }
    if (aTxn->mUdpFd6 >= 0)
    {
        close(aTxn->mUdpFd6);
        aTxn->mUdpFd6 = -1;
    }
    aTxn->mThreadTxn = nullptr;
}

void Resolver::UpdateFdSet(otSysMainloopContext &aContext)
{
    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn != nullptr)
        {
            FD_SET(txn.mUdpFd4, &aContext.mReadFdSet);
            FD_SET(txn.mUdpFd4, &aContext.mErrorFdSet);
            FD_SET(txn.mUdpFd6, &aContext.mReadFdSet);
            FD_SET(txn.mUdpFd6, &aContext.mErrorFdSet);

            if (txn.mUdpFd6 > aContext.mMaxFd)
            {
                aContext.mMaxFd = txn.mUdpFd6;
            }
            if (txn.mUdpFd4 > aContext.mMaxFd)
            {
                aContext.mMaxFd = txn.mUdpFd4;
            }
        }
    }
}

void Resolver::Process(const otSysMainloopContext &aContext)
{
    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn != nullptr)
        {
            // Note: On Linux, we can only get the error via read, so they should share the same logic.
            if (FD_ISSET(txn.mUdpFd4, &aContext.mErrorFdSet) || FD_ISSET(txn.mUdpFd4, &aContext.mReadFdSet))
            {
                ForwardResponse(txn.mThreadTxn, txn.mUdpFd4);
                CloseTransaction(&txn);
            }
            else if (FD_ISSET(txn.mUdpFd6, &aContext.mErrorFdSet) || FD_ISSET(txn.mUdpFd6, &aContext.mReadFdSet))
            {
                ForwardResponse(txn.mThreadTxn, txn.mUdpFd6);
                CloseTransaction(&txn);
            }
        }
    }
}

void Resolver::SetUpstreamDnsServers(const otIp6Address *aUpstreamDnsServers, uint32_t aNumServers)
{
    mUpstreamDnsServerCount = OT_MIN(aNumServers, static_cast<uint32_t>(kMaxUpstreamServerCount));
    memcpy(mUpstreamDnsServerList, aUpstreamDnsServers, mUpstreamDnsServerCount * sizeof(otIp6Address));

    LogInfo("Set upstream DNS server list, count: %d", mUpstreamDnsServerCount);
}

void Resolver::SetRecursiveDnsServerList(const otIp6Address *aRecursiveDnsServers, uint32_t aNumServers)
{
    mRecursiveDnsServerCount = OT_MIN(aNumServers, static_cast<uint32_t>(kMaxRecursiveServerCount));
    memcpy(mRecursiveDnsServerList, aRecursiveDnsServers, mRecursiveDnsServerCount * sizeof(otIp6Address));

    LogInfo("Set recursive DNS server list, count: %d", mRecursiveDnsServerCount);
}

int Resolver::CreateUdpSocket(sa_family_t aFamily)
{
    int fd = -1;

    VerifyOrExit(otSysGetInfraNetifName() != nullptr, LogDebg("No infra network interface available"));
    fd = socket(aFamily, SOCK_DGRAM, IPPROTO_UDP);
    VerifyOrExit(fd >= 0, LogDebg("Failed to create the UDP socket: %s", strerror(errno)));
#if OPENTHREAD_POSIX_CONFIG_UPSTREAM_DNS_BIND_TO_INFRA_NETIF
    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, otSysGetInfraNetifName(), strlen(otSysGetInfraNetifName())) < 0)
    {
        LogDebg("Failed to bind the UDP socket to infra interface %s: %s", otSysGetInfraNetifName(), strerror(errno));
        close(fd);
        fd = -1;
        ExitNow();
    }
#endif

exit:
    return fd;
}

} // namespace Posix
} // namespace ot

void platformResolverProcess(const otSysMainloopContext *aContext) { gResolver.Process(*aContext); }

void platformResolverUpdateFdSet(otSysMainloopContext *aContext) { gResolver.UpdateFdSet(*aContext); }

void platformResolverSetUp(void) { gResolver.Setup(); }

void platformResolverInit(void) { gResolver.Init(); }

void otPlatDnsStartUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery)
{
    OT_UNUSED_VARIABLE(aInstance);

    gResolver.Query(aTxn, aQuery);
}

void otPlatDnsCancelUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn)
{
    OT_UNUSED_VARIABLE(aInstance);

    gResolver.Cancel(aTxn);
}

void otSysUpstreamDnsServerSetResolvConfEnabled(bool aEnabled) { gResolver.SetResolvConfEnabled(aEnabled); }

void otSysUpstreamDnsSetServerList(const otIp6Address *aUpstreamDnsServers, int aNumServers)
{
    gResolver.SetUpstreamDnsServers(aUpstreamDnsServers, aNumServers);
}

#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
