/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the platform UDP driver for POSIX.
 *
 *   It provides:
 *     - IPv6 UDP socket lifecycle: create, bind, connect, close.
 *     - Unicast and multicast send/receive via sendmsg()/recvmsg().
 *     - Multicast group join/leave per network interface.
 *     - Integration with the OpenThread POSIX mainloop (select-based).
 *     - Proper cleanup of all open platform sockets on Deinit().
 */

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <openthread/udp.h>
#include <openthread/platform/udp.h>

#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE

#include "posix/platform/infra_if.hpp"
#include "posix/platform/ip6_utils.hpp"
#include "posix/platform/mainloop.hpp"
#include "posix/platform/udp.hpp"
#include "posix/platform/utils.hpp"

using namespace ot::Posix::Ip6Utils;

namespace {

/// Maximum UDP payload size (bytes). Matches the minimum IPv6 MTU.
constexpr size_t kMaxUdpSize = 1280;

// ----------------------------------------------------------------------------
// Helper: convert between a POSIX fd (int) and the opaque mHandle (void *).
// The cast via `long` avoids pointer-size warnings on 64-bit platforms.
// ----------------------------------------------------------------------------

void *FdToHandle(int aFd) { return reinterpret_cast<void *>(static_cast<long>(aFd)); }
int   FdFromHandle(void *aHandle) { return static_cast<int>(reinterpret_cast<long>(aHandle)); }

// ----------------------------------------------------------------------------
// transmitPacket
//
// Sends a UDP payload to the peer described by aMessageInfo using sendmsg()
// with ancillary data for hop limit and source address (IPV6_PKTINFO).
// ----------------------------------------------------------------------------

otError transmitPacket(int aFd, uint8_t *aPayload, uint16_t aLength, const otMessageInfo &aMessageInfo)
{
#ifdef __APPLE__
    // CMSG_SPACE is not a constant expression on macOS; use a fixed safe size.
    constexpr size_t kBufferSize = 128;
#else
    constexpr size_t kBufferSize = CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int));
#endif

    struct sockaddr_in6 peerAddr;
    uint8_t             control[kBufferSize];
    size_t              controlLength = 0;
    struct iovec        iov;
    struct msghdr       msg;
    struct cmsghdr     *cmsg;
    ssize_t             rval;
    otError             error = OT_ERROR_NONE;

    memset(&peerAddr, 0, sizeof(peerAddr));
    peerAddr.sin6_port   = htons(aMessageInfo.mPeerPort);
    peerAddr.sin6_family = AF_INET6;
    CopyIp6AddressTo(aMessageInfo.mPeerAddr, &peerAddr.sin6_addr);

    // sin6_scope_id must only be non-zero for link-local destinations.
    if (IsIp6AddressLinkLocal(aMessageInfo.mPeerAddr))
    {
        if (aMessageInfo.mIsHostInterface)
        {
#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
            peerAddr.sin6_scope_id = ot::Posix::InfraNetif::Get().GetNetifIndex();
#endif
            // Remains 0 if infra interface is not enabled.
        }
        else
        {
            peerAddr.sin6_scope_id = gNetifIndex;
        }
    }

    memset(control, 0, sizeof(control));

    iov.iov_base = aPayload;
    iov.iov_len  = aLength;

    msg.msg_name       = &peerAddr;
    msg.msg_namelen    = sizeof(peerAddr);
    msg.msg_control    = control;
    msg.msg_controllen = static_cast<decltype(msg.msg_controllen)>(sizeof(control));
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    // Ancillary data 1: hop limit (IPV6_HOPLIMIT).
    {
        int hopLimit = (aMessageInfo.mHopLimit ? aMessageInfo.mHopLimit : OPENTHREAD_CONFIG_IP6_HOP_LIMIT_DEFAULT);

        cmsg             = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type  = IPV6_HOPLIMIT;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(int));

        memcpy(CMSG_DATA(cmsg), &hopLimit, sizeof(int));
        controlLength += CMSG_SPACE(sizeof(int));
    }

    // Ancillary data 2: source address (IPV6_PKTINFO), only when the source
    // address is a specific unicast address (not multicast or unspecified).
    if (!IsIp6AddressMulticast(aMessageInfo.mSockAddr) && !IsIp6AddressUnspecified(aMessageInfo.mSockAddr))
    {
        struct in6_pktinfo pktinfo;

        cmsg             = CMSG_NXTHDR(&msg, cmsg);
        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type  = IPV6_PKTINFO;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(pktinfo));

        // For link-local, ifindex must match the scope; 0 is fine for others.
        pktinfo.ipi6_ifindex = peerAddr.sin6_scope_id;
        CopyIp6AddressTo(aMessageInfo.mSockAddr, &pktinfo.ipi6_addr);
        memcpy(CMSG_DATA(cmsg), &pktinfo, sizeof(pktinfo));

        controlLength += CMSG_SPACE(sizeof(pktinfo));
    }

#ifdef __APPLE__
    msg.msg_controllen = static_cast<socklen_t>(controlLength);
#else
    msg.msg_controllen = controlLength;
#endif

    rval = sendmsg(aFd, &msg, 0);

    if (rval < 0)
    {
        // EINVAL can occur when the interface address changes (e.g., child→router
        // role transition). Signal the caller to retry after the transition settles.
        if (errno == EINVAL)
        {
            error = OT_ERROR_INVALID_STATE;
        }
        else
        {
            ot::Posix::Udp::LogWarn("sendmsg() failed: %s", strerror(errno));
            error = OT_ERROR_FAILED;
        }
    }

    return error;
}

// ----------------------------------------------------------------------------
// receivePacket
//
// Receives one UDP datagram from aFd into aPayload, populating aMessageInfo
// with peer address/port, hop limit, and local destination address.
// ----------------------------------------------------------------------------

otError receivePacket(int aFd, uint8_t *aPayload, uint16_t &aLength, otMessageInfo &aMessageInfo)
{
    struct sockaddr_in6 peerAddr;
    uint8_t             control[kMaxUdpSize];
    struct iovec        iov;
    struct msghdr       msg;
    ssize_t             rval;

    memset(&peerAddr, 0, sizeof(peerAddr));
    memset(control,   0, sizeof(control));
    memset(&msg,      0, sizeof(msg));

    iov.iov_base = aPayload;
    iov.iov_len  = aLength;

    msg.msg_name       = &peerAddr;
    msg.msg_namelen    = sizeof(peerAddr);
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    rval = recvmsg(aFd, &msg, 0);

    if (rval <= 0)
    {
        if (rval < 0)
        {
            ot::Posix::Udp::LogWarn("recvmsg() failed: %s", strerror(errno));
        }

        return OT_ERROR_FAILED;
    }

    aLength = static_cast<uint16_t>(rval);

    // Parse ancillary data for hop limit and local packet info.
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level != IPPROTO_IPV6)
        {
            continue;
        }

        if (cmsg->cmsg_type == IPV6_HOPLIMIT)
        {
            int hoplimit;
            memcpy(&hoplimit, CMSG_DATA(cmsg), sizeof(hoplimit));
            aMessageInfo.mHopLimit = static_cast<uint8_t>(hoplimit);
        }
        else if (cmsg->cmsg_type == IPV6_PKTINFO)
        {
            struct in6_pktinfo pktinfo;
            memcpy(&pktinfo, CMSG_DATA(cmsg), sizeof(pktinfo));
            aMessageInfo.mIsHostInterface = (pktinfo.ipi6_ifindex != gNetifIndex);
            ReadIp6AddressFrom(&pktinfo.ipi6_addr, aMessageInfo.mSockAddr);
        }
    }

    aMessageInfo.mPeerPort = ntohs(peerAddr.sin6_port);
    ReadIp6AddressFrom(&peerAddr.sin6_addr, aMessageInfo.mPeerAddr);

    return OT_ERROR_NONE;
}

} // namespace

// ----------------------------------------------------------------------------
// otPlatUdpSocket — allocate a new IPv6/UDP socket and store it in mHandle.
// ----------------------------------------------------------------------------

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    assert(aUdpSocket->mHandle == nullptr);

    fd = ot::Posix::SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, ot::Posix::kSocketNonBlock);
    VerifyOrExit(fd >= 0, error = OT_ERROR_FAILED);

    aUdpSocket->mHandle = FdToHandle(fd);

exit:
    if (error != OT_ERROR_NONE)
    {
        ot::Posix::Udp::LogCrit("Failed to create UDP socket: %s", strerror(errno));
    }

    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpClose — close and invalidate the socket.
// ----------------------------------------------------------------------------

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    // Only close sockets that were opened by the platform (mHandle != nullptr).
    VerifyOrExit(aUdpSocket->mHandle != nullptr);

    fd = FdFromHandle(aUdpSocket->mHandle);

    if (close(fd) != 0)
    {
        ot::Posix::Udp::LogWarn("close() failed for fd=%d: %s", fd, strerror(errno));
        error = OT_ERROR_FAILED;
    }

    aUdpSocket->mHandle = nullptr;

exit:
    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpBind — bind the socket to the port and address in mSockName,
// then enable ancillary data reception for hop limit and packet info.
// ----------------------------------------------------------------------------

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    assert(gNetifIndex != 0);
    assert(aUdpSocket->mHandle != nullptr);
    VerifyOrExit(aUdpSocket->mSockName.mPort != 0, error = OT_ERROR_INVALID_ARGS);

    fd = FdFromHandle(aUdpSocket->mHandle);

    {
        struct sockaddr_in6 sin6;

        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_port   = htons(aUdpSocket->mSockName.mPort);
        sin6.sin6_family = AF_INET6;
        CopyIp6AddressTo(aUdpSocket->mSockName.mAddress, &sin6.sin6_addr);

        VerifyOrExit(bind(fd, reinterpret_cast<struct sockaddr *>(&sin6), sizeof(sin6)) == 0,
                     error = OT_ERROR_FAILED);
    }

    {
        int on = 1;
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)) == 0, error = OT_ERROR_FAILED);
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,  &on, sizeof(on)) == 0, error = OT_ERROR_FAILED);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        ot::Posix::Udp::LogCrit("Failed to bind UDP socket (port=%u): %s",
                                aUdpSocket->mSockName.mPort, strerror(errno));
    }

    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpBindToNetif — bind the socket to a specific network interface.
// ----------------------------------------------------------------------------

otError otPlatUdpBindToNetif(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier)
{
    otError error = OT_ERROR_NONE;
    int     fd    = FdFromHandle(aUdpSocket->mHandle);
    int     one   = 1;
    int     zero  = 0;

    switch (aNetifIdentifier)
    {
    case OT_NETIF_UNSPECIFIED:
    {
#ifdef __linux__
        VerifyOrExit(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, nullptr, 0) == 0, error = OT_ERROR_FAILED);
#else
        unsigned int netifIndex = 0;
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &netifIndex, sizeof(netifIndex)) == 0,
                     error = OT_ERROR_FAILED);
#endif
        break;
    }

    case OT_NETIF_THREAD_HOST:
    {
#ifdef __linux__
        VerifyOrExit(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, gNetifName, strlen(gNetifName)) == 0,
                     error = OT_ERROR_FAILED);
#else
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &gNetifIndex, sizeof(gNetifIndex)) == 0,
                     error = OT_ERROR_FAILED);
#endif
        break;
    }

    case OT_NETIF_BACKBONE:
    {
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        const char *infraName = otSysGetInfraNetifName();

        if (infraName == nullptr || infraName[0] == '\0')
        {
            ot::Posix::Udp::LogWarn("No backbone interface configured; %s() aborted.", __func__);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

#ifdef __linux__
        VerifyOrExit(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, infraName, strlen(infraName)) == 0,
                     error = OT_ERROR_FAILED);
#else
        uint32_t backboneNetifIndex = otSysGetInfraNetifIndex();
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &backboneNetifIndex, sizeof(backboneNetifIndex)) == 0,
                     error = OT_ERROR_FAILED);
#endif
        // Backbone sockets need multicast hops set so multicast TTL is correct.
        VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &one, sizeof(one)) == 0,
                     error = OT_ERROR_FAILED);
#else
        ExitNow(error = OT_ERROR_NOT_IMPLEMENTED);
#endif
        break;
    }

    case OT_NETIF_THREAD_INTERNAL:
        // THREAD_INTERNAL sockets are managed entirely within the stack;
        // they should never reach this platform-level bind call.
        assert(false);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // Suppress local multicast loopback by default for all interfaces.
    VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &zero, sizeof(zero)) == 0,
                 error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        ot::Posix::Udp::LogWarn("otPlatUdpBindToNetif() failed: %s", strerror(errno));
    }

    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpConnect — connect the socket to a remote peer, or disconnect it.
// ----------------------------------------------------------------------------

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
    otError             error = OT_ERROR_NONE;
    struct sockaddr_in6 sin6;
    int                 fd;
    bool                isDisconnect;

    VerifyOrExit(aUdpSocket->mHandle != nullptr, error = OT_ERROR_INVALID_ARGS);

    fd           = FdFromHandle(aUdpSocket->mHandle);
    isDisconnect = IsIp6AddressUnspecified(aUdpSocket->mPeerName.mAddress) && (aUdpSocket->mPeerName.mPort == 0);

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_port = htons(aUdpSocket->mPeerName.mPort);

    if (!isDisconnect)
    {
        sin6.sin6_family = AF_INET6;
        CopyIp6AddressTo(aUdpSocket->mPeerName.mAddress, &sin6.sin6_addr);
    }
    else
    {
#ifdef __APPLE__
        sin6.sin6_family = AF_UNSPEC;
#else
        // Linux has a kernel bug where connecting to AF_UNSPEC does not
        // properly disconnect. Work around by recreating the socket.
        char      netifName[IFNAMSIZ];
        socklen_t len = sizeof(netifName);

        memset(netifName, 0, sizeof(netifName));

        if (getsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, netifName, &len) != 0)
        {
            ot::Posix::Udp::LogWarn("getsockopt(SO_BINDTODEVICE) failed: %s", strerror(errno));
            len = 0;
        }

        SuccessOrExit(error = otPlatUdpClose(aUdpSocket));
        SuccessOrExit(error = otPlatUdpSocket(aUdpSocket));
        SuccessOrExit(error = otPlatUdpBind(aUdpSocket));

        if (len > 0 && netifName[0] != '\0')
        {
            fd = FdFromHandle(aUdpSocket->mHandle);
            VerifyOrExit(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, netifName, len) == 0,
                         {
                             ot::Posix::Udp::LogWarn("SO_BINDTODEVICE restore failed: %s", strerror(errno));
                             error = OT_ERROR_FAILED;
                         });
        }

        ExitNow();
#endif
    }

    if (connect(fd, reinterpret_cast<struct sockaddr *>(&sin6), sizeof(sin6)) != 0)
    {
#ifdef __APPLE__
        // On macOS, EAFNOSUPPORT is expected when disconnecting via AF_UNSPEC.
        VerifyOrExit(errno == EAFNOSUPPORT && isDisconnect);
#endif
        ot::Posix::Udp::LogWarn("connect() to [%s]:%u failed: %s",
                                Ip6AddressString(&aUdpSocket->mPeerName.mAddress).AsCString(),
                                aUdpSocket->mPeerName.mPort, strerror(errno));
        error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpSend — read the message payload and transmit it via sendmsg().
// Temporarily enables IPV6_MULTICAST_LOOP if mMulticastLoop is set.
// ----------------------------------------------------------------------------

otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError  error = OT_ERROR_NONE;
    int      fd;
    uint16_t len;
    uint8_t  payload[kMaxUdpSize];

    VerifyOrExit(aUdpSocket->mHandle != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMessage != nullptr,            error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aMessageInfo != nullptr,        error = OT_ERROR_INVALID_ARGS);

    fd  = FdFromHandle(aUdpSocket->mHandle);
    len = otMessageGetLength(aMessage);

    VerifyOrExit(len <= kMaxUdpSize, error = OT_ERROR_NO_BUFS);
    VerifyOrExit(len == otMessageRead(aMessage, 0, payload, len), error = OT_ERROR_PARSE);

    if (aMessageInfo->mMulticastLoop)
    {
        int value = 1;
        VerifyOrDie(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &value, sizeof(value)) == 0,
                    OT_EXIT_ERROR_ERRNO);
    }

    error = transmitPacket(fd, payload, len, *aMessageInfo);

    if (aMessageInfo->mMulticastLoop)
    {
        int value = 0;
        VerifyOrDie(setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &value, sizeof(value)) == 0,
                    OT_EXIT_ERROR_ERRNO);
    }

exit:
    // Free the message regardless of transmit success — ownership always transfers.
    if (aMessage != nullptr)
    {
        otMessageFree(aMessage);
    }

    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpJoinMulticastGroup — subscribe the socket to a multicast group.
// EADDRINUSE is tolerated (already a member).
// ----------------------------------------------------------------------------

otError otPlatUdpJoinMulticastGroup(otUdpSocket        *aUdpSocket,
                                    otNetifIdentifier   aNetifIdentifier,
                                    const otIp6Address *aAddress)
{
    otError          error = OT_ERROR_NONE;
    struct ipv6_mreq mreq  = {};
    int              fd;

    VerifyOrExit(aUdpSocket->mHandle != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aAddress != nullptr,            error = OT_ERROR_INVALID_ARGS);

    fd = FdFromHandle(aUdpSocket->mHandle);
    CopyIp6AddressTo(*aAddress, &mreq.ipv6mr_multiaddr);

    switch (aNetifIdentifier)
    {
    case OT_NETIF_UNSPECIFIED:
        mreq.ipv6mr_interface = 0;
        break;
    case OT_NETIF_THREAD_HOST:
        mreq.ipv6mr_interface = gNetifIndex;
        break;
    case OT_NETIF_BACKBONE:
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        mreq.ipv6mr_interface = otSysGetInfraNetifIndex();
#else
        ExitNow(error = OT_ERROR_NOT_IMPLEMENTED);
#endif
        break;
    case OT_NETIF_THREAD_INTERNAL:
        assert(false);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) == 0 || errno == EADDRINUSE,
                 error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        ot::Posix::Udp::LogCrit("IPV6_JOIN_GROUP failed: %s", strerror(errno));
    }

    return error;
}

// ----------------------------------------------------------------------------
// otPlatUdpLeaveMulticastGroup — unsubscribe the socket from a multicast group.
// EADDRINUSE is tolerated (already left / never joined).
// ----------------------------------------------------------------------------

otError otPlatUdpLeaveMulticastGroup(otUdpSocket        *aUdpSocket,
                                     otNetifIdentifier   aNetifIdentifier,
                                     const otIp6Address *aAddress)
{
    otError          error = OT_ERROR_NONE;
    struct ipv6_mreq mreq  = {};
    int              fd;

    VerifyOrExit(aUdpSocket->mHandle != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aAddress != nullptr,            error = OT_ERROR_INVALID_ARGS);

    fd = FdFromHandle(aUdpSocket->mHandle);
    CopyIp6AddressTo(*aAddress, &mreq.ipv6mr_multiaddr);

    switch (aNetifIdentifier)
    {
    case OT_NETIF_UNSPECIFIED:
        mreq.ipv6mr_interface = 0;
        break;
    case OT_NETIF_THREAD_HOST:
        mreq.ipv6mr_interface = gNetifIndex;
        break;
    case OT_NETIF_BACKBONE:
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        mreq.ipv6mr_interface = otSysGetInfraNetifIndex();
#else
        ExitNow(error = OT_ERROR_NOT_IMPLEMENTED);
#endif
        break;
    case OT_NETIF_THREAD_INTERNAL:
        assert(false);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(setsockopt(fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq)) == 0 || errno == EADDRINUSE,
                 error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        ot::Posix::Udp::LogCrit("IPV6_LEAVE_GROUP failed: %s", strerror(errno));
    }

    return error;
}

// ============================================================================
// ot::Posix::Udp — mainloop integration class
// ============================================================================

namespace ot {
namespace Posix {

const char Udp::kLogModuleName[] = "Udp";

// ----------------------------------------------------------------------------
// Update — called each mainloop iteration to register readable fds with select.
// ----------------------------------------------------------------------------

void Udp::Update(Mainloop::Context &aContext)
{
    VerifyOrExit(gNetifIndex != 0);

    for (otUdpSocket *socket = otUdpGetSockets(gInstance); socket != nullptr; socket = socket->mNext)
    {
        if (socket->mHandle == nullptr)
        {
            continue;
        }

        int fd = FdFromHandle(socket->mHandle);
        Mainloop::AddToReadFdSet(fd, aContext);
    }

exit:
    return;
}

// ----------------------------------------------------------------------------
// Init — resolves the network interface name to an index and stores both.
// ----------------------------------------------------------------------------

void Udp::Init(const char *aIfName)
{
    if (aIfName == nullptr)
    {
        DieNow(OT_EXIT_INVALID_ARGUMENTS);
    }

    if (aIfName != gNetifName)
    {
        VerifyOrDie(strlen(aIfName) < sizeof(gNetifName) - 1, OT_EXIT_INVALID_ARGUMENTS);
        assert(gNetifIndex == 0);
        strcpy(gNetifName, aIfName);
        gNetifIndex = if_nametoindex(gNetifName);
        VerifyOrDie(gNetifIndex != 0, OT_EXIT_ERROR_ERRNO);
    }

    assert(gNetifIndex != 0);
}

void Udp::SetUp(void)   { Mainloop::Manager::Get().Add(*this); }
void Udp::TearDown(void){ Mainloop::Manager::Get().Remove(*this); }

// ----------------------------------------------------------------------------
// Deinit — close all open platform UDP sockets to prevent fd leaks.
//
// Previously a TODO; now iterates all registered sockets and closes any that
// still hold a valid platform handle.
// ----------------------------------------------------------------------------

void Udp::Deinit(void)
{
    for (otUdpSocket *socket = otUdpGetSockets(gInstance); socket != nullptr; socket = socket->mNext)
    {
        if (socket->mHandle != nullptr)
        {
            int fd = FdFromHandle(socket->mHandle);
            LogInfo("Closing platform UDP socket fd=%d on Deinit", fd);
            close(fd);
            socket->mHandle = nullptr;
        }
    }
}

// ----------------------------------------------------------------------------
// Get — returns the singleton Udp instance.
// ----------------------------------------------------------------------------

Udp &Udp::Get(void)
{
    static Udp sInstance;
    return sInstance;
}

// ----------------------------------------------------------------------------
// Process — called after select() returns; reads one datagram per readable
// socket, assembles an otMessage, and dispatches it to the socket handler.
//
// Improvement: explicit fd validity check changed from `fd > 0` to `fd >= 0`
// since fd=0 (stdin) is technically valid, though uncommon in this context.
// ----------------------------------------------------------------------------

void Udp::Process(const Mainloop::Context &aContext)
{
    otMessageSettings msgSettings = {false, OT_MESSAGE_PRIORITY_NORMAL};

    for (otUdpSocket *socket = otUdpGetSockets(gInstance); socket != nullptr; socket = socket->mNext)
    {
        if (socket->mHandle == nullptr)
        {
            continue;
        }

        int fd = FdFromHandle(socket->mHandle);

        // fd >= 0: corrected from original `fd > 0` (fd=0 is valid on POSIX).
        if (fd >= 0 && Mainloop::IsFdReadable(fd, aContext))
        {
            otMessageInfo messageInfo;
            otMessage    *message = nullptr;
            uint8_t       payload[kMaxUdpSize];
            uint16_t      length = sizeof(payload);

            memset(&messageInfo, 0, sizeof(messageInfo));
            messageInfo.mSockPort = socket->mSockName.mPort;

            if (receivePacket(fd, payload, length, messageInfo) != OT_ERROR_NONE)
            {
                continue;
            }

            message = otUdpNewMessage(gInstance, &msgSettings);

            if (message == nullptr)
            {
                LogWarn("Failed to allocate otMessage for incoming UDP packet (fd=%d)", fd);
                continue;
            }

            if (otMessageAppend(message, payload, length) != OT_ERROR_NONE)
            {
                LogWarn("otMessageAppend failed for incoming UDP packet (fd=%d, len=%u)", fd, length);
                otMessageFree(message);
                continue;
            }

            socket->mHandler(socket->mContext, message, &messageInfo);
            otMessageFree(message);

            // Process one socket per mainloop iteration to avoid starvation.
            break;
        }
    }
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE