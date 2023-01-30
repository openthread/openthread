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

#include "common/code_utils.hpp"

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <cassert>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <fstream>
#include <string>

#if OPENTHREAD_CONFIG_DNS_UPSTREAM_QUERY_ENABLE

namespace {
constexpr char      kResolveConfLocation[] = "/etc/resolv.conf";
constexpr char      kNameserverItem[]      = "nameserver";
} // namespace

extern ot::Posix::Resolver gResolver;

namespace ot {
namespace Posix {

void Resolver::Init(void)
{
    memset(mUpstreamTransaction, 0, sizeof(mUpstreamTransaction));

    LoadDnsServerListFromConf();
}

void Resolver::LoadDnsServerListFromConf(void)
{
    std::string   line;
    std::ifstream fp;

    fp.open(kResolveConfLocation);
    if (fp.bad())
    {
        otLogCritPlat("Cannot read %s for domain name servers, default to 127.0.0.1", kResolveConfLocation);
        mUpstreamDnsServerCount = 1;
        assert(inet_pton(AF_INET, "127.0.0.1", &mUpstreamDnsServerList[0]) == 1);
        ExitNow();
    }

    while (std::getline(fp, line) && mUpstreamDnsServerCount < kMaxUpstreamServerCount)
    {
        if (line.find(kNameserverItem, 0) == 0)
        {
            in_addr_t addr;

            if (inet_pton(AF_INET, &line.c_str()[sizeof(kNameserverItem)], &addr) == 1)
            {
                otLogCritPlat("Got nameserver #%d: %s", mUpstreamDnsServerCount,
                              &line.c_str()[sizeof(kNameserverItem)]);
                mUpstreamDnsServerList[mUpstreamDnsServerCount] = addr;
                mUpstreamDnsServerCount++;
            }
        }
    }

exit:
    return;
}

void Resolver::Query(otPlatDnsUpstreamQuery *aTxn, const otMessage *aQuery)
{
    char        packet[kMaxDnsMessageSize];
    otError     error  = OT_ERROR_NONE;
    uint16_t    length = otMessageGetLength(aQuery);
    sockaddr_in serverAddr;

    Transaction *txn;

    VerifyOrExit(length <= kMaxDnsMessageSize, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(otMessageRead(aQuery, 0, &packet, sizeof(packet)) == length, error = OT_ERROR_NO_BUFS);

    txn = AllocateTransaction(aTxn);
    VerifyOrExit(txn != nullptr, error = OT_ERROR_NO_BUFS);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(53);
    for (int i = 0; i < mUpstreamDnsServerCount; i++)
    {
        serverAddr.sin_addr.s_addr = mUpstreamDnsServerList[i];
        VerifyOrExit(sendto(txn->mUdpFd, packet, length, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)),
                     error = OT_ERROR_NO_ROUTE);
    }
    otLogInfoPlat("Forwarded DNS query to %d servers", mUpstreamDnsServerCount);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Failed to send DNS query to server: %d", error);
    }
    return;
}

void Resolver::Cancel(otPlatDnsUpstreamQuery *aTxn)
{
    Transaction *txn = GetTransaction(aTxn);

    CloseTransaction(txn);
}

Resolver::Transaction *Resolver::AllocateTransaction(otPlatDnsUpstreamQuery *aThreadTxn)
{
    int          i;
    int          error     = 0;
    int          fdOrError = 0;
    Transaction *ret       = nullptr;

    for (i = 0; i < kMaxUpstreamTransactionCount; i++)
    {
        if (mUpstreamTransaction[i].mThreadTxn == nullptr)
        {
            ret       = &mUpstreamTransaction[i];
            fdOrError = socket(AF_INET, SOCK_DGRAM, 0);
            if (fdOrError < 0)
            {
                otLogInfoPlat("Failed to create socket: %d", fdOrError);
                error = fdOrError;
                break;
            }
            ret->mUdpFd                        = fdOrError;
            mUpstreamTransaction[i].mThreadTxn = aThreadTxn;
            break;
        }
    }

    if (ret != nullptr && error != 0)
    {
        CloseTransaction(ret);
        ret = nullptr;
    }

    return ret;
}

void Resolver::ForwardResponse(Transaction *aTxn)
{
    char       response[kMaxDnsMessageSize];
    ssize_t    readSize;
    otMessage *message;

    VerifyOrExit((readSize = read(aTxn->mUdpFd, response, sizeof(response))) > 0);

    message = otUdpNewMessage(gInstance, nullptr);
    SuccessOrExit(otMessageAppend(message, response, readSize));

    otPlatDnsUpstreamQueryDone(gInstance, aTxn->mThreadTxn, message);
    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }
}

Resolver::Transaction *Resolver::GetTransaction(int aFd)
{
    Transaction *ret = nullptr;

    for (int i = 0; i < kMaxUpstreamTransactionCount; i++)
    {
        if (mUpstreamTransaction[i].mThreadTxn != nullptr && mUpstreamTransaction[i].mUdpFd == aFd)
        {
            ret = &mUpstreamTransaction[i];
            break;
        }
    }

    return ret;
}

Resolver::Transaction *Resolver::GetTransaction(otPlatDnsUpstreamQuery *aThreadTxn)
{
    Transaction *ret = nullptr;

    for (int i = 0; i < kMaxUpstreamTransactionCount; i++)
    {
        if (mUpstreamTransaction[i].mThreadTxn == aThreadTxn)
        {
            ret = &mUpstreamTransaction[i];
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

void Resolver::UpdateFdSet(fd_set *aReadFdSet, fd_set *aErrorFdSet, int *aMaxFd)
{
    for (int i = 0; i < kMaxUpstreamTransactionCount; i++)
    {
        if (mUpstreamTransaction[i].mThreadTxn != nullptr)
        {
            FD_SET(mUpstreamTransaction[i].mUdpFd, aReadFdSet);
            FD_SET(mUpstreamTransaction[i].mUdpFd, aErrorFdSet);
            if (mUpstreamTransaction[i].mUdpFd > *aMaxFd)
            {
                *aMaxFd = mUpstreamTransaction[i].mUdpFd;
            }
        }
    }
}

void Resolver::HandleSelect(const fd_set *aReadFdSet, const fd_set *aErrorFdSet)
{
    for (int i = 0; i < kMaxUpstreamTransactionCount; i++)
    {
        if (mUpstreamTransaction[i].mThreadTxn != nullptr)
        {
            if (FD_ISSET(mUpstreamTransaction[i].mUdpFd, aErrorFdSet))
            {
                close(mUpstreamTransaction[i].mUdpFd);
                DieNow(OT_EXIT_FAILURE);
            }

            if (FD_ISSET(mUpstreamTransaction[i].mUdpFd, aReadFdSet))
            {
                ForwardResponse(&mUpstreamTransaction[i]);
            }
        }
    }
}

} // namespace Posix
} // namespace ot

void otPlatDnsStartUpstreamQuery(otInstance *aInstance, otPlatDnsUpstreamQuery *aTxn, otMessage *aQuery)
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
