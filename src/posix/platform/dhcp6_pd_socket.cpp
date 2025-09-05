/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "dhcp6_pd_socket.hpp"

#if OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <openthread/platform/infra_if.h>

#include "infra_if.hpp"
#include "ip6_utils.hpp"
#include "platform-posix.h"
#include "common/code_utils.hpp"

extern "C" void otPlatInfraIfDhcp6PdClientSetListeningEnabled(otInstance *aInstance,
                                                              bool        aEnable,
                                                              uint32_t    aInfraIfIndex)
{
    ot::Posix::InfraNetif::GetDhcp6PdSocket().SetListeningEnabled(aInstance, aEnable, aInfraIfIndex);
}

extern "C" void otPlatInfraIfDhcp6PdClientSend(otInstance   *aInstance,
                                               otMessage    *aMessage,
                                               otIp6Address *aDestAddress,
                                               uint32_t      aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    return ot::Posix::InfraNetif::GetDhcp6PdSocket().Send(aMessage, aDestAddress, aInfraIfIndex);
}

namespace ot {
namespace Posix {

using namespace ot::Posix::Ip6Utils;

const char Dhcp6PdSocket::kLogModuleName[] = "Dhcp6PdSocket";

void Dhcp6PdSocket::Init(void)
{
    mEnabled      = false;
    mInfraIfIndex = 0;
    mFd6          = -1;
    mPendingTx    = false;

    // `All_DHCP_Relay_Agents_and_Servers` "ff02::1:2"
    memset(&mMulticastAddress, 0, sizeof(otIp6Address));
    mMulticastAddress.mFields.m8[0]  = 0xff;
    mMulticastAddress.mFields.m8[1]  = 0x02;
    mMulticastAddress.mFields.m8[13] = 0x01;
    mMulticastAddress.mFields.m8[15] = 0x02;

    memset(&mTxQueue, 0, sizeof(mTxQueue));
}

void Dhcp6PdSocket::SetUp(void) { otMessageQueueInit(&mTxQueue); }

void Dhcp6PdSocket::TearDown(void)
{
    if (mEnabled)
    {
        ClearTxQueue();
        mEnabled = false;
    }
}

void Dhcp6PdSocket::Deinit(void) { CloseSocket(); }

void Dhcp6PdSocket::Update(otSysMainloopContext &aContext)
{
    VerifyOrExit(mEnabled);

    FD_SET(mFd6, &aContext.mReadFdSet);

    if (mPendingTx)
    {
        FD_SET(mFd6, &aContext.mWriteFdSet);
    }

    if (aContext.mMaxFd < mFd6)
    {
        aContext.mMaxFd = mFd6;
    }

exit:
    return;
}

void Dhcp6PdSocket::Process(const otSysMainloopContext &aContext)
{
    VerifyOrExit(mEnabled);

    if (FD_ISSET(mFd6, &aContext.mWriteFdSet))
    {
        SendQueuedMessages();
    }

    if (FD_ISSET(mFd6, &aContext.mReadFdSet))
    {
        ReceiveMessage();
    }

exit:
    return;
}

void Dhcp6PdSocket::SetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    VerifyOrExit(aEnable != mEnabled);
    mInstance = aInstance;

    if (aEnable)
    {
        Enable(aInfraIfIndex);
    }
    else
    {
        Disable(aInfraIfIndex);
    }

exit:
    return;
}

void Dhcp6PdSocket::Enable(uint32_t aInfraIfIndex)
{
    SuccessOrDie(OpenSocket(aInfraIfIndex));
    SuccessOrDie(JoinOrLeaveMulticastGroup(/* aJoin */ true, aInfraIfIndex));

    mEnabled      = true;
    mInfraIfIndex = aInfraIfIndex;

    LogInfo("Enabled");
}

void Dhcp6PdSocket::Disable(uint32_t aInfraIfIndex)
{
    ClearTxQueue();

    IgnoreError(JoinOrLeaveMulticastGroup(/* aJoin */ false, aInfraIfIndex));
    CloseSocket();

    mEnabled = false;

    LogInfo("Disabled");
}

void Dhcp6PdSocket::Send(otMessage *aMessage, const otIp6Address *aAddress, uint32_t aInfraIfIndex)
{
    Metadata metadata;
    uint16_t length;

    VerifyOrExit(mEnabled);
    VerifyOrExit(aInfraIfIndex == mInfraIfIndex);

    length = otMessageGetLength(aMessage);

    if (length > kMaxMessageLength)
    {
        LogWarn("Msg length %u is longer than max %u", length, kMaxMessageLength);
        ExitNow();
    }

    memset(&metadata, 0, sizeof(Metadata));

    metadata.mAddress = *aAddress;
    metadata.mPort    = kServerPort;

    SuccessOrExit(otMessageAppend(aMessage, &metadata, sizeof(Metadata)));

    mPendingTx = true;

    otMessageQueueEnqueue(&mTxQueue, aMessage);
    aMessage = NULL;

exit:
    if (aMessage != NULL)
    {
        otMessageFree(aMessage);
    }
}

void Dhcp6PdSocket::ClearTxQueue(void)
{
    otMessage *message;

    while ((message = otMessageQueueGetHead(&mTxQueue)) != NULL)
    {
        otMessageQueueDequeue(&mTxQueue, message);
        otMessageFree(message);
    }

    mPendingTx = false;
}

void Dhcp6PdSocket::SendQueuedMessages(void)
{
    otMessage *message;

    while ((message = otMessageQueueGetHead(&mTxQueue)) != nullptr)
    {
        uint16_t            length;
        uint16_t            offset;
        int                 bytesSent;
        Metadata            metadata;
        uint8_t             buffer[kMaxMessageLength];
        struct sockaddr_in6 addr6;

        length = otMessageGetLength(message);

        offset = length - sizeof(Metadata);
        length -= sizeof(Metadata);

        otMessageRead(message, offset, &metadata, sizeof(Metadata));
        otMessageRead(message, 0, buffer, length);

        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port   = htons(metadata.mPort);
        CopyIp6AddressTo(metadata.mAddress, &addr6.sin6_addr);

        bytesSent = sendto(mFd6, buffer, length, 0, reinterpret_cast<struct sockaddr *>(&addr6), sizeof(addr6));
        VerifyOrExit(bytesSent == length);

        otMessageQueueDequeue(&mTxQueue, message);
        otMessageFree(message);
    }

    mPendingTx = false;

exit:
    return;
}

void Dhcp6PdSocket::ReceiveMessage(void)
{
    otMessage          *message = nullptr;
    uint8_t             buffer[kMaxMessageLength];
    uint16_t            length = 0;
    struct sockaddr_in6 sockaddr6;
    socklen_t           len;
    ssize_t             rval;

    len = sizeof(sockaddr6);
    memset(&sockaddr6, 0, sizeof(sockaddr6));

    rval = recvfrom(mFd6, reinterpret_cast<char *>(&buffer), sizeof(buffer), 0,
                    reinterpret_cast<struct sockaddr *>(&sockaddr6), &len);

    VerifyOrExit(rval >= 0, LogCrit("recvfrom() for IPv6 socket failed, errno: %s", strerror(errno)));
    length = static_cast<uint16_t>(rval);

    VerifyOrExit(length > 0);

    message = otIp6NewMessage(mInstance, nullptr);
    VerifyOrExit(message != nullptr);

    SuccessOrExit(otMessageAppend(message, buffer, length));

    otPlatInfraIfDhcp6PdClientHandleReceived(mInstance, message, mInfraIfIndex);

    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Socket helpers

otError Dhcp6PdSocket::OpenSocket(uint32_t aInfraIfIndex)
{
    otError             error = OT_ERROR_FAILED;
    struct sockaddr_in6 addr6;
    int                 fd;
    int                 ifindex = static_cast<int>(aInfraIfIndex);

    fd = socket(AF_INET6, SOCK_DGRAM, 0);
    VerifyOrExit(fd >= 0, LogCrit("Failed to create socket"));

#ifdef __linux__
    {
        char        nameBuffer[IF_NAMESIZE];
        const char *ifname;

        ifname = if_indextoname(aInfraIfIndex, nameBuffer);
        VerifyOrExit(ifname != NULL, LogCrit("if_indextoname() failed"));

        error = SetSocketOptionValue(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname), "SO_BINDTODEVICE");
        SuccessOrExit(error);
    }
#else
    {
        SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_BOUND_IF, ifindex, "IPV6_BOUND_IF"));
    }
#endif

    SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 255, "IPV6_MULTICAST_HOPS"));
    SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, 255, "IPV6_UNICAST_HOPS"));
    SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_V6ONLY, 1, "IPV6_V6ONLY"));
    SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, ifindex, "IPV6_MULTICAST_IF"));
    SuccessOrExit(error = SetSocketOption<int>(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 1, "IPV6_MULTICAST_LOOP"));
    SuccessOrExit(error = SetReuseAddrPortOptions(fd));

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port   = htons(kClientPort);

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr6), sizeof(addr6)) < 0)
    {
        LogCrit("bind() to DHCPv6 Client port for IPv6 socket failed, errno: %s", strerror(errno));
        error = OT_ERROR_FAILED;
        ExitNow();
    }

    mFd6 = fd;

    LogInfo("Successfully opened IPv6 socket");

exit:
    return error;
}

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

otError Dhcp6PdSocket::JoinOrLeaveMulticastGroup(bool aJoin, uint32_t aInfraIfIndex)
{
    struct ipv6_mreq mreq6;

    memset(&mreq6, 0, sizeof(mreq6));
    Ip6Utils::CopyIp6AddressTo(mMulticastAddress, &mreq6.ipv6mr_multiaddr);

    mreq6.ipv6mr_interface = static_cast<int>(aInfraIfIndex);

    if (aJoin)
    {
        // Suggested workaround for netif not dropping
        // a previous multicast membership.
        setsockopt(mFd6, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq6, sizeof(mreq6));
    }

    return SetSocketOptionValue(mFd6, IPPROTO_IPV6, aJoin ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP, &mreq6,
                                sizeof(mreq6), "IP6_ADD/DROP_MEMBERSHIP");
}

void Dhcp6PdSocket::CloseSocket(void)
{
    if (mFd6 >= 0)
    {
        close(mFd6);
        mFd6 = -1;
    }
}

otError Dhcp6PdSocket::SetReuseAddrPortOptions(int aFd)
{
    otError error;

    SuccessOrExit(error = SetSocketOption<int>(aFd, SOL_SOCKET, SO_REUSEADDR, 1, "SO_REUSEADDR"));
    SuccessOrExit(error = SetSocketOption<int>(aFd, SOL_SOCKET, SO_REUSEPORT, 1, "SO_REUSEPORT"));

exit:
    return error;
}

otError Dhcp6PdSocket::SetSocketOptionValue(int         aFd,
                                            int         aLevel,
                                            int         aOption,
                                            const void *aValue,
                                            uint32_t    aValueLength,
                                            const char *aOptionName)
{
    otError error = OT_ERROR_NONE;

    if (setsockopt(aFd, aLevel, aOption, aValue, aValueLength) != 0)
    {
        error = OT_ERROR_FAILED;
        LogCrit("Failed to setsockopt(%s) - errno: %s", aOptionName, strerror(errno));
    }

    return error;
}

} // namespace Posix
} // namespace ot

#endif // OT_POSIX_CONFIG_DHCP6_PD_SOCKET_ENABLE
