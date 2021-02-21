/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements the infrastructure interface for posix.
 */

#include "platform-posix.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include <errno.h>
#include <ifaddrs.h>
// clang-format off
#include <netinet/in.h>
#include <netinet/icmp6.h>
// clang-format on
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/rtnetlink.h>
#endif

#include <openthread/border_router.h>
#include <openthread/platform/infra_if.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "lib/platform/exit_code.h"

static char         sInfraIfName[IFNAMSIZ];
static uint32_t     sInfraIfIndex       = 0;
static int          sInfraIfIcmp6Socket = -1;
static int          sNetLinkSocket      = -1;
static otIp6Address sInfraIfLinkLocalAddr;

static int  CreateIcmp6Socket(void);
static int  CreateNetLinkSocket(void);
static void ReceiveNetLinkMessage(otInstance *aInstance);
static void ReceiveIcmp6Message(otInstance *aInstance);

otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t *     aBuffer,
                                 uint16_t            aBufferLength)
{
    otError error = OT_ERROR_NONE;

    struct iovec        iov;
    struct in6_pktinfo *packetInfo;

    int                 hopLimit = 255;
    uint8_t             cmsgBuffer[CMSG_SPACE(sizeof(*packetInfo)) + CMSG_SPACE(sizeof(hopLimit))];
    struct msghdr       msgHeader;
    struct cmsghdr *    cmsgPointer;
    ssize_t             rval;
    struct sockaddr_in6 dest;

    VerifyOrExit(sInfraIfIcmp6Socket >= 0, error = OT_ERROR_FAILED);
    VerifyOrExit(aInfraIfIndex == sInfraIfIndex, error = OT_ERROR_DROP);

    memset(cmsgBuffer, 0, sizeof(cmsgBuffer));

    // Send the message
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    memcpy(&dest.sin6_addr, aDestAddress, sizeof(*aDestAddress));
    if (IN6_IS_ADDR_LINKLOCAL(&dest.sin6_addr) || IN6_IS_ADDR_MC_LINKLOCAL(&dest.sin6_addr))
    {
        dest.sin6_scope_id = sInfraIfIndex;
    }

    iov.iov_base = const_cast<uint8_t *>(aBuffer);
    iov.iov_len  = aBufferLength;

    msgHeader.msg_namelen    = sizeof(dest);
    msgHeader.msg_name       = &dest;
    msgHeader.msg_iov        = &iov;
    msgHeader.msg_iovlen     = 1;
    msgHeader.msg_control    = cmsgBuffer;
    msgHeader.msg_controllen = sizeof(cmsgBuffer);

    // Specify the interface.
    cmsgPointer             = CMSG_FIRSTHDR(&msgHeader);
    cmsgPointer->cmsg_level = IPPROTO_IPV6;
    cmsgPointer->cmsg_type  = IPV6_PKTINFO;
    cmsgPointer->cmsg_len   = CMSG_LEN(sizeof(*packetInfo));
    packetInfo              = (struct in6_pktinfo *)CMSG_DATA(cmsgPointer);
    memset(packetInfo, 0, sizeof(*packetInfo));
    packetInfo->ipi6_ifindex = sInfraIfIndex;

    // Per section 6.1.2 of RFC 4861, we need to send the ICMPv6 message with IP Hop Limit 255.
    cmsgPointer             = CMSG_NXTHDR(&msgHeader, cmsgPointer);
    cmsgPointer->cmsg_level = IPPROTO_IPV6;
    cmsgPointer->cmsg_type  = IPV6_HOPLIMIT;
    cmsgPointer->cmsg_len   = CMSG_LEN(sizeof(hopLimit));
    memcpy(CMSG_DATA(cmsgPointer), &hopLimit, sizeof(hopLimit));

    rval = sendmsg(sInfraIfIcmp6Socket, &msgHeader, 0);
    if (rval < 0)
    {
        otLogWarnPlat("failed to send ICMPv6 message: %s", strerror(errno));
        ExitNow(error = OT_ERROR_FAILED);
    }

    if (static_cast<size_t>(rval) != iov.iov_len)
    {
        otLogWarnPlat("failed to send ICMPv6 message: partially sent");
        ExitNow(error = OT_ERROR_FAILED);
    }

exit:
    return error;
}

const otIp6Address *platformInfraIfGetLinkLocalAddress(void)
{
    const otIp6Address *ret     = nullptr;
    struct ifaddrs *    ifAddrs = nullptr;

    VerifyOrDie(getifaddrs(&ifAddrs) != -1, OT_EXIT_ERROR_ERRNO);

    for (struct ifaddrs *addr = ifAddrs; addr != nullptr; addr = addr->ifa_next)
    {
        struct sockaddr_in6 *ip6Addr;

        if (strncmp(addr->ifa_name, sInfraIfName, sizeof(sInfraIfName)) != 0 || addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        ip6Addr = reinterpret_cast<sockaddr_in6 *>(addr->ifa_addr);
        if (IN6_IS_ADDR_LINKLOCAL(&ip6Addr->sin6_addr))
        {
            memcpy(&sInfraIfLinkLocalAddr, &ip6Addr->sin6_addr, sizeof(sInfraIfLinkLocalAddr));
            ExitNow(ret = &sInfraIfLinkLocalAddr);
        }
    }

    otLogWarnPlat("cannot find IPv6 link-local address for interface %s", sInfraIfName);

exit:
    freeifaddrs(ifAddrs);
    return ret;
}

bool platformInfraIfIsRunning(void)
{
    int          sock;
    struct ifreq ifReq;

    OT_ASSERT(sInfraIfIndex != 0);

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    memset(&ifReq, 0, sizeof(ifReq));
    strncpy(ifReq.ifr_name, sInfraIfName, sizeof(ifReq.ifr_name));
    VerifyOrDie(ioctl(sock, SIOCGIFFLAGS, &ifReq) != -1, OT_EXIT_ERROR_ERRNO);

    close(sock);

    return (ifReq.ifr_flags & IFF_RUNNING);
}

static int CreateIcmp6Socket(void)
{
    int                 sock;
    int                 rval;
    struct icmp6_filter filter;
    const int           kEnable             = 1;
    const int           kIpv6ChecksumOffset = 2;
    const int           kHopLimit           = 255;

    // Initializes the ICMPv6 socket.
    sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    // Only accept router advertisements and solicitations.
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);

    rval = setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    // We want a source address and interface index.
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &kEnable, sizeof(kEnable));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

#ifdef __linux__
    rval = setsockopt(sock, IPPROTO_RAW, IPV6_CHECKSUM, &kIpv6ChecksumOffset, sizeof(kIpv6ChecksumOffset));
#else
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_CHECKSUM, &kIpv6ChecksumOffset, sizeof(kIpv6ChecksumOffset));
#endif
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    // We need to be able to reject RAs arriving from off-link.
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &kEnable, sizeof(kEnable));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &kHopLimit, sizeof(kHopLimit));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &kHopLimit, sizeof(kHopLimit));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    return sock;
}

uint32_t platformInfraIfInit(otInstance *aInstance, const char *aIfName)
{
    OT_UNUSED_VARIABLE(aInstance);

    ssize_t  rval;
    uint32_t ifIndex = 0;

    if (strlen(aIfName) >= sizeof(sInfraIfName))
    {
        otLogCritPlat("infra interface name '%s' is too long", aIfName);
        DieNow(OT_EXIT_INVALID_ARGUMENTS);
    }
    strcpy(sInfraIfName, aIfName);

    // Initializes the infra interface.
    ifIndex = if_nametoindex(aIfName);
    if (ifIndex == 0)
    {
        otLogCritPlat("failed to get the index for infra interface %s", aIfName);
        DieNow(OT_EXIT_INVALID_ARGUMENTS);
    }
    sInfraIfIndex = ifIndex;

    sInfraIfIcmp6Socket = CreateIcmp6Socket();
#ifdef __linux__
    rval = setsockopt(sInfraIfIcmp6Socket, SOL_SOCKET, SO_BINDTODEVICE, sInfraIfName, strlen(sInfraIfName));
#else  // __NetBSD__ || __FreeBSD__ || __APPLE__
    rval = setsockopt(sInfraIfIcmp6Socket, IPPROTO_IP, IP_BOUND_IF, &sInfraIfIndex, sizeof(sInfraIfIndex));
#endif // __linux__
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    sNetLinkSocket = CreateNetLinkSocket();

    return sInfraIfIndex;
}

void platformInfraIfDeinit(void)
{
    if (sInfraIfIcmp6Socket != -1)
    {
        close(sInfraIfIcmp6Socket);
        sInfraIfIcmp6Socket = -1;
    }

    if (sNetLinkSocket != -1)
    {
        close(sNetLinkSocket);
        sNetLinkSocket = -1;
    }

    sInfraIfIndex = 0;
}

void platformInfraIfUpdateFdSet(fd_set &aReadFdSet, int &aMaxFd)
{
    VerifyOrExit(sInfraIfIcmp6Socket != -1);
    VerifyOrExit(sNetLinkSocket != -1);

    FD_SET(sInfraIfIcmp6Socket, &aReadFdSet);
    aMaxFd = OT_MAX(aMaxFd, sInfraIfIcmp6Socket);

    FD_SET(sNetLinkSocket, &aReadFdSet);
    aMaxFd = OT_MAX(aMaxFd, sNetLinkSocket);

exit:
    return;
}

// Create a net-link socket that subscribes to link & addresses events.
static int CreateNetLinkSocket(void)
{
    int                sock;
    int                rval;
    struct sockaddr_nl addr;

    sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV6_IFADDR;
    addr.nl_pid    = getpid();

    rval = bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    return sock;
}

static void ReceiveNetLinkMessage(otInstance *aInstance)
{
    const size_t kMaxNetLinkBufSize = 8192;
    ssize_t      len;
    union
    {
        nlmsghdr mHeader;
        uint8_t  mBuffer[kMaxNetLinkBufSize];
    } msgBuffer;

    len = recv(sNetLinkSocket, msgBuffer.mBuffer, sizeof(msgBuffer.mBuffer), 0);
    if (len < 0)
    {
        otLogCritPlat("failed to receive netlink message: %s", strerror(errno));
        ExitNow();
    }

    for (struct nlmsghdr *header = &msgBuffer.mHeader; NLMSG_OK(header, static_cast<size_t>(len));
         header                  = NLMSG_NEXT(header, len))
    {
        switch (header->nlmsg_type)
        {
        case RTM_NEWLINK:
        case RTM_DELLINK:
        case RTM_NEWADDR:
        case RTM_DELADDR:
            SuccessOrDie(otPlatInfraIfStateChanged(aInstance, sInfraIfIndex, platformInfraIfIsRunning(),
                                                   platformInfraIfGetLinkLocalAddress()));
            break;
        case NLMSG_ERROR:
        {
            struct nlmsgerr *errMsg = reinterpret_cast<struct nlmsgerr *>(NLMSG_DATA(header));

            OT_UNUSED_VARIABLE(errMsg);
            otLogWarnPlat("netlink NLMSG_ERROR response: seq=%u, error=%d", header->nlmsg_seq, errMsg->error);
        }
        default:
            break;
        }
    }

exit:
    return;
}

static void ReceiveIcmp6Message(otInstance *aInstance)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  buffer[1500];
    uint16_t bufferLength;

    ssize_t         rval;
    struct msghdr   msg;
    struct iovec    bufp;
    char            cmsgbuf[128];
    struct cmsghdr *cmh;
    uint32_t        ifIndex  = 0;
    int             hopLimit = -1;

    struct sockaddr_in6 srcAddr;
    struct in6_addr     dstAddr;

    memset(&srcAddr, 0, sizeof(srcAddr));
    memset(&dstAddr, 0, sizeof(dstAddr));

    bufp.iov_base      = buffer;
    bufp.iov_len       = sizeof(buffer);
    msg.msg_iov        = &bufp;
    msg.msg_iovlen     = 1;
    msg.msg_name       = &srcAddr;
    msg.msg_namelen    = sizeof(srcAddr);
    msg.msg_control    = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    rval = recvmsg(sInfraIfIcmp6Socket, &msg, 0);
    if (rval < 0)
    {
        otLogWarnPlat("failed to receive ICMPv6 message: %s", strerror(errno));
        ExitNow(error = OT_ERROR_DROP);
    }

    bufferLength = static_cast<uint16_t>(rval);

    for (cmh = CMSG_FIRSTHDR(&msg); cmh; cmh = CMSG_NXTHDR(&msg, cmh))
    {
        if (cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_PKTINFO &&
            cmh->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo)))
        {
            struct in6_pktinfo pktinfo;

            memcpy(&pktinfo, CMSG_DATA(cmh), sizeof pktinfo);
            ifIndex = pktinfo.ipi6_ifindex;
            dstAddr = pktinfo.ipi6_addr;
        }
        else if (cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_HOPLIMIT &&
                 cmh->cmsg_len == CMSG_LEN(sizeof(int)))
        {
            hopLimit = *(int *)CMSG_DATA(cmh);
        }
    }

    VerifyOrExit(ifIndex == sInfraIfIndex, error = OT_ERROR_DROP);

    // We currently accept only RA & RS messages for the Border Router and it requires that
    // the hoplimit must be 255 and the source address must be a link-local address.
    VerifyOrExit(hopLimit == 255 && IN6_IS_ADDR_LINKLOCAL(&srcAddr.sin6_addr), error = OT_ERROR_DROP);

    otPlatInfraIfRecvIcmp6Nd(aInstance, ifIndex, reinterpret_cast<otIp6Address *>(&srcAddr.sin6_addr), buffer,
                             bufferLength);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogDebgPlat("failed to handle ICMPv6 message: %s", otThreadErrorToString(error));
    }
}

void platformInfraIfProcess(otInstance *aInstance, const fd_set &aReadFdSet)
{
    VerifyOrExit(sInfraIfIcmp6Socket != -1);
    VerifyOrExit(sNetLinkSocket != -1);

    if (FD_ISSET(sInfraIfIcmp6Socket, &aReadFdSet))
    {
        ReceiveIcmp6Message(aInstance);
    }

    if (FD_ISSET(sNetLinkSocket, &aReadFdSet))
    {
        ReceiveNetLinkMessage(aInstance);
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
