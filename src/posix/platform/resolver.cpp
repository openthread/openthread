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

#include "platform-posix.h"

#include <openthread/logging.h>
#include <openthread/message.h>
#include <openthread/udp.h>
#include <openthread/platform/dns.h>
#include <openthread/platform/time.h>

#include "common/code_utils.hpp"

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <cassert>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <string>

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

namespace {
constexpr char kResolvConfFullPath[] = "/etc/resolv.conf";
constexpr char kNameserverItem[]     = "nameserver";
} // namespace

extern ot::Posix::Resolver gResolver;

namespace ot {
namespace Posix {

void Resolver::Init(void)
{
    memset(mUpstreamTransaction, 0, sizeof(mUpstreamTransaction));
    LoadDnsServerListFromConf();
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

    mUpstreamDnsServerCount = 0;

    fp.open(kResolvConfFullPath);

    while (fp.good() && std::getline(fp, line) && mUpstreamDnsServerCount < kMaxUpstreamServerCount)
    {
        if (line.find(kNameserverItem, 0) == 0)
        {
            in_addr_t addr;

            if (inet_pton(AF_INET, &line.c_str()[sizeof(kNameserverItem)], &addr) == 1)
            {
                otLogInfoPlat("Got nameserver #%d: %s", mUpstreamDnsServerCount,
                              &line.c_str()[sizeof(kNameserverItem)]);
                mUpstreamDnsServerList[mUpstreamDnsServerCount] = addr;
                mUpstreamDnsServerCount++;
            }
        }
    }

    if (mUpstreamDnsServerCount == 0)
    {
        otLogCritPlat("No domain name servers found in %s, default to 127.0.0.1", kResolvConfFullPath);
    }

    mUpstreamDnsServerListFreshness = otPlatTimeGet();
}

void Resolver::Query(otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery)
{
    char        packet[kMaxDnsMessageSize];
    otError     error  = OT_ERROR_NONE;
    uint16_t    length = otMessageGetLength(aQuery);
    sockaddr_in serverAddr;

    Transaction *txn = nullptr;

    VerifyOrExit(length <= kMaxDnsMessageSize, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(otMessageRead(aQuery, 0, &packet, sizeof(packet)) == length, error = OT_ERROR_NO_BUFS);

    txn = AllocateTransaction(aTxn);
    VerifyOrExit(txn != nullptr, error = OT_ERROR_NO_BUFS);

    TryRefreshDnsServerList();

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(53);
    for (int i = 0; i < mUpstreamDnsServerCount; i++)
    {
        serverAddr.sin_addr.s_addr = mUpstreamDnsServerList[i];
        VerifyOrExit(
            sendto(txn->mUdpFd, packet, length, MSG_DONTWAIT, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) > 0,
            error = OT_ERROR_NO_ROUTE);
    }
    otLogInfoPlat("Forwarded DNS query %p to %d server(s).", static_cast<void *>(aTxn), mUpstreamDnsServerCount);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Failed to forward DNS query %p to server: %d", static_cast<void *>(aTxn), error);
    }
    return;
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
    int          fdOrError = 0;
    Transaction *ret       = nullptr;

    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn == nullptr)
        {
            fdOrError = socket(AF_INET, SOCK_DGRAM, 0);
            if (fdOrError < 0)
            {
                otLogInfoPlat("Failed to create socket for upstream resolver: %d", fdOrError);
                break;
            }
            ret             = &txn;
            ret->mUdpFd     = fdOrError;
            ret->mThreadTxn = aThreadTxn;
            break;
        }
    }

    return ret;
}

void Resolver::ForwardResponse(Transaction *aTxn)
{
    char       response[kMaxDnsMessageSize];
    ssize_t    readSize;
    otError    error   = OT_ERROR_NONE;
    otMessage *message = nullptr;

    VerifyOrExit((readSize = read(aTxn->mUdpFd, response, sizeof(response))) > 0);

    message = otUdpNewMessage(gInstance, nullptr);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageAppend(message, response, readSize));

    otPlatDnsUpstreamQueryDone(gInstance, aTxn->mThreadTxn, message);
    message = nullptr;

exit:
    if (readSize < 0)
    {
        otLogInfoPlat("Failed to read response from upstream resolver socket: %d", errno);
    }
    if (error != OT_ERROR_NONE)
    {
        otLogInfoPlat("Failed to forward upstream DNS response: %s", otThreadErrorToString(error));
    }
    if (message != nullptr)
    {
        otMessageFree(message);
    }
}

Resolver::Transaction *Resolver::GetTransaction(int aFd)
{
    Transaction *ret = nullptr;

    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn != nullptr && txn.mUdpFd == aFd)
        {
            ret = &txn;
            break;
        }
    }

    return ret;
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
    if (aTxn->mUdpFd >= 0)
    {
        close(aTxn->mUdpFd);
        aTxn->mUdpFd = -1;
    }
    aTxn->mThreadTxn = nullptr;
}

void Resolver::UpdateFdSet(otSysMainloopContext &aContext)
{
    for (Transaction &txn : mUpstreamTransaction)
    {
        if (txn.mThreadTxn != nullptr)
        {
            FD_SET(txn.mUdpFd, &aContext.mReadFdSet);
            FD_SET(txn.mUdpFd, &aContext.mErrorFdSet);
            if (txn.mUdpFd > aContext.mMaxFd)
            {
                aContext.mMaxFd = txn.mUdpFd;
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
            if (FD_ISSET(txn.mUdpFd, &aContext.mErrorFdSet) || FD_ISSET(txn.mUdpFd, &aContext.mReadFdSet))
            {
                ForwardResponse(&txn);
                CloseTransaction(&txn);
            }
        }
    }
}

} // namespace Posix
} // namespace ot

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

#endif // OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE
