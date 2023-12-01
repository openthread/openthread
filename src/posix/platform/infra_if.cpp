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

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
// clang-format off
#include <netinet/in.h>
#include <netinet/icmp6.h>
// clang-format on
#include <signal.h>
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
#include "posix/platform/infra_if.hpp"

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
    bool            ret     = false;
    struct ifaddrs *ifAddrs = nullptr;

    VerifyOrDie(getifaddrs(&ifAddrs) != -1, OT_EXIT_ERROR_ERRNO);

    for (struct ifaddrs *addr = ifAddrs; addr != nullptr; addr = addr->ifa_next)
    {
        struct sockaddr_in6 *ip6Addr;

        if (if_nametoindex(addr->ifa_name) != aInfraIfIndex || addr->ifa_addr == nullptr ||
            addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        ip6Addr = reinterpret_cast<sockaddr_in6 *>(addr->ifa_addr);
        if (memcmp(&ip6Addr->sin6_addr, aAddress, sizeof(*aAddress)) == 0)
        {
            ExitNow(ret = true);
        }
    }

exit:
    freeifaddrs(ifAddrs);
    return ret;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t      *aBuffer,
                                 uint16_t            aBufferLength)
{
    return ot::Posix::InfraNetif::Get().SendIcmp6Nd(aInfraIfIndex, *aDestAddress, aBuffer, aBufferLength);
}
#endif

otError otPlatInfraIfDiscoverNat64Prefix(uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

#if OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
    return ot::Posix::InfraNetif::Get().DiscoverNat64Prefix(aInfraIfIndex);
#else
    return OT_ERROR_DROP;
#endif
}

bool otSysInfraIfIsRunning(void) { return ot::Posix::InfraNetif::Get().IsRunning(); }

const char *otSysGetInfraNetifName(void) { return ot::Posix::InfraNetif::Get().GetNetifName(); }

uint32_t otSysGetInfraNetifIndex(void) { return ot::Posix::InfraNetif::Get().GetNetifIndex(); }

uint32_t otSysGetInfraNetifFlags(void) { return ot::Posix::InfraNetif::Get().GetFlags(); }

void otSysCountInfraNetifAddresses(otSysInfraNetIfAddressCounters *aAddressCounters)
{
    ot::Posix::InfraNetif::Get().CountAddresses(*aAddressCounters);
}

namespace ot {
namespace Posix {

int InfraNetif::CreateIcmp6Socket(const char *aInfraIfName)
{
    int                 sock;
    int                 rval;
    struct icmp6_filter filter;
    const int           kEnable             = 1;
    const int           kIpv6ChecksumOffset = 2;
    const int           kHopLimit           = 255;

    // Initializes the ICMPv6 socket.
    sock = SocketWithCloseExec(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, kSocketBlock);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    // Only accept Router Advertisements, Router Solicitations and Neighbor Advertisements.
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);
    ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filter);

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

#ifdef __linux__
    rval = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, aInfraIfName, strlen(aInfraIfName));
#else  // __NetBSD__ || __FreeBSD__ || __APPLE__
    rval = setsockopt(mInfraIfIcmp6Socket, IPPROTO_IPV6, IPV6_BOUND_IF, &mInfraIfIndex, sizeof(mInfraIfIndex));
#endif // __linux__
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    return sock;
}

bool IsAddressLinkLocal(const in6_addr &aAddress)
{
    return ((aAddress.s6_addr[0] & 0xff) == 0xfe) && ((aAddress.s6_addr[1] & 0xc0) == 0x80);
}

bool IsAddressUniqueLocal(const in6_addr &aAddress) { return (aAddress.s6_addr[0] & 0xfe) == 0xfc; }

bool IsAddressGlobalUnicast(const in6_addr &aAddress) { return (aAddress.s6_addr[0] & 0xe0) == 0x20; }

// Create a net-link socket that subscribes to link & addresses events.
int CreateNetLinkSocket(void)
{
    int                sock;
    int                rval;
    struct sockaddr_nl addr;

    sock = SocketWithCloseExec(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE, kSocketBlock);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV6_IFADDR;

    rval = bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    VerifyOrDie(rval == 0, OT_EXIT_ERROR_ERRNO);

    return sock;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
otError InfraNetif::SendIcmp6Nd(uint32_t            aInfraIfIndex,
                                const otIp6Address &aDestAddress,
                                const uint8_t      *aBuffer,
                                uint16_t            aBufferLength)
{
    otError error = OT_ERROR_NONE;

    struct iovec        iov;
    struct in6_pktinfo *packetInfo;

    int                 hopLimit = 255;
    uint8_t             cmsgBuffer[CMSG_SPACE(sizeof(*packetInfo)) + CMSG_SPACE(sizeof(hopLimit))];
    struct msghdr       msgHeader;
    struct cmsghdr     *cmsgPointer;
    ssize_t             rval;
    struct sockaddr_in6 dest;

    VerifyOrExit(mInfraIfIcmp6Socket >= 0, error = OT_ERROR_FAILED);
    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = OT_ERROR_DROP);

    memset(cmsgBuffer, 0, sizeof(cmsgBuffer));

    // Send the message
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    memcpy(&dest.sin6_addr, &aDestAddress, sizeof(aDestAddress));
    if (IN6_IS_ADDR_LINKLOCAL(&dest.sin6_addr) || IN6_IS_ADDR_MC_LINKLOCAL(&dest.sin6_addr))
    {
        dest.sin6_scope_id = mInfraIfIndex;
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
    packetInfo->ipi6_ifindex = mInfraIfIndex;

    // Per section 6.1.2 of RFC 4861, we need to send the ICMPv6 message with IP Hop Limit 255.
    cmsgPointer             = CMSG_NXTHDR(&msgHeader, cmsgPointer);
    cmsgPointer->cmsg_level = IPPROTO_IPV6;
    cmsgPointer->cmsg_type  = IPV6_HOPLIMIT;
    cmsgPointer->cmsg_len   = CMSG_LEN(sizeof(hopLimit));
    memcpy(CMSG_DATA(cmsgPointer), &hopLimit, sizeof(hopLimit));

    rval = sendmsg(mInfraIfIcmp6Socket, &msgHeader, 0);
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
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

bool InfraNetif::IsRunning(void) const
{
    return mInfraIfIndex ? ((GetFlags() & IFF_RUNNING) && HasLinkLocalAddress()) : false;
}

uint32_t InfraNetif::GetFlags(void) const
{
    int          sock;
    struct ifreq ifReq;
    uint32_t     flags = 0;

    OT_ASSERT(mInfraIfIndex != 0);

    sock = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_IP, kSocketBlock);
    VerifyOrDie(sock != -1, OT_EXIT_ERROR_ERRNO);

    memset(&ifReq, 0, sizeof(ifReq));
    static_assert(sizeof(ifReq.ifr_name) >= sizeof(mInfraIfName), "mInfraIfName is not of appropriate size.");
    strcpy(ifReq.ifr_name, mInfraIfName);

    if (ioctl(sock, SIOCGIFFLAGS, &ifReq) == -1)
    {
#if OPENTHREAD_POSIX_CONFIG_EXIT_ON_INFRA_NETIF_LOST_ENABLE
        otLogCritPlat("The infra link %s may be lost. Exiting.", mInfraIfName);
        DieNow(OT_EXIT_ERROR_ERRNO);
#endif
        ExitNow();
    }
    flags = static_cast<uint32_t>(ifReq.ifr_flags);

exit:
    close(sock);

    return flags;
}

void InfraNetif::CountAddresses(otSysInfraNetIfAddressCounters &aAddressCounters) const
{
    struct ifaddrs *ifAddrs = nullptr;

    aAddressCounters.mLinkLocalAddresses     = 0;
    aAddressCounters.mUniqueLocalAddresses   = 0;
    aAddressCounters.mGlobalUnicastAddresses = 0;

    if (getifaddrs(&ifAddrs) < 0)
    {
        otLogWarnPlat("failed to get netif addresses: %s", strerror(errno));
        ExitNow();
    }

    for (struct ifaddrs *addr = ifAddrs; addr != nullptr; addr = addr->ifa_next)
    {
        in6_addr *in6Addr;

        if (strncmp(addr->ifa_name, mInfraIfName, sizeof(mInfraIfName)) != 0 || addr->ifa_addr == nullptr ||
            addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        in6Addr = &(reinterpret_cast<sockaddr_in6 *>(addr->ifa_addr)->sin6_addr);
        aAddressCounters.mLinkLocalAddresses += IsAddressLinkLocal(*in6Addr);
        aAddressCounters.mUniqueLocalAddresses += IsAddressUniqueLocal(*in6Addr);
        aAddressCounters.mGlobalUnicastAddresses += IsAddressGlobalUnicast(*in6Addr);
    }

    freeifaddrs(ifAddrs);

exit:
    return;
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
void InfraNetif::HandleBackboneStateChange(otInstance *aInstance, otChangedFlags aFlags)
{
    OT_ASSERT(gInstance == aInstance);

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFlags);

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    mMulticastRoutingManager.HandleStateChange(aInstance, aFlags);
#endif
}
#endif

bool InfraNetif::HasLinkLocalAddress(void) const
{
    bool            hasLla  = false;
    struct ifaddrs *ifAddrs = nullptr;

    if (getifaddrs(&ifAddrs) < 0)
    {
        otLogCritPlat("failed to get netif addresses: %s", strerror(errno));
        DieNow(OT_EXIT_ERROR_ERRNO);
    }

    for (struct ifaddrs *addr = ifAddrs; addr != nullptr; addr = addr->ifa_next)
    {
        struct sockaddr_in6 *ip6Addr;

        if (strncmp(addr->ifa_name, mInfraIfName, sizeof(mInfraIfName)) != 0 || addr->ifa_addr == nullptr ||
            addr->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        ip6Addr = reinterpret_cast<sockaddr_in6 *>(addr->ifa_addr);
        if (IN6_IS_ADDR_LINKLOCAL(&ip6Addr->sin6_addr))
        {
            hasLla = true;
            break;
        }
    }

    freeifaddrs(ifAddrs);
    return hasLla;
}

void InfraNetif::Init(void) { mNetLinkSocket = CreateNetLinkSocket(); }

void InfraNetif::SetInfraNetif(const char *aIfName, int aIcmp6Socket)
{
    uint32_t ifIndex = 0;

    OT_UNUSED_VARIABLE(aIcmp6Socket);

    OT_ASSERT(gInstance != nullptr);
    VerifyOrDie(mNetLinkSocket != -1, OT_EXIT_INVALID_STATE);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    SetInfraNetifIcmp6SocketForBorderRouting(aIcmp6Socket);
#endif
#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    VerifyOrDie(!mMulticastRoutingManager.IsEnabled(), OT_EXIT_INVALID_STATE);
#endif

    if (aIfName == nullptr || aIfName[0] == '\0')
    {
        otLogWarnPlat("Border Routing/Backbone Router feature is disabled: infra interface is missing");
        ExitNow();
    }

    VerifyOrDie(strnlen(aIfName, sizeof(mInfraIfName)) <= sizeof(mInfraIfName) - 1, OT_EXIT_INVALID_ARGUMENTS);
    strcpy(mInfraIfName, aIfName);

    // Initializes the infra interface.
    ifIndex = if_nametoindex(aIfName);
    if (ifIndex == 0)
    {
        otLogCritPlat("Failed to get the index for infra interface %s", aIfName);
        DieNow(OT_EXIT_INVALID_ARGUMENTS);
    }

    mInfraIfIndex = ifIndex;

exit:
    return;
}

void InfraNetif::SetUp(void)
{
    OT_ASSERT(gInstance != nullptr);
    VerifyOrExit(mNetLinkSocket != -1);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    SuccessOrDie(otBorderRoutingInit(gInstance, mInfraIfIndex, otSysInfraIfIsRunning()));
    SuccessOrDie(otBorderRoutingSetEnabled(gInstance, /* aEnabled */ true));
#endif

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    mMulticastRoutingManager.SetUp();
#endif

    Mainloop::Manager::Get().Add(*this);
exit:
    return;
}

void InfraNetif::TearDown(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    IgnoreError(otBorderRoutingSetEnabled(gInstance, false));
#endif

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    mMulticastRoutingManager.TearDown();
#endif

    Mainloop::Manager::Get().Remove(*this);
}

void InfraNetif::Deinit(void)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    if (mInfraIfIcmp6Socket != -1)
    {
        close(mInfraIfIcmp6Socket);
        mInfraIfIcmp6Socket = -1;
    }
#endif

    if (mNetLinkSocket != -1)
    {
        close(mNetLinkSocket);
        mNetLinkSocket = -1;
    }

    mInfraIfName[0] = '\0';
    mInfraIfIndex   = 0;
}

void InfraNetif::Update(otSysMainloopContext &aContext)
{
    VerifyOrExit(mNetLinkSocket != -1);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    VerifyOrExit(mInfraIfIcmp6Socket != -1);

    FD_SET(mInfraIfIcmp6Socket, &aContext.mReadFdSet);
    aContext.mMaxFd = OT_MAX(aContext.mMaxFd, mInfraIfIcmp6Socket);
#endif

    FD_SET(mNetLinkSocket, &aContext.mReadFdSet);
    aContext.mMaxFd = OT_MAX(aContext.mMaxFd, mNetLinkSocket);

exit:
    return;
}

void InfraNetif::ReceiveNetLinkMessage(void)
{
    const size_t kMaxNetLinkBufSize = 8192;
    ssize_t      len;
    union
    {
        nlmsghdr mHeader;
        uint8_t  mBuffer[kMaxNetLinkBufSize];
    } msgBuffer;

    len = recv(mNetLinkSocket, msgBuffer.mBuffer, sizeof(msgBuffer.mBuffer), 0);
    if (len < 0)
    {
        otLogCritPlat("Failed to receive netlink message: %s", strerror(errno));
        ExitNow();
    }

    for (struct nlmsghdr *header = &msgBuffer.mHeader; NLMSG_OK(header, static_cast<size_t>(len));
         header                  = NLMSG_NEXT(header, len))
    {
        switch (header->nlmsg_type)
        {
        // There are no effective netlink message types to get us notified
        // of interface RUNNING state changes. But addresses events are
        // usually associated with interface state changes.
        case RTM_NEWADDR:
        case RTM_DELADDR:
        case RTM_NEWLINK:
        case RTM_DELLINK:
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
            SuccessOrDie(otPlatInfraIfStateChanged(gInstance, mInfraIfIndex, otSysInfraIfIsRunning()));
#endif
            break;
        case NLMSG_ERROR:
        {
            struct nlmsgerr *errMsg = reinterpret_cast<struct nlmsgerr *>(NLMSG_DATA(header));

            OT_UNUSED_VARIABLE(errMsg);
            otLogWarnPlat("netlink NLMSG_ERROR response: seq=%u, error=%d", header->nlmsg_seq, errMsg->error);
            break;
        }
        default:
            break;
        }
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void InfraNetif::ReceiveIcmp6Message(void)
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

    rval = recvmsg(mInfraIfIcmp6Socket, &msg, 0);
    if (rval < 0)
    {
        otLogWarnPlat("Failed to receive ICMPv6 message: %s", strerror(errno));
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

    VerifyOrExit(ifIndex == mInfraIfIndex, error = OT_ERROR_DROP);

    // We currently accept only RA & RS messages for the Border Router and it requires that
    // the hoplimit must be 255 and the source address must be a link-local address.
    VerifyOrExit(hopLimit == 255 && IN6_IS_ADDR_LINKLOCAL(&srcAddr.sin6_addr), error = OT_ERROR_DROP);

    otPlatInfraIfRecvIcmp6Nd(gInstance, ifIndex, reinterpret_cast<otIp6Address *>(&srcAddr.sin6_addr), buffer,
                             bufferLength);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogDebgPlat("Failed to handle ICMPv6 message: %s", otThreadErrorToString(error));
    }
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE
const char         InfraNetif::kWellKnownIpv4OnlyName[]   = "ipv4only.arpa";
const otIp4Address InfraNetif::kWellKnownIpv4OnlyAddress1 = {{{192, 0, 0, 170}}};
const otIp4Address InfraNetif::kWellKnownIpv4OnlyAddress2 = {{{192, 0, 0, 171}}};
const uint8_t      InfraNetif::kValidNat64PrefixLength[]  = {96, 64, 56, 48, 40, 32};

void InfraNetif::DiscoverNat64PrefixDone(union sigval sv)
{
    struct gaicb    *req = (struct gaicb *)sv.sival_ptr;
    struct addrinfo *res = (struct addrinfo *)req->ar_result;

    otIp6Prefix prefix = {};

    VerifyOrExit((char *)req->ar_name == kWellKnownIpv4OnlyName);

    otLogInfoPlat("Handling host address response for %s", kWellKnownIpv4OnlyName);

    // We extract the first valid NAT64 prefix from the address look-up response.
    for (struct addrinfo *rp = res; rp != NULL && prefix.mLength == 0; rp = rp->ai_next)
    {
        struct sockaddr_in6 *ip6Addr;
        otIp6Address         ip6Address;

        if (rp->ai_family != AF_INET6)
        {
            continue;
        }

        ip6Addr = reinterpret_cast<sockaddr_in6 *>(rp->ai_addr);
        memcpy(&ip6Address.mFields.m8, &ip6Addr->sin6_addr.s6_addr, OT_IP6_ADDRESS_SIZE);
        for (uint8_t length : kValidNat64PrefixLength)
        {
            otIp4Address ip4Address;

            otIp4ExtractFromIp6Address(length, &ip6Address, &ip4Address);
            if (otIp4IsAddressEqual(&ip4Address, &kWellKnownIpv4OnlyAddress1) ||
                otIp4IsAddressEqual(&ip4Address, &kWellKnownIpv4OnlyAddress2))
            {
                // We check that the well-known IPv4 address is present only once in the IPv6 address.
                // In case another instance of the value is found for another prefix length, we ignore this address
                // and search for the other well-known IPv4 address (per RFC 7050 section 3).
                bool foundDuplicate = false;

                for (uint8_t dupLength : kValidNat64PrefixLength)
                {
                    otIp4Address dupIp4Address;

                    if (dupLength == length)
                    {
                        continue;
                    }

                    otIp4ExtractFromIp6Address(dupLength, &ip6Address, &dupIp4Address);
                    if (otIp4IsAddressEqual(&dupIp4Address, &ip4Address))
                    {
                        foundDuplicate = true;
                        break;
                    }
                }

                if (!foundDuplicate)
                {
                    otIp6GetPrefix(&ip6Address, length, &prefix);
                    break;
                }
            }

            if (prefix.mLength != 0)
            {
                break;
            }
        }
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    otPlatInfraIfDiscoverNat64PrefixDone(gInstance, Get().mInfraIfIndex, &prefix);
#endif

exit:
    freeaddrinfo(res);
    freeaddrinfo((struct addrinfo *)req->ar_request);
    free(req);
}

otError InfraNetif::DiscoverNat64Prefix(uint32_t aInfraIfIndex)
{
    otError          error   = OT_ERROR_NONE;
    struct addrinfo *hints   = nullptr;
    struct gaicb    *reqs[1] = {nullptr};
    struct sigevent  sig;
    int              status;

    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = OT_ERROR_DROP);
    hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
    VerifyOrExit(hints != nullptr, error = OT_ERROR_NO_BUFS);
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family   = AF_INET6;
    hints->ai_socktype = SOCK_STREAM;

    reqs[0] = (struct gaicb *)malloc(sizeof(struct gaicb));
    VerifyOrExit(reqs[0] != nullptr, error = OT_ERROR_NO_BUFS);
    memset(reqs[0], 0, sizeof(struct gaicb));
    reqs[0]->ar_name    = kWellKnownIpv4OnlyName;
    reqs[0]->ar_request = hints;

    memset(&sig, 0, sizeof(struct sigevent));
    sig.sigev_notify          = SIGEV_THREAD;
    sig.sigev_value.sival_ptr = reqs[0];
    sig.sigev_notify_function = &InfraNetif::DiscoverNat64PrefixDone;

    status = getaddrinfo_a(GAI_NOWAIT, reqs, 1, &sig);

    if (status != 0)
    {
        otLogNotePlat("getaddrinfo_a failed: %s", gai_strerror(status));
        ExitNow(error = OT_ERROR_FAILED);
    }
    otLogInfoPlat("getaddrinfo_a requested for %s", kWellKnownIpv4OnlyName);
exit:
    if (error != OT_ERROR_NONE)
    {
        if (hints)
        {
            freeaddrinfo(hints);
        }
        free(reqs[0]);
    }
    return error;
}
#endif // OPENTHREAD_POSIX_CONFIG_NAT64_AIL_PREFIX_ENABLE

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
void InfraNetif::SetInfraNetifIcmp6SocketForBorderRouting(int aIcmp6Socket)
{
    otBorderRoutingState state = otBorderRoutingGetState(gInstance);

    VerifyOrDie(state == OT_BORDER_ROUTING_STATE_UNINITIALIZED || state == OT_BORDER_ROUTING_STATE_DISABLED,
                OT_EXIT_INVALID_STATE);

    if (mInfraIfIcmp6Socket != -1)
    {
        close(mInfraIfIcmp6Socket);
    }
    mInfraIfIcmp6Socket = aIcmp6Socket;
}
#endif

void InfraNetif::Process(const otSysMainloopContext &aContext)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    VerifyOrExit(mInfraIfIcmp6Socket != -1);
#endif
    VerifyOrExit(mNetLinkSocket != -1);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    if (FD_ISSET(mInfraIfIcmp6Socket, &aContext.mReadFdSet))
    {
        ReceiveIcmp6Message();
    }
#endif

    if (FD_ISSET(mNetLinkSocket, &aContext.mReadFdSet))
    {
        ReceiveNetLinkMessage();
    }

exit:
    return;
}

InfraNetif &InfraNetif::Get(void)
{
    static InfraNetif sInstance;

    return sInstance;
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
